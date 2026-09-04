#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "udon/decision.hpp"
#include "udon/orienteering.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/slack_refiner.hpp"
#include "udon/validator.hpp"

namespace {

constexpr std::int32_t kHarnessHarvestMode = 7;
constexpr std::int32_t kHarnessFutureHarvestMode = 7;

struct SpotSpec {
    std::int32_t brand = 0;
    udon::CellId position = udon::kInvalidCell;
    std::int32_t stock = 1;
};

struct FixtureSpec {
    std::string name;
    std::string family;
    std::uint64_t seed = 0;
    std::int32_t width = 8;
    std::int32_t height = 8;
    std::vector<std::int32_t> terrain;
    std::vector<SpotSpec> spots;
    std::vector<udon::CellId> starts{8, 9, 10, 11};
    std::vector<std::int32_t> daySteps{16, 16, 16, 16};
    std::int32_t fuelLimit = 16;
    std::int32_t players = 3;
    std::int32_t busyThreshold = 3;
    std::int32_t jammedThreshold = 6;
};

struct Options {
    std::string version;
    std::string track;
    std::string suite = "general";
    std::uint64_t firstSeed = 100000;
    std::int32_t seedCount = 6;
    std::chrono::milliseconds dayBudget{1200};
    std::chrono::milliseconds roleBudget{500};
    std::string roleMode = "exhaustive";
    std::uint32_t fixedRoleMask = 0;
    std::chrono::milliseconds protectedRefineBudget{0};
    std::chrono::milliseconds protectedWaitBudget{0};
    std::chrono::milliseconds terminalSparseBudget{0};
    std::chrono::milliseconds publicWindowProbeBudget{0};
    std::chrono::milliseconds postAckBudget{0};
    std::chrono::milliseconds postAckSliceBudget{100};
    bool publicWindowApply = false;
    bool checkpointClosedLoop = false;
    std::int32_t spotCount = 0;
    std::int32_t playersOverride = 0;
    std::uint64_t sparseRouteStates = 0;
    std::uint64_t permutedTerminalProbeStates = 0;
    std::uint64_t causalPermutedTerminalApplyStates = 0;
    std::string fuelProfile = "generated";
    bool protectedWaitDetours = false;
    bool protectedWaitClosedLoop = false;
    bool terminalPairExchange = false;
    bool terminalMarginalReservoir = false;
    bool middayChainAdoption = false;
    bool middayPairExchange = false;
    bool middayTargetTerminalFollowup = false;
    bool shortRoleFallback = false;
    bool roleDetails = false;
    bool dayDetails = false;
};

struct Metrics {
    udon::OfficialScore score;
    udon::OfficialScore protectedVirtualScore;
    udon::OfficialScore checkpointClosedLoopParentScore;
    std::vector<std::int64_t> responseTimes;
    std::int64_t roleMilliseconds = 0;
    std::int64_t combinationsVisited = 0;
    std::uint32_t roleMask = 0;
    std::int32_t invalidPlans = 0;
    std::int32_t emergencyDays = 0;
    std::int32_t searchCompleteDays = 0;
    std::int32_t searchDeadlineDays = 0;
    std::vector<udon::OfficialScore> cumulativeDayScores;
    std::vector<udon::OfficialScore> protectedVirtualDayScores;
    std::vector<udon::DayScore> exactDayScores;
    std::vector<std::uint64_t> planHashes;
    std::vector<bool> deadlineDays;
    std::vector<std::int32_t> exactSupportedAgents;
    std::vector<std::uint64_t> exactSettledStates;
    std::vector<std::int32_t> exactSeedServings;
    std::vector<std::int32_t> exactLocalServings;
    std::array<std::int32_t, 4> todayOpenTierCounts{};
    std::array<std::int32_t, 4> candidateOpenTierCounts{};
    std::array<std::int32_t, 4> viabilityOpenTierCounts{};
    std::array<std::int32_t, 4> absoluteOpenTierCounts{};
    std::int32_t invalidOptimalityEnvelopes = 0;
    std::int32_t viabilityDeadlineDays = 0;
    std::vector<udon::OptimalityGapDiagnostics> optimalityGaps;
    std::int32_t protectedTerminalAttempts = 0;
    std::int32_t protectedTerminalTakeovers = 0;
    std::int32_t protectedTerminalInvalid = 0;
    udon::OfficialScore protectedTerminalParentScore;
    udon::OfficialScore protectedTerminalRefinedScore;
    std::int64_t protectedWaitPlans = 0;
    std::int64_t protectedWaitValid = 0;
    std::int64_t protectedWaitLiftable = 0;
    std::int32_t protectedWaitBestDay = 0;
    udon::OfficialScore protectedWaitParentScore;
    udon::OfficialScore protectedWaitBestScore;
    std::int32_t protectedWaitTakeovers = 0;
    std::int32_t protectedWaitDeadlineDays = 0;
    std::int32_t protectedWaitWitnessAgent = -1;
    udon::CellId protectedWaitWitnessAnchor = udon::kInvalidCell;
    udon::CellId protectedWaitWitnessSpot = udon::kInvalidCell;
    std::int32_t protectedWaitWitnessDuration = 0;
    std::int32_t protectedWaitWitnessTravelSteps = 0;
    std::int32_t protectedWaitWitnessParentFuel = 0;
    std::int32_t protectedWaitWitnessCandidateFuel = 0;
    std::uint64_t protectedWaitWitnessPlanHash = 0;
    std::int64_t terminalSparseRoutes = 0;
    std::int64_t terminalSparsePlans = 0;
    std::int64_t terminalSparseValid = 0;
    std::int64_t terminalSparseStrict = 0;
    std::int64_t terminalSparseRounds = 0;
    std::int64_t terminalPairAcceptances = 0;
    std::int32_t terminalSparseTakeovers = 0;
    std::int32_t terminalSparseDeadline = 0;
    std::int32_t terminalSparseFailure = 0;
    udon::OfficialScore terminalSparseParentScore;
    udon::OfficialScore terminalSparseFirstRoundScore;
    udon::OfficialScore terminalSparseRefinedScore;
    std::int64_t terminalMarginalRoutes = 0;
    std::int64_t terminalMarginalPlans = 0;
    std::int64_t terminalMarginalValid = 0;
    std::int64_t terminalMarginalAcceptances = 0;
    std::int64_t terminalMarginalRounds = 0;
    std::int32_t terminalMarginalDeadline = 0;
    std::int32_t terminalMarginalFailure = 0;
    udon::OfficialScore terminalMarginalParentScore;
    udon::OfficialScore terminalMarginalRefinedScore;
    std::int64_t middayRoutes = 0;
    std::int64_t middayPlans = 0;
    std::int64_t middayValid = 0;
    std::int64_t middayChainAcceptances = 0;
    std::int64_t middayPairAcceptances = 0;
    std::int64_t middayRounds = 0;
    std::int64_t middayTargetRoutes = 0;
    std::int64_t middayTargetPlans = 0;
    std::int64_t middayTargetValid = 0;
    std::int64_t middayTargetAcceptances = 0;
    std::int64_t middayTargetRounds = 0;
    std::int32_t middayTakeovers = 0;
    std::int32_t middayDeadlineDays = 0;
    std::int32_t middayFailureDays = 0;
    std::int32_t middayBestDay = 0;
    std::int32_t publicWindowProbeDays = 0;
    std::int32_t publicWindowProbeTakeovers = 0;
    std::int32_t publicWindowProbeDeadlineDays = 0;
    std::int32_t publicWindowProbeFailureDays = 0;
    std::int32_t publicWindowProbeTier1Gains = 0;
    std::int64_t cacheEligible = 0;
    std::int64_t cacheReused = 0;
    std::int64_t cacheRejected = 0;
    std::int64_t postAckCalls = 0;
    std::int64_t postAckContingencies = 0;
    std::int64_t postAckProofCalls = 0;
    std::int64_t postAckCompletedProofs = 0;
    std::int64_t postAckStrongProofRecords = 0;
    std::int32_t publicWindowProbeTier2Gains = 0;
    std::int64_t publicWindowProbeServingGain = 0;
    std::int64_t publicWindowProbeRoutes = 0;
    std::int64_t publicWindowProbePlans = 0;
    std::int64_t publicWindowProbeValid = 0;
    std::int64_t publicWindowProbeAcceptances = 0;
    std::int32_t checkpointClosedLoopTakeovers = 0;
    std::int32_t checkpointClosedLoopFailures = 0;
    std::int64_t sparseRouteRepresentatives = 0;
    std::int64_t sparseRoutes = 0;
    std::int64_t sparseValidPlans = 0;
    std::int32_t sparseTakeovers = 0;
    std::int64_t sparseServingGain = 0;
    std::int64_t permutedProbeTasks = 0;
    std::int64_t permutedProbeRoutes = 0;
    std::int64_t permutedProbeCrossPairs = 0;
    std::int64_t permutedProbeValidPlans = 0;
    std::int64_t permutedProbeCertified = 0;
    std::int32_t permutedProbeYieldDays = 0;
    std::int64_t permutedProbeLifetimeGain = 0;
    std::int64_t permutedProbeDailyGain = 0;
    std::int64_t permutedProbeServingGain = 0;
    std::int32_t permutedProbeMutationFailures = 0;
    udon::OfficialScore permutedCausalParentScore;
    std::vector<udon::OfficialScore> permutedCausalDayScores;
    std::vector<std::uint64_t> permutedCausalPlanHashes;
    std::int64_t permutedApplyTasks = 0;
    std::int64_t permutedApplyRoutes = 0;
    std::int64_t permutedApplyCrossPairs = 0;
    std::int64_t permutedApplyValidPlans = 0;
    std::int64_t permutedApplyCertified = 0;
    std::int32_t permutedApplyTakeovers = 0;
    std::int64_t permutedApplyLifetimeGain = 0;
    std::int64_t permutedApplyDailyGain = 0;
    std::int64_t permutedApplyServingGain = 0;
    std::int32_t permutedApplyDeadlineDays = 0;
    std::int32_t permutedApplyMappingFailures = 0;
    std::int32_t permutedApplyRoleFailures = 0;
    std::int32_t permutedApplyShadowStateFailures = 0;
    std::int32_t permutedApplyShadowLedgerFailures = 0;
    std::int32_t permutedApplyUncertifiedTakeovers = 0;
    std::int32_t permutedCausalNoopFailures = 0;
};

[[nodiscard]] std::uint64_t plan_hash(const udon::DayPlan& plan) {
    std::uint64_t hash = 1469598103934665603ULL;
    const auto mix = [&hash](std::uint64_t value) {
        hash ^= value;
        hash *= 1099511628211ULL;
    };
    mix(plan.actions.size());
    for (const udon::AgentPlan& actions : plan.actions) {
        mix(actions.size());
        for (const udon::PlanAction& action : actions) {
            mix(static_cast<std::uint32_t>(action.wire_value()));
        }
    }
    return hash;
}

struct SparseCurrentDayChoice {
    udon::DayPlan plan;
    udon::SimulationResult simulation;
    udon::OfficialScore scoreAfterToday;
    std::int32_t representatives = 0;
    std::int64_t routes = 0;
    std::int64_t validPlans = 0;
    bool improved = false;
};

[[nodiscard]] SparseCurrentDayChoice sparse_current_day_exchange(
    const udon::MatchConfig& config,
    const udon::DayState& state,
    const udon::MatchLedger& ledger,
    const udon::DayPlan& parentPlan,
    const udon::SimulationResult& parentSimulation,
    std::uint64_t maximumSettledStates) {
    SparseCurrentDayChoice result;
    result.plan = parentPlan;
    result.simulation = parentSimulation;
    result.scoreAfterToday = udon::OfficialScore::after_day(
        ledger,
        parentSimulation.score);
    if (maximumSettledStates == 0U || config.spots.size() > 32U) {
        return result;
    }

    udon::BrandMask preferredBrands;
    for (std::int32_t brand = 0; brand < config.brand_count(); ++brand) {
        if (!udon::has_brand(ledger.lifetimeBrands, brand)) {
            preferredBrands |= udon::brand_bit(brand);
        }
    }
    const std::int32_t minimumSpots = preferredBrands.any()
        ? 1
        : std::min<std::int32_t>(
              std::max(1, config.brand_count() - 1),
              static_cast<std::int32_t>(config.spots.size()));
    std::vector<udon::AgentIndex> representatives;
    for (udon::AgentIndex agent = 0;
         agent < config.agent_count();
         ++agent) {
        const udon::AgentState& agentState = state.agents.at(
            static_cast<std::size_t>(agent));
        if (agentState.kind != udon::AgentKind::Patrol) {
            continue;
        }
        const bool represented = std::any_of(
            representatives.begin(),
            representatives.end(),
            [&state, &agentState](udon::AgentIndex existing) {
                const udon::AgentState& existingState = state.agents.at(
                    static_cast<std::size_t>(existing));
                return existingState.position == agentState.position &&
                    existingState.fuel == agentState.fuel;
            });
        if (!represented) {
            representatives.push_back(agent);
        }
    }
    result.representatives = static_cast<std::int32_t>(
        representatives.size());

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    for (const udon::AgentIndex agent : representatives) {
        const udon::ExactOrienteeringReachability reachability =
            udon::enumerate_sparse_anytime_resource_routes(
                config,
                state,
                agent,
                minimumSpots,
                64U,
                maximumSettledStates,
                std::nullopt,
                preferredBrands);
        std::vector<const udon::ExactOrienteeringRoute*> routes;
        routes.reserve(
            reachability.maximalRoutes.size() +
            reachability.supplementalRoutes.size());
        for (const udon::ExactOrienteeringRoute& route :
             reachability.maximalRoutes) {
            routes.push_back(&route);
        }
        for (const udon::ExactOrienteeringRoute& route :
             reachability.supplementalRoutes) {
            routes.push_back(&route);
        }
        result.routes += static_cast<std::int64_t>(routes.size());
        for (const udon::ExactOrienteeringRoute* route : routes) {
            udon::DayPlan mutation = parentPlan;
            mutation.actions.at(static_cast<std::size_t>(agent)) =
                route->actions;
            const udon::SimulationResult simulation = simulator.simulate(
                state,
                mutation,
                false);
            const udon::SimulationResult validation = validator.validate(
                state,
                mutation,
                false);
            std::string mismatch;
            if (!simulation.valid ||
                !validator.agrees_with(simulation, validation, mismatch)) {
                continue;
            }
            ++result.validPlans;
            const udon::OfficialScore score = udon::OfficialScore::after_day(
                ledger,
                simulation.score);
            if (result.scoreAfterToday < score) {
                result.plan = std::move(mutation);
                result.simulation = simulation;
                result.scoreAfterToday = score;
                result.improved = true;
            }
        }
    }
    return result;
}

struct PermutedTerminalPairProbe {
    udon::DayPlan plan;
    udon::SimulationResult simulation;
    std::vector<std::size_t> baselineToCandidate;
    std::int64_t tasks = 0;
    std::int64_t routes = 0;
    std::int64_t crossPairs = 0;
    std::int64_t validPlans = 0;
    std::int64_t certifiedPlans = 0;
    udon::OfficialScore parentScore;
    udon::OfficialScore bestScore;
    bool improved = false;
    bool deadlineReached = false;
};

[[nodiscard]] std::optional<std::vector<std::size_t>>
exact_role_permutation_mapping(
    const std::vector<udon::AgentState>& baseline,
    const std::vector<udon::AgentState>& candidate) {
    if (baseline.size() != candidate.size()) {
        return std::nullopt;
    }
    std::vector<bool> used(candidate.size(), false);
    std::vector<std::size_t> baselineToCandidate;
    baselineToCandidate.reserve(baseline.size());
    for (const udon::AgentState& expected : baseline) {
        bool matched = false;
        for (std::size_t index = 0; index < candidate.size(); ++index) {
            const udon::AgentState& actual = candidate.at(index);
            if (!used.at(index) && expected.kind == actual.kind &&
                expected.position == actual.position &&
                expected.fuel == actual.fuel) {
                used.at(index) = true;
                baselineToCandidate.push_back(index);
                matched = true;
                break;
            }
        }
        if (!matched) {
            return std::nullopt;
        }
    }
    return baselineToCandidate;
}

[[nodiscard]] std::vector<std::size_t> identity_agent_mapping(
    std::size_t agentCount) {
    std::vector<std::size_t> mapping(agentCount);
    std::iota(mapping.begin(), mapping.end(), std::size_t{0});
    return mapping;
}

[[nodiscard]] bool valid_agent_mapping(
    const std::vector<std::size_t>& mapping,
    std::size_t agentCount) {
    if (mapping.size() != agentCount) {
        return false;
    }
    std::vector<bool> used(agentCount, false);
    for (const std::size_t physical : mapping) {
        if (physical >= agentCount || used.at(physical)) {
            return false;
        }
        used.at(physical) = true;
    }
    return true;
}

[[nodiscard]] std::vector<udon::AgentState> physical_agents_in_logical_order(
    const std::vector<udon::AgentState>& physicalAgents,
    const std::vector<std::size_t>& logicalToPhysical) {
    if (!valid_agent_mapping(logicalToPhysical, physicalAgents.size())) {
        throw std::invalid_argument("invalid logical-to-physical agent mapping");
    }
    std::vector<udon::AgentState> logicalAgents;
    logicalAgents.reserve(physicalAgents.size());
    for (const std::size_t physical : logicalToPhysical) {
        logicalAgents.push_back(physicalAgents.at(physical));
    }
    return logicalAgents;
}

[[nodiscard]] udon::DayPlan physical_plan_from_logical(
    const udon::DayPlan& logicalPlan,
    const std::vector<std::size_t>& logicalToPhysical) {
    if (!valid_agent_mapping(logicalToPhysical, logicalPlan.actions.size())) {
        throw std::invalid_argument("invalid logical plan agent mapping");
    }
    udon::DayPlan physicalPlan;
    physicalPlan.actions.resize(logicalPlan.actions.size());
    for (std::size_t logical = 0; logical < logicalPlan.actions.size(); ++logical) {
        physicalPlan.actions.at(logicalToPhysical.at(logical)) =
            logicalPlan.actions.at(logical);
    }
    return physicalPlan;
}

[[nodiscard]] std::vector<std::size_t> advance_agent_mapping(
    const std::vector<std::size_t>& logicalToPhysical,
    const std::vector<std::size_t>& baselineToCandidate) {
    if (!valid_agent_mapping(logicalToPhysical, baselineToCandidate.size()) ||
        !valid_agent_mapping(baselineToCandidate, logicalToPhysical.size())) {
        return {};
    }
    std::vector<std::size_t> nextMapping(baselineToCandidate.size());
    for (std::size_t baseline = 0;
         baseline < baselineToCandidate.size();
         ++baseline) {
        nextMapping.at(baseline) = logicalToPhysical.at(
            baselineToCandidate.at(baseline));
    }
    return nextMapping;
}

[[nodiscard]] bool exact_agent_state_vector(
    const std::vector<udon::AgentState>& left,
    const std::vector<udon::AgentState>& right) {
    return left.size() == right.size() &&
        std::equal(
            left.begin(),
            left.end(),
            right.begin(),
            [](const udon::AgentState& lhs,
               const udon::AgentState& rhs) {
                return lhs.kind == rhs.kind &&
                    lhs.position == rhs.position &&
                    lhs.fuel == rhs.fuel;
            });
}

[[nodiscard]] bool strict_permuted_terminal_improvement(
    const udon::MatchLedger& ledger,
    const udon::SimulationResult& baseline,
    const udon::SimulationResult& candidate) {
    const bool identityState =
        baseline.finalAgents.size() == candidate.finalAgents.size() &&
        std::equal(
            baseline.finalAgents.begin(),
            baseline.finalAgents.end(),
            candidate.finalAgents.begin(),
            [](const udon::AgentState& left,
               const udon::AgentState& right) {
                return left.kind == right.kind &&
                    left.position == right.position &&
                    left.fuel == right.fuel;
            });
    if (!baseline.valid || !candidate.valid || identityState ||
        baseline.roadFootprint != candidate.roadFootprint ||
        !exact_role_permutation_mapping(
             baseline.finalAgents,
             candidate.finalAgents).has_value()) {
        return false;
    }
    const udon::BrandMask baselineLifetime =
        ledger.lifetimeBrands | baseline.score.brands;
    const udon::BrandMask candidateLifetime =
        ledger.lifetimeBrands | candidate.score.brands;
    if (udon::brand_difference(baselineLifetime, candidateLifetime).any() ||
        candidate.score.dailyDistinct < baseline.score.dailyDistinct ||
        candidate.score.servings < baseline.score.servings) {
        return false;
    }
    return baselineLifetime != candidateLifetime ||
        candidate.score.dailyDistinct > baseline.score.dailyDistinct ||
        candidate.score.servings > baseline.score.servings;
}

[[nodiscard]] PermutedTerminalPairProbe probe_permuted_terminal_pairs(
    const udon::MatchConfig& config,
    const udon::DayState& state,
    const udon::MatchLedger& ledger,
    const udon::DayPlan& parentPlan,
    const udon::SimulationResult& parentSimulation,
    std::uint64_t maximumSettledStates,
    std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt,
    bool stopOnFirst = false) {
    PermutedTerminalPairProbe result;
    result.plan = parentPlan;
    result.simulation = parentSimulation;
    result.parentScore = udon::OfficialScore::after_day(
        ledger,
        parentSimulation.score);
    result.bestScore = result.parentScore;
    if (maximumSettledStates == 0U || config.spots.empty() ||
        config.spots.size() > 32U || !parentSimulation.valid) {
        return result;
    }
    const auto expired = [&deadline]() {
        return deadline.has_value() &&
            std::chrono::steady_clock::now() >= *deadline;
    };

    udon::BrandMask preferredBrands;
    for (std::int32_t brand = 0; brand < config.brand_count(); ++brand) {
        if (!udon::has_brand(ledger.lifetimeBrands, brand)) {
            preferredBrands |= udon::brand_bit(brand);
        }
    }
    const bool hasMissingLifetimeBrand = preferredBrands.any();
    if (!hasMissingLifetimeBrand) {
        for (std::int32_t brand = 0; brand < config.brand_count(); ++brand) {
            if (!udon::has_brand(parentSimulation.score.brands, brand)) {
                preferredBrands |= udon::brand_bit(brand);
            }
        }
    }
    const std::int32_t minimumSpots = hasMissingLifetimeBrand
        ? 1
        : std::min<std::int32_t>(
              std::max(1, config.brand_count() - 1),
              static_cast<std::int32_t>(config.spots.size()));
    std::vector<udon::AgentIndex> patrols;
    for (udon::AgentIndex agent = 0;
         agent < config.agent_count();
         ++agent) {
        if (state.agents.at(static_cast<std::size_t>(agent)).kind ==
            udon::AgentKind::Patrol) {
            patrols.push_back(agent);
        }
    }

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    std::unordered_set<std::uint64_t> planHashes;
    planHashes.insert(plan_hash(parentPlan));
    for (std::size_t leftOffset = 0;
         leftOffset + 1U < patrols.size();
         ++leftOffset) {
        for (std::size_t rightOffset = leftOffset + 1U;
             rightOffset < patrols.size();
             ++rightOffset) {
            if (expired()) {
                result.deadlineReached = true;
                return result;
            }
            const udon::AgentIndex left = patrols.at(leftOffset);
            const udon::AgentIndex right = patrols.at(rightOffset);
            const udon::CellId leftTerminal = parentSimulation.finalAgents.at(
                static_cast<std::size_t>(left)).position;
            const udon::CellId rightTerminal = parentSimulation.finalAgents.at(
                static_cast<std::size_t>(right)).position;
            const udon::ExactOrienteeringReachability leftReachability =
                udon::enumerate_sparse_anytime_resource_routes_to_terminal(
                    config,
                    state,
                    left,
                    rightTerminal,
                    minimumSpots,
                    32U,
                    maximumSettledStates,
                    deadline,
                    preferredBrands);
            const udon::ExactOrienteeringReachability rightReachability =
                udon::enumerate_sparse_anytime_resource_routes_to_terminal(
                    config,
                    state,
                    right,
                    leftTerminal,
                    minimumSpots,
                    32U,
                    maximumSettledStates,
                    deadline,
                    preferredBrands);
            if (expired()) {
                result.deadlineReached = true;
                return result;
            }
            result.tasks += 2;
            std::vector<const udon::ExactOrienteeringRoute*> leftRoutes;
            std::vector<const udon::ExactOrienteeringRoute*> rightRoutes;
            const auto append_routes = [](
                const udon::ExactOrienteeringReachability& reachability,
                std::vector<const udon::ExactOrienteeringRoute*>& routes) {
                for (const udon::ExactOrienteeringRoute& route :
                     reachability.maximalRoutes) {
                    routes.push_back(&route);
                }
                for (const udon::ExactOrienteeringRoute& route :
                     reachability.supplementalRoutes) {
                    routes.push_back(&route);
                }
            };
            append_routes(leftReachability, leftRoutes);
            append_routes(rightReachability, rightRoutes);
            result.routes += static_cast<std::int64_t>(
                leftRoutes.size() + rightRoutes.size());
            for (const udon::ExactOrienteeringRoute* leftRoute : leftRoutes) {
                for (const udon::ExactOrienteeringRoute* rightRoute :
                     rightRoutes) {
                    ++result.crossPairs;
                    if ((result.crossPairs & 1023) == 0 && expired()) {
                        result.deadlineReached = true;
                        return result;
                    }
                    udon::DayPlan candidate = parentPlan;
                    candidate.actions.at(static_cast<std::size_t>(left)) =
                        leftRoute->actions;
                    candidate.actions.at(static_cast<std::size_t>(right)) =
                        rightRoute->actions;
                    if (!planHashes.insert(plan_hash(candidate)).second) {
                        continue;
                    }
                    const udon::SimulationResult simulation =
                        simulator.simulate(state, candidate, false);
                    const udon::SimulationResult validation =
                        validator.validate(state, candidate, false);
                    std::string mismatch;
                    if (!simulation.valid ||
                        !validator.agrees_with(
                            simulation,
                            validation,
                            mismatch)) {
                        continue;
                    }
                    ++result.validPlans;
                    if (!strict_permuted_terminal_improvement(
                            ledger,
                            parentSimulation,
                            simulation)) {
                        continue;
                    }
                    ++result.certifiedPlans;
                    const udon::OfficialScore candidateScore =
                        udon::OfficialScore::after_day(
                            ledger,
                            simulation.score);
                    if (result.bestScore < candidateScore) {
                        result.bestScore = candidateScore;
                        result.plan = candidate;
                        result.simulation = simulation;
                        result.baselineToCandidate =
                            *exact_role_permutation_mapping(
                                parentSimulation.finalAgents,
                                simulation.finalAgents);
                        result.improved = true;
                        if (stopOnFirst) {
                            return result;
                        }
                    }
                }
            }
        }
    }
    return result;
}

[[nodiscard]] std::vector<std::int32_t> plain_map(std::size_t cellCount) {
    return std::vector<std::int32_t>(
        cellCount,
        static_cast<std::int32_t>(udon::Terrain::Plain));
}

void preserve_plain_cells(FixtureSpec& fixture) {
    for (const udon::CellId start : fixture.starts) {
        fixture.terrain.at(static_cast<std::size_t>(start)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    for (const SpotSpec& spot : fixture.spots) {
        fixture.terrain.at(static_cast<std::size_t>(spot.position)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }
}

[[nodiscard]] FixtureSpec generated_fixture(std::uint64_t seed) {
    static constexpr std::array<const char*, 6> families{
        "balanced",
        "rare-brand",
        "threshold-corridor",
        "fuel-tight",
        "high-stock",
        "overnight",
    };
    FixtureSpec fixture;
    fixture.seed = seed;
    fixture.family = families.at(static_cast<std::size_t>(seed % families.size()));
    fixture.name = fixture.family + "-seed-" + std::to_string(seed);
    fixture.terrain = plain_map(64U);

    std::mt19937_64 random(seed ^ 0x9e3779b97f4a7c15ULL);
    std::vector<udon::CellId> cells(64U);
    std::iota(cells.begin(), cells.end(), 0);
    std::shuffle(cells.begin(), cells.end(), random);
    fixture.starts.assign(cells.begin(), cells.begin() + 4);

    const std::int32_t spotCount = 6 + static_cast<std::int32_t>(random() % 3U);
    const std::int32_t brandCount = 5 + static_cast<std::int32_t>(random() % 3U);
    fixture.spots.reserve(static_cast<std::size_t>(spotCount));
    for (std::int32_t spotIndex = 0; spotIndex < spotCount; ++spotIndex) {
        std::int32_t brandIndex = spotIndex % brandCount;
        if (fixture.family == "rare-brand") {
            brandIndex = spotIndex == spotCount - 1
                ? brandCount - 1
                : spotIndex % std::max(1, brandCount - 1);
        }
        std::int32_t stock = 1 + static_cast<std::int32_t>(random() % 2U);
        if (fixture.family == "high-stock") {
            stock = 3 + static_cast<std::int32_t>(random() % 3U);
        }
        stock = std::min(
            stock,
            static_cast<std::int32_t>(fixture.starts.size()));
        fixture.spots.push_back(SpotSpec{
            100 + brandIndex,
            cells.at(static_cast<std::size_t>(4 + spotIndex)),
            stock,
        });
    }

    std::int32_t roadPercent = 16;
    std::int32_t mountainPercent = 10;
    if (fixture.family == "threshold-corridor") {
        roadPercent = 34;
        mountainPercent = 6;
    } else if (fixture.family == "fuel-tight") {
        roadPercent = 24;
        mountainPercent = 22;
    } else if (fixture.family == "overnight") {
        roadPercent = 22;
        mountainPercent = 16;
    }
    for (std::int32_t cell = 0; cell < 64; ++cell) {
        const std::int32_t draw = static_cast<std::int32_t>(random() % 100U);
        if (draw < roadPercent) {
            fixture.terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Road);
        } else if (draw < roadPercent + mountainPercent) {
            fixture.terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Mountain);
        }
    }

    fixture.daySteps.assign(
        fixture.family == "overnight" ? 5U : 4U,
        16 + static_cast<std::int32_t>(random() % 5U));
    fixture.fuelLimit = fixture.family == "fuel-tight"
        ? 8 + static_cast<std::int32_t>(random() % 3U)
        : 14 + static_cast<std::int32_t>(random() % 7U);
    fixture.players = 2 + static_cast<std::int32_t>(random() % 4U);
    fixture.busyThreshold = 2 + static_cast<std::int32_t>(random() % 3U);
    fixture.jammedThreshold =
        fixture.busyThreshold + 2 + static_cast<std::int32_t>(random() % 3U);
    preserve_plain_cells(fixture);
    return fixture;
}

[[nodiscard]] FixtureSpec generated_btc_large_fixture(
    std::uint64_t seed,
    std::int32_t side = 32,
    std::int32_t days = 10,
    std::int32_t agentCount = 8,
    std::int32_t spotCount = 12) {
    static constexpr std::array<const char*, 6> families{
        "balanced",
        "rare-brand",
        "threshold-corridor",
        "fuel-tight",
        "high-stock",
        "overnight",
    };
    const std::int32_t cellCount = side * side;
    FixtureSpec fixture;
    fixture.seed = seed;
    fixture.family = families.at(static_cast<std::size_t>(seed % families.size()));
    fixture.name = "btc-large-" + fixture.family + "-seed-" + std::to_string(seed);
    fixture.width = side;
    fixture.height = side;
    fixture.terrain = plain_map(static_cast<std::size_t>(cellCount));

    std::mt19937_64 random(seed ^ 0xd1b54a32d192ed03ULL);
    std::vector<udon::CellId> roadCells;
    const std::size_t targetRoads = static_cast<std::size_t>(std::max(
        side,
        static_cast<std::int32_t>(
            std::llround(63.0 * static_cast<double>(cellCount) / 1024.0))));
    roadCells.reserve(targetRoads);
    const auto addRoad = [&fixture, &roadCells, side](
                             std::int32_t row,
                             std::int32_t column) {
        const udon::CellId cell = row * side + column;
        if (fixture.terrain.at(static_cast<std::size_t>(cell)) !=
            static_cast<std::int32_t>(udon::Terrain::Road)) {
            fixture.terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Road);
            roadCells.push_back(cell);
        }
    };
    const std::int32_t interiorSpan = std::max(1, side - 4);
    std::int32_t column = 2 +
        static_cast<std::int32_t>(random() %
            static_cast<std::uint64_t>(interiorSpan));
    for (std::int32_t row = 0;
         row < side && roadCells.size() < targetRoads;
         ++row) {
        column = std::clamp(
            column + static_cast<std::int32_t>(random() % 3U) - 1,
            1,
            side - 2);
        addRoad(row, column);
    }
    std::int32_t row = 2 +
        static_cast<std::int32_t>(random() %
            static_cast<std::uint64_t>(interiorSpan));
    for (column = 0;
         column < side && roadCells.size() < targetRoads;
         ++column) {
        row = std::clamp(
            row + static_cast<std::int32_t>(random() % 3U) - 1,
            1,
            side - 2);
        addRoad(row, column);
    }
    for (std::int32_t cell = 0;
         roadCells.size() < targetRoads && cell < cellCount;
         ++cell) {
        const udon::CellId candidate = static_cast<udon::CellId>(
            (cell * 37 + static_cast<std::int32_t>(seed % 31U)) % cellCount);
        addRoad(candidate / side, candidate % side);
    }

    std::vector<udon::CellId> cells(static_cast<std::size_t>(cellCount));
    std::iota(cells.begin(), cells.end(), 0);
    std::shuffle(cells.begin(), cells.end(), random);
    const std::int32_t targetMountains = std::max(
        1,
        static_cast<std::int32_t>(
            std::llround(56.0 * static_cast<double>(cellCount) / 1024.0)));
    const std::int32_t targetPonds = std::max(
        1,
        static_cast<std::int32_t>(
            std::llround(5.0 * static_cast<double>(cellCount) / 1024.0)));
    std::int32_t mountains = 0;
    std::int32_t ponds = 0;
    for (const udon::CellId cell : cells) {
        std::int32_t& terrain = fixture.terrain.at(static_cast<std::size_t>(cell));
        if (terrain != static_cast<std::int32_t>(udon::Terrain::Plain)) {
            continue;
        }
        if (mountains < targetMountains) {
            terrain = static_cast<std::int32_t>(udon::Terrain::Mountain);
            ++mountains;
        } else if (ponds < targetPonds) {
            terrain = static_cast<std::int32_t>(udon::Terrain::Pond);
            ++ponds;
        } else {
            break;
        }
    }

    std::shuffle(cells.begin(), cells.end(), random);
    std::vector<udon::CellId> plainCells;
    plainCells.reserve(cells.size());
    for (const udon::CellId cell : cells) {
        if (fixture.terrain.at(static_cast<std::size_t>(cell)) ==
            static_cast<std::int32_t>(udon::Terrain::Plain)) {
            plainCells.push_back(cell);
        }
    }
    fixture.starts.assign(
        plainCells.begin(),
        plainCells.begin() + agentCount);
    if (spotCount <= 0 ||
        static_cast<std::size_t>(agentCount + spotCount) > plainCells.size()) {
        throw std::invalid_argument("dense fixture spot count exceeds plain cells");
    }
    fixture.spots.reserve(static_cast<std::size_t>(spotCount));
    for (std::int32_t spotIndex = 0; spotIndex < spotCount; ++spotIndex) {
        std::int32_t brandIndex = spotIndex % 6;
        if (fixture.family == "rare-brand") {
            brandIndex = spotIndex == spotCount - 1
                ? 5
                : spotIndex % 5;
        }
        std::int32_t stock = 1 + static_cast<std::int32_t>(random() % 8U);
        if (fixture.family == "high-stock") {
            stock = 5 + static_cast<std::int32_t>(random() % 4U);
        }
        stock = std::min(stock, agentCount);
        fixture.spots.push_back(SpotSpec{
            brandIndex,
            plainCells.at(static_cast<std::size_t>(agentCount + spotIndex)),
            stock,
        });
    }
    fixture.daySteps.assign(static_cast<std::size_t>(days), 100);
    fixture.fuelLimit = fixture.family == "fuel-tight" ? 120 : 200;
    fixture.players = 4;
    fixture.busyThreshold = 5;
    fixture.jammedThreshold = 10;
    return fixture;
}

[[nodiscard]] FixtureSpec make_fixture(const Options& options, std::uint64_t seed) {
    if (options.suite == "general") {
        return generated_fixture(seed);
    }
    if (options.suite == "general-roadless") {
        FixtureSpec fixture = generated_fixture(seed);
        fixture.family = "roadless-" + fixture.family;
        fixture.name = "roadless-" + fixture.name;
        for (std::int32_t& terrain : fixture.terrain) {
            if (terrain == static_cast<std::int32_t>(udon::Terrain::Road)) {
                terrain = static_cast<std::int32_t>(udon::Terrain::Plain);
            }
        }
        return fixture;
    }
    if (options.suite == "tanker-roadless-scale") {
        FixtureSpec fixture = generated_fixture(seed);
        fixture.family = "tanker-roadless-" + fixture.family;
        fixture.name = "tanker-roadless-" + fixture.name;
        for (std::int32_t& terrain : fixture.terrain) {
            if (terrain == static_cast<std::int32_t>(udon::Terrain::Road)) {
                terrain = static_cast<std::int32_t>(udon::Terrain::Plain);
            }
        }
        fixture.starts.resize(3U);
        static constexpr std::array<std::size_t, 4> spotCounts{3U, 4U, 5U, 6U};
        const std::size_t spotCount = spotCounts.at(
            static_cast<std::size_t>((seed / 6U) % spotCounts.size()));
        fixture.spots.resize(spotCount);
        static constexpr std::array<std::int32_t, 3> fuelProfiles{2, 8, 16};
        fixture.fuelLimit = fuelProfiles.at(
            static_cast<std::size_t>((seed / 24U) % fuelProfiles.size()));
        return fixture;
    }
    FixtureSpec fixture;
    const std::int32_t spotCount = options.spotCount > 0
        ? options.spotCount
        : 12;
    if (options.suite == "stratified-easy") {
        fixture = generated_btc_large_fixture(seed, 14, 5, 4, spotCount);
    } else if (options.suite == "stratified-easy-brand8") {
        fixture = generated_btc_large_fixture(seed, 12, 5, 4, spotCount);
        fixture.daySteps.assign(5U, 60);
        for (std::size_t spotIndex = 0;
             spotIndex < fixture.spots.size();
             ++spotIndex) {
            fixture.spots.at(spotIndex).brand =
                static_cast<std::int32_t>(spotIndex % 8U);
        }
    } else if (options.suite == "stratified-medium") {
        fixture = generated_btc_large_fixture(seed, 20, 7, 4, spotCount);
    } else if (options.suite == "stratified-hard") {
        fixture = generated_btc_large_fixture(seed, 26, 8, 6, spotCount);
    } else if (options.suite == "stratified-very-hard") {
        fixture = generated_btc_large_fixture(seed, 32, 10, 8, spotCount);
    } else if (options.suite == "multiteam-12") {
        fixture = generated_btc_large_fixture(seed, 12, 4, 4, spotCount);
    } else if (options.suite == "multiteam-16") {
        fixture = generated_btc_large_fixture(seed, 16, 5, 6, spotCount);
    } else if (options.suite == "multiteam-24") {
        fixture = generated_btc_large_fixture(seed, 24, 7, 8, spotCount);
    } else if (options.suite == "multiteam-32") {
        fixture = generated_btc_large_fixture(seed, 32, 10, 8, spotCount);
    } else {
        fixture = generated_btc_large_fixture(seed, 32, 10, 8, spotCount);
    }
    if (options.suite.starts_with("stratified-") ||
        options.suite.starts_with("multiteam-")) {
        fixture.name = options.suite + "-" + fixture.family +
            "-seed-" + std::to_string(seed);
    }
    if (options.suite == "btc-highfuel") {
        fixture.family = "high-fuel-" + fixture.family;
        fixture.name = "btc-highfuel-" + fixture.name;
        fixture.fuelLimit = 3 * fixture.daySteps.front();
    } else if (options.suite == "btc-lowfuel") {
        fixture.family = "low-fuel-" + fixture.family;
        fixture.name = "btc-lowfuel-" + fixture.name;
        fixture.fuelLimit = fixture.daySteps.front();
    } else if (options.suite == "stratified-easy" ||
               options.suite == "stratified-easy-brand8" ||
               options.suite == "stratified-medium" ||
               options.suite == "stratified-hard" ||
               options.suite == "stratified-very-hard" ||
               options.suite.starts_with("multiteam-")) {
        if (options.fuelProfile == "generated") {
            const std::uint64_t profile = seed % 9U;
            if (profile < 3U) {
                fixture.family = "low-fuel-" + fixture.family;
                fixture.name = "low-fuel-" + fixture.name;
                fixture.fuelLimit = fixture.daySteps.front();
            } else if (profile >= 6U) {
                fixture.family = "high-fuel-" + fixture.family;
                fixture.name = "high-fuel-" + fixture.name;
                fixture.fuelLimit = 3 * fixture.daySteps.front();
            }
        }
    } else if (options.suite != "btc-large" &&
               !options.suite.starts_with("multiteam-")) {
        throw std::invalid_argument("unknown suite: " + options.suite);
    }
    if (options.fuelProfile == "default") {
        // Retain the family-derived BTC-like default (120 or 200).
    } else if (options.fuelProfile == "high") {
        fixture.fuelLimit = 3 * fixture.daySteps.front();
    } else if (options.fuelProfile == "low") {
        fixture.fuelLimit = fixture.daySteps.front();
    } else if (options.fuelProfile != "generated") {
        throw std::invalid_argument("unknown fuel profile: " + options.fuelProfile);
    }
    return fixture;
}

[[nodiscard]] std::string config_document(const FixtureSpec& fixture) {
    std::ostringstream output;
    output << "{\"startsAt\":1778227200,\"daySeconds\":[";
    for (std::size_t day = 0; day < fixture.daySteps.size(); ++day) {
        if (day != 0U) {
            output << ',';
        }
        output << 5;
    }
    output << "],\"daySteps\":[";
    for (std::size_t day = 0; day < fixture.daySteps.size(); ++day) {
        if (day != 0U) {
            output << ',';
        }
        output << fixture.daySteps.at(day);
    }
    output << "],\"map\":{\"height\":" << fixture.height
           << ",\"width\":" << fixture.width << ",\"cells\":[";
    for (std::int32_t mapRow = 0; mapRow < fixture.height; ++mapRow) {
        if (mapRow != 0) {
            output << ',';
        }
        output << '[';
        for (std::int32_t mapColumn = 0; mapColumn < fixture.width; ++mapColumn) {
            if (mapColumn != 0) {
                output << ',';
            }
            output << fixture.terrain.at(
                static_cast<std::size_t>(mapRow * fixture.width + mapColumn));
        }
        output << ']';
    }
    output << "]},\"spots\":[";
    for (std::size_t index = 0; index < fixture.spots.size(); ++index) {
        if (index != 0U) {
            output << ',';
        }
        const SpotSpec& spot = fixture.spots.at(index);
        output << "{\"brand\":" << spot.brand
               << ",\"pos\":" << spot.position
               << ",\"stocks\":" << spot.stock << '}';
    }
    output << "],\"agents\":[";
    for (std::size_t index = 0; index < fixture.starts.size(); ++index) {
        if (index != 0U) {
            output << ',';
        }
        output << fixture.starts.at(index);
    }
    output << "],\"fuelLimits\":" << fixture.fuelLimit
           << ",\"players\":" << fixture.players
           << ",\"busyThreshold\":" << fixture.busyThreshold
           << ",\"jammedThreshold\":" << fixture.jammedThreshold << '}';
    return output.str();
}

[[nodiscard]] std::uint64_t mix64(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

[[nodiscard]] std::vector<std::vector<std::int32_t>> opponent_footprints(
    const FixtureSpec& fixture,
    const udon::MatchConfig& config) {
    std::vector<std::vector<std::int32_t>> footprints(
        static_cast<std::size_t>(config.day_count()),
        std::vector<std::int32_t>(
            static_cast<std::size_t>(config.map.cell_count()),
            0));
    for (std::int32_t day = 1; day <= config.day_count(); ++day) {
        for (std::size_t roadIndex = 0; roadIndex < config.roadCells.size(); ++roadIndex) {
            const udon::CellId road = config.roadCells.at(roadIndex);
            const std::uint64_t draw = mix64(
                fixture.seed ^
                (static_cast<std::uint64_t>(day) << 48U) ^
                (static_cast<std::uint64_t>(road) << 16U) ^
                static_cast<std::uint64_t>(roadIndex));
            const std::int32_t busyMass = config.players * config.busyThreshold;
            const std::int32_t jammedMass = config.players * config.jammedThreshold;
            std::int32_t stays = static_cast<std::int32_t>(
                draw % static_cast<std::uint64_t>(std::max(1, jammedMass + 3)));
            if (fixture.family == "threshold-corridor" ||
                fixture.family == "high-fuel-threshold-corridor") {
                stays = busyMass - 2 + static_cast<std::int32_t>(draw % 7U);
                if ((roadIndex + static_cast<std::size_t>(day)) % 4U == 0U) {
                    stays = jammedMass - 2 + static_cast<std::int32_t>(draw % 5U);
                }
            } else if (fixture.family == "balanced" ||
                fixture.family == "high-fuel-balanced") {
                stays /= 2;
            } else if ((fixture.family == "overnight" ||
                fixture.family == "high-fuel-overnight") && day % 2 == 0) {
                stays += config.players;
            }
            footprints.at(static_cast<std::size_t>(day - 1)).at(
                static_cast<std::size_t>(road)) = std::max(0, stays);
        }
    }
    return footprints;
}

[[nodiscard]] std::vector<udon::RoadStatus> road_statuses(
    const udon::MatchConfig& config,
    std::int32_t dayNumber,
    const std::vector<std::vector<std::int32_t>>& ownFootprints,
    const std::vector<std::vector<std::int32_t>>& opponentFootprints) {
    std::vector<udon::RoadStatus> statuses(
        static_cast<std::size_t>(config.map.cell_count()),
        udon::RoadStatus::Smooth);
    for (const udon::CellId road : config.roadCells) {
        std::int32_t stays = 0;
        for (std::int32_t offset = 1; offset <= 2; ++offset) {
            const std::int32_t completedDay = dayNumber - offset;
            if (completedDay <= 0) {
                continue;
            }
            const std::size_t historyIndex = static_cast<std::size_t>(completedDay - 1);
            stays += ownFootprints.at(historyIndex).at(static_cast<std::size_t>(road));
            stays += opponentFootprints.at(historyIndex).at(static_cast<std::size_t>(road));
        }
        udon::RoadStatus status = udon::RoadStatus::Smooth;
        if (stays >= config.players * config.jammedThreshold) {
            status = udon::RoadStatus::Jammed;
        } else if (stays >= config.players * config.busyThreshold) {
            status = udon::RoadStatus::Busy;
        }
        statuses.at(static_cast<std::size_t>(road)) = status;
    }
    return statuses;
}

[[nodiscard]] std::vector<udon::AgentKind> fallback_roles(const udon::MatchConfig& config) {
    std::vector<udon::AgentKind> roles(
        static_cast<std::size_t>(config.agent_count()),
        udon::AgentKind::Patrol);
    if (!roles.empty()) {
        roles.back() = udon::AgentKind::Tanker;
    }
    return roles;
}

[[nodiscard]] std::vector<udon::AgentKind> roles_from_mask(
    const udon::MatchConfig& config,
    std::uint32_t mask) {
    std::vector<udon::AgentKind> roles(
        static_cast<std::size_t>(config.agent_count()),
        udon::AgentKind::Patrol);
    std::int32_t patrols = config.agent_count();
    for (std::int32_t agent = 0; agent < config.agent_count(); ++agent) {
        if ((mask & (std::uint32_t{1} << static_cast<std::uint32_t>(agent))) != 0U) {
            roles.at(static_cast<std::size_t>(agent)) = udon::AgentKind::Tanker;
            --patrols;
        }
    }
    if (patrols <= 0) {
        throw std::invalid_argument("fixed role mask must leave at least one patrol");
    }
    return roles;
}

[[nodiscard]] std::int64_t percentile(
    std::vector<std::int64_t> values,
    std::int32_t percent) {
    if (values.empty()) {
        return 0;
    }
    std::sort(values.begin(), values.end());
    const std::size_t index = std::min(
        values.size() - 1U,
        static_cast<std::size_t>(
            (static_cast<std::int64_t>(percent) *
                static_cast<std::int64_t>(values.size()) + 99) /
                100 -
            1));
    return values.at(index);
}

[[nodiscard]] Metrics run_fixture(
    const FixtureSpec& fixture,
    const Options& options) {
    const udon::MatchConfig config = udon::parse_match_config(
        udon::JsonValue::parse(config_document(fixture)));
    udon::UdonShieldEngine engine(
        config,
        {},
        {},
        udon::RoutePoolSearch::SinglePass,
        kHarnessHarvestMode,
        false,
        kHarnessFutureHarvestMode);
    engine.set_short_horizon_role_fallback(options.shortRoleFallback);
    const auto roleStarted = std::chrono::steady_clock::now();
    Metrics metrics;
    std::vector<udon::AgentKind> roles;
    if (options.roleMode == "fixed") {
        roles = roles_from_mask(config, options.fixedRoleMask);
    } else {
        std::vector<udon::RoleAssignment> assignments;
        std::vector<std::vector<std::int32_t>> rolloutDailyDistinct;
        if (options.roleDetails) {
            udon::RoleSelectionDiagnostics diagnostics =
                options.roleMode == "exhaustive"
                ? engine.select_roles_exhaustive_oracle_with_diagnostics(3)
                : engine.select_roles_until_with_diagnostics(
                    options.roleBudget,
                    3);
            assignments = std::move(diagnostics.assignments);
            rolloutDailyDistinct = std::move(
                diagnostics.rolloutDailyDistinct);
        } else {
            assignments = options.roleMode == "exhaustive"
                ? engine.select_roles_exhaustive_oracle(3)
                : engine.select_roles_until(
                    options.roleBudget,
                    3);
        }
        if (options.roleDetails) {
            for (std::size_t index = 0; index < assignments.size(); ++index) {
                const udon::RoleAssignment& assignment = assignments.at(index);
                std::string mask;
                for (const udon::AgentKind role : assignment.roles) {
                    mask.push_back(role == udon::AgentKind::Patrol ? 'P' : 'T');
                }
                std::string trace;
                for (const std::int32_t daily :
                     rolloutDailyDistinct.at(index)) {
                    if (!trace.empty()) {
                        trace.push_back(',');
                    }
                    trace += std::to_string(daily);
                }
                std::cout << "role_detail,fixture=" << fixture.name
                          << ",rank=" << index + 1U
                          << ",roles=" << mask
                          << ",patrols=" << assignment.patrolCount
                          << ",rollout=" << assignment.rolloutScore.lifetimeDistinct
                          << '/' << assignment.rolloutScore.totalDailyDistinct
                          << '/' << assignment.rolloutScore.totalServings
                          << ",trace=" << trace
                          << '\n';
            }
        }
        roles = assignments.empty()
            ? fallback_roles(config)
            : assignments.front().roles;
    }
    metrics.roleMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - roleStarted).count();
    for (std::size_t index = 0; index < roles.size(); ++index) {
        if (roles.at(index) == udon::AgentKind::Tanker) {
            metrics.roleMask |= std::uint32_t{1}
                << static_cast<std::uint32_t>(index);
        }
    }

    std::vector<udon::AgentState> agents;
    agents.reserve(static_cast<std::size_t>(config.agent_count()));
    for (udon::AgentIndex agent = 0; agent < config.agent_count(); ++agent) {
        agents.push_back(udon::AgentState{
            roles.at(static_cast<std::size_t>(agent)),
            config.initialAgents.at(static_cast<std::size_t>(agent)),
            config.fuelLimit,
        });
    }

    udon::MatchLedger ledger;
    udon::MatchLedger virtualLedger;
    udon::MatchLedger richerLedger;
    udon::MatchLedger richerShadowLedger;
    std::vector<udon::AgentState> virtualAgents = agents;
    std::vector<udon::AgentState> richerAgents = agents;
    std::vector<udon::AgentState> richerShadowAgents = agents;
    std::vector<std::size_t> richerLogicalToPhysical =
        identity_agent_mapping(agents.size());
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    udon::ProtectedSlackRefiner slackRefiner(config);
    slackRefiner.enableTerminalPairExchange = options.terminalPairExchange;
    slackRefiner.enableMiddayChainAdoption = options.middayChainAdoption;
    slackRefiner.enableMiddayPairExchange = options.middayPairExchange;
    slackRefiner.enableMiddayTargetTerminalFollowup =
        options.middayTargetTerminalFollowup;
    std::vector<std::vector<std::int32_t>> ownFootprints(
        static_cast<std::size_t>(config.day_count()),
        std::vector<std::int32_t>(
            static_cast<std::size_t>(config.map.cell_count()),
            0));
    const std::vector<std::vector<std::int32_t>> opponentFootprints =
        opponent_footprints(fixture, config);
    for (std::int32_t day = 1; day <= config.day_count(); ++day) {
        const bool checkpointClosedLoopActive =
            options.checkpointClosedLoop &&
            options.publicWindowProbeBudget.count() > 5000;
        const bool causalPermutationActive = checkpointClosedLoopActive &&
            options.causalPermutedTerminalApplyStates > 0U;
        udon::DayState state;
        state.endsAt = config.startsAt + static_cast<std::int64_t>(day) * 5;
        state.dayNumber = day;
        state.agents = agents;
        state.roadStatuses =
            road_statuses(config, day, ownFootprints, opponentFootprints);
        udon::DayState planningState = state;
        if (options.protectedWaitClosedLoop) {
            planningState.agents = virtualAgents;
        }
        udon::DayState richerState = state;
        if (checkpointClosedLoopActive) {
            richerState.agents = causalPermutationActive
                ? richerShadowAgents
                : richerAgents;
        }
        udon::DayState richerPhysicalState = state;
        if (causalPermutationActive) {
            richerPhysicalState.agents = richerAgents;
            if (!valid_agent_mapping(
                    richerLogicalToPhysical,
                    richerAgents.size())) {
                ++metrics.permutedApplyMappingFailures;
                throw std::runtime_error(
                    "causal persistent permutation mapping is not bijective for " +
                    fixture.name + " on day " + std::to_string(day));
            }
            const std::vector<udon::AgentState> mappedPhysical =
                physical_agents_in_logical_order(
                    richerAgents,
                    richerLogicalToPhysical);
            bool roleMismatch = false;
            for (std::size_t index = 0; index < mappedPhysical.size(); ++index) {
                roleMismatch = roleMismatch ||
                    mappedPhysical.at(index).kind !=
                        richerShadowAgents.at(index).kind;
            }
            if (roleMismatch) {
                ++metrics.permutedApplyRoleFailures;
                throw std::runtime_error(
                    "causal persistent permutation role mapping mismatch for " +
                    fixture.name + " on day " + std::to_string(day));
            }
            if (!exact_agent_state_vector(
                    mappedPhysical,
                    richerShadowAgents)) {
                ++metrics.permutedApplyShadowStateFailures;
                throw std::runtime_error(
                    "causal persistent permutation state mapping mismatch for " +
                    fixture.name + " on day " + std::to_string(day));
            }
        }
        const udon::MatchLedger& richerPlanningLedger =
            causalPermutationActive ? richerShadowLedger : richerLedger;

        const auto started = std::chrono::steady_clock::now();
        udon::DecisionResult decision =
            engine.solve_day(
                planningState,
                options.protectedWaitClosedLoop ? virtualLedger : ledger,
                options.dayBudget);
        std::optional<udon::DecisionResult> refinementDecision;
        if (options.protectedRefineBudget.count() > 0 &&
            day == config.day_count()) {
            refinementDecision = engine.solve_day(
                planningState,
                options.protectedWaitClosedLoop ? virtualLedger : ledger,
                options.protectedRefineBudget);
        }
        const std::chrono::milliseconds elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - started);
        metrics.responseTimes.push_back(elapsed.count());
        metrics.emergencyDays += decision.emergency ? 1 : 0;
        metrics.combinationsVisited += decision.diagnostics.combinationsVisited;
        metrics.searchCompleteDays += decision.diagnostics.searchComplete ? 1 : 0;
        metrics.searchDeadlineDays += decision.diagnostics.deadlineReached ? 1 : 0;
        const udon::OptimalityGapDiagnostics& optimalityGap =
            decision.audit.optimalityGap;
        const auto recordGap = [&metrics](
                                   const udon::LexicographicGapDiagnostics& gap,
                                   std::array<std::int32_t, 4>& counts) {
            if (gap.firstOpenTier < 0 || gap.firstOpenTier > 3) {
                throw std::runtime_error(
                    "optimality envelope emitted an invalid first-open tier");
            }
            ++counts.at(static_cast<std::size_t>(gap.firstOpenTier));
            metrics.invalidOptimalityEnvelopes += gap.validEnvelope ? 0 : 1;
        };
        recordGap(optimalityGap.todayPortfolio, metrics.todayOpenTierCounts);
        recordGap(
            optimalityGap.candidateHorizon,
            metrics.candidateOpenTierCounts);
        recordGap(
            optimalityGap.viabilityHorizon,
            metrics.viabilityOpenTierCounts);
        recordGap(
            optimalityGap.absoluteHorizon,
            metrics.absoluteOpenTierCounts);
        metrics.viabilityDeadlineDays +=
            optimalityGap.viabilityDeadlineReached ? 1 : 0;
        metrics.optimalityGaps.push_back(optimalityGap);
        metrics.cacheEligible += decision.cacheRepair.eligibleContingencies;
        metrics.cacheReused += decision.cacheRepair.reusedContingencies;
        metrics.cacheRejected += decision.cacheRepair.rejectedContingencies;
        udon::SimulationResult detailed =
            simulator.simulate(state, decision.candidate.plan, false);
        const udon::SimulationResult independent =
            validator.validate(state, decision.candidate.plan, false);
        udon::SimulationResult virtualDetailed =
            simulator.simulate(
                planningState,
                decision.candidate.plan,
                false);
        const udon::SimulationResult virtualIndependent =
            validator.validate(
                planningState,
                decision.candidate.plan,
                false);
        std::string mismatch;
        const udon::DecisionResult* selectedDecision = &decision;
        udon::DayPlan appliedPlan = decision.candidate.plan;
        udon::DayPlan richerAppliedPlan = appliedPlan;
        udon::SimulationResult richerDetailed;
        udon::DayPlan richerPhysicalAppliedPlan = richerAppliedPlan;
        udon::SimulationResult richerPhysicalDetailed;
        bool sparseRouteApplied = false;
        std::uint64_t appliedPlanHash =
            plan_hash(decision.candidate.plan);
        std::string virtualMismatch;
        if (!detailed.valid ||
            !validator.agrees_with(detailed, independent, mismatch) ||
            !virtualDetailed.valid ||
            !validator.agrees_with(
                virtualDetailed,
                virtualIndependent,
                virtualMismatch)) {
            if (options.protectedWaitClosedLoop) {
                throw std::runtime_error(
                    "protected virtual-parent transition invalid for " +
                    fixture.name + ": actual=" + mismatch +
                    ", virtual=" + virtualMismatch);
            }
            ++metrics.invalidPlans;
            const udon::DayPlan wait = udon::emergency_wait_plan(config, state);
            detailed = simulator.simulate(state, wait, false);
            const udon::SimulationResult waitValidation =
                validator.validate(state, wait, false);
            if (!detailed.valid ||
                !validator.agrees_with(detailed, waitValidation, mismatch)) {
                throw std::runtime_error(
                    "fallback validation failed for " + fixture.name + ": " + mismatch);
            }
            virtualDetailed = detailed;
            appliedPlan = wait;
            appliedPlanHash = plan_hash(wait);
        } else {
            if (options.protectedWaitDetours &&
                day < config.day_count()) {
                const auto protectedDeadline =
                    options.protectedWaitBudget.count() > 0
                    ? std::chrono::steady_clock::now() +
                        options.protectedWaitBudget
                    : std::chrono::steady_clock::time_point::max();
                udon::ProtectedSlackResult protectedChoice =
                    slackRefiner.refine_wait_detours(
                    state,
                    ledger,
                    decision.candidate.plan,
                    detailed,
                    protectedDeadline);
                metrics.protectedWaitPlans +=
                    protectedChoice.diagnostics.generatedPlans;
                metrics.protectedWaitValid +=
                    protectedChoice.diagnostics.validPlans;
                metrics.protectedWaitLiftable +=
                    protectedChoice.diagnostics.liftablePlans;
                metrics.protectedWaitDeadlineDays +=
                    protectedChoice.diagnostics.deadlineReached ? 1 : 0;
                if (protectedChoice.improved) {
                    metrics.protectedWaitBestDay = state.dayNumber;
                    metrics.protectedWaitParentScore =
                        udon::OfficialScore::after_day(ledger, detailed.score);
                    metrics.protectedWaitBestScore =
                        protectedChoice.scoreAfterToday;
                    metrics.protectedWaitWitnessAgent =
                        protectedChoice.witnessAgent;
                    metrics.protectedWaitWitnessAnchor =
                        protectedChoice.witnessAnchor;
                    metrics.protectedWaitWitnessSpot =
                        protectedChoice.witnessSpot;
                    metrics.protectedWaitWitnessDuration =
                        protectedChoice.witnessDuration;
                    metrics.protectedWaitWitnessTravelSteps =
                        protectedChoice.witnessTravelSteps;
                    metrics.protectedWaitWitnessParentFuel =
                        protectedChoice.witnessParentFuel;
                    metrics.protectedWaitWitnessCandidateFuel =
                        protectedChoice.witnessCandidateFuel;
                    metrics.protectedWaitWitnessPlanHash =
                        plan_hash(protectedChoice.plan);
                }
                if (options.protectedWaitClosedLoop &&
                    protectedChoice.improved) {
                    appliedPlan = protectedChoice.plan;
                    detailed = protectedChoice.simulation;
                    appliedPlanHash = plan_hash(protectedChoice.plan);
                    ++metrics.protectedWaitTakeovers;
                }
                // Registered SCORE-MIDDAY-CHAIN-ADOPTION-210: the mid-day
                // deep-chain lane consumes only what remains of the same
                // protected window, strictly after the wait-detour fixed
                // point, and accepts solely through the unchanged
                // strict_protected_improvement certificate.
                if (options.middayChainAdoption) {
                    const udon::DayPlan middayBase =
                        options.protectedWaitClosedLoop &&
                            protectedChoice.improved
                        ? protectedChoice.plan
                        : decision.candidate.plan;
                    const udon::ProtectedSlackResult middayChoice =
                        slackRefiner.refine_midday_chains(
                            state,
                            ledger,
                            middayBase,
                            detailed,
                            protectedDeadline);
                    metrics.middayRoutes +=
                        middayChoice.diagnostics.middayRoutes;
                    metrics.middayPlans +=
                        middayChoice.diagnostics.middayGeneratedPlans;
                    metrics.middayValid +=
                        middayChoice.diagnostics.middayValidPlans;
                    metrics.middayChainAcceptances +=
                        middayChoice.diagnostics.middayChainAcceptances;
                    metrics.middayPairAcceptances +=
                        middayChoice.diagnostics.middayPairAcceptances;
                    metrics.middayRounds +=
                        middayChoice.diagnostics.middayRounds;
                    metrics.middayTargetRoutes +=
                        middayChoice.diagnostics.middayTargetRoutes;
                    metrics.middayTargetPlans +=
                        middayChoice.diagnostics.middayTargetGeneratedPlans;
                    metrics.middayTargetValid +=
                        middayChoice.diagnostics.middayTargetValidPlans;
                    metrics.middayTargetAcceptances +=
                        middayChoice.diagnostics.middayTargetAcceptances;
                    metrics.middayTargetRounds +=
                        middayChoice.diagnostics.middayTargetRounds;
                    metrics.middayDeadlineDays +=
                        middayChoice.diagnostics.deadlineReached ? 1 : 0;
                    metrics.middayFailureDays +=
                        middayChoice.diagnostics.middayFailure ? 1 : 0;
                    if (options.protectedWaitClosedLoop &&
                        middayChoice.improved) {
                        appliedPlan = middayChoice.plan;
                        detailed = middayChoice.simulation;
                        appliedPlanHash = plan_hash(middayChoice.plan);
                        metrics.middayBestDay = state.dayNumber;
                        ++metrics.middayTakeovers;
                    }
                }
            }
            if (options.terminalSparseBudget.count() > 0 &&
                day == config.day_count()) {
                metrics.terminalSparseParentScore =
                    udon::OfficialScore::after_day(ledger, detailed.score);
                const udon::ProtectedSlackResult terminalChoice =
                    slackRefiner.refine_terminal_sparse(
                        state,
                        ledger,
                        decision.candidate.plan,
                        detailed,
                        started + options.terminalSparseBudget);
                metrics.terminalSparseRoutes +=
                    terminalChoice.diagnostics.sparseRoutes;
                metrics.terminalSparsePlans +=
                    terminalChoice.diagnostics.generatedPlans;
                metrics.terminalSparseValid +=
                    terminalChoice.diagnostics.validPlans;
                metrics.terminalSparseStrict +=
                    terminalChoice.diagnostics.strictTerminalImprovements;
                metrics.terminalSparseRounds +=
                    terminalChoice.diagnostics.terminalSparseRounds;
                metrics.terminalPairAcceptances +=
                    terminalChoice.diagnostics.terminalPairAcceptances;
                metrics.terminalSparseDeadline +=
                    terminalChoice.diagnostics.deadlineReached ? 1 : 0;
                metrics.terminalSparseFailure +=
                    terminalChoice.diagnostics.sparseFailure ? 1 : 0;
                metrics.terminalSparseRefinedScore =
                    terminalChoice.scoreAfterToday;
                metrics.terminalSparseFirstRoundScore =
                    terminalChoice.firstRoundScore;
                if (terminalChoice.improved) {
                    appliedPlan = terminalChoice.plan;
                    detailed = terminalChoice.simulation;
                    appliedPlanHash = plan_hash(terminalChoice.plan);
                    ++metrics.terminalSparseTakeovers;
                }
            }
            if (refinementDecision.has_value() &&
                day == config.day_count()) {
                ++metrics.protectedTerminalAttempts;
                const udon::SimulationResult refinedDetailed =
                    simulator.simulate(
                        state,
                        refinementDecision->candidate.plan,
                        false);
                const udon::SimulationResult refinedIndependent =
                    validator.validate(
                        state,
                        refinementDecision->candidate.plan,
                        false);
                std::string refinedMismatch;
                metrics.protectedTerminalParentScore =
                    udon::OfficialScore::after_day(ledger, detailed.score);
                if (!refinedDetailed.valid ||
                    !validator.agrees_with(
                        refinedDetailed,
                        refinedIndependent,
                        refinedMismatch)) {
                    ++metrics.protectedTerminalInvalid;
                } else {
                    metrics.protectedTerminalRefinedScore =
                        udon::OfficialScore::after_day(
                            ledger,
                            refinedDetailed.score);
                    if (metrics.protectedTerminalParentScore <
                        metrics.protectedTerminalRefinedScore) {
                        appliedPlan = refinementDecision->candidate.plan;
                        detailed = refinedDetailed;
                        selectedDecision = &*refinementDecision;
                        appliedPlanHash = plan_hash(appliedPlan);
                        ++metrics.protectedTerminalTakeovers;
                    }
                }
            }

            // DEADLINE-PUBLIC-WINDOW-218 stage-A falsification probe. Keep the
            // complete current checkpoint and every later match state
            // unchanged. The extra public window is used only to ask whether
            // the existing protected refiner can produce a strict exact
            // certificate from that exact checkpoint. A SCORE successor is
            // forbidden unless this non-propagating probe is recurrent on
            // fresh development fixtures.
            if (!checkpointClosedLoopActive &&
                options.publicWindowProbeBudget.count() > 5000) {
                ++metrics.publicWindowProbeDays;
                constexpr auto kTransportSafety =
                    std::chrono::milliseconds{1100};
                const auto publicDeadline = started +
                    options.publicWindowProbeBudget - kTransportSafety;
                udon::ProtectedSlackResult probe;
                probe.plan = appliedPlan;
                probe.simulation = detailed;
                probe.scoreAfterToday =
                    udon::OfficialScore::after_day(ledger, detailed.score);
                if (std::chrono::steady_clock::now() < publicDeadline) {
                    if (day == config.day_count()) {
                        probe = slackRefiner.refine_terminal_sparse(
                            state,
                            ledger,
                            appliedPlan,
                            detailed,
                            publicDeadline);
                        metrics.publicWindowProbeRoutes +=
                            probe.diagnostics.sparseRoutes;
                        metrics.publicWindowProbePlans +=
                            probe.diagnostics.generatedPlans;
                        metrics.publicWindowProbeValid +=
                            probe.diagnostics.validPlans;
                        metrics.publicWindowProbeAcceptances +=
                            probe.diagnostics.strictTerminalImprovements;
                    } else {
                        const udon::ProtectedSlackResult waitProbe =
                            slackRefiner.refine_wait_detours(
                                state,
                                ledger,
                                appliedPlan,
                                detailed,
                                publicDeadline);
                        probe = waitProbe;
                        const udon::ProtectedSlackResult middayProbe =
                            slackRefiner.refine_midday_chains(
                                state,
                                ledger,
                                waitProbe.plan,
                                waitProbe.simulation,
                                publicDeadline);
                        metrics.publicWindowProbeRoutes +=
                            middayProbe.diagnostics.middayRoutes;
                        metrics.publicWindowProbePlans +=
                            middayProbe.diagnostics.middayGeneratedPlans;
                        metrics.publicWindowProbeValid +=
                            middayProbe.diagnostics.middayValidPlans;
                        metrics.publicWindowProbeAcceptances +=
                            middayProbe.diagnostics.middayChainAcceptances;
                        if (middayProbe.improved) {
                            probe = middayProbe;
                        }
                        probe.diagnostics.deadlineReached =
                            waitProbe.diagnostics.deadlineReached ||
                            middayProbe.diagnostics.deadlineReached;
                        probe.diagnostics.middayFailure =
                            waitProbe.diagnostics.middayFailure ||
                            middayProbe.diagnostics.middayFailure;
                    }
                } else {
                    probe.diagnostics.deadlineReached = true;
                }
                metrics.publicWindowProbeDeadlineDays +=
                    probe.diagnostics.deadlineReached ? 1 : 0;
                metrics.publicWindowProbeFailureDays +=
                    (probe.diagnostics.sparseFailure ||
                     probe.diagnostics.middayFailure) ? 1 : 0;
                if (probe.improved) {
                    if (day < config.day_count() &&
                        !udon::protected_slack_transition_dominates(
                            detailed,
                            probe.simulation)) {
                        throw std::runtime_error(
                            "public-window probe violated checkpoint transition dominance for " +
                            fixture.name + " on day " + std::to_string(day));
                    }
                    const udon::OfficialScore checkpointScore =
                        udon::OfficialScore::after_day(ledger, detailed.score);
                    if (!(checkpointScore < probe.scoreAfterToday)) {
                        throw std::runtime_error(
                            "public-window probe reported a non-strict score for " +
                            fixture.name + " on day " + std::to_string(day));
                    }
                    ++metrics.publicWindowProbeTakeovers;
                    if (checkpointScore.lifetimeDistinct <
                        probe.scoreAfterToday.lifetimeDistinct) {
                        ++metrics.publicWindowProbeTier1Gains;
                    } else if (checkpointScore.totalDailyDistinct <
                        probe.scoreAfterToday.totalDailyDistinct) {
                        ++metrics.publicWindowProbeTier2Gains;
                    } else {
                        metrics.publicWindowProbeServingGain +=
                            probe.scoreAfterToday.totalServings -
                            checkpointScore.totalServings;
                    }
                    if (options.publicWindowApply) {
                        appliedPlan = probe.plan;
                        detailed = probe.simulation;
                        appliedPlanHash = plan_hash(appliedPlan);
                    }
                }
            }

            // DEADLINE-CHECKPOINT-CLOSED-LOOP-219. The complete current
            // checkpoint is advanced independently of the richer branch. The
            // exact checkpoint plan must first replay on the richer state with
            // the same footprint and a dominating transition; only then may
            // public time refine the richer branch. The main solver above is
            // still called exactly once from the existing virtual parent.
            if (checkpointClosedLoopActive) {
                const udon::SimulationResult replayedCheckpoint =
                    simulator.simulate(richerState, appliedPlan, false);
                const udon::SimulationResult replayedIndependent =
                    validator.validate(richerState, appliedPlan, false);
                std::string replayMismatch;
                udon::MatchLedger checkpointAfter = ledger;
                checkpointAfter.apply(detailed.score);
                udon::MatchLedger richerAfter = richerPlanningLedger;
                if (!replayedCheckpoint.valid ||
                    !validator.agrees_with(
                        replayedCheckpoint,
                        replayedIndependent,
                        replayMismatch) ||
                    !udon::protected_slack_transition_dominates(
                        detailed,
                        replayedCheckpoint)) {
                    ++metrics.checkpointClosedLoopFailures;
                    throw std::runtime_error(
                        "checkpoint replay failed richer-state dominance for " +
                        fixture.name + " on day " + std::to_string(day) +
                        ": " + replayMismatch);
                }
                richerAfter.apply(replayedCheckpoint.score);
                if (!udon::protected_slack_ledger_dominates(
                        checkpointAfter,
                        richerAfter)) {
                    ++metrics.checkpointClosedLoopFailures;
                    throw std::runtime_error(
                        "checkpoint replay failed richer-ledger dominance for " +
                        fixture.name + " on day " + std::to_string(day));
                }

                richerAppliedPlan = appliedPlan;
                richerDetailed = replayedCheckpoint;
                // Reuse only cache entries already earned by the unchanged
                // checkpoint pipeline.  Public continuation writes into this
                // snapshot, so its extra work cannot warm or otherwise alter
                // the checkpoint refiner on a later day.
                udon::ProtectedSlackRefiner publicSlackRefiner = slackRefiner;
                publicSlackRefiner.enableTerminalMarginalReservoir =
                    options.terminalMarginalReservoir;
                ++metrics.publicWindowProbeDays;
                constexpr auto kTransportSafety =
                    std::chrono::milliseconds{1100};
                const auto publicDeadline = started +
                    options.publicWindowProbeBudget - kTransportSafety;
                udon::ProtectedSlackResult continuation;
                continuation.plan = richerAppliedPlan;
                continuation.simulation = richerDetailed;
                continuation.scoreAfterToday =
                        udon::OfficialScore::after_day(
                        richerPlanningLedger,
                        richerDetailed.score);
                if (std::chrono::steady_clock::now() < publicDeadline) {
                    if (day == config.day_count()) {
                        continuation = publicSlackRefiner.refine_terminal_sparse(
                            richerState,
                            richerPlanningLedger,
                            richerAppliedPlan,
                            richerDetailed,
                            publicDeadline);
                        metrics.publicWindowProbeRoutes +=
                            continuation.diagnostics.sparseRoutes;
                        metrics.publicWindowProbePlans +=
                            continuation.diagnostics.generatedPlans;
                        metrics.publicWindowProbeValid +=
                            continuation.diagnostics.validPlans;
                        metrics.publicWindowProbeAcceptances +=
                            continuation.diagnostics.strictTerminalImprovements;
                        metrics.terminalMarginalRoutes +=
                            continuation.diagnostics.terminalMarginalRoutes;
                        metrics.terminalMarginalPlans +=
                            continuation.diagnostics.terminalMarginalGeneratedPlans;
                        metrics.terminalMarginalValid +=
                            continuation.diagnostics.terminalMarginalValidPlans;
                        metrics.terminalMarginalAcceptances +=
                            continuation.diagnostics.terminalMarginalAcceptances;
                        metrics.terminalMarginalRounds +=
                            continuation.diagnostics.terminalMarginalRounds;
                        metrics.terminalMarginalDeadline +=
                            continuation.diagnostics.terminalMarginalDeadline
                            ? 1
                            : 0;
                        metrics.terminalMarginalFailure +=
                            continuation.diagnostics.terminalMarginalFailure
                            ? 1
                            : 0;
                        metrics.terminalMarginalParentScore =
                            continuation.canonicalTerminalScore;
                        metrics.terminalMarginalRefinedScore =
                            continuation.scoreAfterToday;
                    } else {
                        const udon::ProtectedSlackResult waitContinuation =
                            publicSlackRefiner.refine_wait_detours(
                                richerState,
                                richerPlanningLedger,
                                richerAppliedPlan,
                                richerDetailed,
                                publicDeadline);
                        continuation = waitContinuation;
                        const udon::ProtectedSlackResult middayContinuation =
                            publicSlackRefiner.refine_midday_chains(
                                richerState,
                                richerPlanningLedger,
                                waitContinuation.plan,
                                waitContinuation.simulation,
                                publicDeadline);
                        metrics.publicWindowProbeRoutes +=
                            middayContinuation.diagnostics.middayRoutes;
                        metrics.publicWindowProbePlans +=
                            middayContinuation.diagnostics.middayGeneratedPlans;
                        metrics.publicWindowProbeValid +=
                            middayContinuation.diagnostics.middayValidPlans;
                        metrics.publicWindowProbeAcceptances +=
                            middayContinuation.diagnostics.middayChainAcceptances;
                        if (middayContinuation.improved) {
                            continuation = middayContinuation;
                        }
                        continuation.diagnostics.deadlineReached =
                            waitContinuation.diagnostics.deadlineReached ||
                            middayContinuation.diagnostics.deadlineReached;
                        continuation.diagnostics.middayFailure =
                            waitContinuation.diagnostics.middayFailure ||
                            middayContinuation.diagnostics.middayFailure;
                    }
                } else {
                    continuation.diagnostics.deadlineReached = true;
                }
                metrics.publicWindowProbeDeadlineDays +=
                    continuation.diagnostics.deadlineReached ? 1 : 0;
                metrics.publicWindowProbeFailureDays +=
                    (continuation.diagnostics.sparseFailure ||
                     continuation.diagnostics.middayFailure) ? 1 : 0;
                if (continuation.improved) {
                    if (day < config.day_count() &&
                        !udon::protected_slack_transition_dominates(
                            richerDetailed,
                            continuation.simulation)) {
                        ++metrics.checkpointClosedLoopFailures;
                        throw std::runtime_error(
                            "public continuation violated richer transition dominance for " +
                            fixture.name + " on day " + std::to_string(day));
                    }
                    const udon::OfficialScore richerCheckpointScore =
                        udon::OfficialScore::after_day(
                            richerPlanningLedger,
                            richerDetailed.score);
                    if (!(richerCheckpointScore <
                          continuation.scoreAfterToday)) {
                        ++metrics.checkpointClosedLoopFailures;
                        throw std::runtime_error(
                            "public continuation reported a non-strict score for " +
                            fixture.name + " on day " + std::to_string(day));
                    }
                    ++metrics.publicWindowProbeTakeovers;
                    ++metrics.checkpointClosedLoopTakeovers;
                    if (richerCheckpointScore.lifetimeDistinct <
                        continuation.scoreAfterToday.lifetimeDistinct) {
                        ++metrics.publicWindowProbeTier1Gains;
                    } else if (richerCheckpointScore.totalDailyDistinct <
                        continuation.scoreAfterToday.totalDailyDistinct) {
                        ++metrics.publicWindowProbeTier2Gains;
                    } else {
                        metrics.publicWindowProbeServingGain +=
                            continuation.scoreAfterToday.totalServings -
                            richerCheckpointScore.totalServings;
                    }
                    richerAppliedPlan = continuation.plan;
                    richerDetailed = continuation.simulation;
                }

                udon::MatchLedger candidateAfter = richerPlanningLedger;
                candidateAfter.apply(richerDetailed.score);
                const bool terminalDay = day == config.day_count();
                const bool completeCheckpointDominates =
                    (terminalDay ||
                     udon::protected_slack_transition_dominates(
                         detailed,
                         richerDetailed)) &&
                    udon::protected_slack_ledger_relation_for_day(
                        checkpointAfter,
                        candidateAfter,
                        terminalDay);
                if (!completeCheckpointDominates) {
                    ++metrics.checkpointClosedLoopFailures;
                    throw std::runtime_error(
                        "public continuation failed complete checkpoint dominance for " +
                        fixture.name + " on day " + std::to_string(day));
                }

                if (causalPermutationActive) {
                    // SCORE-CAUSAL-PERSISTENT-AGENT-PERMUTATION-254. The
                    // already-complete public continuation above is the one
                    // causal parent. The logical shadow stays on that exact
                    // transition; only the physical wire identity and actual
                    // score ledger may advance after the full certificate.
                    const auto validatePhysicalPlan = [&](const udon::DayPlan& plan) {
                        const udon::SimulationResult simulated =
                            simulator.simulate(richerPhysicalState, plan, false);
                        const udon::SimulationResult validated =
                            validator.validate(richerPhysicalState, plan, false);
                        std::string physicalMismatch;
                        if (!simulated.valid ||
                            !validator.agrees_with(
                                simulated,
                                validated,
                                physicalMismatch)) {
                            ++metrics.permutedApplyUncertifiedTakeovers;
                            throw std::runtime_error(
                                "causal persistent permutation physical plan failed validation for " +
                                fixture.name + " on day " +
                                std::to_string(day) + ": " + physicalMismatch);
                        }
                        return simulated;
                    };
                    richerPhysicalAppliedPlan = physical_plan_from_logical(
                        richerAppliedPlan,
                        richerLogicalToPhysical);
                    richerPhysicalDetailed = validatePhysicalPlan(
                        richerPhysicalAppliedPlan);
                    const std::vector<udon::AgentState> baselineLogicalFinal =
                        physical_agents_in_logical_order(
                            richerPhysicalDetailed.finalAgents,
                            richerLogicalToPhysical);
                    if (!exact_agent_state_vector(
                            baselineLogicalFinal,
                            richerDetailed.finalAgents) ||
                        richerPhysicalDetailed.roadFootprint !=
                            richerDetailed.roadFootprint ||
                        richerPhysicalDetailed.score.brands !=
                            richerDetailed.score.brands ||
                        richerPhysicalDetailed.score.dailyDistinct !=
                            richerDetailed.score.dailyDistinct ||
                        richerPhysicalDetailed.score.servings !=
                            richerDetailed.score.servings) {
                        ++metrics.permutedApplyShadowStateFailures;
                        throw std::runtime_error(
                            "causal persistent permutation baseline remap lost logical equivalence for " +
                            fixture.name + " on day " + std::to_string(day));
                    }

                    if (day < config.day_count()) {
                        constexpr auto kPermutationValidationReserve =
                            std::chrono::milliseconds{75};
                        const auto permutationDeadline =
                            publicDeadline - kPermutationValidationReserve;
                        PermutedTerminalPairProbe permutationProbe;
                        if (std::chrono::steady_clock::now() <
                            permutationDeadline) {
                            permutationProbe = probe_permuted_terminal_pairs(
                                config,
                                richerState,
                                richerPlanningLedger,
                                richerAppliedPlan,
                                richerDetailed,
                                options.causalPermutedTerminalApplyStates,
                                permutationDeadline,
                                true);
                        } else {
                            permutationProbe.deadlineReached = true;
                        }
                        metrics.permutedApplyTasks += permutationProbe.tasks;
                        metrics.permutedApplyRoutes += permutationProbe.routes;
                        metrics.permutedApplyCrossPairs +=
                            permutationProbe.crossPairs;
                        metrics.permutedApplyValidPlans +=
                            permutationProbe.validPlans;
                        metrics.permutedApplyCertified +=
                            permutationProbe.certifiedPlans;
                        metrics.permutedApplyDeadlineDays +=
                            permutationProbe.deadlineReached ? 1 : 0;
                        if (permutationProbe.improved &&
                            !permutationProbe.deadlineReached) {
                            const std::vector<std::size_t> nextMapping =
                                advance_agent_mapping(
                                    richerLogicalToPhysical,
                                    permutationProbe.baselineToCandidate);
                            if (!valid_agent_mapping(
                                    nextMapping,
                                    richerAgents.size())) {
                                ++metrics.permutedApplyMappingFailures;
                                throw std::runtime_error(
                                    "causal persistent permutation produced a non-bijective mapping for " +
                                    fixture.name + " on day " +
                                    std::to_string(day));
                            }
                            const udon::DayPlan physicalCandidate =
                                physical_plan_from_logical(
                                    permutationProbe.plan,
                                    richerLogicalToPhysical);
                            const udon::SimulationResult physicalCandidateDetailed =
                                validatePhysicalPlan(physicalCandidate);
                            const std::vector<udon::AgentState> remappedFinal =
                                physical_agents_in_logical_order(
                                    physicalCandidateDetailed.finalAgents,
                                    nextMapping);
                            if (!exact_agent_state_vector(
                                    remappedFinal,
                                    richerDetailed.finalAgents) ||
                                physicalCandidateDetailed.roadFootprint !=
                                    permutationProbe.simulation.roadFootprint ||
                                physicalCandidateDetailed.score.brands !=
                                    permutationProbe.simulation.score.brands ||
                                physicalCandidateDetailed.score.dailyDistinct !=
                                    permutationProbe.simulation.score.dailyDistinct ||
                                physicalCandidateDetailed.score.servings !=
                                    permutationProbe.simulation.score.servings) {
                                ++metrics.permutedApplyShadowStateFailures;
                                ++metrics.permutedApplyUncertifiedTakeovers;
                                throw std::runtime_error(
                                    "causal persistent permutation candidate failed physical/shadow certificate for " +
                                    fixture.name + " on day " +
                                    std::to_string(day));
                            }
                            const udon::OfficialScore physicalParentScore =
                                udon::OfficialScore::after_day(
                                    richerLedger,
                                    richerPhysicalDetailed.score);
                            const udon::OfficialScore physicalCandidateScore =
                                udon::OfficialScore::after_day(
                                    richerLedger,
                                    physicalCandidateDetailed.score);
                            if (physicalParentScore < physicalCandidateScore) {
                                ++metrics.permutedApplyTakeovers;
                                metrics.permutedApplyLifetimeGain +=
                                    physicalCandidateScore.lifetimeDistinct -
                                    physicalParentScore.lifetimeDistinct;
                                metrics.permutedApplyDailyGain +=
                                    physicalCandidateScore.totalDailyDistinct -
                                    physicalParentScore.totalDailyDistinct;
                                metrics.permutedApplyServingGain +=
                                    physicalCandidateScore.totalServings -
                                    physicalParentScore.totalServings;
                                richerPhysicalAppliedPlan = physicalCandidate;
                                richerPhysicalDetailed = physicalCandidateDetailed;
                                richerLogicalToPhysical = nextMapping;
                            }
                        }
                    }
                }

            }
            if (options.permutedTerminalProbeStates > 0U &&
                day < config.day_count()) {
                const std::uint64_t incumbentHashBefore =
                    plan_hash(appliedPlan);
                const PermutedTerminalPairProbe probe =
                    probe_permuted_terminal_pairs(
                        config,
                        state,
                        ledger,
                        appliedPlan,
                        detailed,
                        options.permutedTerminalProbeStates);
                metrics.permutedProbeTasks += probe.tasks;
                metrics.permutedProbeRoutes += probe.routes;
                metrics.permutedProbeCrossPairs += probe.crossPairs;
                metrics.permutedProbeValidPlans += probe.validPlans;
                metrics.permutedProbeCertified += probe.certifiedPlans;
                if (probe.certifiedPlans > 0) {
                    ++metrics.permutedProbeYieldDays;
                    metrics.permutedProbeLifetimeGain +=
                        probe.bestScore.lifetimeDistinct -
                        probe.parentScore.lifetimeDistinct;
                    metrics.permutedProbeDailyGain +=
                        probe.bestScore.totalDailyDistinct -
                        probe.parentScore.totalDailyDistinct;
                    metrics.permutedProbeServingGain +=
                        probe.bestScore.totalServings -
                        probe.parentScore.totalServings;
                }
                if (plan_hash(appliedPlan) != incumbentHashBefore) {
                    ++metrics.permutedProbeMutationFailures;
                    throw std::runtime_error(
                        "permuted-terminal attribution mutated incumbent for " +
                        fixture.name + " on day " + std::to_string(day));
                }
            }
            if (options.sparseRouteStates > 0U) {
                const udon::OfficialScore parentScore =
                    udon::OfficialScore::after_day(ledger, detailed.score);
                const SparseCurrentDayChoice sparse =
                    sparse_current_day_exchange(
                        config,
                        state,
                        ledger,
                        appliedPlan,
                        detailed,
                        options.sparseRouteStates);
                metrics.sparseRouteRepresentatives +=
                    sparse.representatives;
                metrics.sparseRoutes += sparse.routes;
                metrics.sparseValidPlans += sparse.validPlans;
                if (sparse.improved) {
                    appliedPlan = sparse.plan;
                    detailed = sparse.simulation;
                    appliedPlanHash = plan_hash(appliedPlan);
                    sparseRouteApplied = true;
                    ++metrics.sparseTakeovers;
                    if (parentScore.lifetimeDistinct ==
                            sparse.scoreAfterToday.lifetimeDistinct &&
                        parentScore.totalDailyDistinct ==
                            sparse.scoreAfterToday.totalDailyDistinct) {
                        metrics.sparseServingGain +=
                            sparse.scoreAfterToday.totalServings -
                            parentScore.totalServings;
                    }
                }
            }
            udon::DecisionResult acknowledgedDecision = *selectedDecision;
            if (sparseRouteApplied) {
                engine.record_applied_transition(state, detailed);
            } else {
                engine.record_submitted(*selectedDecision, elapsed);
            }
            if (options.postAckBudget.count() > 0 &&
                day < config.day_count()) {
                ++metrics.postAckCalls;
                const auto postAckDeadline =
                    std::chrono::steady_clock::now() + options.postAckBudget;
                bool precomputeAtFixedPoint = false;
                while (!precomputeAtFixedPoint &&
                       std::chrono::steady_clock::now() < postAckDeadline &&
                       engine.remaining_post_ack_compute_budget().count() > 0) {
                    const auto sliceDeadline = std::min(
                        postAckDeadline,
                        std::chrono::steady_clock::now() +
                            options.postAckSliceBudget);
                    const std::int32_t added =
                        engine.precompute_next_day_contingencies(
                            state,
                            ledger,
                            acknowledgedDecision,
                            sliceDeadline);
                    metrics.postAckContingencies += added;
                    precomputeAtFixedPoint = added == 0;
                }
                if (precomputeAtFixedPoint &&
                    std::chrono::steady_clock::now() < postAckDeadline &&
                    engine.remaining_post_ack_compute_budget().count() > 0) {
                    const std::size_t proofRecordsBefore =
                        engine.response_ledger().strongProofs.size();
                    ++metrics.postAckProofCalls;
                    metrics.postAckCompletedProofs +=
                        engine.prove_remaining_horizon(
                            state,
                            ledger,
                            acknowledgedDecision,
                            postAckDeadline);
                    metrics.postAckStrongProofRecords +=
                        static_cast<std::int64_t>(
                            engine.response_ledger().strongProofs.size() -
                            proofRecordsBefore);
                }
            }
        }
        if (options.protectedWaitClosedLoop &&
            day < config.day_count() &&
            !udon::protected_slack_transition_dominates(
                virtualDetailed,
                detailed)) {
            throw std::runtime_error(
                "protected virtual-parent state relation failed for " +
                fixture.name + " on day " + std::to_string(day));
        }
        ledger.apply(detailed.score);
        if (checkpointClosedLoopActive) {
            if (causalPermutationActive) {
                richerShadowLedger.apply(richerDetailed.score);
                richerLedger.apply(richerPhysicalDetailed.score);
            } else {
                richerLedger.apply(richerDetailed.score);
            }
            const bool terminalDay = day == config.day_count();
            const udon::MatchLedger& causalRicherLedger =
                causalPermutationActive ? richerShadowLedger : richerLedger;
            const bool closedLoopRelationValid =
                (terminalDay ||
                 udon::protected_slack_transition_dominates(
                     detailed,
                     richerDetailed)) &&
                udon::protected_slack_ledger_relation_for_day(
                    ledger,
                    causalRicherLedger,
                    terminalDay);
            if (!closedLoopRelationValid) {
                ++metrics.checkpointClosedLoopFailures;
                throw std::runtime_error(
                    "checkpoint closed-loop relation failed for " +
                    fixture.name + " on day " + std::to_string(day));
            }
            if (causalPermutationActive &&
                !udon::protected_slack_ledger_relation_for_day(
                    richerShadowLedger,
                    richerLedger,
                    terminalDay)) {
                ++metrics.permutedApplyShadowLedgerFailures;
                throw std::runtime_error(
                    "causal persistent permutation actual ledger lost shadow dominance for " +
                    fixture.name + " on day " + std::to_string(day));
            }
        }
        if (options.protectedWaitClosedLoop) {
            virtualLedger.apply(virtualDetailed.score);
            const bool ledgerRelationValid = day == config.day_count()
                ? !(udon::OfficialScore{
                        ledger.lifetime_distinct(),
                        ledger.totalDailyDistinct,
                        ledger.totalServings} <
                    udon::OfficialScore{
                        virtualLedger.lifetime_distinct(),
                        virtualLedger.totalDailyDistinct,
                        virtualLedger.totalServings})
                : udon::protected_slack_ledger_dominates(
                      virtualLedger,
                      ledger);
            if (!ledgerRelationValid) {
                throw std::runtime_error(
                    "protected virtual-parent ledger relation failed for " +
                    fixture.name + " on day " + std::to_string(day));
            }
            metrics.protectedVirtualDayScores.push_back(
                udon::OfficialScore{
                    virtualLedger.lifetime_distinct(),
                    virtualLedger.totalDailyDistinct,
                    virtualLedger.totalServings,
                });
        }
        const udon::SimulationResult& reportedDetailed =
            checkpointClosedLoopActive
            ? (causalPermutationActive
                   ? richerPhysicalDetailed
                   : richerDetailed)
            : detailed;
        const udon::MatchLedger& reportedLedger =
            checkpointClosedLoopActive ? richerLedger : ledger;
        metrics.exactDayScores.push_back(reportedDetailed.score);
        metrics.cumulativeDayScores.push_back(udon::OfficialScore{
            reportedLedger.lifetime_distinct(),
            reportedLedger.totalDailyDistinct,
            reportedLedger.totalServings,
        });
        metrics.planHashes.push_back(
            checkpointClosedLoopActive
            ? plan_hash(
                  causalPermutationActive
                  ? richerPhysicalAppliedPlan
                  : richerAppliedPlan)
            : appliedPlanHash);
        const udon::MatchLedger& causalParentLedger =
            causalPermutationActive ? richerShadowLedger : reportedLedger;
        metrics.permutedCausalDayScores.push_back(udon::OfficialScore{
            causalParentLedger.lifetime_distinct(),
            causalParentLedger.totalDailyDistinct,
            causalParentLedger.totalServings,
        });
        metrics.permutedCausalPlanHashes.push_back(
            causalPermutationActive
            ? plan_hash(richerAppliedPlan)
            : metrics.planHashes.back());
        metrics.deadlineDays.push_back(decision.diagnostics.deadlineReached);
        metrics.exactSupportedAgents.push_back(
            decision.audit.columnGeneration.exactOrienteeringSupportedAgents);
        metrics.exactSettledStates.push_back(
            decision.audit.columnGeneration.exactOrienteeringSettledStates);
        metrics.exactSeedServings.push_back(
            decision.audit.columnGeneration.exactOrienteeringSeedServings);
        metrics.exactLocalServings.push_back(
            decision.audit.columnGeneration.exactOrienteeringLocalServings);
        agents = detailed.finalAgents;
        if (checkpointClosedLoopActive) {
            if (causalPermutationActive) {
                richerShadowAgents = richerDetailed.finalAgents;
                richerAgents = richerPhysicalDetailed.finalAgents;
            } else {
                richerAgents = richerDetailed.finalAgents;
            }
        }
        if (options.protectedWaitClosedLoop) {
            virtualAgents = virtualDetailed.finalAgents;
        }
        ownFootprints.at(static_cast<std::size_t>(day - 1)) =
            detailed.roadFootprint;
    }

