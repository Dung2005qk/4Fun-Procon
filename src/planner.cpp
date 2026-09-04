#include "udon/planner.hpp"
#include "udon/orienteering.hpp"
#include "udon/protocol.hpp"

#include <algorithm>
#include <atomic>
#include <array>
#include <bit>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
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
    for (const RoadStatus desired : {RoadStatus::Jammed, RoadStatus::Busy}) {
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

struct ExactOrienteeringBeamState {
    std::array<std::uint8_t, 16> spotCounts{};
    BrandMask brands;
    std::int32_t servings = 0;
    std::int32_t usedSteps = 0;
    std::array<std::int16_t, kMaximumAgents> routeByAgent{};
};

[[nodiscard]] BrandMask exact_orienteering_brand_mask(
    const MatchConfig& config,
    std::uint32_t spotMask) {
    BrandMask brands;
    for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
        if ((spotMask & (std::uint32_t{1} << spot)) != 0U &&
            config.spots.at(spot).stock > 0) {
            brands |= brand_bit(config.spots.at(spot).brandIndex);
        }
    }
    return brands;
}

[[nodiscard]] OfficialScore exact_orienteering_score(
    const MatchLedger& ledger,
    const ExactOrienteeringBeamState& state) {
    return OfficialScore{
        brand_count(ledger.lifetimeBrands | state.brands),
        ledger.totalDailyDistinct + brand_count(state.brands),
        ledger.totalServings + state.servings,
    };
}

[[nodiscard]] std::uint64_t exact_orienteering_count_key(
    const MatchConfig& config,
    const ExactOrienteeringBeamState& state) {
    std::uint64_t key = 0;
    for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
        key |= static_cast<std::uint64_t>(state.spotCounts.at(spot)) << (4U * spot);
    }
    return key;
}

[[nodiscard]] std::vector<const ExactOrienteeringRoute*>
select_coordinated_exact_orienteering_routes(
    const MatchConfig& config,
    const MatchLedger& ledger,
    const std::vector<ExactOrienteeringReachability>& reachability,
    std::optional<std::chrono::steady_clock::time_point> deadline,
    ColumnGenerationDiagnostics* diagnostics) {
    std::vector<const ExactOrienteeringRoute*> selected(
        static_cast<std::size_t>(config.agent_count()),
        nullptr);
    const auto deadline_expired = [&deadline, diagnostics]() {
        if (!deadline.has_value() ||
            std::chrono::steady_clock::now() < *deadline) {
            return false;
        }
        if (diagnostics != nullptr) {
            diagnostics->deadlineReached = true;
        }
        return true;
    };
    if (deadline_expired()) {
        return selected;
    }
    std::vector<AgentIndex> ordering;
    for (AgentIndex agent = 0; agent < config.agent_count(); ++agent) {
        const ExactOrienteeringReachability& exact =
            reachability.at(static_cast<std::size_t>(agent));
        if (!exact.maximalRoutes.empty()) {
            ordering.push_back(agent);
        }
    }
    if (ordering.empty()) {
        return selected;
    }
    std::sort(
        ordering.begin(),
        ordering.end(),
        [&reachability](AgentIndex left, AgentIndex right) {
            return std::pair{
                       reachability.at(static_cast<std::size_t>(left)).maximalRoutes.size(),
                       left} <
                std::pair{
                       reachability.at(static_cast<std::size_t>(right)).maximalRoutes.size(),
                       right};
        });
    if (deadline_expired()) {
        return selected;
    }

    std::vector<std::uint32_t> suffixSpots(ordering.size() + 1U, 0U);
    std::vector<std::array<std::uint8_t, 16>> suffixReachCount(ordering.size() + 1U);
    for (std::size_t depth = ordering.size(); depth-- > 0U;) {
        if (deadline_expired()) {
            return selected;
        }
        suffixSpots.at(depth) = suffixSpots.at(depth + 1U);
        suffixReachCount.at(depth) = suffixReachCount.at(depth + 1U);
        std::uint32_t agentSpots = 0;
        for (const ExactOrienteeringRoute& route :
             reachability.at(static_cast<std::size_t>(ordering.at(depth))).maximalRoutes) {
            agentSpots |= route.spotMask;
        }
        suffixSpots.at(depth) |= agentSpots;
        for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
            if ((agentSpots & (std::uint32_t{1} << spot)) != 0U) {
                ++suffixReachCount.at(depth).at(spot);
            }
        }
    }

    const auto apply_route = [&config](
                                 ExactOrienteeringBeamState state,
                                 AgentIndex agent,
                                 std::int16_t routeIndex,
                                 const ExactOrienteeringRoute& route) {
        state.routeByAgent.at(static_cast<std::size_t>(agent)) = routeIndex;
        state.usedSteps += route.usedSteps;
        for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
            if ((route.spotMask & (std::uint32_t{1} << spot)) == 0U) {
                continue;
            }
            const std::uint8_t capacity = static_cast<std::uint8_t>(std::clamp(
                config.spots.at(spot).stock,
                0,
                config.agent_count()));
            if (state.spotCounts.at(spot) < capacity) {
                ++state.spotCounts.at(spot);
                ++state.servings;
            }
        }
        state.brands |= exact_orienteering_brand_mask(config, route.spotMask);
        return state;
    };
    const auto optimistic_score = [&config,
                                   &ledger,
                                   &suffixSpots,
                                   &suffixReachCount](
                                      const ExactOrienteeringBeamState& state,
                                      std::size_t depth) {
        ExactOrienteeringBeamState optimistic = state;
        optimistic.brands |= exact_orienteering_brand_mask(
            config,
            suffixSpots.at(depth));
        for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
            const std::int32_t capacity = std::clamp(
                config.spots.at(spot).stock,
                0,
                config.agent_count());
            optimistic.servings += std::min<std::int32_t>(
                capacity - optimistic.spotCounts.at(spot),
                suffixReachCount.at(depth).at(spot));
        }
        return exact_orienteering_score(ledger, optimistic);
    };
    const auto select_routes = [&selected, &ordering, &reachability](
                                   const ExactOrienteeringBeamState& state) {
        std::vector<const ExactOrienteeringRoute*> result = selected;
        for (const AgentIndex agent : ordering) {
            const std::int16_t routeIndex =
                state.routeByAgent.at(static_cast<std::size_t>(agent));
            if (routeIndex >= 0) {
                result.at(static_cast<std::size_t>(agent)) =
                    &reachability.at(static_cast<std::size_t>(agent))
                         .maximalRoutes.at(static_cast<std::size_t>(routeIndex));
            }
        }
        return result;
    };

    ExactOrienteeringBeamState root;
    root.routeByAgent.fill(-1);
    std::vector<ExactOrienteeringBeamState> beam{root};
    constexpr std::size_t kExactTeamBeamWidth = 1U;
    for (std::size_t depth = 0; depth < ordering.size(); ++depth) {
        if (deadline_expired()) {
            return selected;
        }
        const AgentIndex agent = ordering.at(depth);
        const std::vector<ExactOrienteeringRoute>& routes =
            reachability.at(static_cast<std::size_t>(agent)).maximalRoutes;
        std::vector<ExactOrienteeringBeamState> expanded;
        expanded.reserve(beam.size() * routes.size());
        std::size_t expansionCount = 0U;
        for (const ExactOrienteeringBeamState& partial : beam) {
            for (std::size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex) {
                if ((expansionCount++ & 255U) == 0U && deadline_expired()) {
                    return selected;
                }
                expanded.push_back(apply_route(
                    partial,
                    agent,
                    static_cast<std::int16_t>(routeIndex),
                    routes.at(routeIndex)));
            }
        }
        if (deadline_expired()) {
            return selected;
        }
        const std::size_t nextDepth = depth + 1U;
        std::sort(
            expanded.begin(),
            expanded.end(),
            [&ledger, &optimistic_score, nextDepth](
                const ExactOrienteeringBeamState& left,
                const ExactOrienteeringBeamState& right) {
                const OfficialScore leftUpper = optimistic_score(left, nextDepth);
                const OfficialScore rightUpper = optimistic_score(right, nextDepth);
                if (leftUpper != rightUpper) {
                    return rightUpper < leftUpper;
                }
                const OfficialScore leftScore = exact_orienteering_score(ledger, left);
                const OfficialScore rightScore = exact_orienteering_score(ledger, right);
                if (leftScore != rightScore) {
                    return rightScore < leftScore;
                }
                if (left.usedSteps != right.usedSteps) {
                    return left.usedSteps < right.usedSteps;
                }
                return left.routeByAgent < right.routeByAgent;
            });
        if (deadline_expired()) {
            return selected;
        }
        std::unordered_set<std::uint64_t> seenCounts;
        std::vector<ExactOrienteeringBeamState> retained;
        retained.reserve(std::min(kExactTeamBeamWidth, expanded.size()));
        for (ExactOrienteeringBeamState& candidate : expanded) {
            if (!seenCounts.insert(exact_orienteering_count_key(config, candidate)).second) {
                continue;
            }
            retained.push_back(std::move(candidate));
            if (retained.size() >= kExactTeamBeamWidth) {
                break;
            }
        }
        beam = std::move(retained);
        if (beam.empty()) {
            return selected;
        }
    }
    std::sort(
        beam.begin(),
        beam.end(),
        [&ledger](
            const ExactOrienteeringBeamState& left,
            const ExactOrienteeringBeamState& right) {
            const OfficialScore leftScore = exact_orienteering_score(ledger, left);
            const OfficialScore rightScore = exact_orienteering_score(ledger, right);
            if (leftScore != rightScore) {
                return rightScore < leftScore;
            }
            if (left.usedSteps != right.usedSteps) {
                return left.usedSteps < right.usedSteps;
            }
            return left.routeByAgent < right.routeByAgent;
        });
    const auto better_complete = [&ledger](
                                     const ExactOrienteeringBeamState& left,
                                     const ExactOrienteeringBeamState& right) {
        const OfficialScore leftScore = exact_orienteering_score(ledger, left);
        const OfficialScore rightScore = exact_orienteering_score(ledger, right);
        if (leftScore != rightScore) {
            return rightScore < leftScore;
        }
        if (left.usedSteps != right.usedSteps) {
            return left.usedSteps < right.usedSteps;
        }
        return left.routeByAgent < right.routeByAgent;
    };
    const auto rebuild = [&ordering, &reachability, &apply_route](
                             const std::array<std::int16_t, kMaximumAgents>& choices,
                             AgentIndex skippedLeft,
                             AgentIndex skippedRight) {
        ExactOrienteeringBeamState rebuilt;
        rebuilt.routeByAgent.fill(-1);
        for (const AgentIndex agent : ordering) {
            if (agent == skippedLeft || agent == skippedRight) {
                continue;
            }
            const std::int16_t routeIndex =
                choices.at(static_cast<std::size_t>(agent));
            if (routeIndex < 0) {
                continue;
            }
            rebuilt = apply_route(
                rebuilt,
                agent,
                routeIndex,
                reachability.at(static_cast<std::size_t>(agent))
                    .maximalRoutes.at(static_cast<std::size_t>(routeIndex)));
        }
        return rebuilt;
    };
    ExactOrienteeringBeamState best = beam.front();
    if (diagnostics != nullptr) {
        diagnostics->exactOrienteeringSeedServings = best.servings;
    }
    ExactOrienteeringBeamState greedy;
    greedy.routeByAgent.fill(-1);
    for (const AgentIndex agent : ordering) {
        if (deadline_expired()) {
            return select_routes(best);
        }
        const std::vector<ExactOrienteeringRoute>& routes =
            reachability.at(static_cast<std::size_t>(agent)).maximalRoutes;
        std::optional<ExactOrienteeringBeamState> greedyChoice;
        for (std::size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex) {
            if ((routeIndex & 255U) == 0U && deadline_expired()) {
                return select_routes(best);
            }
            ExactOrienteeringBeamState candidate = apply_route(
                greedy,
                agent,
                static_cast<std::int16_t>(routeIndex),
                routes.at(routeIndex));
            if (!greedyChoice.has_value() ||
                exact_orienteering_score(ledger, *greedyChoice) <
                    exact_orienteering_score(ledger, candidate)) {
                greedyChoice = std::move(candidate);
            }
        }
        if (greedyChoice.has_value()) {
            greedy = std::move(*greedyChoice);
        }
    }
    if (better_complete(greedy, best)) {
        best = std::move(greedy);
    }
    for (std::int32_t round = 0; round < 3; ++round) {
        if (deadline_expired()) {
            return select_routes(best);
        }
        bool improved = false;
        for (const AgentIndex agent : ordering) {
            if (deadline_expired()) {
                return select_routes(best);
            }
            const ExactOrienteeringBeamState base = rebuild(
                best.routeByAgent,
                agent,
                kInvalidAgent);
            ExactOrienteeringBeamState localBest = best;
            const std::vector<ExactOrienteeringRoute>& routes =
                reachability.at(static_cast<std::size_t>(agent)).maximalRoutes;
            for (std::size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex) {
                if ((routeIndex & 255U) == 0U && deadline_expired()) {
                    return select_routes(best);
                }
                ExactOrienteeringBeamState candidate = apply_route(
                    base,
                    agent,
                    static_cast<std::int16_t>(routeIndex),
                    routes.at(routeIndex));
                if (better_complete(candidate, localBest)) {
                    localBest = std::move(candidate);
                }
            }
            if (better_complete(localBest, best)) {
                best = std::move(localBest);
                improved = true;
            }
        }
        std::size_t pairCombinationCount = 0U;
        for (std::size_t leftOffset = 0; leftOffset < ordering.size(); ++leftOffset) {
            if (deadline_expired()) {
                return select_routes(best);
            }
            const AgentIndex leftAgent = ordering.at(leftOffset);
            for (std::size_t rightOffset = leftOffset + 1U;
                 rightOffset < ordering.size();
                 ++rightOffset) {
                const AgentIndex rightAgent = ordering.at(rightOffset);
                const ExactOrienteeringBeamState base = rebuild(
                    best.routeByAgent,
                    leftAgent,
                    rightAgent);
                ExactOrienteeringBeamState localBest = best;
                const std::vector<ExactOrienteeringRoute>& leftRoutes =
                    reachability.at(static_cast<std::size_t>(leftAgent)).maximalRoutes;
                const std::vector<ExactOrienteeringRoute>& rightRoutes =
                    reachability.at(static_cast<std::size_t>(rightAgent)).maximalRoutes;
                for (std::size_t leftRoute = 0; leftRoute < leftRoutes.size(); ++leftRoute) {
                    const ExactOrienteeringBeamState withLeft = apply_route(
                        base,
                        leftAgent,
                        static_cast<std::int16_t>(leftRoute),
                        leftRoutes.at(leftRoute));
                    for (std::size_t rightRoute = 0;
                         rightRoute < rightRoutes.size();
                         ++rightRoute) {
                        if ((pairCombinationCount++ & 255U) == 0U &&
                            deadline_expired()) {
                            return select_routes(best);
                        }
                        ExactOrienteeringBeamState candidate = apply_route(
                            withLeft,
                            rightAgent,
                            static_cast<std::int16_t>(rightRoute),
                            rightRoutes.at(rightRoute));
                        if (better_complete(candidate, localBest)) {
                            localBest = std::move(candidate);
                        }
                    }
                }
                if (better_complete(localBest, best)) {
                    best = std::move(localBest);
                    improved = true;
                }
            }
        }
        if (!improved) {
            break;
        }
    }
    if (diagnostics != nullptr) {
        diagnostics->exactOrienteeringLocalServings = best.servings;
    }
    ExactOrienteeringBeamState absoluteUpperBound;
    const std::int32_t exactPatrolCount =
        static_cast<std::int32_t>(ordering.size());
    for (const Spot& spot : config.spots) {
        if (spot.stock <= 0) {
            continue;
        }
        absoluteUpperBound.brands |= brand_bit(spot.brandIndex);
        absoluteUpperBound.servings +=
            std::min(spot.stock, exactPatrolCount);
    }
    if (!(exact_orienteering_score(ledger, best) <
          exact_orienteering_score(ledger, absoluteUpperBound))) {
        return select_routes(best);
    }
    std::vector<std::vector<std::int16_t>> scarcityOrderedRoutes(
        static_cast<std::size_t>(config.agent_count()));
    const auto service_count = [&config](std::uint32_t mask) {
        std::int32_t servings = 0;
        for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
            if ((mask & (std::uint32_t{1} << spot)) != 0U &&
                config.spots.at(spot).stock > 0) {
                ++servings;
            }
        }
        return servings;
    };
    for (const AgentIndex agent : ordering) {
        if (deadline_expired()) {
            return select_routes(best);
        }
        const std::vector<ExactOrienteeringRoute>& routes =
            reachability.at(static_cast<std::size_t>(agent)).maximalRoutes;
        std::vector<std::int16_t>& routeOrder =
            scarcityOrderedRoutes.at(static_cast<std::size_t>(agent));
        routeOrder.reserve(routes.size());
        for (std::size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex) {
            routeOrder.push_back(static_cast<std::int16_t>(routeIndex));
        }
        std::sort(
            routeOrder.begin(),
            routeOrder.end(),
            [&config, &routes, &service_count](std::int16_t left, std::int16_t right) {
                const auto rank = [&config, &routes, &service_count](std::int16_t index) {
                    const std::uint32_t mask =
                        routes.at(static_cast<std::size_t>(index)).spotMask;
                    std::int32_t scarcity = 0;
                    for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
                        if ((mask & (std::uint32_t{1} << spot)) != 0U &&
                            config.spots.at(spot).stock > 0) {
                            scarcity += 10000 / config.spots.at(spot).stock;
                        }
                    }
                    return std::tuple{
                        service_count(mask),
                        -scarcity,
                        -routes.at(static_cast<std::size_t>(index)).usedSteps,
                        -static_cast<std::int32_t>(index)};
                };
                return rank(left) > rank(right);
            });
        if (deadline_expired()) {
            return select_routes(best);
        }
    }
    std::vector<std::int32_t> suffixMaximumServings(ordering.size() + 1U, 0);
    std::vector<BrandMask> suffixBrands(ordering.size() + 1U);
    for (std::size_t depth = ordering.size(); depth-- > 0U;) {
        if (deadline_expired()) {
            return select_routes(best);
        }
        suffixMaximumServings.at(depth) = suffixMaximumServings.at(depth + 1U);
        suffixBrands.at(depth) = suffixBrands.at(depth + 1U);
        const AgentIndex agent = ordering.at(depth);
        std::int32_t agentMaximum = 0;
        for (const ExactOrienteeringRoute& route :
             reachability.at(static_cast<std::size_t>(agent)).maximalRoutes) {
            agentMaximum = std::max(agentMaximum, service_count(route.spotMask));
            suffixBrands.at(depth) |= exact_orienteering_brand_mask(
                config,
                route.spotMask);
        }
        suffixMaximumServings.at(depth) += agentMaximum;
    }
    std::array<std::uint8_t, 16> feasibilityCounts{};
    std::array<std::int16_t, kMaximumAgents> feasibilityChoices{};
    feasibilityChoices.fill(-1);
    std::array<std::int16_t, kMaximumAgents> improvedChoices{};
    improvedChoices.fill(-1);
    OfficialScore feasibilityIncumbentScore = exact_orienteering_score(ledger, best);
    std::vector<std::unordered_set<std::uint64_t>> feasibilityMemo(
        ordering.size() + 1U);
    const auto feasibility_count_key = [&config, &feasibilityCounts]() {
        std::uint64_t key = 0U;
        for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
            key |= static_cast<std::uint64_t>(feasibilityCounts.at(spot)) <<
                (4U * spot);
        }
        return key;
    };
    std::uint64_t feasibilityNodes = 0;
    std::uint64_t overlapFeasibilityNodes = 0;
    std::int32_t feasibilityImprovements = 0;
    std::uint64_t feasibilityPhaseNodeEnd = 0U;
    bool feasibilityDeadlineReached = false;
    constexpr std::uint64_t kStrictFeasibilityNodes = 3000000U;
    constexpr std::uint64_t kOverlapFeasibilityNodes = 500000U;
    constexpr std::uint64_t kMaximumFeasibilityNodes =
        kStrictFeasibilityNodes + kOverlapFeasibilityNodes;
    constexpr std::int32_t kMaximumFeasibilityImprovements = 2;
    const auto capacity_feasibility_search =
        [&](auto&& self,
            std::size_t depth,
            std::int32_t servings,
            BrandMask brands,
            bool allowSaturatedOverlap) -> bool {
            if (feasibilityDeadlineReached) {
                return false;
            }
            ++feasibilityNodes;
            if (feasibilityNodes > feasibilityPhaseNodeEnd) {
                return false;
            }
            if ((feasibilityNodes & 1023U) == 0U && deadline_expired()) {
                feasibilityDeadlineReached = true;
                return false;
            }
            ExactOrienteeringBeamState optimistic;
            optimistic.brands = brands | suffixBrands.at(depth);
            optimistic.servings = servings + suffixMaximumServings.at(depth);
            if (allowSaturatedOverlap) {
                optimistic.servings = servings;
                for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
                    const std::int32_t capacity = std::clamp(
                        config.spots.at(spot).stock,
                        0,
                        config.agent_count());
                    optimistic.servings += std::min<std::int32_t>(
                        capacity - feasibilityCounts.at(spot),
                        suffixReachCount.at(depth).at(spot));
                }
            }
            if (!(feasibilityIncumbentScore < exact_orienteering_score(ledger, optimistic))) {
                return false;
            }
            if (!feasibilityMemo.at(depth).insert(feasibility_count_key()).second) {
                return false;
            }
            if (depth == ordering.size()) {
                improvedChoices = feasibilityChoices;
                return true;
            }
            const AgentIndex agent = ordering.at(depth);
            const std::vector<ExactOrienteeringRoute>& routes =
                reachability.at(static_cast<std::size_t>(agent)).maximalRoutes;
            for (const std::int16_t routeIndex :
                 scarcityOrderedRoutes.at(static_cast<std::size_t>(agent))) {
                if (feasibilityDeadlineReached) {
                    return false;
                }
                const ExactOrienteeringRoute& route =
                    routes.at(static_cast<std::size_t>(routeIndex));
                std::uint32_t addedSpots = 0U;
                bool strictFeasible = true;
                for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
                    if ((route.spotMask & (std::uint32_t{1} << spot)) != 0U &&
                        config.spots.at(spot).stock > 0) {
                        const std::uint8_t capacity = static_cast<std::uint8_t>(
                            std::min(
                                config.spots.at(spot).stock,
                                config.agent_count()));
                        if (feasibilityCounts.at(spot) >= capacity) {
                            strictFeasible = allowSaturatedOverlap;
                            if (!allowSaturatedOverlap) {
                                break;
                            }
                            continue;
                        }
                        ++feasibilityCounts.at(spot);
                        addedSpots |= std::uint32_t{1} << spot;
                    }
                }
                if (!strictFeasible) {
                    for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
                        if ((addedSpots & (std::uint32_t{1} << spot)) != 0U) {
                            --feasibilityCounts.at(spot);
                        }
                    }
                    continue;
                }
                feasibilityChoices.at(static_cast<std::size_t>(agent)) = routeIndex;
                if (self(
                        self,
                        depth + 1U,
                        servings + static_cast<std::int32_t>(std::popcount(addedSpots)),
                        brands | exact_orienteering_brand_mask(config, addedSpots),
                        allowSaturatedOverlap)) {
                    return true;
                }
                for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
                    if ((addedSpots & (std::uint32_t{1} << spot)) != 0U) {
                        --feasibilityCounts.at(spot);
                    }
                }
                if (feasibilityDeadlineReached) {
                    return false;
                }
            }
            return false;
        };
    while (feasibilityNodes < kMaximumFeasibilityNodes &&
           feasibilityImprovements < kMaximumFeasibilityImprovements &&
           !feasibilityDeadlineReached &&
           (!deadline.has_value() || std::chrono::steady_clock::now() < *deadline)) {
        feasibilityCounts.fill(0U);
        feasibilityChoices.fill(-1);
        improvedChoices.fill(-1);
        for (std::unordered_set<std::uint64_t>& memo : feasibilityMemo) {
            memo.clear();
        }
        feasibilityPhaseNodeEnd = std::min(
            kMaximumFeasibilityNodes,
            feasibilityNodes + kStrictFeasibilityNodes);
        bool foundImprovement = capacity_feasibility_search(
            capacity_feasibility_search,
            0U,
            0,
            0U,
            false);
        if (feasibilityDeadlineReached) {
            break;
        }
        bool overlapFoundImprovement = false;
        if (!foundImprovement &&
            !feasibilityDeadlineReached &&
            feasibilityNodes < kMaximumFeasibilityNodes &&
            (!deadline.has_value() || std::chrono::steady_clock::now() < *deadline)) {
            feasibilityCounts.fill(0U);
            feasibilityChoices.fill(-1);
            improvedChoices.fill(-1);
            for (std::unordered_set<std::uint64_t>& memo : feasibilityMemo) {
                memo.clear();
            }
            feasibilityPhaseNodeEnd = std::min(
                kMaximumFeasibilityNodes,
                feasibilityNodes + kOverlapFeasibilityNodes);
            const std::uint64_t overlapStartedAt = feasibilityNodes;
            foundImprovement = capacity_feasibility_search(
                capacity_feasibility_search,
                0U,
                0,
                0U,
                true);
            overlapFeasibilityNodes += feasibilityNodes - overlapStartedAt;
            overlapFoundImprovement = foundImprovement;
            if (feasibilityDeadlineReached) {
                break;
            }
        }
        if (!foundImprovement) {
            break;
        }
        ExactOrienteeringBeamState improved;
        improved.routeByAgent.fill(-1);
        for (const AgentIndex agent : ordering) {
            const std::int16_t routeIndex =
                improvedChoices.at(static_cast<std::size_t>(agent));
            improved = apply_route(
                improved,
                agent,
                routeIndex,
                reachability.at(static_cast<std::size_t>(agent))
                    .maximalRoutes.at(static_cast<std::size_t>(routeIndex)));
        }
        if (better_complete(improved, best)) {
            best = std::move(improved);
            feasibilityIncumbentScore = exact_orienteering_score(ledger, best);
            ++feasibilityImprovements;
            if (diagnostics != nullptr) {
                diagnostics->exactOrienteeringFeasibilityImproved = true;
                diagnostics->exactOrienteeringOverlapFeasibilityImproved =
                    diagnostics->exactOrienteeringOverlapFeasibilityImproved ||
                    overlapFoundImprovement;
            }
            if (feasibilityNodes >= 500000U) {
                break;
            }
        } else {
            break;
        }
    }
    if (diagnostics != nullptr) {
        diagnostics->exactOrienteeringFeasibilityNodes = feasibilityNodes;
        diagnostics->exactOrienteeringOverlapFeasibilityNodes =
            overlapFeasibilityNodes;
        diagnostics->exactOrienteeringFeasibilityImprovements =
            feasibilityImprovements;
    }
    return select_routes(best);
}

