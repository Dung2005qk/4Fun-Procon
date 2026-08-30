#include "udon/slack_refiner.hpp"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstddef>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "udon/orienteering.hpp"

namespace udon {
namespace {

struct WaitAnchor {
    std::size_t actionIndex = 0;
    CellId cell = kInvalidCell;
    std::int32_t duration = 0;
};

[[nodiscard]] std::uint64_t plan_hash(const DayPlan& plan) {
    std::uint64_t hash = 1469598103934665603ULL;
    const auto mix = [&hash](std::uint64_t value) {
        hash ^= value;
        hash *= 1099511628211ULL;
    };
    mix(plan.actions.size());
    for (const AgentPlan& actions : plan.actions) {
        mix(actions.size());
        for (const PlanAction& action : actions) {
            mix(static_cast<std::uint32_t>(action.wire_value()));
        }
    }
    return hash;
}

[[nodiscard]] std::vector<WaitAnchor> wait_anchors(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agent,
    const AgentPlan& plan) {
    std::vector<WaitAnchor> anchors;
    CellId cell = state.agents.at(static_cast<std::size_t>(agent)).position;
    for (std::size_t actionIndex = 0;
         actionIndex < plan.size();
         ++actionIndex) {
        const PlanAction& action = plan.at(actionIndex);
        if (action.kind == ActionKind::Wait) {
            if (action.value > 0) {
                anchors.push_back(WaitAnchor{actionIndex, cell, action.value});
            }
            continue;
        }
        cell = config.map.neighbors.at(static_cast<std::size_t>(cell)).at(
            static_cast<std::size_t>(action.value));
        if (cell == kInvalidCell) {
            throw std::runtime_error(
                "protected slack refinement encountered an invalid incumbent move");
        }
    }
    return anchors;
}

[[nodiscard]] bool path_avoids_roads(
    const MatchConfig& config,
    CellId source,
    const std::vector<std::int32_t>& directions,
    CellId expectedTerminal) {
    CellId cell = source;
    for (const std::int32_t direction : directions) {
        if (config.map.terrain.at(static_cast<std::size_t>(cell)) ==
            Terrain::Road) {
            return false;
        }
        cell = config.map.neighbors.at(static_cast<std::size_t>(cell)).at(
            static_cast<std::size_t>(direction));
        if (cell == kInvalidCell ||
            config.map.terrain.at(static_cast<std::size_t>(cell)) ==
                Terrain::Pond) {
            return false;
        }
    }
    return cell == expectedTerminal &&
        config.map.terrain.at(static_cast<std::size_t>(cell)) != Terrain::Road;
}

[[nodiscard]] bool strict_protected_improvement(
    const MatchLedger& ledger,
    const SimulationResult& incumbent,
    const SimulationResult& challenger) {
    if (!protected_slack_transition_dominates(incumbent, challenger)) {
        return false;
    }
    const BrandMask incumbentLifetime =
        ledger.lifetimeBrands | incumbent.score.brands;
    const BrandMask challengerLifetime =
        ledger.lifetimeBrands | challenger.score.brands;
    if (brand_difference(incumbentLifetime, challengerLifetime).any() ||
        challenger.score.dailyDistinct < incumbent.score.dailyDistinct ||
        challenger.score.servings < incumbent.score.servings) {
        return false;
    }
    return challenger.score.dailyDistinct > incumbent.score.dailyDistinct ||
        challenger.score.servings > incumbent.score.servings;
}

} // namespace

bool protected_slack_transition_dominates(
    const SimulationResult& baseline,
    const SimulationResult& actual) {
    if (!baseline.valid || !actual.valid ||
        baseline.roadFootprint != actual.roadFootprint) {
        return false;
    }
    return protected_slack_agents_dominate(
        baseline.finalAgents,
        actual.finalAgents);
}

bool protected_slack_agents_dominate(
    const std::vector<AgentState>& baseline,
    const std::vector<AgentState>& actual) {
    if (baseline.size() != actual.size()) {
        return false;
    }
    for (std::size_t agent = 0;
         agent < baseline.size();
         ++agent) {
        const AgentState& left = baseline.at(agent);
        const AgentState& right = actual.at(agent);
        if (left.kind != right.kind || left.position != right.position ||
            (left.kind == AgentKind::Patrol && right.fuel < left.fuel)) {
            return false;
        }
    }
    return true;
}

bool protected_slack_ledger_dominates(
    const MatchLedger& baseline,
    const MatchLedger& actual) {
    return !brand_difference(baseline.lifetimeBrands, actual.lifetimeBrands).any() &&
        actual.totalDailyDistinct >= baseline.totalDailyDistinct &&
        actual.totalServings >= baseline.totalServings;
}

bool protected_slack_ledger_relation_for_day(
    const MatchLedger& baseline,
    const MatchLedger& actual,
    bool terminalDay) {
    if (!terminalDay) {
        return protected_slack_ledger_dominates(baseline, actual);
    }
    const OfficialScore baselineScore{
        baseline.lifetime_distinct(),
        baseline.totalDailyDistinct,
        baseline.totalServings,
    };
    const OfficialScore actualScore{
        actual.lifetime_distinct(),
        actual.totalDailyDistinct,
        actual.totalServings,
    };
    return !(actualScore < baselineScore);
}

ProtectedSlackRefiner::ProtectedSlackRefiner(const MatchConfig& config)
    : config_(config),
      router_(config_),
      simulator_(config_),
      validator_(config_) {}

ProtectedSlackResult ProtectedSlackRefiner::refine_wait_detours(
    const DayState& state,
    const MatchLedger& ledger,
    const DayPlan& incumbentPlan,
    std::chrono::steady_clock::time_point deadline) const {
    const SimulationResult incumbentSimulation =
        simulator_.simulate(state, incumbentPlan, false);
    const SimulationResult independent =
        validator_.validate(state, incumbentPlan, false);
    std::string mismatch;
    if (!incumbentSimulation.valid ||
        !validator_.agrees_with(
            incumbentSimulation,
            independent,
            mismatch)) {
        ProtectedSlackResult result;
        result.plan = incumbentPlan;
        result.simulation = incumbentSimulation;
        result.scoreAfterToday =
            OfficialScore::after_day(ledger, incumbentSimulation.score);
        return result;
    }
    return refine_wait_detours(
        state,
        ledger,
        incumbentPlan,
        incumbentSimulation,
        deadline);
}

ProtectedSlackResult ProtectedSlackRefiner::refine_wait_detours(
    const DayState& state,
    const MatchLedger& ledger,
    const DayPlan& incumbentPlan,
    const SimulationResult& incumbentSimulation,
    std::chrono::steady_clock::time_point deadline) const {
    ProtectedSlackResult result;
    result.plan = incumbentPlan;
    result.simulation = incumbentSimulation;
    result.scoreAfterToday =
        OfficialScore::after_day(ledger, incumbentSimulation.score);
    if (!incumbentSimulation.valid ||
        incumbentPlan.actions.size() !=
            static_cast<std::size_t>(config_.agent_count())) {
        return result;
    }

    std::set<std::uint64_t> planHashes;
    const auto deadline_reached = [&]() {
        if (std::chrono::steady_clock::now() < deadline) {
            return false;
        }
        result.diagnostics.deadlineReached = true;
        return true;
    };

    for (AgentIndex agent = 0; agent < config_.agent_count(); ++agent) {
        if (deadline_reached()) {
            return result;
        }
        if (state.agents.at(static_cast<std::size_t>(agent)).kind !=
            AgentKind::Patrol) {
            continue;
        }
        const AgentPlan& incumbentAgentPlan =
            incumbentPlan.actions.at(static_cast<std::size_t>(agent));
        for (const WaitAnchor& wait :
             wait_anchors(config_, state, agent, incumbentAgentPlan)) {
            ++result.diagnostics.waitAnchors;
            if (deadline_reached()) {
                return result;
            }
            if (wait.duration < 2 ||
                config_.map.terrain.at(static_cast<std::size_t>(wait.cell)) ==
                    Terrain::Road) {
                continue;
            }
            ++result.diagnostics.eligibleWaitAnchors;
            ParetoSearchOptions routeOptions;
            routeOptions.maximumTravelSteps = wait.duration;
            routeOptions.maximumPatrolFuel =
                state.agents.at(static_cast<std::size_t>(agent)).fuel;
            routeOptions.maximumLabelsPerCell = 64;
            routeOptions.maximumPaths = 12;
            routeOptions.patrol = true;
            for (const Spot& spot : config_.spots) {
                if (deadline_reached()) {
                    return result;
                }
                if (spot.position == wait.cell) {
                    continue;
                }
                const std::vector<ParetoPath> outbound = router_.find_paths(
                    wait.cell,
                    spot.position,
                    state.roadStatuses,
                    routeOptions);
                const std::vector<ParetoPath> inbound = router_.find_paths(
                    spot.position,
                    wait.cell,
                    state.roadStatuses,
                    routeOptions);
                for (const ParetoPath& out : outbound) {
                    if (deadline_reached()) {
                        return result;
                    }
                    if (!path_avoids_roads(
                            config_,
                            wait.cell,
                            out.directions,
                            spot.position)) {
                        continue;
                    }
                    for (const ParetoPath& back : inbound) {
                        ++result.diagnostics.routePairs;
                        if (deadline_reached()) {
                            return result;
                        }
                        const std::int32_t usedSteps =
                            out.travelSteps + back.travelSteps;
                        if (usedSteps > wait.duration ||
                            !path_avoids_roads(
                                config_,
                                spot.position,
                                back.directions,
                                wait.cell)) {
                            continue;
                        }

                        DayPlan candidate = incumbentPlan;
                        AgentPlan replacement;
                        replacement.reserve(
                            out.directions.size() + back.directions.size() + 1U);
                        for (const std::int32_t direction : out.directions) {
                            replacement.push_back(PlanAction::move(direction));
                        }
                        for (const std::int32_t direction : back.directions) {
                            replacement.push_back(PlanAction::move(direction));
                        }
                        if (usedSteps < wait.duration) {
                            replacement.push_back(
                                PlanAction::wait(wait.duration - usedSteps));
                        }
                        AgentPlan& candidateAgent = candidate.actions.at(
                            static_cast<std::size_t>(agent));
                        candidateAgent.erase(
                            candidateAgent.begin() +
                            static_cast<std::ptrdiff_t>(wait.actionIndex));
                        candidateAgent.insert(
                            candidateAgent.begin() +
                                static_cast<std::ptrdiff_t>(wait.actionIndex),
                            replacement.begin(),
                            replacement.end());

                        if (!planHashes.insert(plan_hash(candidate)).second) {
                            continue;
                        }
                        ++result.diagnostics.generatedPlans;
                        const SimulationResult detailed =
                            simulator_.simulate(state, candidate, false);
                        const SimulationResult independent =
                            validator_.validate(state, candidate, false);
                        std::string mismatch;
                        if (!detailed.valid ||
                            !validator_.agrees_with(
                                detailed,
                                independent,
                                mismatch)) {
                            continue;
                        }
                        ++result.diagnostics.validPlans;
                        if (!strict_protected_improvement(
                                ledger,
                                incumbentSimulation,
                                detailed)) {
                            continue;
                        }
                        ++result.diagnostics.liftablePlans;
                        const OfficialScore candidateScore =
                            OfficialScore::after_day(ledger, detailed.score);
                        if (result.improved &&
                            !(result.scoreAfterToday < candidateScore)) {
                            continue;
                        }
                        result.plan = std::move(candidate);
                        result.simulation = detailed;
                        result.scoreAfterToday = candidateScore;
                        result.improved = true;
                        result.witnessAgent = agent;
                        result.witnessAnchor = wait.cell;
                        result.witnessSpot = spot.position;
                        result.witnessDuration = wait.duration;
                        result.witnessTravelSteps = usedSteps;
                        result.witnessParentFuel =
                            incumbentSimulation.finalAgents.at(
                                static_cast<std::size_t>(agent)).fuel;
                        result.witnessCandidateFuel =
                            detailed.finalAgents.at(
                                static_cast<std::size_t>(agent)).fuel;
                    }
                }
            }
        }
    }
    return result;
}

ProtectedSlackResult ProtectedSlackRefiner::refine_terminal_sparse(
    const DayState& state,
    const MatchLedger& ledger,
    const DayPlan& incumbentPlan,
    const SimulationResult& incumbentSimulation,
    std::chrono::steady_clock::time_point deadline) const {
    ProtectedSlackResult result;
    result.plan = incumbentPlan;
    result.simulation = incumbentSimulation;
    result.scoreAfterToday =
        OfficialScore::after_day(ledger, incumbentSimulation.score);
    result.firstRoundScore = result.scoreAfterToday;
    result.canonicalTerminalScore = result.scoreAfterToday;
    if (!incumbentSimulation.valid ||
        state.dayNumber != config_.day_count() ||
        incumbentPlan.actions.size() !=
            static_cast<std::size_t>(config_.agent_count()) ||
        config_.spots.size() > 32U ||
        exact_orienteering_dense_state_supported(config_)) {
        return result;
    }
    result.diagnostics.terminalSparse = true;
    const SimulationResult independentIncumbent =
        validator_.validate(state, incumbentPlan, false);
    std::string incumbentMismatch;
    if (!validator_.agrees_with(
            incumbentSimulation,
            independentIncumbent,
            incumbentMismatch)) {
        return result;
    }
    constexpr auto kValidationReserve = std::chrono::milliseconds{80};
    const auto now = std::chrono::steady_clock::now();
    if (now + kValidationReserve >= deadline) {
        result.diagnostics.deadlineReached = true;
        return result;
    }
    const auto searchDeadline = deadline - kValidationReserve;

    BrandMask preferredBrands;
    for (std::int32_t brand = 0; brand < config_.brand_count(); ++brand) {
        if (!has_brand(ledger.lifetimeBrands, brand)) {
            preferredBrands |= brand_bit(brand);
        }
    }
    const std::int32_t minimumSpots = preferredBrands != 0U
        ? 1
        : std::min<std::int32_t>(
              std::max(1, config_.brand_count() - 1),
              static_cast<std::int32_t>(config_.spots.size()));

    const auto visited_spot_mask = [this, &state, &incumbentPlan](
                                       AgentIndex agent) {
        std::uint32_t mask = 0U;
        CellId cell = state.agents.at(static_cast<std::size_t>(agent)).position;
        const SpotIndex initialSpot =
            config_.spotAtCell.at(static_cast<std::size_t>(cell));
        if (initialSpot != kInvalidSpot) {
            mask |= std::uint32_t{1} << static_cast<std::uint32_t>(initialSpot);
        }
        for (const PlanAction& action :
             incumbentPlan.actions.at(static_cast<std::size_t>(agent))) {
            if (action.kind == ActionKind::Wait) {
                continue;
            }
            cell = config_.map.neighbors.at(static_cast<std::size_t>(cell)).at(
                static_cast<std::size_t>(action.value));
            if (cell == kInvalidCell) {
                return std::numeric_limits<std::uint32_t>::max();
            }
            const SpotIndex spot =
                config_.spotAtCell.at(static_cast<std::size_t>(cell));
            if (spot != kInvalidSpot) {
                mask |= std::uint32_t{1} << static_cast<std::uint32_t>(spot);
            }
        }
        return mask;
    };
    const auto visited_spot_count = [&visited_spot_mask](AgentIndex agent) {
        const std::uint32_t mask = visited_spot_mask(agent);
        return mask == std::numeric_limits<std::uint32_t>::max()
            ? std::numeric_limits<std::int32_t>::max()
            : static_cast<std::int32_t>(std::popcount(mask));
    };
    std::vector<AgentIndex> tasks;
    for (AgentIndex agent = 0; agent < config_.agent_count(); ++agent) {
        if (state.agents.at(static_cast<std::size_t>(agent)).kind ==
            AgentKind::Patrol) {
            tasks.push_back(agent);
        }
    }
    std::stable_sort(
        tasks.begin(),
        tasks.end(),
        [&visited_spot_count](AgentIndex left, AgentIndex right) {
            return visited_spot_count(left) < visited_spot_count(right);
        });
    std::vector<std::uint32_t> incumbentMasks(
        static_cast<std::size_t>(config_.agent_count()),
        0U);
    std::vector<std::int32_t> incumbentClaims(config_.spots.size(), 0);
    for (const AgentIndex agent : tasks) {
        const std::uint32_t mask = visited_spot_mask(agent);
        if (mask == std::numeric_limits<std::uint32_t>::max()) {
            return result;
        }
        incumbentMasks.at(static_cast<std::size_t>(agent)) = mask;
        for (std::size_t spot = 0; spot < config_.spots.size(); ++spot) {
            if ((mask &
                 (std::uint32_t{1} << static_cast<std::uint32_t>(spot))) != 0U) {
                ++incumbentClaims.at(spot);
            }
        }
    }
    std::vector<TerminalMarginalRouteContext> marginalContexts(
        static_cast<std::size_t>(config_.agent_count()));
    if (enableTerminalMarginalReservoir) {
        for (const AgentIndex agent : tasks) {
            TerminalMarginalRouteContext& context = marginalContexts.at(
                static_cast<std::size_t>(agent));
            context.lifetimeBrands = ledger.lifetimeBrands;
            context.incumbentClaimsWithoutAgent = incumbentClaims;
            const std::uint32_t ownMask = incumbentMasks.at(
                static_cast<std::size_t>(agent));
            for (std::size_t spot = 0; spot < config_.spots.size(); ++spot) {
                if ((ownMask &
                     (std::uint32_t{1} << static_cast<std::uint32_t>(spot))) !=
                    0U) {
                    --context.incumbentClaimsWithoutAgent.at(spot);
                }
            }
        }
    }
    std::vector<ExactOrienteeringReachability> reachability(
        static_cast<std::size_t>(config_.agent_count()));
    std::atomic<std::size_t> nextTask{0U};
    std::atomic<bool> workerFailed{false};
    constexpr std::size_t kWorkerLimit = 4U;
    const std::size_t workerCount = std::min(kWorkerLimit, tasks.size());
    {
        std::vector<std::jthread> workers;
        workers.reserve(workerCount);
        for (std::size_t worker = 0; worker < workerCount; ++worker) {
            workers.emplace_back([&]() {
                try {
                    while (!workerFailed.load() &&
                           std::chrono::steady_clock::now() < searchDeadline) {
                        const std::size_t task = nextTask.fetch_add(1U);
                        if (task >= tasks.size()) {
                            return;
                        }
                        const AgentIndex agent = tasks.at(task);
                        reachability.at(static_cast<std::size_t>(agent)) =
                            enumerate_sparse_anytime_resource_routes(
                                config_,
                                state,
                                agent,
                                minimumSpots,
                                32U,
                                1250000U,
                                searchDeadline,
                                preferredBrands,
                                enableTerminalMarginalReservoir
                                    ? &marginalContexts.at(
                                          static_cast<std::size_t>(agent))
                                    : nullptr);
                    }
                } catch (...) {
                    workerFailed.store(true);
                }
            });
        }
    }
    if (workerFailed.load()) {
        result.diagnostics.sparseFailure = true;
        return result;
    }

    std::set<std::uint64_t> planHashes;
    planHashes.insert(plan_hash(incumbentPlan));
    bool firstRound = true;
    const auto run_one_agent_ascent = [&]() -> bool {
    for (;;) {
        const DayPlan roundBasePlan = result.plan;
        const SimulationResult roundBaseSimulation = result.simulation;
        const OfficialScore roundBaseScore = result.scoreAfterToday;
        DayPlan roundBestPlan = roundBasePlan;
        SimulationResult roundBestSimulation = roundBaseSimulation;
        OfficialScore roundBestScore = roundBaseScore;
        AgentIndex roundBestAgent = kInvalidAgent;
        bool roundDeadline = false;

        for (const AgentIndex agent : tasks) {
            const ExactOrienteeringReachability& routes =
                reachability.at(static_cast<std::size_t>(agent));
            const auto evaluate = [&](const ExactOrienteeringRoute& route) {
                if (std::chrono::steady_clock::now() >= deadline) {
                    result.diagnostics.deadlineReached = true;
                    roundDeadline = true;
                    return false;
                }
                ++result.diagnostics.sparseRoutes;
                DayPlan candidate = roundBasePlan;
                candidate.actions.at(static_cast<std::size_t>(agent)) =
                    route.actions;
                if (!planHashes.insert(plan_hash(candidate)).second) {
                    return true;
                }
                ++result.diagnostics.generatedPlans;
                const SimulationResult detailed =
                    simulator_.simulate(state, candidate, false);
                const SimulationResult independent =
                    validator_.validate(state, candidate, false);
                std::string mismatch;
                if (!detailed.valid ||
                    !validator_.agrees_with(detailed, independent, mismatch)) {
                    return true;
                }
                ++result.diagnostics.validPlans;
                const OfficialScore candidateScore =
                    OfficialScore::after_day(ledger, detailed.score);
                if (!(roundBestScore < candidateScore)) {
                    return true;
                }
                ++result.diagnostics.strictTerminalImprovements;
                roundBestPlan = std::move(candidate);
                roundBestSimulation = detailed;
                roundBestScore = candidateScore;
                roundBestAgent = agent;
                return true;
            };
            for (const ExactOrienteeringRoute& route : routes.maximalRoutes) {
                if (!evaluate(route)) {
                    break;
                }
            }
            if (!roundDeadline) {
                for (const ExactOrienteeringRoute& route :
                     routes.supplementalRoutes) {
                    if (!evaluate(route)) {
                        break;
                    }
                }
            }
            if (roundDeadline) {
                break;
            }
        }

        if (roundBaseScore < roundBestScore) {
            result.plan = std::move(roundBestPlan);
            result.simulation = roundBestSimulation;
            result.scoreAfterToday = roundBestScore;
            result.improved = true;
            result.witnessAgent = roundBestAgent;
            result.witnessParentFuel = roundBaseSimulation.finalAgents.at(
                static_cast<std::size_t>(roundBestAgent)).fuel;
            result.witnessCandidateFuel =
                roundBestSimulation.finalAgents.at(
                    static_cast<std::size_t>(roundBestAgent)).fuel;
            ++result.diagnostics.terminalSparseRounds;
        }
        if (firstRound) {
            result.firstRoundScore = result.scoreAfterToday;
            firstRound = false;
        }
        if (roundDeadline || !(roundBaseScore < roundBestScore)) {
            return roundDeadline;
        }
    }
    };
    bool deadlineReached = run_one_agent_ascent();

    // Pair-exchange phase: only after the unchanged one-agent ascent reaches
    // its natural fixed point, the remaining protected budget evaluates joint
    // replacements of two patrols' plans from the already-enumerated sparse
    // route pools. A coordinate method stops at one-agent local optima; a
    // strict joint improvement re-enters the one-agent ascent. The parent
    // work prefix is order-identical, so a deadline inside the ascent means
    // this phase never runs.
    while (enableTerminalPairExchange && !deadlineReached) {
        bool pairImproved = false;
        for (std::size_t leftTask = 0;
             leftTask + 1U < tasks.size() && !pairImproved && !deadlineReached;
             ++leftTask) {
            for (std::size_t rightTask = leftTask + 1U;
                 rightTask < tasks.size() && !pairImproved && !deadlineReached;
                 ++rightTask) {
                const AgentIndex left = tasks.at(leftTask);
                const AgentIndex right = tasks.at(rightTask);
                const ExactOrienteeringReachability& leftRoutes =
                    reachability.at(static_cast<std::size_t>(left));
                const ExactOrienteeringReachability& rightRoutes =
                    reachability.at(static_cast<std::size_t>(right));
                for (const ExactOrienteeringRoute& leftRoute :
                     leftRoutes.maximalRoutes) {
                    if (pairImproved || deadlineReached) {
                        break;
                    }
                    for (const ExactOrienteeringRoute& rightRoute :
                         rightRoutes.maximalRoutes) {
                        if (std::chrono::steady_clock::now() >= deadline) {
                            result.diagnostics.deadlineReached = true;
                            deadlineReached = true;
                            break;
                        }
                        ++result.diagnostics.sparseRoutes;
                        DayPlan candidate = result.plan;
                        candidate.actions.at(static_cast<std::size_t>(left)) =
                            leftRoute.actions;
                        candidate.actions.at(static_cast<std::size_t>(right)) =
                            rightRoute.actions;
                        if (!planHashes.insert(plan_hash(candidate)).second) {
                            continue;
                        }
                        ++result.diagnostics.generatedPlans;
                        const SimulationResult detailed =
                            simulator_.simulate(state, candidate, false);
                        const SimulationResult independent =
                            validator_.validate(state, candidate, false);
                        std::string mismatch;
                        if (!detailed.valid ||
                            !validator_.agrees_with(
                                detailed,
                                independent,
                                mismatch)) {
                            continue;
                        }
                        ++result.diagnostics.validPlans;
                        const OfficialScore candidateScore =
                            OfficialScore::after_day(ledger, detailed.score);
                        if (!(result.scoreAfterToday < candidateScore)) {
                            continue;
                        }
                        ++result.diagnostics.strictTerminalImprovements;
                        ++result.diagnostics.terminalPairAcceptances;
                        result.witnessParentFuel = result.simulation.finalAgents.at(
                            static_cast<std::size_t>(left)).fuel;
                        result.plan = std::move(candidate);
                        result.simulation = detailed;
                        result.scoreAfterToday = candidateScore;
                        result.improved = true;
                        result.witnessAgent = left;
                        result.witnessCandidateFuel = detailed.finalAgents.at(
                            static_cast<std::size_t>(left)).fuel;
                        ++result.diagnostics.terminalSparseRounds;
                        pairImproved = true;
                        break;
                    }
                }
            }
        }
        if (!pairImproved) {
            break;
        }
        deadlineReached = run_one_agent_ascent();
    }

    // SCORE-TERMINAL-STOCK-MARGINAL-RESERVOIR-258. Freeze the complete
    // canonical ascent/pair result before touching the additive reservoir.
    // The suffix is terminal-only and accepts solely a dual-valid strict
    // official gain, so deadline/no-gain returns this exact prefix.
    result.canonicalTerminalScore = result.scoreAfterToday;
    if (!enableTerminalMarginalReservoir || deadlineReached) {
        return result;
    }
    const DayPlan canonicalPlan = result.plan;
    const SimulationResult canonicalSimulation = result.simulation;
    const OfficialScore canonicalScore = result.scoreAfterToday;
    const bool canonicalImproved = result.improved;
    const AgentIndex canonicalWitnessAgent = result.witnessAgent;
    const std::int32_t canonicalWitnessParentFuel = result.witnessParentFuel;
    const std::int32_t canonicalWitnessCandidateFuel =
        result.witnessCandidateFuel;
    const std::int64_t canonicalTerminalSparseRounds =
        result.diagnostics.terminalSparseRounds;
    const auto restore_canonical = [&](bool failure) {
        result.plan = canonicalPlan;
        result.simulation = canonicalSimulation;
        result.scoreAfterToday = canonicalScore;
        result.improved = canonicalImproved;
        result.witnessAgent = canonicalWitnessAgent;
        result.witnessParentFuel = canonicalWitnessParentFuel;
        result.witnessCandidateFuel = canonicalWitnessCandidateFuel;
        result.diagnostics.terminalMarginalAcceptances = 0;
        result.diagnostics.terminalMarginalRounds = 0;
        result.diagnostics.terminalSparseRounds =
            canonicalTerminalSparseRounds;
        result.diagnostics.terminalMarginalDeadline = !failure;
        result.diagnostics.terminalMarginalFailure = failure;
    };
    for (;;) {
        const DayPlan roundBasePlan = result.plan;
        const SimulationResult roundBaseSimulation = result.simulation;
        const OfficialScore roundBaseScore = result.scoreAfterToday;
        DayPlan roundBestPlan = roundBasePlan;
        SimulationResult roundBestSimulation = roundBaseSimulation;
        OfficialScore roundBestScore = roundBaseScore;
        AgentIndex roundBestAgent = kInvalidAgent;
        bool roundDeadline = false;
        bool roundFailure = false;

        for (const AgentIndex agent : tasks) {
            const ExactOrienteeringReachability& routes = reachability.at(
                static_cast<std::size_t>(agent));
            for (const ExactOrienteeringRoute& route :
                 routes.terminalMarginalRoutes) {
                if (std::chrono::steady_clock::now() >= deadline) {
                    result.diagnostics.deadlineReached = true;
                    roundDeadline = true;
                    break;
                }
                ++result.diagnostics.sparseRoutes;
                ++result.diagnostics.terminalMarginalRoutes;
                DayPlan candidate = roundBasePlan;
                candidate.actions.at(static_cast<std::size_t>(agent)) =
                    route.actions;
                if (!planHashes.insert(plan_hash(candidate)).second) {
                    continue;
                }
                ++result.diagnostics.generatedPlans;
                ++result.diagnostics.terminalMarginalGeneratedPlans;
                const SimulationResult detailed = simulator_.simulate(
                    state,
                    candidate,
                    false);
                const SimulationResult independent = validator_.validate(
                    state,
                    candidate,
                    false);
                std::string mismatch;
                if (!detailed.valid) {
                    continue;
                }
                if (!validator_.agrees_with(
                        detailed,
                        independent,
                        mismatch)) {
                    roundFailure = true;
                    break;
                }
                ++result.diagnostics.validPlans;
                ++result.diagnostics.terminalMarginalValidPlans;
                const OfficialScore candidateScore = OfficialScore::after_day(
                    ledger,
                    detailed.score);
                if (!(roundBestScore < candidateScore)) {
                    continue;
                }
                ++result.diagnostics.strictTerminalImprovements;
                roundBestPlan = std::move(candidate);
                roundBestSimulation = detailed;
                roundBestScore = candidateScore;
                roundBestAgent = agent;
            }
            if (roundDeadline) {
                break;
            }
            if (roundFailure) {
                break;
            }
        }

        if (roundDeadline || roundFailure) {
            restore_canonical(roundFailure);
            return result;
        }

        if (roundBaseScore < roundBestScore) {
            result.plan = std::move(roundBestPlan);
            result.simulation = roundBestSimulation;
            result.scoreAfterToday = roundBestScore;
            result.improved = true;
            result.witnessAgent = roundBestAgent;
            result.witnessParentFuel = roundBaseSimulation.finalAgents.at(
                static_cast<std::size_t>(roundBestAgent)).fuel;
            result.witnessCandidateFuel = roundBestSimulation.finalAgents.at(
                static_cast<std::size_t>(roundBestAgent)).fuel;
            ++result.diagnostics.terminalMarginalAcceptances;
            ++result.diagnostics.terminalMarginalRounds;
            ++result.diagnostics.terminalSparseRounds;
        }
        if (roundDeadline || !(roundBaseScore < roundBestScore)) {
            break;
        }
    }
    return result;
}

ProtectedSlackResult ProtectedSlackRefiner::refine_midday_chains(
    const DayState& state,
    const MatchLedger& ledger,
    const DayPlan& incumbentPlan,
    const SimulationResult& incumbentSimulation,
    std::chrono::steady_clock::time_point deadline) const {
    ProtectedSlackResult result;
    result.plan = incumbentPlan;
    result.simulation = incumbentSimulation;
    result.scoreAfterToday =
        OfficialScore::after_day(ledger, incumbentSimulation.score);
    result.firstRoundScore = result.scoreAfterToday;
    if (!enableMiddayChainAdoption ||
        !incumbentSimulation.valid ||
        state.dayNumber >= config_.day_count() ||
        incumbentPlan.actions.size() !=
            static_cast<std::size_t>(config_.agent_count()) ||
        config_.spots.size() > 32U) {
        return result;
    }
    result.diagnostics.middayChain = true;
    const SimulationResult independentIncumbent =
        validator_.validate(state, incumbentPlan, false);
    std::string incumbentMismatch;
    if (!validator_.agrees_with(
            incumbentSimulation,
            independentIncumbent,
            incumbentMismatch)) {
        return result;
    }
    constexpr auto kValidationReserve = std::chrono::milliseconds{80};
    const auto now = std::chrono::steady_clock::now();
    if (now + kValidationReserve >= deadline) {
        result.diagnostics.deadlineReached = true;
        return result;
    }
    const auto searchDeadline = deadline - kValidationReserve;

    BrandMask preferredBrands;
    for (std::int32_t brand = 0; brand < config_.brand_count(); ++brand) {
        if (!has_brand(ledger.lifetimeBrands, brand)) {
            preferredBrands |= brand_bit(brand);
        }
    }
    if (preferredBrands == 0U) {
        for (std::int32_t brand = 0; brand < config_.brand_count(); ++brand) {
            if (!has_brand(incumbentSimulation.score.brands, brand)) {
                preferredBrands |= brand_bit(brand);
            }
        }
    }
    const std::int32_t minimumSpots = std::min<std::int32_t>(
        std::max(1, config_.brand_count() - 1),
        static_cast<std::int32_t>(config_.spots.size()));

    std::vector<AgentIndex> tasks;
    for (AgentIndex agent = 0; agent < config_.agent_count(); ++agent) {
        if (state.agents.at(static_cast<std::size_t>(agent)).kind ==
            AgentKind::Patrol) {
            tasks.push_back(agent);
        }
    }
    if (tasks.empty()) {
        return result;
    }
    std::vector<ExactOrienteeringReachability> reachability(
        static_cast<std::size_t>(config_.agent_count()));
    std::atomic<std::size_t> nextTask{0U};
    std::atomic<bool> workerFailed{false};
    constexpr std::size_t kWorkerLimit = 4U;
    const std::size_t workerCount = std::min(kWorkerLimit, tasks.size());
    {
        std::vector<std::jthread> workers;
        workers.reserve(workerCount);
        for (std::size_t worker = 0; worker < workerCount; ++worker) {
            workers.emplace_back([&]() {
                try {
                    while (!workerFailed.load() &&
                           std::chrono::steady_clock::now() < searchDeadline) {
                        const std::size_t task = nextTask.fetch_add(1U);
                        if (task >= tasks.size()) {
                            return;
                        }
                        const AgentIndex agent = tasks.at(task);
                        reachability.at(static_cast<std::size_t>(agent)) =
                            enumerate_sparse_anytime_resource_routes(
                                config_,
                                state,
                                agent,
                                minimumSpots,
                                32U,
                                1250000U,
                                searchDeadline,
                                preferredBrands);
                    }
                } catch (...) {
                    workerFailed.store(true);
                }
            });
        }
    }
    if (workerFailed.load()) {
        result.diagnostics.middayFailure = true;
        return result;
    }

    std::set<std::uint64_t> planHashes;
    planHashes.insert(plan_hash(incumbentPlan));
    bool firstRound = true;
    const auto run_one_agent_midday_ascent = [&]() -> bool {
    for (;;) {
        const DayPlan roundBasePlan = result.plan;
        const SimulationResult roundBaseSimulation = result.simulation;
        const OfficialScore roundBaseScore = result.scoreAfterToday;
        DayPlan roundBestPlan = roundBasePlan;
        SimulationResult roundBestSimulation = roundBaseSimulation;
        OfficialScore roundBestScore = roundBaseScore;
        AgentIndex roundBestAgent = kInvalidAgent;
        bool roundDeadline = false;

        for (const AgentIndex agent : tasks) {
            const ExactOrienteeringReachability& routes =
                reachability.at(static_cast<std::size_t>(agent));
            const CellId incumbentTerminal =
                roundBaseSimulation.finalAgents.at(
                    static_cast<std::size_t>(agent)).position;
            const auto evaluate = [&](const ExactOrienteeringRoute& route) {
                if (std::chrono::steady_clock::now() >= deadline) {
                    result.diagnostics.deadlineReached = true;
                    roundDeadline = true;
                    return false;
                }
                ++result.diagnostics.middayRoutes;
                // The strict_protected_improvement certificate requires the
                // same per-agent terminal cell; skipping mismatches before
                // simulation only removes certain rejections.
                if (route.terminalCell != incumbentTerminal) {
                    return true;
                }
                DayPlan candidate = roundBasePlan;
                candidate.actions.at(static_cast<std::size_t>(agent)) =
                    route.actions;
                if (!planHashes.insert(plan_hash(candidate)).second) {
                    return true;
                }
                ++result.diagnostics.middayGeneratedPlans;
                const SimulationResult detailed =
                    simulator_.simulate(state, candidate, false);
                const SimulationResult independent =
                    validator_.validate(state, candidate, false);
                std::string mismatch;
                if (!detailed.valid ||
                    !validator_.agrees_with(detailed, independent, mismatch)) {
                    return true;
                }
                ++result.diagnostics.middayValidPlans;
                if (!strict_protected_improvement(
                        ledger,
                        roundBaseSimulation,
                        detailed)) {
                    return true;
                }
                const OfficialScore candidateScore =
                    OfficialScore::after_day(ledger, detailed.score);
                if (!(roundBestScore < candidateScore)) {
                    return true;
                }
                roundBestPlan = std::move(candidate);
                roundBestSimulation = detailed;
                roundBestScore = candidateScore;
                roundBestAgent = agent;
                return true;
            };
            for (const ExactOrienteeringRoute& route : routes.maximalRoutes) {
                if (!evaluate(route)) {
                    break;
                }
            }
            if (!roundDeadline) {
                for (const ExactOrienteeringRoute& route :
                     routes.supplementalRoutes) {
                    if (!evaluate(route)) {
                        break;
                    }
                }
            }
            if (roundDeadline) {
                break;
            }
        }

        if (roundBaseScore < roundBestScore) {
            ++result.diagnostics.middayChainAcceptances;
            result.plan = std::move(roundBestPlan);
            result.simulation = roundBestSimulation;
            result.scoreAfterToday = roundBestScore;
            result.improved = true;
            result.witnessAgent = roundBestAgent;
            result.witnessParentFuel = roundBaseSimulation.finalAgents.at(
                static_cast<std::size_t>(roundBestAgent)).fuel;
            result.witnessCandidateFuel =
                result.simulation.finalAgents.at(
                    static_cast<std::size_t>(roundBestAgent)).fuel;
            ++result.diagnostics.middayRounds;
        }
        if (firstRound) {
            result.firstRoundScore = result.scoreAfterToday;
            firstRound = false;
        }
        if (roundDeadline || !(roundBaseScore < roundBestScore)) {
            return roundDeadline;
        }
    }
    };
    bool deadlineReached = run_one_agent_midday_ascent();

    // Registered SCORE-MIDDAY-TARGET-FOLLOWUP-215. The accepted global-pool
    // ascent above remains an order-identical prefix. Only after it reaches a
    // fixed point may a target-terminal pool spend remaining protected time;
    // every replacement still passes strict_protected_improvement.
    if (enableMiddayTargetTerminalFollowup) {
        result.diagnostics.middayTargetFollowup = true;
        if (!deadlineReached) {
            std::vector<ExactOrienteeringReachability> targetReachability(
                static_cast<std::size_t>(config_.agent_count()));
            std::atomic<std::size_t> nextTargetTask{0U};
            std::atomic<std::size_t> completedTargetTasks{0U};
            std::atomic<bool> targetWorkerFailed{false};
            {
                std::vector<std::jthread> workers;
                workers.reserve(workerCount);
                for (std::size_t worker = 0; worker < workerCount; ++worker) {
                    workers.emplace_back([&]() {
                        try {
                            while (!targetWorkerFailed.load() &&
                                   std::chrono::steady_clock::now() <
                                       searchDeadline) {
                                const std::size_t task =
                                    nextTargetTask.fetch_add(1U);
                                if (task >= tasks.size()) {
                                    return;
                                }
                                const AgentIndex agent = tasks.at(task);
                                const CellId terminal =
                                    result.simulation.finalAgents.at(
                                        static_cast<std::size_t>(agent))
                                        .position;
                                targetReachability.at(
                                    static_cast<std::size_t>(agent)) =
                                    enumerate_sparse_anytime_resource_routes_to_terminal(
                                        config_,
                                        state,
                                        agent,
                                        terminal,
                                        minimumSpots,
                                        32U,
                                        1250000U,
                                        searchDeadline,
                                        preferredBrands);
                                completedTargetTasks.fetch_add(1U);
                            }
                        } catch (...) {
                            targetWorkerFailed.store(true);
                        }
                    });
                }
            }
            if (targetWorkerFailed.load()) {
                result.diagnostics.middayFailure = true;
                return result;
            }

            reachability = std::move(targetReachability);
            const std::int64_t routesBefore =
                result.diagnostics.middayRoutes;
            const std::int64_t plansBefore =
                result.diagnostics.middayGeneratedPlans;
            const std::int64_t validBefore =
                result.diagnostics.middayValidPlans;
            const std::int64_t acceptancesBefore =
                result.diagnostics.middayChainAcceptances;
            const std::int64_t roundsBefore =
                result.diagnostics.middayRounds;
            deadlineReached = run_one_agent_midday_ascent();
            result.diagnostics.middayTargetRoutes =
                result.diagnostics.middayRoutes - routesBefore;
            result.diagnostics.middayTargetGeneratedPlans =
                result.diagnostics.middayGeneratedPlans - plansBefore;
            result.diagnostics.middayTargetValidPlans =
                result.diagnostics.middayValidPlans - validBefore;
            result.diagnostics.middayTargetAcceptances =
                result.diagnostics.middayChainAcceptances -
                acceptancesBefore;
            result.diagnostics.middayTargetRounds =
                result.diagnostics.middayRounds - roundsBefore;
            if (completedTargetTasks.load() < tasks.size()) {
                result.diagnostics.deadlineReached = true;
                deadlineReached = true;
            }
        }
    }

    // Registered SCORE-MIDDAY-PAIR-EXCHANGE-211: only after the unchanged
    // one-agent mid-day ascent reaches its natural fixed point, the
    // remaining protected budget evaluates joint replacements of two
    // patrols' plans from the already-enumerated route pools, under the
    // same strict_protected_improvement certificate. A strict joint
    // improvement re-enters the one-agent ascent (207 loop structure).
    while (enableMiddayPairExchange && !deadlineReached) {
        bool pairImproved = false;
        for (std::size_t leftTask = 0;
             leftTask + 1U < tasks.size() && !pairImproved && !deadlineReached;
             ++leftTask) {
            for (std::size_t rightTask = leftTask + 1U;
                 rightTask < tasks.size() && !pairImproved && !deadlineReached;
                 ++rightTask) {
                const AgentIndex left = tasks.at(leftTask);
                const AgentIndex right = tasks.at(rightTask);
                const CellId leftTerminal = result.simulation.finalAgents.at(
                    static_cast<std::size_t>(left)).position;
                const CellId rightTerminal = result.simulation.finalAgents.at(
                    static_cast<std::size_t>(right)).position;
                const ExactOrienteeringReachability& leftRoutes =
                    reachability.at(static_cast<std::size_t>(left));
                const ExactOrienteeringReachability& rightRoutes =
                    reachability.at(static_cast<std::size_t>(right));
                // The same-terminal pre-filter is tight, so the pair phase
                // draws from the same pools the one-agent ascent consumes
                // (maximal + supplemental); maximal-only would under-sample
                // same-terminal candidates and could fake an inert verdict.
                const std::vector<ExactOrienteeringRoute>* leftPools[2] = {
                    &leftRoutes.maximalRoutes, &leftRoutes.supplementalRoutes};
                const std::vector<ExactOrienteeringRoute>* rightPools[2] = {
                    &rightRoutes.maximalRoutes,
                    &rightRoutes.supplementalRoutes};
                for (const auto* leftPool : leftPools) {
                if (pairImproved || deadlineReached) {
                    break;
                }
                for (const ExactOrienteeringRoute& leftRoute : *leftPool) {
                    if (pairImproved || deadlineReached) {
                        break;
                    }
                    if (leftRoute.terminalCell != leftTerminal) {
                        continue;
                    }
                    for (const auto* rightPool : rightPools) {
                    if (pairImproved || deadlineReached) {
                        break;
                    }
                    for (const ExactOrienteeringRoute& rightRoute :
                         *rightPool) {
                        if (std::chrono::steady_clock::now() >= deadline) {
                            result.diagnostics.deadlineReached = true;
                            deadlineReached = true;
                            break;
                        }
                        ++result.diagnostics.middayRoutes;
                        if (rightRoute.terminalCell != rightTerminal) {
                            continue;
                        }
                        DayPlan candidate = result.plan;
                        candidate.actions.at(static_cast<std::size_t>(left)) =
                            leftRoute.actions;
                        candidate.actions.at(static_cast<std::size_t>(right)) =
                            rightRoute.actions;
                        if (!planHashes.insert(plan_hash(candidate)).second) {
                            continue;
                        }
                        ++result.diagnostics.middayGeneratedPlans;
                        const SimulationResult detailed =
                            simulator_.simulate(state, candidate, false);
                        const SimulationResult independent =
                            validator_.validate(state, candidate, false);
                        std::string mismatch;
                        if (!detailed.valid ||
                            !validator_.agrees_with(
                                detailed,
                                independent,
                                mismatch)) {
                            continue;
                        }
                        ++result.diagnostics.middayValidPlans;
                        if (!strict_protected_improvement(
                                ledger,
                                result.simulation,
                                detailed)) {
                            continue;
                        }
                        const OfficialScore candidateScore =
                            OfficialScore::after_day(ledger, detailed.score);
                        if (!(result.scoreAfterToday < candidateScore)) {
                            continue;
                        }
                        ++result.diagnostics.middayChainAcceptances;
                        ++result.diagnostics.middayPairAcceptances;
                        ++result.diagnostics.middayRounds;
                        result.witnessParentFuel =
                            result.simulation.finalAgents.at(
                                static_cast<std::size_t>(left)).fuel;
                        result.plan = std::move(candidate);
                        result.simulation = detailed;
                        result.scoreAfterToday = candidateScore;
                        result.improved = true;
                        result.witnessAgent = left;
                        result.witnessCandidateFuel =
                            detailed.finalAgents.at(
                                static_cast<std::size_t>(left)).fuel;
                        pairImproved = true;
                        break;
                    }
                    }
                }
                }
            }
        }
        if (!pairImproved) {
            break;
        }
        deadlineReached = run_one_agent_midday_ascent();
    }
    return result;
}

} // namespace udon