    metrics.score = udon::OfficialScore{
        options.checkpointClosedLoop &&
            options.publicWindowProbeBudget.count() > 5000
            ? richerLedger.lifetime_distinct()
            : ledger.lifetime_distinct(),
        options.checkpointClosedLoop &&
            options.publicWindowProbeBudget.count() > 5000
            ? richerLedger.totalDailyDistinct
            : ledger.totalDailyDistinct,
        options.checkpointClosedLoop &&
            options.publicWindowProbeBudget.count() > 5000
            ? richerLedger.totalServings
            : ledger.totalServings,
    };
    if (options.terminalMarginalReservoir &&
        metrics.terminalMarginalParentScore == udon::OfficialScore{}) {
        // A 5000-ms lane, or a longer lane with no post-checkpoint time, never
        // enters the marginal suffix. Its exact causal parent is therefore the
        // emitted canonical result itself.
        metrics.terminalMarginalParentScore = metrics.score;
        metrics.terminalMarginalRefinedScore = metrics.score;
    }
    metrics.checkpointClosedLoopParentScore = udon::OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
    metrics.permutedCausalParentScore =
        options.causalPermutedTerminalApplyStates > 0U &&
            options.checkpointClosedLoop &&
            options.publicWindowProbeBudget.count() > 5000
        ? udon::OfficialScore{
              richerShadowLedger.lifetime_distinct(),
              richerShadowLedger.totalDailyDistinct,
              richerShadowLedger.totalServings,
          }
        : metrics.score;
    if (options.causalPermutedTerminalApplyStates > 0U &&
        options.publicWindowProbeBudget.count() <= 5000 &&
        (metrics.permutedCausalParentScore.lifetimeDistinct !=
             metrics.score.lifetimeDistinct ||
         metrics.permutedCausalParentScore.totalDailyDistinct !=
             metrics.score.totalDailyDistinct ||
         metrics.permutedCausalParentScore.totalServings !=
             metrics.score.totalServings ||
         metrics.permutedCausalPlanHashes != metrics.planHashes)) {
        ++metrics.permutedCausalNoopFailures;
        throw std::runtime_error(
            "causal persistent permutation changed the 5000-ms no-op lane for " +
            fixture.name);
    }
    if (options.protectedWaitClosedLoop) {
        metrics.protectedVirtualScore = udon::OfficialScore{
            virtualLedger.lifetime_distinct(),
            virtualLedger.totalDailyDistinct,
            virtualLedger.totalServings,
        };
    }
    return metrics;
}

