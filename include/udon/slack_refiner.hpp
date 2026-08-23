#pragma once

#include <chrono>
#include <cstdint>

#include "udon/graph.hpp"
#include "udon/simulator.hpp"
#include "udon/types.hpp"
#include "udon/validator.hpp"

namespace udon {

struct ProtectedSlackDiagnostics {
    std::int64_t waitAnchors = 0;
    std::int64_t eligibleWaitAnchors = 0;
    std::int64_t routePairs = 0;
    std::int64_t generatedPlans = 0;
    std::int64_t validPlans = 0;
    std::int64_t liftablePlans = 0;
    std::int64_t sparseRoutes = 0;
    std::int64_t strictTerminalImprovements = 0;
    std::int64_t terminalSparseRounds = 0;
    std::int64_t terminalPairAcceptances = 0;
    bool deadlineReached = false;
    bool terminalSparse = false;
    bool sparseFailure = false;
};

struct ProtectedSlackResult {
    DayPlan plan;
    SimulationResult simulation;
    OfficialScore scoreAfterToday;
    OfficialScore firstRoundScore;
    ProtectedSlackDiagnostics diagnostics;
    bool improved = false;
    AgentIndex witnessAgent = kInvalidAgent;
    CellId witnessAnchor = kInvalidCell;
    CellId witnessSpot = kInvalidCell;
    std::int32_t witnessDuration = 0;
    std::int32_t witnessTravelSteps = 0;
    std::int32_t witnessParentFuel = 0;
    std::int32_t witnessCandidateFuel = 0;
};

[[nodiscard]] bool protected_slack_transition_dominates(
    const SimulationResult& baseline,
    const SimulationResult& actual);

[[nodiscard]] bool protected_slack_agents_dominate(
    const std::vector<AgentState>& baseline,
    const std::vector<AgentState>& actual);

[[nodiscard]] bool protected_slack_ledger_dominates(
    const MatchLedger& baseline,
    const MatchLedger& actual);

class ProtectedSlackRefiner {
public:
    explicit ProtectedSlackRefiner(const MatchConfig& config);

    // Accepted SCORE-TERMINAL-PAIR-EXCHANGE-207: enables the pair-exchange
    // phase after the one-agent terminal ascent reaches its fixed point.
    // Production (btc_main) enables it; the default stays off so research
    // harnesses keep a byte-identical 191 parent for causal A/B runs.
    bool enableTerminalPairExchange = false;

    [[nodiscard]] ProtectedSlackResult refine_wait_detours(
        const DayState& state,
        const MatchLedger& ledger,
        const DayPlan& incumbentPlan,
        std::chrono::steady_clock::time_point deadline) const;

    [[nodiscard]] ProtectedSlackResult refine_wait_detours(
        const DayState& state,
        const MatchLedger& ledger,
        const DayPlan& incumbentPlan,
        const SimulationResult& incumbentSimulation,
        std::chrono::steady_clock::time_point deadline) const;

    [[nodiscard]] ProtectedSlackResult refine_terminal_sparse(
        const DayState& state,
        const MatchLedger& ledger,
        const DayPlan& incumbentPlan,
        const SimulationResult& incumbentSimulation,
        std::chrono::steady_clock::time_point deadline) const;

private:
    const MatchConfig& config_;
    ParetoRouter router_;
    ExactStepSimulator simulator_;
    IndependentDayValidator validator_;
};

} // namespace udon
