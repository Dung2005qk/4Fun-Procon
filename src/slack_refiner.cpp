#include "udon/slack_refiner.hpp"

#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

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
    const std::uint64_t incumbentLifetime =
        ledger.lifetimeBrands | incumbent.score.brands;
    const std::uint64_t challengerLifetime =
        ledger.lifetimeBrands | challenger.score.brands;
    if ((incumbentLifetime & ~challengerLifetime) != 0U ||
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
    return (baseline.lifetimeBrands & ~actual.lifetimeBrands) == 0U &&
        actual.totalDailyDistinct >= baseline.totalDailyDistinct &&
        actual.totalServings >= baseline.totalServings;
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

} // namespace udon
