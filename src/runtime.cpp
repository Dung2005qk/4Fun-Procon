#include "udon/runtime.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace udon {

namespace {

[[nodiscard]] std::chrono::milliseconds bounded_post_ack_budget(
    std::chrono::milliseconds requested,
    std::chrono::milliseconds remaining) {
    return std::min(competition_compute_budget(requested), remaining);
}

} // namespace

MatchSession::MatchSession(
    const MatchConfig& config,
    RiskPolicy policy,
    DeadlineCalibration deadlineCalibration,
    std::int32_t harvestExtensionMode,
    std::int32_t futureHarvestExtensionMode)
    : config_(config),
      engine_(
          config_,
          std::move(policy),
          std::move(deadlineCalibration),
          RoutePoolSearch::SinglePass,
          harvestExtensionMode,
          true,
          futureHarvestExtensionMode) {}

std::vector<RoleAssignment> MatchSession::select_roles_until(
    std::chrono::milliseconds available,
    std::int32_t beamWidth) const {
    return engine_.select_roles_until(available, beamWidth);
}

RoleSelectionDiagnostics MatchSession::select_roles_until_with_diagnostics(
    std::chrono::milliseconds available,
    std::int32_t beamWidth) const {
    return engine_.select_roles_until_with_diagnostics(available, beamWidth);
}

void MatchSession::set_short_horizon_role_fallback(bool enabled) {
    engine_.set_short_horizon_role_fallback(enabled);
}

SessionDecision MatchSession::on_authoritative_state(
    const DayState& state,
    const MatchLedger& ledger,
    std::chrono::system_clock::time_point receivedAt) {
    const std::chrono::system_clock::time_point endsAt =
        std::chrono::system_clock::time_point{std::chrono::seconds{state.endsAt}};
    return on_authoritative_state_for(
        state,
        ledger,
        std::max(
            std::chrono::milliseconds{0},
            std::chrono::duration_cast<std::chrono::milliseconds>(endsAt - receivedAt)));
}

SessionDecision MatchSession::on_authoritative_state_for(
    const DayState& state,
    const MatchLedger& ledger,
    std::chrono::milliseconds available) {
    if (pendingDecision_.has_value()) {
        throw std::logic_error("an authoritative acknowledgement is required before planning another state");
    }
    DecisionResult decision = engine_.solve_day(
        state,
        ledger,
        std::max(std::chrono::milliseconds{0}, available));
    const bool maySubmit = engine_.may_submit(decision);
    SessionDecision result;
    result.replay = serialize_decision_replay(config_, state, ledger, decision);
    result.decision = decision;
    result.maySubmit = maySubmit;
    if (maySubmit) {
        pendingDecision_ = std::move(decision);
        pendingState_ = state;
        pendingLedger_ = ledger;
    }
    return result;
}

PostAckWork MatchSession::acknowledge_submitted(
    std::chrono::milliseconds responseTime,
    std::chrono::milliseconds postAckBudget) {
    if (!pendingDecision_.has_value()) {
        throw std::logic_error("cannot acknowledge without a pending submission");
    }
    if (postAckBudget.count() < 0) {
        throw std::invalid_argument("post-ACK budget cannot be negative");
    }
    engine_.record_submitted(*pendingDecision_, responseTime);
    acknowledgedDecision_ = *pendingDecision_;
    acknowledgedState_ = *pendingState_;
    acknowledgedLedger_ = *pendingLedger_;
    pendingDecision_.reset();
    pendingState_.reset();
    pendingLedger_.reset();
    PostAckWork work;
    postAckBudget = bounded_post_ack_budget(
        postAckBudget,
        engine_.remaining_post_ack_compute_budget());
    if (postAckBudget.count() == 0 ||
        acknowledgedDecision_->dayNumber >= config_.day_count()) {
        return work;
    }
    const std::chrono::steady_clock::time_point started =
        std::chrono::steady_clock::now();
    const std::chrono::steady_clock::time_point deadline =
        started + postAckBudget;
    const std::chrono::steady_clock::time_point precomputeDeadline =
        started + postAckBudget / 3;
    static_cast<void>(engine_.precompute_next_day_contingencies(
        *acknowledgedState_,
        *acknowledgedLedger_,
        *acknowledgedDecision_,
        precomputeDeadline));
    work.cachedContingencies = static_cast<std::int32_t>(
        engine_.response_ledger().cachedContingencies.size());
    if (std::chrono::steady_clock::now() < deadline) {
        work.completedProofs = engine_.prove_remaining_horizon(
            *acknowledgedState_,
            *acknowledgedLedger_,
            *acknowledgedDecision_,
            deadline);
    }
    return work;
}

