#include "udon/planner.hpp"
#include "udon/protocol.hpp"

#include <algorithm>
#include <bit>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace udon {

namespace {

struct TargetSeed {
    SpotIndex spot = kInvalidSpot;
    ParetoPath path;
    std::int32_t priority = 0;
};

[[nodiscard]] std::vector<CellId> select_critical_roads(
    const MatchConfig& config,
    const DayState& state,
    const std::vector<CellId>& promotedHints) {
    std::vector<CellId> result;
    result.reserve(16U);
    const auto append = [&config, &result](CellId road) {
        if (road == kInvalidCell || !config.map.contains(road) ||
            config.map.terrain.at(static_cast<std::size_t>(road)) != Terrain::Road ||
            std::find(result.begin(), result.end(), road) != result.end()) {
            return false;
        }
        result.push_back(road);
        return true;
    };
    for (const CellId road : promotedHints) {
        if (append(road) && result.size() == 16U) {
            return result;
        }
    }
    for (const RoadStatus desired : {RoadStatus::Jammed, RoadStatus::Busy, RoadStatus::Smooth}) {
        for (const CellId road : config.roadCells) {
            if (state.roadStatuses.at(static_cast<std::size_t>(road)) == desired) {
                if (append(road) && result.size() == 16U) {
                    return result;
                }
            }
        }
    }
    return result;
}

[[nodiscard]] std::int32_t brand_rarity(const MatchConfig& config, std::int32_t brandIndex) {
    std::int32_t count = 0;
    for (const Spot& spot : config.spots) {
        if (spot.brandIndex == brandIndex) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::int32_t reservation_priority(
    const std::vector<MandatoryReservation>& reservations,
    const DayState& state,
    SpotIndex spot) {
    std::int32_t result = 0;
    for (const MandatoryReservation& reservation : reservations) {
        if (reservation.representativeSpot != spot || reservation.latestSafeDay > state.dayNumber) {
            continue;
        }
        const std::int32_t priority = is_proven_reservation(reservation)
            ? 4000000
            : reservation.evidence == ReservationEvidence::WitnessBacked ? 3000000 : 2000000;
        result = std::max(result, priority);
    }
    return result;
}

[[nodiscard]] AgentPlan complete_actions(
    const MatchConfig& config,
    const DayState& state,
    const ParetoPath& path) {
    AgentPlan result;
    result.reserve(path.directions.size() + 1U);
    for (const std::int32_t direction : path.directions) {
        result.push_back(PlanAction::move(direction));
    }
    const std::int32_t remaining = config.steps_for_day(state.dayNumber) - path.travelSteps;
    if (remaining > 0) {
        result.push_back(PlanAction::wait(remaining));
    }
    return result;
}

[[nodiscard]] AgentPlan rendezvous_actions(
    const MatchConfig& config,
    const DayState& state,
    std::int32_t initialWait,
    const ParetoPath& arrival,
    const ParetoPath& departure,
    std::int32_t rendezvousWait = 0) {
    const std::int32_t used = initialWait + arrival.travelSteps + rendezvousWait + departure.travelSteps;
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    if (initialWait < 0 || used > daySteps) {
        throw std::invalid_argument("rendezvous route exceeds the current day budget");
    }
    AgentPlan result;
    result.reserve(
        static_cast<std::size_t>(arrival.directions.size() + departure.directions.size() + 2U));
    if (initialWait > 0) {
        result.push_back(PlanAction::wait(initialWait));
    }
    for (const std::int32_t direction : arrival.directions) {
        result.push_back(PlanAction::move(direction));
    }
    if (rendezvousWait > 0) {
        result.push_back(PlanAction::wait(rendezvousWait));
    }
    for (const std::int32_t direction : departure.directions) {
        result.push_back(PlanAction::move(direction));
    }
    const std::int32_t remaining = daySteps - used;
    if (remaining > 0) {
        result.push_back(PlanAction::wait(remaining));
    }
    return result;
}

[[nodiscard]] AgentPlan rendezvous_wait_actions(
    const MatchConfig& config,
    const DayState& state,
    std::int32_t initialWait,
    const ParetoPath& arrival) {
    const ParetoPath emptyDeparture;
    return rendezvous_actions(config, state, initialWait, arrival, emptyDeparture);
}

[[nodiscard]] AgentPlan wait_actions(const MatchConfig& config, const DayState& state) {
    return AgentPlan{PlanAction::wait(config.steps_for_day(state.dayNumber))};
}

[[nodiscard]] std::optional<AgentPlan> prepend_start_harvest_wait(const AgentPlan& actions) {
    if (actions.empty() || actions.front().kind != ActionKind::Move ||
        actions.back().kind != ActionKind::Wait) {
        return std::nullopt;
    }
    AgentPlan result = actions;
    if (result.back().value == 1) {
        result.pop_back();
    } else {
        --result.back().value;
    }
    result.insert(result.begin(), PlanAction::wait(1));
    return result;
}

void populate_first_visits(
    const MatchConfig& config,
    const DayState& state,
    RouteColumn& column) {
    column.firstVisits.clear();
    column.fullFootprint.entries.clear();
    column.timeline.clear();
    column.escortSegments.clear();
    column.terminalFeatures = RouteTerminalFeatures{};
    column.hasExactTimeline = false;
    if (column.agent < 0 || column.agent >= config.agent_count()) {
        return;
    }
    const AgentState& initialAgent = state.agents.at(static_cast<std::size_t>(column.agent));
    const bool patrol = initialAgent.kind == AgentKind::Patrol;
    if (!patrol) {
        column.providedRefuels.clear();
    }
    const std::int32_t stepCount = config.steps_for_day(state.dayNumber);
    CellId position = initialAgent.position;
    std::int32_t fuel = initialAgent.fuel;
    std::int32_t pendingFuelCost = 0;
    std::size_t actionOffset = 0;
    PlanAction activeAction;
    CellId activeDestination = position;
    std::int32_t remaining = 0;
    bool active = false;
    const auto schedule = [&]() -> bool {
        if (actionOffset >= column.actions.size()) {
            return false;
        }
        activeAction = column.actions.at(actionOffset++);
        if (activeAction.kind == ActionKind::Wait) {
            if (activeAction.value <= 0) {
                return false;
            }
            activeDestination = position;
            remaining = activeAction.value;
            active = true;
            return true;
        }
        if (activeAction.kind != ActionKind::Move || activeAction.value < 0 || activeAction.value >= kDirectionCount) {
            return false;
        }
        const CellId destination = config.map.neighbors.at(static_cast<std::size_t>(position)).at(
            static_cast<std::size_t>(activeAction.value));
        if (destination == kInvalidCell || config.map.terrain.at(static_cast<std::size_t>(destination)) == Terrain::Pond) {
            return false;
        }
        const MoveCost cost = config.move_cost(
            position,
            state.roadStatuses.at(static_cast<std::size_t>(position)));
        if (cost.steps <= 0 || (patrol && fuel < cost.patrolFuel)) {
            return false;
        }
        activeDestination = destination;
        remaining = cost.steps;
        pendingFuelCost = patrol ? cost.patrolFuel : 0;
        active = true;
        return true;
    };

    if (!schedule()) {
        return;
    }
    column.timeline.reserve(static_cast<std::size_t>(stepCount + 1));
    column.timeline.push_back(RouteStepState{position, fuel});
    std::vector<bool> visited(config.spots.size(), false);
    for (std::int32_t step = 1; step <= stepCount; ++step) {
        if (!active || remaining <= 0) {
            column.firstVisits.clear();
            column.timeline.clear();
            return;
        }
        --remaining;
        bool completed = false;
        if (remaining == 0) {
            if (activeAction.kind == ActionKind::Move) {
                fuel -= pendingFuelCost;
                if (fuel < 0) {
                    column.firstVisits.clear();
                    column.timeline.clear();
                    return;
                }
                position = activeDestination;
            }
            active = false;
            completed = true;
        }
        const SpotIndex spot = config.spotAtCell.at(static_cast<std::size_t>(position));
        if (patrol && completed && spot != kInvalidSpot &&
            !visited.at(static_cast<std::size_t>(spot))) {
            visited.at(static_cast<std::size_t>(spot)) = true;
            column.firstVisits.push_back(ColumnVisitEvent{
                spot,
                step,
                true,
                config.spots.at(static_cast<std::size_t>(spot)).brandIndex,
                false,
            });
        }
        if (!patrol) {
            column.providedRefuels.push_back(RefuelEvent{position, step});
        } else if (std::find(
                       column.requiredRefuels.begin(),
                       column.requiredRefuels.end(),
                       RefuelEvent{position, step}) != column.requiredRefuels.end()) {
            fuel = config.fuelLimit;
        }
        if (config.map.terrain.at(static_cast<std::size_t>(position)) == Terrain::Road) {
            column.fullFootprint.add(position, 1);
        }
        column.timeline.push_back(RouteStepState{position, fuel});
        if (step < stepCount && !active && !schedule()) {
            column.firstVisits.clear();
            column.timeline.clear();
            return;
        }
    }
    if (active || actionOffset != column.actions.size()) {
        column.firstVisits.clear();
        column.fullFootprint.entries.clear();
        column.timeline.clear();
        return;
    }
    column.terminalCell = position;
    column.terminalFuel = fuel;
    column.terminalFeatures.spot = config.spotAtCell.at(static_cast<std::size_t>(position));
    column.terminalFeatures.overnightHarvestCandidate =
        patrol && column.terminalFeatures.spot != kInvalidSpot;
    column.terminalFeatures.endStepDockRequired = patrol && std::find(
        column.requiredRefuels.begin(),
        column.requiredRefuels.end(),
        RefuelEvent{position, stepCount}) != column.requiredRefuels.end();
    if (column.escortGroup >= 0) {
        EscortSegment segment;
        segment.group = column.escortGroup;
        if (column.lockstepEscort) {
            segment.firstStep = 0;
            segment.lastStep = stepCount;
            segment.positions.reserve(column.timeline.size());
            for (const RouteStepState& stateAtStep : column.timeline) {
                segment.positions.push_back(stateAtStep.position);
            }
        } else {
            if (patrol && !column.requiredRefuels.empty()) {
                const RefuelEvent& rendezvous = column.requiredRefuels.front();
                segment.firstStep = rendezvous.step;
                segment.lastStep = rendezvous.step;
                segment.positions.push_back(rendezvous.cell);
            } else {
                std::int32_t rendezvousStep = stepCount;
                for (std::int32_t step = 1; step <= stepCount; ++step) {
                    if (column.timeline.at(static_cast<std::size_t>(step)).position == position) {
                        rendezvousStep = step;
                        break;
                    }
                }
                segment.firstStep = rendezvousStep;
                segment.lastStep = rendezvousStep;
                segment.positions.push_back(position);
            }
        }
        column.escortSegments.push_back(std::move(segment));
    }
    column.hasExactTimeline = true;
}

void populate_exact_escort_segments(
    const DayState& state,
    RoutePortfolio& portfolio) {
    std::map<std::int32_t, std::vector<RouteColumn*>> groups;
    for (std::vector<RouteColumn>& columns : portfolio.columnsByAgent) {
        for (RouteColumn& column : columns) {
            if (column.escortGroup >= 0 && column.hasExactTimeline) {
                groups[column.escortGroup].push_back(&column);
            }
        }
    }
    for (auto& [groupId, columns] : groups) {
        if (columns.size() < 2U) {
            continue;
        }
        const std::int32_t tankerCount = static_cast<std::int32_t>(std::count_if(
            columns.begin(),
            columns.end(),
            [&state](const RouteColumn* column) {
                return state.agents.at(static_cast<std::size_t>(column->agent)).kind == AgentKind::Tanker;
            }));
        if (tankerCount != 1) {
            continue;
        }
        std::size_t commonSteps = columns.front()->timeline.size();
        for (const RouteColumn* column : columns) {
            commonSteps = std::min(commonSteps, column->timeline.size());
        }
        std::int32_t bestFirst = -1;
        std::int32_t bestLast = -1;
        std::int32_t runFirst = -1;
        for (std::size_t step = 0; step < commonSteps; ++step) {
            const CellId position = columns.front()->timeline.at(step).position;
            const bool colocated = std::all_of(
                columns.begin() + 1,
                columns.end(),
                [step, position](const RouteColumn* column) {
                    return column->timeline.at(step).position == position;
                });
            if (colocated) {
                if (runFirst < 0) {
                    runFirst = static_cast<std::int32_t>(step);
                }
            } else if (runFirst >= 0) {
                const std::int32_t runLast = static_cast<std::int32_t>(step) - 1;
                if (bestFirst < 0 || runLast - runFirst > bestLast - bestFirst) {
                    bestFirst = runFirst;
                    bestLast = runLast;
                }
                runFirst = -1;
            }
        }
        if (runFirst >= 0) {
            const std::int32_t runLast = static_cast<std::int32_t>(commonSteps) - 1;
            if (bestFirst < 0 || runLast - runFirst > bestLast - bestFirst) {
                bestFirst = runFirst;
                bestLast = runLast;
            }
        }
        if (bestFirst < 0) {
            continue;
        }
        EscortSegment segment;
        segment.group = groupId;
        segment.firstStep = bestFirst;
        segment.lastStep = bestLast;
        segment.positions.reserve(static_cast<std::size_t>(bestLast - bestFirst + 1));
        for (std::int32_t step = bestFirst; step <= bestLast; ++step) {
            segment.positions.push_back(columns.front()->timeline.at(static_cast<std::size_t>(step)).position);
        }
        for (RouteColumn* column : columns) {
            column->escortSegments.clear();
            column->escortSegments.push_back(segment);
        }
    }
}

[[nodiscard]] std::string actions_key(
    const AgentPlan& actions,
    std::int32_t escortGroup,
    std::int32_t contingencyBundle,
    const std::vector<RefuelEvent>& requiredRefuels) {
    std::string result = std::to_string(escortGroup) + ":" + std::to_string(contingencyBundle) + ":";
    for (const PlanAction& action : actions) {
        result += std::to_string(action.wire_value());
        result.push_back(',');
    }
    result.push_back('|');
    for (const RefuelEvent& event : requiredRefuels) {
        result += std::to_string(event.cell);
        result.push_back('@');
        result += std::to_string(event.step);
        result.push_back(',');
    }
    return result;
}

[[nodiscard]] bool same_agent_plan(const AgentPlan& left, const AgentPlan& right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t actionIndex = 0; actionIndex < left.size(); ++actionIndex) {
        if (left.at(actionIndex).kind != right.at(actionIndex).kind ||
            left.at(actionIndex).value != right.at(actionIndex).value) {
            return false;
        }
    }
    return true;
}

void prune_columns(std::vector<RouteColumn>& columns, std::int32_t maximumColumns) {
    std::sort(
        columns.begin(),
        columns.end(),
        [](const RouteColumn& left, const RouteColumn& right) {
            if (left.priority != right.priority) {
                return left.priority > right.priority;
            }
            if (left.estimatedBrands != right.estimatedBrands) {
                return left.estimatedBrands > right.estimatedBrands;
            }
            if (left.estimatedServings != right.estimatedServings) {
                return left.estimatedServings > right.estimatedServings;
            }
            return actions_key(left.actions, left.escortGroup, left.contingencyBundle, left.requiredRefuels) <
                actions_key(right.actions, right.escortGroup, right.contingencyBundle, right.requiredRefuels);
        });
    std::set<std::string> seen;
    std::vector<RouteColumn> unique;
    unique.reserve(columns.size());
    for (RouteColumn& column : columns) {
        const std::string key = actions_key(
            column.actions,
            column.escortGroup,
            column.contingencyBundle,
            column.requiredRefuels);
        if (seen.insert(key).second) {
            unique.push_back(std::move(column));
        }
    }
    const auto safeWait = std::find_if(
        unique.begin(),
        unique.end(),
        [](const RouteColumn& column) {
            return column.escortGroup < 0 && column.contingencyBundle < 0 &&
                column.actions.size() == 1U &&
                column.actions.front().kind == ActionKind::Wait;
        });
    std::optional<RouteColumn> retainedWait;
    if (safeWait != unique.end()) {
        retainedWait = *safeWait;
    }
    const auto staging = std::min_element(
        unique.begin(),
        unique.end(),
        [](const RouteColumn& left, const RouteColumn& right) {
            const bool leftEligible =
                left.escortGroup < 0 && left.contingencyBundle < 0 &&
                left.firstVisits.empty() &&
                left.terminalFeatures.nearestUncollectedBrandSteps !=
                    std::numeric_limits<std::int32_t>::max() &&
                !(left.actions.size() == 1U &&
                  left.actions.front().kind == ActionKind::Wait);
            const bool rightEligible =
                right.escortGroup < 0 && right.contingencyBundle < 0 &&
                right.firstVisits.empty() &&
                right.terminalFeatures.nearestUncollectedBrandSteps !=
                    std::numeric_limits<std::int32_t>::max() &&
                !(right.actions.size() == 1U &&
                  right.actions.front().kind == ActionKind::Wait);
            if (leftEligible != rightEligible) {
                return leftEligible;
            }
            if (!leftEligible) {
                return false;
            }
            if (left.terminalFeatures.nearestUncollectedBrandSteps !=
                right.terminalFeatures.nearestUncollectedBrandSteps) {
                return left.terminalFeatures.nearestUncollectedBrandSteps <
                    right.terminalFeatures.nearestUncollectedBrandSteps;
            }
            return left.columnId < right.columnId;
        });
    std::optional<RouteColumn> retainedStaging;
    if (staging != unique.end() && staging->firstVisits.empty() &&
        staging->terminalFeatures.nearestUncollectedBrandSteps !=
            std::numeric_limits<std::int32_t>::max() &&
        !(staging->actions.size() == 1U &&
          staging->actions.front().kind == ActionKind::Wait)) {
        retainedStaging = *staging;
    }
    if (maximumColumns > 0 && static_cast<std::int32_t>(unique.size()) > maximumColumns) {
        std::vector<std::int32_t> retainedIds;
        retainedIds.reserve(static_cast<std::size_t>(maximumColumns));
        const auto retain = [&](std::int32_t columnId) {
            if (static_cast<std::int32_t>(retainedIds.size()) >= maximumColumns ||
                std::find(retainedIds.begin(), retainedIds.end(), columnId) != retainedIds.end()) {
                return;
            }
            retainedIds.push_back(columnId);
        };
        if (retainedWait.has_value()) {
            retain(retainedWait->columnId);
        }
        if (maximumColumns >= 2 && retainedStaging.has_value()) {
            retain(retainedStaging->columnId);
        }
        for (const RouteColumn& column : unique) {
            if (column.contingencyBundle >= 0) {
                retain(column.columnId);
            }
        }

        std::uint64_t coveredBrands = 0;
        for (const RouteColumn& column : unique) {
            if (std::find(retainedIds.begin(), retainedIds.end(), column.columnId) != retainedIds.end()) {
                coveredBrands |= column.estimatedBrands;
            }
        }
        const std::int32_t diversityLimit = std::max(1, maximumColumns / 2);
        for (std::int32_t diversityPick = 0;
             diversityPick < diversityLimit && static_cast<std::int32_t>(retainedIds.size()) < maximumColumns;
             ++diversityPick) {
            const RouteColumn* best = nullptr;
            std::int32_t bestMarginal = 0;
            for (const RouteColumn& column : unique) {
                if (column.estimatedBrands == 0 ||
                    std::find(retainedIds.begin(), retainedIds.end(), column.columnId) != retainedIds.end()) {
                    continue;
                }
                const std::int32_t marginal = static_cast<std::int32_t>(
                    std::popcount(column.estimatedBrands & ~coveredBrands));
                if (marginal == 0) {
                    continue;
                }
                const bool columnIndependent = column.escortGroup < 0 && column.contingencyBundle < 0;
                const bool bestIndependent = best != nullptr && best->escortGroup < 0 && best->contingencyBundle < 0;
                const std::int32_t columnBreadth = static_cast<std::int32_t>(std::popcount(column.estimatedBrands));
                const std::int32_t bestBreadth = best == nullptr
                    ? -1
                    : static_cast<std::int32_t>(std::popcount(best->estimatedBrands));
                if (best == nullptr || marginal > bestMarginal ||
                    (marginal == bestMarginal && columnIndependent != bestIndependent && columnIndependent) ||
                    (marginal == bestMarginal && columnIndependent == bestIndependent &&
                     columnBreadth > bestBreadth) ||
                    (marginal == bestMarginal && columnIndependent == bestIndependent &&
                     columnBreadth == bestBreadth && column.estimatedServings > best->estimatedServings) ||
                    (marginal == bestMarginal && columnIndependent == bestIndependent &&
                     columnBreadth == bestBreadth && column.estimatedServings == best->estimatedServings &&
                     column.priority > best->priority)) {
                    best = &column;
                    bestMarginal = marginal;
                }
            }
            if (best == nullptr) {
                break;
            }
            retain(best->columnId);
            coveredBrands |= best->estimatedBrands;
        }
        for (const RouteColumn& column : unique) {
            retain(column.columnId);
        }

        std::vector<RouteColumn> retained;
        retained.reserve(retainedIds.size());
        for (const RouteColumn& column : unique) {
            if (std::find(retainedIds.begin(), retainedIds.end(), column.columnId) != retainedIds.end()) {
                retained.push_back(column);
            }
        }
        unique = std::move(retained);
    }
    columns = std::move(unique);
}

[[nodiscard]] std::optional<ParetoPath> staging_prefix(
    const MatchConfig& config,
    const DayState& state,
    const AgentState& agent,
    const ParetoPath& fullPath) {
    ParetoPath prefix;
    CellId current = agent.position;
    for (const std::int32_t direction : fullPath.directions) {
        const MoveCost cost = config.move_cost(
            current,
            state.roadStatuses.at(static_cast<std::size_t>(current)));
        const std::int32_t nextFuel = prefix.patrolFuel +
            (agent.kind == AgentKind::Patrol ? cost.patrolFuel : 0);
        if (prefix.travelSteps + cost.steps >
                config.steps_for_day(state.dayNumber) ||
            (agent.kind == AgentKind::Patrol && nextFuel > agent.fuel)) {
            break;
        }
        const CellId next = config.map.neighbors.at(
            static_cast<std::size_t>(current)).at(
                static_cast<std::size_t>(direction));
        if (next == kInvalidCell) {
            break;
        }
        prefix.directions.push_back(direction);
        prefix.travelSteps += cost.steps;
        prefix.patrolFuel = nextFuel;
        if (config.map.terrain.at(static_cast<std::size_t>(current)) ==
            Terrain::Road) {
            prefix.heuristicFootprint.add(current, cost.steps);
        }
        current = next;
    }
    if (prefix.directions.empty() ||
        prefix.directions.size() == fullPath.directions.size()) {
        return std::nullopt;
    }
    return prefix;
}

[[nodiscard]] bool escort_feasible(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex patrol,
    const ParetoPath& path) {
    CellId current = state.agents.at(static_cast<std::size_t>(patrol)).position;
    bool firstMove = true;
    for (const std::int32_t direction : path.directions) {
        const MoveCost cost = config.move_cost(current, state.roadStatuses.at(static_cast<std::size_t>(current)));
        if (cost.patrolFuel > config.fuelLimit) {
            return false;
        }
        if (firstMove && state.agents.at(static_cast<std::size_t>(patrol)).fuel < cost.patrolFuel) {
            return false;
        }
        const CellId next = config.map.neighbors.at(static_cast<std::size_t>(current)).at(static_cast<std::size_t>(direction));
        if (next == kInvalidCell) {
            return false;
        }
        current = next;
        firstMove = false;
    }
    return true;
}

[[nodiscard]] bool synchronized_selection_is_valid(
    const DayState& state,
    const std::vector<const RouteColumn*>& selected) {
    std::map<std::int32_t, std::int32_t> contingencyBundles;
    std::map<std::int32_t, std::vector<AgentIndex>> groups;
    for (AgentIndex agentIndex = 0; agentIndex < static_cast<AgentIndex>(selected.size()); ++agentIndex) {
        const RouteColumn* column = selected.at(static_cast<std::size_t>(agentIndex));
        if (column == nullptr) {
            continue;
        }
        if (column->contingencyBundle >= 0) {
            ++contingencyBundles[column->contingencyBundle];
        }
        if (column->escortGroup >= 0) {
            groups[column->escortGroup].push_back(agentIndex);
        }
    }
    for (const auto& [bundleId, selectedCount] : contingencyBundles) {
        static_cast<void>(bundleId);
        if (selectedCount != static_cast<std::int32_t>(selected.size())) {
            return false;
        }
    }
    for (const auto& [groupId, agents] : groups) {
        if (agents.size() < 2U) {
            return false;
        }
        const std::int32_t tankerCount = static_cast<std::int32_t>(std::count_if(
            agents.begin(),
            agents.end(),
            [&state](AgentIndex agent) {
                return state.agents.at(static_cast<std::size_t>(agent)).kind == AgentKind::Tanker;
            }));
        if (tankerCount != 1) {
            return false;
        }
        const RouteColumn& reference = *selected.at(static_cast<std::size_t>(agents.front()));
        const auto referenceSegment = std::find_if(
            reference.escortSegments.begin(),
            reference.escortSegments.end(),
            [groupId](const EscortSegment& segment) { return segment.group == groupId; });
        for (std::size_t member = 1; member < agents.size(); ++member) {
            const RouteColumn& candidate = *selected.at(static_cast<std::size_t>(agents.at(member)));
            if (reference.lockstepEscort != candidate.lockstepEscort) {
                return false;
            }
            const auto candidateSegment = std::find_if(
                candidate.escortSegments.begin(),
                candidate.escortSegments.end(),
                [groupId](const EscortSegment& segment) { return segment.group == groupId; });
            if (referenceSegment == reference.escortSegments.end() ||
                candidateSegment == candidate.escortSegments.end()) {
                if (reference.lockstepEscort) {
                    if (!same_agent_plan(reference.actions, candidate.actions)) {
                        return false;
                    }
                } else if (reference.terminalCell != candidate.terminalCell) {
                    return false;
                }
                continue;
            }
            if (referenceSegment->firstStep != candidateSegment->firstStep ||
                referenceSegment->lastStep != candidateSegment->lastStep ||
                referenceSegment->positions != candidateSegment->positions) {
                return false;
            }
        }
    }
    for (AgentIndex agentIndex = 0; agentIndex < static_cast<AgentIndex>(selected.size()); ++agentIndex) {
        const RouteColumn* patrolColumn = selected.at(static_cast<std::size_t>(agentIndex));
        if (patrolColumn == nullptr || state.agents.at(static_cast<std::size_t>(agentIndex)).kind != AgentKind::Patrol) {
            continue;
        }
        for (const RefuelEvent& required : patrolColumn->requiredRefuels) {
            bool covered = false;
            for (AgentIndex tankerIndex = 0; tankerIndex < static_cast<AgentIndex>(selected.size()); ++tankerIndex) {
                const RouteColumn* tankerColumn = selected.at(static_cast<std::size_t>(tankerIndex));
                if (tankerColumn == nullptr ||
                    state.agents.at(static_cast<std::size_t>(tankerIndex)).kind != AgentKind::Tanker) {
                    continue;
                }
                covered = std::find(
                    tankerColumn->providedRefuels.begin(),
                    tankerColumn->providedRefuels.end(),
                    required) != tankerColumn->providedRefuels.end();
                if (covered) {
                    break;
                }
            }
            if (!covered) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] bool partial_synchronized_selection_is_feasible(
    const DayState& state,
    const std::vector<const RouteColumn*>& selected,
    const std::vector<std::vector<const RouteColumn*>>& availableColumns) {
    struct ActiveGroup {
        std::int32_t selectedMembers = 0;
        std::int32_t selectedTankers = 0;
    };
    std::map<std::int32_t, ActiveGroup> activeGroups;
    for (AgentIndex agentIndex = 0; agentIndex < static_cast<AgentIndex>(selected.size()); ++agentIndex) {
        const RouteColumn* column = selected.at(static_cast<std::size_t>(agentIndex));
        if (column == nullptr || column->escortGroup < 0) {
            continue;
        }
        ActiveGroup& group = activeGroups[column->escortGroup];
        ++group.selectedMembers;
        group.selectedTankers +=
            state.agents.at(static_cast<std::size_t>(agentIndex)).kind == AgentKind::Tanker ? 1 : 0;
    }
    for (const auto& [groupId, active] : activeGroups) {
        if (active.selectedTankers > 1) {
            return false;
        }
        std::int32_t possibleMembers = active.selectedMembers;
        bool tankerPossible = active.selectedTankers == 1;
        for (AgentIndex agentIndex = 0; agentIndex < static_cast<AgentIndex>(selected.size()); ++agentIndex) {
            if (selected.at(static_cast<std::size_t>(agentIndex)) != nullptr) {
                continue;
            }
            const bool canJoin = std::any_of(
                availableColumns.at(static_cast<std::size_t>(agentIndex)).begin(),
                availableColumns.at(static_cast<std::size_t>(agentIndex)).end(),
                [groupId](const RouteColumn* column) { return column->escortGroup == groupId; });
            if (!canJoin) {
                continue;
            }
            ++possibleMembers;
            tankerPossible = tankerPossible ||
                state.agents.at(static_cast<std::size_t>(agentIndex)).kind == AgentKind::Tanker;
        }
        if (possibleMembers < 2 || !tankerPossible) {
            return false;
        }
    }

    for (AgentIndex patrolIndex = 0; patrolIndex < static_cast<AgentIndex>(selected.size()); ++patrolIndex) {
        const RouteColumn* patrol = selected.at(static_cast<std::size_t>(patrolIndex));
        if (patrol == nullptr ||
            state.agents.at(static_cast<std::size_t>(patrolIndex)).kind != AgentKind::Patrol) {
            continue;
        }
        for (const RefuelEvent& required : patrol->requiredRefuels) {
            bool coveragePossible = false;
            for (AgentIndex tankerIndex = 0;
                 tankerIndex < static_cast<AgentIndex>(selected.size()) && !coveragePossible;
                 ++tankerIndex) {
                if (state.agents.at(static_cast<std::size_t>(tankerIndex)).kind != AgentKind::Tanker) {
                    continue;
                }
                const RouteColumn* tanker = selected.at(static_cast<std::size_t>(tankerIndex));
                if (tanker != nullptr) {
                    coveragePossible = std::find(
                        tanker->providedRefuels.begin(),
                        tanker->providedRefuels.end(),
                        required) != tanker->providedRefuels.end();
                    continue;
                }
                coveragePossible = std::any_of(
                    availableColumns.at(static_cast<std::size_t>(tankerIndex)).begin(),
                    availableColumns.at(static_cast<std::size_t>(tankerIndex)).end(),
                    [&required](const RouteColumn* candidate) {
                        return std::find(
                            candidate->providedRefuels.begin(),
                            candidate->providedRefuels.end(),
                            required) != candidate->providedRefuels.end();
                    });
            }
            if (!coveragePossible) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] bool simulation_covers_proven_reservations(
    const SimulationResult& simulation,
    const DayState& state,
    const std::vector<MandatoryReservation>& reservations) {
    for (const MandatoryReservation& reservation : reservations) {
        if (!is_proven_reservation(reservation) || reservation.latestSafeDay > state.dayNumber ||
            reservation.representativeSpot == kInvalidSpot) {
            continue;
        }
        const bool covered = std::any_of(
            simulation.claims.begin(),
            simulation.claims.end(),
            [&reservation](const ClaimEvent& claim) {
                return claim.spot == reservation.representativeSpot && claim.served;
            });
        if (!covered) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] TrafficSafety calculate_traffic_safety(
    const MatchConfig& config,
    const SimulationResult& result) {
    TrafficSafety safety;
    const std::int64_t busyOwnThreshold =
        static_cast<std::int64_t>(config.players) *
        config.busyThreshold;
    const std::int64_t jammedOwnThreshold =
        static_cast<std::int64_t>(config.players) *
        config.jammedThreshold;
    for (const CellId road : config.roadCells) {
        const std::int32_t stays = result.roadFootprint.at(static_cast<std::size_t>(road));
        safety.totalRoadStays += stays;
        if (stays >= busyOwnThreshold || stays >= jammedOwnThreshold) {
            ++safety.thresholdCrossings;
        }
        if (stays == busyOwnThreshold - 1 || stays == jammedOwnThreshold - 1) {
            ++safety.thresholdBandRoads;
        }
    }
    return safety;
}

[[nodiscard]] std::vector<std::vector<std::int32_t>> build_terminal_distance_cache(const MatchConfig& config) {
    const std::int32_t cellCount = config.map.cell_count();
    std::vector<std::vector<std::int32_t>> distances;
    distances.reserve(config.spots.size());
    for (const Spot& spot : config.spots) {
        std::vector<std::int32_t> distance(
            static_cast<std::size_t>(cellCount),
            std::numeric_limits<std::int32_t>::max());
        using QueueEntry = std::pair<std::int32_t, CellId>;
        std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> queue;
        distance.at(static_cast<std::size_t>(spot.position)) = 0;
        queue.push(QueueEntry{0, spot.position});
        while (!queue.empty()) {
            const auto [travelSteps, cell] = queue.top();
            queue.pop();
            if (travelSteps != distance.at(static_cast<std::size_t>(cell))) {
                continue;
            }
            for (std::int32_t direction = 0; direction < kDirectionCount; ++direction) {
                const CellId predecessor = config.map.neighbors.at(static_cast<std::size_t>(cell)).at(
                    static_cast<std::size_t>(direction));
                if (predecessor == kInvalidCell ||
                    config.map.terrain.at(static_cast<std::size_t>(predecessor)) == Terrain::Pond) {
                    continue;
                }
                const MoveCost cost = config.move_cost(predecessor, RoadStatus::Smooth);
                const std::int32_t candidate = travelSteps + cost.steps;
                if (candidate >= distance.at(static_cast<std::size_t>(predecessor))) {
                    continue;
                }
                distance.at(static_cast<std::size_t>(predecessor)) = candidate;
                queue.push(QueueEntry{candidate, predecessor});
            }
        }
        distances.push_back(std::move(distance));
    }
    return distances;
}

[[nodiscard]] TerminalSlack calculate_terminal_slack(
    const MatchConfig& config,
    const std::vector<std::vector<std::int32_t>>& distancesToSpots,
    const SimulationResult& result,
    const MatchLedger& ledger) {
    TerminalSlack slack;
    const std::uint64_t collectedBrands = ledger.lifetimeBrands | result.score.brands;
    for (const AgentState& agent : result.finalAgents) {
        if (agent.kind == AgentKind::Patrol) {
            slack.patrolFuelReserve += agent.fuel;
            const SpotIndex terminalSpot = config.spotAtCell.at(static_cast<std::size_t>(agent.position));
            if (terminalSpot != kInvalidSpot) {
                ++slack.overnightSpotCount;
            }
        }
    }
    constexpr std::int32_t unreachable = 1000000;
    for (std::int32_t brandIndex = 0; brandIndex < config.brand_count(); ++brandIndex) {
        if (has_brand(collectedBrands, brandIndex)) {
            continue;
        }
        std::int32_t bestDistance = unreachable;
        for (SpotIndex spotIndex = 0; spotIndex < static_cast<SpotIndex>(config.spots.size()); ++spotIndex) {
            if (config.spots.at(static_cast<std::size_t>(spotIndex)).brandIndex != brandIndex) {
                continue;
            }
            const std::vector<std::int32_t>& distance = distancesToSpots.at(static_cast<std::size_t>(spotIndex));
            for (const AgentState& agent : result.finalAgents) {
                if (agent.kind != AgentKind::Patrol) {
                    continue;
                }
                const std::int32_t candidate = distance.at(static_cast<std::size_t>(agent.position));
                bestDistance = std::min(bestDistance, candidate);
            }
        }
        slack.worstRemainingBrandSteps = std::max(slack.worstRemainingBrandSteps, bestDistance);
        slack.totalRemainingBrandSteps += bestDistance;
    }
    return slack;
}

void populate_terminal_brand_feature(
    const MatchConfig& config,
    const MatchLedger& ledger,
    const std::vector<std::vector<std::int32_t>>& distancesToSpots,
    RouteColumn& column) {
    if (!column.hasExactTimeline || column.terminalCell == kInvalidCell) {
        return;
    }
    const std::uint64_t collectedBrands = ledger.lifetimeBrands | column.estimatedBrands;
    std::int32_t nearest = std::numeric_limits<std::int32_t>::max();
    for (SpotIndex spotIndex = 0; spotIndex < static_cast<SpotIndex>(config.spots.size()); ++spotIndex) {
        const Spot& spot = config.spots.at(static_cast<std::size_t>(spotIndex));
        if (has_brand(collectedBrands, spot.brandIndex)) {
            continue;
        }
        nearest = std::min(
            nearest,
            distancesToSpots.at(static_cast<std::size_t>(spotIndex)).at(
                static_cast<std::size_t>(column.terminalCell)));
    }
    column.terminalFeatures.nearestUncollectedBrandSteps = nearest;
}

[[nodiscard]] bool better_candidate(const MasterCandidate& left, const MasterCandidate& right) {
    const std::int32_t scoreOrder = compare_lexicographic(left.scoreAfterToday, right.scoreAfterToday);
    if (scoreOrder != 0) {
        return scoreOrder > 0;
    }
    const std::int32_t terminalOrder = compare_terminal_slack(left.terminalSlack, right.terminalSlack);
    if (terminalOrder != 0) {
        return terminalOrder > 0;
    }
    if (left.trafficSafety.thresholdCrossings != right.trafficSafety.thresholdCrossings) {
        return left.trafficSafety.thresholdCrossings < right.trafficSafety.thresholdCrossings;
    }
    if (left.trafficSafety.thresholdBandRoads != right.trafficSafety.thresholdBandRoads) {
        return left.trafficSafety.thresholdBandRoads < right.trafficSafety.thresholdBandRoads;
    }
    if (left.trafficSafety.totalRoadStays != right.trafficSafety.totalRoadStays) {
        return left.trafficSafety.totalRoadStays < right.trafficSafety.totalRoadStays;
    }
    return left.stableId < right.stableId;
}

[[nodiscard]] std::int32_t candidate_plan_distance(
    const MasterCandidate& left,
    const MasterCandidate& right) {
    const std::size_t agentCount = std::max(
        left.plan.actions.size(),
        right.plan.actions.size());
    std::int32_t distance = 0;
    for (std::size_t agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        if (agentIndex >= left.plan.actions.size() ||
            agentIndex >= right.plan.actions.size()) {
            const AgentPlan& existing = agentIndex < left.plan.actions.size()
                ? left.plan.actions.at(agentIndex)
                : right.plan.actions.at(agentIndex);
            distance += std::max(1, static_cast<std::int32_t>(existing.size()));
            continue;
        }
        const AgentPlan& leftActions = left.plan.actions.at(agentIndex);
        const AgentPlan& rightActions = right.plan.actions.at(agentIndex);
        const std::size_t actionCount = std::max(leftActions.size(), rightActions.size());
        for (std::size_t actionIndex = 0; actionIndex < actionCount; ++actionIndex) {
            if (actionIndex >= leftActions.size() ||
                actionIndex >= rightActions.size() ||
                leftActions.at(actionIndex).kind != rightActions.at(actionIndex).kind ||
                leftActions.at(actionIndex).value != rightActions.at(actionIndex).value) {
                ++distance;
            }
        }
    }
    return distance;
}

void retain_alns_population(
    std::vector<MasterCandidate>& candidates,
    std::int32_t maximumCandidates,
    std::int32_t diversityCandidates) {
    std::sort(candidates.begin(), candidates.end(), better_candidate);
    if (static_cast<std::int32_t>(candidates.size()) <= maximumCandidates) {
        return;
    }
    const std::int32_t diversitySlots = std::clamp(
        diversityCandidates,
        0,
        maximumCandidates - 1);
    if (diversitySlots == 0) {
        candidates.resize(static_cast<std::size_t>(maximumCandidates));
        return;
    }
    const std::size_t qualityCount = static_cast<std::size_t>(
        maximumCandidates - diversitySlots);
    std::vector<std::size_t> selected;
    selected.reserve(static_cast<std::size_t>(maximumCandidates));
    for (std::size_t index = 0; index < qualityCount; ++index) {
        selected.push_back(index);
    }
    std::vector<bool> used(candidates.size(), false);
    for (const std::size_t index : selected) {
        used.at(index) = true;
    }
    while (selected.size() < static_cast<std::size_t>(maximumCandidates)) {
        std::size_t bestIndex = candidates.size();
        std::int32_t bestMinimumDistance = -1;
        for (std::size_t candidateIndex = qualityCount;
             candidateIndex < candidates.size();
             ++candidateIndex) {
            if (used.at(candidateIndex)) {
                continue;
            }
            std::int32_t minimumDistance = std::numeric_limits<std::int32_t>::max();
            for (const std::size_t selectedIndex : selected) {
                minimumDistance = std::min(
                    minimumDistance,
                    candidate_plan_distance(
                        candidates.at(candidateIndex),
                        candidates.at(selectedIndex)));
            }
            if (minimumDistance > bestMinimumDistance) {
                bestMinimumDistance = minimumDistance;
                bestIndex = candidateIndex;
            }
        }
        if (bestIndex == candidates.size()) {
            break;
        }
        used.at(bestIndex) = true;
        selected.push_back(bestIndex);
    }
    std::sort(selected.begin(), selected.end());
    std::vector<MasterCandidate> retained;
    retained.reserve(selected.size());
    for (const std::size_t index : selected) {
        retained.push_back(std::move(candidates.at(index)));
    }
    std::sort(retained.begin(), retained.end(), better_candidate);
    candidates = std::move(retained);
}

[[nodiscard]] std::uint64_t column_brand_mask(
    const MatchConfig& config,
    const RouteColumn& column) {
    std::uint64_t result = 0;
    for (const ColumnVisitEvent& event : column.firstVisits) {
        result |= brand_bit(config.spots.at(static_cast<std::size_t>(event.spot)).brandIndex);
    }
    return result;
}

[[nodiscard]] bool portfolio_has_exact_metadata(const RoutePortfolio& portfolio) {
    return std::all_of(
        portfolio.columnsByAgent.begin(),
        portfolio.columnsByAgent.end(),
        [](const std::vector<RouteColumn>& columns) {
            return std::all_of(
                columns.begin(),
                columns.end(),
                [](const RouteColumn& column) { return column.hasExactTimeline; });
        });
}

[[nodiscard]] OfficialScore optimistic_partial_score(
    const MatchConfig& config,
    const MatchLedger& ledger,
    const std::vector<const RouteColumn*>& selected,
    const std::vector<AgentIndex>& ordering,
    std::size_t depth,
    const std::vector<std::vector<const RouteColumn*>>& orderedColumns) {
    std::uint64_t dailyBrands = 0;
    std::vector<std::int32_t> possibleClaims(config.spots.size(), 0);
    for (const RouteColumn* column : selected) {
        if (column == nullptr) {
            continue;
        }
        dailyBrands |= column_brand_mask(config, *column);
        for (const ColumnVisitEvent& event : column->firstVisits) {
            if (event.claimedServing) {
                ++possibleClaims.at(static_cast<std::size_t>(event.spot));
            }
        }
    }
    for (std::size_t remainingDepth = depth; remainingDepth < ordering.size(); ++remainingDepth) {
        const AgentIndex agentIndex = ordering.at(remainingDepth);
        std::uint64_t possibleBrands = 0;
        std::vector<bool> agentCanClaim(config.spots.size(), false);
        for (const RouteColumn* column : orderedColumns.at(static_cast<std::size_t>(agentIndex))) {
            possibleBrands |= column_brand_mask(config, *column);
            for (const ColumnVisitEvent& event : column->firstVisits) {
                if (event.claimedServing) {
                    agentCanClaim.at(static_cast<std::size_t>(event.spot)) = true;
                }
            }
        }
        for (std::size_t spotOffset = 0; spotOffset < agentCanClaim.size(); ++spotOffset) {
            if (agentCanClaim.at(spotOffset)) {
                ++possibleClaims.at(spotOffset);
            }
        }
        dailyBrands |= possibleBrands;
    }
    std::int32_t servings = 0;
    for (std::size_t spotOffset = 0; spotOffset < possibleClaims.size(); ++spotOffset) {
        servings += std::min(
            possibleClaims.at(spotOffset),
            config.spots.at(spotOffset).stock);
    }
    return OfficialScore{
        static_cast<std::int32_t>(std::popcount(ledger.lifetimeBrands | dailyBrands)),
        ledger.totalDailyDistinct + static_cast<std::int32_t>(std::popcount(dailyBrands)),
        ledger.totalServings + servings,
    };
}

[[nodiscard]] const MasterCandidate& worst_candidate(const std::vector<MasterCandidate>& candidates) {
    const MasterCandidate* worst = &candidates.front();
    for (const MasterCandidate& candidate : candidates) {
        if (better_candidate(*worst, candidate)) {
            worst = &candidate;
        }
    }
    return *worst;
}

struct StockCutState {
    struct EventKey {
        std::int32_t step = 0;
        AgentIndex agent = kInvalidAgent;

        [[nodiscard]] friend bool operator==(const EventKey& left, const EventKey& right) = default;
        [[nodiscard]] friend bool operator<(const EventKey& left, const EventKey& right) {
            return std::tie(left.step, left.agent) < std::tie(right.step, right.agent);
        }
    };

    struct PrefixCut {
        EventKey denied;
        std::vector<EventKey> predecessors;
    };

    std::vector<bool> capacityCut;
    std::vector<bool> promoted;
    std::vector<std::vector<PrefixCut>> prefixes;
};

struct ServiceCreditAssignment {
    struct Event {
        SpotIndex spot = kInvalidSpot;
        StockCutState::EventKey key;
        bool credited = false;
    };

    std::vector<Event> events;
    std::int32_t creditedServings = 0;
    std::int32_t deniedServings = 0;
};

[[nodiscard]] ServiceCreditAssignment assign_service_credits(
    const MatchConfig& config,
    const std::vector<const RouteColumn*>& selected) {
    ServiceCreditAssignment assignment;
    for (const RouteColumn* column : selected) {
        if (column == nullptr) {
            continue;
        }
        for (const ColumnVisitEvent& visit : column->firstVisits) {
            if (visit.claimedServing) {
                assignment.events.push_back(ServiceCreditAssignment::Event{
                    visit.spot,
                    StockCutState::EventKey{visit.step, column->agent},
                    false,
                });
            }
        }
    }
    std::sort(
        assignment.events.begin(),
        assignment.events.end(),
        [](const ServiceCreditAssignment::Event& left, const ServiceCreditAssignment::Event& right) {
            return std::tie(left.spot, left.key.step, left.key.agent) <
                std::tie(right.spot, right.key.step, right.key.agent);
        });
    std::size_t begin = 0;
    while (begin < assignment.events.size()) {
        const SpotIndex spot = assignment.events.at(begin).spot;
        std::size_t end = begin;
        while (end < assignment.events.size() && assignment.events.at(end).spot == spot) {
            ++end;
        }
        std::int32_t remainingCredits = config.spots.at(static_cast<std::size_t>(spot)).stock;
        for (std::size_t eventIndex = begin; eventIndex < end; ++eventIndex) {
            ServiceCreditAssignment::Event& event = assignment.events.at(eventIndex);
            event.credited = remainingCredits > 0;
            if (event.credited) {
                ++assignment.creditedServings;
                --remainingCredits;
            } else {
                ++assignment.deniedServings;
            }
        }
        begin = end;
    }
    return assignment;
}

[[nodiscard]] bool credits_match_exact(
    const ServiceCreditAssignment& assignment,
    const SimulationResult& simulation) {
    for (const ClaimEvent& claim : simulation.claims) {
        const auto event = std::find_if(
            assignment.events.begin(),
            assignment.events.end(),
            [&claim](const ServiceCreditAssignment::Event& candidate) {
                return candidate.spot == claim.spot && candidate.key.step == claim.step &&
                    candidate.key.agent == claim.agent;
            });
        if (event == assignment.events.end() || event->credited != claim.served) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool record_stock_diagnostics(
    const SimulationResult& result,
    StockCutState& cutState,
    MasterDiagnostics& diagnostics) {
    bool learnedCut = false;
    for (std::size_t claimIndex = 0; claimIndex < result.claims.size(); ++claimIndex) {
        const ClaimEvent& claim = result.claims.at(claimIndex);
        if (claim.served) {
            continue;
        }
        ++diagnostics.stockCapacityConflicts;
        const std::size_t spotOffset = static_cast<std::size_t>(claim.spot);
        if (!cutState.capacityCut.at(spotOffset)) {
            cutState.capacityCut.at(spotOffset) = true;
            ++diagnostics.capCuts;
            learnedCut = true;
        }
        StockCutState::PrefixCut prefix;
        prefix.denied = StockCutState::EventKey{claim.step, claim.agent};
        for (std::size_t precedingIndex = 0; precedingIndex <= claimIndex; ++precedingIndex) {
            const ClaimEvent& preceding = result.claims.at(precedingIndex);
            if (preceding.spot == claim.spot) {
                prefix.predecessors.push_back(StockCutState::EventKey{preceding.step, preceding.agent});
            }
        }
        std::vector<StockCutState::PrefixCut>& prefixes = cutState.prefixes.at(spotOffset);
        const bool isNewPrefix = std::none_of(
            prefixes.begin(),
            prefixes.end(),
            [&prefix](const StockCutState::PrefixCut& existing) {
                return existing.denied == prefix.denied && existing.predecessors == prefix.predecessors;
            });
        if (isNewPrefix) {
            prefixes.push_back(std::move(prefix));
            ++diagnostics.prefixConflicts;
            ++diagnostics.prefixCuts;
            learnedCut = true;
            if (prefixes.size() == 2U) {
                ++diagnostics.hotspotPromotions;
                cutState.promoted.at(spotOffset) = true;
            }
        }
    }
    return learnedCut;
}

[[nodiscard]] std::int32_t conflict_aware_priority(
    const RouteColumn& column,
    const StockCutState& cutState) {
    std::int32_t penalty = 0;
    for (const ColumnVisitEvent& event : column.firstVisits) {
        const std::size_t spotOffset = static_cast<std::size_t>(event.spot);
        if (cutState.capacityCut.at(spotOffset)) {
            penalty += 500;
        }
        for (const StockCutState::PrefixCut& prefix : cutState.prefixes.at(spotOffset)) {
            if (prefix.denied == StockCutState::EventKey{event.step, column.agent}) {
                penalty += cutState.promoted.at(spotOffset) ? 1000 : 500;
            }
        }
    }
    return column.priority - penalty;
}

[[nodiscard]] SparseRoadFootprint aggregate_column_footprint(const std::vector<const RouteColumn*>& selected) {
    SparseRoadFootprint aggregate;
    for (const RouteColumn* column : selected) {
        for (const auto& [road, stays] : column->fullFootprint.entries) {
            aggregate.add(road, stays);
        }
    }
    return aggregate;
}

void verify_column_footprint(
    const MatchConfig& config,
    const std::vector<const RouteColumn*>& selected,
    const SimulationResult& simulation) {
    if (std::all_of(
            selected.begin(),
            selected.end(),
            [](const RouteColumn* column) { return column->hasExactTimeline; })) {
        const SparseRoadFootprint aggregate = aggregate_column_footprint(selected);
        for (const CellId road : config.roadCells) {
            if (aggregate.at(road) != simulation.roadFootprint.at(static_cast<std::size_t>(road))) {
                throw std::runtime_error("column timeline footprint disagrees with the exact simulator");
            }
        }
    }
    for (const RouteColumn* column : selected) {
        if (!column->hasExactTimeline ||
            column->agent < 0 || column->agent >= static_cast<AgentIndex>(simulation.finalAgents.size()) ||
            simulation.finalAgents.at(static_cast<std::size_t>(column->agent)).kind != AgentKind::Patrol) {
            continue;
        }
        std::vector<ColumnVisitEvent> actual;
        for (const ClaimEvent& claim : simulation.claims) {
            if (claim.agent == column->agent) {
                actual.push_back(ColumnVisitEvent{claim.spot, claim.step, true});
            }
        }
        if (actual.size() != column->firstVisits.size()) {
            throw std::runtime_error("column first-visit timeline disagrees with the exact simulator");
        }
        for (std::size_t eventIndex = 0; eventIndex < actual.size(); ++eventIndex) {
            if (actual.at(eventIndex).spot != column->firstVisits.at(eventIndex).spot ||
                actual.at(eventIndex).step != column->firstVisits.at(eventIndex).step) {
                throw std::runtime_error("column first-visit event disagrees with the exact simulator");
            }
        }
    }
}

} 

RouteColumnGenerator::RouteColumnGenerator(const MatchConfig& config, const ParetoRouter& router)
    : config_(config),
      router_(router),
      terminalDistancesToSpots_(build_terminal_distance_cache(config)) {}

RoutePortfolio RouteColumnGenerator::generate(
    const DayState& state,
    const MatchLedger& ledger,
    const ColumnGenerationOptions& options) const {
    if (static_cast<std::int32_t>(state.agents.size()) != config_.agent_count()) {
        throw std::invalid_argument("column generation requires a complete day state");
    }
    RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(static_cast<std::size_t>(config_.agent_count()));
    std::int32_t nextColumnId = 0;
    std::int32_t nextEscortGroup = 0;
    const std::vector<CellId> criticalRoads = select_critical_roads(config_, state, options.criticalRoadHints);
    const auto deadline_expired = [&options]() {
        return options.deadline.has_value() && std::chrono::steady_clock::now() >= *options.deadline;
    };
    std::vector<std::vector<std::vector<ParetoPath>>> spotTransitionCache(
        config_.spots.size(),
        std::vector<std::vector<ParetoPath>>(config_.spots.size()));
    std::vector<std::vector<bool>> spotTransitionReady(
        config_.spots.size(),
        std::vector<bool>(config_.spots.size(), false));
    const auto spot_transition = [this,
                                  &state,
                                  &options,
                                  &criticalRoads,
                                  &spotTransitionCache,
                                  &spotTransitionReady](
                                     SpotIndex from,
                                     SpotIndex to,
                                     std::int32_t remainingSteps,
                                     std::int32_t remainingFuel) -> std::optional<ParetoPath> {
        if (remainingSteps < 0 || remainingFuel < 0) {
            return std::nullopt;
        }
        const std::size_t fromOffset = static_cast<std::size_t>(from);
        const std::size_t toOffset = static_cast<std::size_t>(to);
        if (!spotTransitionReady.at(fromOffset).at(toOffset)) {
            spotTransitionReady.at(fromOffset).at(toOffset) = true;
            ParetoSearchOptions transitionOptions;
            transitionOptions.maximumTravelSteps = config_.steps_for_day(state.dayNumber);
            transitionOptions.maximumPatrolFuel = config_.fuelLimit;
            transitionOptions.maximumLabelsPerCell = 32;
            transitionOptions.maximumPaths = std::max(1, std::min(2, options.maximumPathsPerTarget));
            transitionOptions.patrol = true;
            transitionOptions.criticalRoads = criticalRoads;
            transitionOptions.deadline = options.deadline;
            spotTransitionCache.at(fromOffset).at(toOffset) = router_.find_paths(
                config_.spots.at(fromOffset).position,
                config_.spots.at(toOffset).position,
                state.roadStatuses,
                transitionOptions);
        }
        for (const ParetoPath& path : spotTransitionCache.at(fromOffset).at(toOffset)) {
            if (path.travelSteps <= remainingSteps && path.patrolFuel <= remainingFuel) {
                return path;
            }
        }
        return std::nullopt;
    };
    std::int32_t nextContingencyBundle = 0;
    std::int32_t acceptedSeedPlans = 0;
    std::set<std::string> seenSeedPlans;
    for (const DayPlan& seedPlan : options.seedPlans) {
        if (deadline_expired() || acceptedSeedPlans >= std::max(0, options.maximumSeedPlans)) {
            break;
        }
        if (seedPlan.actions.size() != static_cast<std::size_t>(config_.agent_count())) {
            continue;
        }
        if (!seenSeedPlans.insert(canonical_plan_bytes(seedPlan)).second) {
            continue;
        }
        const std::int32_t bundle = nextContingencyBundle++;
        ++acceptedSeedPlans;
        for (AgentIndex agentIndex = 0; agentIndex < config_.agent_count(); ++agentIndex) {
            const AgentState& agent = state.agents.at(static_cast<std::size_t>(agentIndex));
            RouteColumn column;
            column.columnId = nextColumnId++;
            column.agent = agentIndex;
            column.actions = seedPlan.actions.at(static_cast<std::size_t>(agentIndex));
            column.terminalCell = agent.position;
            column.terminalFuel = agent.fuel;
            column.contingencyBundle = bundle;
            column.priority = 1750000;
            portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex)).push_back(std::move(column));
        }
    }

    for (AgentIndex agentIndex = 0; agentIndex < config_.agent_count(); ++agentIndex) {
        const AgentState& agent = state.agents.at(static_cast<std::size_t>(agentIndex));
        RouteColumn waitColumn;
        waitColumn.columnId = nextColumnId++;
        waitColumn.agent = agentIndex;
        waitColumn.actions = wait_actions(config_, state);
        waitColumn.terminalCell = agent.position;
        waitColumn.terminalFuel = agent.fuel;
        waitColumn.priority = -1;
        portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex)).push_back(std::move(waitColumn));
        if (deadline_expired()) {
            continue;
        }

        ParetoSearchOptions searchOptions;
        searchOptions.maximumTravelSteps = config_.steps_for_day(state.dayNumber);
        searchOptions.maximumPatrolFuel = agent.kind == AgentKind::Patrol ? agent.fuel : 0;
        searchOptions.maximumLabelsPerCell = 32;
        searchOptions.maximumPaths = std::max(1, options.maximumPathsPerTarget);
        searchOptions.patrol = agent.kind == AgentKind::Patrol;
        searchOptions.criticalRoads = criticalRoads;
        searchOptions.deadline = options.deadline;

        std::vector<TargetSeed> seeds;
        std::vector<SpotIndex> targetSpots;
        targetSpots.reserve(config_.spots.size());
        for (SpotIndex spotIndex = 0;
             spotIndex < static_cast<SpotIndex>(config_.spots.size());
             ++spotIndex) {
            targetSpots.push_back(spotIndex);
        }
        std::sort(
            targetSpots.begin(),
            targetSpots.end(),
            [this, &ledger, &options, &state](SpotIndex left, SpotIndex right) {
                const Spot& leftSpot = config_.spots.at(static_cast<std::size_t>(left));
                const Spot& rightSpot = config_.spots.at(static_cast<std::size_t>(right));
                const std::int32_t leftPriority =
                    reservation_priority(options.mandatoryReservations, state, left) +
                    (!has_brand(ledger.lifetimeBrands, leftSpot.brandIndex) ? 1000000 : 0) +
                    (config_.brand_count() - brand_rarity(config_, leftSpot.brandIndex)) * 1000 +
                    leftSpot.stock * 10;
                const std::int32_t rightPriority =
                    reservation_priority(options.mandatoryReservations, state, right) +
                    (!has_brand(ledger.lifetimeBrands, rightSpot.brandIndex) ? 1000000 : 0) +
                    (config_.brand_count() - brand_rarity(config_, rightSpot.brandIndex)) * 1000 +
                    rightSpot.stock * 10;
                if (leftPriority != rightPriority) {
                    return leftPriority > rightPriority;
                }
                return left < right;
            });
        const std::int32_t targetQueryLimit = std::min<std::int32_t>(
            static_cast<std::int32_t>(targetSpots.size()),
            std::max(0, options.maximumTargetSpots) * 2);
        targetSpots.resize(static_cast<std::size_t>(targetQueryLimit));
        for (const SpotIndex spotIndex : targetSpots) {
            if (deadline_expired()) {
                break;
            }
            const Spot& spot = config_.spots.at(static_cast<std::size_t>(spotIndex));
            const std::vector<ParetoPath> paths = router_.find_paths(
                agent.position,
                spot.position,
                state.roadStatuses,
                searchOptions);
            const bool newLifetimeBrand = !has_brand(ledger.lifetimeBrands, spot.brandIndex);
            const std::int32_t rarity = brand_rarity(config_, spot.brandIndex);
            for (const ParetoPath& path : paths) {
                const std::int32_t priority =
                    reservation_priority(options.mandatoryReservations, state, spotIndex) +
                    (newLifetimeBrand ? 1000000 : 0) + (config_.brand_count() - rarity) * 1000 + spot.stock * 10 -
                    path.travelSteps;
                seeds.push_back(TargetSeed{spotIndex, path, priority});
            }
        }
        if (agent.kind == AgentKind::Patrol && !deadline_expired()) {
            std::set<SpotIndex> directlyReachable;
            for (const TargetSeed& seed : seeds) {
                directlyReachable.insert(seed.spot);
            }
            const std::int32_t remainingHorizonSteps = std::accumulate(
                config_.daySteps.begin() +
                    static_cast<std::ptrdiff_t>(state.dayNumber - 1),
                config_.daySteps.end(),
                0);
            const std::int32_t stagingLimit = std::min<std::int32_t>(
                4,
                static_cast<std::int32_t>(targetSpots.size()));
            std::int32_t stagingColumns = 0;
            for (const SpotIndex spotIndex : targetSpots) {
                if (stagingColumns >= stagingLimit || deadline_expired()) {
                    break;
                }
                if (directlyReachable.contains(spotIndex)) {
                    continue;
                }
                ParetoSearchOptions stagingOptions = searchOptions;
                stagingOptions.maximumTravelSteps = remainingHorizonSteps;
                stagingOptions.maximumPaths = 1;
                const Spot& spot = config_.spots.at(
                    static_cast<std::size_t>(spotIndex));
                const std::vector<ParetoPath> fullPaths = router_.find_paths(
                    agent.position,
                    spot.position,
                    state.roadStatuses,
                    stagingOptions);
                if (fullPaths.empty()) {
                    continue;
                }
                const std::optional<ParetoPath> prefix = staging_prefix(
                    config_,
                    state,
                    agent,
                    fullPaths.front());
                if (!prefix.has_value()) {
                    continue;
                }
                RouteColumn stagingColumn;
                stagingColumn.columnId = nextColumnId++;
                stagingColumn.agent = agentIndex;
                stagingColumn.actions = complete_actions(
                    config_,
                    state,
                    *prefix);
                CellId terminal = agent.position;
                for (const std::int32_t direction : prefix->directions) {
                    terminal = config_.map.neighbors.at(
                        static_cast<std::size_t>(terminal)).at(
                            static_cast<std::size_t>(direction));
                }
                stagingColumn.terminalCell = terminal;
                stagingColumn.terminalFuel = agent.fuel - prefix->patrolFuel;
                stagingColumn.heuristicFootprint = prefix->heuristicFootprint;
                stagingColumn.priority =
                    reservation_priority(
                        options.mandatoryReservations,
                        state,
                        spotIndex) +
                    (!has_brand(ledger.lifetimeBrands, spot.brandIndex)
                         ? 700000
                         : 0) +
                    (config_.brand_count() -
                     brand_rarity(config_, spot.brandIndex)) *
                        1000 -
                    prefix->travelSteps;
                portfolio.columnsByAgent.at(
                    static_cast<std::size_t>(agentIndex)).push_back(
                        std::move(stagingColumn));
                ++stagingColumns;
            }
        }
        std::sort(
            seeds.begin(),
            seeds.end(),
            [](const TargetSeed& left, const TargetSeed& right) {
                if (left.priority != right.priority) {
                    return left.priority > right.priority;
                }
                if (left.path.travelSteps != right.path.travelSteps) {
                    return left.path.travelSteps < right.path.travelSteps;
                }
                return left.spot < right.spot;
            });
        if (static_cast<std::int32_t>(seeds.size()) > options.maximumTargetSpots) {
            std::vector<TargetSeed> diverseSeeds;
            diverseSeeds.reserve(static_cast<std::size_t>(options.maximumTargetSpots));
            std::set<SpotIndex> representedSpots;
            std::vector<bool> selected(seeds.size(), false);
            for (std::size_t seedIndex = 0;
                 seedIndex < seeds.size() &&
                 static_cast<std::int32_t>(diverseSeeds.size()) < options.maximumTargetSpots;
                 ++seedIndex) {
                if (representedSpots.insert(seeds.at(seedIndex).spot).second) {
                    diverseSeeds.push_back(seeds.at(seedIndex));
                    selected.at(seedIndex) = true;
                }
            }
            for (std::size_t seedIndex = 0;
                 seedIndex < seeds.size() &&
                 static_cast<std::int32_t>(diverseSeeds.size()) < options.maximumTargetSpots;
                 ++seedIndex) {
                if (!selected.at(seedIndex)) {
                    diverseSeeds.push_back(seeds.at(seedIndex));
                }
            }
            seeds = std::move(diverseSeeds);
        }

        for (const TargetSeed& seed : seeds) {
            RouteColumn column;
            column.columnId = nextColumnId++;
            column.agent = agentIndex;
            column.actions = complete_actions(config_, state, seed.path);
            column.terminalCell = config_.spots.at(static_cast<std::size_t>(seed.spot)).position;
            column.terminalFuel = agent.kind == AgentKind::Patrol ? agent.fuel - seed.path.patrolFuel : agent.fuel;
            column.heuristicFootprint = seed.path.heuristicFootprint;
            column.priority = seed.priority;
            if (agent.kind == AgentKind::Patrol) {
                const Spot& spot = config_.spots.at(static_cast<std::size_t>(seed.spot));
                column.estimatedBrands = brand_bit(spot.brandIndex);
                column.estimatedServings = 1;
            }
            portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex)).push_back(std::move(column));
        }

        if (agent.kind == AgentKind::Patrol && !deadline_expired()) {
            const std::int32_t pairLimit = std::min<std::int32_t>(static_cast<std::int32_t>(seeds.size()), 6);
            std::int32_t tripleColumnsGenerated = 0;
            const std::int32_t tripleColumnBudget = std::max(
                1,
                options.maximumColumnsPerAgent);
            std::int32_t quadrupleColumnsGenerated = 0;
            const std::int32_t quadrupleColumnBudget = std::max(
                1,
                options.maximumColumnsPerAgent / 8);
            for (std::int32_t firstIndex = 0; firstIndex < pairLimit; ++firstIndex) {
                if (deadline_expired()) {
                    break;
                }
                const TargetSeed& first = seeds.at(static_cast<std::size_t>(firstIndex));
                const Spot& firstSpot = config_.spots.at(static_cast<std::size_t>(first.spot));
                for (std::int32_t secondIndex = 0; secondIndex < pairLimit; ++secondIndex) {
                    if (deadline_expired()) {
                        break;
                    }
                    const TargetSeed& second = seeds.at(static_cast<std::size_t>(secondIndex));
                    if (first.spot == second.spot) {
                        continue;
                    }
                    const Spot& secondSpot = config_.spots.at(static_cast<std::size_t>(second.spot));
                    const std::optional<ParetoPath> secondPathResult = spot_transition(
                        first.spot,
                        second.spot,
                        config_.steps_for_day(state.dayNumber) - first.path.travelSteps,
                        agent.fuel - first.path.patrolFuel);
                    if (!secondPathResult.has_value()) {
                        continue;
                    }
                    const ParetoPath& secondPath = *secondPathResult;
                    ParetoPath combined;
                    combined.directions = first.path.directions;
                    combined.directions.insert(
                        combined.directions.end(),
                        secondPath.directions.begin(),
                        secondPath.directions.end());
                    combined.travelSteps = first.path.travelSteps + secondPath.travelSteps;
                    combined.patrolFuel = first.path.patrolFuel + secondPath.patrolFuel;
                    combined.heuristicFootprint = first.path.heuristicFootprint;
                    for (const auto& [road, stays] : secondPath.heuristicFootprint.entries) {
                        combined.heuristicFootprint.add(road, stays);
                    }
                    RouteColumn column;
                    column.columnId = nextColumnId++;
                    column.agent = agentIndex;
                    column.actions = complete_actions(config_, state, combined);
                    column.terminalCell = secondSpot.position;
                    column.terminalFuel = agent.fuel - combined.patrolFuel;
                    column.estimatedBrands = brand_bit(firstSpot.brandIndex) | brand_bit(secondSpot.brandIndex);
                    column.estimatedServings = 2;
                    column.heuristicFootprint = combined.heuristicFootprint;
                    column.priority = first.priority + second.priority;
                    portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex)).push_back(std::move(column));

                    if (options.maximumColumnsPerAgent < 12 ||
                        tripleColumnsGenerated >= tripleColumnBudget) {
                        continue;
                    }
                    const std::int32_t tripleLimit = std::min(pairLimit, 5);
                    for (std::int32_t thirdIndex = 0;
                         thirdIndex < tripleLimit &&
                         tripleColumnsGenerated < tripleColumnBudget;
                         ++thirdIndex) {
                        if (deadline_expired()) {
                            break;
                        }
                        const TargetSeed& third = seeds.at(static_cast<std::size_t>(thirdIndex));
                        if (third.spot == first.spot || third.spot == second.spot) {
                            continue;
                        }
                        const Spot& thirdSpot =
                            config_.spots.at(static_cast<std::size_t>(third.spot));
                        const std::optional<ParetoPath> thirdPathResult = spot_transition(
                            second.spot,
                            third.spot,
                            config_.steps_for_day(state.dayNumber) - combined.travelSteps,
                            agent.fuel - combined.patrolFuel);
                        if (!thirdPathResult.has_value()) {
                            continue;
                        }
                        const ParetoPath& thirdPath = *thirdPathResult;
                        ParetoPath triple = combined;
                        triple.directions.insert(
                            triple.directions.end(),
                            thirdPath.directions.begin(),
                            thirdPath.directions.end());
                        triple.travelSteps += thirdPath.travelSteps;
                        triple.patrolFuel += thirdPath.patrolFuel;
                        for (const auto& [road, stays] : thirdPath.heuristicFootprint.entries) {
                            triple.heuristicFootprint.add(road, stays);
                        }
                        const std::uint64_t tripleBrands =
                            brand_bit(firstSpot.brandIndex) |
                            brand_bit(secondSpot.brandIndex) |
                            brand_bit(thirdSpot.brandIndex);
                        RouteColumn tripleColumn;
                        tripleColumn.columnId = nextColumnId++;
                        tripleColumn.agent = agentIndex;
                        tripleColumn.actions = complete_actions(config_, state, triple);
                        tripleColumn.terminalCell = thirdSpot.position;
                        tripleColumn.terminalFuel = agent.fuel - triple.patrolFuel;
                        tripleColumn.estimatedBrands = tripleBrands;
                        tripleColumn.estimatedServings = 3;
                        tripleColumn.heuristicFootprint =
                            triple.heuristicFootprint;
                        tripleColumn.priority =
                            first.priority + second.priority + third.priority;
                        portfolio.columnsByAgent.at(
                            static_cast<std::size_t>(agentIndex)).push_back(
                                std::move(tripleColumn));
                        ++tripleColumnsGenerated;

                        if (options.maximumColumnsPerAgent < 16 ||
                            quadrupleColumnsGenerated >= quadrupleColumnBudget) {
                            continue;
                        }
                        std::vector<std::int32_t> fourthOrder;
                        fourthOrder.reserve(static_cast<std::size_t>(pairLimit));
                        for (std::int32_t fourthIndex = 0; fourthIndex < pairLimit; ++fourthIndex) {
                            const SpotIndex fourthSpotIndex = seeds.at(
                                static_cast<std::size_t>(fourthIndex)).spot;
                            if (fourthSpotIndex != first.spot && fourthSpotIndex != second.spot &&
                                fourthSpotIndex != third.spot) {
                                fourthOrder.push_back(fourthIndex);
                            }
                        }
                        std::sort(
                            fourthOrder.begin(),
                            fourthOrder.end(),
                            [this, &seeds, &thirdSpot](std::int32_t left, std::int32_t right) {
                                const TargetSeed& leftSeed = seeds.at(static_cast<std::size_t>(left));
                                const TargetSeed& rightSeed = seeds.at(static_cast<std::size_t>(right));
                                const Spot& leftSpot = config_.spots.at(static_cast<std::size_t>(leftSeed.spot));
                                const Spot& rightSpot = config_.spots.at(static_cast<std::size_t>(rightSeed.spot));
                                const std::int32_t leftDistance = terminalDistancesToSpots_.at(
                                    static_cast<std::size_t>(leftSeed.spot)).at(
                                        static_cast<std::size_t>(thirdSpot.position));
                                const std::int32_t rightDistance = terminalDistancesToSpots_.at(
                                    static_cast<std::size_t>(rightSeed.spot)).at(
                                        static_cast<std::size_t>(thirdSpot.position));
                                const std::int64_t leftValue =
                                    static_cast<std::int64_t>(leftSeed.priority) + leftSpot.stock * 100LL - leftDistance;
                                const std::int64_t rightValue =
                                    static_cast<std::int64_t>(rightSeed.priority) + rightSpot.stock * 100LL - rightDistance;
                                if (leftValue != rightValue) {
                                    return leftValue > rightValue;
                                }
                                return leftSeed.spot < rightSeed.spot;
                            });
                        const std::int32_t fourthAttemptLimit = std::min<std::int32_t>(
                            2,
                            static_cast<std::int32_t>(fourthOrder.size()));
                        for (std::int32_t attempt = 0; attempt < fourthAttemptLimit; ++attempt) {
                            if (deadline_expired()) {
                                break;
                            }
                            const TargetSeed& fourth = seeds.at(static_cast<std::size_t>(
                                fourthOrder.at(static_cast<std::size_t>(attempt))));
                            const Spot& fourthSpot =
                                config_.spots.at(static_cast<std::size_t>(fourth.spot));
                            const std::optional<ParetoPath> fourthPathResult = spot_transition(
                                third.spot,
                                fourth.spot,
                                config_.steps_for_day(state.dayNumber) - triple.travelSteps,
                                agent.fuel - triple.patrolFuel);
                            if (!fourthPathResult.has_value()) {
                                continue;
                            }
                            ParetoPath quadruple = triple;
                            const ParetoPath& fourthPath = *fourthPathResult;
                            quadruple.directions.insert(
                                quadruple.directions.end(),
                                fourthPath.directions.begin(),
                                fourthPath.directions.end());
                            quadruple.travelSteps += fourthPath.travelSteps;
                            quadruple.patrolFuel += fourthPath.patrolFuel;
                            for (const auto& [road, stays] : fourthPath.heuristicFootprint.entries) {
                                quadruple.heuristicFootprint.add(road, stays);
                            }
                            RouteColumn quadrupleColumn;
                            quadrupleColumn.columnId = nextColumnId++;
                            quadrupleColumn.agent = agentIndex;
                            quadrupleColumn.actions = complete_actions(config_, state, quadruple);
                            quadrupleColumn.terminalCell = fourthSpot.position;
                            quadrupleColumn.terminalFuel = agent.fuel - quadruple.patrolFuel;
                            quadrupleColumn.estimatedBrands = tripleBrands | brand_bit(fourthSpot.brandIndex);
                            quadrupleColumn.estimatedServings = 4;
                            quadrupleColumn.heuristicFootprint = std::move(quadruple.heuristicFootprint);
                            quadrupleColumn.priority =
                                first.priority + second.priority + third.priority + fourth.priority;
                            portfolio.columnsByAgent.at(
                                static_cast<std::size_t>(agentIndex)).push_back(
                                    std::move(quadrupleColumn));
                            ++quadrupleColumnsGenerated;
                            break;
                        }
                    }
                }
            }
        }
    }

