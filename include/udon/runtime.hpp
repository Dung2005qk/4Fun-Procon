#pragma once

#include <chrono>
#include <optional>
#include <vector>

#include "udon/audit.hpp"
#include "udon/decision.hpp"

namespace udon {

struct SessionDecision {
    DecisionResult decision;
    bool maySubmit = false;
    JsonValue replay;
};

struct PostAckWork {
    std::int32_t cachedContingencies = 0;
    std::int32_t completedProofs = 0;
};

class MatchSession {
public:
    explicit MatchSession(
        const MatchConfig& config,
        RiskPolicy policy = {},
        DeadlineCalibration deadlineCalibration = {},
        std::int32_t harvestExtensionMode = 6,
        std::int32_t futureHarvestExtensionMode = -1);

    [[nodiscard]] std::vector<RoleAssignment> select_roles_until(
        std::chrono::milliseconds available,
        std::int32_t beamWidth = 3) const;

    [[nodiscard]] SessionDecision on_authoritative_state(
        const DayState& state,
        const MatchLedger& ledger,
        std::chrono::system_clock::time_point receivedAt = std::chrono::system_clock::now());

    [[nodiscard]] SessionDecision on_authoritative_state_for(
        const DayState& state,
        const MatchLedger& ledger,
        std::chrono::milliseconds available);

    [[nodiscard]] PostAckWork acknowledge_submitted(
        std::chrono::milliseconds responseTime,
        std::chrono::milliseconds postAckBudget = std::chrono::milliseconds{0});

    void reject_pending_submission();

    [[nodiscard]] std::int32_t precompute_until(std::chrono::milliseconds available);

    [[nodiscard]] std::int32_t prove_until(std::chrono::milliseconds available);

    [[nodiscard]] bool has_pending_submission() const;
    [[nodiscard]] const ResponseLedger& response_ledger() const;

private:
    MatchConfig config_;
    UdonShieldEngine engine_;
    std::optional<DecisionResult> pendingDecision_;
    std::optional<DayState> pendingState_;
    std::optional<MatchLedger> pendingLedger_;
    std::optional<DecisionResult> acknowledgedDecision_;
    std::optional<DayState> acknowledgedState_;
    std::optional<MatchLedger> acknowledgedLedger_;
};

} 