struct ExactBundleResourceProfile {
    std::vector<const ExactOrienteeringRoute*> routes;
    std::int32_t patrolFuelUsed = 0;
    std::int32_t terminalOnSpot = 0;
    std::int32_t totalTerminalBrandDistance = 0;
    std::int32_t worstTerminalBrandDistance = 0;
    std::int32_t usedSteps = 0;
    std::string terminalSignature;
};

[[nodiscard]] ExactOrienteeringBeamState summarize_exact_bundle(
    const MatchConfig& config,
    const std::vector<const ExactOrienteeringRoute*>& routes) {
    ExactOrienteeringBeamState result;
    result.routeByAgent.fill(-1);
    for (const ExactOrienteeringRoute* route : routes) {
        if (route == nullptr) {
            continue;
        }
        result.usedSteps += route->usedSteps;
        for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
            if ((route->spotMask & (std::uint32_t{1} << spot)) == 0U) {
                continue;
            }
            const std::uint8_t capacity = static_cast<std::uint8_t>(std::clamp(
                config.spots.at(spot).stock,
                0,
                config.agent_count()));
            if (result.spotCounts.at(spot) < capacity) {
                ++result.spotCounts.at(spot);
                ++result.servings;
            }
        }
        result.brands |= exact_orienteering_brand_mask(config, route->spotMask);
    }
    return result;
}

[[nodiscard]] ExactBundleResourceProfile profile_exact_bundle(
    const DayState& state,
    std::vector<const ExactOrienteeringRoute*> routes) {
    ExactBundleResourceProfile profile;
    profile.routes = std::move(routes);
    std::vector<std::tuple<std::uint8_t, CellId, std::int32_t>> terminalState;
    terminalState.reserve(profile.routes.size());
    for (std::size_t agent = 0; agent < profile.routes.size(); ++agent) {
        const ExactOrienteeringRoute* route = profile.routes.at(agent);
        if (route == nullptr) {
            continue;
        }
        profile.patrolFuelUsed += route->patrolFuel;
        profile.terminalOnSpot += route->terminalOnSpot ? 1 : 0;
        profile.totalTerminalBrandDistance += route->terminalBrandDistance;
        profile.worstTerminalBrandDistance = std::max(
            profile.worstTerminalBrandDistance,
            route->terminalBrandDistance);
        profile.usedSteps += route->usedSteps;
        terminalState.emplace_back(
            static_cast<std::uint8_t>(state.agents.at(agent).kind),
            route->terminalCell,
            std::max(0, state.agents.at(agent).fuel - route->patrolFuel));
    }
    std::sort(terminalState.begin(), terminalState.end());
    std::ostringstream signature;
    for (const auto& [kind, cell, fuel] : terminalState) {
        signature << static_cast<std::int32_t>(kind) << ':' << cell << ':' << fuel << ';';
    }
    profile.terminalSignature = signature.str();
    return profile;
}

[[nodiscard]] bool exact_bundle_resource_dominates(
    const ExactBundleResourceProfile& left,
    const ExactBundleResourceProfile& right) {
    const bool noWorse =
        left.patrolFuelUsed <= right.patrolFuelUsed &&
        left.terminalOnSpot >= right.terminalOnSpot &&
        left.totalTerminalBrandDistance <= right.totalTerminalBrandDistance &&
        left.worstTerminalBrandDistance <= right.worstTerminalBrandDistance;
    const bool strict =
        left.patrolFuelUsed < right.patrolFuelUsed ||
        left.terminalOnSpot > right.terminalOnSpot ||
        left.totalTerminalBrandDistance < right.totalTerminalBrandDistance ||
        left.worstTerminalBrandDistance < right.worstTerminalBrandDistance;
    return noWorse && strict;
}