void MatchSession::reject_pending_submission() {
    if (!pendingDecision_.has_value()) {
        throw std::logic_error("cannot reject without a pending submission");
    }
    pendingDecision_.reset();
    pendingState_.reset();
    pendingLedger_.reset();
}

void MatchSession::record_applied_transition(
    const DayState& state,
    const SimulationResult& simulation) {
    if (pendingDecision_.has_value()) {
        throw std::logic_error("a pending submission must be rejected before recording an external transition");
    }
    engine_.record_applied_transition(state, simulation);
    acknowledgedDecision_.reset();
    acknowledgedState_.reset();
    acknowledgedLedger_.reset();
}

void MatchSession::restore_response_artifacts(
    std::vector<ResponseLedger::CachedContingency> cachedContingencies,
    std::vector<ResponseLedger::StrongProofRecord> strongProofs) {
    if (pendingDecision_.has_value()) {
        throw std::logic_error("cannot restore response artifacts with a pending submission");
    }
    engine_.restore_response_artifacts(
        std::move(cachedContingencies),
        std::move(strongProofs));
}

std::int32_t MatchSession::precompute_until(std::chrono::milliseconds available) {
    if (available.count() < 0) {
        throw std::invalid_argument("precompute budget cannot be negative");
    }
    if (!acknowledgedDecision_.has_value() || !acknowledgedState_.has_value() || !acknowledgedLedger_.has_value()) {
        throw std::logic_error("an acknowledged decision is required before contingency precompute");
    }
    available = bounded_post_ack_budget(
        available,
        engine_.remaining_post_ack_compute_budget());
    if (available.count() == 0) {
        return 0;
    }
    return engine_.precompute_next_day_contingencies(
        *acknowledgedState_,
        *acknowledgedLedger_,
        *acknowledgedDecision_,
        std::chrono::steady_clock::now() + available);
}

std::int32_t MatchSession::prove_until(std::chrono::milliseconds available) {
    if (available.count() < 0) {
        throw std::invalid_argument("proof budget cannot be negative");
    }
    if (!acknowledgedDecision_.has_value() || !acknowledgedState_.has_value() || !acknowledgedLedger_.has_value()) {
        throw std::logic_error("an acknowledged decision is required before strong proof search");
    }
    available = bounded_post_ack_budget(
        available,
        engine_.remaining_post_ack_compute_budget());
    if (available.count() == 0) {
        return 0;
    }
    return engine_.prove_remaining_horizon(
        *acknowledgedState_,
        *acknowledgedLedger_,
        *acknowledgedDecision_,
        std::chrono::steady_clock::now() + available);
}

bool MatchSession::has_pending_submission() const {
    return pendingDecision_.has_value();
}

const ResponseLedger& MatchSession::response_ledger() const {
    return engine_.response_ledger();
}

const std::vector<std::int32_t>& MatchSession::previous_own_footprint() const {
    return engine_.previous_own_footprint();
}

std::chrono::milliseconds MatchSession::remaining_post_ack_compute_budget() const {
    return engine_.remaining_post_ack_compute_budget();
}

} 