[[nodiscard]] Options parse_options(int argumentCount, char** arguments) {
    Options options;
    for (int argument = 1; argument < argumentCount; ++argument) {
        const std::string value = arguments[argument];
        const auto next = [&]() -> std::string {
            if (argument + 1 >= argumentCount) {
                throw std::invalid_argument("missing value after " + value);
            }
            return arguments[++argument];
        };
        if (value == "--version") {
            options.version = next();
        } else if (value == "--track") {
            options.track = next();
        } else if (value == "--suite") {
            options.suite = next();
        } else if (value == "--first-seed") {
            options.firstSeed = std::stoull(next());
        } else if (value == "--seeds") {
            options.seedCount = std::stoi(next());
        } else if (value == "--budget-ms") {
            options.dayBudget = std::chrono::milliseconds{std::stoll(next())};
        } else if (value == "--role-ms") {
            options.roleBudget = std::chrono::milliseconds{std::stoll(next())};
        } else if (value == "--role-mode") {
            options.roleMode = next();
        } else if (value == "--role-mask") {
            options.fixedRoleMask = static_cast<std::uint32_t>(std::stoul(next()));
        } else if (value == "--protected-refine-ms") {
            options.protectedRefineBudget =
                std::chrono::milliseconds{std::stoll(next())};
        } else if (value == "--protected-wait-ms") {
            options.protectedWaitBudget =
                std::chrono::milliseconds{std::stoll(next())};
        } else if (value == "--terminal-sparse-ms") {
            options.terminalSparseBudget =
                std::chrono::milliseconds{std::stoll(next())};
        } else if (value == "--public-window-probe-ms") {
            options.publicWindowProbeBudget =
                std::chrono::milliseconds{std::stoll(next())};
        } else if (value == "--post-ack-ms") {
            options.postAckBudget =
                std::chrono::milliseconds{std::stoll(next())};
        } else if (value == "--post-ack-slice-ms") {
            options.postAckSliceBudget =
                std::chrono::milliseconds{std::stoll(next())};
        } else if (value == "--public-window-apply") {
            const std::string enabled = next();
            if (enabled != "0" && enabled != "1") {
                throw std::invalid_argument(
                    "--public-window-apply must be 0 or 1");
            }
            options.publicWindowApply = enabled == "1";
        } else if (value == "--checkpoint-closed-loop") {
            const std::string enabled = next();
            if (enabled != "0" && enabled != "1") {
                throw std::invalid_argument(
                    "--checkpoint-closed-loop must be 0 or 1");
            }
            options.checkpointClosedLoop = enabled == "1";
        } else if (value == "--spot-count") {
            options.spotCount = std::stoi(next());
        } else if (value == "--players") {
            options.playersOverride = std::stoi(next());
        } else if (value == "--sparse-route-states") {
            options.sparseRouteStates = std::stoull(next());
        } else if (value == "--permuted-terminal-probe-states") {
            options.permutedTerminalProbeStates = std::stoull(next());
        } else if (value == "--causal-permuted-terminal-apply-states") {
            options.causalPermutedTerminalApplyStates = std::stoull(next());
        } else if (value == "--fuel-profile") {
            options.fuelProfile = next();
        } else if (value == "--protected-wait-detours") {
            options.protectedWaitDetours = true;
        } else if (value == "--terminal-pair") {
            options.terminalPairExchange = std::stoi(next()) != 0;
        } else if (value == "--terminal-marginal-reservoir") {
            options.terminalMarginalReservoir = std::stoi(next()) != 0;
        } else if (value == "--midday-chain") {
            options.middayChainAdoption = std::stoi(next()) != 0;
        } else if (value == "--midday-pair") {
            options.middayPairExchange = std::stoi(next()) != 0;
        } else if (value == "--midday-target-followup") {
            options.middayTargetTerminalFollowup = std::stoi(next()) != 0;
        } else if (value == "--short-role-fallback") {
            options.shortRoleFallback = std::stoi(next()) != 0;
        } else if (value == "--role-details") {
            options.roleDetails = true;
        } else if (value == "--protected-wait-closed-loop") {
            options.protectedWaitDetours = true;
            options.protectedWaitClosedLoop = true;
        } else if (value == "--day-details") {
            options.dayDetails = true;
        } else {
            throw std::invalid_argument("unknown argument: " + value);
        }
    }
    if (options.version.empty() || options.track.empty() ||
        options.seedCount <= 0 || options.dayBudget.count() <= 0 ||
        options.roleBudget.count() <= 0 ||
        options.protectedRefineBudget.count() < 0 ||
        options.protectedWaitBudget.count() < 0 ||
        options.terminalSparseBudget.count() < 0 ||
        options.publicWindowProbeBudget.count() < 0 ||
        options.postAckBudget.count() < 0 ||
        options.postAckSliceBudget.count() <= 0 ||
        options.spotCount < 0 || options.playersOverride < 0) {
        throw std::invalid_argument(
            "--version, --track, positive --seeds, --budget-ms and --role-ms are required");
    }
    if (options.checkpointClosedLoop &&
        !options.protectedWaitClosedLoop) {
        throw std::invalid_argument(
            "--checkpoint-closed-loop requires --protected-wait-closed-loop");
    }
    if (options.causalPermutedTerminalApplyStates > 0U &&
        (!options.checkpointClosedLoop ||
         !options.protectedWaitClosedLoop)) {
        throw std::invalid_argument(
            "--causal-permuted-terminal-apply-states requires checkpoint and protected-wait closed loop");
    }
    if (options.sparseRouteStates > 0U &&
        (options.protectedRefineBudget.count() > 0 ||
         options.protectedWaitDetours ||
         options.terminalSparseBudget.count() > 0 ||
         options.publicWindowProbeBudget.count() > 0 ||
         options.postAckBudget.count() > 0)) {
        throw std::invalid_argument(
            "--sparse-route-states is isolated attribution and cannot be mixed with refiners or post-ACK work");
    }
    if (options.roleMode != "exhaustive" &&
        options.roleMode != "deadline" &&
        options.roleMode != "native" &&
        options.roleMode != "fixed") {
        throw std::invalid_argument(
            "--role-mode must be exhaustive, deadline, native or fixed");
    }
    if (options.fuelProfile != "generated" &&
        options.fuelProfile != "default" &&
        options.fuelProfile != "high" &&
        options.fuelProfile != "low") {
        throw std::invalid_argument(
            "--fuel-profile must be generated, default, high or low");
    }
    return options;
}