    std::int32_t createdEscorts = 0;
    for (AgentIndex tankerIndex = 0;
         tankerIndex < config_.agent_count() && createdEscorts < options.maximumEscorts &&
         !deadline_expired();
         ++tankerIndex) {
        const AgentState& tanker = state.agents.at(static_cast<std::size_t>(tankerIndex));
        if (tanker.kind != AgentKind::Tanker) {
            continue;
        }
        std::vector<AgentIndex> coLocatedPatrols;
        for (AgentIndex patrolIndex = 0; patrolIndex < config_.agent_count(); ++patrolIndex) {
            const AgentState& patrol = state.agents.at(static_cast<std::size_t>(patrolIndex));
            if (patrol.kind == AgentKind::Patrol && patrol.position == tanker.position) {
                coLocatedPatrols.push_back(patrolIndex);
            }
        }
        if (coLocatedPatrols.size() < 2U) {
            continue;
        }
        ParetoSearchOptions escortOptions;
        escortOptions.maximumTravelSteps = config_.steps_for_day(state.dayNumber);
        escortOptions.maximumPatrolFuel = std::numeric_limits<std::int32_t>::max() / 4;
        escortOptions.maximumLabelsPerCell = 32;
        escortOptions.maximumPaths = 1;
        escortOptions.patrol = true;
        escortOptions.criticalRoads = criticalRoads;
        escortOptions.deadline = options.deadline;
        for (const Spot& spot : config_.spots) {
            if (createdEscorts >= options.maximumEscorts || deadline_expired()) {
                break;
            }
            const std::vector<ParetoPath> paths = router_.find_paths(
                tanker.position,
                spot.position,
                state.roadStatuses,
                escortOptions);
            if (paths.empty() || paths.front().directions.empty() ||
                !std::all_of(
                    coLocatedPatrols.begin(),
                    coLocatedPatrols.end(),
                    [this, &state, &paths](AgentIndex patrolIndex) {
                        return escort_feasible(config_, state, patrolIndex, paths.front());
                    })) {
                continue;
            }
            const ParetoPath& path = paths.front();
            const std::int32_t escortGroup = nextEscortGroup++;
            const std::int32_t priority =
                950000 + spot.stock * 10 - path.travelSteps;
            for (const AgentIndex patrolIndex : coLocatedPatrols) {
                RouteColumn patrolColumn;
                patrolColumn.columnId = nextColumnId++;
                patrolColumn.agent = patrolIndex;
                patrolColumn.actions = complete_actions(config_, state, path);
                patrolColumn.terminalCell = spot.position;
                patrolColumn.terminalFuel = config_.fuelLimit;
                patrolColumn.estimatedBrands = brand_bit(spot.brandIndex);
                patrolColumn.estimatedServings = 1;
                patrolColumn.heuristicFootprint = path.heuristicFootprint;
                patrolColumn.escortGroup = escortGroup;
                patrolColumn.lockstepEscort = true;
                patrolColumn.priority = priority;
                portfolio.columnsByAgent.at(
                    static_cast<std::size_t>(patrolIndex)).push_back(
                        std::move(patrolColumn));
            }
            RouteColumn tankerColumn;
            tankerColumn.columnId = nextColumnId++;
            tankerColumn.agent = tankerIndex;
            tankerColumn.actions = complete_actions(config_, state, path);
            tankerColumn.terminalCell = spot.position;
            tankerColumn.terminalFuel = tanker.fuel;
            tankerColumn.heuristicFootprint = path.heuristicFootprint;
            tankerColumn.escortGroup = escortGroup;
            tankerColumn.lockstepEscort = true;
            tankerColumn.priority = priority;
            portfolio.columnsByAgent.at(
                static_cast<std::size_t>(tankerIndex)).push_back(
                    std::move(tankerColumn));
            ++createdEscorts;
        }
    }
    for (AgentIndex patrolIndex = 0;
         patrolIndex < config_.agent_count() && !deadline_expired();
         ++patrolIndex) {
        if (state.agents.at(static_cast<std::size_t>(patrolIndex)).kind != AgentKind::Patrol) {
            continue;
        }
        for (AgentIndex tankerIndex = 0; tankerIndex < config_.agent_count(); ++tankerIndex) {
            if (deadline_expired()) {
                break;
            }
            if (state.agents.at(static_cast<std::size_t>(tankerIndex)).kind != AgentKind::Tanker ||
                state.agents.at(static_cast<std::size_t>(tankerIndex)).position !=
                    state.agents.at(static_cast<std::size_t>(patrolIndex)).position) {
                continue;
            }
            ParetoSearchOptions escortOptions;
            escortOptions.maximumTravelSteps = config_.steps_for_day(state.dayNumber);
            escortOptions.maximumPatrolFuel = std::numeric_limits<std::int32_t>::max() / 4;
            escortOptions.maximumLabelsPerCell = 32;
            escortOptions.maximumPaths = 1;
            escortOptions.patrol = true;
            escortOptions.criticalRoads = criticalRoads;
            escortOptions.deadline = options.deadline;
            for (const Spot& spot : config_.spots) {
                if (createdEscorts >= options.maximumEscorts || deadline_expired()) {
                    break;
                }
                const std::vector<ParetoPath> paths = router_.find_paths(
                    state.agents.at(static_cast<std::size_t>(patrolIndex)).position,
                    spot.position,
                    state.roadStatuses,
                    escortOptions);
                if (paths.empty() || paths.front().directions.empty() ||
                    !escort_feasible(config_, state, patrolIndex, paths.front())) {
                    continue;
                }
                const std::int32_t escortGroup = nextEscortGroup++;
                const ParetoPath& path = paths.front();
                RouteColumn patrolColumn;
                patrolColumn.columnId = nextColumnId++;
                patrolColumn.agent = patrolIndex;
                patrolColumn.actions = complete_actions(config_, state, path);
                patrolColumn.terminalCell = spot.position;
                patrolColumn.terminalFuel = config_.fuelLimit;
                patrolColumn.estimatedBrands = brand_bit(spot.brandIndex);
                patrolColumn.estimatedServings = 1;
                patrolColumn.heuristicFootprint = path.heuristicFootprint;
                patrolColumn.escortGroup = escortGroup;
                patrolColumn.lockstepEscort = true;
                patrolColumn.priority = 900000 + spot.stock * 10 - path.travelSteps;

                RouteColumn tankerColumn;
                tankerColumn.columnId = nextColumnId++;
                tankerColumn.agent = tankerIndex;
                tankerColumn.actions = complete_actions(config_, state, path);
                tankerColumn.terminalCell = spot.position;
                tankerColumn.terminalFuel = state.agents.at(static_cast<std::size_t>(tankerIndex)).fuel;
                tankerColumn.heuristicFootprint = path.heuristicFootprint;
                tankerColumn.escortGroup = escortGroup;
                tankerColumn.lockstepEscort = true;
                tankerColumn.priority = patrolColumn.priority;

                portfolio.columnsByAgent.at(static_cast<std::size_t>(patrolIndex)).push_back(std::move(patrolColumn));
                portfolio.columnsByAgent.at(static_cast<std::size_t>(tankerIndex)).push_back(std::move(tankerColumn));
                ++createdEscorts;
            }
        }
    }