[[nodiscard]] std::vector<std::vector<const ExactOrienteeringRoute*>>
select_coordinated_exact_orienteering_frontier_routes(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    const std::vector<std::vector<const ExactOrienteeringRoute*>>& canonicalBundles,
    std::int32_t maximumAdditionalBundles,
    std::optional<std::chrono::steady_clock::time_point> deadline,
    ColumnGenerationDiagnostics* diagnostics) {
    std::vector<std::vector<const ExactOrienteeringRoute*>> result;
    if (maximumAdditionalBundles <= 0 || canonicalBundles.empty() ||
        !std::all_of(
            canonicalBundles.front().begin(),
            canonicalBundles.front().end(),
            [](const auto* route) { return route != nullptr; })) {
        return result;
    }
    const auto deadline_expired = [&deadline, diagnostics]() {
        if (!deadline.has_value() || std::chrono::steady_clock::now() < *deadline) {
            return false;
        }
        if (diagnostics != nullptr) {
            diagnostics->deadlineReached = true;
        }
        return true;
    };
    const ExactOrienteeringBeamState primaryState =
        summarize_exact_bundle(config, canonicalBundles.front());
    const OfficialScore primaryScore = exact_orienteering_score(ledger, primaryState);
    std::vector<ExactBundleResourceProfile> candidates;
    std::set<std::string> seenSignatures;
    std::set<std::string> seenPlans;
    const auto plan_key = [&config, &state](
                              const std::vector<const ExactOrienteeringRoute*>& routes) {
        DayPlan plan;
        plan.actions.reserve(routes.size());
        for (const ExactOrienteeringRoute* route : routes) {
            plan.actions.push_back(
                route != nullptr ? route->actions : wait_actions(config, state));
        }
        return canonical_plan_bytes(plan);
    };
    std::vector<std::vector<const ExactOrienteeringRoute*>> completeCanonical;
    for (const auto& bundle : canonicalBundles) {
        seenPlans.insert(plan_key(bundle));
        if (!std::all_of(bundle.begin(), bundle.end(), [](const auto* route) {
                return route != nullptr;
            })) {
            continue;
        }
        ExactBundleResourceProfile profile = profile_exact_bundle(state, bundle);
        seenSignatures.insert(profile.terminalSignature);
        candidates.push_back(std::move(profile));
        completeCanonical.push_back(bundle);
    }
    const std::size_t canonicalCandidateCount = candidates.size();
    std::size_t considered = 0U;
    for (const auto& base : completeCanonical) {
        for (const auto& donor : completeCanonical) {
            for (std::size_t agent = 0; agent < base.size(); ++agent) {
                if ((considered++ & 255U) == 0U && deadline_expired()) {
                    break;
                }
                if (base.at(agent) == donor.at(agent) ||
                    same_agent_plan(base.at(agent)->actions, donor.at(agent)->actions)) {
                    continue;
                }
                std::vector<const ExactOrienteeringRoute*> alternative = base;
                alternative.at(agent) = donor.at(agent);
                if (exact_orienteering_score(
                        ledger,
                        summarize_exact_bundle(config, alternative)) != primaryScore) {
                    continue;
                }
                if (!seenPlans.insert(plan_key(alternative)).second) {
                    continue;
                }
                ExactBundleResourceProfile profile =
                    profile_exact_bundle(state, std::move(alternative));
                if (!seenSignatures.insert(profile.terminalSignature).second) {
                    continue;
                }
                candidates.push_back(std::move(profile));
            }
        }
    }
    if (diagnostics != nullptr) {
        diagnostics->exactOrienteeringFrontierCandidates +=
            static_cast<std::int32_t>(candidates.size() - canonicalCandidateCount);
    }
    std::vector<std::size_t> frontier;
    for (std::size_t candidate = canonicalCandidateCount;
         candidate < candidates.size();
         ++candidate) {
        bool dominated = false;
        for (std::size_t other = canonicalCandidateCount;
             other < candidates.size();
             ++other) {
            if (candidate != other &&
                exact_bundle_resource_dominates(candidates.at(other), candidates.at(candidate))) {
                dominated = true;
                break;
            }
        }
        if (!dominated) {
            frontier.push_back(candidate);
        }
    }
    const auto select_best = [&candidates, &frontier, &result](const auto& rank) {
        std::optional<std::size_t> best;
        for (const std::size_t candidate : frontier) {
            const bool alreadySelected = std::any_of(
                result.begin(),
                result.end(),
                [&candidates, candidate](const auto& routes) {
                    return routes == candidates.at(candidate).routes;
                });
            if (!alreadySelected &&
                (!best.has_value() || rank(candidates.at(candidate)) < rank(candidates.at(*best)))) {
                best = candidate;
            }
        }
        if (best.has_value()) {
            result.push_back(candidates.at(*best).routes);
        }
    };
    while (result.size() < static_cast<std::size_t>(maximumAdditionalBundles)) {
        const std::size_t before = result.size();
        select_best([](const ExactBundleResourceProfile& profile) {
            return std::tuple{
                profile.patrolFuelUsed,
                -profile.terminalOnSpot,
                profile.worstTerminalBrandDistance,
                profile.totalTerminalBrandDistance,
                profile.usedSteps,
                profile.terminalSignature};
        });
        if (result.size() >= static_cast<std::size_t>(maximumAdditionalBundles)) {
            break;
        }
        select_best([](const ExactBundleResourceProfile& profile) {
            return std::tuple{
                -profile.terminalOnSpot,
                profile.patrolFuelUsed,
                profile.worstTerminalBrandDistance,
                profile.totalTerminalBrandDistance,
                profile.usedSteps,
                profile.terminalSignature};
        });
        if (result.size() >= static_cast<std::size_t>(maximumAdditionalBundles)) {
            break;
        }
        select_best([](const ExactBundleResourceProfile& profile) {
            return std::tuple{
                profile.worstTerminalBrandDistance,
                profile.totalTerminalBrandDistance,
                profile.patrolFuelUsed,
                -profile.terminalOnSpot,
                profile.usedSteps,
                profile.terminalSignature};
        });
        if (result.size() == before) {
            break;
        }
    }
    if (diagnostics != nullptr) {
        diagnostics->exactOrienteeringFrontierBundles +=
            static_cast<std::int32_t>(result.size());
    }
    return result;
}

enum class ExactTerminalObjective : std::uint8_t {
    Fuel,
    BrandAccess,
    TankerAccess,
};

[[nodiscard]] std::vector<const ExactOrienteeringRoute*>
select_exact_terminal_variant(
    const MatchConfig& config,
    const DayState& state,
    const std::vector<const ExactOrienteeringRoute*>& base,
    const std::vector<ExactOrienteeringReachability>& reachability,
    ExactTerminalObjective objective) {
    std::vector<const ExactOrienteeringRoute*> variant = base;
    for (std::size_t agent = 0; agent < base.size(); ++agent) {
        const ExactOrienteeringRoute* selected = base.at(agent);
        if (selected == nullptr) {
            continue;
        }
        const auto tanker_distance = [&config, &state](CellId terminalCell) {
            std::int32_t distance = std::numeric_limits<std::int32_t>::max();
            for (const AgentState& candidate : state.agents) {
                if (candidate.kind == AgentKind::Tanker) {
                    distance = std::min(
                        distance,
                        config.map.hex_distance(terminalCell, candidate.position));
                }
            }
            return distance;
        };
        const auto rank = [objective, &tanker_distance](const ExactOrienteeringRoute& route) {
            if (objective == ExactTerminalObjective::Fuel) {
                return std::tuple{
                    route.patrolFuel,
                    route.terminalOnSpot ? 0 : 1,
                    route.terminalBrandDistance,
                    route.usedSteps,
                    route.terminalCell};
            }
            if (objective == ExactTerminalObjective::TankerAccess) {
                return std::tuple{
                    tanker_distance(route.terminalCell),
                    route.patrolFuel,
                    route.terminalBrandDistance,
                    route.usedSteps,
                    route.terminalCell};
            }
            return std::tuple{
                route.terminalOnSpot ? 0 : 1,
                route.terminalBrandDistance,
                route.patrolFuel,
                route.usedSteps,
                route.terminalCell};
        };
        const auto consider = [&](const std::vector<ExactOrienteeringRoute>& routes) {
            for (const ExactOrienteeringRoute& candidate : routes) {
                if (candidate.spotMask == selected->spotMask &&
                    rank(candidate) < rank(*variant.at(agent))) {
                    variant.at(agent) = &candidate;
                }
            }
        };
        consider(reachability.at(agent).maximalRoutes);
        consider(reachability.at(agent).terminalVariants);
    }
    return variant;
}

void prune_columns(
    std::vector<RouteColumn>& columns,
    std::int32_t maximumColumns,
    bool retainSpotDiversity) {
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
            const std::string leftKey = actions_key(
                left.actions,
                left.escortGroup,
                left.contingencyBundle,
                left.requiredRefuels);
            const std::string rightKey = actions_key(
                right.actions,
                right.escortGroup,
                right.contingencyBundle,
                right.requiredRefuels);
            if (leftKey != rightKey) {
                return leftKey < rightKey;
            }
            if (left.harvestExtensionSourceRank !=
                right.harvestExtensionSourceRank) {
                return left.harvestExtensionSourceRank <
                    right.harvestExtensionSourceRank;
            }
            return left.columnId < right.columnId;
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

        BrandMask coveredBrands;
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
                const std::int32_t marginal =
                    brand_difference_count(column.estimatedBrands, coveredBrands);
                if (marginal == 0) {
                    continue;
                }
                const bool columnIndependent = column.escortGroup < 0 && column.contingencyBundle < 0;
                const bool bestIndependent = best != nullptr && best->escortGroup < 0 && best->contingencyBundle < 0;
                const std::int32_t columnBreadth = brand_count(column.estimatedBrands);
                const std::int32_t bestBreadth = best == nullptr
                    ? -1
                    : brand_count(best->estimatedBrands);
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
        if (retainSpotDiversity) {
            SpotIndex maximumSpot = -1;
            for (const RouteColumn& column : unique) {
                for (const ColumnVisitEvent& event : column.firstVisits) {
                    maximumSpot = std::max(maximumSpot, event.spot);
                }
            }
            std::vector<bool> coveredSpots(
                maximumSpot >= 0
                    ? static_cast<std::size_t>(maximumSpot) + 1U
                    : 0U,
                false);
            const auto mark_claimed_spots = [&coveredSpots](const RouteColumn& column) {
                for (const ColumnVisitEvent& event : column.firstVisits) {
                    if (event.claimedServing) {
                        coveredSpots.at(static_cast<std::size_t>(event.spot)) = true;
                    }
                }
            };
            for (const RouteColumn& column : unique) {
                if (std::find(
                        retainedIds.begin(),
                        retainedIds.end(),
                        column.columnId) != retainedIds.end()) {
                    mark_claimed_spots(column);
                }
            }
            const std::int32_t spotDiversityLimit = std::max(1, maximumColumns / 4);
            for (std::int32_t diversityPick = 0;
                 diversityPick < spotDiversityLimit &&
                     static_cast<std::int32_t>(retainedIds.size()) < maximumColumns;
                 ++diversityPick) {
                const RouteColumn* best = nullptr;
                std::int32_t bestMarginal = 0;
                for (const RouteColumn& column : unique) {
                    if (std::find(
                            retainedIds.begin(),
                            retainedIds.end(),
                            column.columnId) != retainedIds.end()) {
                        continue;
                    }
                    std::int32_t marginal = 0;
                    for (const ColumnVisitEvent& event : column.firstVisits) {
                        if (event.claimedServing &&
                            !coveredSpots.at(static_cast<std::size_t>(event.spot))) {
                            ++marginal;
                        }
                    }
                    const std::int32_t columnBreadth = brand_count(column.estimatedBrands);
                    const std::int32_t bestBreadth = best == nullptr
                        ? -1
                        : brand_count(best->estimatedBrands);
                    if (marginal > bestMarginal ||
                        (marginal == bestMarginal && marginal > 0 &&
                         column.estimatedServings > best->estimatedServings) ||
                        (marginal == bestMarginal && marginal > 0 &&
                         column.estimatedServings == best->estimatedServings &&
                         columnBreadth > bestBreadth) ||
                        (marginal == bestMarginal && marginal > 0 &&
                         column.estimatedServings == best->estimatedServings &&
                         columnBreadth == bestBreadth &&
                         column.priority > best->priority)) {
                        best = &column;
                        bestMarginal = marginal;
                    }
                }
                if (best == nullptr || bestMarginal <= 0) {
                    break;
                }
                retain(best->columnId);
                mark_claimed_spots(*best);
            }
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

struct SynchronizationAvailability {
    std::vector<std::uint32_t> escortAgentMasks;
    std::vector<std::uint32_t> escortTankerMasks;
    std::unordered_map<std::uint64_t, std::uint32_t> refuelProviderMasks;
};

[[nodiscard]] std::uint64_t refuel_event_key(const RefuelEvent& event) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(event.step)) << 32U) |
        static_cast<std::uint32_t>(event.cell);
}

