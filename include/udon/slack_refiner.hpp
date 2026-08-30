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
    std::int64_t terminalMarginalRoutes = 0;
    std::int64_t terminalMarginalGeneratedPlans = 0;
    std::int64_t terminalMarginalValidPlans = 0;
    std::int64_t terminalMarginalAcceptances = 0;
    std::int64_t terminalMarginalRounds = 0;
    bool terminalMarginalDeadline = false;
    bool terminalMarginalFailure = false;
    std::int64_t middayRoutes = 0;
    std::int64_t middayGeneratedPlans = 0;
    std::int64_t middayValidPlans = 0;
    std::int64_t middayChainAcceptances = 0;
    std::int64_t middayPairAcceptances = 0;
    std::int64_t middayRounds = 0;
    std::int64_t middayTargetRoutes = 0;
    std::int64_t middayTargetGeneratedPlans = 0;
    std::int64_t middayTargetValidPlans = 0;
    std::int64_t middayTargetAcceptances = 0;
    std::int64_t middayTargetRounds = 0;
    bool deadlineReached = false;
    bool terminalSparse = false;
    bool sparseFailure = false;
    bool middayChain = false;
    bool middayTargetFollowup = false;
    bool middayFailure = false;
};

struct ProtectedSlackResult {
    DayPlan plan;
    SimulationResult simulation;
    OfficialScore scoreAfterToday;
    OfficialScore firstRoundScore;
    OfficialScore canonicalTerminalScore;
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

// Nonterminal protected continuations must preserve every ledger component so
// their state remains safe for later days. On the terminal day no future state
// exists, so the authoritative relation is official lexicographic
// non-regression.
[[nodiscard]] bool protected_slack_ledger_relation_for_day(
    const MatchLedger& baseline,
    const MatchLedger& actual,
    bool terminalDay);

class ProtectedSlackRefiner {
public:
    explicit ProtectedSlackRefiner(const MatchConfig& config);

    // Accepted SCORE-TERMINAL-PAIR-EXCHANGE-207: enables the pair-exchange
    // phase after the one-agent terminal ascent reaches its fixed point.
    // Production (btc_main) enables it; the default stays off so research
    // harnesses keep a byte-identical 191 parent for causal A/B runs.
    bool enableTerminalPairExchange = false;

    // SCORE-TERMINAL-STOCK-MARGINAL-RESERVOIR-258 research switch. The
    // canonical terminal ascent and pair fixed point remain the complete
    // prefix/fallback; only remaining public time may evaluate the additive
    // stock-aware route reservoir. Production stays off until promotion.
    bool enableTerminalMarginalReservoir = false;

    // Registered SCORE-MIDDAY-CHAIN-ADOPTION-210 (research A/B only until
    // accepted): mid-day one-agent deep-chain substitution accepted solely
    // through the unchanged strict_protected_improvement certificate, so
    // every takeover preserves the day transition (equal road footprint,
    // same terminal cells, patrol fuel >=) while strictly improving the
    // official day score. Default off keeps the parent byte-identical.
    bool enableMiddayChainAdoption = false;

    // Registered SCORE-MIDDAY-PAIR-EXCHANGE-211 (research A/B only until
    // accepted): after the one-agent mid-day ascent reaches its fixed point,
    // the remaining protected budget evaluates joint two-patrol replacements
    // from the already-enumerated route pools under the same
    // strict_protected_improvement certificate. Default off keeps accepted
    // 210 byte-identical.
    bool enableMiddayPairExchange = false;

    // SCORE-MIDDAY-TARGET-FOLLOWUP-215: preserve the complete
    // accepted 210 global-pool ascent as a prefix, then use only remaining
    // protected time for a second sparse pool conditioned on each patrol's
    // already protected terminal. The switch remains explicit so the frozen
    // same-binary direct-parent lane can disable only this suffix.
    bool enableMiddayTargetTerminalFollowup = false;

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

    [[nodiscard]] ProtectedSlackResult refine_midday_chains(
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