    std::vector<SpotIndex> rendezvousTargets;
    rendezvousTargets.reserve(config_.spots.size());
    for (SpotIndex spotIndex = 0; spotIndex < static_cast<SpotIndex>(config_.spots.size()); ++spotIndex) {
        rendezvousTargets.push_back(spotIndex);
    }
    std::sort(
        rendezvousTargets.begin(),
        rendezvousTargets.end(),
        [this, &ledger, &options, &state](SpotIndex left, SpotIndex right) {
            const Spot& leftSpot = config_.spots.at(static_cast<std::size_t>(left));
            const Spot& rightSpot = config_.spots.at(static_cast<std::size_t>(right));
            const std::int32_t leftPriority =
                reservation_priority(options.mandatoryReservations, state, left) +
                (!has_brand(ledger.lifetimeBrands, leftSpot.brandIndex) ? 1000000 : 0) +
                (config_.brand_count() - brand_rarity(config_, leftSpot.brandIndex)) * 1000 + leftSpot.stock * 10;
            const std::int32_t rightPriority =
                reservation_priority(options.mandatoryReservations, state, right) +
                (!has_brand(ledger.lifetimeBrands, rightSpot.brandIndex) ? 1000000 : 0) +
                (config_.brand_count() - brand_rarity(config_, rightSpot.brandIndex)) * 1000 + rightSpot.stock * 10;
            if (leftPriority != rightPriority) {
                return leftPriority > rightPriority;
            }
            return left < right;
        });
    std::vector<CellId> rendezvousCells = criticalRoads;
    rendezvousCells.reserve(criticalRoads.size() + state.agents.size() + 8U);
    for (const AgentState& agent : state.agents) {
        rendezvousCells.push_back(agent.position);
    }
    const std::int32_t rendezvousSpotLimit = std::min<std::int32_t>(
        8,
        static_cast<std::int32_t>(rendezvousTargets.size()));
    for (std::int32_t targetOffset = 0; targetOffset < rendezvousSpotLimit; ++targetOffset) {
        rendezvousCells.push_back(config_.spots.at(static_cast<std::size_t>(
            rendezvousTargets.at(static_cast<std::size_t>(targetOffset)))).position);
    }
    std::sort(rendezvousCells.begin(), rendezvousCells.end());
    rendezvousCells.erase(std::unique(rendezvousCells.begin(), rendezvousCells.end()), rendezvousCells.end());
    if (rendezvousCells.size() > 16U) {
        rendezvousCells.resize(16U);
    }
    const std::int32_t rendezvousLimit = std::max(1, options.maximumEscorts / 2);
    std::int32_t createdRendezvous = 0;
    for (AgentIndex patrolIndex = 0;
         patrolIndex < config_.agent_count() && createdRendezvous < rendezvousLimit && !deadline_expired();
         ++patrolIndex) {
        const AgentState& patrol = state.agents.at(static_cast<std::size_t>(patrolIndex));
        if (patrol.kind != AgentKind::Patrol) {
            continue;
        }
        for (AgentIndex tankerIndex = 0;
             tankerIndex < config_.agent_count() && createdRendezvous < rendezvousLimit && !deadline_expired();
             ++tankerIndex) {
            const AgentState& tanker = state.agents.at(static_cast<std::size_t>(tankerIndex));
            if (tanker.kind != AgentKind::Tanker) {
                continue;
            }
            ParetoSearchOptions patrolArrivalOptions;
            patrolArrivalOptions.maximumTravelSteps = config_.steps_for_day(state.dayNumber);
            patrolArrivalOptions.maximumPatrolFuel = patrol.fuel;
            patrolArrivalOptions.maximumLabelsPerCell = 32;
            patrolArrivalOptions.maximumPaths = 1;
            patrolArrivalOptions.patrol = true;
            patrolArrivalOptions.criticalRoads = criticalRoads;
            patrolArrivalOptions.deadline = options.deadline;
            ParetoSearchOptions tankerArrivalOptions = patrolArrivalOptions;
            tankerArrivalOptions.maximumPatrolFuel = 0;
            tankerArrivalOptions.patrol = false;
            for (const CellId rendezvous : rendezvousCells) {
                if (createdRendezvous >= rendezvousLimit || deadline_expired()) {
                    break;
                }
                const std::vector<ParetoPath> patrolArrivals = router_.find_paths(
                    patrol.position,
                    rendezvous,
                    state.roadStatuses,
                    patrolArrivalOptions);
                const std::vector<ParetoPath> tankerArrivals = router_.find_paths(
                    tanker.position,
                    rendezvous,
                    state.roadStatuses,
                    tankerArrivalOptions);
                if (patrolArrivals.empty() || tankerArrivals.empty()) {
                    continue;
                }
                const ParetoPath& patrolArrival = patrolArrivals.front();
                const ParetoPath& tankerArrival = tankerArrivals.front();
                const std::int32_t rendezvousStep = std::max(
                    patrolArrival.travelSteps,
                    tankerArrival.travelSteps);
                if (rendezvousStep <= 0 || rendezvousStep >= config_.steps_for_day(state.dayNumber)) {
                    continue;
                }
                const std::int32_t refuelStep = rendezvousStep + 1;
                if (refuelStep >= config_.steps_for_day(state.dayNumber)) {
                    continue;
                }
                ParetoSearchOptions departureOptions;
                departureOptions.maximumTravelSteps = config_.steps_for_day(state.dayNumber) - refuelStep;
                departureOptions.maximumPatrolFuel = config_.fuelLimit;
                departureOptions.maximumLabelsPerCell = 32;
                departureOptions.maximumPaths = 1;
                departureOptions.patrol = true;
                departureOptions.criticalRoads = criticalRoads;
                departureOptions.deadline = options.deadline;
                const std::int32_t targetLimit = std::min<std::int32_t>(
                    6,
                    static_cast<std::int32_t>(rendezvousTargets.size()));
                for (std::int32_t targetOffset = 0;
                     targetOffset < targetLimit && createdRendezvous < rendezvousLimit && !deadline_expired();
                     ++targetOffset) {
                    const SpotIndex spotIndex = rendezvousTargets.at(static_cast<std::size_t>(targetOffset));
                    const Spot& spot = config_.spots.at(static_cast<std::size_t>(spotIndex));
                    if (spot.position == rendezvous) {
                        continue;
                    }
                    const std::vector<ParetoPath> departures = router_.find_paths(
                        rendezvous,
                        spot.position,
                        state.roadStatuses,
                        departureOptions);
                    if (departures.empty()) {
                        continue;
                    }
                    const ParetoPath& departure = departures.front();
                    const std::int32_t patrolInitialWait = rendezvousStep - patrolArrival.travelSteps;
                    const std::int32_t tankerInitialWait = rendezvousStep - tankerArrival.travelSteps;
                    ParetoPath sharedDeparture;
                    CellId sharedTerminal = rendezvous;
                    std::int32_t sharedStep = refuelStep;
                    const std::int32_t sharedDirectionCount = std::min<std::int32_t>(
                        2,
                        static_cast<std::int32_t>(departure.directions.size()));
                    for (std::int32_t directionIndex = 0;
                         directionIndex < sharedDirectionCount;
                         ++directionIndex) {
                        const std::int32_t direction = departure.directions.at(
                            static_cast<std::size_t>(directionIndex));
                        const MoveCost cost = config_.move_cost(
                            sharedTerminal,
                            state.roadStatuses.at(static_cast<std::size_t>(sharedTerminal)));
                        const CellId next = config_.map.neighbors.at(
                            static_cast<std::size_t>(sharedTerminal)).at(
                                static_cast<std::size_t>(direction));
                        if (next == kInvalidCell ||
                            sharedStep + cost.steps > config_.steps_for_day(state.dayNumber)) {
                            break;
                        }
                        sharedDeparture.directions.push_back(direction);
                        sharedDeparture.travelSteps += cost.steps;
                        sharedDeparture.patrolFuel += cost.patrolFuel;
                        if (config_.map.terrain.at(static_cast<std::size_t>(sharedTerminal)) == Terrain::Road) {
                            sharedDeparture.heuristicFootprint.add(sharedTerminal, cost.steps);
                        }
                        sharedTerminal = next;
                        sharedStep += cost.steps;
                    }
                    const std::int32_t group = nextEscortGroup++;
                    RouteColumn patrolColumn;
                    patrolColumn.columnId = nextColumnId++;
                    patrolColumn.agent = patrolIndex;
                    patrolColumn.actions = rendezvous_actions(
                        config_,
                        state,
                        patrolInitialWait,
                        patrolArrival,
                        departure,
                        1);
                    patrolColumn.terminalCell = spot.position;
                    patrolColumn.terminalFuel = config_.fuelLimit - departure.patrolFuel;
                    patrolColumn.estimatedBrands = brand_bit(spot.brandIndex);
                    patrolColumn.estimatedServings = 1;
                    patrolColumn.heuristicFootprint = patrolArrival.heuristicFootprint;
                    for (const auto& [road, stays] : departure.heuristicFootprint.entries) {
                        patrolColumn.heuristicFootprint.add(road, stays);
                    }
                    patrolColumn.requiredRefuels.push_back(RefuelEvent{rendezvous, refuelStep});
                    patrolColumn.escortGroup = group;
                    patrolColumn.priority = reservation_priority(options.mandatoryReservations, state, spotIndex) +
                        (!has_brand(ledger.lifetimeBrands, spot.brandIndex) ? 1500000 : 0) +
                        spot.stock * 10 + sharedDeparture.travelSteps * 10 -
                        rendezvousStep - departure.travelSteps;

                    RouteColumn tankerColumn;
                    tankerColumn.columnId = nextColumnId++;
                    tankerColumn.agent = tankerIndex;
                    tankerColumn.actions = rendezvous_actions(
                        config_,
                        state,
                        tankerInitialWait,
                        tankerArrival,
                        sharedDeparture,
                        1);
                    tankerColumn.terminalCell = sharedTerminal;
                    tankerColumn.terminalFuel = tanker.fuel;
                    tankerColumn.heuristicFootprint = tankerArrival.heuristicFootprint;
                    for (const auto& [road, stays] : sharedDeparture.heuristicFootprint.entries) {
                        tankerColumn.heuristicFootprint.add(road, stays);
                    }
                    tankerColumn.escortGroup = group;
                    tankerColumn.priority = patrolColumn.priority;

                    portfolio.columnsByAgent.at(static_cast<std::size_t>(patrolIndex)).push_back(std::move(patrolColumn));
                    portfolio.columnsByAgent.at(static_cast<std::size_t>(tankerIndex)).push_back(std::move(tankerColumn));
                    ++createdRendezvous;
                }
            }
        }
    }