void print_result(
    const FixtureSpec& fixture,
    const Options& options,
    const Metrics& metrics) {
    const std::int64_t totalMilliseconds = std::accumulate(
        metrics.responseTimes.begin(),
        metrics.responseTimes.end(),
        std::int64_t{0});
    const std::int64_t meanMilliseconds = metrics.responseTimes.empty()
        ? 0
        : totalMilliseconds /
            static_cast<std::int64_t>(metrics.responseTimes.size());
    std::cout << "result"
              << ",version=" << options.version
              << ",track=" << options.track
              << ",suite=" << options.suite
              << ",budget_ms=" << options.dayBudget.count()
              << ",harvest_mode=" << kHarnessHarvestMode
              << ",future_harvest_mode=" << kHarnessFutureHarvestMode
              << ",role_mode=" << options.roleMode
              << ",players=" << fixture.players
              << ",spot_count=" << fixture.spots.size()
              << ",fuel_profile=" << options.fuelProfile
              << ",fixture=" << fixture.name
              << ",family=" << fixture.family
              << ",seed=" << fixture.seed
              << ",lifetime=" << metrics.score.lifetimeDistinct
              << ",daily=" << metrics.score.totalDailyDistinct
              << ",servings=" << metrics.score.totalServings
              << ",invalid=" << metrics.invalidPlans
              << ",emergency=" << metrics.emergencyDays
              << ",search_complete_days=" << metrics.searchCompleteDays
              << ",search_deadline_days=" << metrics.searchDeadlineDays
              << ",today_open_tiers="
              << metrics.todayOpenTierCounts.at(0) << '/'
              << metrics.todayOpenTierCounts.at(1) << '/'
              << metrics.todayOpenTierCounts.at(2) << '/'
              << metrics.todayOpenTierCounts.at(3)
              << ",candidate_open_tiers="
              << metrics.candidateOpenTierCounts.at(0) << '/'
              << metrics.candidateOpenTierCounts.at(1) << '/'
              << metrics.candidateOpenTierCounts.at(2) << '/'
              << metrics.candidateOpenTierCounts.at(3)
              << ",viability_open_tiers="
              << metrics.viabilityOpenTierCounts.at(0) << '/'
              << metrics.viabilityOpenTierCounts.at(1) << '/'
              << metrics.viabilityOpenTierCounts.at(2) << '/'
              << metrics.viabilityOpenTierCounts.at(3)
              << ",absolute_open_tiers="
              << metrics.absoluteOpenTierCounts.at(0) << '/'
              << metrics.absoluteOpenTierCounts.at(1) << '/'
              << metrics.absoluteOpenTierCounts.at(2) << '/'
              << metrics.absoluteOpenTierCounts.at(3)
              << ",invalid_optimality_envelopes="
              << metrics.invalidOptimalityEnvelopes
              << ",viability_deadline_days="
              << metrics.viabilityDeadlineDays
              << ",combinations=" << metrics.combinationsVisited
              << ",role_mask=" << metrics.roleMask
              << ",exact_supported_agent_days="
              << std::accumulate(
                     metrics.exactSupportedAgents.begin(),
                     metrics.exactSupportedAgents.end(),
                     std::int64_t{0})
              << ",exact_settled="
              << std::accumulate(
                     metrics.exactSettledStates.begin(),
                     metrics.exactSettledStates.end(),
                     std::uint64_t{0})
              << ",role_ms=" << metrics.roleMilliseconds
              << ",protected_terminal_attempts="
              << metrics.protectedTerminalAttempts
              << ",protected_terminal_takeovers="
              << metrics.protectedTerminalTakeovers
              << ",protected_terminal_invalid="
              << metrics.protectedTerminalInvalid
              << ",protected_terminal_parent="
              << metrics.protectedTerminalParentScore.lifetimeDistinct << '/'
              << metrics.protectedTerminalParentScore.totalDailyDistinct << '/'
              << metrics.protectedTerminalParentScore.totalServings
              << ",protected_terminal_refined="
              << metrics.protectedTerminalRefinedScore.lifetimeDistinct << '/'
              << metrics.protectedTerminalRefinedScore.totalDailyDistinct << '/'
              << metrics.protectedTerminalRefinedScore.totalServings
              << ",protected_wait_plans=" << metrics.protectedWaitPlans
              << ",protected_wait_valid=" << metrics.protectedWaitValid
              << ",protected_wait_liftable=" << metrics.protectedWaitLiftable
              << ",protected_wait_best_day=" << metrics.protectedWaitBestDay
              << ",protected_wait_parent="
              << metrics.protectedWaitParentScore.lifetimeDistinct << '/'
              << metrics.protectedWaitParentScore.totalDailyDistinct << '/'
              << metrics.protectedWaitParentScore.totalServings
              << ",protected_wait_best="
              << metrics.protectedWaitBestScore.lifetimeDistinct << '/'
              << metrics.protectedWaitBestScore.totalDailyDistinct << '/'
              << metrics.protectedWaitBestScore.totalServings
              << ",protected_wait_takeovers="
              << metrics.protectedWaitTakeovers
              << ",protected_wait_deadline_days="
              << metrics.protectedWaitDeadlineDays
              << ",protected_virtual_score="
              << metrics.protectedVirtualScore.lifetimeDistinct << '/'
              << metrics.protectedVirtualScore.totalDailyDistinct << '/'
              << metrics.protectedVirtualScore.totalServings
              << ",protected_wait_witness_agent="
              << metrics.protectedWaitWitnessAgent
              << ",protected_wait_witness_anchor="
              << metrics.protectedWaitWitnessAnchor
              << ",protected_wait_witness_spot="
              << metrics.protectedWaitWitnessSpot
              << ",protected_wait_witness_duration="
              << metrics.protectedWaitWitnessDuration
              << ",protected_wait_witness_travel="
              << metrics.protectedWaitWitnessTravelSteps
              << ",protected_wait_witness_fuel="
              << metrics.protectedWaitWitnessParentFuel << '/'
              << metrics.protectedWaitWitnessCandidateFuel
              << ",protected_wait_witness_hash="
              << metrics.protectedWaitWitnessPlanHash
              << ",terminal_sparse_routes="
              << metrics.terminalSparseRoutes
              << ",terminal_sparse_plans="
              << metrics.terminalSparsePlans
              << ",terminal_sparse_valid="
              << metrics.terminalSparseValid
              << ",terminal_sparse_strict="
              << metrics.terminalSparseStrict
              << ",terminal_pair_acceptances="
              << metrics.terminalPairAcceptances
              << ",terminal_sparse_rounds="
              << metrics.terminalSparseRounds
              << ",terminal_sparse_takeovers="
              << metrics.terminalSparseTakeovers
              << ",terminal_sparse_deadline="
              << metrics.terminalSparseDeadline
              << ",terminal_sparse_failure="
              << metrics.terminalSparseFailure
              << ",terminal_sparse_parent="
              << metrics.terminalSparseParentScore.lifetimeDistinct << '/'
              << metrics.terminalSparseParentScore.totalDailyDistinct << '/'
              << metrics.terminalSparseParentScore.totalServings
              << ",terminal_sparse_round1="
              << metrics.terminalSparseFirstRoundScore.lifetimeDistinct << '/'
              << metrics.terminalSparseFirstRoundScore.totalDailyDistinct << '/'
              << metrics.terminalSparseFirstRoundScore.totalServings
              << ",terminal_sparse_refined="
              << metrics.terminalSparseRefinedScore.lifetimeDistinct << '/'
              << metrics.terminalSparseRefinedScore.totalDailyDistinct << '/'
              << metrics.terminalSparseRefinedScore.totalServings
              << ",terminal_marginal_routes="
              << metrics.terminalMarginalRoutes
              << ",terminal_marginal_plans="
              << metrics.terminalMarginalPlans
              << ",terminal_marginal_valid="
              << metrics.terminalMarginalValid
              << ",terminal_marginal_acceptances="
              << metrics.terminalMarginalAcceptances
              << ",terminal_marginal_rounds="
              << metrics.terminalMarginalRounds
              << ",terminal_marginal_deadline="
              << metrics.terminalMarginalDeadline
              << ",terminal_marginal_failure="
              << metrics.terminalMarginalFailure
              << ",terminal_marginal_parent="
              << metrics.terminalMarginalParentScore.lifetimeDistinct << '/'
              << metrics.terminalMarginalParentScore.totalDailyDistinct << '/'
              << metrics.terminalMarginalParentScore.totalServings
              << ",terminal_marginal_refined="
              << metrics.terminalMarginalRefinedScore.lifetimeDistinct << '/'
              << metrics.terminalMarginalRefinedScore.totalDailyDistinct << '/'
              << metrics.terminalMarginalRefinedScore.totalServings
              << ",midday_routes=" << metrics.middayRoutes
              << ",midday_plans=" << metrics.middayPlans
              << ",midday_valid=" << metrics.middayValid
              << ",midday_chain_acceptances="
              << metrics.middayChainAcceptances
              << ",midday_pair_acceptances="
              << metrics.middayPairAcceptances
              << ",midday_rounds=" << metrics.middayRounds
              << ",midday_target_routes=" << metrics.middayTargetRoutes
              << ",midday_target_plans=" << metrics.middayTargetPlans
              << ",midday_target_valid=" << metrics.middayTargetValid
              << ",midday_target_acceptances="
              << metrics.middayTargetAcceptances
              << ",midday_target_rounds=" << metrics.middayTargetRounds
              << ",midday_takeovers=" << metrics.middayTakeovers
              << ",midday_deadline_days=" << metrics.middayDeadlineDays
              << ",midday_failure_days=" << metrics.middayFailureDays
              << ",midday_best_day=" << metrics.middayBestDay
              << ",public_window_probe_days="
              << metrics.publicWindowProbeDays
              << ",public_window_probe_takeovers="
              << metrics.publicWindowProbeTakeovers
              << ",public_window_probe_deadline_days="
              << metrics.publicWindowProbeDeadlineDays
              << ",public_window_probe_failure_days="
              << metrics.publicWindowProbeFailureDays
              << ",public_window_probe_tier1_gains="
              << metrics.publicWindowProbeTier1Gains
              << ",public_window_probe_tier2_gains="
              << metrics.publicWindowProbeTier2Gains
              << ",public_window_probe_serving_gain="
              << metrics.publicWindowProbeServingGain
              << ",public_window_probe_routes="
              << metrics.publicWindowProbeRoutes
              << ",public_window_probe_plans="
              << metrics.publicWindowProbePlans
              << ",public_window_probe_valid="
              << metrics.publicWindowProbeValid
              << ",public_window_probe_acceptances="
              << metrics.publicWindowProbeAcceptances
              << ",public_window_apply="
              << (options.publicWindowApply ? 1 : 0)
              << ",checkpoint_closed_loop="
              << (options.checkpointClosedLoop ? 1 : 0)
              << ",checkpoint_closed_loop_parent="
              << metrics.checkpointClosedLoopParentScore.lifetimeDistinct
              << '/'
              << metrics.checkpointClosedLoopParentScore.totalDailyDistinct
              << '/'
              << metrics.checkpointClosedLoopParentScore.totalServings
              << ",checkpoint_closed_loop_takeovers="
              << metrics.checkpointClosedLoopTakeovers
              << ",checkpoint_closed_loop_failures="
              << metrics.checkpointClosedLoopFailures
              << ",sparse_route_states="
              << options.sparseRouteStates
              << ",sparse_route_representatives="
              << metrics.sparseRouteRepresentatives
              << ",sparse_routes=" << metrics.sparseRoutes
              << ",sparse_valid=" << metrics.sparseValidPlans
              << ",sparse_takeovers=" << metrics.sparseTakeovers
              << ",sparse_serving_gain=" << metrics.sparseServingGain
              << ",permuted_probe_states="
              << options.permutedTerminalProbeStates
              << ",permuted_probe_tasks="
              << metrics.permutedProbeTasks
              << ",permuted_probe_routes="
              << metrics.permutedProbeRoutes
              << ",permuted_probe_cross_pairs="
              << metrics.permutedProbeCrossPairs
              << ",permuted_probe_valid="
              << metrics.permutedProbeValidPlans
              << ",permuted_probe_certified="
              << metrics.permutedProbeCertified
              << ",permuted_probe_yield_days="
              << metrics.permutedProbeYieldDays
              << ",permuted_probe_lifetime_gain="
              << metrics.permutedProbeLifetimeGain
              << ",permuted_probe_daily_gain="
              << metrics.permutedProbeDailyGain
              << ",permuted_probe_serving_gain="
              << metrics.permutedProbeServingGain
              << ",permuted_probe_mutation_failures="
              << metrics.permutedProbeMutationFailures
              << ",permuted_causal_parent_lifetime="
              << metrics.permutedCausalParentScore.lifetimeDistinct
              << ",permuted_causal_parent_daily="
              << metrics.permutedCausalParentScore.totalDailyDistinct
              << ",permuted_causal_parent_servings="
              << metrics.permutedCausalParentScore.totalServings
              << ",permuted_apply_states="
              << options.causalPermutedTerminalApplyStates
              << ",permuted_apply_tasks="
              << metrics.permutedApplyTasks
              << ",permuted_apply_routes="
              << metrics.permutedApplyRoutes
              << ",permuted_apply_cross_pairs="
              << metrics.permutedApplyCrossPairs
              << ",permuted_apply_valid="
              << metrics.permutedApplyValidPlans
              << ",permuted_apply_certified="
              << metrics.permutedApplyCertified
              << ",permuted_apply_takeovers="
              << metrics.permutedApplyTakeovers
              << ",permuted_apply_lifetime_gain="
              << metrics.permutedApplyLifetimeGain
              << ",permuted_apply_daily_gain="
              << metrics.permutedApplyDailyGain
              << ",permuted_apply_serving_gain="
              << metrics.permutedApplyServingGain
              << ",permuted_apply_deadline_days="
              << metrics.permutedApplyDeadlineDays
              << ",permuted_apply_mapping_failures="
              << metrics.permutedApplyMappingFailures
              << ",permuted_apply_role_failures="
              << metrics.permutedApplyRoleFailures
              << ",permuted_apply_shadow_state_failures="
              << metrics.permutedApplyShadowStateFailures
              << ",permuted_apply_shadow_ledger_failures="
              << metrics.permutedApplyShadowLedgerFailures
              << ",permuted_apply_uncertified_takeovers="
              << metrics.permutedApplyUncertifiedTakeovers
              << ",permuted_causal_noop_failures="
              << metrics.permutedCausalNoopFailures
              << ",cache_eligible=" << metrics.cacheEligible
              << ",cache_reused=" << metrics.cacheReused
              << ",cache_rejected=" << metrics.cacheRejected
              << ",post_ack_calls=" << metrics.postAckCalls
              << ",post_ack_contingencies="
              << metrics.postAckContingencies
              << ",post_ack_proof_calls="
              << metrics.postAckProofCalls
              << ",post_ack_completed_proofs="
              << metrics.postAckCompletedProofs
              << ",post_ack_strong_proof_records="
              << metrics.postAckStrongProofRecords
              << ",mean_ms=" << meanMilliseconds
              << ",p95_ms=" << percentile(metrics.responseTimes, 95)
              << ",max_ms=" << percentile(metrics.responseTimes, 100)
              << '\n';
    if (options.dayDetails) {
        for (std::size_t day = 0; day < metrics.exactDayScores.size(); ++day) {
            const udon::DayScore& exact = metrics.exactDayScores.at(day);
            const udon::OfficialScore& cumulative =
                metrics.cumulativeDayScores.at(day);
            const udon::OptimalityGapDiagnostics& gap =
                metrics.optimalityGaps.at(day);
            std::cout << "day_detail"
                      << ",version=" << options.version
                      << ",track=" << options.track
                      << ",seed=" << fixture.seed
                      << ",day=" << day + 1U
                      << ",daily=" << exact.dailyDistinct
                      << '/' << exact.servings
                      << ",cumulative=" << cumulative.lifetimeDistinct
                      << '/' << cumulative.totalDailyDistinct
                      << '/' << cumulative.totalServings
                      << ",virtual_cumulative=";
            if (day < metrics.protectedVirtualDayScores.size()) {
                const udon::OfficialScore& virtualCumulative =
                    metrics.protectedVirtualDayScores.at(day);
                std::cout << virtualCumulative.lifetimeDistinct << '/'
                          << virtualCumulative.totalDailyDistinct << '/'
                          << virtualCumulative.totalServings;
            } else {
                std::cout << "0/0/0";
            }
            std::cout
                      << ",causal_parent_cumulative="
                      << metrics.permutedCausalDayScores.at(day).lifetimeDistinct
                      << '/'
                      << metrics.permutedCausalDayScores.at(day).totalDailyDistinct
                      << '/'
                      << metrics.permutedCausalDayScores.at(day).totalServings
                      << ",causal_parent_plan_hash="
                      << metrics.permutedCausalPlanHashes.at(day)
                      << ",plan_hash=" << metrics.planHashes.at(day)
                      << ",response_ms=" << metrics.responseTimes.at(day)
                      << ",deadline=" << (metrics.deadlineDays.at(day) ? 1 : 0)
                      << ",exact_supported="
                      << metrics.exactSupportedAgents.at(day)
                      << ",exact_settled="
                      << metrics.exactSettledStates.at(day)
                      << ",exact_seed_servings="
                      << metrics.exactSeedServings.at(day)
                      << ",exact_local_servings="
                      << metrics.exactLocalServings.at(day)
                      << ",portfolio_search_complete="
                      << (gap.portfolioSearchComplete ? 1 : 0)
                      << ",viability_deadline="
                      << (gap.viabilityDeadlineReached ? 1 : 0)
                      << ",today_lower="
                      << gap.todayPortfolio.lowerBound.lifetimeDistinct << '/'
                      << gap.todayPortfolio.lowerBound.totalDailyDistinct << '/'
                      << gap.todayPortfolio.lowerBound.totalServings
                      << ",today_upper="
                      << gap.todayPortfolio.upperBound.lifetimeDistinct << '/'
                      << gap.todayPortfolio.upperBound.totalDailyDistinct << '/'
                      << gap.todayPortfolio.upperBound.totalServings
                      << ",today_gap="
                      << gap.todayPortfolio.componentGaps.at(0) << '/'
                      << gap.todayPortfolio.componentGaps.at(1) << '/'
                      << gap.todayPortfolio.componentGaps.at(2)
                      << ",today_open_tier="
                      << gap.todayPortfolio.firstOpenTier
                      << ",today_valid_envelope="
                      << (gap.todayPortfolio.validEnvelope ? 1 : 0)
                      << ",candidate_lower="
                      << gap.candidateHorizon.lowerBound.lifetimeDistinct << '/'
                      << gap.candidateHorizon.lowerBound.totalDailyDistinct << '/'
                      << gap.candidateHorizon.lowerBound.totalServings
                      << ",candidate_upper="
                      << gap.candidateHorizon.upperBound.lifetimeDistinct << '/'
                      << gap.candidateHorizon.upperBound.totalDailyDistinct << '/'
                      << gap.candidateHorizon.upperBound.totalServings
                      << ",candidate_gap="
                      << gap.candidateHorizon.componentGaps.at(0) << '/'
                      << gap.candidateHorizon.componentGaps.at(1) << '/'
                      << gap.candidateHorizon.componentGaps.at(2)
                      << ",candidate_open_tier="
                      << gap.candidateHorizon.firstOpenTier
                      << ",candidate_valid_envelope="
                      << (gap.candidateHorizon.validEnvelope ? 1 : 0)
                      << ",viability_lower="
                      << gap.viabilityHorizon.lowerBound.lifetimeDistinct << '/'
                      << gap.viabilityHorizon.lowerBound.totalDailyDistinct << '/'
                      << gap.viabilityHorizon.lowerBound.totalServings
                      << ",viability_upper="
                      << gap.viabilityHorizon.upperBound.lifetimeDistinct << '/'
                      << gap.viabilityHorizon.upperBound.totalDailyDistinct << '/'
                      << gap.viabilityHorizon.upperBound.totalServings
                      << ",viability_gap="
                      << gap.viabilityHorizon.componentGaps.at(0) << '/'
                      << gap.viabilityHorizon.componentGaps.at(1) << '/'
                      << gap.viabilityHorizon.componentGaps.at(2)
                      << ",viability_open_tier="
                      << gap.viabilityHorizon.firstOpenTier
                      << ",viability_valid_envelope="
                      << (gap.viabilityHorizon.validEnvelope ? 1 : 0)
                      << ",absolute_lower="
                      << gap.absoluteHorizon.lowerBound.lifetimeDistinct << '/'
                      << gap.absoluteHorizon.lowerBound.totalDailyDistinct << '/'
                      << gap.absoluteHorizon.lowerBound.totalServings
                      << ",absolute_upper="
                      << gap.absoluteHorizon.upperBound.lifetimeDistinct << '/'
                      << gap.absoluteHorizon.upperBound.totalDailyDistinct << '/'
                      << gap.absoluteHorizon.upperBound.totalServings
                      << ",absolute_gap="
                      << gap.absoluteHorizon.componentGaps.at(0) << '/'
                      << gap.absoluteHorizon.componentGaps.at(1) << '/'
                      << gap.absoluteHorizon.componentGaps.at(2)
                      << ",absolute_open_tier="
                      << gap.absoluteHorizon.firstOpenTier
                      << ",absolute_valid_envelope="
                      << (gap.absoluteHorizon.validEnvelope ? 1 : 0)
                      << '\n';
        }
    }
}

}  // namespace

int main(int argumentCount, char** arguments) {
    try {
        const Options options = parse_options(argumentCount, arguments);
        for (std::int32_t offset = 0; offset < options.seedCount; ++offset) {
            FixtureSpec fixture = make_fixture(
                options,
                options.firstSeed + static_cast<std::uint64_t>(offset));
            if (options.playersOverride > 0) {
                fixture.players = options.playersOverride;
            }
            print_result(fixture, options, run_fixture(fixture, options));
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "historical tournament failed: " << error.what() << '\n';
        return 1;
    }
}