[[nodiscard]] bool partial_synchronized_selection_is_feasible(
    const DayState& state,
    const std::vector<const RouteColumn*>& selected,
    const SynchronizationAvailability& availability) {
    struct ActiveGroup {
        std::int32_t groupId = -1;
        std::int32_t selectedMembers = 0;
        std::int32_t selectedTankers = 0;
    };
    std::array<ActiveGroup, static_cast<std::size_t>(kMaximumAgents)> activeGroups{};
    std::size_t activeGroupCount = 0;
    std::uint32_t assignedAgentMask = 0;
    for (AgentIndex agentIndex = 0; agentIndex < static_cast<AgentIndex>(selected.size()); ++agentIndex) {
        const RouteColumn* column = selected.at(static_cast<std::size_t>(agentIndex));
        if (column == nullptr) {
            continue;
        }
        assignedAgentMask |=
            std::uint32_t{1} << static_cast<std::uint32_t>(agentIndex);
        if (column->escortGroup < 0) {
            continue;
        }
        auto groupIterator = std::find_if(
            activeGroups.begin(),
            activeGroups.begin() + static_cast<std::ptrdiff_t>(activeGroupCount),
            [column](const ActiveGroup& group) { return group.groupId == column->escortGroup; });
        if (groupIterator == activeGroups.begin() + static_cast<std::ptrdiff_t>(activeGroupCount)) {
            groupIterator = activeGroups.begin() + static_cast<std::ptrdiff_t>(activeGroupCount);
            *groupIterator = ActiveGroup{column->escortGroup};
            ++activeGroupCount;
        }
        ActiveGroup& group = *groupIterator;
        ++group.selectedMembers;
        group.selectedTankers +=
            state.agents.at(static_cast<std::size_t>(agentIndex)).kind == AgentKind::Tanker ? 1 : 0;
    }
    for (std::size_t groupIndex = 0; groupIndex < activeGroupCount; ++groupIndex) {
        const ActiveGroup& active = activeGroups.at(groupIndex);
        if (active.selectedTankers > 1) {
            return false;
        }
        const std::size_t groupOffset = static_cast<std::size_t>(active.groupId);
        const std::uint32_t unassignedMembers = groupOffset <
                availability.escortAgentMasks.size()
            ? availability.escortAgentMasks.at(groupOffset) & ~assignedAgentMask
            : 0U;
        const std::int32_t possibleMembers = active.selectedMembers +
            static_cast<std::int32_t>(std::popcount(unassignedMembers));
        const bool tankerPossible = active.selectedTankers == 1 ||
            (groupOffset < availability.escortTankerMasks.size() &&
             (availability.escortTankerMasks.at(groupOffset) & ~assignedAgentMask) != 0U);
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
            const auto availableProviders = availability.refuelProviderMasks.find(
                refuel_event_key(required));
            bool coveragePossible = availableProviders !=
                    availability.refuelProviderMasks.end() &&
                (availableProviders->second & ~assignedAgentMask) != 0U;
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
                }
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
    const BrandMask collectedBrands = ledger.lifetimeBrands | result.score.brands;
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
    const BrandMask collectedBrands = ledger.lifetimeBrands | column.estimatedBrands;
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

using CandidateDiversityDistance =
    std::tuple<std::int32_t, std::int64_t, std::int32_t>;

[[nodiscard]] CandidateDiversityDistance candidate_diversity_distance(
    const MasterCandidate& left,
    const MasterCandidate& right) {
    const std::size_t agentCount = std::min(
        left.simulation.finalAgents.size(),
        right.simulation.finalAgents.size());
    std::int32_t differentTerminalPositions =
        static_cast<std::int32_t>(
            std::max(
                left.simulation.finalAgents.size(),
                right.simulation.finalAgents.size()) -
            agentCount);
    std::int64_t stateDistance = 0;
    for (std::size_t agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        const AgentState& leftAgent =
            left.simulation.finalAgents.at(agentIndex);
        const AgentState& rightAgent =
            right.simulation.finalAgents.at(agentIndex);
        differentTerminalPositions +=
            leftAgent.position != rightAgent.position ? 1 : 0;
        stateDistance += std::abs(leftAgent.fuel - rightAgent.fuel);
    }
    const std::size_t footprintCount = std::min(
        left.simulation.roadFootprint.size(),
        right.simulation.roadFootprint.size());
    for (std::size_t roadIndex = 0;
         roadIndex < footprintCount;
         ++roadIndex) {
        stateDistance += std::abs(
            left.simulation.roadFootprint.at(roadIndex) -
            right.simulation.roadFootprint.at(roadIndex));
    }
    return CandidateDiversityDistance{
        differentTerminalPositions,
        stateDistance,
        candidate_plan_distance(left, right),
    };
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
        std::optional<CandidateDiversityDistance> bestMinimumDistance;
        for (std::size_t candidateIndex = qualityCount;
             candidateIndex < candidates.size();
             ++candidateIndex) {
            if (used.at(candidateIndex)) {
                continue;
            }
            std::optional<CandidateDiversityDistance> minimumDistance;
            for (const std::size_t selectedIndex : selected) {
                const CandidateDiversityDistance distance =
                    candidate_diversity_distance(
                        candidates.at(candidateIndex),
                        candidates.at(selectedIndex));
                if (!minimumDistance.has_value() ||
                    distance < *minimumDistance) {
                    minimumDistance = distance;
                }
            }
            if (minimumDistance.has_value() &&
                (!bestMinimumDistance.has_value() ||
                 *minimumDistance > *bestMinimumDistance)) {
                bestMinimumDistance = *minimumDistance;
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

[[nodiscard]] BrandMask column_brand_mask(
    const MatchConfig& config,
    const RouteColumn& column) {
    BrandMask result;
    for (const ColumnVisitEvent& event : column.firstVisits) {
        result |= brand_bit(config.spots.at(static_cast<std::size_t>(event.spot)).brandIndex);
    }
    return result;
}

[[nodiscard]] std::int32_t marginal_stock_credits(
    const MatchConfig& config,
    const std::vector<const RouteColumn*>& selected,
    const RouteColumn& candidate) {
    std::int32_t marginalCredits = 0;
    for (const ColumnVisitEvent& event : candidate.firstVisits) {
        if (!event.claimedServing) {
            continue;
        }
        std::int32_t selectedClaims = 0;
        for (const RouteColumn* selectedColumn : selected) {
            if (selectedColumn == nullptr) {
                continue;
            }
            selectedClaims += static_cast<std::int32_t>(std::count_if(
                selectedColumn->firstVisits.begin(),
                selectedColumn->firstVisits.end(),
                [&event](const ColumnVisitEvent& selectedEvent) {
                    return selectedEvent.claimedServing && selectedEvent.spot == event.spot;
                }));
        }
        if (selectedClaims < config.spots.at(static_cast<std::size_t>(event.spot)).stock) {
            ++marginalCredits;
        }
    }
    return marginalCredits;
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
    BrandMask dailyBrands;
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
        BrandMask possibleBrands;
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
        brand_count(ledger.lifetimeBrands | dailyBrands),
        ledger.totalDailyDistinct + brand_count(dailyBrands),
        ledger.totalServings + servings,
    };
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
    const ColumnGenerationOptions& options,
    ColumnGenerationDiagnostics* diagnostics) const {
    if (static_cast<std::int32_t>(state.agents.size()) != config_.agent_count()) {
        throw std::invalid_argument("column generation requires a complete day state");
    }
    if (diagnostics != nullptr) {
        *diagnostics = ColumnGenerationDiagnostics{};
        diagnostics->agentMilliseconds.assign(
            static_cast<std::size_t>(config_.agent_count()),
            0);
        diagnostics->agentParetoQueries.assign(
            static_cast<std::size_t>(config_.agent_count()),
            0);
    }
    RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(static_cast<std::size_t>(config_.agent_count()));
    std::int32_t nextColumnId = 0;
    std::int32_t nextEscortGroup = 0;
    const std::vector<CellId> criticalRoads = select_critical_roads(config_, state, options.criticalRoadHints);
    if (diagnostics != nullptr) {
        diagnostics->criticalRoads = criticalRoads;
    }
    const auto deadline_expired = [&options]() {
        return options.deadline.has_value() && std::chrono::steady_clock::now() >= *options.deadline;
    };
    std::vector<ExactOrienteeringReachability> exactOrienteering(
        static_cast<std::size_t>(config_.agent_count()));
    std::vector<ExactOrienteeringReachability>
        enhancedExactOrienteering;
    std::vector<std::vector<const ExactOrienteeringRoute*>>
        coordinatedExactRouteBundles;
    std::vector<std::jthread> exactResourceWorkers;
    std::vector<AgentIndex> exactResourceTasks;
    std::vector<std::pair<AgentIndex, AgentIndex>> exactResourceAliases;
    std::atomic<std::size_t> nextExactResourceTask{0U};
    std::chrono::steady_clock::time_point exactStarted{};
    bool exactDeferred = false;
    bool exactFinalized = false;
    const auto finalize_exact_orienteering = [&]() {
        if (exactFinalized) {
            return;
        }
        exactFinalized = true;
        const std::chrono::steady_clock::time_point finalizationStarted =
            std::chrono::steady_clock::now();
        if (diagnostics != nullptr) {
            diagnostics->exactOrienteeringEnumerationMilliseconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    finalizationStarted - exactStarted)
                    .count();
        }
        for (const auto& [agent, representative] : exactResourceAliases) {
            exactOrienteering.at(static_cast<std::size_t>(agent)) =
                exactOrienteering.at(static_cast<std::size_t>(representative));
        }
        if (diagnostics != nullptr) {
            diagnostics->exactOrienteeringCacheHits +=
                static_cast<std::int32_t>(exactResourceAliases.size());
            for (const ExactOrienteeringReachability& exact : exactOrienteering) {
                diagnostics->exactOrienteeringSupportedAgents += exact.supported ? 1 : 0;
                diagnostics->exactOrienteeringCompleteAgents += exact.complete ? 1 : 0;
                diagnostics->exactOrienteeringSettledStates += exact.settledStates;
                diagnostics->exactOrienteeringTerminalVariants +=
                    exact.terminalVariants.size();
            }
        }
        const auto append_exact_bundles =
            [&](const std::vector<ExactOrienteeringReachability>&
                    reachability) {
                if (deadline_expired()) {
                    if (diagnostics != nullptr) {
                        diagnostics->deadlineReached = true;
                    }
                    return;
                }
                std::vector<const ExactOrienteeringRoute*> routes =
                    select_coordinated_exact_orienteering_routes(
                        config_,
                        ledger,
                        reachability,
                        options.deadline,
                        diagnostics);
                if (!std::any_of(
                        routes.begin(),
                        routes.end(),
                        [](const ExactOrienteeringRoute* route) {
                            return route != nullptr;
                        })) {
                    return;
                }
                coordinatedExactRouteBundles.push_back(routes);
                coordinatedExactRouteBundles.push_back(
                    select_exact_terminal_variant(
                        config_,
                        state,
                        routes,
                        reachability,
                        ExactTerminalObjective::Fuel));
                coordinatedExactRouteBundles.push_back(
                    select_exact_terminal_variant(
                        config_,
                        state,
                        routes,
                        reachability,
                        ExactTerminalObjective::BrandAccess));
                coordinatedExactRouteBundles.push_back(
                    select_exact_terminal_variant(
                        config_,
                        state,
                        routes,
                        reachability,
                        ExactTerminalObjective::TankerAccess));
            };
        append_exact_bundles(exactOrienteering);
        const bool hasSupplementalRoutes = std::any_of(
            exactOrienteering.begin(),
            exactOrienteering.end(),
            [](const ExactOrienteeringReachability& exact) {
                return !exact.supplementalRoutes.empty();
            });
        if (hasSupplementalRoutes) {
            enhancedExactOrienteering = exactOrienteering;
            for (ExactOrienteeringReachability& exact :
                 enhancedExactOrienteering) {
                exact.maximalRoutes.insert(
                    exact.maximalRoutes.end(),
                    exact.supplementalRoutes.begin(),
                    exact.supplementalRoutes.end());
            }
            append_exact_bundles(enhancedExactOrienteering);
        }
        for (const Spot& target : config_.spots) {
            if (deadline_expired()) {
                if (diagnostics != nullptr) {
                    diagnostics->deadlineReached = true;
                }
                break;
            }
            std::vector<const ExactOrienteeringRoute*> aligned(
                static_cast<std::size_t>(config_.agent_count()),
                nullptr);
            bool hasAlignedRoute = false;
            for (AgentIndex agent = 0;
                 agent < config_.agent_count();
                 ++agent) {
                const std::vector<ExactOrienteeringRoute>& routes =
                    exactOrienteering.at(static_cast<std::size_t>(agent))
                        .servedSpotFuelRoutes;
                const auto found = std::find_if(
                    routes.begin(),
                    routes.end(),
                    [&target](const ExactOrienteeringRoute& route) {
                        return route.terminalCell == target.position;
                    });
                if (found != routes.end()) {
                    aligned.at(static_cast<std::size_t>(agent)) = &*found;
                    hasAlignedRoute = true;
                }
            }
            if (hasAlignedRoute) {
                coordinatedExactRouteBundles.push_back(std::move(aligned));
            }
        }
        const std::int32_t maximumAdditionalFrontierBundles = std::clamp(
            options.maximumCoordinatedExactBundles - 1,
            0,
            3);
        if (maximumAdditionalFrontierBundles > 0) {
            std::vector<std::vector<const ExactOrienteeringRoute*>> frontier =
                select_coordinated_exact_orienteering_frontier_routes(
                    config_,
                    state,
                    ledger,
                    coordinatedExactRouteBundles,
                    maximumAdditionalFrontierBundles,
                    options.deadline,
                    diagnostics);
            coordinatedExactRouteBundles.insert(
                coordinatedExactRouteBundles.end(),
                std::make_move_iterator(frontier.begin()),
                std::make_move_iterator(frontier.end()));
        }
        std::set<std::string> seenExactPlans;
        std::erase_if(
            coordinatedExactRouteBundles,
            [&config = config_, &state, &seenExactPlans](
                const std::vector<const ExactOrienteeringRoute*>&
                    routes) {
                DayPlan plan;
                plan.actions.reserve(routes.size());
                for (const ExactOrienteeringRoute* route : routes) {
                    plan.actions.push_back(
                        route != nullptr
                            ? route->actions
                            : wait_actions(config, state));
                }
                return !seenExactPlans
                            .insert(canonical_plan_bytes(plan))
                            .second;
            });
        if (diagnostics != nullptr) {
            diagnostics->exactOrienteeringBundles =
                static_cast<std::int32_t>(coordinatedExactRouteBundles.size());
            const std::chrono::steady_clock::time_point exactFinished =
                std::chrono::steady_clock::now();
            diagnostics->exactOrienteeringMilliseconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    exactFinished - exactStarted)
                    .count();
            diagnostics->exactOrienteeringFinalizationMilliseconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    exactFinished - finalizationStarted)
                    .count();
            if (options.deadline.has_value() && exactFinished > *options.deadline) {
                diagnostics->exactOrienteeringDeadlineOverrunMilliseconds =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        exactFinished - *options.deadline)
                        .count();
            }
        }
    };
    if (options.enableExactHarvestOrienteering &&
        options.allowUncachedHarvestTargets) {
        exactStarted = std::chrono::steady_clock::now();
        if (diagnostics != nullptr && options.deadline.has_value()) {
            diagnostics->exactOrienteeringDeadlineRemainingAtStartMilliseconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    *options.deadline - exactStarted)
                    .count();
        }
        const bool requiresFuelConstrainedSearch =
            (options.enableFuelConstrainedExactHarvestOrienteering ||
             options.enableAnytimeFuelConstrainedHarvestOrienteering) &&
            std::any_of(
                state.agents.begin(),
                state.agents.end(),
                [this, &state](const AgentState& agent) {
                    return agent.kind == AgentKind::Patrol &&
                        static_cast<std::int64_t>(agent.fuel) <
                            2LL * static_cast<std::int64_t>(
                                config_.steps_for_day(state.dayNumber));
                });
        BrandMask missingLifetimeBrands;
        for (std::int32_t brand = 0;
             brand < config_.brand_count();
             ++brand) {
            if (!has_brand(ledger.lifetimeBrands, brand)) {
                missingLifetimeBrands |= brand_bit(brand);
            }
        }
        std::map<std::pair<CellId, std::int32_t>, AgentIndex> exactByStart;
        if (requiresFuelConstrainedSearch) {
            for (AgentIndex agent = 0; agent < config_.agent_count(); ++agent) {
                const AgentState& agentState =
                    state.agents.at(static_cast<std::size_t>(agent));
                if (agentState.kind != AgentKind::Patrol) {
                    continue;
                }
                const auto exactCacheKey =
                    std::pair{agentState.position, agentState.fuel};
                const auto [iterator, inserted] =
                    exactByStart.emplace(exactCacheKey, agent);
                if (inserted) {
                    exactResourceTasks.push_back(agent);
                } else {
                    exactResourceAliases.emplace_back(agent, iterator->second);
                }
            }
            constexpr std::size_t kExactResourceWorkerCount = 4U;
            const std::size_t workerCount =
                std::min(kExactResourceWorkerCount, exactResourceTasks.size());
            exactResourceWorkers.reserve(workerCount);
            for (std::size_t worker = 0; worker < workerCount; ++worker) {
                exactResourceWorkers.emplace_back(
                    [this,
                     &state,
                     missingLifetimeBrands,
                     &options,
                     &exactOrienteering,
                     &exactResourceTasks,
                     &nextExactResourceTask]() {
                        while (true) {
                            const std::size_t task = nextExactResourceTask.fetch_add(
                                1U,
                                std::memory_order_relaxed);
                            if (task >= exactResourceTasks.size()) {
                                return;
                            }
                            const AgentIndex agent = exactResourceTasks.at(task);
                            const AgentState& agentState =
                                state.agents.at(static_cast<std::size_t>(agent));
                            const bool fuelConstrained =
                                static_cast<std::int64_t>(agentState.fuel) <
                                    2LL * static_cast<std::int64_t>(
                                        config_.steps_for_day(state.dayNumber));
                            exactOrienteering.at(static_cast<std::size_t>(agent)) =
                                options.enableAnytimeFuelConstrainedHarvestOrienteering &&
                                    fuelConstrained
                                ? enumerate_anytime_resource_routes(
                                      config_,
                                      state,
                                      agent,
                                      missingLifetimeBrands != 0U
                                        ? 1
                                        : std::min<std::int32_t>(
                                            std::max(
                                                1,
                                                config_.brand_count() - 1),
                                            static_cast<std::int32_t>(
                                                config_.spots.size())),
                                      32U,
                                      1250000U,
                                      options.deadline,
                                      missingLifetimeBrands)
                                : options.enableFuelConstrainedExactHarvestOrienteering &&
                                      fuelConstrained
                                ? enumerate_exact_resource_routes(
                                      config_,
                                      state,
                                      agent,
                                      options.deadline)
                                : enumerate_exact_high_fuel_routes(
                                      config_,
                                      state,
                                      agent,
                                      options.deadline);
                        }
                    });
            }
            exactDeferred = true;
        } else {
            for (AgentIndex agent = 0; agent < config_.agent_count(); ++agent) {
                const AgentState& agentState =
                    state.agents.at(static_cast<std::size_t>(agent));
                if (agentState.kind != AgentKind::Patrol) {
                    continue;
                }
                const auto exactCacheKey =
                    std::pair{agentState.position, agentState.fuel};
                const auto [iterator, inserted] =
                    exactByStart.emplace(exactCacheKey, agent);
                if (inserted) {
                    exactResourceTasks.push_back(agent);
                } else {
                    exactResourceAliases.emplace_back(agent, iterator->second);
                }
            }
            constexpr std::size_t kExactHighFuelWorkerCount = 4U;
            const std::size_t workerCount =
                std::min(kExactHighFuelWorkerCount, exactResourceTasks.size());
            exactResourceWorkers.reserve(workerCount);
            for (std::size_t worker = 0; worker < workerCount; ++worker) {
                exactResourceWorkers.emplace_back(
                    [this,
                     &state,
                     &options,
                     &exactOrienteering,
                     &exactResourceTasks,
                     &nextExactResourceTask]() {
                        while (true) {
                            const std::size_t task = nextExactResourceTask.fetch_add(
                                1U,
                                std::memory_order_relaxed);
                            if (task >= exactResourceTasks.size()) {
                                return;
                            }
                            const AgentIndex agent = exactResourceTasks.at(task);
                            exactOrienteering.at(static_cast<std::size_t>(agent)) =
                                enumerate_exact_high_fuel_routes(
                                    config_,
                                    state,
                                    agent,
                                    options.deadline);
                        }
                    });
            }
            exactResourceWorkers.clear();
            finalize_exact_orienteering();
        }
    }
    const auto find_paths = [this, &state, diagnostics](
                                CellId source,
                                CellId target,
                                const ParetoSearchOptions& searchOptions) {
        return router_.find_paths(
            source,
            target,
            state.roadStatuses,
            searchOptions,
            diagnostics != nullptr ? &diagnostics->pareto : nullptr);
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
                                  &spotTransitionReady,
                                  &find_paths](
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
            spotTransitionCache.at(fromOffset).at(toOffset) = find_paths(
                config_.spots.at(fromOffset).position,
                config_.spots.at(toOffset).position,
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
        const auto agentGenerationStarted = std::chrono::steady_clock::now();
        const std::int32_t agentQueriesBefore =
            diagnostics != nullptr ? diagnostics->pareto.queries : 0;
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
            if (diagnostics != nullptr) {
                diagnostics->agentMilliseconds.at(static_cast<std::size_t>(agentIndex)) =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - agentGenerationStarted)
                        .count();
                diagnostics->agentParetoQueries.at(static_cast<std::size_t>(agentIndex)) =
                    diagnostics->pareto.queries - agentQueriesBefore;
                diagnostics->deadlineReached = true;
            }
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
            const std::vector<ParetoPath> paths = find_paths(
                agent.position,
                spot.position,
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
                const std::vector<ParetoPath> fullPaths = find_paths(
                    agent.position,
                    spot.position,
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
                        const BrandMask tripleBrands =
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
        if (diagnostics != nullptr) {
            diagnostics->agentMilliseconds.at(static_cast<std::size_t>(agentIndex)) =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - agentGenerationStarted)
                    .count();
            diagnostics->agentParetoQueries.at(static_cast<std::size_t>(agentIndex)) =
                diagnostics->pareto.queries - agentQueriesBefore;
        }
    }

    const auto coordinationStarted = std::chrono::steady_clock::now();
    const std::int32_t coordinationQueriesBefore =
        diagnostics != nullptr ? diagnostics->pareto.queries : 0;
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
            const std::vector<ParetoPath> paths = find_paths(
                tanker.position,
                spot.position,
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
                const std::vector<ParetoPath> paths = find_paths(
                    state.agents.at(static_cast<std::size_t>(patrolIndex)).position,
                    spot.position,
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
    const bool multiDayFuelCarry =
        static_cast<std::int64_t>(config_.fuelLimit) >=
            3LL * static_cast<std::int64_t>(
                config_.steps_for_day(state.dayNumber));
    std::vector<CellId> rendezvousCells = multiDayFuelCarry
        ? std::vector<CellId>{}
        : criticalRoads;
    rendezvousCells.reserve(criticalRoads.size() + state.agents.size() + 8U);
    const auto append_rendezvous_cell = [&rendezvousCells](CellId cell) {
        if (std::find(rendezvousCells.begin(), rendezvousCells.end(), cell) ==
            rendezvousCells.end()) {
            rendezvousCells.push_back(cell);
        }
    };
    for (const AgentState& agent : state.agents) {
        if (multiDayFuelCarry) {
            append_rendezvous_cell(agent.position);
        } else {
            rendezvousCells.push_back(agent.position);
        }
    }
    if (multiDayFuelCarry) {
        for (const CellId road : criticalRoads) {
            append_rendezvous_cell(road);
        }
    }
    const std::int32_t rendezvousSpotLimit = std::min<std::int32_t>(
        8,
        static_cast<std::int32_t>(rendezvousTargets.size()));
    for (std::int32_t targetOffset = 0; targetOffset < rendezvousSpotLimit; ++targetOffset) {
        const CellId target = config_.spots.at(static_cast<std::size_t>(
            rendezvousTargets.at(static_cast<std::size_t>(targetOffset)))).position;
        if (multiDayFuelCarry) {
            append_rendezvous_cell(target);
        } else {
            rendezvousCells.push_back(target);
        }
    }
    if (!multiDayFuelCarry) {
        std::sort(rendezvousCells.begin(), rendezvousCells.end());
        rendezvousCells.erase(
            std::unique(rendezvousCells.begin(), rendezvousCells.end()),
            rendezvousCells.end());
    }
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
                const std::vector<ParetoPath> patrolArrivals = find_paths(
                    patrol.position,
                    rendezvous,
                    patrolArrivalOptions);
                const std::vector<ParetoPath> tankerArrivals = find_paths(
                    tanker.position,
                    rendezvous,
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
                    const std::vector<ParetoPath> departures = find_paths(
                        rendezvous,
                        spot.position,
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
                const std::vector<ParetoPath> patrolPaths = find_paths(
                    patrol.position,
                    spot.position,
                    patrolOptions);
                if (patrolPaths.empty()) {
                    continue;
                }
                const std::vector<ParetoPath> tankerPaths = find_paths(
                    tanker.position,
                    spot.position,
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
        if (agent.kind == AgentKind::Patrol &&
            options.enableHarvestExtensions &&
            options.maximumColumnsPerAgent >= 12 &&
            options.maximumColumnsPerAgent <= 16 &&
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
            const auto ordered_extension_targets =
                [this, &dockingTargets](
                    const RouteColumn& current,
                    bool requireCached,
                    const std::vector<std::vector<bool>>& ready) {
                    const SpotIndex terminalSpot = config_.spotAtCell.at(
                        static_cast<std::size_t>(current.terminalCell));
                    std::vector<SpotIndex> targets;
                    targets.reserve(dockingTargets.size());
                    for (const SpotIndex targetSpot : dockingTargets) {
                        const bool alreadyVisited = std::any_of(
                            current.firstVisits.begin(),
                            current.firstVisits.end(),
                            [targetSpot](const ColumnVisitEvent& visit) {
                                return visit.spot == targetSpot;
                            });
                        if (!alreadyVisited && targetSpot != terminalSpot &&
                            (!requireCached ||
                             ready.at(static_cast<std::size_t>(terminalSpot)).at(
                                 static_cast<std::size_t>(targetSpot)))) {
                            targets.push_back(targetSpot);
                        }
                    }
                    if (!requireCached) {
                        std::sort(
                            targets.begin(),
                            targets.end(),
                            [this, &current](SpotIndex left, SpotIndex right) {
                                const Spot& leftSpot =
                                    config_.spots.at(static_cast<std::size_t>(left));
                                const Spot& rightSpot =
                                    config_.spots.at(static_cast<std::size_t>(right));
                                const bool leftAddsBrand =
                                    !has_brand(current.estimatedBrands, leftSpot.brandIndex);
                                const bool rightAddsBrand =
                                    !has_brand(current.estimatedBrands, rightSpot.brandIndex);
                                if (leftAddsBrand != rightAddsBrand) {
                                    return leftAddsBrand;
                                }
                                const std::int32_t leftDistance =
                                    terminalDistancesToSpots_.at(static_cast<std::size_t>(left)).at(
                                        static_cast<std::size_t>(current.terminalCell));
                                const std::int32_t rightDistance =
                                    terminalDistancesToSpots_.at(static_cast<std::size_t>(right)).at(
                                        static_cast<std::size_t>(current.terminalCell));
                                if (leftDistance != rightDistance) {
                                    return leftDistance < rightDistance;
                                }
                                if (leftSpot.stock != rightSpot.stock) {
                                    return leftSpot.stock > rightSpot.stock;
                                }
                                return left < right;
                            });
                    }
                    return targets;
                };
            const auto extend_to_target =
                [this,
                 &ledger,
                 &enrich_column,
                 &nextColumnId,
                 &spot_transition](
                    const RouteColumn& current,
                    SpotIndex targetSpot) -> std::optional<RouteColumn> {
                    if (current.actions.empty() ||
                        current.actions.back().kind != ActionKind::Wait ||
                        current.actions.back().value <= 1) {
                        return std::nullopt;
                    }
                    const SpotIndex terminalSpot = config_.spotAtCell.at(
                        static_cast<std::size_t>(current.terminalCell));
                    const std::int32_t trailingWait = current.actions.back().value;
                    const std::optional<ParetoPath> extension = spot_transition(
                        terminalSpot,
                        targetSpot,
                        trailingWait - 1,
                        current.terminalFuel);
                    if (!extension.has_value() || extension->travelSteps >= trailingWait) {
                        return std::nullopt;
                    }
                    RouteColumn extended = current;
                    extended.columnId = nextColumnId++;
                    extended.harvestExtension = true;
                    extended.actions.pop_back();
                    for (const std::int32_t direction : extension->directions) {
                        extended.actions.push_back(PlanAction::move(direction));
                    }
                    const std::int32_t finalWait = trailingWait - extension->travelSteps;
                    if (finalWait > 0) {
                        extended.actions.push_back(PlanAction::wait(finalWait));
                    }
                    const Spot& target =
                        config_.spots.at(static_cast<std::size_t>(targetSpot));
                    extended.terminalCell = target.position;
                    extended.terminalFuel =
                        current.terminalFuel - extension->patrolFuel;
                    for (const auto& [road, stays] :
                         extension->heuristicFootprint.entries) {
                        extended.heuristicFootprint.add(road, stays);
                    }
                    extended.priority +=
                        (!has_brand(ledger.lifetimeBrands, target.brandIndex)
                             ? 1000000
                             : 0) +
                        (config_.brand_count() -
                         brand_rarity(config_, target.brandIndex)) *
                            1000 +
                        target.stock * 10 - extension->travelSteps;
                    enrich_column(extended);
                    if (extended.estimatedServings <= current.estimatedServings) {
                        return std::nullopt;
                    }
                    return extended;
                };
            if (!options.allowUncachedHarvestTargets) {
                bool extendedRoute = false;
                for (const std::size_t sourceIndex : sourceOrder) {
                    if (deadline_expired() || extendedRoute) {
                        break;
                    }
                    RouteColumn current = columns.at(sourceIndex);
                    for (std::int32_t extensionDepth = 0;
                         extensionDepth < 2;
                         ++extensionDepth) {
                        if (deadline_expired()) {
                            break;
                        }
                        const std::vector<SpotIndex> targets =
                            ordered_extension_targets(
                                current,
                                true,
                                spotTransitionReady);
                        bool extendedAtDepth = false;
                        for (const SpotIndex targetSpot : targets) {
                            if (std::optional<RouteColumn> extended =
                                    extend_to_target(current, targetSpot);
                                extended.has_value()) {
                                current = *extended;
                                harvestExtensions.push_back(
                                    std::move(*extended));
                                extendedRoute = true;
                                extendedAtDepth = true;
                                break;
                            }
                        }
                        if (!extendedAtDepth) {
                            break;
                        }
                    }
                }
            } else {
                std::int32_t extendedSources = 0;
                const std::int32_t maximumExtendedSources = std::max(
                    1,
                    options.maximumHarvestExtensionSources);
                const std::int32_t maximumExtensionDepth = std::max(
                    1,
                    options.maximumHarvestExtensionDepth);
                for (const std::size_t sourceIndex : sourceOrder) {
                    if (deadline_expired() ||
                        extendedSources >= maximumExtendedSources) {
                        break;
                    }
                    const std::int32_t sourceRank = extendedSources;
                    bool extendedSource = false;
                    std::vector<RouteColumn> frontier{columns.at(sourceIndex)};
                    for (std::int32_t extensionDepth = 0;
                         extensionDepth < maximumExtensionDepth;
                         ++extensionDepth) {
                        if (deadline_expired()) {
                            break;
                        }
                        std::vector<RouteColumn> nextFrontier;
                        const std::int32_t branchesPerState =
                            multiDayFuelCarry && extensionDepth == 0 ? 2 : 1;
                        for (const RouteColumn& current : frontier) {
                            const std::vector<SpotIndex> targets =
                                ordered_extension_targets(
                                    current,
                                    false,
                                    spotTransitionReady);
                            const std::size_t targetLimit =
                                std::min<std::size_t>(4U, targets.size());
                            std::int32_t acceptedBranches = 0;
                            for (std::size_t targetOffset = 0;
                                 targetOffset < targetLimit &&
                                     acceptedBranches < branchesPerState &&
                                     !deadline_expired();
                                 ++targetOffset) {
                                if (std::optional<RouteColumn> extended =
                                        extend_to_target(
                                            current,
                                            targets.at(targetOffset));
                                    extended.has_value()) {
                                    extended->harvestExtensionSourceRank = sourceRank;
                                    nextFrontier.push_back(*extended);
                                    harvestExtensions.push_back(
                                        std::move(*extended));
                                    extendedSource = true;
                                    ++acceptedBranches;
                                }
                            }
                        }
                        if (nextFrontier.empty()) {
                            break;
                        }
                        frontier = std::move(nextFrontier);
                    }
                    if (options.enableHarvestOrienteering &&
                        multiDayFuelCarry && !deadline_expired()) {
                        const auto remaining_wait = [](const RouteColumn& column) {
                            return !column.actions.empty() &&
                                    column.actions.back().kind == ActionKind::Wait
                                ? column.actions.back().value
                                : 0;
                        };
                        const auto better_orienteering_column =
                            [&ledger, &remaining_wait](
                                const RouteColumn& left,
                                const RouteColumn& right) {
                                const std::int32_t leftLifetimeGain =
                                    brand_difference_count(
                                        left.estimatedBrands,
                                        ledger.lifetimeBrands);
                                const std::int32_t rightLifetimeGain =
                                    brand_difference_count(
                                        right.estimatedBrands,
                                        ledger.lifetimeBrands);
                                if (leftLifetimeGain != rightLifetimeGain) {
                                    return leftLifetimeGain > rightLifetimeGain;
                                }
                                const std::int32_t leftBrandBreadth =
                                    brand_count(left.estimatedBrands);
                                const std::int32_t rightBrandBreadth =
                                    brand_count(right.estimatedBrands);
                                if (leftBrandBreadth != rightBrandBreadth) {
                                    return leftBrandBreadth > rightBrandBreadth;
                                }
                                if (left.estimatedServings !=
                                    right.estimatedServings) {
                                    return left.estimatedServings >
                                        right.estimatedServings;
                                }
                                const std::int32_t leftWait =
                                    remaining_wait(left);
                                const std::int32_t rightWait =
                                    remaining_wait(right);
                                if (leftWait != rightWait) {
                                    return leftWait > rightWait;
                                }
                                if (left.terminalFuel != right.terminalFuel) {
                                    return left.terminalFuel > right.terminalFuel;
                                }
                                if (left.priority != right.priority) {
                                    return left.priority > right.priority;
                                }
                                return actions_key(
                                           left.actions,
                                           left.escortGroup,
                                           left.contingencyBundle,
                                           left.requiredRefuels) <
                                    actions_key(
                                           right.actions,
                                           right.escortGroup,
                                           right.contingencyBundle,
                                           right.requiredRefuels);
                            };
                        constexpr std::size_t kOrienteeringBeamWidth = 6U;
                        constexpr std::size_t kOrienteeringTargetLimit = 8U;
                        constexpr std::size_t kOrienteeringResultLimit = 4U;
                        std::vector<RouteColumn> orienteeringFrontier{
                            columns.at(sourceIndex)};
                        std::vector<RouteColumn> orienteeringCandidates;
                        for (std::int32_t extensionDepth = 0;
                             extensionDepth < maximumExtensionDepth &&
                                 !deadline_expired();
                             ++extensionDepth) {
                            std::vector<RouteColumn> children;
                            for (const RouteColumn& current :
                                 orienteeringFrontier) {
                                const std::vector<SpotIndex> targets =
                                    ordered_extension_targets(
                                        current,
                                        false,
                                        spotTransitionReady);
                                const std::size_t targetLimit =
                                    std::min(
                                        kOrienteeringTargetLimit,
                                        targets.size());
                                for (std::size_t targetOffset = 0;
                                     targetOffset < targetLimit &&
                                         !deadline_expired();
                                     ++targetOffset) {
                                    if (std::optional<RouteColumn> extended =
                                            extend_to_target(
                                                current,
                                                targets.at(targetOffset));
                                        extended.has_value()) {
                                        extended->harvestExtensionSourceRank =
                                            sourceRank;
                                        children.push_back(
                                            std::move(*extended));
                                    }
                                }
                            }
                            if (children.empty()) {
                                break;
                            }
                            std::sort(
                                children.begin(),
                                children.end(),
                                better_orienteering_column);
                            std::set<std::string> seenActions;
                            std::set<CellId> representedTerminals;
                            std::vector<RouteColumn> nextFrontier;
                            nextFrontier.reserve(kOrienteeringBeamWidth);
                            const auto retain_child = [
                                &nextFrontier,
                                &seenActions](const RouteColumn& child) {
                                if (nextFrontier.size() >=
                                    kOrienteeringBeamWidth) {
                                    return;
                                }
                                const std::string key = actions_key(
                                    child.actions,
                                    child.escortGroup,
                                    child.contingencyBundle,
                                    child.requiredRefuels);
                                if (seenActions.insert(key).second) {
                                    nextFrontier.push_back(child);
                                }
                            };
                            for (const RouteColumn& child : children) {
                                if (representedTerminals.insert(
                                        child.terminalCell).second) {
                                    retain_child(child);
                                }
                                if (nextFrontier.size() >=
                                    kOrienteeringBeamWidth) {
                                    break;
                                }
                            }
                            for (const RouteColumn& child : children) {
                                retain_child(child);
                                if (nextFrontier.size() >=
                                    kOrienteeringBeamWidth) {
                                    break;
                                }
                            }
                            orienteeringCandidates.insert(
                                orienteeringCandidates.end(),
                                nextFrontier.begin(),
                                nextFrontier.end());
                            orienteeringFrontier =
                                std::move(nextFrontier);
                        }
                        std::sort(
                            orienteeringCandidates.begin(),
                            orienteeringCandidates.end(),
                            better_orienteering_column);
                        std::set<std::string> existingActions;
                        for (const RouteColumn& extension :
                             harvestExtensions) {
                            existingActions.insert(actions_key(
                                extension.actions,
                                extension.escortGroup,
                                extension.contingencyBundle,
                                extension.requiredRefuels));
                        }
                        std::size_t retainedOrienteering = 0;
                        for (RouteColumn& candidate :
                             orienteeringCandidates) {
                            if (retainedOrienteering >=
                                    kOrienteeringResultLimit ||
                                deadline_expired()) {
                                break;
                            }
                            const std::string key = actions_key(
                                candidate.actions,
                                candidate.escortGroup,
                                candidate.contingencyBundle,
                                candidate.requiredRefuels);
                            if (!existingActions.insert(key).second) {
                                continue;
                            }
                            harvestExtensions.push_back(
                                std::move(candidate));
                            extendedSource = true;
                            ++retainedOrienteering;
                        }
                    }
                    if (extendedSource) {
                        ++extendedSources;
                    }
                }
            }
            columns.insert(
                columns.end(),
                std::make_move_iterator(harvestExtensions.begin()),
                std::make_move_iterator(harvestExtensions.end()));
        }
        prune_columns(
            columns,
            std::max(1, options.maximumColumnsPerAgent),
            static_cast<std::int64_t>(config_.fuelLimit) >=
                3LL * static_cast<std::int64_t>(
                    config_.steps_for_day(state.dayNumber)));
    }
    if (exactDeferred) {
        exactResourceWorkers.clear();
        finalize_exact_orienteering();
    }
    for (const std::vector<const ExactOrienteeringRoute*>& coordinatedExactRoutes :
         coordinatedExactRouteBundles) {
        const std::int32_t exactBundle = nextContingencyBundle++;
        const DayPlan* exactFallbackPlan =
            !options.seedPlans.empty() &&
                options.seedPlans.front().actions.size() ==
                    static_cast<std::size_t>(config_.agent_count())
            ? &options.seedPlans.front()
            : nullptr;
        std::vector<RouteColumn> exactBundleColumns;
        exactBundleColumns.reserve(static_cast<std::size_t>(config_.agent_count()));
        for (AgentIndex agentIndex = 0;
             agentIndex < config_.agent_count();
             ++agentIndex) {
            const ExactOrienteeringRoute* route =
                coordinatedExactRoutes.at(static_cast<std::size_t>(agentIndex));
            RouteColumn exact;
            exact.columnId = nextColumnId++;
            exact.agent = agentIndex;
            exact.actions = route != nullptr
                ? route->actions
                : exactFallbackPlan != nullptr
                    ? exactFallbackPlan->actions.at(
                        static_cast<std::size_t>(agentIndex))
                    : wait_actions(config_, state);
            exact.priority = 5000000;
            exact.harvestExtension = true;
            exact.exactOrienteering = true;
            exact.harvestExtensionSourceRank = -1;
            exact.contingencyBundle = exactBundle;
            populate_first_visits(config_, state, exact);
            if (route != nullptr && !exact.hasExactTimeline) {
                exactBundleColumns.clear();
                break;
            }
            std::uint32_t actualMask = 0;
            for (const ColumnVisitEvent& event : exact.firstVisits) {
                actualMask |= std::uint32_t{1} << static_cast<std::uint32_t>(event.spot);
                exact.estimatedBrands |= brand_bit(event.brandIndex);
                if (event.claimedServing) {
                    ++exact.estimatedServings;
                }
            }
            if (route != nullptr && actualMask != route->spotMask) {
                exactBundleColumns.clear();
                break;
            }
            exact.heuristicFootprint = exact.fullFootprint;
            populate_terminal_brand_feature(
                config_,
                ledger,
                terminalDistancesToSpots_,
                exact);
            exactBundleColumns.push_back(std::move(exact));
        }
        if (exactBundleColumns.size() == static_cast<std::size_t>(config_.agent_count())) {
            for (RouteColumn& exact : exactBundleColumns) {
                portfolio.columnsByAgent.at(static_cast<std::size_t>(exact.agent))
                    .push_back(std::move(exact));
            }
        }
    }
    populate_exact_escort_segments(state, portfolio);
    if (diagnostics != nullptr) {
        diagnostics->coordinationMilliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - coordinationStarted)
                .count();
        diagnostics->coordinationParetoQueries =
            diagnostics->pareto.queries - coordinationQueriesBefore;
        diagnostics->deadlineReached = diagnostics->deadlineReached || deadline_expired();
    }
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
        prune_columns(
            columns,
            maximumColumnsPerAgent,
            static_cast<std::int64_t>(config_.fuelLimit) >=
                3LL * static_cast<std::int64_t>(
                    config_.steps_for_day(state.dayNumber)));
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
    diagnostics.stockCappedSearchOrder = options.preferStockCappedSearchOrder;
    diagnostics.bundleAwareUpperBound =
        options.enableBundleAwareUpperBound;
    if (portfolio.columnsByAgent.size() != static_cast<std::size_t>(config_.agent_count()) ||
        options.maximumCombinations <= 0 || options.maximumCandidates <= 0) {
        return {};
    }
    const bool hasSynchronizationConstraints = std::any_of(
        portfolio.columnsByAgent.begin(),
        portfolio.columnsByAgent.end(),
        [](const std::vector<RouteColumn>& columns) {
            return std::any_of(
                columns.begin(),
                columns.end(),
                [](const RouteColumn& column) {
                    return column.escortGroup >= 0 || !column.requiredRefuels.empty();
                });
        });
    SynchronizationAvailability synchronizationAvailability;
    if (hasSynchronizationConstraints) {
        for (std::size_t agentOffset = 0;
             agentOffset < portfolio.columnsByAgent.size();
             ++agentOffset) {
            const std::uint32_t agentMask =
                std::uint32_t{1} << static_cast<std::uint32_t>(agentOffset);
            const bool isTanker =
                state.agents.at(agentOffset).kind == AgentKind::Tanker;
            for (const RouteColumn& column : portfolio.columnsByAgent.at(agentOffset)) {
                if (column.escortGroup >= 0) {
                    const std::size_t groupOffset =
                        static_cast<std::size_t>(column.escortGroup);
                    if (groupOffset >= synchronizationAvailability.escortAgentMasks.size()) {
                        synchronizationAvailability.escortAgentMasks.resize(groupOffset + 1U);
                        synchronizationAvailability.escortTankerMasks.resize(groupOffset + 1U);
                    }
                    synchronizationAvailability.escortAgentMasks.at(groupOffset) |= agentMask;
                    if (isTanker) {
                        synchronizationAvailability.escortTankerMasks.at(groupOffset) |= agentMask;
                    }
                }
                if (isTanker) {
                    for (const RefuelEvent& refuel : column.providedRefuels) {
                        synchronizationAvailability.refuelProviderMasks[refuel_event_key(refuel)] |=
                            agentMask;
                    }
                }
            }
        }
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
    std::map<std::int32_t, std::vector<const RouteColumn*>> completeBundles;
    for (AgentIndex agent = 0; agent < config_.agent_count(); ++agent) {
        for (const RouteColumn& column :
             portfolio.columnsByAgent.at(static_cast<std::size_t>(agent))) {
            if (column.contingencyBundle < 0) {
                continue;
            }
            std::vector<const RouteColumn*>& bundle =
                completeBundles[column.contingencyBundle];
            if (bundle.empty()) {
                bundle.resize(static_cast<std::size_t>(config_.agent_count()), nullptr);
            }
            const RouteColumn*& assigned = bundle.at(static_cast<std::size_t>(agent));
            if (assigned == nullptr || column.priority > assigned->priority) {
                assigned = &column;
            }
        }
    }
    for (const auto& [bundleId, bundle] : completeBundles) {
        static_cast<void>(bundleId);
        if (std::any_of(
                bundle.begin(),
                bundle.end(),
                [](const RouteColumn* column) { return column == nullptr; })) {
            continue;
        }
        const bool exactOrienteeringBundle = std::any_of(
            bundle.begin(),
            bundle.end(),
            [](const RouteColumn* column) {
                return column != nullptr && column->exactOrienteering;
            });
        if (exactOrienteeringBundle) {
            ++diagnostics.exactBundlesDiscovered;
        }
        if (deadline_expired() && !exactOrienteeringBundle) {
            continue;
        }
        DayPlan plan;
        plan.actions.resize(static_cast<std::size_t>(config_.agent_count()));
        for (AgentIndex agent = 0; agent < config_.agent_count(); ++agent) {
            plan.actions.at(static_cast<std::size_t>(agent)) =
                bundle.at(static_cast<std::size_t>(agent))->actions;
        }
        const std::string planKey = canonical_plan_bytes(plan);
        if (!evaluatedPlans.insert(planKey).second) {
            continue;
        }
        if (exactOrienteeringBundle) {
            ++diagnostics.exactBundlesEvaluated;
        }
        if (std::optional<MasterCandidate> candidate = evaluate_exact_plan(
                state,
                ledger,
                plan,
                options.mandatoryReservations);
            candidate.has_value()) {
            ++diagnostics.simulatorValidCombinations;
            if (exactOrienteeringBundle) {
                ++diagnostics.exactBundlesAccepted;
                if (diagnostics.bestExactBundleScore < candidate->scoreAfterToday) {
                    diagnostics.bestExactBundleScore = candidate->scoreAfterToday;
                }
            }
            candidates.push_back(std::move(*candidate));
        } else {
            ++diagnostics.invalidPlanCombinations;
        }
    }

    for (std::int32_t resolveRound = 0;
         resolveRound < maximumResolveRounds && diagnostics.combinationsVisited < options.maximumCombinations;
         ++resolveRound) {
        if (deadline_expired()) {
            diagnostics.deadlineReached = true;
            break;
        }
        ++diagnostics.cutRounds;
        const std::chrono::steady_clock::time_point roundPreparationStarted =
            std::chrono::steady_clock::now();
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
                [&config = config_, &ledger, &cutState, &options](const RouteColumn* left, const RouteColumn* right) {
                    const BrandMask leftBrands = column_brand_mask(config, *left);
                    const BrandMask rightBrands = column_brand_mask(config, *right);
                    const std::int32_t leftLifetimeGain =
                        brand_difference_count(leftBrands, ledger.lifetimeBrands);
                    const std::int32_t rightLifetimeGain =
                        brand_difference_count(rightBrands, ledger.lifetimeBrands);
                    if (leftLifetimeGain != rightLifetimeGain) {
                        return leftLifetimeGain > rightLifetimeGain;
                    }
                    const std::int32_t leftDailyGain = brand_count(leftBrands);
                    const std::int32_t rightDailyGain = brand_count(rightBrands);
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
                    if (options.preferBaselineHarvestSources &&
                        left->harvestExtensionSourceRank !=
                        right->harvestExtensionSourceRank) {
                        return left->harvestExtensionSourceRank <
                            right->harvestExtensionSourceRank;
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
        std::vector<BrandMask> suffixPossibleBrands(ordering.size() + 1U);
        std::vector<std::vector<std::int32_t>> suffixPossibleClaims(
            ordering.size() + 1U,
            std::vector<std::int32_t>(config_.spots.size(), 0));
        std::vector<std::vector<SpotIndex>> claimableSpotsByDepth(ordering.size());
        std::vector<std::int32_t> suffixMaximumAgentClaims(ordering.size() + 1U, 0);
        if (exactMetadata) {
            for (std::size_t depth = ordering.size(); depth-- > 0U;) {
                suffixPossibleBrands.at(depth) = suffixPossibleBrands.at(depth + 1U);
                suffixPossibleClaims.at(depth) = suffixPossibleClaims.at(depth + 1U);
                suffixMaximumAgentClaims.at(depth) =
                    suffixMaximumAgentClaims.at(depth + 1U);
                const AgentIndex agentIndex = ordering.at(depth);
                std::vector<bool> agentCanClaim(config_.spots.size(), false);
                std::int32_t maximumAgentClaims = 0;
                for (const RouteColumn* column :
                     orderedColumns.at(static_cast<std::size_t>(agentIndex))) {
                    suffixPossibleBrands.at(depth) |= column_brand_mask(config_, *column);
                    std::int32_t columnClaims = 0;
                    for (const ColumnVisitEvent& event : column->firstVisits) {
                        if (event.claimedServing) {
                            agentCanClaim.at(static_cast<std::size_t>(event.spot)) = true;
                            ++columnClaims;
                        }
                    }
                    maximumAgentClaims = std::max(maximumAgentClaims, columnClaims);
                }
                suffixMaximumAgentClaims.at(depth) += maximumAgentClaims;
                for (std::size_t spotOffset = 0; spotOffset < agentCanClaim.size(); ++spotOffset) {
                    if (agentCanClaim.at(spotOffset)) {
                        ++suffixPossibleClaims.at(depth).at(spotOffset);
                        claimableSpotsByDepth.at(depth).push_back(
                            static_cast<SpotIndex>(spotOffset));
                    }
                }
            }
        }
        struct BundleBoundMetadata {
            std::int32_t bundle = -1;
            std::vector<bool> suffixFeasible;
            std::vector<BrandMask> suffixPossibleBrands;
            std::vector<std::vector<std::int32_t>>
                suffixPossibleClaims;
            std::vector<std::int32_t>
                suffixMaximumAgentClaims;
            std::vector<std::vector<BrandMask>>
                suffixMaximalBrandMasks;
            std::vector<bool>
                suffixExactBrandFrontier;
        };
        std::vector<BundleBoundMetadata> bundleBoundMetadata;
        std::map<std::int32_t, std::size_t> bundleBoundIndex;
        if (exactMetadata &&
            options.enableBundleAwareUpperBound) {
            std::set<std::int32_t> bundleModes{-1};
            for (const std::vector<const RouteColumn*>& columns :
                 orderedColumns) {
                for (const RouteColumn* column : columns) {
                    bundleModes.insert(column->contingencyBundle);
                }
            }
            bundleBoundMetadata.reserve(bundleModes.size());
            for (const std::int32_t bundle : bundleModes) {
                BundleBoundMetadata metadata;
                metadata.bundle = bundle;
                metadata.suffixFeasible.assign(
                    ordering.size() + 1U,
                    true);
                metadata.suffixPossibleBrands.assign(
                    ordering.size() + 1U,
                    0U);
                metadata.suffixPossibleClaims.assign(
                    ordering.size() + 1U,
                    std::vector<std::int32_t>(
                        config_.spots.size(),
                        0));
                metadata.suffixMaximumAgentClaims.assign(
                    ordering.size() + 1U,
                    0);
                metadata.suffixMaximalBrandMasks.resize(
                    ordering.size() + 1U);
                metadata.suffixMaximalBrandMasks.back().push_back(0U);
                metadata.suffixExactBrandFrontier.assign(
                    ordering.size() + 1U,
                    true);
                for (std::size_t depth = ordering.size();
                     depth-- > 0U;) {
                    metadata.suffixFeasible.at(depth) =
                        metadata.suffixFeasible.at(depth + 1U);
                    metadata.suffixPossibleBrands.at(depth) =
                        metadata.suffixPossibleBrands.at(
                            depth + 1U);
                    metadata.suffixPossibleClaims.at(depth) =
                        metadata.suffixPossibleClaims.at(
                            depth + 1U);
                    metadata.suffixMaximumAgentClaims.at(depth) =
                        metadata.suffixMaximumAgentClaims.at(
                            depth + 1U);
                    const AgentIndex agentIndex =
                        ordering.at(depth);
                    std::vector<bool> agentCanClaim(
                        config_.spots.size(),
                        false);
                    std::vector<BrandMask> agentBrandMasks;
                    std::int32_t maximumAgentClaims = 0;
                    bool hasCompatibleColumn = false;
                    for (const RouteColumn* column :
                         orderedColumns.at(
                             static_cast<std::size_t>(
                                 agentIndex))) {
                        if (column->contingencyBundle != bundle) {
                            continue;
                        }
                        hasCompatibleColumn = true;
                        const BrandMask columnBrands =
                            column_brand_mask(config_, *column);
                        metadata.suffixPossibleBrands.at(depth) |=
                            columnBrands;
                        agentBrandMasks.push_back(columnBrands);
                        std::int32_t columnClaims = 0;
                        for (const ColumnVisitEvent& event :
                             column->firstVisits) {
                            if (!event.claimedServing) {
                                continue;
                            }
                            agentCanClaim.at(
                                static_cast<std::size_t>(
                                    event.spot)) = true;
                            ++columnClaims;
                        }
                        maximumAgentClaims = std::max(
                            maximumAgentClaims,
                            columnClaims);
                    }
                    metadata.suffixFeasible.at(depth) =
                        metadata.suffixFeasible.at(depth) &&
                        hasCompatibleColumn;
                    metadata.suffixMaximumAgentClaims.at(depth) +=
                        maximumAgentClaims;
                    for (std::size_t spotOffset = 0;
                         spotOffset < agentCanClaim.size();
                         ++spotOffset) {
                        if (!agentCanClaim.at(spotOffset)) {
                            continue;
                        }
                        ++metadata.suffixPossibleClaims.at(depth)
                              .at(spotOffset);
                    }
                    constexpr std::size_t
                        kMaximumRawBrandFrontier = 32768U;
                    constexpr std::size_t
                        kMaximumRetainedBrandFrontier = 4096U;
                    if (!hasCompatibleColumn ||
                        !metadata.suffixExactBrandFrontier.at(
                            depth + 1U)) {
                        metadata.suffixExactBrandFrontier.at(depth) =
                            hasCompatibleColumn &&
                            metadata.suffixExactBrandFrontier.at(
                                depth + 1U);
                        continue;
                    }
                    std::sort(
                        agentBrandMasks.begin(),
                        agentBrandMasks.end());
                    agentBrandMasks.erase(
                        std::unique(
                            agentBrandMasks.begin(),
                            agentBrandMasks.end()),
                        agentBrandMasks.end());
                    const std::vector<BrandMask>& suffixMasks =
                        metadata.suffixMaximalBrandMasks.at(
                            depth + 1U);
                    if (agentBrandMasks.empty() ||
                        suffixMasks.size() >
                            kMaximumRawBrandFrontier /
                                agentBrandMasks.size()) {
                        metadata.suffixExactBrandFrontier.at(depth) =
                            false;
                        ++diagnostics.bundleBrandFrontierFallbacks;
                        continue;
                    }
                    std::vector<BrandMask> brandCandidates;
                    brandCandidates.reserve(
                        agentBrandMasks.size() *
                        suffixMasks.size());
                    for (const BrandMask& agentMask :
                         agentBrandMasks) {
                        for (const BrandMask& suffixMask :
                             suffixMasks) {
                            brandCandidates.push_back(
                                agentMask | suffixMask);
                        }
                    }
                    std::sort(
                        brandCandidates.begin(),
                        brandCandidates.end());
                    brandCandidates.erase(
                        std::unique(
                            brandCandidates.begin(),
                            brandCandidates.end()),
                        brandCandidates.end());
                    std::stable_sort(
                        brandCandidates.begin(),
                        brandCandidates.end(),
                        [](const BrandMask& left,
                           const BrandMask& right) {
                            return brand_count(left) >
                                brand_count(right);
                        });
                    std::vector<BrandMask>& retained =
                        metadata.suffixMaximalBrandMasks.at(depth);
                    retained.reserve(std::min(
                        brandCandidates.size(),
                        kMaximumRetainedBrandFrontier));
                    for (const BrandMask& candidate :
                         brandCandidates) {
                        const bool dominated = std::any_of(
                            retained.begin(),
                            retained.end(),
                            [&candidate](const BrandMask& existing) {
                                return (candidate & existing) ==
                                    candidate;
                            });
                        if (dominated) {
                            continue;
                        }
                        retained.push_back(candidate);
                        if (retained.size() >
                            kMaximumRetainedBrandFrontier) {
                            retained.clear();
                            metadata.suffixExactBrandFrontier.at(
                                depth) = false;
                            ++diagnostics
                                  .bundleBrandFrontierFallbacks;
                            break;
                        }
                    }
                    if (metadata.suffixExactBrandFrontier.at(
                            depth)) {
                        diagnostics.bundleBrandFrontierStates +=
                            static_cast<std::int32_t>(
                                retained.size());
                    }
                }
                bundleBoundIndex.emplace(
                    bundle,
                    bundleBoundMetadata.size());
                bundleBoundMetadata.push_back(
                    std::move(metadata));
            }
        }
        std::vector<std::int32_t> selectedClaimCounts(config_.spots.size(), 0);
        const auto suffix_upper_bound_score =
            [this, &ledger, &suffixPossibleBrands, &suffixMaximumAgentClaims](
                BrandMask selectedBrands,
                std::size_t depth,
                std::int32_t servingUpperBound,
                std::int32_t selectedRawClaims) {
                const BrandMask dailyBrands =
                    selectedBrands | suffixPossibleBrands.at(depth);
                const std::int32_t agentServingUpperBound =
                    selectedRawClaims + suffixMaximumAgentClaims.at(depth);
                return OfficialScore{
                    brand_count(ledger.lifetimeBrands | dailyBrands),
                    ledger.totalDailyDistinct +
                        brand_count(dailyBrands),
                    ledger.totalServings +
                        std::min(servingUpperBound, agentServingUpperBound),
                };
            };
        constexpr std::int32_t kUnsetBundle =
            std::numeric_limits<std::int32_t>::min();
        const auto bundle_upper_bound_for_mode =
            [this,
             &ledger,
             &selectedClaimCounts](
                const BundleBoundMetadata& metadata,
                BrandMask selectedBrands,
                std::size_t depth,
                std::int32_t selectedRawClaims)
                -> std::optional<OfficialScore> {
                if (!metadata.suffixFeasible.at(depth)) {
                    return std::nullopt;
                }
                std::int32_t lifetimeDistinct = -1;
                std::int32_t dailyDistinct = -1;
                if (metadata.suffixExactBrandFrontier.at(depth)) {
                    for (const BrandMask& suffixBrands :
                         metadata.suffixMaximalBrandMasks.at(
                             depth)) {
                        const BrandMask dailyBrands =
                            selectedBrands | suffixBrands;
                        const std::int32_t candidateLifetime =
                            brand_count(
                                ledger.lifetimeBrands |
                                dailyBrands);
                        const std::int32_t candidateDaily =
                            brand_count(dailyBrands);
                        if (candidateLifetime >
                                lifetimeDistinct ||
                            (candidateLifetime ==
                                 lifetimeDistinct &&
                             candidateDaily >
                                 dailyDistinct)) {
                            lifetimeDistinct =
                                candidateLifetime;
                            dailyDistinct = candidateDaily;
                        }
                    }
                } else {
                    const BrandMask dailyBrands =
                        selectedBrands |
                        metadata.suffixPossibleBrands.at(depth);
                    lifetimeDistinct =
                        brand_count(
                            ledger.lifetimeBrands |
                            dailyBrands);
                    dailyDistinct =
                        brand_count(dailyBrands);
                }
                std::int32_t stockUpperBound = 0;
                for (std::size_t spotOffset = 0;
                     spotOffset < config_.spots.size();
                     ++spotOffset) {
                    stockUpperBound += std::min(
                        selectedClaimCounts.at(spotOffset) +
                            metadata.suffixPossibleClaims.at(depth)
                                .at(spotOffset),
                        config_.spots.at(spotOffset).stock);
                }
                const std::int32_t agentUpperBound =
                    selectedRawClaims +
                    metadata.suffixMaximumAgentClaims.at(depth);
                return OfficialScore{
                    lifetimeDistinct,
                    ledger.totalDailyDistinct +
                        dailyDistinct,
                    ledger.totalServings +
                        std::min(
                            stockUpperBound,
                            agentUpperBound),
                };
            };
        const auto active_bundle_upper_bound =
            [&bundleBoundMetadata,
             &bundleBoundIndex,
             &bundle_upper_bound_for_mode](
                std::int32_t activeBundle,
                BrandMask selectedBrands,
                std::size_t depth,
                std::int32_t selectedRawClaims)
                -> std::optional<OfficialScore> {
                if (activeBundle != kUnsetBundle) {
                    const auto found =
                        bundleBoundIndex.find(activeBundle);
                    if (found == bundleBoundIndex.end()) {
                        return std::nullopt;
                    }
                    return bundle_upper_bound_for_mode(
                        bundleBoundMetadata.at(found->second),
                        selectedBrands,
                        depth,
                        selectedRawClaims);
                }
                std::optional<OfficialScore> best;
                for (const BundleBoundMetadata& metadata :
                     bundleBoundMetadata) {
                    const std::optional<OfficialScore> candidate =
                        bundle_upper_bound_for_mode(
                            metadata,
                            selectedBrands,
                            depth,
                            selectedRawClaims);
                    if (candidate.has_value() &&
                        (!best.has_value() ||
                         compare_lexicographic(
                             *candidate,
                             *best) > 0)) {
                        best = candidate;
                    }
                }
                return best;
            };
        std::int32_t rootServingUpperBound = 0;
        if (exactMetadata) {
            for (std::size_t spotOffset = 0;
                 spotOffset < config_.spots.size();
                 ++spotOffset) {
                rootServingUpperBound += std::min(
                    suffixPossibleClaims.front().at(spotOffset),
                    config_.spots.at(spotOffset).stock);
            }
        }
        diagnostics.optimisticUpperBound = exactMetadata
            ? suffix_upper_bound_score(
                  0,
                  0U,
                  rootServingUpperBound,
                  0)
            : OfficialScore{
                  config_.brand_count(),
                  ledger.totalDailyDistinct + config_.brand_count(),
                  ledger.totalServings + std::accumulate(
                      config_.spots.begin(),
                      config_.spots.end(),
                  0,
                  [](std::int32_t total, const Spot& spot) { return total + spot.stock; }),
              };
        diagnostics.searchGuidanceUpperBound =
            diagnostics.optimisticUpperBound;
        if (exactMetadata &&
            options.enableBundleAwareUpperBound) {
            if (const std::optional<OfficialScore> bundleUpperBound =
                    active_bundle_upper_bound(
                        kUnsetBundle,
                        0U,
                        0U,
                        0);
                bundleUpperBound.has_value() &&
                compare_lexicographic(
                    *bundleUpperBound,
                    diagnostics.optimisticUpperBound) < 0) {
                diagnostics.optimisticUpperBound =
                    *bundleUpperBound;
            }
        }
        diagnostics.roundPreparationMicroseconds += std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - roundPreparationStarted).count();
        if (resolveRound == 0 && exactMetadata && options.maximumCombinations >= 128 &&
            diagnostics.combinationsVisited < roundLimit && !deadline_expired()) {
            const std::chrono::steady_clock::time_point beamConstructionStarted =
                std::chrono::steady_clock::now();
            struct BeamSelection {
                std::vector<const RouteColumn*> columns;
                BrandMask brands;
                std::int32_t rankedServings = 0;
                std::int32_t explorationRank = 0;
                std::int64_t priority = 0;
            };
            const auto tanker_terminal_signature =
                [&state](const BeamSelection& selection) {
                    std::vector<CellId> signature;
                    for (AgentIndex agentIndex = 0;
                         agentIndex < static_cast<AgentIndex>(selection.columns.size());
                         ++agentIndex) {
                        if (state.agents.at(static_cast<std::size_t>(agentIndex)).kind !=
                            AgentKind::Tanker) {
                            continue;
                        }
                        const RouteColumn* column =
                            selection.columns.at(static_cast<std::size_t>(agentIndex));
                        if (column != nullptr) {
                            signature.push_back(column->terminalCell);
                        }
                    }
                    return signature;
                };
            const auto retain_beam_diversity =
                [&tanker_terminal_signature](
                    std::vector<BeamSelection>& selections,
                    std::int32_t limit) {
                    if (limit <= 0 ||
                        static_cast<std::int32_t>(selections.size()) <= limit) {
                        return;
                    }
                    const std::size_t retainedLimit =
                        static_cast<std::size_t>(limit);
                    const std::size_t qualityCount =
                        std::max<std::size_t>(1, retainedLimit * 3U / 4U);
                    std::vector<std::size_t> retained;
                    retained.reserve(retainedLimit);
                    std::vector<bool> used(selections.size(), false);
                    for (std::size_t index = 0;
                         index < qualityCount;
                         ++index) {
                        retained.push_back(index);
                        used.at(index) = true;
                    }
                    std::map<std::vector<CellId>, std::vector<std::size_t>>
                        candidatesByTankerSignature;
                    for (std::size_t index = qualityCount;
                         index < selections.size();
                         ++index) {
                        const std::vector<CellId> signature =
                            tanker_terminal_signature(selections.at(index));
                        if (!signature.empty()) {
                            candidatesByTankerSignature[signature].push_back(index);
                        }
                    }
                    for (std::size_t round = 0;
                         retained.size() < retainedLimit;
                         ++round) {
                        bool added = false;
                        for (const auto& [signature, indices] :
                             candidatesByTankerSignature) {
                            static_cast<void>(signature);
                            if (round >= indices.size()) {
                                continue;
                            }
                            const std::size_t index = indices.at(round);
                            retained.push_back(index);
                            used.at(index) = true;
                            added = true;
                            if (retained.size() == retainedLimit) {
                                break;
                            }
                        }
                        if (!added) {
                            break;
                        }
                    }
                    for (std::size_t index = qualityCount;
                         index < selections.size() &&
                         retained.size() < retainedLimit;
                         ++index) {
                        if (!used.at(index)) {
                            retained.push_back(index);
                        }
                    }
                    std::sort(retained.begin(), retained.end());
                    std::vector<BeamSelection> diversified;
                    diversified.reserve(retained.size());
                    for (const std::size_t index : retained) {
                        diversified.push_back(std::move(selections.at(index)));
                    }
                    selections = std::move(diversified);
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
                        if (hasSynchronizationConstraints &&
                            !partial_synchronized_selection_is_feasible(
                                state,
                                candidate.columns,
                                synchronizationAvailability)) {
                            continue;
                        }
                        candidate.brands |= column_brand_mask(config_, *column);
                        candidate.rankedServings += options.preferStockCappedSearchOrder
                            ? marginal_stock_credits(config_, partial.columns, *column)
                            : column->estimatedServings;
                        candidate.explorationRank +=
                            column->harvestExtensionSourceRank;
                        candidate.priority += conflict_aware_priority(*column, cutState);
                        expanded.push_back(std::move(candidate));
                    }
                }
                std::sort(
                    expanded.begin(),
                    expanded.end(),
                    [&ledger, &options](const BeamSelection& left, const BeamSelection& right) {
                        const std::int32_t lifetimeOrder =
                            brand_count(ledger.lifetimeBrands | left.brands) -
                            brand_count(ledger.lifetimeBrands | right.brands);
                        if (lifetimeOrder != 0) {
                            return lifetimeOrder > 0;
                        }
                        const std::int32_t dailyOrder =
                            brand_count(left.brands) -
                            brand_count(right.brands);
                        if (dailyOrder != 0) {
                            return dailyOrder > 0;
                        }
                        if (left.rankedServings != right.rankedServings) {
                            return left.rankedServings > right.rankedServings;
                        }
                        if (options.preferBaselineHarvestSources &&
                            left.explorationRank != right.explorationRank) {
                            return left.explorationRank < right.explorationRank;
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
                retain_beam_diversity(expanded, beamWidth);
                beam = std::move(expanded);
                if (beam.empty()) {
                    beamComplete = false;
                    break;
                }
            }
            diagnostics.beamConstructionMicroseconds += std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - beamConstructionStarted).count();
            const std::int32_t evaluationLimit = std::min(
                static_cast<std::int32_t>(beam.size()),
                std::max(32, options.maximumCandidates * 8));
            retain_beam_diversity(beam, evaluationLimit);
            const std::int32_t combinationsBeforeBeam = diagnostics.combinationsVisited;
            const std::chrono::steady_clock::time_point beamEvaluationStarted =
                std::chrono::steady_clock::now();
            for (std::int32_t beamIndex = 0;
                 beamComplete &&
                 beamIndex < static_cast<std::int32_t>(beam.size()) &&
                 diagnostics.combinationsVisited < roundLimit && !deadline_expired();
                 ++beamIndex) {
                const std::vector<const RouteColumn*>& seedSelection =
                    beam.at(static_cast<std::size_t>(beamIndex)).columns;
                ++diagnostics.combinationsVisited;
                if (hasSynchronizationConstraints &&
                    !synchronized_selection_is_valid(state, seedSelection)) {
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
            diagnostics.beamCombinationsVisited +=
                diagnostics.combinationsVisited - combinationsBeforeBeam;
            diagnostics.beamEvaluationMicroseconds += std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - beamEvaluationStarted).count();
        }
        struct BranchColumnRank {
            const RouteColumn* column = nullptr;
            BrandMask brands;
            bool matchesEscort = false;
            std::int32_t lifetimeGain = 0;
            std::int32_t dailyGain = 0;
            std::int32_t servingGain = 0;
            std::int32_t preservedServingPotential = 0;
            std::int32_t rawClaims = 0;
        };
        std::vector<std::vector<BranchColumnRank>> branchColumnsByDepth(ordering.size());
        std::vector<std::vector<std::int32_t>> activeEscortGroupsByDepth(ordering.size());
        for (std::size_t depth = 0; depth < ordering.size(); ++depth) {
            branchColumnsByDepth.at(depth).reserve(
                orderedColumns.at(static_cast<std::size_t>(ordering.at(depth))).size());
            activeEscortGroupsByDepth.at(depth).reserve(ordering.size());
        }
        std::optional<OfficialScore> worstCandidateScore;
        const auto refresh_worst_candidate_score = [&]() {
            worstCandidateScore.reset();
            for (const MasterCandidate& candidate : candidates) {
                if (!worstCandidateScore.has_value() ||
                    compare_lexicographic(candidate.scoreAfterToday, *worstCandidateScore) < 0) {
                    worstCandidateScore = candidate.scoreAfterToday;
                }
            }
        };
        refresh_worst_candidate_score();
        const auto search = [&](auto&& self,
                                std::size_t depth,
                BrandMask selectedBrands,
                std::int32_t activeBundle,
                std::int32_t activeSynchronizationConstraints,
                std::int32_t servingUpperBound,
                std::int32_t selectedRawClaims) -> bool {
            if (diagnostics.combinationsVisited >= roundLimit) {
                return false;
            }
            if (deadline_expired()) {
                diagnostics.deadlineReached = true;
                return false;
            }
            if (options.enableLexicographicBranchAndBound && exactMetadata &&
                static_cast<std::int32_t>(candidates.size()) >= options.maximumCandidates) {
                ++diagnostics.upperBoundChecks;
                const OfficialScore branchUpperBound =
                    suffix_upper_bound_score(
                        selectedBrands,
                        depth,
                        servingUpperBound,
                        selectedRawClaims);
                if (compare_lexicographic(
                        branchUpperBound,
                        *worstCandidateScore) < 0) {
                    ++diagnostics.branchesPruned;
                    ++diagnostics.upperBoundPrunes;
                    return true;
                }
                if (options.enableBundleAwareUpperBound) {
                    ++diagnostics.bundleUpperBoundChecks;
                    const std::optional<OfficialScore>
                        bundleUpperBound =
                            active_bundle_upper_bound(
                                activeBundle,
                                selectedBrands,
                                depth,
                                selectedRawClaims);
                    if (!bundleUpperBound.has_value() ||
                        compare_lexicographic(
                            *bundleUpperBound,
                            *worstCandidateScore) < 0) {
                        ++diagnostics.branchesPruned;
                        ++diagnostics.upperBoundPrunes;
                        ++diagnostics.bundleUpperBoundPrunes;
                        return true;
                    }
                }
            }
            if (depth != ordering.size()) {
                const AgentIndex agentIndex = ordering.at(depth);
                std::vector<std::int32_t>& activeEscortGroups =
                    activeEscortGroupsByDepth.at(depth);
                activeEscortGroups.clear();
                for (const RouteColumn* selectedColumn : selected) {
                    if (selectedColumn != nullptr && selectedColumn->escortGroup >= 0 &&
                        std::find(
                            activeEscortGroups.begin(),
                            activeEscortGroups.end(),
                            selectedColumn->escortGroup) == activeEscortGroups.end()) {
                        activeEscortGroups.push_back(selectedColumn->escortGroup);
                    }
                }
                std::vector<BranchColumnRank>& branchColumns =
                    branchColumnsByDepth.at(depth);
                branchColumns.clear();
                std::int32_t servingLossWithoutClaim = 0;
                if (exactMetadata) {
                    for (const SpotIndex spot : claimableSpotsByDepth.at(depth)) {
                        const std::size_t spotOffset = static_cast<std::size_t>(spot);
                        servingLossWithoutClaim +=
                            selectedClaimCounts.at(spotOffset) +
                                    suffixPossibleClaims.at(depth).at(spotOffset) <=
                                config_.spots.at(spotOffset).stock
                            ? 1
                            : 0;
                    }
                }
                for (const RouteColumn* column :
                     orderedColumns.at(static_cast<std::size_t>(agentIndex))) {
                    const bool bundleCompatible =
                        activeBundle == kUnsetBundle ||
                        column->contingencyBundle == activeBundle ||
                        (activeBundle < 0 &&
                         column->contingencyBundle < 0);
                    if (!bundleCompatible) {
                        ++diagnostics.branchesPruned;
                        ++diagnostics.bundlePrunes;
                        continue;
                    }
                    const BrandMask brands = column_brand_mask(config_, *column);
                    std::int32_t servingGain = column->estimatedServings;
                    std::int32_t preservedServingPotential = 0;
                    std::int32_t rawClaims = 0;
                    if (exactMetadata && options.preferStockCappedSearchOrder) {
                        servingGain = 0;
                        for (const ColumnVisitEvent& event : column->firstVisits) {
                            if (!event.claimedServing) {
                                continue;
                            }
                            ++rawClaims;
                            const std::size_t spotOffset =
                                static_cast<std::size_t>(event.spot);
                            servingGain += selectedClaimCounts.at(spotOffset) <
                                    config_.spots.at(spotOffset).stock
                                ? 1
                                : 0;
                            preservedServingPotential +=
                                selectedClaimCounts.at(spotOffset) +
                                        suffixPossibleClaims.at(depth).at(spotOffset) <=
                                    config_.spots.at(spotOffset).stock
                                ? 1
                                : 0;
                        }
                    } else if (exactMetadata) {
                        for (const ColumnVisitEvent& event : column->firstVisits) {
                            if (!event.claimedServing) {
                                continue;
                            }
                            ++rawClaims;
                            const std::size_t spotOffset =
                                static_cast<std::size_t>(event.spot);
                            preservedServingPotential +=
                                selectedClaimCounts.at(spotOffset) +
                                        suffixPossibleClaims.at(depth).at(spotOffset) <=
                                    config_.spots.at(spotOffset).stock
                                ? 1
                                : 0;
                        }
                    }
                    branchColumns.push_back(BranchColumnRank{
                        column,
                        brands,
                        column->escortGroup >= 0 &&
                            std::find(
                                activeEscortGroups.begin(),
                                activeEscortGroups.end(),
                                column->escortGroup) != activeEscortGroups.end(),
                        brand_difference_count(
                            brands,
                            ledger.lifetimeBrands | selectedBrands),
                        brand_difference_count(brands, selectedBrands),
                        servingGain,
                        preservedServingPotential,
                        rawClaims,
                    });
                }
                ++diagnostics.branchOrderingCalls;
                std::stable_sort(
                    branchColumns.begin(),
                    branchColumns.end(),
                    [&options](const BranchColumnRank& left, const BranchColumnRank& right) {
                        if (left.matchesEscort != right.matchesEscort) {
                            return left.matchesEscort;
                        }
                        if (left.lifetimeGain != right.lifetimeGain) {
                            return left.lifetimeGain > right.lifetimeGain;
                        }
                        if (left.dailyGain != right.dailyGain) {
                            return left.dailyGain > right.dailyGain;
                        }
                        if (left.servingGain != right.servingGain) {
                            return left.servingGain > right.servingGain;
                        }
                        if (options.preferBaselineHarvestSources &&
                            left.column->harvestExtensionSourceRank !=
                            right.column->harvestExtensionSourceRank) {
                            return left.column->harvestExtensionSourceRank <
                                right.column->harvestExtensionSourceRank;
                        }
                        return false;
                });
                for (const BranchColumnRank& branchColumn : branchColumns) {
                    const RouteColumn* column = branchColumn.column;
                    selected.at(static_cast<std::size_t>(agentIndex)) = column;
                    const std::int32_t childSynchronizationConstraints =
                        activeSynchronizationConstraints +
                        ((column->escortGroup >= 0 || !column->requiredRefuels.empty()) ? 1 : 0);
                    if (childSynchronizationConstraints > 0) {
                        ++diagnostics.partialSynchronizationChecks;
                    }
                    if (childSynchronizationConstraints > 0 &&
                        !partial_synchronized_selection_is_feasible(
                            state,
                            selected,
                            synchronizationAvailability)) {
                        ++diagnostics.branchesPruned;
                        ++diagnostics.partialSynchronizationPrunes;
                        selected.at(static_cast<std::size_t>(agentIndex)) = nullptr;
                        continue;
                    }
                    for (const ColumnVisitEvent& event : column->firstVisits) {
                        if (event.claimedServing) {
                            ++selectedClaimCounts.at(static_cast<std::size_t>(event.spot));
                        }
                    }
                    const bool continued =
                        self(
                            self,
                            depth + 1U,
                            selectedBrands | branchColumn.brands,
                            activeBundle == kUnsetBundle
                                ? column->contingencyBundle
                                : activeBundle,
                            childSynchronizationConstraints,
                            servingUpperBound - servingLossWithoutClaim +
                                branchColumn.preservedServingPotential,
                            selectedRawClaims + branchColumn.rawClaims);
                    for (const ColumnVisitEvent& event : column->firstVisits) {
                        if (event.claimedServing) {
                            --selectedClaimCounts.at(static_cast<std::size_t>(event.spot));
                        }
                    }
                    if (!continued) {
                        selected.at(static_cast<std::size_t>(agentIndex)) = nullptr;
                        return false;
                    }
                }
                selected.at(static_cast<std::size_t>(agentIndex)) = nullptr;
                return true;
            }
            ++diagnostics.combinationsVisited;
            if (activeSynchronizationConstraints > 0 &&
                !synchronized_selection_is_valid(state, selected)) {
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
            if (!worstCandidateScore.has_value() ||
                compare_lexicographic(
                    candidates.back().scoreAfterToday,
                    *worstCandidateScore) < 0) {
                worstCandidateScore = candidates.back().scoreAfterToday;
            }
            if (static_cast<std::int32_t>(candidates.size()) > options.maximumCandidates * 2) {
                retain_alns_population(
                    candidates,
                    options.maximumCandidates,
                    options.diversityCandidates);
                refresh_worst_candidate_score();
            }
            return true;
        };
        const std::int32_t combinationsBeforeDepthFirst = diagnostics.combinationsVisited;
        const std::chrono::steady_clock::time_point depthFirstSearchStarted =
            std::chrono::steady_clock::now();
        const bool roundCompleted = search(
            search,
            0U,
            0,
            kUnsetBundle,
            0,
            rootServingUpperBound,
            0);
        diagnostics.depthFirstCombinationsVisited +=
            diagnostics.combinationsVisited - combinationsBeforeDepthFirst;
        diagnostics.depthFirstSearchMicroseconds += std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - depthFirstSearchStarted).count();
        diagnostics.searchComplete = diagnostics.searchComplete || roundCompleted;
        if (diagnostics.deadlineReached || roundCompleted || !learnedCut) {
            break;
        }
    }
    const std::chrono::steady_clock::time_point populationMaintenanceStarted =
        std::chrono::steady_clock::now();
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
    diagnostics.populationMaintenanceMicroseconds += std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - populationMaintenanceStarted).count();
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
                const BrandMask coveredAfterCandidate =
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
    const auto deadline_expired = [&options]() {
        return options.deadline.has_value() && std::chrono::steady_clock::now() >= *options.deadline;
    };
    std::array<std::int32_t, kAlnsOperatorCount> weights{8, 6, 5, 4, 5, 5, 4, 6};
    std::array<std::int32_t, kAlnsOperatorCount> deficits{};
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
    BrandMask allBrands;
    for (std::int32_t brand = 0; brand < config_.brand_count(); ++brand) {
        allBrands |= brand_bit(brand);
    }
    std::vector<BrandMask> directReachableByAgent(
        static_cast<std::size_t>(config_.agent_count()),
        allBrands);
    std::vector<BrandMask> escortedReachableByAgent(
        static_cast<std::size_t>(config_.agent_count()),
        allBrands);
    std::vector<BrandMask> stationReachableByAgent(
        static_cast<std::size_t>(config_.agent_count()),
        allBrands);
    for (AgentIndex agentIndex = 0; agentIndex < config_.agent_count(); ++agentIndex) {
        if (deadline_expired()) {
            break;
        }
        BrandMask directReachable;
        BrandMask escortedReachable;
        BrandMask stationReachable;
        bool agentScanComplete = true;
        ParetoSearchOptions options;
        options.maximumTravelSteps = totalSteps;
        options.maximumPatrolFuel = config_.fuelLimit;
        options.maximumLabelsPerCell = 4;
        options.maximumPaths = 1;
        options.patrol = true;
        options.deadline = deadline;
        for (const Spot& spot : config_.spots) {
            if (deadline_expired()) {
                agentScanComplete = false;
                break;
            }
            ParetoSearchDiagnostics directDiagnostics;
            const std::vector<ParetoPath> paths = router.find_paths(
                config_.initialAgents.at(static_cast<std::size_t>(agentIndex)),
                spot.position,
                smooth,
                options,
                &directDiagnostics);
            if (!paths.empty()) {
                directReachable |= brand_bit(spot.brandIndex);
                if (std::any_of(
                        paths.begin(),
                        paths.end(),
                        [this](const ParetoPath& path) {
                            return path.travelSteps <= config_.steps_for_day(1);
                        })) {
                    stationReachable |= brand_bit(spot.brandIndex);
                }
            }
            if (directDiagnostics.deadlineRejectedQueries > 0 ||
                directDiagnostics.deadlineInterruptedQueries > 0) {
                agentScanComplete = false;
                break;
            }
            options.maximumPatrolFuel = std::numeric_limits<std::int32_t>::max() / 4;
            ParetoSearchDiagnostics escortedDiagnostics;
            const std::vector<ParetoPath> escortedPaths = router.find_paths(
                config_.initialAgents.at(static_cast<std::size_t>(agentIndex)),
                spot.position,
                smooth,
                options,
                &escortedDiagnostics);
            if (!escortedPaths.empty()) {
                escortedReachable |= brand_bit(spot.brandIndex);
            }
            options.maximumPatrolFuel = config_.fuelLimit;
            if (escortedDiagnostics.deadlineRejectedQueries > 0 ||
                escortedDiagnostics.deadlineInterruptedQueries > 0) {
                agentScanComplete = false;
                break;
            }
        }
        if (!agentScanComplete) {
            break;
        }
        directReachableByAgent.at(static_cast<std::size_t>(agentIndex)) = directReachable;
        escortedReachableByAgent.at(static_cast<std::size_t>(agentIndex)) = escortedReachable;
        stationReachableByAgent.at(static_cast<std::size_t>(agentIndex)) = stationReachable;
    }
    const auto maximum_matching_coverage =
        [this](const std::vector<BrandMask>& reachableByAgent,
               const std::vector<AgentKind>& roles) {
            std::vector<AgentIndex> matchedAgentByBrand(
                static_cast<std::size_t>(config_.brand_count()),
                kInvalidAgent);
            const auto augment =
                [&](auto&& self,
                    AgentIndex agentIndex,
                    std::vector<bool>& visitedBrands) -> bool {
                    const BrandMask& reachable =
                        reachableByAgent.at(static_cast<std::size_t>(agentIndex));
                    for (std::int32_t brandIndex = 0;
                         brandIndex < config_.brand_count();
                         ++brandIndex) {
                        if (!has_brand(reachable, brandIndex) ||
                            visitedBrands.at(static_cast<std::size_t>(brandIndex))) {
                            continue;
                        }
                        visitedBrands.at(static_cast<std::size_t>(brandIndex)) = true;
                        AgentIndex& matched =
                            matchedAgentByBrand.at(static_cast<std::size_t>(brandIndex));
                        if (matched == kInvalidAgent ||
                            self(self, matched, visitedBrands)) {
                            matched = agentIndex;
                            return true;
                        }
                    }
                    return false;
                };
            std::int32_t coverage = 0;
            for (AgentIndex agentIndex = 0;
                 agentIndex < config_.agent_count();
                 ++agentIndex) {
                if (roles.at(static_cast<std::size_t>(agentIndex)) != AgentKind::Patrol) {
                    continue;
                }
                std::vector<bool> visitedBrands(
                    static_cast<std::size_t>(config_.brand_count()),
                    false);
                coverage += augment(augment, agentIndex, visitedBrands) ? 1 : 0;
            }
            return coverage;
        };
    std::int32_t perDayServings = 0;
    for (const Spot& spot : config_.spots) {
        perDayServings += spot.stock;
    }
    const OfficialScore feasibleSeedLowerBound{};
    std::vector<RoleAssignment> assignments;
    assignments.reserve(assignmentCount - 1U);
    for (std::uint32_t mask = 0; mask < assignmentCount; ++mask) {
        RoleAssignment assignment;
        assignment.roles.resize(static_cast<std::size_t>(config_.agent_count()), AgentKind::Patrol);
        BrandMask reachableBrands;
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
        const std::int32_t lifetime = brand_count(reachableBrands);
        const std::int32_t daily = lifetime * config_.day_count();
        const std::int32_t servingCapPerDay = std::min(
            perDayServings,
            assignment.patrolCount * static_cast<std::int32_t>(config_.spots.size()));
        assignment.cheapUpperBound = OfficialScore{
            lifetime,
            daily,
            servingCapPerDay * config_.day_count(),
        };
        const bool fuelCoversWorstCaseHorizon =
            config_.fuelLimit >= 2 * totalSteps;
        assignment.sustainableCoverage = maximum_matching_coverage(
            tankerCount > 0
                ? escortedReachableByAgent
                : (fuelCoversWorstCaseHorizon
                    ? directReachableByAgent
                    : stationReachableByAgent),
            assignment.roles);
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
            if (left.sustainableCoverage != right.sustainableCoverage) {
                return left.sustainableCoverage > right.sustainableCoverage;
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