    std::vector<SpotIndex> dockingTargets;
    dockingTargets.reserve(config_.spots.size());
    for (SpotIndex spotIndex = 0; spotIndex < static_cast<SpotIndex>(config_.spots.size()); ++spotIndex) {
        dockingTargets.push_back(spotIndex);
    }
    std::sort(
        dockingTargets.begin(),
        dockingTargets.end(),
        [this, &ledger](SpotIndex left, SpotIndex right) {
            const Spot& leftSpot = config_.spots.at(static_cast<std::size_t>(left));
            const Spot& rightSpot = config_.spots.at(static_cast<std::size_t>(right));
            const std::int32_t leftPriority =
                (!has_brand(ledger.lifetimeBrands, leftSpot.brandIndex) ? 1000000 : 0) +
                (config_.brand_count() - brand_rarity(config_, leftSpot.brandIndex)) * 1000 + leftSpot.stock * 10;
            const std::int32_t rightPriority =
                (!has_brand(ledger.lifetimeBrands, rightSpot.brandIndex) ? 1000000 : 0) +
                (config_.brand_count() - brand_rarity(config_, rightSpot.brandIndex)) * 1000 + rightSpot.stock * 10;
            if (leftPriority != rightPriority) {
                return leftPriority > rightPriority;
            }
            return left < right;
        });
    const std::int32_t dockingTargetLimit = std::min<std::int32_t>(4, static_cast<std::int32_t>(dockingTargets.size()));
    const std::int32_t dockingLimit = std::max(1, options.maximumEscorts / 2);
    std::int32_t createdDocks = 0;
    for (AgentIndex patrolIndex = 0;
         patrolIndex < config_.agent_count() && createdDocks < dockingLimit && !deadline_expired();
         ++patrolIndex) {
        const AgentState& patrol = state.agents.at(static_cast<std::size_t>(patrolIndex));
        if (patrol.kind != AgentKind::Patrol) {
            continue;
        }
        for (AgentIndex tankerIndex = 0;
             tankerIndex < config_.agent_count() && createdDocks < dockingLimit && !deadline_expired();
             ++tankerIndex) {
            const AgentState& tanker = state.agents.at(static_cast<std::size_t>(tankerIndex));
            if (tanker.kind != AgentKind::Tanker || tanker.position == patrol.position) {
                continue;
            }
            ParetoSearchOptions patrolOptions;
            patrolOptions.maximumTravelSteps = config_.steps_for_day(state.dayNumber);
            patrolOptions.maximumPatrolFuel = patrol.fuel;
            patrolOptions.maximumLabelsPerCell = 32;
            patrolOptions.maximumPaths = 1;
            patrolOptions.patrol = true;
            patrolOptions.criticalRoads = criticalRoads;
            patrolOptions.deadline = options.deadline;
            ParetoSearchOptions tankerOptions = patrolOptions;
            tankerOptions.maximumPatrolFuel = 0;
            tankerOptions.patrol = false;
            for (std::int32_t targetOffset = 0;
                 targetOffset < dockingTargetLimit && createdDocks < dockingLimit && !deadline_expired();
                 ++targetOffset) {
                const SpotIndex spotIndex = dockingTargets.at(static_cast<std::size_t>(targetOffset));
                const Spot& spot = config_.spots.at(static_cast<std::size_t>(spotIndex));
                const std::vector<ParetoPath> patrolPaths = router_.find_paths(
                    patrol.position,
                    spot.position,
                    state.roadStatuses,
                    patrolOptions);
                if (patrolPaths.empty()) {
                    continue;
                }
                const std::vector<ParetoPath> tankerPaths = router_.find_paths(
                    tanker.position,
                    spot.position,
                    state.roadStatuses,
                    tankerOptions);
                if (tankerPaths.empty()) {
                    continue;
                }
                const ParetoPath& patrolPath = patrolPaths.front();
                const ParetoPath& tankerPath = tankerPaths.front();
                const std::int32_t group = nextEscortGroup++;
                const std::int32_t priority =
                    (!has_brand(ledger.lifetimeBrands, spot.brandIndex) ? 800000 : 0) + spot.stock * 10 -
                    patrolPath.travelSteps - tankerPath.travelSteps;
                RouteColumn patrolColumn;
                patrolColumn.columnId = nextColumnId++;
                patrolColumn.agent = patrolIndex;
                patrolColumn.actions = complete_actions(config_, state, patrolPath);
                patrolColumn.terminalCell = spot.position;
                patrolColumn.terminalFuel = config_.fuelLimit;
                patrolColumn.estimatedBrands = brand_bit(spot.brandIndex);
                patrolColumn.estimatedServings = 1;
                patrolColumn.heuristicFootprint = patrolPath.heuristicFootprint;
                patrolColumn.escortGroup = group;
                patrolColumn.priority = priority;

                RouteColumn tankerColumn;
                tankerColumn.columnId = nextColumnId++;
                tankerColumn.agent = tankerIndex;
                tankerColumn.actions = complete_actions(config_, state, tankerPath);
                tankerColumn.terminalCell = spot.position;
                tankerColumn.terminalFuel = tanker.fuel;
                tankerColumn.heuristicFootprint = tankerPath.heuristicFootprint;
                tankerColumn.escortGroup = group;
                tankerColumn.priority = priority;

                portfolio.columnsByAgent.at(static_cast<std::size_t>(patrolIndex)).push_back(std::move(patrolColumn));
                portfolio.columnsByAgent.at(static_cast<std::size_t>(tankerIndex)).push_back(std::move(tankerColumn));
                ++createdDocks;
            }
        }
    }

