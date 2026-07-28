#include "udon/runtime.hpp"

#include <stdexcept>
#include <utility>

namespace udon {

MatchSession::MatchSession(
    const MatchConfig& config,
    RiskPolicy policy,
    DeadlineCalibration deadlineCalibration)
    : config_(config), engine_(config_, std::move(policy), std::move(deadlineCalibration)) {}

std::vector<RoleAssignment> MatchSession::select_roles_until(
    std::chrono::milliseconds available,
    std::int32_t beamWidth) const {
    return engine_.select_roles_until(available, beamWidth);
}

SessionDecision MatchSession::on_authoritative_state(
    const DayState& state,
    const MatchLedger& ledger,
    std::chrono::system_clock::time_point receivedAt) {
    if (pendingDecision_.has_value()) {
        throw std::logic_error("an authoritative acknowledgement is required before planning another state");
    }
    DecisionResult decision = engine_.solve_day_until(state, ledger, receivedAt);
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

std::int32_t MatchSession::precompute_until(std::chrono::milliseconds available) {
    if (available.count() < 0) {
        throw std::invalid_argument("precompute budget cannot be negative");
    }
    if (!acknowledgedDecision_.has_value() || !acknowledgedState_.has_value() || !acknowledgedLedger_.has_value()) {
        throw std::logic_error("an acknowledged decision is required before contingency precompute");
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

} 