    for (AgentIndex agentIndex = 0; agentIndex < config_.agent_count(); ++agentIndex) {
        std::vector<RouteColumn>& columns = portfolio.columnsByAgent.at(
            static_cast<std::size_t>(agentIndex));
        const AgentState& agent = state.agents.at(static_cast<std::size_t>(agentIndex));
        const SpotIndex startSpot = config_.spotAtCell.at(static_cast<std::size_t>(agent.position));
        if (agent.kind == AgentKind::Patrol && startSpot != kInvalidSpot) {
            const std::size_t originalCount = columns.size();
            const Spot& spot = config_.spots.at(static_cast<std::size_t>(startSpot));
            const std::int32_t harvestPriority =
                (!has_brand(ledger.lifetimeBrands, spot.brandIndex) ? 1000000 : 0) +
                (config_.brand_count() - brand_rarity(config_, spot.brandIndex)) * 1000 +
                spot.stock * 10;
            for (std::size_t columnIndex = 0; columnIndex < originalCount; ++columnIndex) {
                const RouteColumn& source = columns.at(columnIndex);
                if (source.escortGroup >= 0 || source.contingencyBundle >= 0 ||
                    !source.requiredRefuels.empty()) {
                    continue;
                }
                const std::optional<AgentPlan> actions = prepend_start_harvest_wait(source.actions);
                if (!actions.has_value()) {
                    continue;
                }
                RouteColumn harvested = source;
                harvested.columnId = nextColumnId++;
                harvested.actions = *actions;
                harvested.priority += harvestPriority;
                columns.push_back(std::move(harvested));
            }
        }
        const auto enrich_column = [this, &state, &ledger](RouteColumn& column) {
            populate_first_visits(config_, state, column);
            column.estimatedBrands = 0;
            column.estimatedServings = 0;
            if (state.agents.at(static_cast<std::size_t>(column.agent)).kind == AgentKind::Patrol) {
                for (const ColumnVisitEvent& event : column.firstVisits) {
                    column.estimatedBrands |= brand_bit(event.brandIndex);
                    if (event.claimedServing) {
                        ++column.estimatedServings;
                    }
                }
            }
            populate_terminal_brand_feature(
                config_,
                ledger,
                terminalDistancesToSpots_,
                column);
        };
        for (RouteColumn& column : columns) {
            enrich_column(column);
        }
        if (agent.kind == AgentKind::Patrol && options.maximumColumnsPerAgent == 12 &&
            !deadline_expired()) {
            std::vector<std::size_t> sourceOrder;
            sourceOrder.reserve(columns.size());
            for (std::size_t columnIndex = 0; columnIndex < columns.size(); ++columnIndex) {
                const RouteColumn& source = columns.at(columnIndex);
                if (source.estimatedServings >= 3 && source.escortGroup < 0 &&
                    source.contingencyBundle < 0 && source.requiredRefuels.empty() &&
                    !source.actions.empty() && source.actions.back().kind == ActionKind::Wait &&
                    source.actions.back().value > 1 &&
                    config_.spotAtCell.at(static_cast<std::size_t>(source.terminalCell)) != kInvalidSpot) {
                    sourceOrder.push_back(columnIndex);
                }
            }
            std::sort(
                sourceOrder.begin(),
                sourceOrder.end(),
                [&columns](std::size_t left, std::size_t right) {
                    const RouteColumn& leftColumn = columns.at(left);
                    const RouteColumn& rightColumn = columns.at(right);
                    if (leftColumn.estimatedServings != rightColumn.estimatedServings) {
                        return leftColumn.estimatedServings > rightColumn.estimatedServings;
                    }
                    if (leftColumn.priority != rightColumn.priority) {
                        return leftColumn.priority > rightColumn.priority;
                    }
                    return leftColumn.columnId < rightColumn.columnId;
                });
            std::vector<RouteColumn> harvestExtensions;
            bool extendedRoute = false;
            for (const std::size_t sourceIndex : sourceOrder) {
                if (deadline_expired() || extendedRoute) {
                    break;
                }
                const RouteColumn& source = columns.at(sourceIndex);
                RouteColumn current = source;
                std::set<SpotIndex> visitedSpots;
                for (const ColumnVisitEvent& visit : source.firstVisits) {
                    visitedSpots.insert(visit.spot);
                }
                for (std::int32_t extensionDepth = 0; extensionDepth < 2; ++extensionDepth) {
                    if (deadline_expired() || current.actions.empty() ||
                        current.actions.back().kind != ActionKind::Wait ||
                        current.actions.back().value <= 1) {
                        break;
                    }
                    const SpotIndex terminalSpot = config_.spotAtCell.at(
                        static_cast<std::size_t>(current.terminalCell));
                    const std::int32_t trailingWait = current.actions.back().value;
                    bool extendedAtDepth = false;
                    for (const SpotIndex targetSpot : dockingTargets) {
                        if (deadline_expired() || visitedSpots.contains(targetSpot) ||
                            targetSpot == terminalSpot ||
                            !spotTransitionReady.at(static_cast<std::size_t>(terminalSpot)).at(
                                static_cast<std::size_t>(targetSpot))) {
                            continue;
                        }
                        const std::optional<ParetoPath> extension = spot_transition(
                            terminalSpot,
                            targetSpot,
                            trailingWait - 1,
                            current.terminalFuel);
                        if (!extension.has_value() || extension->travelSteps >= trailingWait) {
                            continue;
                        }
                        RouteColumn extended = current;
                        extended.columnId = nextColumnId++;
                        extended.actions.pop_back();
                        for (const std::int32_t direction : extension->directions) {
                            extended.actions.push_back(PlanAction::move(direction));
                        }
                        const std::int32_t finalWait = trailingWait - extension->travelSteps;
                        if (finalWait > 0) {
                            extended.actions.push_back(PlanAction::wait(finalWait));
                        }
                        const Spot& target = config_.spots.at(static_cast<std::size_t>(targetSpot));
                        extended.terminalCell = target.position;
                        extended.terminalFuel = current.terminalFuel - extension->patrolFuel;
                        for (const auto& [road, stays] : extension->heuristicFootprint.entries) {
                            extended.heuristicFootprint.add(road, stays);
                        }
                        extended.priority +=
                            (!has_brand(ledger.lifetimeBrands, target.brandIndex) ? 1000000 : 0) +
                            (config_.brand_count() - brand_rarity(config_, target.brandIndex)) * 1000 +
                            target.stock * 10 - extension->travelSteps;
                        enrich_column(extended);
                        if (extended.estimatedServings <= current.estimatedServings) {
                            continue;
                        }
                        visitedSpots.insert(targetSpot);
                        current = extended;
                        harvestExtensions.push_back(std::move(extended));
                        extendedRoute = true;
                        extendedAtDepth = true;
                        break;
                    }
                    if (!extendedAtDepth) {
                        break;
                    }
                }
            }
            columns.insert(
                columns.end(),
                std::make_move_iterator(harvestExtensions.begin()),
                std::make_move_iterator(harvestExtensions.end()));
        }
        prune_columns(columns, std::max(1, options.maximumColumnsPerAgent));
    }
    populate_exact_escort_segments(state, portfolio);
    return portfolio;
}

RoutePoolAugmentation RouteColumnGenerator::augment_with_candidate_routes(
    const DayState& state,
    RoutePortfolio portfolio,
    const std::vector<MasterCandidate>& candidates,
    std::int32_t maximumColumnsPerAgent) const {
    RoutePoolAugmentation augmentation;
    augmentation.portfolio = std::move(portfolio);
    if (augmentation.portfolio.columnsByAgent.size() != static_cast<std::size_t>(config_.agent_count()) ||
        state.agents.size() != static_cast<std::size_t>(config_.agent_count()) ||
        maximumColumnsPerAgent <= 0) {
        return augmentation;
    }

    std::int32_t nextColumnId = 0;
    for (const std::vector<RouteColumn>& columns : augmentation.portfolio.columnsByAgent) {
        for (const RouteColumn& column : columns) {
            nextColumnId = std::max(nextColumnId, column.columnId + 1);
        }
    }

    std::set<std::int32_t> novelColumnIds;
    for (std::size_t candidateIndex = 0; candidateIndex < candidates.size(); ++candidateIndex) {
        const MasterCandidate& candidate = candidates.at(candidateIndex);
        if (candidate.plan.actions.size() != static_cast<std::size_t>(config_.agent_count()) ||
            candidate.simulation.finalAgents.size() != static_cast<std::size_t>(config_.agent_count())) {
            continue;
        }
        for (AgentIndex agentIndex = 0; agentIndex < config_.agent_count(); ++agentIndex) {
            ++augmentation.routesConsidered;
            std::vector<RouteColumn>& columns =
                augmentation.portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex));
            const AgentPlan& actions = candidate.plan.actions.at(static_cast<std::size_t>(agentIndex));
            const bool alreadyPresent = std::any_of(
                columns.begin(),
                columns.end(),
                [&actions](const RouteColumn& column) { return same_agent_plan(column.actions, actions); });
            if (alreadyPresent) {
                continue;
            }

            RouteColumn column;
            column.columnId = nextColumnId++;
            column.agent = agentIndex;
            column.actions = actions;
            const AgentState& terminal =
                candidate.simulation.finalAgents.at(static_cast<std::size_t>(agentIndex));
            column.terminalCell = terminal.position;
            column.terminalFuel = terminal.fuel;
            const std::int32_t rankPenalty = static_cast<std::int32_t>(std::min<std::size_t>(
                candidateIndex,
                static_cast<std::size_t>(1000)));
            column.priority = 1650000 - rankPenalty * 1000;
            populate_first_visits(config_, state, column);
            if (!column.hasExactTimeline) {
                continue;
            }
            for (const ColumnVisitEvent& event : column.firstVisits) {
                column.estimatedBrands |= brand_bit(
                    config_.spots.at(static_cast<std::size_t>(event.spot)).brandIndex);
                if (event.claimedServing) {
                    ++column.estimatedServings;
                }
            }
            column.heuristicFootprint = column.fullFootprint;
            novelColumnIds.insert(column.columnId);
            columns.push_back(std::move(column));
            ++augmentation.novelRoutes;
        }
    }

    for (std::vector<RouteColumn>& columns : augmentation.portfolio.columnsByAgent) {
        prune_columns(columns, maximumColumnsPerAgent);
        augmentation.retainedNovelRoutes += static_cast<std::int32_t>(std::count_if(
            columns.begin(),
            columns.end(),
            [&novelColumnIds](const RouteColumn& column) {
                return novelColumnIds.contains(column.columnId);
            }));
    }
    return augmentation;
}

RouteMaster::RouteMaster(
    const MatchConfig& config,
    const ExactStepSimulator& simulator,
    const IndependentDayValidator& validator)
    : config_(config),
      simulator_(simulator),
      validator_(validator),
      terminalDistancesToSpots_(build_terminal_distance_cache(config)) {}

std::optional<MasterCandidate> RouteMaster::evaluate_exact_plan(
    const DayState& state,
    const MatchLedger& ledger,
    const DayPlan& plan,
    const std::vector<MandatoryReservation>& mandatoryReservations) const {
    const SimulationResult simulation = simulator_.simulate(state, plan, true);
    if (!simulation.valid || !simulation_covers_proven_reservations(simulation, state, mandatoryReservations)) {
        return std::nullopt;
    }
    const SimulationResult validation = validator_.validate(state, plan, true);
    std::string mismatch;
    if (!validator_.agrees_with(simulation, validation, mismatch)) {
        throw std::runtime_error("independent validation disagreement: " + mismatch);
    }
    MasterCandidate candidate;
    candidate.plan = plan;
    candidate.simulation = simulation;
    candidate.scoreAfterToday = OfficialScore::after_day(ledger, simulation.score);
    candidate.terminalSlack = calculate_terminal_slack(config_, terminalDistancesToSpots_, simulation, ledger);
    candidate.trafficSafety = calculate_traffic_safety(config_, simulation);
    candidate.stableId = canonical_plan_bytes(plan);
    candidate.creditedServings = simulation.score.servings;
    return candidate;
}

std::vector<MasterCandidate> RouteMaster::solve(
    const DayState& state,
    const MatchLedger& ledger,
    const RoutePortfolio& portfolio,
    const MasterOptions& options,
    MasterDiagnostics& diagnostics) const {
    diagnostics = MasterDiagnostics{};
    diagnostics.nativeExactStockCredits = options.useStockCredits;
    if (portfolio.columnsByAgent.size() != static_cast<std::size_t>(config_.agent_count()) ||
        options.maximumCombinations <= 0 || options.maximumCandidates <= 0) {
        return {};
    }
    std::vector<MasterCandidate> candidates;
    std::set<std::string> evaluatedPlans;
    StockCutState cutState;
    cutState.capacityCut.assign(config_.spots.size(), false);
    cutState.promoted.assign(config_.spots.size(), false);
    cutState.prefixes.resize(config_.spots.size());
    const std::int32_t maximumResolveRounds = options.maximumResolveRounds > 0
        ? options.maximumResolveRounds
        : std::max(1, std::min(8, 2 * config_.agent_count()));
    const auto deadline_expired = [&]() {
        return options.deadline.has_value() && std::chrono::steady_clock::now() >= *options.deadline;
    };

    for (std::int32_t resolveRound = 0;
         resolveRound < maximumResolveRounds && diagnostics.combinationsVisited < options.maximumCombinations;
         ++resolveRound) {
        if (deadline_expired()) {
            diagnostics.deadlineReached = true;
            break;
        }
        ++diagnostics.cutRounds;
        std::vector<std::vector<const RouteColumn*>> orderedColumns(
            static_cast<std::size_t>(config_.agent_count()));
        std::vector<AgentIndex> ordering;
        ordering.reserve(static_cast<std::size_t>(config_.agent_count()));
        for (AgentIndex agentIndex = 0; agentIndex < config_.agent_count(); ++agentIndex) {
            const std::vector<RouteColumn>& columns = portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex));
            if (columns.empty()) {
                return {};
            }
            std::vector<const RouteColumn*>& ordered = orderedColumns.at(static_cast<std::size_t>(agentIndex));
            ordered.reserve(columns.size());
            for (const RouteColumn& column : columns) {
                ordered.push_back(&column);
            }
            std::sort(
                ordered.begin(),
                ordered.end(),
                [&config = config_, &ledger, &cutState](const RouteColumn* left, const RouteColumn* right) {
                    const std::uint64_t leftBrands = column_brand_mask(config, *left);
                    const std::uint64_t rightBrands = column_brand_mask(config, *right);
                    const std::int32_t leftLifetimeGain = static_cast<std::int32_t>(
                        std::popcount(leftBrands & ~ledger.lifetimeBrands));
                    const std::int32_t rightLifetimeGain = static_cast<std::int32_t>(
                        std::popcount(rightBrands & ~ledger.lifetimeBrands));
                    if (leftLifetimeGain != rightLifetimeGain) {
                        return leftLifetimeGain > rightLifetimeGain;
                    }
                    const std::int32_t leftDailyGain = static_cast<std::int32_t>(std::popcount(leftBrands));
                    const std::int32_t rightDailyGain = static_cast<std::int32_t>(std::popcount(rightBrands));
                    if (leftDailyGain != rightDailyGain) {
                        return leftDailyGain > rightDailyGain;
                    }
                    const std::int32_t leftClaims = static_cast<std::int32_t>(std::count_if(
                        left->firstVisits.begin(),
                        left->firstVisits.end(),
                        [](const ColumnVisitEvent& event) { return event.claimedServing; }));
                    const std::int32_t rightClaims = static_cast<std::int32_t>(std::count_if(
                        right->firstVisits.begin(),
                        right->firstVisits.end(),
                        [](const ColumnVisitEvent& event) { return event.claimedServing; }));
                    if (leftClaims != rightClaims) {
                        return leftClaims > rightClaims;
                    }
                    const std::int32_t leftPriority = conflict_aware_priority(*left, cutState);
                    const std::int32_t rightPriority = conflict_aware_priority(*right, cutState);
                    if (leftPriority != rightPriority) {
                        return leftPriority > rightPriority;
                    }
                    return left->columnId < right->columnId;
                });
            ordering.push_back(agentIndex);
        }
        std::sort(
            ordering.begin(),
            ordering.end(),
            [&orderedColumns](AgentIndex left, AgentIndex right) {
                return orderedColumns.at(static_cast<std::size_t>(left)).size() <
                    orderedColumns.at(static_cast<std::size_t>(right)).size();
            });

        const std::int32_t remainingBudget = options.maximumCombinations - diagnostics.combinationsVisited;
        const std::int32_t roundBudget = resolveRound + 1 == maximumResolveRounds
            ? remainingBudget
            : std::max(1, remainingBudget / 2);
        const std::int32_t roundLimit = diagnostics.combinationsVisited + roundBudget;
        std::vector<const RouteColumn*> selected(static_cast<std::size_t>(config_.agent_count()), nullptr);
        bool learnedCut = false;
        const bool exactMetadata = portfolio_has_exact_metadata(portfolio);
        diagnostics.optimisticUpperBound = exactMetadata
            ? optimistic_partial_score(config_, ledger, selected, ordering, 0U, orderedColumns)
            : OfficialScore{
                  config_.brand_count(),
                  ledger.totalDailyDistinct + config_.brand_count(),
                  ledger.totalServings + std::accumulate(
                      config_.spots.begin(),
                      config_.spots.end(),
                      0,
                      [](std::int32_t total, const Spot& spot) { return total + spot.stock; }),
              };
        if (resolveRound == 0 && exactMetadata && options.maximumCombinations >= 128 &&
            diagnostics.combinationsVisited < roundLimit && !deadline_expired()) {
            struct BeamSelection {
                std::vector<const RouteColumn*> columns;
                std::uint64_t brands = 0;
                std::int32_t claimedServings = 0;
                std::int64_t priority = 0;
            };
            std::vector<BeamSelection> beam;
            beam.push_back(BeamSelection{
                std::vector<const RouteColumn*>(static_cast<std::size_t>(config_.agent_count()), nullptr)});
            const std::int32_t beamWidth = std::clamp(
                options.maximumCandidates * 16,
                128,
                1024);
            bool beamComplete = true;
            for (std::size_t depth = 0; depth < ordering.size(); ++depth) {
                if (deadline_expired()) {
                    diagnostics.deadlineReached = true;
                    beamComplete = false;
                    break;
                }
                const AgentIndex agentIndex = ordering.at(depth);
                std::vector<BeamSelection> expanded;
                expanded.reserve(
                    beam.size() * orderedColumns.at(static_cast<std::size_t>(agentIndex)).size());
                for (const BeamSelection& partial : beam) {
                    for (const RouteColumn* column : orderedColumns.at(static_cast<std::size_t>(agentIndex))) {
                        bool bundleCompatible = true;
                        for (const RouteColumn* assigned : partial.columns) {
                            if (assigned == nullptr || assigned->agent == agentIndex) {
                                continue;
                            }
                            if (assigned->contingencyBundle != column->contingencyBundle &&
                                (assigned->contingencyBundle >= 0 || column->contingencyBundle >= 0)) {
                                bundleCompatible = false;
                                break;
                            }
                        }
                        if (!bundleCompatible) {
                            continue;
                        }
                        BeamSelection candidate = partial;
                        candidate.columns.at(static_cast<std::size_t>(agentIndex)) = column;
                        if (!partial_synchronized_selection_is_feasible(
                                state,
                                candidate.columns,
                                orderedColumns)) {
                            continue;
                        }
                        candidate.brands |= column_brand_mask(config_, *column);
                        candidate.claimedServings += column->estimatedServings;
                        candidate.priority += conflict_aware_priority(*column, cutState);
                        expanded.push_back(std::move(candidate));
                    }
                }
                std::sort(
                    expanded.begin(),
                    expanded.end(),
                    [&ledger](const BeamSelection& left, const BeamSelection& right) {
                        const std::int32_t lifetimeOrder =
                            static_cast<std::int32_t>(std::popcount(ledger.lifetimeBrands | left.brands)) -
                            static_cast<std::int32_t>(std::popcount(ledger.lifetimeBrands | right.brands));
                        if (lifetimeOrder != 0) {
                            return lifetimeOrder > 0;
                        }
                        const std::int32_t dailyOrder =
                            static_cast<std::int32_t>(std::popcount(left.brands)) -
                            static_cast<std::int32_t>(std::popcount(right.brands));
                        if (dailyOrder != 0) {
                            return dailyOrder > 0;
                        }
                        if (left.claimedServings != right.claimedServings) {
                            return left.claimedServings > right.claimedServings;
                        }
                        if (left.priority != right.priority) {
                            return left.priority > right.priority;
                        }
                        for (std::size_t agentOffset = 0; agentOffset < left.columns.size(); ++agentOffset) {
                            const std::int32_t leftId = left.columns.at(agentOffset) == nullptr
                                ? -1
                                : left.columns.at(agentOffset)->columnId;
                            const std::int32_t rightId = right.columns.at(agentOffset) == nullptr
                                ? -1
                                : right.columns.at(agentOffset)->columnId;
                            if (leftId != rightId) {
                                return leftId < rightId;
                            }
                        }
                        return false;
                    });
                if (static_cast<std::int32_t>(expanded.size()) > beamWidth) {
                    expanded.resize(static_cast<std::size_t>(beamWidth));
                }
                beam = std::move(expanded);
                if (beam.empty()) {
                    beamComplete = false;
                    break;
                }
            }
            const std::int32_t evaluationLimit = std::min(
                static_cast<std::int32_t>(beam.size()),
                std::max(32, options.maximumCandidates * 4));
            for (std::int32_t beamIndex = 0;
                 beamComplete && beamIndex < evaluationLimit &&
                 diagnostics.combinationsVisited < roundLimit && !deadline_expired();
                 ++beamIndex) {
                const std::vector<const RouteColumn*>& seedSelection =
                    beam.at(static_cast<std::size_t>(beamIndex)).columns;
                ++diagnostics.combinationsVisited;
                if (!synchronized_selection_is_valid(state, seedSelection)) {
                    ++diagnostics.synchronizationConflicts;
                    continue;
                }
                DayPlan plan;
                plan.actions.resize(static_cast<std::size_t>(config_.agent_count()));
                for (AgentIndex agentIndex = 0; agentIndex < config_.agent_count(); ++agentIndex) {
                    plan.actions.at(static_cast<std::size_t>(agentIndex)) =
                        seedSelection.at(static_cast<std::size_t>(agentIndex))->actions;
                }
                const std::string planId = canonical_plan_bytes(plan);
                if (!evaluatedPlans.insert(planId).second) {
                    ++diagnostics.duplicatePlansSkipped;
                    continue;
                }
                std::optional<MasterCandidate> candidate = evaluate_exact_plan(
                    state,
                    ledger,
                    plan,
                    options.mandatoryReservations);
                if (!candidate.has_value()) {
                    ++diagnostics.invalidPlanCombinations;
                    continue;
                }
                verify_column_footprint(config_, seedSelection, candidate->simulation);
                ++diagnostics.simulatorValidCombinations;
                learnedCut = record_stock_diagnostics(
                    candidate->simulation,
                    cutState,
                    diagnostics) || learnedCut;
                candidates.push_back(std::move(*candidate));
                if (static_cast<std::int32_t>(candidates.size()) > options.maximumCandidates * 2) {
                    retain_alns_population(
                        candidates,
                        options.maximumCandidates,
                        options.diversityCandidates);
                }
            }
        }
        std::function<bool(std::size_t)> search;
        search = [&](std::size_t depth) {
            if (diagnostics.combinationsVisited >= roundLimit) {
                return false;
            }
            if (deadline_expired()) {
                diagnostics.deadlineReached = true;
                return false;
            }
            if (options.enableLexicographicBranchAndBound && exactMetadata &&
                static_cast<std::int32_t>(candidates.size()) >= options.maximumCandidates) {
                const OfficialScore branchUpperBound = optimistic_partial_score(
                    config_,
                    ledger,
                    selected,
                    ordering,
                    depth,
                    orderedColumns);
                if (compare_lexicographic(
                        branchUpperBound,
                        worst_candidate(candidates).scoreAfterToday) < 0) {
                    ++diagnostics.branchesPruned;
                    return true;
                }
            }
            if (depth != ordering.size()) {
                const AgentIndex agentIndex = ordering.at(depth);
                std::uint64_t selectedBrands = 0;
                std::set<std::int32_t> activeEscortGroups;
                for (const RouteColumn* selectedColumn : selected) {
                    if (selectedColumn != nullptr) {
                        selectedBrands |= column_brand_mask(config_, *selectedColumn);
                        if (selectedColumn->escortGroup >= 0) {
                            activeEscortGroups.insert(selectedColumn->escortGroup);
                        }
                    }
                }
                std::vector<const RouteColumn*> branchColumns =
                    orderedColumns.at(static_cast<std::size_t>(agentIndex));
                std::stable_sort(
                    branchColumns.begin(),
                    branchColumns.end(),
                    [&ledger, selectedBrands, &activeEscortGroups, this](
                        const RouteColumn* left,
                        const RouteColumn* right) {
                        const bool leftMatchesEscort = activeEscortGroups.contains(left->escortGroup);
                        const bool rightMatchesEscort = activeEscortGroups.contains(right->escortGroup);
                        if (leftMatchesEscort != rightMatchesEscort) {
                            return leftMatchesEscort;
                        }
                        const std::uint64_t leftBrands = column_brand_mask(config_, *left);
                        const std::uint64_t rightBrands = column_brand_mask(config_, *right);
                        const std::int32_t leftLifetimeGain = static_cast<std::int32_t>(std::popcount(
                            leftBrands & ~(ledger.lifetimeBrands | selectedBrands)));
                        const std::int32_t rightLifetimeGain = static_cast<std::int32_t>(std::popcount(
                            rightBrands & ~(ledger.lifetimeBrands | selectedBrands)));
                        if (leftLifetimeGain != rightLifetimeGain) {
                            return leftLifetimeGain > rightLifetimeGain;
                        }
                        const std::int32_t leftDailyGain = static_cast<std::int32_t>(
                            std::popcount(leftBrands & ~selectedBrands));
                        const std::int32_t rightDailyGain = static_cast<std::int32_t>(
                            std::popcount(rightBrands & ~selectedBrands));
                        return leftDailyGain > rightDailyGain;
                    });
                for (const RouteColumn* column : branchColumns) {
                    bool bundleCompatible = true;
                    for (const RouteColumn* assigned : selected) {
                        if (assigned == nullptr || assigned->agent == agentIndex) {
                            continue;
                        }
                        if (assigned->contingencyBundle != column->contingencyBundle &&
                            (assigned->contingencyBundle >= 0 || column->contingencyBundle >= 0)) {
                            bundleCompatible = false;
                            break;
                        }
                    }
                    if (!bundleCompatible) {
                        ++diagnostics.branchesPruned;
                        continue;
                    }
                    selected.at(static_cast<std::size_t>(agentIndex)) = column;
                    if (!partial_synchronized_selection_is_feasible(state, selected, orderedColumns)) {
                        ++diagnostics.branchesPruned;
                        selected.at(static_cast<std::size_t>(agentIndex)) = nullptr;
                        continue;
                    }
                    if (!search(depth + 1U)) {
                        selected.at(static_cast<std::size_t>(agentIndex)) = nullptr;
                        return false;
                    }
                }
                selected.at(static_cast<std::size_t>(agentIndex)) = nullptr;
                return true;
            }
            ++diagnostics.combinationsVisited;
            if (!synchronized_selection_is_valid(state, selected)) {
                ++diagnostics.synchronizationConflicts;
                return true;
            }
            DayPlan plan;
            plan.actions.resize(static_cast<std::size_t>(config_.agent_count()));
            for (AgentIndex agentIndex = 0; agentIndex < config_.agent_count(); ++agentIndex) {
                plan.actions.at(static_cast<std::size_t>(agentIndex)) =
                    selected.at(static_cast<std::size_t>(agentIndex))->actions;
            }
            const std::string planId = canonical_plan_bytes(plan);
            if (!evaluatedPlans.insert(planId).second) {
                ++diagnostics.duplicatePlansSkipped;
                return true;
            }
            const ServiceCreditAssignment credits = options.useStockCredits
                ? assign_service_credits(config_, selected)
                : ServiceCreditAssignment{};
            const SimulationResult simulation = simulator_.simulate(state, plan, false);
            if (!simulation.valid) {
                ++diagnostics.invalidPlanCombinations;
                return true;
            }
            if (!simulation_covers_proven_reservations(
                    simulation,
                    state,
                    options.mandatoryReservations)) {
                ++diagnostics.reservationConflicts;
                return true;
            }
            verify_column_footprint(config_, selected, simulation);
            const SimulationResult validation = validator_.validate(state, plan, false);
            std::string mismatch;
            if (!validator_.agrees_with(simulation, validation, mismatch)) {
                throw std::runtime_error("independent validation disagreement: " + mismatch);
            }
            ++diagnostics.simulatorValidCombinations;
            if (options.useStockCredits) {
                diagnostics.stockCreditDenials += static_cast<std::int32_t>(std::count_if(
                    simulation.claims.begin(),
                    simulation.claims.end(),
                    [](const ClaimEvent& claim) { return !claim.served; }));
                if (exactMetadata && !credits_match_exact(credits, simulation)) {
                    ++diagnostics.exactCreditMismatches;
                }
            }
            learnedCut = record_stock_diagnostics(simulation, cutState, diagnostics) || learnedCut;
            MasterCandidate candidate;
            candidate.plan = std::move(plan);
            candidate.simulation = simulation;
            candidate.scoreAfterToday = OfficialScore::after_day(ledger, simulation.score);
            candidate.terminalSlack = calculate_terminal_slack(config_, terminalDistancesToSpots_, simulation, ledger);
            candidate.trafficSafety = calculate_traffic_safety(config_, simulation);
            candidate.stableId = planId;
            candidate.creditedServings = simulation.score.servings;
            candidates.push_back(std::move(candidate));
            if (static_cast<std::int32_t>(candidates.size()) > options.maximumCandidates * 2) {
                retain_alns_population(
                    candidates,
                    options.maximumCandidates,
                    options.diversityCandidates);
            }
            return true;
        };
        const bool roundCompleted = search(0U);
        diagnostics.searchComplete = diagnostics.searchComplete || roundCompleted;
        if (diagnostics.deadlineReached || roundCompleted || !learnedCut) {
            break;
        }
    }
    candidates.erase(
        std::unique(
            candidates.begin(),
            candidates.end(),
            [](const MasterCandidate& left, const MasterCandidate& right) { return left.stableId == right.stableId; }),
        candidates.end());
    retain_alns_population(
        candidates,
        options.maximumCandidates,
        options.diversityCandidates);
    return candidates;
}

RoleAssignmentEnumerator::RoleAssignmentEnumerator(const MatchConfig& config)
    : config_(config) {}

namespace {

constexpr std::size_t kAlnsOperatorCount = 8U;

[[nodiscard]] std::size_t alns_index(AlnsOperator value) {
    return static_cast<std::size_t>(value);
}

[[nodiscard]] std::int32_t column_critical_stays(
    const RouteColumn& column,
    const std::vector<CellId>& criticalRoads) {
    std::int32_t stays = 0;
    for (const CellId road : criticalRoads) {
        stays += column.fullFootprint.at(road);
    }
    return stays;
}

[[nodiscard]] const RouteColumn* column_for_plan(
    const std::vector<RouteColumn>& columns,
    const AgentPlan& plan) {
    const auto iterator = std::find_if(
        columns.begin(),
        columns.end(),
        [&plan](const RouteColumn& column) { return same_agent_plan(column.actions, plan); });
    return iterator == columns.end() ? nullptr : &*iterator;
}

[[nodiscard]] bool column_visits_spot(const RouteColumn& column, SpotIndex spot) {
    return std::any_of(
        column.firstVisits.begin(),
        column.firstVisits.end(),
        [spot](const ColumnVisitEvent& event) { return event.spot == spot; });
}

[[nodiscard]] std::optional<SpotIndex> multi_visit_spot(
    const MatchConfig& config,
    const RouteColumn& column) {
    for (const ColumnVisitEvent& event : column.firstVisits) {
        if (config.spots.at(static_cast<std::size_t>(event.spot)).stock > 1) {
            return event.spot;
        }
    }
    return std::nullopt;
}

[[nodiscard]] bool column_matches_alns_operator(
    AlnsOperator operation,
    const MatchConfig& config,
    const MatchLedger& ledger,
    const RouteColumn& column,
    const AlnsOptions& options,
    CellId currentTerminal,
    std::int32_t currentCriticalStays,
    std::int32_t dayNumber) {
    switch (operation) {
    case AlnsOperator::RareBrandRescue:
        return std::any_of(
            column.firstVisits.begin(),
            column.firstVisits.end(),
            [&config, &ledger](const ColumnVisitEvent& event) {
                return !has_brand(
                    ledger.lifetimeBrands,
                    config.spots.at(static_cast<std::size_t>(event.spot)).brandIndex);
            });
    case AlnsOperator::OvernightHarvest:
        return column.terminalFeatures.overnightHarvestCandidate;
    case AlnsOperator::DockOrRendezvous:
        return column.escortGroup >= 0 || !column.requiredRefuels.empty() ||
            column.terminalFeatures.endStepDockRequired;
    case AlnsOperator::MergeSplit:
        return column.escortGroup >= 0 && !column.lockstepEscort &&
            std::any_of(
                column.escortSegments.begin(),
                column.escortSegments.end(),
                [](const EscortSegment& segment) {
                    return segment.lastStep > segment.firstStep;
                });
    case AlnsOperator::StockMultiVisit:
        return multi_visit_spot(config, column).has_value();
    case AlnsOperator::CriticalRoadBypass:
        return !options.criticalRoads.empty() &&
            column_critical_stays(column, options.criticalRoads) < currentCriticalStays;
    case AlnsOperator::TerminalShift:
        return column.terminalCell != kInvalidCell && column.terminalCell != currentTerminal;
    case AlnsOperator::ViabilityRepair:
        return std::any_of(
            column.firstVisits.begin(),
            column.firstVisits.end(),
            [&config, &options, dayNumber](const ColumnVisitEvent& event) {
                const std::int32_t brand = config.spots.at(static_cast<std::size_t>(event.spot)).brandIndex;
                return std::any_of(
                    options.mandatoryReservations.begin(),
                    options.mandatoryReservations.end(),
                    [brand, dayNumber](const MandatoryReservation& reservation) {
                        return reservation.brandIndex == brand && reservation.latestSafeDay <= dayNumber;
                    });
            });
    }
    return false;
}

[[nodiscard]] std::int32_t alns_column_score(
    AlnsOperator operation,
    const MatchConfig& config,
    const MatchLedger& ledger,
    const RouteColumn& column,
    const AlnsOptions& options) {
    std::int32_t score = column.priority;
    if (operation == AlnsOperator::RareBrandRescue) {
        for (const ColumnVisitEvent& event : column.firstVisits) {
            if (!has_brand(ledger.lifetimeBrands, config.spots.at(static_cast<std::size_t>(event.spot)).brandIndex)) {
                score += 1000000;
                const std::int32_t brand = config.spots.at(static_cast<std::size_t>(event.spot)).brandIndex;
                if (brand >= 0 && static_cast<std::size_t>(brand) < options.brandSlack.size()) {
                    const std::int32_t slack = options.brandSlack.at(static_cast<std::size_t>(brand));
                    score += std::max(0, 10000 - std::max(0, slack)) * 100;
                }
                if (brand >= 0 && static_cast<std::size_t>(brand) < options.latestSafeDayByBrand.size()) {
                    const std::int32_t latestSafeDay = options.latestSafeDayByBrand.at(static_cast<std::size_t>(brand));
                    score += std::max(0, config.day_count() - latestSafeDay + 1) * 1000;
                }
            }
        }
    }
    if (operation == AlnsOperator::CriticalRoadBypass) {
        score -= column_critical_stays(column, options.criticalRoads) * 1000;
    }
    if (operation == AlnsOperator::MergeSplit) {
        for (const EscortSegment& segment : column.escortSegments) {
            score += std::max(0, segment.lastStep - segment.firstStep) * 1000;
        }
    }
    if (operation == AlnsOperator::OvernightHarvest &&
        column.terminalFeatures.spot != kInvalidSpot) {
        const Spot& terminalSpot = config.spots.at(
            static_cast<std::size_t>(column.terminalFeatures.spot));
        score += terminalSpot.stock * 1000;
        if (!has_brand(ledger.lifetimeBrands, terminalSpot.brandIndex)) {
            score += 1000000;
        }
    }
    if (operation == AlnsOperator::DockOrRendezvous) {
        score += column.terminalFeatures.endStepDockRequired ? 2000000 : 0;
        score += static_cast<std::int32_t>(column.requiredRefuels.size()) * 10000;
    }
    return score;
}

[[nodiscard]] bool apply_contingency_bundle(
    DayPlan& mutation,
    const RoutePortfolio& portfolio,
    std::int32_t bundle) {
    if (bundle < 0 || mutation.actions.size() != portfolio.columnsByAgent.size()) {
        return false;
    }
    for (std::size_t agentIndex = 0; agentIndex < portfolio.columnsByAgent.size(); ++agentIndex) {
        const auto column = std::find_if(
            portfolio.columnsByAgent.at(agentIndex).begin(),
            portfolio.columnsByAgent.at(agentIndex).end(),
            [bundle](const RouteColumn& candidate) { return candidate.contingencyBundle == bundle; });
        if (column == portfolio.columnsByAgent.at(agentIndex).end()) {
            return false;
        }
        mutation.actions.at(agentIndex) = column->actions;
    }
    return true;
}

[[nodiscard]] bool apply_escort_group(
    DayPlan& mutation,
    const RoutePortfolio& portfolio,
    std::int32_t escortGroup) {
    if (escortGroup < 0 || mutation.actions.size() != portfolio.columnsByAgent.size()) {
        return false;
    }
    std::vector<std::pair<std::size_t, const RouteColumn*>> group;
    for (std::size_t agentIndex = 0; agentIndex < portfolio.columnsByAgent.size(); ++agentIndex) {
        const auto column = std::find_if(
            portfolio.columnsByAgent.at(agentIndex).begin(),
            portfolio.columnsByAgent.at(agentIndex).end(),
            [escortGroup](const RouteColumn& candidate) { return candidate.escortGroup == escortGroup; });
        if (column != portfolio.columnsByAgent.at(agentIndex).end()) {
            group.emplace_back(agentIndex, &*column);
        }
    }
    if (group.size() < 2U) {
        return false;
    }
    for (const auto& [agentIndex, column] : group) {
        mutation.actions.at(agentIndex) = column->actions;
    }
    return true;
}

[[nodiscard]] bool apply_stock_partner(
    const MatchConfig& config,
    DayPlan& mutation,
    const DayPlan& seed,
    const DayState& state,
    const RoutePortfolio& portfolio,
    AgentIndex primaryAgent,
    const RouteColumn& primaryColumn) {
    const std::optional<SpotIndex> target = multi_visit_spot(config, primaryColumn);
    if (!target.has_value()) {
        return false;
    }
    for (AgentIndex agentIndex = 0; agentIndex < static_cast<AgentIndex>(portfolio.columnsByAgent.size()); ++agentIndex) {
        if (agentIndex == primaryAgent ||
            state.agents.at(static_cast<std::size_t>(agentIndex)).kind != AgentKind::Patrol) {
            continue;
        }
        const std::vector<RouteColumn>& columns = portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex));
        const RouteColumn* original = column_for_plan(
            columns,
            seed.actions.at(static_cast<std::size_t>(agentIndex)));
        if (original != nullptr && column_visits_spot(*original, *target)) {
            return true;
        }
        const RouteColumn* replacement = nullptr;
        for (const RouteColumn& candidate : columns) {
            if (candidate.escortGroup >= 0 || candidate.contingencyBundle >= 0 ||
                !column_visits_spot(candidate, *target) ||
                same_agent_plan(candidate.actions, seed.actions.at(static_cast<std::size_t>(agentIndex)))) {
                continue;
            }
            if (replacement == nullptr || candidate.priority > replacement->priority ||
                (candidate.priority == replacement->priority && candidate.columnId < replacement->columnId)) {
                replacement = &candidate;
            }
        }
        if (replacement != nullptr) {
            mutation.actions.at(static_cast<std::size_t>(agentIndex)) = replacement->actions;
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::int32_t repair_target_priority(
    AlnsOperator operation,
    const MatchConfig& config,
    const MatchLedger& ledger,
    const AlnsOptions& options,
    std::int32_t dayNumber,
    SpotIndex spotIndex) {
    const Spot& spot = config.spots.at(static_cast<std::size_t>(spotIndex));
    const bool unseen = !has_brand(ledger.lifetimeBrands, spot.brandIndex);
    std::int32_t priority = unseen ? 1000000 : 0;
    priority += spot.stock * 100;
    if (operation == AlnsOperator::RareBrandRescue) {
        priority += (config.brand_count() - brand_rarity(config, spot.brandIndex)) * 10000;
        if (static_cast<std::size_t>(spot.brandIndex) < options.brandSlack.size()) {
            priority += std::max(0, 10000 - std::max(0, options.brandSlack.at(
                static_cast<std::size_t>(spot.brandIndex))));
        }
        if (static_cast<std::size_t>(spot.brandIndex) < options.latestSafeDayByBrand.size()) {
            priority += std::max(
                0,
                config.day_count() - options.latestSafeDayByBrand.at(
                    static_cast<std::size_t>(spot.brandIndex)) + 1) * 1000;
        }
    }
    if (operation == AlnsOperator::StockMultiVisit) {
        priority += spot.stock > 1 ? 2000000 : -2000000;
    }
    if (operation == AlnsOperator::ViabilityRepair) {
        for (const MandatoryReservation& reservation : options.mandatoryReservations) {
            if (reservation.representativeSpot == spotIndex && reservation.latestSafeDay <= dayNumber) {
                priority += is_proven_reservation(reservation) ? 4000000 : 3000000;
            }
        }
    }
    return priority;
}

[[nodiscard]] std::optional<AlnsOperator> proof_gap_operation(
    const OfficialScore& upperBound,
    const OfficialScore& incumbent) {
    if (upperBound.lifetimeDistinct > incumbent.lifetimeDistinct) {
        return AlnsOperator::RareBrandRescue;
    }
    if (upperBound.lifetimeDistinct < incumbent.lifetimeDistinct) {
        return std::nullopt;
    }
    if (upperBound.totalDailyDistinct > incumbent.totalDailyDistinct) {
        return AlnsOperator::OvernightHarvest;
    }
    if (upperBound.totalDailyDistinct < incumbent.totalDailyDistinct) {
        return std::nullopt;
    }
    if (upperBound.totalServings > incumbent.totalServings) {
        return AlnsOperator::StockMultiVisit;
    }
    return std::nullopt;
}

[[nodiscard]] ParetoPath combine_paths(const ParetoPath& first, const ParetoPath& second) {
    ParetoPath combined;
    combined.directions = first.directions;
    combined.directions.insert(
        combined.directions.end(),
        second.directions.begin(),
        second.directions.end());
    combined.travelSteps = first.travelSteps + second.travelSteps;
    combined.patrolFuel = first.patrolFuel + second.patrolFuel;
    combined.heuristicFootprint = first.heuristicFootprint;
    for (const auto& [road, stays] : second.heuristicFootprint.entries) {
        combined.heuristicFootprint.add(road, stays);
    }
    return combined;
}

[[nodiscard]] bool portfolio_contains_agent_plan(
    const std::vector<RouteColumn>& columns,
    const AgentPlan& plan) {
    return std::any_of(
        columns.begin(),
        columns.end(),
        [&plan](const RouteColumn& column) { return same_agent_plan(column.actions, plan); });
}

[[nodiscard]] std::vector<RouteColumn> synthesize_repair_columns(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    const RoutePortfolio& portfolio,
    const MasterCandidate& seed,
    const RouteColumn* seedColumn,
    AlnsOperator operation,
    AgentIndex agentIndex,
    const AlnsOptions& options,
    const ParetoRouter& router) {
    std::vector<RouteColumn> result;
    const AgentState& agent = state.agents.at(static_cast<std::size_t>(agentIndex));
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    std::vector<std::pair<CellId, std::int32_t>> targets;
    const auto append_target = [&targets, &config](CellId cell, std::int32_t priority) {
        if (!config.map.contains(cell) ||
            config.map.terrain.at(static_cast<std::size_t>(cell)) == Terrain::Pond) {
            return;
        }
        const auto existing = std::find_if(
            targets.begin(),
            targets.end(),
            [cell](const std::pair<CellId, std::int32_t>& target) { return target.first == cell; });
        if (existing == targets.end()) {
            targets.emplace_back(cell, priority);
        } else {
            existing->second = std::max(existing->second, priority);
        }
    };

    const bool spotTargetOperation =
        operation == AlnsOperator::RareBrandRescue ||
        operation == AlnsOperator::OvernightHarvest ||
        operation == AlnsOperator::StockMultiVisit ||
        operation == AlnsOperator::TerminalShift ||
        operation == AlnsOperator::ViabilityRepair;
    if (operation == AlnsOperator::CriticalRoadBypass) {
        append_target(
            seed.simulation.finalAgents.at(static_cast<std::size_t>(agentIndex)).position,
            4000000);
    } else if (operation == AlnsOperator::DockOrRendezvous ||
               operation == AlnsOperator::MergeSplit) {
        for (AgentIndex otherAgent = 0; otherAgent < config.agent_count(); ++otherAgent) {
            if (otherAgent == agentIndex ||
                state.agents.at(static_cast<std::size_t>(otherAgent)).kind == agent.kind) {
                continue;
            }
            append_target(
                seed.simulation.finalAgents.at(static_cast<std::size_t>(otherAgent)).position,
                3000000);
        }
    }
    if (spotTargetOperation) {
        for (SpotIndex spotIndex = 0; spotIndex < static_cast<SpotIndex>(config.spots.size()); ++spotIndex) {
            const Spot& spot = config.spots.at(static_cast<std::size_t>(spotIndex));
            std::int32_t priority =
                repair_target_priority(operation, config, ledger, options, state.dayNumber, spotIndex);
            if (operation == AlnsOperator::RareBrandRescue) {
                const std::uint64_t coveredAfterCandidate =
                    ledger.lifetimeBrands | seed.simulation.score.brands;
                priority += has_brand(coveredAfterCandidate, spot.brandIndex)
                    ? -3000000
                    : 3000000;
            } else if (operation == AlnsOperator::OvernightHarvest) {
                priority += has_brand(seed.simulation.score.brands, spot.brandIndex)
                    ? -1500000
                    : 2000000;
            } else if (operation == AlnsOperator::StockMultiVisit) {
                const std::int32_t served = static_cast<std::int32_t>(std::count_if(
                    seed.simulation.claims.begin(),
                    seed.simulation.claims.end(),
                    [spotIndex](const ClaimEvent& claim) {
                        return claim.spot == spotIndex && claim.served;
                    }));
                priority += served < spot.stock ? 2000000 : -2000000;
            }
            append_target(
                spot.position,
                priority);
        }
    }
    std::sort(
        targets.begin(),
        targets.end(),
        [](const std::pair<CellId, std::int32_t>& left, const std::pair<CellId, std::int32_t>& right) {
            if (left.second != right.second) {
                return left.second > right.second;
            }
            return left.first < right.first;
        });
    if (targets.size() > 8U) {
        targets.resize(8U);
    }

    std::set<std::string> generated;
    const std::vector<RouteColumn>& existingColumns = portfolio.columnsByAgent.at(
        static_cast<std::size_t>(agentIndex));
    const auto append_path = [&](const ParetoPath& path, CellId terminalCell, std::int32_t priority) {
        if (path.travelSteps > daySteps) {
            return;
        }
        AgentPlan actions = complete_actions(config, state, path);
        if (same_agent_plan(actions, seed.plan.actions.at(static_cast<std::size_t>(agentIndex))) ||
            portfolio_contains_agent_plan(existingColumns, actions)) {
            return;
        }
        const std::string key = actions_key(actions, -1, -1, {});
        if (!generated.insert(key).second) {
            return;
        }
        RouteColumn repaired;
        repaired.columnId = -1;
        repaired.agent = agentIndex;
        repaired.actions = std::move(actions);
        repaired.terminalCell = terminalCell;
        repaired.terminalFuel = agent.kind == AgentKind::Patrol
            ? agent.fuel - path.patrolFuel
            : agent.fuel;
        repaired.heuristicFootprint = path.heuristicFootprint;
        repaired.priority = priority;
        populate_first_visits(config, state, repaired);
        if (repaired.hasExactTimeline &&
            (operation != AlnsOperator::CriticalRoadBypass ||
             column_critical_stays(repaired, options.criticalRoads) <
                 (seedColumn == nullptr
                     ? std::numeric_limits<std::int32_t>::max()
                     : column_critical_stays(*seedColumn, options.criticalRoads)))) {
            result.push_back(std::move(repaired));
        }
    };

    ParetoSearchOptions directOptions;
    directOptions.maximumTravelSteps = daySteps;
    directOptions.maximumPatrolFuel = agent.kind == AgentKind::Patrol
        ? agent.fuel
        : std::numeric_limits<std::int32_t>::max();
    directOptions.maximumLabelsPerCell = 64;
    directOptions.maximumPaths = 8;
    directOptions.patrol = agent.kind == AgentKind::Patrol;
    directOptions.criticalRoads = options.criticalRoads;
    directOptions.deadline = options.deadline;
    for (const auto& [targetCell, targetPriority] : targets) {
        const std::vector<ParetoPath> directPaths = router.find_paths(
            agent.position,
            targetCell,
            state.roadStatuses,
            directOptions);
        for (const ParetoPath& path : directPaths) {
            append_path(path, targetCell, targetPriority - path.travelSteps);
            if (static_cast<std::int32_t>(result.size()) >= options.maximumAlternativesPerIteration) {
                return result;
            }
        }
        if (seedColumn == nullptr || seedColumn->firstVisits.empty()) {
            continue;
        }
        const SpotIndex anchorSpot = seedColumn->firstVisits.front().spot;
        const CellId anchorCell = config.spots.at(static_cast<std::size_t>(anchorSpot)).position;
        if (anchorCell == targetCell) {
            continue;
        }
        ParetoSearchOptions firstOptions = directOptions;
        firstOptions.maximumPaths = 2;
        const std::vector<ParetoPath> firstPaths = router.find_paths(
            agent.position,
            anchorCell,
            state.roadStatuses,
            firstOptions);
        for (const ParetoPath& firstPath : firstPaths) {
            ParetoSearchOptions secondOptions = directOptions;
            secondOptions.maximumTravelSteps = daySteps - firstPath.travelSteps;
            secondOptions.maximumPatrolFuel = agent.kind == AgentKind::Patrol
                ? agent.fuel - firstPath.patrolFuel
                : std::numeric_limits<std::int32_t>::max();
            secondOptions.maximumPaths = 4;
            const std::vector<ParetoPath> secondPaths = router.find_paths(
                anchorCell,
                targetCell,
                state.roadStatuses,
                secondOptions);
            for (const ParetoPath& secondPath : secondPaths) {
                append_path(
                    combine_paths(firstPath, secondPath),
                    targetCell,
                    targetPriority - firstPath.travelSteps - secondPath.travelSteps);
                if (static_cast<std::int32_t>(result.size()) >= options.maximumAlternativesPerIteration) {
                    return result;
                }
            }
        }
    }
    return result;
}

} 

AdaptiveRouteImprover::AdaptiveRouteImprover(const MatchConfig& config, const RouteMaster& master)
    : config_(config), master_(master), repairRouter_(config_) {}

std::vector<MasterCandidate> AdaptiveRouteImprover::improve(
    const DayState& state,
    const MatchLedger& ledger,
    const RoutePortfolio& portfolio,
    std::vector<MasterCandidate> candidates,
    const AlnsOptions& options,
    AlnsDiagnostics& diagnostics) const {
    diagnostics = AlnsDiagnostics{};
    if (options.maximumIterations <= 0 || options.maximumCandidates <= 0 || candidates.empty() ||
        portfolio.columnsByAgent.size() != static_cast<std::size_t>(config_.agent_count())) {
        return candidates;
    }
    retain_alns_population(
        candidates,
        options.maximumCandidates,
        options.diversityCandidates);
    std::set<std::string> seen;
    for (const MasterCandidate& candidate : candidates) {
        seen.insert(candidate.stableId);
    }
    std::array<std::int32_t, kAlnsOperatorCount> weights{8, 6, 5, 4, 5, 5, 4, 6};
    std::array<std::int32_t, kAlnsOperatorCount> deficits{};
    const auto deadline_expired = [&options]() {
        return options.deadline.has_value() && std::chrono::steady_clock::now() >= *options.deadline;
    };
    for (std::int32_t iteration = 0; iteration < options.maximumIterations && !deadline_expired(); ++iteration) {
        const std::int32_t totalWeight = std::accumulate(weights.begin(), weights.end(), 0);
        for (std::size_t operatorIndex = 0; operatorIndex < kAlnsOperatorCount; ++operatorIndex) {
            deficits.at(operatorIndex) += weights.at(operatorIndex);
        }
        const std::size_t selectedOperator = static_cast<std::size_t>(std::distance(
            deficits.begin(),
            std::max_element(deficits.begin(), deficits.end())));
        deficits.at(selectedOperator) -= totalWeight;
        const AlnsOperator operation = static_cast<AlnsOperator>(selectedOperator);
        ++diagnostics.attemptedByOperator.at(selectedOperator);
        ++diagnostics.iterations;
        const MasterCandidate seed = candidates.at(static_cast<std::size_t>(iteration) % candidates.size());
        const AgentIndex agentIndex = iteration % config_.agent_count();
        const std::vector<RouteColumn>& columns = portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex));
        if (columns.empty()) {
            continue;
        }
        const CellId currentTerminal = seed.simulation.finalAgents.at(static_cast<std::size_t>(agentIndex)).position;
        const RouteColumn* seedColumn = column_for_plan(
            columns,
            seed.plan.actions.at(static_cast<std::size_t>(agentIndex)));
        const std::int32_t currentCriticalStays = seedColumn == nullptr
            ? std::numeric_limits<std::int32_t>::max()
            : column_critical_stays(*seedColumn, options.criticalRoads);
        std::vector<const RouteColumn*> alternatives;
        alternatives.reserve(columns.size());
        for (const RouteColumn& column : columns) {
            if (same_agent_plan(column.actions, seed.plan.actions.at(static_cast<std::size_t>(agentIndex))) ||
                !column_matches_alns_operator(
                    operation,
                    config_,
                    ledger,
                    column,
                    options,
                    currentTerminal,
                    currentCriticalStays,
                    state.dayNumber)) {
                continue;
            }
            alternatives.push_back(&column);
        }
        std::sort(
            alternatives.begin(),
            alternatives.end(),
            [&operation, this, &ledger, &options](const RouteColumn* left, const RouteColumn* right) {
                const std::int32_t leftScore = alns_column_score(
                    operation, config_, ledger, *left, options);
                const std::int32_t rightScore = alns_column_score(
                    operation, config_, ledger, *right, options);
                if (leftScore != rightScore) {
                    return leftScore > rightScore;
                }
                return left->columnId < right->columnId;
            });
        const std::int32_t limit = std::min(
            options.maximumAlternativesPerIteration,
            static_cast<std::int32_t>(alternatives.size()));
        bool accepted = false;
        bool improved = false;
        for (std::int32_t alternativeIndex = 0;
              alternativeIndex < limit && !deadline_expired();
              ++alternativeIndex) {
            DayPlan mutation = seed.plan;
            const RouteColumn& alternative = *alternatives.at(static_cast<std::size_t>(alternativeIndex));
            bool applied = false;
            if (alternative.contingencyBundle >= 0) {
                applied = apply_contingency_bundle(mutation, portfolio, alternative.contingencyBundle);
            } else if (alternative.escortGroup >= 0) {
                applied = apply_escort_group(mutation, portfolio, alternative.escortGroup);
            } else {
                mutation.actions.at(static_cast<std::size_t>(agentIndex)) = alternative.actions;
                applied = true;
            }
            if (!applied || canonical_plan_bytes(mutation) == seed.stableId) {
                continue;
            }
            if (operation == AlnsOperator::StockMultiVisit && !apply_stock_partner(
                    config_,
                    mutation,
                    seed.plan,
                    state,
                    portfolio,
                    agentIndex,
                    alternative)) {
                continue;
            }
            std::optional<MasterCandidate> evaluated = master_.evaluate_exact_plan(
                state,
                ledger,
                mutation,
                options.mandatoryReservations);
            if (!evaluated.has_value() || !seen.insert(evaluated->stableId).second) {
                continue;
            }
            accepted = true;
            const bool improvement = better_candidate(*evaluated, candidates.front());
            improved = improved || improvement;
            candidates.push_back(std::move(*evaluated));
            retain_alns_population(
                candidates,
                options.maximumCandidates,
                options.diversityCandidates);
            break;
        }
        const bool supportsIndependentRepair =
            operation != AlnsOperator::DockOrRendezvous &&
            operation != AlnsOperator::MergeSplit;
        if (!accepted && supportsIndependentRepair && !deadline_expired()) {
            std::vector<RouteColumn> repairedColumns = synthesize_repair_columns(
                config_,
                state,
                ledger,
                portfolio,
                seed,
                seedColumn,
                operation,
                agentIndex,
                options,
                repairRouter_);
            diagnostics.synthesizedRoutes += static_cast<std::int32_t>(repairedColumns.size());
            for (RouteColumn& repaired : repairedColumns) {
                if (deadline_expired()) {
                    break;
                }
                DayPlan mutation = seed.plan;
                mutation.actions.at(static_cast<std::size_t>(agentIndex)) = repaired.actions;
                if (operation == AlnsOperator::StockMultiVisit && !apply_stock_partner(
                        config_,
                        mutation,
                        seed.plan,
                        state,
                        portfolio,
                        agentIndex,
                        repaired)) {
                    continue;
                }
                std::optional<MasterCandidate> evaluated = master_.evaluate_exact_plan(
                    state,
                    ledger,
                    mutation,
                    options.mandatoryReservations);
                if (!evaluated.has_value() || !seen.insert(evaluated->stableId).second) {
                    continue;
                }
                accepted = true;
                ++diagnostics.synthesizedAccepted;
                const bool improvement = better_candidate(*evaluated, candidates.front());
                improved = improved || improvement;
                candidates.push_back(std::move(*evaluated));
                retain_alns_population(
                    candidates,
                    options.maximumCandidates,
                    options.diversityCandidates);
                break;
            }
        }
        if (accepted) {
            ++diagnostics.accepted;
            ++diagnostics.acceptedByOperator.at(selectedOperator);
            weights.at(selectedOperator) = std::min(32, weights.at(selectedOperator) + 1);
            if (improved) {
                ++diagnostics.improvements;
                weights.at(selectedOperator) = std::min(32, weights.at(selectedOperator) + 3);
            }
        } else {
            weights.at(selectedOperator) = std::max(1, weights.at(selectedOperator) - 1);
        }
    }
    for (std::int32_t proofIteration = 0;
         proofIteration < options.maximumProofGuidedIterations && !deadline_expired();
         ++proofIteration) {
        retain_alns_population(
            candidates,
            options.maximumCandidates,
            options.diversityCandidates);
        const std::optional<AlnsOperator> operation = proof_gap_operation(
            options.proofUpperBound,
            candidates.front().scoreAfterToday);
        if (!operation.has_value()) {
            break;
        }
        const AgentIndex agentIndex = proofIteration % config_.agent_count();
        if (state.agents.at(static_cast<std::size_t>(agentIndex)).kind != AgentKind::Patrol) {
            continue;
        }
        ++diagnostics.proofGuidedIterations;
        const MasterCandidate seed = candidates.front();
        const std::vector<RouteColumn>& columns =
            portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex));
        const RouteColumn* seedColumn = column_for_plan(
            columns,
            seed.plan.actions.at(static_cast<std::size_t>(agentIndex)));
        std::vector<RouteColumn> repairedColumns = synthesize_repair_columns(
            config_,
            state,
            ledger,
            portfolio,
            seed,
            seedColumn,
            *operation,
            agentIndex,
            options,
            repairRouter_);
        diagnostics.proofGuidedRoutes += static_cast<std::int32_t>(repairedColumns.size());
        for (RouteColumn& repaired : repairedColumns) {
            if (deadline_expired()) {
                break;
            }
            DayPlan mutation = seed.plan;
            mutation.actions.at(static_cast<std::size_t>(agentIndex)) = repaired.actions;
            if (*operation == AlnsOperator::StockMultiVisit && !apply_stock_partner(
                    config_,
                    mutation,
                    seed.plan,
                    state,
                    portfolio,
                    agentIndex,
                    repaired)) {
                continue;
            }
            std::optional<MasterCandidate> evaluated = master_.evaluate_exact_plan(
                state,
                ledger,
                mutation,
                options.mandatoryReservations);
            if (!evaluated.has_value() || !seen.insert(evaluated->stableId).second) {
                continue;
            }
            const bool improvement = better_candidate(*evaluated, candidates.front());
            candidates.push_back(std::move(*evaluated));
            ++diagnostics.proofGuidedAccepted;
            if (improvement) {
                ++diagnostics.proofGuidedImprovements;
            }
            break;
        }
    }
    retain_alns_population(
        candidates,
        options.maximumCandidates,
        options.diversityCandidates);
    return candidates;
}

std::vector<RoleAssignment> RoleAssignmentEnumerator::shortlist(
    std::int32_t beamWidth,
    std::optional<std::chrono::steady_clock::time_point> deadline) const {
    if (beamWidth <= 0) {
        return {};
    }
    const auto deadline_expired = [&deadline]() {
        return deadline.has_value() && std::chrono::steady_clock::now() >= *deadline;
    };
    ParetoRouter router(config_);
    std::vector<RoadStatus> smooth(static_cast<std::size_t>(config_.map.cell_count()), RoadStatus::Smooth);
    const std::int32_t totalSteps = std::accumulate(config_.daySteps.begin(), config_.daySteps.end(), 0);
    const std::uint32_t assignmentCount = std::uint32_t{1} << static_cast<std::uint32_t>(config_.agent_count());
    std::vector<std::uint64_t> directReachableByAgent(static_cast<std::size_t>(config_.agent_count()), 0);
    std::vector<std::uint64_t> escortedReachableByAgent(static_cast<std::size_t>(config_.agent_count()), 0);
    for (AgentIndex agentIndex = 0; agentIndex < config_.agent_count(); ++agentIndex) {
        if (deadline_expired()) {
            break;
        }
        ParetoSearchOptions options;
        options.maximumTravelSteps = totalSteps;
        options.maximumPatrolFuel = config_.fuelLimit;
        options.maximumLabelsPerCell = 4;
        options.maximumPaths = 1;
        options.patrol = true;
        options.deadline = deadline;
        for (const Spot& spot : config_.spots) {
            if (deadline_expired()) {
                break;
            }
            const std::vector<ParetoPath> paths = router.find_paths(
                config_.initialAgents.at(static_cast<std::size_t>(agentIndex)),
                spot.position,
                smooth,
                options);
            if (!paths.empty()) {
                directReachableByAgent.at(static_cast<std::size_t>(agentIndex)) |= brand_bit(spot.brandIndex);
            }
            options.maximumPatrolFuel = std::numeric_limits<std::int32_t>::max() / 4;
            const std::vector<ParetoPath> escortedPaths = router.find_paths(
                config_.initialAgents.at(static_cast<std::size_t>(agentIndex)),
                spot.position,
                smooth,
                options);
            if (!escortedPaths.empty()) {
                escortedReachableByAgent.at(static_cast<std::size_t>(agentIndex)) |= brand_bit(spot.brandIndex);
            }
            options.maximumPatrolFuel = config_.fuelLimit;
        }
    }
    std::int32_t perDayServings = 0;
    for (const Spot& spot : config_.spots) {
        perDayServings += spot.stock;
    }
    const OfficialScore feasibleSeedLowerBound{};
    std::vector<RoleAssignment> assignments;
    assignments.reserve(assignmentCount - 1U);
    for (std::uint32_t mask = 0; mask < assignmentCount; ++mask) {
        if (deadline_expired()) {
            break;
        }
        RoleAssignment assignment;
        assignment.roles.resize(static_cast<std::size_t>(config_.agent_count()), AgentKind::Patrol);
        std::uint64_t reachableBrands = 0;
        const std::int32_t tankerCount = static_cast<std::int32_t>(std::popcount(mask));
        for (AgentIndex agentIndex = 0; agentIndex < config_.agent_count(); ++agentIndex) {
            const bool tanker = (mask & (std::uint32_t{1} << static_cast<std::uint32_t>(agentIndex))) != 0U;
            assignment.roles.at(static_cast<std::size_t>(agentIndex)) = tanker ? AgentKind::Tanker : AgentKind::Patrol;
            if (tanker) {
                continue;
            }
            ++assignment.patrolCount;
            reachableBrands |= tankerCount > 0
                ? escortedReachableByAgent.at(static_cast<std::size_t>(agentIndex))
                : directReachableByAgent.at(static_cast<std::size_t>(agentIndex));
        }
        if (assignment.patrolCount == 0) {
            continue;
        }
        const std::int32_t lifetime = static_cast<std::int32_t>(std::popcount(reachableBrands));
        const std::int32_t daily = lifetime * config_.day_count();
        const std::int32_t servingCapPerDay = std::min(
            perDayServings,
            assignment.patrolCount * static_cast<std::int32_t>(config_.spots.size()));
        assignment.cheapUpperBound = OfficialScore{
            lifetime,
            daily,
            servingCapPerDay * config_.day_count(),
        };
        if (compare_lexicographic(
                assignment.cheapUpperBound,
                feasibleSeedLowerBound) <= 0) {
            continue;
        }
        assignments.push_back(std::move(assignment));
    }
    std::sort(
        assignments.begin(),
        assignments.end(),
        [](const RoleAssignment& left, const RoleAssignment& right) {
            const std::int32_t order = compare_lexicographic(left.cheapUpperBound, right.cheapUpperBound);
            if (order != 0) {
                return order > 0;
            }
            if (left.patrolCount != right.patrolCount) {
                return left.patrolCount > right.patrolCount;
            }
            return left.roles < right.roles;
        });
    if (static_cast<std::int32_t>(assignments.size()) > beamWidth) {
        assignments.resize(static_cast<std::size_t>(beamWidth));
    }
    return assignments;
}

GreedyPlanner::GreedyPlanner(
    const MatchConfig& config,
    const RouteColumnGenerator& generator,
    const RouteMaster& master)
    : config_(config), generator_(generator), master_(master) {}

MasterCandidate GreedyPlanner::build_incumbent(
    const DayState& state,
    const MatchLedger& ledger,
    const ColumnGenerationOptions& generationOptions,
    const MasterOptions& masterOptions,
    MasterDiagnostics& diagnostics) const {
    const RoutePortfolio portfolio = generator_.generate(state, ledger, generationOptions);
    std::vector<MasterCandidate> candidates = master_.solve(state, ledger, portfolio, masterOptions, diagnostics);
    if (candidates.empty()) {
        throw std::runtime_error("the exact wait column portfolio did not yield a valid incumbent");
    }
    return candidates.front();
}

DayPlan emergency_wait_plan(const MatchConfig& config, const DayState& state) {
    if (state.dayNumber < 1 || state.dayNumber > config.day_count() ||
        state.agents.size() != static_cast<std::size_t>(config.agent_count())) {
        throw std::invalid_argument("emergency wait plan requires a complete valid day state");
    }
    DayPlan plan;
    plan.actions.reserve(static_cast<std::size_t>(config.agent_count()));
    const std::int32_t duration = config.steps_for_day(state.dayNumber);
    for (AgentIndex agentIndex = 0; agentIndex < config.agent_count(); ++agentIndex) {
        plan.actions.push_back(AgentPlan{PlanAction::wait(duration)});
    }
    return plan;
}

}
