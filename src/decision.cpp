#include "udon/decision.hpp"
#include "udon/protocol.hpp"
#include "blank_slate/planners.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace udon {

namespace {

[[nodiscard]] std::int32_t maximum_day_steps(const MatchConfig& config) {
    return *std::max_element(config.daySteps.begin(), config.daySteps.end());
}

[[nodiscard]] std::int32_t harvest_extension_depth(
    const MatchConfig& config,
    std::int32_t dayNumber,
    std::int32_t mode) {
    if (mode > 4 &&
        static_cast<std::int64_t>(config.fuelLimit) >=
            3LL * config.steps_for_day(dayNumber)) {
        return 4;
    }
    return mode > 3 ? 3 : 2;
}

[[nodiscard]] std::int32_t maximum_total_occupancy(const MatchConfig& config) {
    const std::int64_t value = static_cast<std::int64_t>(2) * config.players * config.agent_count() *
        maximum_day_steps(config);
    return value > std::numeric_limits<std::int32_t>::max()
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(value);
}

[[nodiscard]] std::int64_t maximum_other_occupancy_for_day(
    const MatchConfig& config,
    std::int32_t dayNumber) {
    if (dayNumber < 1 || dayNumber > config.day_count()) {
        return 0;
    }
    return static_cast<std::int64_t>(config.players - 1) * config.agent_count() * config.steps_for_day(dayNumber);
}

[[nodiscard]] RoadLoadInterval total_load_interval(const MatchConfig& config, RoadStatus status) {
    const auto bounded = [](std::int64_t value) {
        return static_cast<std::int32_t>(std::min<std::int64_t>(
            std::numeric_limits<std::int32_t>::max(),
            std::max<std::int64_t>(0, value)));
    };
    const std::int32_t busy = bounded(
        static_cast<std::int64_t>(config.players) *
        config.busyThreshold);
    const std::int32_t jammed = bounded(
        static_cast<std::int64_t>(config.players) *
        config.jammedThreshold);
    switch (status) {
    case RoadStatus::Smooth:
        return RoadLoadInterval{0, std::max(0, busy - 1)};
    case RoadStatus::Busy:
        return RoadLoadInterval{busy, std::max(busy, jammed - 1)};
    case RoadStatus::Jammed:
        return RoadLoadInterval{jammed, maximum_total_occupancy(config)};
    }
    return RoadLoadInterval{};
}

[[nodiscard]] RoadStatus classify_traffic(
    const MatchConfig& config,
    std::int64_t totalStays) {
    const std::int64_t busy = static_cast<std::int64_t>(config.players) * config.busyThreshold;
    const std::int64_t jammed = static_cast<std::int64_t>(config.players) * config.jammedThreshold;
    if (totalStays < busy) {
        return RoadStatus::Smooth;
    }
    if (totalStays < jammed) {
        return RoadStatus::Busy;
    }
    return RoadStatus::Jammed;
}

[[nodiscard]] std::vector<std::int32_t> shortest_travel_times(
    const MatchConfig& config,
    const DayState& state,
    CellId source) {
    using Entry = std::pair<std::int32_t, CellId>;
    std::vector<std::int32_t> distance(
        static_cast<std::size_t>(config.map.cell_count()),
        std::numeric_limits<std::int32_t>::max());
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> queue;
    distance.at(static_cast<std::size_t>(source)) = 0;
    queue.push(Entry{0, source});
    while (!queue.empty()) {
        const auto [time, cell] = queue.top();
        queue.pop();
        if (time != distance.at(static_cast<std::size_t>(cell))) {
            continue;
        }
        const MoveCost cost = config.move_cost(cell, state.roadStatuses.at(static_cast<std::size_t>(cell)));
        for (std::int32_t direction = 0; direction < kDirectionCount; ++direction) {
            const CellId next = config.map.neighbors.at(static_cast<std::size_t>(cell)).at(static_cast<std::size_t>(direction));
            if (next == kInvalidCell || config.map.terrain.at(static_cast<std::size_t>(next)) == Terrain::Pond) {
                continue;
            }
            const std::int32_t candidate = time + cost.steps;
            if (candidate >= distance.at(static_cast<std::size_t>(next))) {
                continue;
            }
            distance.at(static_cast<std::size_t>(next)) = candidate;
            queue.push(Entry{candidate, next});
        }
    }
    return distance;
}

[[nodiscard]] std::vector<CellId> shortest_public_path(
    const MatchConfig& config,
    const DayState& state,
    CellId source,
    CellId target) {
    using Entry = std::pair<std::int32_t, CellId>;
    const std::size_t cellCount = static_cast<std::size_t>(config.map.cell_count());
    std::vector<std::int32_t> distance(cellCount, std::numeric_limits<std::int32_t>::max());
    std::vector<CellId> parent(cellCount, kInvalidCell);
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> queue;
    distance.at(static_cast<std::size_t>(source)) = 0;
    queue.push(Entry{0, source});
    while (!queue.empty()) {
        const auto [time, cell] = queue.top();
        queue.pop();
        if (time != distance.at(static_cast<std::size_t>(cell))) {
            continue;
        }
        if (cell == target) {
            break;
        }
        const MoveCost cost = config.move_cost(cell, state.roadStatuses.at(static_cast<std::size_t>(cell)));
        for (std::int32_t direction = 0; direction < kDirectionCount; ++direction) {
            const CellId next = config.map.neighbors.at(static_cast<std::size_t>(cell)).at(
                static_cast<std::size_t>(direction));
            if (next == kInvalidCell || config.map.terrain.at(static_cast<std::size_t>(next)) == Terrain::Pond) {
                continue;
            }
            const std::int32_t candidate = time + cost.steps;
            if (candidate < distance.at(static_cast<std::size_t>(next)) ||
                (candidate == distance.at(static_cast<std::size_t>(next)) && cell < parent.at(static_cast<std::size_t>(next)))) {
                distance.at(static_cast<std::size_t>(next)) = candidate;
                parent.at(static_cast<std::size_t>(next)) = cell;
                queue.push(Entry{candidate, next});
            }
        }
    }
    if (distance.at(static_cast<std::size_t>(target)) == std::numeric_limits<std::int32_t>::max()) {
        return {};
    }
    std::vector<CellId> reversed;
    for (CellId cell = target; cell != kInvalidCell; cell = parent.at(static_cast<std::size_t>(cell))) {
        reversed.push_back(cell);
        if (cell == source) {
            break;
        }
    }
    if (reversed.empty() || reversed.back() != source) {
        return {};
    }
    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}

[[nodiscard]] std::optional<CellId> previous_public_opponent_position(
    const TrafficBelief& belief,
    std::int32_t teamId,
    std::size_t agentIndex) {
    const std::vector<std::vector<OtherTeamState>>& history = belief.public_opponent_history();
    if (history.size() < 2U) {
        return std::nullopt;
    }
    const std::vector<OtherTeamState>& previousTeams = history.at(history.size() - 2U);
    const auto team = std::find_if(
        previousTeams.begin(),
        previousTeams.end(),
        [teamId](const OtherTeamState& candidate) { return candidate.teamId == teamId; });
    if (team == previousTeams.end() || agentIndex >= team->agents.size()) {
        return std::nullopt;
    }
    return team->agents.at(agentIndex).position;
}

[[nodiscard]] std::vector<std::int32_t> likely_public_footprint(
    const MatchConfig& config,
    const DayState& state,
    const TrafficBelief& belief) {
    const std::size_t cellCount = static_cast<std::size_t>(config.map.cell_count());
    std::vector<std::int32_t> footprint(cellCount, 0);
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    for (const OtherTeamState& team : state.others) {
        for (std::size_t agentIndex = 0; agentIndex < team.agents.size(); ++agentIndex) {
            const AgentState& agent = team.agents.at(agentIndex);
            const std::vector<std::int32_t> currentDistance = shortest_travel_times(config, state, agent.position);
            std::optional<CellId> previousPosition = previous_public_opponent_position(belief, team.teamId, agentIndex);
            std::vector<std::int32_t> previousDistance;
            if (previousPosition.has_value()) {
                previousDistance = shortest_travel_times(config, state, *previousPosition);
            }
            CellId target = kInvalidCell;
            std::int64_t targetScore = std::numeric_limits<std::int64_t>::max();
            for (const Spot& spot : config.spots) {
                const std::int32_t travel = currentDistance.at(static_cast<std::size_t>(spot.position));
                if (travel == std::numeric_limits<std::int32_t>::max()) {
                    continue;
                }
                std::int32_t progress = 0;
                if (!previousDistance.empty()) {
                    const std::int32_t previous = previousDistance.at(static_cast<std::size_t>(spot.position));
                    if (previous != std::numeric_limits<std::int32_t>::max()) {
                        progress = std::max(0, previous - travel);
                    }
                }
                const std::int64_t score = static_cast<std::int64_t>(travel) * 100 - progress;
                if (score < targetScore || (score == targetScore && spot.position < target)) {
                    targetScore = score;
                    target = spot.position;
                }
            }
            if (target == kInvalidCell) {
                continue;
            }
            const std::vector<CellId> path = shortest_public_path(config, state, agent.position, target);
            std::int32_t elapsed = 0;
            std::int32_t fuelUsed = 0;
            for (std::size_t pathIndex = 0; pathIndex + 1U < path.size(); ++pathIndex) {
                const CellId cell = path.at(pathIndex);
                const MoveCost cost = config.move_cost(cell, state.roadStatuses.at(static_cast<std::size_t>(cell)));
                if (elapsed + cost.steps > daySteps ||
                    (agent.kind == AgentKind::Patrol && fuelUsed + cost.patrolFuel > agent.fuel)) {
                    break;
                }
                if (config.map.terrain.at(static_cast<std::size_t>(cell)) == Terrain::Road) {
                    footprint.at(static_cast<std::size_t>(cell)) += cost.steps;
                }
                elapsed += cost.steps;
                fuelUsed += agent.kind == AgentKind::Patrol ? cost.patrolFuel : 0;
            }
        }
    }
    return footprint;
}

[[nodiscard]] std::vector<std::int32_t> fuel_aware_travel_times(
    const MatchConfig& config,
    const AgentState& agent,
    RoadStatus assumedRoadStatus,
    std::int32_t fuelBudget) {
    struct Label {
        std::int32_t time = 0;
        std::int32_t fuelUsed = 0;
    };
    using QueueEntry = std::tuple<std::int32_t, std::int32_t, CellId>;
    const std::size_t cellCount = static_cast<std::size_t>(config.map.cell_count());
    std::vector<std::vector<Label>> labels(cellCount);
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> queue;
    labels.at(static_cast<std::size_t>(agent.position)).push_back(Label{});
    queue.emplace(0, 0, agent.position);
    const auto accept_label = [&labels](CellId cell, Label candidate) {
        std::vector<Label>& atCell = labels.at(static_cast<std::size_t>(cell));
        for (const Label& existing : atCell) {
            if (existing.time <= candidate.time && existing.fuelUsed <= candidate.fuelUsed) {
                return false;
            }
        }
        atCell.erase(
            std::remove_if(
                atCell.begin(),
                atCell.end(),
                [&candidate](const Label& existing) {
                    return candidate.time <= existing.time && candidate.fuelUsed <= existing.fuelUsed;
                }),
            atCell.end());
        atCell.push_back(candidate);
        return true;
    };
    while (!queue.empty()) {
        const auto [time, fuelUsed, cell] = queue.top();
        queue.pop();
        const std::vector<Label>& active = labels.at(static_cast<std::size_t>(cell));
        if (std::none_of(
                active.begin(),
                active.end(),
                [time, fuelUsed](const Label& label) { return label.time == time && label.fuelUsed == fuelUsed; })) {
            continue;
        }
        const RoadStatus status = config.map.terrain.at(static_cast<std::size_t>(cell)) == Terrain::Road
            ? assumedRoadStatus
            : RoadStatus::Smooth;
        const MoveCost cost = config.move_cost(cell, status);
        for (std::int32_t direction = 0; direction < kDirectionCount; ++direction) {
            const CellId next = config.map.neighbors.at(static_cast<std::size_t>(cell)).at(
                static_cast<std::size_t>(direction));
            if (next == kInvalidCell || config.map.terrain.at(static_cast<std::size_t>(next)) == Terrain::Pond) {
                continue;
            }
            const std::int32_t nextFuel = fuelUsed + (agent.kind == AgentKind::Patrol ? cost.patrolFuel : 0);
            if (nextFuel > fuelBudget) {
                continue;
            }
            const Label candidate{time + cost.steps, nextFuel};
            if (accept_label(next, candidate)) {
                queue.emplace(candidate.time, candidate.fuelUsed, next);
            }
        }
    }
    std::vector<std::int32_t> earliest(cellCount, std::numeric_limits<std::int32_t>::max());
    for (CellId cell = 0; cell < config.map.cell_count(); ++cell) {
        for (const Label& label : labels.at(static_cast<std::size_t>(cell))) {
            earliest.at(static_cast<std::size_t>(cell)) = std::min(
                earliest.at(static_cast<std::size_t>(cell)),
                label.time);
        }
    }
    return earliest;
}

[[nodiscard]] std::int32_t relaxed_brand_flow(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    const std::vector<std::vector<std::int32_t>>& travelByPatrolBrand) {
    struct AgentDaySlot {
        AgentIndex agent = kInvalidAgent;
        std::int32_t cumulativeSteps = 0;
    };
    std::vector<std::int32_t> missingBrands;
    for (std::int32_t brandIndex = 0; brandIndex < config.brand_count(); ++brandIndex) {
        if (!has_brand(ledger.lifetimeBrands, brandIndex)) {
            missingBrands.push_back(brandIndex);
        }
    }
    if (missingBrands.empty() || travelByPatrolBrand.empty()) {
        return 0;
    }
    std::vector<AgentDaySlot> slots;
    const std::int32_t maximumCopies = static_cast<std::int32_t>(missingBrands.size());
    for (std::size_t patrolOffset = 0; patrolOffset < travelByPatrolBrand.size(); ++patrolOffset) {
        std::int32_t cumulativeSteps = 0;
        for (std::int32_t dayNumber = state.dayNumber; dayNumber <= config.day_count(); ++dayNumber) {
            cumulativeSteps += config.steps_for_day(dayNumber);
            const std::int32_t capacity = std::min(maximumCopies, std::max(1, config.steps_for_day(dayNumber)));
            for (std::int32_t copy = 0; copy < capacity; ++copy) {
                slots.push_back(AgentDaySlot{static_cast<AgentIndex>(patrolOffset), cumulativeSteps});
            }
        }
    }
    std::vector<std::int32_t> slotOwner(slots.size(), -1);
    std::function<bool(std::int32_t, std::vector<bool>&)> augment =
        [&missingBrands, &slots, &slotOwner, &travelByPatrolBrand, &augment](std::int32_t brandOffset, std::vector<bool>& seen) {
            const std::int32_t brandIndex = missingBrands.at(static_cast<std::size_t>(brandOffset));
            for (std::size_t slotIndex = 0; slotIndex < slots.size(); ++slotIndex) {
                if (seen.at(slotIndex)) {
                    continue;
                }
                const AgentDaySlot& slot = slots.at(slotIndex);
                const std::int32_t travel = travelByPatrolBrand.at(static_cast<std::size_t>(slot.agent)).at(
                    static_cast<std::size_t>(brandIndex));
                if (travel > slot.cumulativeSteps) {
                    continue;
                }
                seen.at(slotIndex) = true;
                if (slotOwner.at(slotIndex) < 0 || augment(slotOwner.at(slotIndex), seen)) {
                    slotOwner.at(slotIndex) = brandOffset;
                    return true;
                }
            }
            return false;
        };
    std::int32_t matched = 0;
    for (std::int32_t brandOffset = 0; brandOffset < static_cast<std::int32_t>(missingBrands.size()); ++brandOffset) {
        std::vector<bool> seen(slots.size(), false);
        if (augment(brandOffset, seen)) {
            ++matched;
        }
    }
    return matched;
}

[[nodiscard]] std::int32_t maximum_road_dwell(std::int32_t daySteps, std::int32_t travelSteps) {
    if (travelSteps < 0 || travelSteps > daySteps) {
        return 0;
    }
    return travelSteps == 0 ? daySteps : daySteps - travelSteps + 1;
}

[[nodiscard]] std::vector<std::int32_t> direct_reachable_dwell(
    const MatchConfig& config,
    const DayState& state,
    const AgentState& agent) {
    struct Label {
        std::int32_t time = 0;
        std::int32_t fuel = 0;
    };
    using QueueEntry = std::tuple<std::int32_t, std::int32_t, CellId>;
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    const std::size_t cellCount = static_cast<std::size_t>(config.map.cell_count());
    std::vector<std::vector<Label>> labels(cellCount);
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> queue;
    labels.at(static_cast<std::size_t>(agent.position)).push_back(Label{});
    queue.emplace(0, 0, agent.position);
    const auto accept_label = [&labels](CellId cell, Label candidate) {
        std::vector<Label>& atCell = labels.at(static_cast<std::size_t>(cell));
        for (const Label& existing : atCell) {
            if (existing.time <= candidate.time && existing.fuel <= candidate.fuel) {
                return false;
            }
        }
        atCell.erase(
            std::remove_if(
                atCell.begin(),
                atCell.end(),
                [&candidate](const Label& existing) {
                    return candidate.time <= existing.time && candidate.fuel <= existing.fuel;
                }),
            atCell.end());
        atCell.push_back(candidate);
        return true;
    };
    while (!queue.empty()) {
        const auto [time, fuel, cell] = queue.top();
        queue.pop();
        const std::vector<Label>& atCell = labels.at(static_cast<std::size_t>(cell));
        const bool stillActive = std::any_of(
            atCell.begin(),
            atCell.end(),
            [time, fuel](const Label& label) { return label.time == time && label.fuel == fuel; });
        if (!stillActive) {
            continue;
        }
        const MoveCost cost = config.move_cost(
            cell,
            state.roadStatuses.at(static_cast<std::size_t>(cell)));
        for (std::int32_t direction = 0; direction < kDirectionCount; ++direction) {
            const CellId next = config.map.neighbors.at(static_cast<std::size_t>(cell)).at(
                static_cast<std::size_t>(direction));
            if (next == kInvalidCell || config.map.terrain.at(static_cast<std::size_t>(next)) == Terrain::Pond) {
                continue;
            }
            const std::int32_t nextTime = time + cost.steps;
            const std::int32_t nextFuel = fuel + (agent.kind == AgentKind::Patrol ? cost.patrolFuel : 0);
            if (nextTime > daySteps || nextFuel > agent.fuel) {
                continue;
            }
            if (accept_label(next, Label{nextTime, nextFuel})) {
                queue.emplace(nextTime, nextFuel, next);
            }
        }
    }
    std::vector<std::int32_t> dwell(cellCount, 0);
    for (const CellId road : config.roadCells) {
        const std::vector<Label>& atRoad = labels.at(static_cast<std::size_t>(road));
        std::int32_t earliest = std::numeric_limits<std::int32_t>::max();
        for (const Label& label : atRoad) {
            earliest = std::min(earliest, label.time);
        }
        if (earliest != std::numeric_limits<std::int32_t>::max()) {
            dwell.at(static_cast<std::size_t>(road)) = maximum_road_dwell(daySteps, earliest);
        }
    }
    return dwell;
}

[[nodiscard]] bool same_agent_states(
    const std::vector<AgentState>& left,
    const std::vector<AgentState>& right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.size(); ++index) {
        const AgentState& lhs = left.at(index);
        const AgentState& rhs = right.at(index);
        if (lhs.kind != rhs.kind || lhs.position != rhs.position || lhs.fuel != rhs.fuel) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool all_outcomes_certified(const CandidateProfile& profile) {
    return !profile.outcomes.empty() && std::all_of(
        profile.outcomes.begin(),
        profile.outcomes.end(),
        [](const ScenarioOutcome& outcome) { return outcome.witness.certified; });
}

[[nodiscard]] std::int32_t score_component(const OfficialScore& score, std::size_t index) {
    switch (index) {
    case 0:
        return score.lifetimeDistinct;
    case 1:
        return score.totalDailyDistinct;
    case 2:
        return score.totalServings;
    default:
        throw std::out_of_range("official score component is outside the lexicographic vector");
    }
}

[[nodiscard]] std::int32_t road_degree(const MatchConfig& config, CellId road) {
    std::int32_t degree = 0;
    for (std::int32_t direction = 0; direction < kDirectionCount; ++direction) {
        const CellId neighbor = config.map.neighbors.at(static_cast<std::size_t>(road)).at(static_cast<std::size_t>(direction));
        if (neighbor != kInvalidCell && config.map.terrain.at(static_cast<std::size_t>(neighbor)) != Terrain::Pond) {
            ++degree;
        }
    }
    return degree;
}

[[nodiscard]] bool road_touches_spot(const MatchConfig& config, CellId road) {
    for (std::int32_t direction = 0; direction < kDirectionCount; ++direction) {
        const CellId neighbor = config.map.neighbors.at(static_cast<std::size_t>(road)).at(static_cast<std::size_t>(direction));
        if (neighbor != kInvalidCell && config.spotAtCell.at(static_cast<std::size_t>(neighbor)) != kInvalidSpot) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::vector<std::int32_t> midpoint_carry(const TrafficBelief& belief) {
    const std::vector<RoadLoadInterval>& intervals = belief.opponent_carry();
    std::vector<std::int32_t> result(intervals.size(), 0);
    for (std::size_t roadOffset = 0; roadOffset < intervals.size(); ++roadOffset) {
        result.at(roadOffset) = intervals.at(roadOffset).minimum +
            (intervals.at(roadOffset).maximum - intervals.at(roadOffset).minimum) / 2;
    }
    return result;
}

[[nodiscard]] std::vector<std::int32_t> pessimistic_carry(const TrafficBelief& belief) {
    const std::vector<RoadLoadInterval>& intervals = belief.opponent_carry();
    std::vector<std::int32_t> result(intervals.size(), 0);
    for (std::size_t roadOffset = 0; roadOffset < intervals.size(); ++roadOffset) {
        result.at(roadOffset) = intervals.at(roadOffset).maximum;
    }
    return result;
}

[[nodiscard]] std::vector<RoadStatus> predict_with_components(
    const MatchConfig& config,
    const std::vector<std::int32_t>& previousOwn,
    const std::vector<std::int32_t>& currentOwn,
    const std::vector<std::int32_t>& opponentCarry,
    const std::vector<std::int32_t>& opponentCurrent) {
    const std::size_t expected = static_cast<std::size_t>(config.map.cell_count());
    if (previousOwn.size() != expected || currentOwn.size() != expected || opponentCarry.size() != expected ||
        opponentCurrent.size() != expected) {
        throw std::invalid_argument("traffic scenario vectors do not match the map");
    }
    std::vector<RoadStatus> result(expected, RoadStatus::Smooth);
    for (const CellId road : config.roadCells) {
        const std::size_t roadOffset = static_cast<std::size_t>(road);
        const std::int64_t total = static_cast<std::int64_t>(previousOwn.at(roadOffset)) +
            currentOwn.at(roadOffset) + opponentCarry.at(roadOffset) + opponentCurrent.at(roadOffset);
        result.at(roadOffset) = classify_traffic(config, std::max<std::int64_t>(0, total));
    }
    return result;
}

[[nodiscard]] TrafficSafety counterfactual_traffic_safety(
    const MatchConfig& config,
    const TrafficBelief& belief,
    const ScenarioManifest& manifest,
    const SimulationResult& simulation) {
    TrafficSafety safety;
    for (const CellId road : config.roadCells) {
        safety.totalRoadStays += simulation.roadFootprint.at(static_cast<std::size_t>(road));
    }
    for (const TrafficScenario& scenario : manifest.scenarios) {
        std::int32_t crossings = 0;
        std::int32_t thresholdBandRoads = 0;
        for (const CellId road : config.roadCells) {
            const std::size_t roadOffset = static_cast<std::size_t>(road);
            const std::int64_t baseline = static_cast<std::int64_t>(belief.previous_own_footprint().at(roadOffset)) +
                scenario.opponentCarryFootprint.at(roadOffset) + scenario.opponentCurrentFootprint.at(roadOffset);
            const std::int64_t withOwn = baseline + simulation.roadFootprint.at(roadOffset);
            const RoadStatus before = classify_traffic(config, std::max<std::int64_t>(0, baseline));
            const RoadStatus after = classify_traffic(config, std::max<std::int64_t>(0, withOwn));
            if (static_cast<std::int32_t>(after) > static_cast<std::int32_t>(before)) {
                ++crossings;
            }
            const std::int64_t busy = static_cast<std::int64_t>(config.players) * config.busyThreshold;
            const std::int64_t jammed = static_cast<std::int64_t>(config.players) * config.jammedThreshold;
            if (simulation.roadFootprint.at(roadOffset) > 0 && (withOwn == busy - 1 || withOwn == jammed - 1)) {
                ++thresholdBandRoads;
            }
        }
        safety.thresholdCrossings = std::max(safety.thresholdCrossings, crossings);
        safety.thresholdBandRoads = std::max(safety.thresholdBandRoads, thresholdBandRoads);
    }
    return safety;
}

[[nodiscard]] std::vector<CellId> promoted_critical_roads(
    const MatchConfig& config,
    const TrafficBelief& belief,
    const ScenarioManifest& manifest,
    const std::vector<MasterCandidate>& candidates,
    std::int32_t limit = 6) {
    if (limit <= 0) {
        return {};
    }
    std::map<CellId, std::int64_t> urgency;
    for (const MasterCandidate& candidate : candidates) {
        for (const TrafficScenario& scenario : manifest.scenarios) {
            for (const CellId road : config.roadCells) {
                const std::size_t roadOffset = static_cast<std::size_t>(road);
                const std::int64_t baseline = belief.previous_own_footprint().at(roadOffset) +
                    scenario.opponentCarryFootprint.at(roadOffset) + scenario.opponentCurrentFootprint.at(roadOffset);
                const std::int64_t withOwn = baseline + candidate.simulation.roadFootprint.at(roadOffset);
                const RoadStatus before = classify_traffic(config, std::max<std::int64_t>(0, baseline));
                const RoadStatus after = classify_traffic(config, std::max<std::int64_t>(0, withOwn));
                const std::int64_t busy = static_cast<std::int64_t>(config.players) * config.busyThreshold;
                const std::int64_t jammed = static_cast<std::int64_t>(config.players) * config.jammedThreshold;
                const bool crossing = static_cast<std::int32_t>(after) > static_cast<std::int32_t>(before);
                const bool nearThreshold = candidate.simulation.roadFootprint.at(roadOffset) > 0 &&
                    (withOwn == busy - 1 || withOwn == jammed - 1);
                if (crossing || nearThreshold) {
                    urgency[road] += static_cast<std::int64_t>(scenario.weight) *
                        (crossing ? 1000 : 100) + candidate.simulation.roadFootprint.at(roadOffset);
                }
            }
        }
    }
    std::vector<std::pair<CellId, std::int64_t>> ordered(urgency.begin(), urgency.end());
    std::sort(
        ordered.begin(),
        ordered.end(),
        [](const auto& left, const auto& right) {
            if (left.second != right.second) {
                return left.second > right.second;
            }
            return left.first < right.first;
        });
    std::vector<CellId> result;
    result.reserve(static_cast<std::size_t>(std::max(0, limit)));
    for (const auto& [road, score] : ordered) {
        static_cast<void>(score);
        result.push_back(road);
        if (static_cast<std::int32_t>(result.size()) >= limit) {
            break;
        }
    }
    return result;
}

[[nodiscard]] std::vector<CellId> baseline_critical_roads(
    const MatchConfig& config,
    const DayState& state,
    std::int32_t limit = 6) {
    std::vector<CellId> roads;
    roads.reserve(static_cast<std::size_t>(std::max(0, limit)));
    for (const RoadStatus status : {RoadStatus::Jammed, RoadStatus::Busy}) {
        for (const CellId road : config.roadCells) {
            if (state.roadStatuses.at(static_cast<std::size_t>(road)) != status) {
                continue;
            }
            roads.push_back(road);
            if (static_cast<std::int32_t>(roads.size()) >= limit) {
                return roads;
            }
        }
    }
    return roads;
}

void merge_master_diagnostics(MasterDiagnostics& target, const MasterDiagnostics& addition) {
    const bool hadSearch = target.cutRounds > 0;
    target.combinationsVisited += addition.combinationsVisited;
    target.beamCombinationsVisited += addition.beamCombinationsVisited;
    target.depthFirstCombinationsVisited += addition.depthFirstCombinationsVisited;
    target.branchOrderingCalls += addition.branchOrderingCalls;
    target.upperBoundChecks += addition.upperBoundChecks;
    target.upperBoundPrunes += addition.upperBoundPrunes;
    target.bundleUpperBoundChecks +=
        addition.bundleUpperBoundChecks;
    target.bundleUpperBoundPrunes +=
        addition.bundleUpperBoundPrunes;
    target.bundleBrandFrontierStates +=
        addition.bundleBrandFrontierStates;
    target.bundleBrandFrontierFallbacks +=
        addition.bundleBrandFrontierFallbacks;
    target.bundlePrunes += addition.bundlePrunes;
    target.exactBundlesDiscovered += addition.exactBundlesDiscovered;
    target.exactBundlesEvaluated += addition.exactBundlesEvaluated;
    target.exactBundlesAccepted += addition.exactBundlesAccepted;
    target.partialSynchronizationChecks += addition.partialSynchronizationChecks;
    target.partialSynchronizationPrunes += addition.partialSynchronizationPrunes;
    target.simulatorValidCombinations += addition.simulatorValidCombinations;
    target.branchesPruned += addition.branchesPruned;
    target.stockCapacityConflicts += addition.stockCapacityConflicts;
    target.prefixConflicts += addition.prefixConflicts;
    target.hotspotPromotions += addition.hotspotPromotions;
    target.capCuts += addition.capCuts;
    target.prefixCuts += addition.prefixCuts;
    target.cutRounds += addition.cutRounds;
    target.stockCreditDenials += addition.stockCreditDenials;
    target.exactCreditMismatches += addition.exactCreditMismatches;
    target.criticalRoadPromotions += addition.criticalRoadPromotions;
    target.synchronizationConflicts += addition.synchronizationConflicts;
    target.duplicatePlansSkipped += addition.duplicatePlansSkipped;
    target.invalidPlanCombinations += addition.invalidPlanCombinations;
    target.reservationConflicts += addition.reservationConflicts;
    target.roundPreparationMicroseconds += addition.roundPreparationMicroseconds;
    target.beamConstructionMicroseconds += addition.beamConstructionMicroseconds;
    target.beamEvaluationMicroseconds += addition.beamEvaluationMicroseconds;
    target.depthFirstSearchMicroseconds += addition.depthFirstSearchMicroseconds;
    target.populationMaintenanceMicroseconds += addition.populationMaintenanceMicroseconds;
    if (target.bestExactBundleScore < addition.bestExactBundleScore) {
        target.bestExactBundleScore = addition.bestExactBundleScore;
    }
    target.nativeExactStockCredits =
        target.nativeExactStockCredits || addition.nativeExactStockCredits;
    target.stockCappedSearchOrder =
        target.stockCappedSearchOrder || addition.stockCappedSearchOrder;
    target.bundleAwareUpperBound =
        target.bundleAwareUpperBound ||
        addition.bundleAwareUpperBound;
    target.deadlineReached = target.deadlineReached || addition.deadlineReached;
    target.searchComplete = hadSearch
        ? target.searchComplete && addition.searchComplete
        : addition.searchComplete;
    if (compare_lexicographic(addition.optimisticUpperBound, target.optimisticUpperBound) > 0) {
        target.optimisticUpperBound = addition.optimisticUpperBound;
    }
    if (compare_lexicographic(
            addition.searchGuidanceUpperBound,
            target.searchGuidanceUpperBound) > 0) {
        target.searchGuidanceUpperBound =
            addition.searchGuidanceUpperBound;
    }
}

void merge_alns_diagnostics(AlnsDiagnostics& target, const AlnsDiagnostics& addition) {
    target.iterations += addition.iterations;
    target.accepted += addition.accepted;
    target.improvements += addition.improvements;
    target.synthesizedRoutes += addition.synthesizedRoutes;
    target.synthesizedAccepted += addition.synthesizedAccepted;
    target.proofGuidedIterations += addition.proofGuidedIterations;
    target.proofGuidedRoutes += addition.proofGuidedRoutes;
    target.proofGuidedAccepted += addition.proofGuidedAccepted;
    target.proofGuidedImprovements += addition.proofGuidedImprovements;
    target.poolRoutesConsidered += addition.poolRoutesConsidered;
    target.poolNovelRoutes += addition.poolNovelRoutes;
    target.poolRetainedRoutes += addition.poolRetainedRoutes;
    target.recombinationCandidates += addition.recombinationCandidates;
    target.recombinationImprovements += addition.recombinationImprovements;
    target.recombinationDeadlineSkipped =
        target.recombinationDeadlineSkipped || addition.recombinationDeadlineSkipped;
    for (std::size_t index = 0; index < target.attemptedByOperator.size(); ++index) {
        target.attemptedByOperator.at(index) += addition.attemptedByOperator.at(index);
        target.acceptedByOperator.at(index) += addition.acceptedByOperator.at(index);
    }
}

[[nodiscard]] bool better_search_candidate(
    const MasterCandidate& left,
    const MasterCandidate& right) {
    const std::int32_t scoreOrder =
        compare_lexicographic(left.scoreAfterToday, right.scoreAfterToday);
    if (scoreOrder != 0) {
        return scoreOrder > 0;
    }
    const std::int32_t slackOrder =
        compare_terminal_slack(left.terminalSlack, right.terminalSlack);
    if (slackOrder != 0) {
        return slackOrder > 0;
    }
    return left.stableId < right.stableId;
}

[[nodiscard]] std::int32_t search_plan_distance(
    const MasterCandidate& left,
    const MasterCandidate& right) {
    const std::size_t agentCount = std::max(
        left.plan.actions.size(),
        right.plan.actions.size());
    std::int32_t distance = 0;
    for (std::size_t agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        if (agentIndex >= left.plan.actions.size() ||
            agentIndex >= right.plan.actions.size()) {
            const AgentPlan& existing =
                agentIndex < left.plan.actions.size()
                ? left.plan.actions.at(agentIndex)
                : right.plan.actions.at(agentIndex);
            distance += std::max(
                1,
                static_cast<std::int32_t>(existing.size()));
            continue;
        }
        const AgentPlan& leftActions = left.plan.actions.at(agentIndex);
        const AgentPlan& rightActions = right.plan.actions.at(agentIndex);
        const std::size_t actionCount = std::max(
            leftActions.size(),
            rightActions.size());
        for (std::size_t actionIndex = 0;
             actionIndex < actionCount;
             ++actionIndex) {
            if (actionIndex >= leftActions.size() ||
                actionIndex >= rightActions.size() ||
                leftActions.at(actionIndex).kind !=
                    rightActions.at(actionIndex).kind ||
                leftActions.at(actionIndex).value !=
                    rightActions.at(actionIndex).value) {
                ++distance;
            }
        }
    }
    return distance;
}

[[nodiscard]] OfficialScore current_score(const MatchLedger& ledger) {
    return OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
}

[[nodiscard]] LexicographicGapDiagnostics lexicographic_gap(
    OfficialScore lowerBound,
    OfficialScore upperBound) {
    LexicographicGapDiagnostics result;
    result.lowerBound = lowerBound;
    result.upperBound = upperBound;
    result.validEnvelope = compare_lexicographic(upperBound, lowerBound) >= 0;
    for (std::size_t tier = 0; tier < result.componentGaps.size(); ++tier) {
        result.componentGaps.at(tier) = std::max(
            0,
            score_component(upperBound, tier) - score_component(lowerBound, tier));
        if (result.firstOpenTier == 0 && result.componentGaps.at(tier) > 0) {
            result.firstOpenTier = static_cast<std::int32_t>(tier + 1U);
        }
    }
    return result;
}

[[nodiscard]] OfficialScore absolute_horizon_upper_bound(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger) {
    const std::int32_t remainingDays = config.day_count() - state.dayNumber + 1;
    const std::int32_t perDayServings = std::accumulate(
        config.spots.begin(),
        config.spots.end(),
        0,
        [](std::int32_t total, const Spot& spot) {
            return total + spot.stock;
        });
    return OfficialScore{
        config.brand_count(),
        ledger.totalDailyDistinct + remainingDays * config.brand_count(),
        ledger.totalServings + remainingDays * perDayServings,
    };
}

void finalize_optimality_gap(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    DecisionResult& result) {
    const OfficialScore todayUpper = compare_lexicographic(
        result.diagnostics.optimisticUpperBound,
        result.candidate.scoreAfterToday) >= 0
        ? result.diagnostics.optimisticUpperBound
        : result.candidate.scoreAfterToday;
    const OfficialScore certifiedLower = result.profile.certifiedLowerBound;
    const OfficialScore candidateUpper = result.profile.hasValidUpperBound &&
            compare_lexicographic(result.profile.validUpperBound, certifiedLower) >= 0
        ? result.profile.validUpperBound
        : certifiedLower;
    const OfficialScore viabilityUpper = compare_lexicographic(
        result.viability.upperBound,
        certifiedLower) >= 0
        ? result.viability.upperBound
        : certifiedLower;
    result.audit.optimalityGap.todayPortfolio = lexicographic_gap(
        result.candidate.scoreAfterToday,
        todayUpper);
    result.audit.optimalityGap.candidateHorizon = lexicographic_gap(
        certifiedLower,
        candidateUpper);
    result.audit.optimalityGap.viabilityHorizon = lexicographic_gap(
        certifiedLower,
        viabilityUpper);
    result.audit.optimalityGap.absoluteHorizon = lexicographic_gap(
        certifiedLower,
        absolute_horizon_upper_bound(config, state, ledger));
    result.audit.optimalityGap.portfolioSearchComplete = result.diagnostics.searchComplete;
    result.audit.optimalityGap.viabilityDeadlineReached = result.viability.deadlineReached;
}

[[nodiscard]] std::string evaluator_contract_hash() {
    constexpr std::string_view contract =
        "lexicographic-risk-comparator-v1|exact-step-simulator-v1|independent-validator-v1";
    std::uint64_t hash = 14695981039346656037ULL;
    for (const char byte : contract) {
        hash ^= static_cast<std::uint8_t>(byte);
        hash *= 1099511628211ULL;
    }
    return "fnv1a64:" + std::to_string(hash);
}

void bind_manifest_policy(ScenarioManifest& manifest, const RiskPolicy& policy) {
    manifest.generatorSeed = 0;
    manifest.evaluatorHash = evaluator_contract_hash();
    manifest.riskPolicyVersion = policy.version;
    manifest.confidenceBasisPoints = policy.confidenceBasisPoints;
    manifest.safetySlack = policy.safetySlack;
    manifest.resolutionBasisPoints = policy.resolutionBasisPoints;
    manifest.quantileBasisPoints = policy.quantileBasisPoints;
}

[[nodiscard]] ScenarioManifest emergency_manifest(const MatchConfig& config) {
    ScenarioManifest manifest;
    manifest.version = "emergency-v1";
    TrafficScenario scenario;
    scenario.scenarioId = 0;
    scenario.scenarioClass = "emergency";
    scenario.weight = 10000;
    scenario.opponentCurrentFootprint.assign(static_cast<std::size_t>(config.map.cell_count()), 0);
    scenario.opponentCarryFootprint.assign(static_cast<std::size_t>(config.map.cell_count()), 0);
    manifest.scenarios.push_back(std::move(scenario));
    manifest.totalWeight = 10000;
    manifest.survivalSignatureEnabled = false;
    return manifest;
}

[[nodiscard]] OfficialScore minimum_score(const std::vector<ScenarioOutcome>& outcomes) {
    if (outcomes.empty()) {
        return OfficialScore{};
    }
    OfficialScore result = outcomes.front().score;
    for (const ScenarioOutcome& outcome : outcomes) {
        if (outcome.score < result) {
            result = outcome.score;
        }
    }
    return result;
}

[[nodiscard]] OfficialScore weighted_quantile(
    const std::vector<ScenarioOutcome>& outcomes,
    const ScenarioManifest& manifest,
    std::uint32_t requiredBasisPoints) {
    if (outcomes.size() != manifest.scenarios.size() || outcomes.empty() || manifest.totalWeight == 0U) {
        return OfficialScore{};
    }
    std::vector<OfficialScore> support;
    support.reserve(outcomes.size());
    for (const ScenarioOutcome& outcome : outcomes) {
        support.push_back(outcome.score);
    }
    std::sort(support.begin(), support.end());
    support.erase(std::unique(support.begin(), support.end()), support.end());
    OfficialScore result = support.front();
    for (const OfficialScore& threshold : support) {
        std::uint64_t survivingWeight = 0;
        for (std::size_t scenarioIndex = 0; scenarioIndex < outcomes.size(); ++scenarioIndex) {
            if (!(outcomes.at(scenarioIndex).score < threshold)) {
                survivingWeight += manifest.scenarios.at(scenarioIndex).weight;
            }
        }
        if (survivingWeight * 10000U >= static_cast<std::uint64_t>(requiredBasisPoints) * manifest.totalWeight) {
            result = threshold;
        }
    }
    return result;
}

[[nodiscard]] std::int32_t confidence_coverage(
    const std::vector<ScenarioOutcome>& outcomes,
    const ScenarioManifest& manifest,
    std::uint32_t requiredBasisPoints) {
    if (outcomes.size() != manifest.scenarios.size() || outcomes.empty() || manifest.totalWeight == 0U) {
        return 0;
    }
    std::int32_t result = 0;
    for (const ScenarioOutcome& outcome : outcomes) {
        result = std::max(result, outcome.score.lifetimeDistinct);
    }
    for (std::int32_t target = result; target >= 0; --target) {
        std::uint64_t survivingWeight = 0;
        for (std::size_t scenarioIndex = 0; scenarioIndex < outcomes.size(); ++scenarioIndex) {
            if (outcomes.at(scenarioIndex).score.lifetimeDistinct >= target) {
                survivingWeight += manifest.scenarios.at(scenarioIndex).weight;
            }
        }
        if (survivingWeight * 10000U >= static_cast<std::uint64_t>(requiredBasisPoints) * manifest.totalWeight) {
            return target;
        }
    }
    return 0;
}

[[nodiscard]] bool same_quantiles(const CandidateProfile& left, const CandidateProfile& right) {
    return left.quantiles == right.quantiles;
}

[[nodiscard]] std::int32_t compare_signature(
    const std::vector<std::int32_t>& left,
    const std::vector<std::int32_t>& right) {
    const std::size_t shared = std::min(left.size(), right.size());
    for (std::size_t index = 0; index < shared; ++index) {
        if (left.at(index) != right.at(index)) {
            return left.at(index) < right.at(index) ? -1 : 1;
        }
    }
    if (left.size() == right.size()) {
        return 0;
    }
    return left.size() < right.size() ? -1 : 1;
}

[[nodiscard]] bool better_evaluation(const CandidateEvaluation& left, const CandidateEvaluation& right) {
    for (std::size_t quantileIndex = 0; quantileIndex < left.profile.quantiles.size(); ++quantileIndex) {
        const std::int32_t order = compare_lexicographic(
            left.profile.quantiles.at(quantileIndex),
            right.profile.quantiles.at(quantileIndex));
        if (order != 0) {
            return order > 0;
        }
    }
    const std::int32_t signatureOrder = compare_signature(left.profile.survivalSignature, right.profile.survivalSignature);
    if (signatureOrder != 0) {
        return signatureOrder > 0;
    }
    const std::int32_t lowerOrder = compare_lexicographic(
        left.profile.certifiedLowerBound,
        right.profile.certifiedLowerBound);
    if (lowerOrder != 0) {
        return lowerOrder > 0;
    }
    const std::int32_t currentOrder = compare_lexicographic(left.candidate.scoreAfterToday, right.candidate.scoreAfterToday);
    if (currentOrder != 0) {
        return currentOrder > 0;
    }
    const std::int32_t terminalOrder = compare_terminal_slack(
        left.candidate.terminalSlack,
        right.candidate.terminalSlack);
    if (terminalOrder != 0) {
        return terminalOrder > 0;
    }
    if (left.candidate.trafficSafety.thresholdCrossings != right.candidate.trafficSafety.thresholdCrossings) {
        return left.candidate.trafficSafety.thresholdCrossings < right.candidate.trafficSafety.thresholdCrossings;
    }
    if (left.candidate.trafficSafety.thresholdBandRoads != right.candidate.trafficSafety.thresholdBandRoads) {
        return left.candidate.trafficSafety.thresholdBandRoads < right.candidate.trafficSafety.thresholdBandRoads;
    }
    if (left.candidate.trafficSafety.totalRoadStays != right.candidate.trafficSafety.totalRoadStays) {
        return left.candidate.trafficSafety.totalRoadStays < right.candidate.trafficSafety.totalRoadStays;
    }
    return left.candidate.stableId < right.candidate.stableId;
}

[[nodiscard]] bool better_provisional_evaluation(
    const CandidateEvaluation& left,
    const CandidateEvaluation& right) {
    for (std::size_t quantileIndex = 0; quantileIndex < left.profile.quantiles.size(); ++quantileIndex) {
        const std::int32_t order = compare_lexicographic(
            left.profile.quantiles.at(quantileIndex),
            right.profile.quantiles.at(quantileIndex));
        if (order != 0) {
            return order > 0;
        }
    }
    const std::int32_t signatureOrder = compare_signature(left.profile.survivalSignature, right.profile.survivalSignature);
    if (signatureOrder != 0) {
        return signatureOrder > 0;
    }
    const std::int32_t currentOrder = compare_lexicographic(left.candidate.scoreAfterToday, right.candidate.scoreAfterToday);
    if (currentOrder != 0) {
        return currentOrder > 0;
    }
    const std::int32_t terminalOrder = compare_terminal_slack(
        left.candidate.terminalSlack,
        right.candidate.terminalSlack);
    if (terminalOrder != 0) {
        return terminalOrder > 0;
    }
    if (left.candidate.trafficSafety.thresholdCrossings != right.candidate.trafficSafety.thresholdCrossings) {
        return left.candidate.trafficSafety.thresholdCrossings < right.candidate.trafficSafety.thresholdCrossings;
    }
    if (left.candidate.trafficSafety.thresholdBandRoads != right.candidate.trafficSafety.thresholdBandRoads) {
        return left.candidate.trafficSafety.thresholdBandRoads < right.candidate.trafficSafety.thresholdBandRoads;
    }
    if (left.candidate.trafficSafety.totalRoadStays != right.candidate.trafficSafety.totalRoadStays) {
        return left.candidate.trafficSafety.totalRoadStays < right.candidate.trafficSafety.totalRoadStays;
    }
    return left.candidate.stableId < right.candidate.stableId;
}

[[nodiscard]] MasterCandidate make_valid_emergency_candidate(
    const MatchConfig& config,
    const ExactStepSimulator& simulator,
    const IndependentDayValidator& validator,
    const DayState& state,
    const MatchLedger& ledger) {
    MasterCandidate candidate;
    candidate.plan = emergency_wait_plan(config, state);
    candidate.simulation = simulator.simulate(state, candidate.plan, true);
    const SimulationResult validation = validator.validate(state, candidate.plan, true);
    std::string mismatch;
    if (!candidate.simulation.valid || !validator.agrees_with(candidate.simulation, validation, mismatch)) {
        throw std::runtime_error("emergency plan failed independent validation: " + mismatch);
    }
    candidate.scoreAfterToday = OfficialScore::after_day(ledger, candidate.simulation.score);
    candidate.terminalSlack = TerminalSlack{};
    candidate.trafficSafety.totalRoadStays = std::accumulate(
        candidate.simulation.roadFootprint.begin(),
        candidate.simulation.roadFootprint.end(),
        0);
    candidate.stableId = canonical_plan_bytes(candidate.plan);
    return candidate;
}

[[nodiscard]] FutureWitness build_wait_witness(
    const MatchConfig& config,
    const ExactStepSimulator& simulator,
    const IndependentDayValidator& validator,
    const MasterCandidate& candidate,
    const DayState& currentState,
    const MatchLedger& ledger,
    const TrafficBelief& belief,
    const TrafficScenario& scenario,
    std::chrono::steady_clock::time_point deadline) {
    FutureWitness witness;
    MatchLedger futureLedger = ledger;
    futureLedger.apply(candidate.simulation.score);
    witness.score = current_score(futureLedger);
    witness.certified = false;
    witness.lowerBoundOnly = scenario.pessimisticFallback;
    if (scenario.pessimisticFallback) {
        witness.certified = true;
        return witness;
    }
    if (currentState.dayNumber >= config.day_count()) {
        witness.certified = true;
        return witness;
    }
    DayState futureState;
    futureState.dayNumber = currentState.dayNumber + 1;
    futureState.agents = candidate.simulation.finalAgents;
    futureState.others = currentState.others;
    std::vector<std::int32_t> previousOwn = belief.previous_own_footprint();
    std::vector<std::int32_t> currentOwn = candidate.simulation.roadFootprint;
    std::vector<std::int32_t> opponentCarry = scenario.opponentCarryFootprint;
    std::vector<std::int32_t> opponentCurrent = scenario.opponentCurrentFootprint;
    futureState.roadStatuses = predict_with_components(
        config,
        previousOwn,
        currentOwn,
        opponentCarry,
        opponentCurrent);
    for (std::int32_t dayNumber = futureState.dayNumber; dayNumber <= config.day_count(); ++dayNumber) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return witness;
        }
        futureState.dayNumber = dayNumber;
        const DayPlan plan = emergency_wait_plan(config, futureState);
        const SimulationResult detailed = simulator.simulate(futureState, plan, true);
        const SimulationResult validation = validator.validate(futureState, plan, true);
        std::string mismatch;
        if (!detailed.valid || !validator.agrees_with(detailed, validation, mismatch)) {
            throw std::runtime_error("wait witness failed independent validation: " + mismatch);
        }
        witness.futurePlans.push_back(plan);
        futureLedger.apply(detailed.score);
        witness.score = current_score(futureLedger);
        previousOwn = currentOwn;
        currentOwn = detailed.roadFootprint;
        opponentCarry = opponentCurrent;
        std::fill(opponentCurrent.begin(), opponentCurrent.end(), 0);
        futureState.agents = detailed.finalAgents;
        if (dayNumber < config.day_count()) {
            futureState.roadStatuses = predict_with_components(
                config,
                previousOwn,
                currentOwn,
                opponentCarry,
                opponentCurrent);
        }
    }
    witness.certified = true;
    return witness;
}

[[nodiscard]] FutureWitness build_monotone_wait_floor(
    const MatchConfig& config,
    const MasterCandidate& candidate,
    const DayState& currentState) {
    FutureWitness witness;
    witness.score = candidate.scoreAfterToday;
    witness.certified = true;
    witness.lowerBoundOnly = true;
    DayState futureState;
    futureState.agents = candidate.simulation.finalAgents;
    for (std::int32_t dayNumber = currentState.dayNumber + 1;
         dayNumber <= config.day_count();
         ++dayNumber) {
        futureState.dayNumber = dayNumber;
        witness.futurePlans.push_back(emergency_wait_plan(config, futureState));
    }
    return witness;
}

} 

TrafficBelief::TrafficBelief(const MatchConfig& config)
    : config_(config),
      opponentCarry_(static_cast<std::size_t>(config.map.cell_count())),
      previousOwnFootprint_(static_cast<std::size_t>(config.map.cell_count()), 0) {}

void TrafficBelief::observe(const DayState& state) {
    if (state.roadStatuses.size() != static_cast<std::size_t>(config_.map.cell_count())) {
        throw std::invalid_argument("traffic belief requires a complete road status vector");
    }
    if (lastObservedDay.has_value()) {
        if (state.dayNumber < *lastObservedDay) {
            throw std::invalid_argument("traffic belief cannot observe days out of chronological order");
        }
        if (state.dayNumber > *lastObservedDay + 1) {
            ownHistory_.clear();
            publicEndpointHistory_.clear();
            publicOpponentHistory_.clear();
            std::fill(previousOwnFootprint_.begin(), previousOwnFootprint_.end(), 0);
        }
    }
    const bool advancedDay = !lastObservedDay.has_value() || state.dayNumber > *lastObservedDay;
    if (advancedDay) {
        if (state.dayNumber <= 1 || !lastSubmittedDay.has_value() ||
            *lastSubmittedDay != state.dayNumber - 1 || ownHistory_.empty()) {
            std::fill(previousOwnFootprint_.begin(), previousOwnFootprint_.end(), 0);
        } else {
            previousOwnFootprint_ = ownHistory_.back();
        }
        std::vector<CellId> endpoints;
        endpoints.reserve(state.agents.size());
        for (const AgentState& agent : state.agents) {
            endpoints.push_back(agent.position);
        }
        publicEndpointHistory_.push_back(std::move(endpoints));
        if (publicEndpointHistory_.size() > 2U) {
            publicEndpointHistory_.erase(publicEndpointHistory_.begin());
        }
        publicOpponentHistory_.push_back(state.others);
        if (publicOpponentHistory_.size() > 2U) {
            publicOpponentHistory_.erase(publicOpponentHistory_.begin());
        }
        lastObservedDay = state.dayNumber;
    }
    if (state.dayNumber <= 1 || ownHistory_.empty()) {
        std::fill(opponentCarry_.begin(), opponentCarry_.end(), RoadLoadInterval{});
        return;
    }
    std::vector<std::int32_t> knownOwn(static_cast<std::size_t>(config_.map.cell_count()), 0);
    for (const std::vector<std::int32_t>& footprint : ownHistory_) {
        for (const CellId road : config_.roadCells) {
            knownOwn.at(static_cast<std::size_t>(road)) += footprint.at(static_cast<std::size_t>(road));
        }
    }
    const std::int64_t carryDayMaximum = maximum_other_occupancy_for_day(config_, state.dayNumber - 1);
    const std::int64_t olderDayMaximum = maximum_other_occupancy_for_day(config_, state.dayNumber - 2);
    for (const CellId road : config_.roadCells) {
        const std::size_t roadOffset = static_cast<std::size_t>(road);
        const RoadLoadInterval total = total_load_interval(config_, state.roadStatuses.at(roadOffset));
        const std::int64_t twoDayMinimum = std::max<std::int64_t>(0, total.minimum - knownOwn.at(roadOffset));
        const std::int64_t twoDayMaximum = std::max<std::int64_t>(0, total.maximum - knownOwn.at(roadOffset));
        const std::int64_t carryMinimum = std::max<std::int64_t>(0, twoDayMinimum - olderDayMaximum);
        const std::int64_t carryMaximum = std::min(carryDayMaximum, twoDayMaximum);
        opponentCarry_.at(roadOffset) = RoadLoadInterval{
            static_cast<std::int32_t>(std::min<std::int64_t>(carryMinimum, std::numeric_limits<std::int32_t>::max())),
            static_cast<std::int32_t>(std::min<std::int64_t>(std::max(carryMinimum, carryMaximum), std::numeric_limits<std::int32_t>::max())),
        };
    }
}

void TrafficBelief::record_own_footprint(std::int32_t dayNumber, const std::vector<std::int32_t>& footprint) {
    if (footprint.size() != static_cast<std::size_t>(config_.map.cell_count())) {
        throw std::invalid_argument("own traffic footprint has invalid size");
    }
    if (dayNumber < 1 || dayNumber > config_.day_count()) {
        throw std::invalid_argument("submitted footprint day is outside match bounds");
    }
    if (lastSubmittedDay.has_value() && dayNumber < *lastSubmittedDay) {
        throw std::invalid_argument("submitted footprint cannot move backward in time");
    }
    if (lastSubmittedDay.has_value() && dayNumber == *lastSubmittedDay) {
        if (ownHistory_.empty()) {
            throw std::runtime_error("traffic history lost the current submitted day");
        }
        ownHistory_.back() = footprint;
        return;
    }
    ownHistory_.push_back(footprint);
    lastSubmittedDay = dayNumber;
    if (ownHistory_.size() > 2U) {
        ownHistory_.erase(ownHistory_.begin());
    }
}

const std::vector<RoadLoadInterval>& TrafficBelief::opponent_carry() const {
    return opponentCarry_;
}

const std::vector<std::int32_t>& TrafficBelief::previous_own_footprint() const {
    return previousOwnFootprint_;
}

const std::vector<std::vector<CellId>>& TrafficBelief::public_endpoint_history() const {
    return publicEndpointHistory_;
}

const std::vector<std::vector<OtherTeamState>>& TrafficBelief::public_opponent_history() const {
    return publicOpponentHistory_;
}

ScenarioGenerator::ScenarioGenerator(const MatchConfig& config)
    : config_(config),
      roadIndexByCell_(static_cast<std::size_t>(config.map.cell_count()), -1) {
    for (std::size_t roadIndex = 0; roadIndex < config_.roadCells.size(); ++roadIndex) {
        roadIndexByCell_.at(static_cast<std::size_t>(config_.roadCells.at(roadIndex))) =
            static_cast<std::int32_t>(roadIndex);
    }
    componentsWithoutRoad_.reserve(config_.roadCells.size());
    for (const CellId removedRoad : config_.roadCells) {
        std::vector<std::int32_t> component(
            static_cast<std::size_t>(config_.map.cell_count()),
            -1);
        std::vector<CellId> queue;
        queue.reserve(static_cast<std::size_t>(config_.map.cell_count()));
        std::int32_t componentId = 0;
        for (CellId start = 0; start < config_.map.cell_count(); ++start) {
            if (start == removedRoad ||
                config_.map.terrain.at(static_cast<std::size_t>(start)) == Terrain::Pond ||
                component.at(static_cast<std::size_t>(start)) != -1) {
                continue;
            }
            component.at(static_cast<std::size_t>(start)) = componentId;
            queue.clear();
            queue.push_back(start);
            for (std::size_t offset = 0; offset < queue.size(); ++offset) {
                const CellId cell = queue.at(offset);
                for (std::int32_t direction = 0; direction < kDirectionCount; ++direction) {
                    const CellId next = config_.map.neighbors.at(static_cast<std::size_t>(cell)).at(
                        static_cast<std::size_t>(direction));
                    if (next == kInvalidCell || next == removedRoad ||
                        config_.map.terrain.at(static_cast<std::size_t>(next)) == Terrain::Pond ||
                        component.at(static_cast<std::size_t>(next)) != -1) {
                        continue;
                    }
                    component.at(static_cast<std::size_t>(next)) = componentId;
                    queue.push_back(next);
                }
            }
            ++componentId;
        }
        componentsWithoutRoad_.push_back(std::move(component));
    }
}

ScenarioManifest ScenarioGenerator::freeze_manifest(
    const DayState& state,
    const TrafficBelief& belief,
    std::int32_t maximumAdversarialScenarios) const {
    ScenarioManifest manifest;
    manifest.version = "public-exposure-v3-static-fallback";
    manifest.usedStaticFallback = true;
    manifest.requiredClassesCovered = true;
    manifest.effectiveCoverageBasisPoints = 10000;
    manifest.calibrationErrorBasisPoints = 0;
    const std::size_t cellCount = static_cast<std::size_t>(config_.map.cell_count());
    std::vector<std::int32_t> directPotential(cellCount, 0);
    std::vector<std::int32_t> adversarialPotential(cellCount, 0);
    std::vector<std::vector<std::int32_t>> adversarialDwellByOpponent;
    for (const OtherTeamState& team : state.others) {
        for (const AgentState& agent : team.agents) {
            const std::vector<std::int32_t> directDwell = direct_reachable_dwell(config_, state, agent);
            for (const CellId road : config_.roadCells) {
                const std::size_t roadOffset = static_cast<std::size_t>(road);
                const std::int32_t directlyReachable = directDwell.at(roadOffset);
                directPotential.at(roadOffset) += directlyReachable;
                adversarialPotential.at(roadOffset) += directlyReachable;
            }
            adversarialDwellByOpponent.push_back(directDwell);
        }
    }

    TrafficScenario likely;
    likely.scenarioId = 0;
    likely.scenarioClass = "likely";
    likely.likely = true;
    likely.jointFeasible = true;
    likely.construction = "public-endpoint-shortest-path-bundle";
    likely.opponentCurrentFootprint = likely_public_footprint(config_, state, belief);
    likely.opponentCarryFootprint = midpoint_carry(belief);
    if (config_.roadCells.empty()) {
        manifest.version = "public-exposure-v3-deterministic-no-road";
        manifest.usedStaticFallback = false;
        manifest.survivalSignatureEnabled = false;
        likely.scenarioClass = "deterministic-no-road";
        likely.weight = 10000;
        likely.construction = "traffic-invariant-no-road";
        manifest.scenarios.push_back(std::move(likely));
        manifest.totalWeight = 10000;
        return manifest;
    }

    struct Exposure {
        CellId road = kInvalidCell;
        std::int32_t score = 0;
    };
    std::vector<Exposure> exposures;
    exposures.reserve(config_.roadCells.size());
    std::vector<CellId> publicPositions;
    publicPositions.reserve(state.agents.size() + state.others.size() * static_cast<std::size_t>(config_.agent_count()));
    for (const AgentState& agent : state.agents) {
        publicPositions.push_back(agent.position);
    }
    for (const OtherTeamState& team : state.others) {
        for (const AgentState& agent : team.agents) {
            publicPositions.push_back(agent.position);
        }
    }
    const std::vector<std::vector<CellId>>& endpointHistory = belief.public_endpoint_history();
    const std::vector<std::vector<OtherTeamState>>& opponentHistory = belief.public_opponent_history();
    for (const CellId road : config_.roadCells) {
        const std::size_t roadOffset = static_cast<std::size_t>(road);
        const std::int32_t roadIndex = roadIndexByCell_.at(roadOffset);
        const std::vector<std::int32_t>& components = componentsWithoutRoad_.at(
            static_cast<std::size_t>(roadIndex));
        const auto component_of = [&components](CellId cell) {
            return cell == kInvalidCell ? -1 : components.at(static_cast<std::size_t>(cell));
        };
        bool separator = false;
        for (const CellId source : publicPositions) {
            const std::int32_t sourceComponent = component_of(source);
            if (sourceComponent < 0) {
                continue;
            }
            for (const Spot& spot : config_.spots) {
                const std::int32_t spotComponent = component_of(spot.position);
                if (spotComponent >= 0 && spotComponent != sourceComponent) {
                    separator = true;
                    break;
                }
            }
            if (separator) {
                break;
            }
        }
        bool repeatedEndpointCorridor = false;
        if (endpointHistory.size() >= 2U) {
            const std::vector<CellId>& previous = endpointHistory.at(endpointHistory.size() - 2U);
            const std::vector<CellId>& current = endpointHistory.back();
            const std::size_t common = std::min(previous.size(), current.size());
            for (std::size_t agentIndex = 0; agentIndex < common; ++agentIndex) {
                const std::int32_t previousComponent = component_of(previous.at(agentIndex));
                const std::int32_t currentComponent = component_of(current.at(agentIndex));
                if (previousComponent >= 0 && currentComponent >= 0 && previousComponent != currentComponent) {
                    repeatedEndpointCorridor = true;
                    break;
                }
            }
        }
        bool repeatedOpponentCorridor = false;
        if (opponentHistory.size() >= 2U) {
            const std::vector<OtherTeamState>& previousTeams = opponentHistory.at(opponentHistory.size() - 2U);
            const std::vector<OtherTeamState>& currentTeams = opponentHistory.back();
            for (const OtherTeamState& previousTeam : previousTeams) {
                const auto currentTeam = std::find_if(
                    currentTeams.begin(),
                    currentTeams.end(),
                    [&previousTeam](const OtherTeamState& team) { return team.teamId == previousTeam.teamId; });
                if (currentTeam == currentTeams.end()) {
                    continue;
                }
                const std::size_t common = std::min(previousTeam.agents.size(), currentTeam->agents.size());
                for (std::size_t agentIndex = 0; agentIndex < common; ++agentIndex) {
                    const std::int32_t previousComponent = component_of(
                        previousTeam.agents.at(agentIndex).position);
                    const std::int32_t currentComponent = component_of(
                        currentTeam->agents.at(agentIndex).position);
                    if (previousComponent >= 0 && currentComponent >= 0 &&
                        previousComponent != currentComponent) {
                        repeatedOpponentCorridor = true;
                        break;
                    }
                }
                if (repeatedOpponentCorridor) {
                    break;
                }
            }
        }
        const std::int32_t potential = adversarialPotential.at(roadOffset);
        const RoadLoadInterval carry = belief.opponent_carry().at(roadOffset);
        const std::int64_t lowerBase = belief.previous_own_footprint().at(roadOffset) + carry.minimum;
        const std::int64_t upperBase = belief.previous_own_footprint().at(roadOffset) + carry.maximum;
        const bool canChangeStatus =
            classify_traffic(config_, lowerBase) != classify_traffic(config_, lowerBase + potential) ||
            classify_traffic(config_, upperBase) != classify_traffic(config_, upperBase + potential);
        if (potential <= 0 || (!canChangeStatus && !separator && !repeatedEndpointCorridor && !repeatedOpponentCorridor)) {
            continue;
        }
        std::int32_t score = potential * 10 + directPotential.at(roadOffset);
        if (state.roadStatuses.at(static_cast<std::size_t>(road)) != RoadStatus::Smooth) {
            score += 1000;
        }
        if (road_degree(config_, road) <= 2) {
            score += 200;
        }
        if (road_touches_spot(config_, road)) {
            score += 100;
        }
        if (separator) {
            score += 2000;
        }
        if (repeatedEndpointCorridor) {
            score += 3000;
        }
        if (repeatedOpponentCorridor) {
            score += 3000;
        }
        if (score > 0) {
            exposures.push_back(Exposure{road, score});
        }
    }
    std::sort(
        exposures.begin(),
        exposures.end(),
        [](const Exposure& left, const Exposure& right) {
            if (left.score != right.score) {
                return left.score > right.score;
            }
            return left.road < right.road;
        });
    const std::int32_t cap = std::max(1, maximumAdversarialScenarios);
    if (static_cast<std::int32_t>(exposures.size()) > std::max(2, cap)) {
        exposures.resize(static_cast<std::size_t>(std::max(2, cap)));
    }
    likely.weight = 5000;
    manifest.scenarios.push_back(std::move(likely));
    if (exposures.empty()) {
        TrafficScenario fallback;
        fallback.scenarioId = 1;
        fallback.scenarioClass = "fallback-pessimistic-bound";
        fallback.weight = 5000;
        fallback.adversarial = true;
        fallback.pessimisticFallback = true;
        fallback.jointFeasible = false;
        fallback.construction = "certified-pessimistic-bound";
        fallback.opponentCurrentFootprint.assign(cellCount, 0);
        fallback.opponentCarryFootprint = pessimistic_carry(belief);
        manifest.scenarios.push_back(std::move(fallback));
        manifest.requiredClassesCovered = false;
    } else {
        std::vector<std::vector<CellId>> scenarioRoadSets;
        const std::int32_t singletonLimit = cap <= 1 ? 1 : std::max(1, cap - 2);
        for (std::size_t exposureIndex = 0;
             exposureIndex < exposures.size() && static_cast<std::int32_t>(scenarioRoadSets.size()) < singletonLimit;
             ++exposureIndex) {
            scenarioRoadSets.push_back(std::vector<CellId>{exposures.at(exposureIndex).road});
        }
        for (std::size_t left = 0;
             left < exposures.size() && static_cast<std::int32_t>(scenarioRoadSets.size()) < cap;
             ++left) {
            for (std::size_t right = left + 1U;
                 right < exposures.size() && static_cast<std::int32_t>(scenarioRoadSets.size()) < cap;
                 ++right) {
                scenarioRoadSets.push_back(std::vector<CellId>{
                    exposures.at(left).road,
                    exposures.at(right).road,
                });
            }
        }
        const auto allocate_dwell = [&adversarialDwellByOpponent, cellCount](const std::vector<CellId>& roads) {
            std::vector<std::int32_t> footprint(cellCount, 0);
            for (const std::vector<std::int32_t>& dwell : adversarialDwellByOpponent) {
                CellId assignedRoad = kInvalidCell;
                std::int32_t bestDwell = 0;
                for (const CellId road : roads) {
                    const std::int32_t candidate = dwell.at(static_cast<std::size_t>(road));
                    if (candidate > bestDwell || (candidate == bestDwell && candidate > 0 && road < assignedRoad)) {
                        assignedRoad = road;
                        bestDwell = candidate;
                    }
                }
                if (assignedRoad != kInvalidCell && bestDwell > 0) {
                    footprint.at(static_cast<std::size_t>(assignedRoad)) += bestDwell;
                }
            }
            return footprint;
        };
        const std::uint32_t baseWeight = 5000U / static_cast<std::uint32_t>(scenarioRoadSets.size());
        std::uint32_t remainder = 5000U % static_cast<std::uint32_t>(scenarioRoadSets.size());
        for (std::size_t scenarioIndex = 0; scenarioIndex < scenarioRoadSets.size(); ++scenarioIndex) {
            TrafficScenario adversarial;
            adversarial.scenarioId = static_cast<std::int32_t>(scenarioIndex + 1U);
            adversarial.scenarioClass = scenarioRoadSets.at(scenarioIndex).size() == 1U
                ? "public-adversarial"
                : "public-adversarial-combination";
            adversarial.weight = baseWeight + (remainder > 0U ? 1U : 0U);
            remainder = remainder > 0U ? remainder - 1U : 0U;
            adversarial.adversarial = true;
            adversarial.jointFeasible = true;
            adversarial.construction = "direct-fuel-feasible-one-road-per-opponent-agent";
            adversarial.opponentCurrentFootprint = allocate_dwell(scenarioRoadSets.at(scenarioIndex));
            adversarial.opponentCarryFootprint = pessimistic_carry(belief);
            manifest.scenarios.push_back(std::move(adversarial));
        }
    }
    manifest.totalWeight = 0;
    for (const TrafficScenario& scenario : manifest.scenarios) {
        manifest.totalWeight += scenario.weight;
    }
    manifest.survivalSignatureEnabled = manifest.requiredClassesCovered && manifest.totalWeight == 10000U &&
        manifest.scenarios.size() >= 2U;
    return manifest;
}

std::vector<TrafficScenario> ScenarioGenerator::private_sensitivity_scenarios(
    const DayState& state,
    const TrafficBelief& belief,
    const std::vector<CellId>& privateRoads,
    std::int32_t maximumScenarios) const {
    if (maximumScenarios <= 0) {
        return {};
    }
    std::vector<CellId> roads;
    roads.reserve(static_cast<std::size_t>(maximumScenarios));
    for (const CellId road : privateRoads) {
        if (!config_.map.contains(road) ||
            config_.map.terrain.at(static_cast<std::size_t>(road)) != Terrain::Road ||
            std::find(roads.begin(), roads.end(), road) != roads.end()) {
            continue;
        }
        roads.push_back(road);
        if (static_cast<std::int32_t>(roads.size()) >= maximumScenarios) {
            break;
        }
    }
    if (roads.empty()) {
        return {};
    }
    const std::size_t cellCount = static_cast<std::size_t>(config_.map.cell_count());
    std::vector<std::vector<std::int32_t>> dwellByOpponent;
    for (const OtherTeamState& team : state.others) {
        for (const AgentState& agent : team.agents) {
            dwellByOpponent.push_back(direct_reachable_dwell(config_, state, agent));
        }
    }
    std::vector<TrafficScenario> scenarios;
    scenarios.reserve(roads.size());
    for (std::size_t roadIndex = 0; roadIndex < roads.size(); ++roadIndex) {
        const CellId road = roads.at(roadIndex);
        TrafficScenario scenario;
        scenario.scenarioId = static_cast<std::int32_t>(roadIndex);
        scenario.scenarioClass = "private-sensitivity";
        scenario.adversarial = true;
        scenario.privateSensitivity = true;
        scenario.jointFeasible = true;
        scenario.construction = "private-direct-fuel-feasible-sensitivity";
        scenario.opponentCurrentFootprint.assign(cellCount, 0);
        for (const std::vector<std::int32_t>& dwell : dwellByOpponent) {
            scenario.opponentCurrentFootprint.at(static_cast<std::size_t>(road)) +=
                dwell.at(static_cast<std::size_t>(road));
        }
        if (scenario.opponentCurrentFootprint.at(static_cast<std::size_t>(road)) <= 0) {
            continue;
        }
        scenario.opponentCarryFootprint = pessimistic_carry(belief);
        scenarios.push_back(std::move(scenario));
    }
    return scenarios;
}

std::vector<RoadStatus> predict_next_road_statuses(
    const MatchConfig& config,
    const TrafficBelief& belief,
    const TrafficScenario& scenario,
    const std::vector<std::int32_t>& ownCurrentFootprint) {
    return predict_with_components(
        config,
        belief.previous_own_footprint(),
        ownCurrentFootprint,
        scenario.opponentCarryFootprint,
        scenario.opponentCurrentFootprint);
}

FastViabilityAnalyzer::FastViabilityAnalyzer(const MatchConfig& config)
    : config_(config) {}

ViabilityBounds FastViabilityAnalyzer::analyze(
    const DayState& state,
    const MatchLedger& ledger,
    std::optional<std::chrono::steady_clock::time_point> deadline) const {
    ViabilityBounds result;
    result.lowerBound = current_score(ledger);
    const std::int32_t remainingDays = config_.day_count() - state.dayNumber + 1;
    std::int32_t remainingSteps = 0;
    for (std::int32_t dayNumber = state.dayNumber; dayNumber <= config_.day_count(); ++dayNumber) {
        remainingSteps += config_.steps_for_day(dayNumber);
    }
    const std::size_t brandCount = static_cast<std::size_t>(config_.brand_count());
    std::int32_t perDayServings = 0;
    for (const Spot& spot : config_.spots) {
        perDayServings += spot.stock;
    }
    const auto deadline_reached = [&deadline]() {
        return deadline.has_value() && std::chrono::steady_clock::now() >= *deadline;
    };
    const auto coarse_timeout = [&]() {
        result.deadlineReached = true;
        result.coverageCap = config_.brand_count();
        result.coverageSafe = ledger.lifetime_distinct();
        result.confidenceCoverage = result.coverageSafe;
        result.optimisticLatestDayByBrand.assign(brandCount, state.dayNumber - 1);
        result.latestSafeDayByBrand.assign(brandCount, state.dayNumber - 1);
        result.slackByBrand.assign(brandCount, -1);
        result.optimisticAgentDaySlotsByBrand.assign(brandCount, 0);
        result.safeAgentDaySlotsByBrand.assign(brandCount, 0);
        result.matchingRelaxationFeasible = true;
        result.reservations.clear();
        result.upperBound = OfficialScore{
            result.coverageCap,
            ledger.totalDailyDistinct + config_.brand_count() * remainingDays,
            ledger.totalServings + perDayServings * remainingDays,
        };
        result.pessimisticUpperBound = result.upperBound;
        for (std::int32_t coverage = ledger.lifetime_distinct();
             coverage <= result.coverageCap;
             ++coverage) {
            result.conditionalTiers.push_back(ConditionalTierBounds{
                coverage,
                ledger.totalDailyDistinct,
                result.upperBound.totalDailyDistinct,
                ledger.totalServings,
                result.upperBound.totalServings,
                coverage <= ledger.lifetime_distinct(),
            });
        }
        FutureWitness fixedWitness;
        fixedWitness.score = current_score(ledger);
        fixedWitness.certified = true;
        result.frontier.push_back(ViabilityFrontierPoint{
            ledger.lifetime_distinct(),
            0,
            current_score(ledger),
            result.upperBound,
            std::move(fixedWitness),
            -1,
            "fixed-ledger",
        });
        if (result.coverageCap > ledger.lifetime_distinct()) {
            result.frontier.push_back(ViabilityFrontierPoint{
                result.coverageCap,
                2,
                current_score(ledger),
                result.upperBound,
                FutureWitness{},
                -1,
                "timeout-optimistic-bound",
            });
        }
        return result;
    };
    if (deadline_reached()) {
        return coarse_timeout();
    }
    const bool hasTanker = std::any_of(
        state.agents.begin(),
        state.agents.end(),
        [](const AgentState& agent) { return agent.kind == AgentKind::Tanker; });
    std::int64_t noTankerClaimCapacity = 0;
    std::int32_t maximumPatrolFuelCost = 0;
    for (CellId cell = 0; cell < config_.map.cell_count(); ++cell) {
        if (deadline_reached()) {
            return coarse_timeout();
        }
        if (config_.map.terrain.at(static_cast<std::size_t>(cell)) == Terrain::Pond) {
            continue;
        }
        for (const RoadStatus status : {RoadStatus::Smooth, RoadStatus::Busy, RoadStatus::Jammed}) {
            maximumPatrolFuelCost = std::max(
                maximumPatrolFuelCost,
                config_.move_cost(cell, status).patrolFuel);
        }
    }
    const std::int32_t unreachable = std::numeric_limits<std::int32_t>::max();
    std::vector<std::vector<std::int32_t>> optimisticByPatrol;
    std::vector<std::vector<std::int32_t>> pessimisticByPatrol;
    std::vector<std::int32_t> bestTime(brandCount, unreachable);
    std::vector<std::int32_t> pessimisticTime(brandCount, unreachable);
    std::vector<SpotIndex> representative(brandCount, kInvalidSpot);
    for (AgentIndex agentIndex = 0; agentIndex < static_cast<AgentIndex>(state.agents.size()); ++agentIndex) {
        if (deadline_reached()) {
            return coarse_timeout();
        }
        const AgentState& agent = state.agents.at(static_cast<std::size_t>(agentIndex));
        if (agent.kind != AgentKind::Patrol) {
            continue;
        }
        const std::int64_t relaxedFuel = static_cast<std::int64_t>(agent.fuel) +
            static_cast<std::int64_t>(remainingSteps) * maximumPatrolFuelCost;
        const std::int32_t optimisticFuelBudget = hasTanker
            ? static_cast<std::int32_t>(std::min<std::int64_t>(
                  relaxedFuel,
                  std::numeric_limits<std::int32_t>::max() / 4))
            : agent.fuel;
        const std::vector<std::int32_t> optimisticCells = fuel_aware_travel_times(
            config_,
            agent,
            RoadStatus::Smooth,
            optimisticFuelBudget);
        if (!hasTanker) {
            const bool canReachSpot = std::any_of(
                config_.spots.begin(),
                config_.spots.end(),
                [&optimisticCells, unreachable](const Spot& spot) {
                    return optimisticCells.at(
                        static_cast<std::size_t>(spot.position)) != unreachable;
                });
            if (canReachSpot) {
                noTankerClaimCapacity +=
                    static_cast<std::int64_t>(remainingDays) + agent.fuel;
            }
        }
        const std::vector<std::int32_t> pessimisticCells = fuel_aware_travel_times(
            config_,
            agent,
            RoadStatus::Jammed,
            agent.fuel);
        std::vector<std::int32_t> optimisticBrands(brandCount, unreachable);
        std::vector<std::int32_t> pessimisticBrands(brandCount, unreachable);
        for (std::int32_t brandIndex = 0; brandIndex < config_.brand_count(); ++brandIndex) {
            if (deadline_reached()) {
                return coarse_timeout();
            }
            const std::size_t brandOffset = static_cast<std::size_t>(brandIndex);
            for (SpotIndex spotIndex = 0; spotIndex < static_cast<SpotIndex>(config_.spots.size()); ++spotIndex) {
                const Spot& spot = config_.spots.at(static_cast<std::size_t>(spotIndex));
                if (spot.brandIndex != brandIndex) {
                    continue;
                }
                const std::int32_t optimistic = optimisticCells.at(static_cast<std::size_t>(spot.position));
                const std::int32_t pessimistic = pessimisticCells.at(static_cast<std::size_t>(spot.position));
                optimisticBrands.at(brandOffset) = std::min(optimisticBrands.at(brandOffset), optimistic);
                pessimisticBrands.at(brandOffset) = std::min(pessimisticBrands.at(brandOffset), pessimistic);
                if (optimistic < bestTime.at(brandOffset) ||
                    (optimistic == bestTime.at(brandOffset) &&
                     (representative.at(brandOffset) == kInvalidSpot || spotIndex < representative.at(brandOffset)))) {
                    bestTime.at(brandOffset) = optimistic;
                    representative.at(brandOffset) = spotIndex;
                }
            }
            pessimisticTime.at(brandOffset) = std::min(
                pessimisticTime.at(brandOffset),
                pessimisticBrands.at(brandOffset));
        }
        optimisticByPatrol.push_back(std::move(optimisticBrands));
        pessimisticByPatrol.push_back(std::move(pessimisticBrands));
    }
    const std::int32_t optimisticAdditional = relaxed_brand_flow(config_, state, ledger, optimisticByPatrol);
    const std::int32_t pessimisticAdditional = relaxed_brand_flow(config_, state, ledger, pessimisticByPatrol);
    const std::int32_t collected = ledger.lifetime_distinct();
    result.coverageCap = collected + optimisticAdditional;
    result.coverageSafe = collected + pessimisticAdditional;
    result.confidenceCoverage = result.coverageSafe;
    result.optimisticLatestDayByBrand.assign(brandCount, state.dayNumber - 1);
    result.latestSafeDayByBrand.assign(brandCount, state.dayNumber - 1);
    result.slackByBrand.assign(brandCount, -1);
    result.optimisticAgentDaySlotsByBrand.assign(brandCount, 0);
    result.safeAgentDaySlotsByBrand.assign(brandCount, 0);
    const std::int32_t missingBrands = config_.brand_count() - collected;
    for (std::int32_t brandIndex = 0; brandIndex < config_.brand_count(); ++brandIndex) {
        if (deadline_reached()) {
            return coarse_timeout();
        }
        const std::size_t brandOffset = static_cast<std::size_t>(brandIndex);
        std::int32_t optimisticLatestDay = state.dayNumber - 1;
        std::int32_t latestSafeDay = state.dayNumber - 1;
        std::int32_t suffixSteps = 0;
        for (std::int32_t dayNumber = config_.day_count(); dayNumber >= state.dayNumber; --dayNumber) {
            suffixSteps += config_.steps_for_day(dayNumber);
            if (optimisticLatestDay < state.dayNumber && bestTime.at(brandOffset) <= suffixSteps) {
                optimisticLatestDay = dayNumber;
            }
            if (latestSafeDay < state.dayNumber && pessimisticTime.at(brandOffset) <= suffixSteps) {
                latestSafeDay = dayNumber;
            }
            if (optimisticLatestDay >= state.dayNumber && latestSafeDay >= state.dayNumber) {
                break;
            }
        }
        std::int32_t cumulativeSteps = 0;
        for (std::int32_t dayNumber = state.dayNumber; dayNumber <= config_.day_count(); ++dayNumber) {
            cumulativeSteps += config_.steps_for_day(dayNumber);
            for (const std::vector<std::int32_t>& byBrand : optimisticByPatrol) {
                if (byBrand.at(brandOffset) <= cumulativeSteps) {
                    ++result.optimisticAgentDaySlotsByBrand.at(brandOffset);
                }
            }
            for (const std::vector<std::int32_t>& byBrand : pessimisticByPatrol) {
                if (byBrand.at(brandOffset) <= cumulativeSteps) {
                    ++result.safeAgentDaySlotsByBrand.at(brandOffset);
                }
            }
        }
        result.optimisticLatestDayByBrand.at(brandOffset) = optimisticLatestDay;
        result.latestSafeDayByBrand.at(brandOffset) = latestSafeDay;
        if (pessimisticTime.at(brandOffset) != unreachable) {
            result.slackByBrand.at(brandOffset) = remainingSteps - pessimisticTime.at(brandOffset);
        } else if (bestTime.at(brandOffset) != unreachable) {
            result.slackByBrand.at(brandOffset) = remainingSteps - bestTime.at(brandOffset);
        }
        const std::int32_t reservationDay = latestSafeDay >= state.dayNumber
            ? latestSafeDay
            : optimisticLatestDay;
        const bool singleRelaxedSlot =
            result.optimisticAgentDaySlotsByBrand.at(brandOffset) == 1 ||
            result.safeAgentDaySlotsByBrand.at(brandOffset) == 1;
        if (!has_brand(ledger.lifetimeBrands, brandIndex) && singleRelaxedSlot &&
            reservationDay == state.dayNumber &&
            representative.at(brandOffset) != kInvalidSpot) {
            const std::int32_t spotCount = static_cast<std::int32_t>(std::count_if(
                config_.spots.begin(),
                config_.spots.end(),
                [brandIndex](const Spot& spot) { return spot.brandIndex == brandIndex; }));
            const bool provenNecessary =
                optimisticAdditional == missingBrands &&
                result.optimisticAgentDaySlotsByBrand.at(brandOffset) == 1 &&
                spotCount == 1;
            result.reservations.push_back(MandatoryReservation{
                brandIndex,
                representative.at(brandOffset),
                reservationDay,
                provenNecessary,
                provenNecessary ? ReservationEvidence::Proven : ReservationEvidence::RelaxationOnly,
            });
        }
    }
    result.matchingRelaxationFeasible = optimisticAdditional >= missingBrands;
    std::int32_t dailyDistinctGainUpper =
        config_.brand_count() * remainingDays;
    std::int32_t servingsGainUpper = perDayServings * remainingDays;
    if (!hasTanker) {
        const std::int32_t resourceClaimUpper =
            static_cast<std::int32_t>(std::min<std::int64_t>(
                noTankerClaimCapacity,
                std::numeric_limits<std::int32_t>::max()));
        dailyDistinctGainUpper = std::min(
            dailyDistinctGainUpper,
            resourceClaimUpper);
        servingsGainUpper = std::min(
            servingsGainUpper,
            resourceClaimUpper);
    }
    result.upperBound = OfficialScore{
        result.coverageCap,
        ledger.totalDailyDistinct + dailyDistinctGainUpper,
        ledger.totalServings + servingsGainUpper,
    };
    result.pessimisticUpperBound = OfficialScore{
        result.coverageSafe,
        ledger.totalDailyDistinct + dailyDistinctGainUpper,
        ledger.totalServings + servingsGainUpper,
    };
    for (std::int32_t coverage = collected; coverage <= result.coverageCap; ++coverage) {
        const OfficialScore& conditionalUpper = coverage <= result.coverageSafe
            ? result.pessimisticUpperBound
            : result.upperBound;
        result.conditionalTiers.push_back(ConditionalTierBounds{
            coverage,
            ledger.totalDailyDistinct,
            conditionalUpper.totalDailyDistinct,
            ledger.totalServings,
            conditionalUpper.totalServings,
            coverage <= collected,
        });
    }
    FutureWitness fixedWitness;
    fixedWitness.score = current_score(ledger);
    fixedWitness.certified = true;
    result.frontier.push_back(ViabilityFrontierPoint{
        collected,
        0,
        current_score(ledger),
        result.upperBound,
        std::move(fixedWitness),
        -1,
        "fixed-ledger",
    });
    if (result.coverageSafe > collected) {
        result.frontier.push_back(ViabilityFrontierPoint{
            result.coverageSafe,
            1,
            current_score(ledger),
            result.pessimisticUpperBound,
            FutureWitness{},
            -1,
            "pessimistic-relaxation",
        });
    }
    if (result.coverageCap > result.coverageSafe) {
        result.frontier.push_back(ViabilityFrontierPoint{
            result.coverageCap,
            2,
            current_score(ledger),
            result.upperBound,
            FutureWitness{},
            -1,
            "optimistic-relaxation",
        });
    }
    return result;
}

FutureWitnessRepairer::FutureWitnessRepairer(
    const MatchConfig& config,
    const RouteColumnGenerator& generator,
    const RouteMaster& master,
    const ExactStepSimulator& simulator,
    const IndependentDayValidator& validator,
    std::int32_t harvestExtensionMode)
    : config_(config),
      generator_(generator),
      master_(master),
      simulator_(simulator),
      validator_(validator),
      harvestExtensionMode_(harvestExtensionMode) {
    if (harvestExtensionMode_ < 0 || harvestExtensionMode_ > 7) {
        throw std::invalid_argument("future harvest extension mode must be in [0,7]");
    }
}

CandidateProfile FutureWitnessRepairer::provisional_profile(
    const MasterCandidate& candidate,
    const DayState& currentState,
    const MatchLedger& ledger,
    const TrafficBelief& belief,
    const ScenarioManifest& manifest,
    OfficialScore validUpperBound,
    std::int32_t fixedOperationCap,
    std::optional<std::chrono::steady_clock::time_point> deadline) const {
    if (fixedOperationCap <= 0) {
        throw std::invalid_argument("F0 requires a positive candidate-independent operation cap");
    }
    CandidateProfile profile;
    profile.validUpperBound = validUpperBound;
    profile.hasValidUpperBound = true;
    profile.provisional = true;
    profile.outcomes.reserve(manifest.scenarios.size());
    profile.scenarioValidUpperBounds.reserve(manifest.scenarios.size());
    const auto deadline_reached = [&deadline]() {
        return deadline.has_value() && std::chrono::steady_clock::now() >= *deadline;
    };
    for (const TrafficScenario& scenario : manifest.scenarios) {
        MatchLedger futureLedger = ledger;
        futureLedger.apply(candidate.simulation.score);
        FutureWitness witness;
        witness.score = candidate.scoreAfterToday;
        witness.certified = false;
        witness.lowerBoundOnly = scenario.pessimisticFallback;
        if (scenario.pessimisticFallback || deadline_reached()) {
            profile.scenarioValidUpperBounds.push_back(validUpperBound);
            profile.outcomes.push_back(ScenarioOutcome{witness.score, std::move(witness)});
            continue;
        }
        if (currentState.dayNumber < config_.day_count()) {
            DayState futureState;
            futureState.dayNumber = currentState.dayNumber + 1;
            futureState.agents = candidate.simulation.finalAgents;
            futureState.others = currentState.others;
            std::vector<std::int32_t> previousOwn = belief.previous_own_footprint();
            std::vector<std::int32_t> currentOwn = candidate.simulation.roadFootprint;
            std::vector<std::int32_t> opponentCarry = scenario.opponentCarryFootprint;
            std::vector<std::int32_t> opponentCurrent = scenario.opponentCurrentFootprint;
            futureState.roadStatuses = predict_with_components(
                config_,
                previousOwn,
                currentOwn,
                opponentCarry,
                opponentCurrent);
            const std::int32_t detailedFutureDays = std::min(
                2,
                config_.day_count() - currentState.dayNumber);
            const std::int32_t finalDetailedDay = currentState.dayNumber + detailedFutureDays;
            const std::int32_t perDayCap = std::max(1, fixedOperationCap / std::max(1, detailedFutureDays));
            for (std::int32_t dayNumber = futureState.dayNumber; dayNumber <= finalDetailedDay; ++dayNumber) {
                if (deadline_reached()) {
                    break;
                }
                futureState.dayNumber = dayNumber;
                ColumnGenerationOptions generationOptions;
                generationOptions.maximumPathsPerTarget = 1;
                generationOptions.maximumColumnsPerAgent = 3;
                generationOptions.maximumTargetSpots = 6;
                generationOptions.maximumEscorts = 4;
                generationOptions.enableHarvestExtensions = harvestExtensionMode_ > 0;
                generationOptions.allowUncachedHarvestTargets = harvestExtensionMode_ > 1;
                generationOptions.enableHarvestOrienteering =
                    harvestExtensionMode_ > 5 &&
                    static_cast<std::int64_t>(config_.fuelLimit) >=
                        3LL * config_.steps_for_day(futureState.dayNumber);
                generationOptions.enableExactHarvestOrienteering =
                    harvestExtensionMode_ > 5 &&
                    static_cast<std::int64_t>(config_.fuelLimit) >=
                        2LL * config_.steps_for_day(futureState.dayNumber) &&
                    (harvestExtensionMode_ > 6 ||
                     futureState.dayNumber == config_.day_count());
                generationOptions.maximumHarvestExtensionSources =
                    harvestExtensionMode_ > 2 ? 4 : 1;
                generationOptions.maximumHarvestExtensionDepth =
                    harvest_extension_depth(
                        config_,
                        futureState.dayNumber,
                        harvestExtensionMode_);
                generationOptions.deadline = deadline;
                MasterOptions masterOptions;
                masterOptions.maximumCombinations = perDayCap;
                masterOptions.maximumCandidates = 1;
                masterOptions.maximumResolveRounds = 1;
                masterOptions.deadline = deadline;
                MasterDiagnostics diagnostics;
                const RoutePortfolio portfolio = generator_.generate(futureState, futureLedger, generationOptions);
                std::vector<MasterCandidate> choices = master_.solve(
                    futureState,
                    futureLedger,
                    portfolio,
                    masterOptions,
                    diagnostics);
                std::optional<MasterCandidate> selected;
                if (!choices.empty()) {
                    selected = std::move(choices.front());
                } else {
                    selected = master_.evaluate_exact_plan(
                        futureState,
                        futureLedger,
                        emergency_wait_plan(config_, futureState));
                }
                if (!selected.has_value()) {
                    break;
                }
                futureLedger.apply(selected->simulation.score);
                witness.score = current_score(futureLedger);
                previousOwn = currentOwn;
                currentOwn = selected->simulation.roadFootprint;
                opponentCarry = opponentCurrent;
                std::fill(opponentCurrent.begin(), opponentCurrent.end(), 0);
                futureState.agents = selected->simulation.finalAgents;
                if (dayNumber < finalDetailedDay) {
                    futureState.roadStatuses = predict_with_components(
                        config_,
                        previousOwn,
                        currentOwn,
                        opponentCarry,
                        opponentCurrent);
                }
            }
        }
        profile.scenarioValidUpperBounds.push_back(validUpperBound);
        profile.outcomes.push_back(ScenarioOutcome{witness.score, std::move(witness)});
    }
    return profile;
}

void FutureWitnessRepairer::repair_profile(
    CandidateProfile& profile,
    const MasterCandidate& candidate,
    const DayState& currentState,
    const MatchLedger& ledger,
    const TrafficBelief& belief,
    const ScenarioManifest& manifest,
    std::int32_t routeCombinationCap,
    std::chrono::steady_clock::time_point deadline) const {
    if (profile.outcomes.size() != manifest.scenarios.size()) {
        throw std::invalid_argument("profile does not align with the frozen scenario manifest");
    }
    const auto build_exact_bundle_witness =
        [this,
         &candidate,
         &currentState,
         &ledger,
         &belief,
         deadline](const TrafficScenario& scenario)
            -> std::optional<FutureWitness> {
            if (currentState.dayNumber >= config_.day_count() ||
                std::chrono::steady_clock::now() >= deadline) {
                return std::nullopt;
            }
            MatchLedger futureLedger = ledger;
            futureLedger.apply(candidate.simulation.score);
            FutureWitness witness;
            witness.score = candidate.scoreAfterToday;
            witness.certified = true;
            DayState futureState;
            futureState.dayNumber = currentState.dayNumber + 1;
            futureState.agents = candidate.simulation.finalAgents;
            futureState.others = currentState.others;
            std::vector<std::int32_t> previousOwn =
                belief.previous_own_footprint();
            std::vector<std::int32_t> currentOwn =
                candidate.simulation.roadFootprint;
            std::vector<std::int32_t> opponentCarry =
                scenario.opponentCarryFootprint;
            std::vector<std::int32_t> opponentCurrent =
                scenario.opponentCurrentFootprint;
            futureState.roadStatuses = predict_with_components(
                config_,
                previousOwn,
                currentOwn,
                opponentCarry,
                opponentCurrent);
            const FastViabilityAnalyzer viability(config_);
            for (std::int32_t dayNumber = futureState.dayNumber;
                 dayNumber <= config_.day_count();
                 ++dayNumber) {
                if (std::chrono::steady_clock::now() >= deadline) {
                    return std::nullopt;
                }
                futureState.dayNumber = dayNumber;
                ColumnGenerationOptions generationOptions;
                generationOptions.maximumPathsPerTarget = 1;
                generationOptions.maximumColumnsPerAgent = 3;
                generationOptions.maximumTargetSpots = 6;
                generationOptions.maximumEscorts = 4;
                generationOptions.enableHarvestExtensions =
                    harvestExtensionMode_ > 0;
                generationOptions.allowUncachedHarvestTargets = true;
                generationOptions.enableExactHarvestOrienteering =
                    harvestExtensionMode_ > 5;
                generationOptions
                    .enableFuelConstrainedExactHarvestOrienteering =
                    harvestExtensionMode_ > 6;
                generationOptions
                    .enableAnytimeFuelConstrainedHarvestOrienteering = false;
                generationOptions.maximumHarvestExtensionSources =
                    harvestExtensionMode_ > 2 ? 4 : 1;
                generationOptions.maximumHarvestExtensionDepth =
                    harvest_extension_depth(
                        config_,
                        futureState.dayNumber,
                        harvestExtensionMode_);
                generationOptions.deadline = deadline;
                ColumnGenerationDiagnostics generationDiagnostics;
                const RoutePortfolio portfolio = generator_.generate(
                    futureState,
                    futureLedger,
                    generationOptions,
                    &generationDiagnostics);
                if (generationDiagnostics.exactOrienteeringSupportedAgents == 0 ||
                    generationDiagnostics.exactOrienteeringCompleteAgents !=
                        generationDiagnostics.exactOrienteeringSupportedAgents) {
                    return std::nullopt;
                }
                struct ExactBundlePlan {
                    DayPlan plan;
                    std::vector<bool> assigned;
                    bool duplicateAgent = false;
                };
                std::map<std::int32_t, ExactBundlePlan> bundles;
                for (std::size_t agent = 0;
                     agent < portfolio.columnsByAgent.size();
                     ++agent) {
                    for (const RouteColumn& column :
                         portfolio.columnsByAgent.at(agent)) {
                        if (!column.exactOrienteering ||
                            column.contingencyBundle < 0) {
                            continue;
                        }
                        ExactBundlePlan& bundle =
                            bundles[column.contingencyBundle];
                        if (bundle.plan.actions.empty()) {
                            bundle.plan.actions.resize(
                                static_cast<std::size_t>(
                                    config_.agent_count()));
                            bundle.assigned.assign(
                                static_cast<std::size_t>(
                                    config_.agent_count()),
                                false);
                        }
                        if (bundle.assigned.at(agent)) {
                            bundle.duplicateAgent = true;
                            continue;
                        }
                        bundle.plan.actions.at(agent) = column.actions;
                        bundle.assigned.at(agent) = true;
                    }
                }
                std::optional<MasterCandidate> selected;
                OfficialScore selectedUpper;
                bool hasSelectedUpper = false;
                for (auto& [bundleId, bundle] : bundles) {
                    static_cast<void>(bundleId);
                    if (bundle.duplicateAgent ||
                        bundle.assigned.size() !=
                            static_cast<std::size_t>(config_.agent_count()) ||
                        !std::all_of(
                            bundle.assigned.begin(),
                            bundle.assigned.end(),
                            [](bool assigned) { return assigned; })) {
                        continue;
                    }
                    std::optional<MasterCandidate> exact =
                        master_.evaluate_exact_plan(
                            futureState,
                            futureLedger,
                            bundle.plan);
                    if (!exact.has_value()) {
                        continue;
                    }
                    MatchLedger nextLedger = futureLedger;
                    nextLedger.apply(exact->simulation.score);
                    OfficialScore upper = exact->scoreAfterToday;
                    if (dayNumber < config_.day_count()) {
                        DayState nextState = futureState;
                        nextState.dayNumber = dayNumber + 1;
                        nextState.agents = exact->simulation.finalAgents;
                        nextState.roadStatuses = predict_with_components(
                            config_,
                            currentOwn,
                            exact->simulation.roadFootprint,
                            opponentCurrent,
                            std::vector<std::int32_t>(
                                opponentCurrent.size(),
                                0));
                        upper = viability.analyze(
                            nextState,
                            nextLedger,
                            deadline).upperBound;
                    }
                    if (!selected.has_value() ||
                        selectedUpper < upper ||
                        (selectedUpper == upper &&
                         selected->scoreAfterToday < exact->scoreAfterToday) ||
                        (selectedUpper == upper &&
                         selected->scoreAfterToday == exact->scoreAfterToday &&
                         exact->stableId < selected->stableId)) {
                        selected = std::move(exact);
                        selectedUpper = upper;
                        hasSelectedUpper = true;
                    }
                }
                if (!selected.has_value() || !hasSelectedUpper) {
                    return std::nullopt;
                }
                witness.futurePlans.push_back(selected->plan);
                futureLedger.apply(selected->simulation.score);
                witness.score = current_score(futureLedger);
                previousOwn = currentOwn;
                currentOwn = selected->simulation.roadFootprint;
                opponentCarry = opponentCurrent;
                std::fill(
                    opponentCurrent.begin(),
                    opponentCurrent.end(),
                    0);
                futureState.agents = selected->simulation.finalAgents;
                if (dayNumber < config_.day_count()) {
                    futureState.roadStatuses = predict_with_components(
                        config_,
                        previousOwn,
                        currentOwn,
                        opponentCarry,
                        opponentCurrent);
                }
            }
            return witness;
        };
    for (std::size_t scenarioIndex = 0; scenarioIndex < manifest.scenarios.size(); ++scenarioIndex) {
        const TrafficScenario& scenario = manifest.scenarios.at(scenarioIndex);
        FutureWitness certifiedFloor = build_monotone_wait_floor(
            config_,
            candidate,
            currentState);
        profile.outcomes.at(scenarioIndex) = ScenarioOutcome{
            certifiedFloor.score,
            std::move(certifiedFloor),
        };
        if (scenario.pessimisticFallback) {
            continue;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            continue;
        }
        MatchLedger futureLedger = ledger;
        futureLedger.apply(candidate.simulation.score);
        FutureWitness witness;
        witness.score = candidate.scoreAfterToday;
        witness.certified = true;
        if (currentState.dayNumber >= config_.day_count()) {
            profile.outcomes.at(scenarioIndex) = ScenarioOutcome{witness.score, std::move(witness)};
            continue;
        }
        DayState futureState;
        futureState.dayNumber = currentState.dayNumber + 1;
        futureState.agents = candidate.simulation.finalAgents;
        futureState.others = currentState.others;
        std::vector<std::int32_t> previousOwn = belief.previous_own_footprint();
        std::vector<std::int32_t> currentOwn = candidate.simulation.roadFootprint;
        std::vector<std::int32_t> opponentCarry = scenario.opponentCarryFootprint;
        std::vector<std::int32_t> opponentCurrent = scenario.opponentCurrentFootprint;
        futureState.roadStatuses = predict_with_components(
            config_,
            previousOwn,
            currentOwn,
            opponentCarry,
            opponentCurrent);

        const std::int32_t detailedFutureDays = std::min(
            2,
            config_.day_count() - currentState.dayNumber);
        const std::int32_t finalDetailedDay = currentState.dayNumber + detailedFutureDays;
        const std::int32_t perDayRepairCap = std::max(
            1,
            routeCombinationCap / std::max(1, detailedFutureDays));
        for (std::int32_t dayNumber = futureState.dayNumber; dayNumber <= config_.day_count(); ++dayNumber) {
            if (std::chrono::steady_clock::now() >= deadline) {
                witness.certified = false;
                break;
            }
            futureState.dayNumber = dayNumber;
            DayPlan plan;
            if (dayNumber <= finalDetailedDay) {
                ColumnGenerationOptions generationOptions;
                generationOptions.maximumPathsPerTarget = 1;
                generationOptions.maximumColumnsPerAgent = 3;
                generationOptions.maximumTargetSpots = 6;
                generationOptions.maximumEscorts = 4;
                generationOptions.enableHarvestExtensions = harvestExtensionMode_ > 0;
                generationOptions.allowUncachedHarvestTargets = harvestExtensionMode_ > 1;
                generationOptions.enableHarvestOrienteering =
                    harvestExtensionMode_ > 5 &&
                    static_cast<std::int64_t>(config_.fuelLimit) >=
                        3LL * config_.steps_for_day(futureState.dayNumber);
                generationOptions.enableExactHarvestOrienteering =
                    harvestExtensionMode_ > 5 &&
                    static_cast<std::int64_t>(config_.fuelLimit) >=
                        2LL * config_.steps_for_day(futureState.dayNumber) &&
                    (harvestExtensionMode_ > 6 ||
                     futureState.dayNumber == config_.day_count());
                generationOptions.maximumHarvestExtensionSources =
                    harvestExtensionMode_ > 2 ? 4 : 1;
                generationOptions.maximumHarvestExtensionDepth =
                    harvest_extension_depth(
                        config_,
                        futureState.dayNumber,
                        harvestExtensionMode_);
                generationOptions.deadline = deadline;
                MasterOptions masterOptions;
                masterOptions.maximumCombinations = perDayRepairCap;
                masterOptions.maximumCandidates = 1;
                masterOptions.maximumResolveRounds = 1;
                masterOptions.deadline = deadline;
                const RoutePortfolio portfolio = generator_.generate(futureState, futureLedger, generationOptions);
                MasterDiagnostics diagnostics;
                std::vector<MasterCandidate> candidates = master_.solve(
                    futureState,
                    futureLedger,
                    portfolio,
                    masterOptions,
                    diagnostics);
                if (candidates.empty()) {
                    witness.certified = false;
                    break;
                }
                plan = std::move(candidates.front().plan);
            } else {
                const ViabilityBounds terminalViability = FastViabilityAnalyzer(config_).analyze(
                    futureState,
                    futureLedger,
                    deadline);
                ColumnGenerationOptions generationOptions;
                generationOptions.maximumPathsPerTarget = 1;
                generationOptions.maximumColumnsPerAgent = 2;
                generationOptions.maximumTargetSpots = 4;
                generationOptions.maximumEscorts = 2;
                generationOptions.maximumSeedPlans = 1;
                generationOptions.enableHarvestExtensions = harvestExtensionMode_ > 0;
                generationOptions.allowUncachedHarvestTargets = harvestExtensionMode_ > 1;
                generationOptions.enableHarvestOrienteering =
                    harvestExtensionMode_ > 5 &&
                    static_cast<std::int64_t>(config_.fuelLimit) >=
                        3LL * config_.steps_for_day(futureState.dayNumber);
                generationOptions.enableExactHarvestOrienteering =
                    harvestExtensionMode_ > 5 &&
                    static_cast<std::int64_t>(config_.fuelLimit) >=
                        2LL * config_.steps_for_day(futureState.dayNumber) &&
                    (harvestExtensionMode_ > 6 ||
                     futureState.dayNumber == config_.day_count());
                generationOptions.maximumHarvestExtensionSources =
                    harvestExtensionMode_ > 2 ? 4 : 1;
                generationOptions.maximumHarvestExtensionDepth =
                    harvest_extension_depth(
                        config_,
                        futureState.dayNumber,
                        harvestExtensionMode_);
                generationOptions.mandatoryReservations = terminalViability.reservations;
                generationOptions.deadline = deadline;
                MasterOptions masterOptions;
                masterOptions.maximumCombinations = std::max(16, perDayRepairCap / 4);
                masterOptions.maximumCandidates = 1;
                masterOptions.maximumResolveRounds = 1;
                masterOptions.mandatoryReservations = terminalViability.reservations;
                masterOptions.deadline = deadline;
                MasterDiagnostics terminalDiagnostics;
                const RoutePortfolio terminalPortfolio = generator_.generate(
                    futureState,
                    futureLedger,
                    generationOptions);
                std::vector<MasterCandidate> terminalCandidates = master_.solve(
                    futureState,
                    futureLedger,
                    terminalPortfolio,
                    masterOptions,
                    terminalDiagnostics);
                plan = terminalCandidates.empty()
                    ? emergency_wait_plan(config_, futureState)
                    : std::move(terminalCandidates.front().plan);
            }
            const SimulationResult detailed = simulator_.simulate(futureState, plan, true);
            const SimulationResult validation = validator_.validate(futureState, plan, true);
            std::string mismatch;
            if (!detailed.valid || !validator_.agrees_with(detailed, validation, mismatch)) {
                witness.certified = false;
                break;
            }
            witness.futurePlans.push_back(std::move(plan));
            futureLedger.apply(detailed.score);
            witness.score = current_score(futureLedger);
            previousOwn = currentOwn;
            currentOwn = detailed.roadFootprint;
            opponentCarry = opponentCurrent;
            std::fill(opponentCurrent.begin(), opponentCurrent.end(), 0);
            futureState.agents = detailed.finalAgents;
            if (dayNumber < config_.day_count()) {
                futureState.roadStatuses = predict_with_components(
                    config_,
                    previousOwn,
                    currentOwn,
                    opponentCarry,
                    opponentCurrent);
            }
        }
        if (!witness.certified) {
            FutureWitness fallback = build_wait_witness(
                config_,
                simulator_,
                validator_,
                candidate,
                currentState,
                ledger,
                belief,
                scenario,
                deadline);
            if (fallback.certified) {
                profile.outcomes.at(scenarioIndex) = ScenarioOutcome{
                    fallback.score,
                    std::move(fallback),
                };
            }
        } else {
            const std::optional<FutureWitness> exactBundleWitness =
                build_exact_bundle_witness(scenario);
            if (exactBundleWitness.has_value() &&
                witness.score < exactBundleWitness->score) {
                witness = *exactBundleWitness;
            }
            profile.outcomes.at(scenarioIndex) =
                ScenarioOutcome{witness.score, std::move(witness)};
        }
    }
    profile.provisional = !all_outcomes_certified(profile);
}

LexicographicRiskComparator::LexicographicRiskComparator(RiskPolicy policy)
    : policy_(std::move(policy)) {
    if (policy_.version.empty() || policy_.confidenceBasisPoints > 10000U || policy_.safetySlack < 0 ||
        policy_.resolutionBasisPoints == 0U || policy_.resolutionBasisPoints > 10000U) {
        throw std::invalid_argument("risk policy has invalid scalar parameters");
    }
    for (std::size_t quantileIndex = 0; quantileIndex < policy_.quantileBasisPoints.size(); ++quantileIndex) {
        if (policy_.quantileBasisPoints.at(quantileIndex) > 10000U ||
            (quantileIndex > 0U &&
             policy_.quantileBasisPoints.at(quantileIndex - 1U) < policy_.quantileBasisPoints.at(quantileIndex))) {
            throw std::invalid_argument("risk policy quantiles must form a descending basis-point ladder");
        }
    }
}

void LexicographicRiskComparator::finalize_profiles(
    std::vector<CandidateEvaluation>& evaluations,
    const ScenarioManifest& manifest,
    ProfileFinalizeDiagnostics* diagnostics) const {
    const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    std::int32_t survivalTieGroups = 0;
    std::int32_t survivalProfilesRebuilt = 0;
    if (manifest.totalWeight == 0U) {
        throw std::invalid_argument("scenario manifest has no probability mass");
    }
    if (!manifest.riskPolicyVersion.empty() &&
        (manifest.riskPolicyVersion != policy_.version ||
         manifest.confidenceBasisPoints != policy_.confidenceBasisPoints ||
         manifest.safetySlack != policy_.safetySlack ||
         manifest.resolutionBasisPoints != policy_.resolutionBasisPoints ||
         manifest.quantileBasisPoints != policy_.quantileBasisPoints ||
         manifest.evaluatorHash.empty())) {
        throw std::invalid_argument("scenario manifest risk/evaluator contract does not match the frozen policy");
    }
    std::uint64_t observedWeight = 0;
    bool hasPessimisticFallback = false;
    for (const TrafficScenario& scenario : manifest.scenarios) {
        if (scenario.privateSensitivity) {
            throw std::invalid_argument("private sensitivity scenario cannot enter a public manifest");
        }
        observedWeight += scenario.weight;
        hasPessimisticFallback = hasPessimisticFallback || scenario.pessimisticFallback;
    }
    if (observedWeight != manifest.totalWeight) {
        throw std::invalid_argument("scenario manifest total weight does not match its scenarios");
    }
    for (CandidateEvaluation& evaluation : evaluations) {
        CandidateProfile& profile = evaluation.profile;
        if (profile.outcomes.size() != manifest.scenarios.size()) {
            throw std::invalid_argument("candidate profile does not cover the manifest");
        }
        if (profile.hasValidUpperBound) {
            if (profile.scenarioValidUpperBounds.empty()) {
                profile.scenarioValidUpperBounds.assign(profile.outcomes.size(), profile.validUpperBound);
            }
            if (profile.scenarioValidUpperBounds.size() != profile.outcomes.size()) {
                throw std::invalid_argument("candidate upper bounds do not cover the manifest");
            }
            profile.validUpperBound = *std::max_element(
                profile.scenarioValidUpperBounds.begin(),
                profile.scenarioValidUpperBounds.end());
        } else if (!profile.scenarioValidUpperBounds.empty()) {
            throw std::invalid_argument("candidate profile has scenario upper bounds without a validity flag");
        }
        profile.coverageCap = 0;
        profile.coverageSafe = std::numeric_limits<std::int32_t>::max();
        bool hasAdversarial = false;
        for (std::size_t scenarioIndex = 0; scenarioIndex < profile.outcomes.size(); ++scenarioIndex) {
            const ScenarioOutcome& outcome = profile.outcomes.at(scenarioIndex);
            profile.coverageCap = std::max(profile.coverageCap, outcome.score.lifetimeDistinct);
            if (manifest.scenarios.at(scenarioIndex).adversarial) {
                hasAdversarial = true;
                profile.coverageSafe = std::min(profile.coverageSafe, outcome.score.lifetimeDistinct);
            }
        }
        if (!hasAdversarial) {
            for (const ScenarioOutcome& outcome : profile.outcomes) {
                profile.coverageSafe = std::min(profile.coverageSafe, outcome.score.lifetimeDistinct);
            }
        }
        if (profile.coverageSafe == std::numeric_limits<std::int32_t>::max()) {
            profile.coverageSafe = 0;
        }
        profile.confidenceCoverage = confidence_coverage(profile.outcomes, manifest, policy_.confidenceBasisPoints);
        for (std::size_t quantileIndex = 0; quantileIndex < profile.quantiles.size(); ++quantileIndex) {
            profile.quantiles.at(quantileIndex) = weighted_quantile(
                profile.outcomes,
                manifest,
                policy_.quantileBasisPoints.at(quantileIndex));
        }
        profile.provisionalLowerBound = minimum_score(profile.outcomes);
        profile.certifiedLowerBound = all_outcomes_certified(profile)
            ? profile.provisionalLowerBound
            : OfficialScore{};
        profile.conditionalTiers.clear();
        profile.frontier.clear();
        std::int32_t maximumCoverage = evaluation.candidate.scoreAfterToday.lifetimeDistinct;
        for (std::size_t scenarioIndex = 0; scenarioIndex < profile.outcomes.size(); ++scenarioIndex) {
            const ScenarioOutcome& outcome = profile.outcomes.at(scenarioIndex);
            const OfficialScore upper = profile.hasValidUpperBound
                ? profile.scenarioValidUpperBounds.at(scenarioIndex)
                : outcome.score;
            maximumCoverage = std::max(
                maximumCoverage,
                std::max(outcome.score.lifetimeDistinct, upper.lifetimeDistinct));
            ViabilityFrontierPoint point;
            point.coverage = outcome.score.lifetimeDistinct;
            point.riskRank = manifest.scenarios.at(scenarioIndex).adversarial ? 0 : 1;
            point.lowerBound = outcome.witness.certified
                ? outcome.score
                : evaluation.candidate.scoreAfterToday;
            point.upperBound = upper;
            point.witness = outcome.witness;
            point.scenarioId = manifest.scenarios.at(scenarioIndex).scenarioId;
            point.scenarioClass = manifest.scenarios.at(scenarioIndex).scenarioClass;
            profile.frontier.push_back(std::move(point));
        }
        for (std::int32_t coverage = evaluation.candidate.scoreAfterToday.lifetimeDistinct;
             coverage <= maximumCoverage;
             ++coverage) {
            ConditionalTierBounds conditional;
            conditional.coverage = coverage;
            bool hasLower = false;
            bool hasUpper = false;
            for (std::size_t scenarioIndex = 0; scenarioIndex < profile.outcomes.size(); ++scenarioIndex) {
                const ScenarioOutcome& outcome = profile.outcomes.at(scenarioIndex);
                if (outcome.witness.certified && outcome.score.lifetimeDistinct >= coverage) {
                    if (!hasLower || outcome.score.totalDailyDistinct > conditional.lowerDailyDistinct) {
                        conditional.lowerDailyDistinct = outcome.score.totalDailyDistinct;
                        conditional.lowerServings = outcome.score.totalServings;
                    } else if (outcome.score.totalDailyDistinct == conditional.lowerDailyDistinct) {
                        conditional.lowerServings = std::max(
                            conditional.lowerServings,
                            outcome.score.totalServings);
                    }
                    hasLower = true;
                }
                const OfficialScore upper = profile.hasValidUpperBound
                    ? profile.scenarioValidUpperBounds.at(scenarioIndex)
                    : outcome.score;
                if (upper.lifetimeDistinct >= coverage) {
                    if (!hasUpper || upper.totalDailyDistinct > conditional.upperDailyDistinct) {
                        conditional.upperDailyDistinct = upper.totalDailyDistinct;
                        conditional.upperServings = upper.totalServings;
                    } else if (upper.totalDailyDistinct == conditional.upperDailyDistinct) {
                        conditional.upperServings = std::max(
                            conditional.upperServings,
                            upper.totalServings);
                    }
                    hasUpper = true;
                }
            }
            if (!hasLower) {
                conditional.lowerDailyDistinct = evaluation.candidate.scoreAfterToday.totalDailyDistinct;
                conditional.lowerServings = evaluation.candidate.scoreAfterToday.totalServings;
            }
            if (!hasUpper) {
                conditional.upperDailyDistinct = conditional.lowerDailyDistinct;
                conditional.upperServings = conditional.lowerServings;
            }
            conditional.witnessBacked = hasLower;
            profile.conditionalTiers.push_back(conditional);
        }
        std::vector<bool> dominated(profile.frontier.size(), false);
        for (std::size_t candidateIndex = 0; candidateIndex < profile.frontier.size(); ++candidateIndex) {
            for (std::size_t challengerIndex = 0; challengerIndex < profile.frontier.size(); ++challengerIndex) {
                if (candidateIndex == challengerIndex) {
                    continue;
                }
                const ViabilityFrontierPoint& candidatePoint = profile.frontier.at(candidateIndex);
                const ViabilityFrontierPoint& challengerPoint = profile.frontier.at(challengerIndex);
                const bool noWorse =
                    compare_lexicographic(challengerPoint.lowerBound, candidatePoint.lowerBound) >= 0 &&
                    compare_lexicographic(challengerPoint.upperBound, candidatePoint.upperBound) >= 0 &&
                    challengerPoint.riskRank <= candidatePoint.riskRank;
                const bool strictlyBetter =
                    compare_lexicographic(challengerPoint.lowerBound, candidatePoint.lowerBound) > 0 ||
                    compare_lexicographic(challengerPoint.upperBound, candidatePoint.upperBound) > 0 ||
                    challengerPoint.riskRank < candidatePoint.riskRank;
                if (noWorse && strictlyBetter) {
                    dominated.at(candidateIndex) = true;
                    break;
                }
            }
        }
        std::vector<ViabilityFrontierPoint> nondominated;
        nondominated.reserve(profile.frontier.size());
        for (std::size_t pointIndex = 0; pointIndex < profile.frontier.size(); ++pointIndex) {
            if (!dominated.at(pointIndex)) {
                nondominated.push_back(std::move(profile.frontier.at(pointIndex)));
            }
        }
        profile.frontier = std::move(nondominated);
        profile.scenarioWeights.clear();
        profile.scenarioWeights.reserve(manifest.scenarios.size());
        for (const TrafficScenario& scenario : manifest.scenarios) {
            profile.scenarioWeights.push_back(scenario.weight);
        }
        profile.survivalSignature.clear();
        profile.signatureEnabled = manifest.survivalSignatureEnabled && manifest.requiredClassesCovered &&
            !hasPessimisticFallback;
    }

    std::vector<bool> grouped(evaluations.size(), false);
    for (std::size_t anchorIndex = 0; anchorIndex < evaluations.size(); ++anchorIndex) {
        if (grouped.at(anchorIndex) || !evaluations.at(anchorIndex).profile.signatureEnabled) {
            continue;
        }
        std::vector<std::size_t> group{anchorIndex};
        grouped.at(anchorIndex) = true;
        for (std::size_t candidateIndex = anchorIndex + 1U; candidateIndex < evaluations.size(); ++candidateIndex) {
            if (!grouped.at(candidateIndex) &&
                same_quantiles(evaluations.at(anchorIndex).profile, evaluations.at(candidateIndex).profile)) {
                grouped.at(candidateIndex) = true;
                group.push_back(candidateIndex);
            }
        }
        if (group.size() < 2U) {
            continue;
        }
        ++survivalTieGroups;
        survivalProfilesRebuilt += static_cast<std::int32_t>(group.size());
        std::vector<OfficialScore> support;
        for (const std::size_t candidateIndex : group) {
            for (const ScenarioOutcome& outcome : evaluations.at(candidateIndex).profile.outcomes) {
                support.push_back(outcome.score);
            }
        }
        std::sort(support.begin(), support.end());
        support.erase(std::unique(support.begin(), support.end()), support.end());
        for (const std::size_t candidateIndex : group) {
            CandidateProfile& profile = evaluations.at(candidateIndex).profile;
            for (std::size_t thresholdIndex = 1; thresholdIndex < support.size(); ++thresholdIndex) {
                std::uint64_t survivingWeight = 0;
                for (std::size_t scenarioIndex = 0; scenarioIndex < profile.outcomes.size(); ++scenarioIndex) {
                    if (!(profile.outcomes.at(scenarioIndex).score < support.at(thresholdIndex))) {
                        survivingWeight += manifest.scenarios.at(scenarioIndex).weight;
                    }
                }
                const std::uint64_t probabilityBasisPoints = survivingWeight * 10000U / manifest.totalWeight;
                profile.survivalSignature.push_back(static_cast<std::int32_t>(
                    probabilityBasisPoints / std::max<std::uint32_t>(1U, policy_.resolutionBasisPoints)));
            }
        }
    }
    if (diagnostics != nullptr) {
        ++diagnostics->passes;
        diagnostics->candidatesFinalized += static_cast<std::int32_t>(evaluations.size());
        diagnostics->survivalTieGroups += survivalTieGroups;
        diagnostics->survivalProfilesRebuilt += survivalProfilesRebuilt;
        diagnostics->elapsed += std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - started);
    }
}

std::size_t LexicographicRiskComparator::choose(
    const std::vector<CandidateEvaluation>& evaluations,
    bool requireUndominatedCurrentFloor) const {
    if (evaluations.empty()) {
        throw std::invalid_argument("cannot choose from an empty candidate pool");
    }
    std::int32_t bestConfidence = 0;
    for (const CandidateEvaluation& evaluation : evaluations) {
        bestConfidence = std::max(bestConfidence, evaluation.profile.confidenceCoverage);
    }
    OfficialScore currentFloor;
    bool hasCurrentFloor = false;
    if (requireUndominatedCurrentFloor) {
        for (const CandidateEvaluation& evaluation : evaluations) {
            if (evaluation.profile.confidenceCoverage <
                bestConfidence - policy_.safetySlack) {
                continue;
            }
            if (!hasCurrentFloor ||
                compare_lexicographic(
                    evaluation.candidate.scoreAfterToday,
                    currentFloor) > 0) {
                currentFloor = evaluation.candidate.scoreAfterToday;
                hasCurrentFloor = true;
            }
        }
    }
    std::size_t selected = 0;
    bool hasSelected = false;
    for (std::size_t candidateIndex = 0; candidateIndex < evaluations.size(); ++candidateIndex) {
        const CandidateEvaluation& candidate = evaluations.at(candidateIndex);
        if (candidate.profile.confidenceCoverage < bestConfidence - policy_.safetySlack) {
            continue;
        }
        if (hasCurrentFloor &&
            compare_lexicographic(
                candidate.candidate.scoreAfterToday,
                currentFloor) < 0) {
            continue;
        }
        if (!hasSelected || better_evaluation(candidate, evaluations.at(selected))) {
            selected = candidateIndex;
            hasSelected = true;
        }
    }
    if (!hasSelected) {
        throw std::runtime_error("relative confidence gate became empty");
    }
    return selected;
}

std::size_t LexicographicRiskComparator::choose_provisional(
    const std::vector<CandidateEvaluation>& evaluations) const {
    if (evaluations.empty()) {
        throw std::invalid_argument("cannot choose from an empty provisional candidate pool");
    }
    std::int32_t bestConfidence = 0;
    for (const CandidateEvaluation& evaluation : evaluations) {
        bestConfidence = std::max(bestConfidence, evaluation.profile.confidenceCoverage);
    }
    std::size_t selected = 0;
    bool hasSelected = false;
    for (std::size_t candidateIndex = 0; candidateIndex < evaluations.size(); ++candidateIndex) {
        const CandidateEvaluation& candidate = evaluations.at(candidateIndex);
        if (candidate.profile.confidenceCoverage < bestConfidence - policy_.safetySlack) {
            continue;
        }
        if (!hasSelected || better_provisional_evaluation(candidate, evaluations.at(selected))) {
            selected = candidateIndex;
            hasSelected = true;
        }
    }
    if (!hasSelected) {
        throw std::runtime_error("relative confidence gate became empty for provisional candidates");
    }
    return selected;
}

bool LexicographicRiskComparator::certified_dominates(
    const CandidateProfile& challenger,
    const CandidateProfile& incumbent) const {
    if (!all_outcomes_certified(challenger) || !incumbent.hasValidUpperBound ||
        challenger.outcomes.size() != incumbent.outcomes.size() ||
        challenger.outcomes.size() != challenger.scenarioWeights.size() ||
        challenger.scenarioWeights != incumbent.scenarioWeights ||
        incumbent.scenarioValidUpperBounds.size() != incumbent.outcomes.size()) {
        return false;
    }
    std::vector<OfficialScore> support;
    support.reserve(challenger.outcomes.size() + incumbent.scenarioValidUpperBounds.size());
    for (const ScenarioOutcome& outcome : challenger.outcomes) {
        support.push_back(outcome.score);
    }
    support.insert(
        support.end(),
        incumbent.scenarioValidUpperBounds.begin(),
        incumbent.scenarioValidUpperBounds.end());
    std::sort(support.begin(), support.end());
    support.erase(std::unique(support.begin(), support.end()), support.end());
    bool strictlyBetter = false;
    for (const OfficialScore& threshold : support) {
        std::uint64_t challengerWeight = 0;
        for (std::size_t scenarioIndex = 0; scenarioIndex < challenger.outcomes.size(); ++scenarioIndex) {
            const std::uint32_t weight = challenger.scenarioWeights.at(scenarioIndex);
            if (!(challenger.outcomes.at(scenarioIndex).score < threshold)) {
                challengerWeight += weight;
            }
        }
        std::uint64_t incumbentPossibleWeight = 0;
        for (std::size_t scenarioIndex = 0; scenarioIndex < incumbent.scenarioValidUpperBounds.size(); ++scenarioIndex) {
            if (!(incumbent.scenarioValidUpperBounds.at(scenarioIndex) < threshold)) {
                incumbentPossibleWeight += incumbent.scenarioWeights.at(scenarioIndex);
            }
        }
        if (challengerWeight < incumbentPossibleWeight) {
            return false;
        }
        strictlyBetter = strictlyBetter || challengerWeight > incumbentPossibleWeight;
    }
    return strictlyBetter;
}

const RiskPolicy& LexicographicRiskComparator::policy() const {
    return policy_;
}

DeadlineScheduler::DeadlineScheduler(DeadlineCalibration calibration)
    : calibration_(std::move(calibration)) {
    if (calibration_.seedFloor.count() < 0 || calibration_.validationFloor.count() < 0 ||
        calibration_.networkFloor.count() < 0 || calibration_.seedPercent < 0 ||
        calibration_.shortFastViabilityPercent < 0 || calibration_.normalFastViabilityPercent < 0 ||
        calibration_.certificationPercent < 0 || calibration_.networkPercent < 0 ||
        calibration_.shortSearchSoftPercent <= 0 || calibration_.shortSearchSoftPercent > 100 ||
        calibration_.normalSearchSoftPercent <= 0 || calibration_.normalSearchSoftPercent > 100 ||
        calibration_.longSearchSoftPercent <= 0 || calibration_.longSearchSoftPercent > 100 ||
        calibration_.shortF0OperationCap <= 0 || calibration_.normalF0OperationCap <= 0 ||
        calibration_.longF0OperationCap <= 0 || calibration_.shortF0CandidateLimit <= 0 ||
        calibration_.normalF0CandidateLimit <= 0 || calibration_.longF0CandidateLimit <= 0 ||
        calibration_.normalProofGuidedPasses < 0 || calibration_.longProofGuidedPasses < 0 ||
        calibration_.f0SearchReservePercent < 0 || calibration_.f0SearchReservePercent >= 100 ||
        calibration_.f0BoundaryGuard.count() < 0 ||
        calibration_.shortThreshold.count() < 0 || calibration_.normalThreshold < calibration_.shortThreshold) {
        throw std::invalid_argument("deadline calibration cannot contain negative values");
    }
    if (calibration_.requireCompetitionReadiness && !calibration_.p99Calibrated) {
        throw std::invalid_argument(
            "competition deadline mode requires a measured p99 calibration");
    }
}

DeadlineProfile DeadlineScheduler::classify(std::chrono::milliseconds available) const {
    const std::chrono::milliseconds seedFloor = calibration_.seedFloor;
    const std::chrono::milliseconds validationFloor = calibration_.validationFloor;
    const std::chrono::milliseconds networkFloor = calibration_.networkFloor;
    const std::chrono::milliseconds required = seedFloor + validationFloor + networkFloor;
    const auto emergency_profile = [available, seedFloor, validationFloor, required, this]() {
        DeadlineProfile profile{
            DeadlineClass::Emergency,
            available,
            std::chrono::milliseconds{0},
            std::chrono::milliseconds{0},
            std::chrono::milliseconds{0},
            std::chrono::milliseconds{0},
            std::chrono::milliseconds{0},
            std::chrono::milliseconds{0},
        };
        profile.seed = std::min(available, seedFloor);
        const std::chrono::milliseconds afterSeed = available - profile.seed;
        profile.certification = std::min(afterSeed, validationFloor);
        profile.network = available - profile.seed - profile.certification;
        profile.meetsMinimumFloors = available >= required;
        profile.p99Calibrated = calibration_.p99Calibrated;
        profile.competitionReady = competition_ready();
        profile.calibrationVersion = calibration_.version;
        return profile;
    };
    if (available < required || available - required < std::chrono::milliseconds{50}) {
        return emergency_profile();
    }
    const auto make_profile = [
                                  available,
                                  seedFloor,
                                  validationFloor,
                                  networkFloor, &emergency_profile, this](
                                      DeadlineClass deadlineClass,
                                      std::int32_t fastPercent,
                                      std::int32_t searchSoftPercent) {
        DeadlineProfile profile;
        profile.deadlineClass = deadlineClass;
        profile.total = available;
        profile.seed = std::max(
            seedFloor,
            std::chrono::milliseconds{available.count() * calibration_.seedPercent / 100});
        profile.certification = std::max(
            validationFloor,
            std::chrono::milliseconds{available.count() * calibration_.certificationPercent / 100});
        profile.network = std::max(
            networkFloor,
            std::chrono::milliseconds{available.count() * calibration_.networkPercent / 100});
        if (profile.seed + profile.certification + profile.network >= available) {
            return emergency_profile();
        }
        const std::chrono::milliseconds remaining = available - profile.seed - profile.certification - profile.network;
        profile.fastViability = std::min(
            remaining,
            std::chrono::milliseconds{available.count() * fastPercent / 100});
        profile.search = remaining - profile.fastViability;
        profile.searchSoft = std::chrono::milliseconds{
            profile.search.count() * searchSoftPercent / 100};
        profile.meetsMinimumFloors = true;
        profile.p99Calibrated = calibration_.p99Calibrated;
        profile.competitionReady = competition_ready();
        profile.calibrationVersion = calibration_.version;
        return profile;
    };
    if (available <= calibration_.shortThreshold) {
        return make_profile(
            DeadlineClass::Short,
            calibration_.shortFastViabilityPercent,
            calibration_.shortSearchSoftPercent);
    }
    if (available <= calibration_.normalThreshold) {
        return make_profile(
            DeadlineClass::Normal,
            calibration_.normalFastViabilityPercent,
            calibration_.normalSearchSoftPercent);
    }
    return make_profile(
        DeadlineClass::Long,
        calibration_.normalFastViabilityPercent,
        calibration_.longSearchSoftPercent);
}

const DeadlineCalibration& DeadlineScheduler::calibration() const {
    return calibration_;
}

bool DeadlineScheduler::competition_ready() const {
    return calibration_.p99Calibrated;
}

namespace {

struct CacheRepairResult {
    std::vector<MasterCandidate> candidates;
    std::vector<DayPlan> seedPlans;
    std::map<std::string, FutureWitness> certifiedSuffixes;
    CacheRepairDiagnostics diagnostics;
};

[[nodiscard]] CacheRepairResult repair_cached_contingencies(
    const MatchConfig& config,
    const ResponseLedger& responseLedger,
    const DayState& state,
    const MatchLedger& ledger,
    const RouteMaster& master,
    ViabilityBounds& viability,
    std::chrono::steady_clock::time_point deadline) {
    CacheRepairResult result;
    std::set<std::string> uniquePlans;
    for (const ResponseLedger::CachedContingency& contingency : responseLedger.cachedContingencies) {
        if (contingency.dayNumber != state.dayNumber) {
            continue;
        }
        ++result.diagnostics.eligibleContingencies;
        if (std::chrono::steady_clock::now() >= deadline) {
            result.diagnostics.deadlineReached = true;
            break;
        }
        const std::string planId = canonical_plan_bytes(contingency.plan);
        if (!uniquePlans.insert(planId).second) {
            continue;
        }
        std::optional<MasterCandidate> candidate = master.evaluate_exact_plan(
            state,
            ledger,
            contingency.plan,
            viability.reservations);
        if (!candidate.has_value()) {
            ++result.diagnostics.rejectedContingencies;
            continue;
        }
        for (MandatoryReservation& reservation : viability.reservations) {
            if (is_proven_reservation(reservation) || reservation.latestSafeDay > state.dayNumber ||
                reservation.representativeSpot == kInvalidSpot) {
                continue;
            }
            const bool served = std::any_of(
                candidate->simulation.claims.begin(),
                candidate->simulation.claims.end(),
                [&reservation](const ClaimEvent& claim) {
                    return claim.spot == reservation.representativeSpot && claim.served;
                });
            if (served) {
                reservation.evidence = ReservationEvidence::WitnessBacked;
            }
        }
        if (compare_lexicographic(candidate->scoreAfterToday, viability.lowerBound) > 0) {
            viability.lowerBound = candidate->scoreAfterToday;
        }
        for (ConditionalTierBounds& conditional : viability.conditionalTiers) {
            if (candidate->scoreAfterToday.lifetimeDistinct < conditional.coverage) {
                continue;
            }
            if (!conditional.witnessBacked ||
                candidate->scoreAfterToday.totalDailyDistinct > conditional.lowerDailyDistinct) {
                conditional.lowerDailyDistinct = candidate->scoreAfterToday.totalDailyDistinct;
                conditional.lowerServings = candidate->scoreAfterToday.totalServings;
            } else if (candidate->scoreAfterToday.totalDailyDistinct == conditional.lowerDailyDistinct) {
                conditional.lowerServings = std::max(
                    conditional.lowerServings,
                    candidate->scoreAfterToday.totalServings);
            }
            conditional.witnessBacked = true;
        }
        FutureWitness repairedWitness;
        repairedWitness.score = candidate->scoreAfterToday;
        repairedWitness.certified = true;
        if (config.roadCells.empty() &&
            contingency.certifiedSuffix.has_value() &&
            !contingency.certifiedSuffix->futurePlans.empty() &&
            canonical_plan_bytes(
                contingency.certifiedSuffix->futurePlans.front()) ==
                canonical_plan_bytes(contingency.plan)) {
            MatchLedger suffixLedger = ledger;
            DayState suffixState = state;
            bool suffixValid = true;
            for (std::size_t planOffset = 0;
                 planOffset <
                     contingency.certifiedSuffix->futurePlans.size();
                 ++planOffset) {
                std::optional<MasterCandidate> suffixDay =
                    planOffset == 0U
                    ? std::optional<MasterCandidate>{*candidate}
                    : master.evaluate_exact_plan(
                          suffixState,
                          suffixLedger,
                          contingency.certifiedSuffix->futurePlans.at(
                              planOffset));
                if (!suffixDay.has_value()) {
                    suffixValid = false;
                    break;
                }
                suffixLedger.apply(suffixDay->simulation.score);
                if (planOffset + 1U <
                    contingency.certifiedSuffix->futurePlans.size()) {
                    suffixState.dayNumber += 1;
                    suffixState.agents = suffixDay->simulation.finalAgents;
                    suffixState.roadStatuses.assign(
                        static_cast<std::size_t>(
                            config.map.cell_count()),
                        RoadStatus::Smooth);
                }
            }
            const OfficialScore suffixScore = current_score(suffixLedger);
            if (suffixValid &&
                suffixScore == contingency.certifiedSuffix->score) {
                repairedWitness = *contingency.certifiedSuffix;
                repairedWitness.futurePlans.erase(
                    repairedWitness.futurePlans.begin());
                const auto found = result.certifiedSuffixes.find(
                    candidate->stableId);
                if (found == result.certifiedSuffixes.end() ||
                    found->second.score < repairedWitness.score) {
                    result.certifiedSuffixes[candidate->stableId] =
                        repairedWitness;
                }
            }
        }
        viability.frontier.push_back(ViabilityFrontierPoint{
            repairedWitness.score.lifetimeDistinct,
            0,
            repairedWitness.score,
            viability.upperBound,
            std::move(repairedWitness),
            contingency.scenarioId,
            "w0-" + contingency.scenarioClass,
        });
        result.seedPlans.push_back(candidate->plan);
        result.candidates.push_back(std::move(*candidate));
        ++result.diagnostics.reusedContingencies;
    }
    return result;
}

[[nodiscard]] OfficialScore candidate_valid_upper_bound(
    const FastViabilityAnalyzer& analyzer,
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    const MasterCandidate& candidate,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
    if (state.dayNumber >= config.day_count()) {
        return candidate.scoreAfterToday;
    }
    MatchLedger futureLedger = ledger;
    futureLedger.apply(candidate.simulation.score);
    DayState futureState;
    futureState.dayNumber = state.dayNumber + 1;
    futureState.agents = candidate.simulation.finalAgents;
    futureState.roadStatuses = state.roadStatuses;
    return analyzer.analyze(futureState, futureLedger, deadline).upperBound;
}

[[nodiscard]] std::optional<std::size_t> select_upside_challenger(
    const std::vector<CandidateEvaluation>& evaluations,
    std::size_t floorLeader) {
    std::optional<std::size_t> selected;
    std::size_t selectedTier = 3;
    for (std::size_t candidateIndex = 0; candidateIndex < evaluations.size(); ++candidateIndex) {
        if (candidateIndex == floorLeader) {
            continue;
        }
        const CandidateProfile& candidate = evaluations.at(candidateIndex).profile;
        const CandidateProfile& floor = evaluations.at(floorLeader).profile;
        if (!candidate.hasValidUpperBound ||
            compare_lexicographic(candidate.validUpperBound, floor.provisionalLowerBound) <= 0) {
            continue;
        }
        std::size_t tier = 0;
        while (tier < 3U &&
               score_component(candidate.validUpperBound, tier) == score_component(floor.provisionalLowerBound, tier)) {
            ++tier;
        }
        if (tier >= 3U) {
            continue;
        }
        if (!selected.has_value() || tier < selectedTier ||
            (tier == selectedTier &&
             compare_lexicographic(
                 candidate.validUpperBound,
                 evaluations.at(*selected).profile.validUpperBound) > 0) ||
            (tier == selectedTier &&
             candidate.validUpperBound == evaluations.at(*selected).profile.validUpperBound &&
             better_evaluation(evaluations.at(candidateIndex), evaluations.at(*selected)))) {
            selected = candidateIndex;
            selectedTier = tier;
        }
    }
    return selected;
}

void append_w1_role(CandidateAuditRecord& record, const std::string& role) {
    if (record.w1Role.empty()) {
        record.w1Role = role;
        return;
    }
    record.w1Role += ",";
    record.w1Role += role;
}

void record_emergency_audit(DecisionResult& result, const std::string& reason, bool preserveCandidates = false) {
    result.audit.selectionReason = reason;
    if (!preserveCandidates) {
        result.audit.candidates.clear();
    }
    CandidateAuditRecord record;
    record.stableId = result.candidate.stableId;
    record.scoreAfterToday = result.candidate.scoreAfterToday;
    record.provisionalLowerBound = result.profile.provisionalLowerBound;
    record.validUpperBound = result.profile.hasValidUpperBound
        ? result.profile.validUpperBound
        : OfficialScore{};
    record.certified = all_outcomes_certified(result.profile);
    record.selected = true;
    record.w1Role = "emergency";
    record.disposition = "selected";
    result.audit.candidates.push_back(std::move(record));
}

} 

UdonShieldEngine::UdonShieldEngine(
    const MatchConfig& config,
    RiskPolicy policy,
    DeadlineCalibration deadlineCalibration,
    RoutePoolSearch routePoolSearch,
    std::int32_t harvestExtensionMode,
    bool requireUndominatedCurrentFloor,
    std::int32_t futureHarvestExtensionMode)
    : config_(config),
      simulator_(config_),
      validator_(config_),
      router_(config_),
      generator_(config_, router_),
      master_(config_, simulator_, validator_),
      alns_(config_, master_),
      greedy_(config_, generator_, master_),
      belief_(config_),
      scenarioGenerator_(config_),
      viabilityAnalyzer_(config_),
      witnessRepairer_(
          config_,
          generator_,
          master_,
          simulator_,
          validator_,
          futureHarvestExtensionMode < 0
              ? harvestExtensionMode
              : futureHarvestExtensionMode),
      comparator_(std::move(policy)),
      deadlineScheduler_(std::move(deadlineCalibration)),
      routePoolSearch_(routePoolSearch),
      harvestExtensionMode_(harvestExtensionMode),
      requireUndominatedCurrentFloor_(requireUndominatedCurrentFloor) {
    if (harvestExtensionMode_ < 0 || harvestExtensionMode_ > 7) {
        throw std::invalid_argument("harvest extension mode must be in [0,7]");
    }
    if (futureHarvestExtensionMode < -1 || futureHarvestExtensionMode > 7) {
        throw std::invalid_argument("future harvest extension mode must be -1 or in [0,7]");
    }
}

namespace {

template <bool CaptureDailyTrace>
class RoleTraceCollector;

template <>
class RoleTraceCollector<false> {
public:
    [[nodiscard]] std::vector<std::int32_t>* slot(
        const std::vector<AgentKind>&) {
        return nullptr;
    }

    void align(
        const std::vector<RoleAssignment>&,
        std::vector<std::vector<std::int32_t>>*) const {}
};

template <>
class RoleTraceCollector<true> {
public:
    [[nodiscard]] std::vector<std::int32_t>* slot(
        const std::vector<AgentKind>& roles) {
        const auto found = std::find_if(
            traces_.begin(),
            traces_.end(),
            [&roles](const auto& entry) { return entry.first == roles; });
        if (found != traces_.end()) {
            return &found->second;
        }
        traces_.emplace_back(roles, std::vector<std::int32_t>{});
        return &traces_.back().second;
    }

    void align(
        const std::vector<RoleAssignment>& assignments,
        std::vector<std::vector<std::int32_t>>* alignedDailyTraces) const {
        if (alignedDailyTraces == nullptr) {
            throw std::logic_error("role diagnostics require an output sink");
        }
        alignedDailyTraces->clear();
        alignedDailyTraces->reserve(assignments.size());
        for (const RoleAssignment& assignment : assignments) {
            const auto found = std::find_if(
                traces_.begin(),
                traces_.end(),
                [&assignment](const auto& entry) {
                    return entry.first == assignment.roles;
                });
            alignedDailyTraces->push_back(
                found == traces_.end()
                    ? std::vector<std::int32_t>{}
                    : found->second);
        }
    }

private:
    std::vector<std::pair<std::vector<AgentKind>, std::vector<std::int32_t>>> traces_;
};

} // namespace

template <bool CaptureDailyTrace>
void UdonShieldEngine::rollout_role_assignment_impl(
    RoleAssignment& assignment,
    std::int32_t maximumDays,
    std::int32_t maximumCombinationsPerDay,
    std::optional<std::chrono::steady_clock::time_point> deadline,
    std::vector<std::int32_t>* dailyTrace) const {
    assignment.rolloutValid = false;
    assignment.rolloutComplete = false;
    assignment.rolloutScore = OfficialScore{};
    if constexpr (CaptureDailyTrace) {
        if (dailyTrace == nullptr) {
            throw std::logic_error("role trace capture requires a trace slot");
        }
        dailyTrace->clear();
    }
    if (maximumDays <= 0 || maximumCombinationsPerDay <= 0 ||
        assignment.roles.size() != static_cast<std::size_t>(config_.agent_count())) {
        return;
    }
    const auto deadline_expired = [&deadline]() {
        return deadline.has_value() && std::chrono::steady_clock::now() >= *deadline;
    };
    DayState rolloutState;
    rolloutState.dayNumber = 1;
    rolloutState.roadStatuses.assign(
        static_cast<std::size_t>(config_.map.cell_count()),
        RoadStatus::Smooth);
    rolloutState.agents.reserve(static_cast<std::size_t>(config_.agent_count()));
    for (AgentIndex agentIndex = 0; agentIndex < config_.agent_count(); ++agentIndex) {
        rolloutState.agents.push_back(AgentState{
            assignment.roles.at(static_cast<std::size_t>(agentIndex)),
            config_.initialAgents.at(static_cast<std::size_t>(agentIndex)),
            config_.fuelLimit,
        });
    }
    MatchLedger rolloutLedger;
    std::vector<std::vector<std::int32_t>> ownFootprints(
        static_cast<std::size_t>(config_.day_count()),
        std::vector<std::int32_t>(
            static_cast<std::size_t>(config_.map.cell_count()),
            0));
    blank_slate::Planner independentPlanner(
        config_,
        blank_slate::Method::Portfolio);
    assignment.rolloutValid = true;
    bool completedWithoutDeadline = true;
    const std::int32_t finalDay = std::min(config_.day_count(), maximumDays);
    for (std::int32_t dayNumber = 1; dayNumber <= finalDay; ++dayNumber) {
        if (deadline_expired()) {
            assignment.rolloutValid = false;
            completedWithoutDeadline = false;
            break;
        }
        rolloutState.dayNumber = dayNumber;
        std::fill(
            rolloutState.roadStatuses.begin(),
            rolloutState.roadStatuses.end(),
            RoadStatus::Smooth);
        for (const CellId road : config_.roadCells) {
            std::int32_t stays = 0;
            for (std::int32_t offset = 1; offset <= 2; ++offset) {
                const std::int32_t completedDay = dayNumber - offset;
                if (completedDay > 0) {
                    stays += ownFootprints.at(
                        static_cast<std::size_t>(completedDay - 1)).at(
                            static_cast<std::size_t>(road));
                }
            }
            rolloutState.roadStatuses.at(static_cast<std::size_t>(road)) =
                stays >= config_.players * config_.jammedThreshold
                ? RoadStatus::Jammed
                : (stays >= config_.players * config_.busyThreshold
                    ? RoadStatus::Busy
                    : RoadStatus::Smooth);
        }
        std::optional<std::chrono::steady_clock::time_point> daySearchDeadline = deadline;
        std::chrono::milliseconds independentBudget{60};
        if (deadline.has_value()) {
            const std::chrono::steady_clock::time_point now =
                std::chrono::steady_clock::now();
            const std::int32_t remainingDays = finalDay - dayNumber + 1;
            const std::chrono::milliseconds remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(*deadline - now);
            const std::chrono::milliseconds daySlice{
                std::max<std::int64_t>(
                    1,
                    remaining.count() / std::max(1, remainingDays))};
            const std::chrono::milliseconds simulationReserve{
                std::min<std::int64_t>(5, std::max<std::int64_t>(1, daySlice.count() / 10))};
            daySearchDeadline = std::min(
                *deadline,
                now + std::max(
                    std::chrono::milliseconds{1},
                    daySlice - simulationReserve));
            independentBudget = std::min(
                std::chrono::milliseconds{40},
                std::max(
                    std::chrono::milliseconds{1},
                    daySlice / 3));
        }
        blank_slate::Diagnostics independentDiagnostics;
        const DayPlan independentPlan = independentPlanner.solve_day(
            rolloutState,
            rolloutLedger,
            independentBudget,
            independentDiagnostics);
        completedWithoutDeadline =
            completedWithoutDeadline && !independentDiagnostics.deadlineReached;
        std::optional<MasterCandidate> selected =
            master_.evaluate_exact_plan(
                rolloutState,
                rolloutLedger,
                independentPlan);
        const bool seedPass = maximumCombinationsPerDay <= 512;
        ColumnGenerationOptions generationOptions;
        generationOptions.maximumPathsPerTarget = seedPass ? 1 : 2;
        generationOptions.maximumColumnsPerAgent = seedPass ? 4 : 10;
        generationOptions.maximumTargetSpots = seedPass ? 6 : 8;
        generationOptions.maximumEscorts = seedPass ? 4 : 8;
        generationOptions.deadline = daySearchDeadline;
        MasterOptions masterOptions;
        masterOptions.maximumCombinations = maximumCombinationsPerDay;
        masterOptions.maximumCandidates = 1;
        masterOptions.maximumResolveRounds = 1;
        masterOptions.deadline = daySearchDeadline;
        ColumnGenerationDiagnostics generationDiagnostics;
        MasterDiagnostics diagnostics;
        const RoutePortfolio portfolio = generator_.generate(
            rolloutState,
            rolloutLedger,
            generationOptions,
            &generationDiagnostics);
        std::vector<MasterCandidate> candidates = master_.solve(
            rolloutState,
            rolloutLedger,
            portfolio,
            masterOptions,
            diagnostics);
        completedWithoutDeadline =
            completedWithoutDeadline &&
            !generationDiagnostics.deadlineReached &&
            !diagnostics.deadlineReached;
        if (!candidates.empty() &&
            (!selected.has_value() ||
             better_search_candidate(candidates.front(), *selected))) {
            selected = candidates.front();
        }
        if (!selected.has_value()) {
            assignment.rolloutValid = false;
            break;
        }
        const SimulationResult detailed = simulator_.simulate(
            rolloutState,
            selected->plan,
            false);
        const SimulationResult validation = validator_.validate(
            rolloutState,
            selected->plan,
            false);
        std::string mismatch;
        if (!detailed.valid || !validator_.agrees_with(detailed, validation, mismatch)) {
            throw std::runtime_error(
                "role rollout disagreed with the independent validator: " + mismatch);
        }
        rolloutLedger.apply(detailed.score);
        if constexpr (CaptureDailyTrace) {
            dailyTrace->push_back(detailed.score.dailyDistinct);
        }
        rolloutState.agents = detailed.finalAgents;
        ownFootprints.at(static_cast<std::size_t>(dayNumber - 1)) =
            detailed.roadFootprint;
    }
    assignment.rolloutScore = current_score(rolloutLedger);
    assignment.rolloutComplete =
        assignment.rolloutValid && completedWithoutDeadline;
}

void UdonShieldEngine::rollout_role_assignment(
    RoleAssignment& assignment,
    std::int32_t maximumDays,
    std::int32_t maximumCombinationsPerDay,
    std::optional<std::chrono::steady_clock::time_point> deadline) const {
    rollout_role_assignment_impl<false>(
        assignment,
        maximumDays,
        maximumCombinationsPerDay,
        deadline,
        nullptr);
}

bool role_assignment_better_after_rollout(
    const RoleAssignment& challenger,
    const RoleAssignment& incumbent) {
    if (challenger.rolloutValid != incumbent.rolloutValid) {
        return challenger.rolloutValid;
    }
    if (challenger.rolloutValid) {
        const std::int32_t rolloutOrder = compare_lexicographic(
            challenger.rolloutScore,
            incumbent.rolloutScore);
        if (rolloutOrder != 0) {
            return rolloutOrder > 0;
        }
    }
    if (challenger.sustainableCoverage != incumbent.sustainableCoverage) {
        return challenger.sustainableCoverage > incumbent.sustainableCoverage;
    }
    const std::int32_t boundOrder = compare_lexicographic(
        challenger.cheapUpperBound,
        incumbent.cheapUpperBound);
    if (boundOrder != 0) {
        return boundOrder > 0;
    }
    if (challenger.patrolCount != incumbent.patrolCount) {
        return challenger.patrolCount > incumbent.patrolCount;
    }
    return false;
}

std::int32_t role_comparison_beam_width(
    const MatchConfig& config,
    std::int32_t requestedWidth) {
    if (requestedWidth <= 0 || config.day_count() <= 5 ||
        config.agent_count() - 2 < config.brand_count()) {
        return requestedWidth;
    }
    const std::int32_t maximumDaySteps = *std::max_element(
        config.daySteps.begin(),
        config.daySteps.end());
    if (config.fuelLimit > maximumDaySteps) {
        return requestedWidth;
    }
    const std::int32_t fullMaskCount =
        std::int32_t{1} << config.agent_count();
    return std::max(
        requestedWidth,
        std::min(fullMaskCount, 2 * config.agent_count()));
}

bool apply_incomplete_long_horizon_role_fallback(
    const MatchConfig& config,
    bool fullHorizonComparisonComplete,
    std::vector<RoleAssignment>& beam,
    bool includeShortHorizon) {
    const bool shortHorizon = config.day_count() <= 5;
    if (fullHorizonComparisonComplete || beam.empty() ||
        (shortHorizon && !includeShortHorizon)) {
        return false;
    }
    if (shortHorizon &&
        beam.front().patrolCount >= config.agent_count() - 1) {
        const std::int32_t maximumDaySteps = *std::max_element(
            config.daySteps.begin(),
            config.daySteps.end());
        const bool immobileAllPatrolFront =
            beam.front().patrolCount == config.agent_count() &&
            config.fuelLimit <= maximumDaySteps;
        if (!immobileAllPatrolFront) {
            // Short horizons displace tanker-heavy leaders and, below the
            // low-fuel boundary, all-patrol fronts whose match fuel cannot
            // cover one day of movement; other parents are kept because
            // incomplete rollouts cannot rank them reliably.
            return false;
        }
    }
    auto bestSingleTanker = beam.end();
    auto bestDoubleTanker = beam.end();
    for (auto candidate = beam.begin(); candidate != beam.end(); ++candidate) {
        if (candidate->patrolCount == config.agent_count() - 1) {
            if (bestSingleTanker == beam.end() ||
                role_assignment_better_after_rollout(
                    *candidate,
                    *bestSingleTanker)) {
                bestSingleTanker = candidate;
            }
        } else if (candidate->patrolCount == config.agent_count() - 2) {
            if (bestDoubleTanker == beam.end() ||
                role_assignment_better_after_rollout(
                    *candidate,
                    *bestDoubleTanker)) {
                bestDoubleTanker = candidate;
            }
        }
    }
    if (bestSingleTanker == beam.end()) {
        return false;
    }
    auto fallback = bestSingleTanker;
    if (role_comparison_beam_width(config, 1) > 1 &&
        bestDoubleTanker != beam.end() &&
        bestSingleTanker->rolloutValid &&
        bestDoubleTanker->rolloutValid &&
        bestDoubleTanker->rolloutScore.lifetimeDistinct ==
            bestSingleTanker->rolloutScore.lifetimeDistinct &&
        bestDoubleTanker->rolloutScore.totalDailyDistinct ==
            bestSingleTanker->rolloutScore.totalDailyDistinct) {
        fallback = bestDoubleTanker;
    }
    if (fallback == beam.begin()) {
        return false;
    }
    std::rotate(
        beam.begin(),
        fallback,
        std::next(fallback));
    return true;
}

template <bool CaptureDailyTrace>
std::vector<RoleAssignment>
UdonShieldEngine::select_roles_exhaustive_oracle_impl(
    std::int32_t beamWidth,
    std::vector<std::vector<std::int32_t>>* alignedDailyTraces) const {
    RoleTraceCollector<CaptureDailyTrace> traces;
    if (beamWidth <= 0) {
        return {};
    }
    const std::int32_t fullMaskCount =
        std::int32_t{1} << config_.agent_count();
    std::vector<RoleAssignment> scanned =
        RoleAssignmentEnumerator(config_).shortlist(fullMaskCount);
    if (scanned.empty()) {
        return {};
    }
    RoleAssignment bestSeed;
    bool hasSeed = false;
    const auto allPatrol = std::find_if(
        scanned.begin(),
        scanned.end(),
        [](const RoleAssignment& assignment) {
            return std::all_of(
                assignment.roles.begin(),
                assignment.roles.end(),
                [](AgentKind kind) { return kind == AgentKind::Patrol; });
        });
    if (allPatrol != scanned.end()) {
        bestSeed = *allPatrol;
        bestSeed.rolloutValid = true;
        bestSeed.rolloutScore = OfficialScore{};
        hasSeed = true;
    }
    const std::vector<AgentKind> centralRoles =
        blank_slate::Planner(config_, blank_slate::Method::Portfolio).select_roles();
    const auto central = std::find_if(
        scanned.begin(),
        scanned.end(),
        [&centralRoles](const RoleAssignment& assignment) {
            return assignment.roles == centralRoles;
        });
    std::optional<RoleAssignment> centralSeed;
    if (central != scanned.end()) {
        RoleAssignment seed = *central;
        rollout_role_assignment_impl<CaptureDailyTrace>(
            seed,
            1,
            512,
            std::nullopt,
            traces.slot(seed.roles));
        centralSeed = seed;
        if (seed.rolloutValid &&
            (!hasSeed || compare_lexicographic(seed.rolloutScore, bestSeed.rolloutScore) > 0)) {
            bestSeed = seed;
            hasSeed = true;
        }
    }
    for (RoleAssignment& assignment : scanned) {
        if (assignment.roles == centralRoles) {
            continue;
        }
        const std::int32_t tankerCount = config_.agent_count() - assignment.patrolCount;
        if (tankerCount > 1) {
            continue;
        }
        RoleAssignment seed = assignment;
        rollout_role_assignment_impl<CaptureDailyTrace>(
            seed,
            1,
            512,
            std::nullopt,
            traces.slot(seed.roles));
        if (seed.rolloutValid &&
            (!hasSeed || compare_lexicographic(seed.rolloutScore, bestSeed.rolloutScore) > 0)) {
            bestSeed = std::move(seed);
            hasSeed = true;
        }
    }
    std::vector<RoleAssignment> beam;
    beam.reserve(static_cast<std::size_t>(beamWidth + 1));
    for (const RoleAssignment& assignment : scanned) {
        if (hasSeed &&
            compare_lexicographic(
                assignment.cheapUpperBound,
                bestSeed.rolloutScore) <= 0) {
            continue;
        }
        beam.push_back(assignment);
        if (static_cast<std::int32_t>(beam.size()) >= beamWidth) {
            break;
        }
    }
    if (centralSeed.has_value()) {
        beam.erase(
            std::remove_if(
                beam.begin(),
                beam.end(),
                [&centralRoles](const RoleAssignment& assignment) {
                    return assignment.roles == centralRoles;
                }),
            beam.end());
        beam.insert(beam.begin(), *centralSeed);
        if (static_cast<std::int32_t>(beam.size()) > beamWidth) {
            beam.pop_back();
        }
    }
    if (hasSeed && std::none_of(
            beam.begin(),
            beam.end(),
            [&bestSeed](const RoleAssignment& assignment) {
                return assignment.roles == bestSeed.roles;
            })) {
        if (static_cast<std::int32_t>(beam.size()) >= beamWidth) {
            beam.back() = bestSeed;
        } else {
            beam.push_back(bestSeed);
        }
    }
    for (RoleAssignment& assignment : beam) {
        rollout_role_assignment_impl<CaptureDailyTrace>(
            assignment,
            config_.day_count(),
            8000,
            std::nullopt,
            traces.slot(assignment.roles));
    }
    std::sort(
        beam.begin(),
        beam.end(),
        [](const RoleAssignment& left, const RoleAssignment& right) {
            if (role_assignment_better_after_rollout(left, right)) {
                return true;
            }
            if (role_assignment_better_after_rollout(right, left)) {
                return false;
            }
            return left.roles < right.roles;
        });
    const auto centralInBeam = std::find_if(
        beam.begin(),
        beam.end(),
        [&centralRoles](const RoleAssignment& assignment) {
            return assignment.roles == centralRoles;
        });
    if (centralInBeam != beam.end()) {
        auto preferred = centralInBeam;
        for (auto candidate = beam.begin(); candidate != beam.end(); ++candidate) {
            if (role_assignment_better_after_rollout(*candidate, *preferred)) {
                preferred = candidate;
            }
        }
        std::rotate(beam.begin(), preferred, std::next(preferred));
    }
    if (static_cast<std::int32_t>(beam.size()) > beamWidth) {
        beam.resize(static_cast<std::size_t>(beamWidth));
    }
    traces.align(beam, alignedDailyTraces);
    return beam;
}

std::vector<RoleAssignment> UdonShieldEngine::select_roles_exhaustive_oracle(
    std::int32_t beamWidth) const {
    return select_roles_exhaustive_oracle_impl<false>(
        beamWidth,
        nullptr);
}

RoleSelectionDiagnostics
UdonShieldEngine::select_roles_exhaustive_oracle_with_diagnostics(
    std::int32_t beamWidth) const {
    RoleSelectionDiagnostics diagnostics;
    diagnostics.assignments = select_roles_exhaustive_oracle_impl<true>(
        beamWidth,
        &diagnostics.rolloutDailyDistinct);
    return diagnostics;
}

void UdonShieldEngine::set_short_horizon_role_fallback(bool enabled) {
    shortHorizonRoleFallback_ = enabled;
}

template <bool CaptureDailyTrace>
std::vector<RoleAssignment> UdonShieldEngine::select_roles_until_impl(
    std::chrono::milliseconds available,
    std::int32_t beamWidth,
    std::vector<std::vector<std::int32_t>>* alignedDailyTraces) const {
    RoleTraceCollector<CaptureDailyTrace> traces;
    available = competition_compute_budget(available);
    if (available.count() <= 0 || beamWidth <= 0) {
        return {};
    }
    const std::chrono::steady_clock::time_point started =
        std::chrono::steady_clock::now();
    const std::chrono::steady_clock::time_point scanDeadline =
        started + std::chrono::milliseconds{available.count() * 15 / 100};
    const std::chrono::steady_clock::time_point probeDeadline =
        started + std::chrono::milliseconds{available.count() * 35 / 100};
    const std::chrono::steady_clock::time_point rolloutDeadline =
        started + std::chrono::milliseconds{available.count() * 85 / 100};
    const std::int32_t comparisonBeamWidth =
        role_comparison_beam_width(config_, beamWidth);
    const std::int32_t fullMaskCount =
        std::int32_t{1} << config_.agent_count();
    std::vector<RoleAssignment> scanned =
        RoleAssignmentEnumerator(config_).shortlist(fullMaskCount, scanDeadline);
    if (scanned.empty()) {
        return {};
    }
    const auto allPatrol = std::find_if(
        scanned.begin(),
        scanned.end(),
        [](const RoleAssignment& assignment) {
            return std::all_of(
                assignment.roles.begin(),
                assignment.roles.end(),
                [](AgentKind kind) { return kind == AgentKind::Patrol; });
        });
    const std::vector<AgentKind> centralRoles =
        blank_slate::Planner(config_, blank_slate::Method::Portfolio).select_roles();
    const auto central = std::find_if(
        scanned.begin(),
        scanned.end(),
        [&centralRoles](const RoleAssignment& assignment) {
            return assignment.roles == centralRoles;
        });
    std::vector<RoleAssignment> probed;
    probed.reserve(static_cast<std::size_t>(config_.agent_count() + 1));
    std::int32_t remainingProbeAssignments = static_cast<std::int32_t>(
        std::count_if(
            scanned.begin(),
            scanned.end(),
            [this](const RoleAssignment& assignment) {
                return config_.agent_count() - assignment.patrolCount <= 1;
            }));
    for (const RoleAssignment& assignment : scanned) {
        if (config_.agent_count() - assignment.patrolCount > 1) {
            continue;
        }
        const std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();
        if (now >= probeDeadline) {
            break;
        }
        const std::chrono::milliseconds remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                probeDeadline - now);
        const std::chrono::steady_clock::time_point assignmentDeadline =
            now + std::chrono::milliseconds{
                std::max<std::int64_t>(
                    1,
                    remaining.count() /
                        std::max(1, remainingProbeAssignments))};
        RoleAssignment refined = assignment;
        rollout_role_assignment_impl<CaptureDailyTrace>(
            refined,
            1,
            256,
            assignmentDeadline,
            traces.slot(refined.roles));
        if (refined.rolloutValid) {
            probed.push_back(std::move(refined));
        }
        --remainingProbeAssignments;
    }
    std::sort(
        probed.begin(),
        probed.end(),
        [](const RoleAssignment& left, const RoleAssignment& right) {
            if (role_assignment_better_after_rollout(left, right)) {
                return true;
            }
            if (role_assignment_better_after_rollout(right, left)) {
                return false;
            }
            return left.roles < right.roles;
        });
    std::vector<RoleAssignment> beam;
    beam.reserve(static_cast<std::size_t>(comparisonBeamWidth));
    const auto append_to_beam =
        [&beam, comparisonBeamWidth](const RoleAssignment& required) {
        if (static_cast<std::int32_t>(beam.size()) >= comparisonBeamWidth) {
            return;
        }
        if (std::any_of(
                beam.begin(),
                beam.end(),
                [&required](const RoleAssignment& assignment) {
                    return assignment.roles == required.roles;
                })) {
            return;
        }
        beam.push_back(required);
    };
    if (!probed.empty()) {
        append_to_beam(probed.front());
    }
    if (central != scanned.end()) {
        const auto refinedCentral = std::find_if(
            probed.begin(),
            probed.end(),
            [&centralRoles](const RoleAssignment& assignment) {
                return assignment.roles == centralRoles;
            });
        append_to_beam(refinedCentral == probed.end() ? *central : *refinedCentral);
    }
    if (allPatrol != scanned.end()) {
        const auto refinedAllPatrol = std::find_if(
            probed.begin(),
            probed.end(),
            [](const RoleAssignment& assignment) {
                return std::all_of(
                    assignment.roles.begin(),
                    assignment.roles.end(),
                    [](AgentKind kind) { return kind == AgentKind::Patrol; });
            });
        append_to_beam(
            refinedAllPatrol == probed.end()
                ? *allPatrol
                : *refinedAllPatrol);
    }
    for (const RoleAssignment& assignment : probed) {
        append_to_beam(assignment);
    }
    for (const RoleAssignment& assignment : scanned) {
        append_to_beam(assignment);
    }
    std::vector<bool> fullHorizonEvidence(beam.size(), false);
    for (std::size_t index = 0; index < beam.size(); ++index) {
        const std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();
        if (now >= rolloutDeadline) {
            break;
        }
        const std::int32_t remainingAssignments =
            static_cast<std::int32_t>(beam.size() - index);
        const std::chrono::milliseconds remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                rolloutDeadline - now);
        const std::chrono::steady_clock::time_point assignmentDeadline =
            now + std::chrono::milliseconds{
                std::max<std::int64_t>(
                    1,
                    remaining.count() / std::max(1, remainingAssignments))};
        RoleAssignment refined = beam.at(index);
        rollout_role_assignment_impl<CaptureDailyTrace>(
            refined,
            config_.day_count(),
            256,
            assignmentDeadline,
            traces.slot(refined.roles));
        if (refined.rolloutValid) {
            beam.at(index) = std::move(refined);
            fullHorizonEvidence.at(index) = beam.at(index).rolloutComplete;
        }
    }
    const bool fullHorizonComparisonComplete = std::all_of(
        fullHorizonEvidence.begin(),
        fullHorizonEvidence.end(),
        [](bool complete) { return complete; });
    std::sort(
        beam.begin(),
        beam.end(),
        [](const RoleAssignment& left, const RoleAssignment& right) {
            if (role_assignment_better_after_rollout(left, right)) {
                return true;
            }
            if (role_assignment_better_after_rollout(right, left)) {
                return false;
            }
            return left.roles < right.roles;
        });
    const auto centralTimedInBeam = std::find_if(
        beam.begin(),
        beam.end(),
        [&centralRoles](const RoleAssignment& assignment) {
            return assignment.roles == centralRoles;
        });
    if (centralTimedInBeam != beam.end()) {
        auto preferred = centralTimedInBeam;
        for (auto candidate = beam.begin(); candidate != beam.end(); ++candidate) {
            if (role_assignment_better_after_rollout(*candidate, *preferred)) {
                preferred = candidate;
            }
        }
        std::rotate(beam.begin(), preferred, std::next(preferred));
    }
    if (static_cast<std::int32_t>(beam.size()) > comparisonBeamWidth) {
        beam.resize(static_cast<std::size_t>(comparisonBeamWidth));
    }
    static_cast<void>(apply_incomplete_long_horizon_role_fallback(
        config_,
        fullHorizonComparisonComplete,
        beam,
        shortHorizonRoleFallback_));
    if (static_cast<std::int32_t>(beam.size()) > beamWidth) {
        beam.resize(static_cast<std::size_t>(beamWidth));
    }
    const std::chrono::steady_clock::time_point prewarmDeadline =
        started + std::chrono::milliseconds{available.count() * 92 / 100};
    if (!beam.empty() && std::chrono::steady_clock::now() < prewarmDeadline) {
        DayState prewarmState;
        prewarmState.dayNumber = 1;
        prewarmState.roadStatuses.assign(
            static_cast<std::size_t>(config_.map.cell_count()),
            RoadStatus::Smooth);
        prewarmState.agents.reserve(static_cast<std::size_t>(config_.agent_count()));
        for (AgentIndex agentIndex = 0;
             agentIndex < config_.agent_count();
             ++agentIndex) {
            prewarmState.agents.push_back(AgentState{
                beam.front().roles.at(static_cast<std::size_t>(agentIndex)),
                config_.initialAgents.at(static_cast<std::size_t>(agentIndex)),
                config_.fuelLimit,
            });
        }
        ColumnGenerationOptions prewarmOptions;
        prewarmOptions.maximumPathsPerTarget = 4;
        prewarmOptions.maximumColumnsPerAgent = 12;
        prewarmOptions.maximumTargetSpots = 12;
        prewarmOptions.maximumEscorts = 16;
        prewarmOptions.maximumSeedPlans = 2;
        prewarmOptions.enableHarvestExtensions = harvestExtensionMode_ > 0;
        prewarmOptions.allowUncachedHarvestTargets = harvestExtensionMode_ > 1;
        prewarmOptions.deadline = prewarmDeadline;
        static_cast<void>(generator_.generate(
            prewarmState,
            MatchLedger{},
            prewarmOptions));
    }
    traces.align(beam, alignedDailyTraces);
    return beam;
}

std::vector<RoleAssignment> UdonShieldEngine::select_roles_until(
    std::chrono::milliseconds available,
    std::int32_t beamWidth) const {
    return select_roles_until_impl<false>(available, beamWidth, nullptr);
}

RoleSelectionDiagnostics UdonShieldEngine::select_roles_until_with_diagnostics(
    std::chrono::milliseconds available,
    std::int32_t beamWidth) const {
    RoleSelectionDiagnostics diagnostics;
    diagnostics.assignments = select_roles_until_impl<true>(
        available,
        beamWidth,
        &diagnostics.rolloutDailyDistinct);
    return diagnostics;
}

DecisionResult UdonShieldEngine::solve_day(
    const DayState& state,
    const MatchLedger& ledger,
    std::chrono::milliseconds available) {
    available = competition_compute_budget(available);
    const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    const auto elapsed = [started]() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started);
    };
    DecisionResult result;
    result.riskPolicy = comparator_.policy();
    result.dayNumber = state.dayNumber;
    if (lastSubmittedDay_.has_value()) {
        if (state.dayNumber < *lastSubmittedDay_) {
            throw std::invalid_argument("cannot solve a day older than the latest submitted decision");
        }
        if (state.dayNumber > *lastSubmittedDay_ + 1) {
            result.reconciledAuthoritativeState = true;
            expectedNextAgents_.reset();
        } else if (state.dayNumber == *lastSubmittedDay_ + 1 && expectedNextAgents_.has_value() &&
                   !same_agent_states(*expectedNextAgents_, state.agents)) {
            result.reconciledAuthoritativeState = true;
            expectedNextAgents_.reset();
        }
    }
    belief_.observe(state);
    result.deadline = deadlineScheduler_.classify(available);
    if (result.deadline.deadlineClass == DeadlineClass::Emergency || !result.deadline.meetsMinimumFloors) {
        result.manifest = emergency_manifest(config_);
        bind_manifest_policy(result.manifest, result.riskPolicy);
        result.viability.lowerBound = current_score(ledger);
        result.viability.upperBound = current_score(ledger);
        result.candidate = make_valid_emergency_candidate(config_, simulator_, validator_, state, ledger);
        result.candidate.trafficSafety = counterfactual_traffic_safety(
            config_, belief_, result.manifest, result.candidate.simulation);
        result.profile = witnessRepairer_.provisional_profile(
            result.candidate,
            state,
            ledger,
            belief_,
            result.manifest,
            result.candidate.scoreAfterToday,
            1);
        std::vector<CandidateEvaluation> evaluations;
        evaluations.push_back(CandidateEvaluation{result.candidate, result.profile});
        comparator_.finalize_profiles(
            evaluations,
            result.manifest,
            &result.audit.profileFinalization);
        result.profile = evaluations.front().profile;
        result.emergency = true;
        record_emergency_audit(result, "deadline-emergency");
        finalize_optimality_gap(config_, state, ledger, result);
        result.timing.total = elapsed();
        return result;
    }
    result.manifest = scenarioGenerator_.freeze_manifest(state, belief_);
    bind_manifest_policy(result.manifest, result.riskPolicy);

    MasterCandidate incumbent = make_valid_emergency_candidate(config_, simulator_, validator_, state, ledger);
    incumbent.trafficSafety = counterfactual_traffic_safety(config_, belief_, result.manifest, incumbent.simulation);
    const std::chrono::steady_clock::time_point seedDeadline = started + result.deadline.seed;
    if (std::chrono::steady_clock::now() < seedDeadline) {
        ColumnGenerationOptions incumbentGeneration;
        incumbentGeneration.maximumPathsPerTarget = 1;
        incumbentGeneration.maximumColumnsPerAgent = 2;
        incumbentGeneration.maximumTargetSpots = 4;
        incumbentGeneration.maximumEscorts = 2;
        incumbentGeneration.deadline = seedDeadline;
        MasterOptions incumbentMaster;
        incumbentMaster.maximumCombinations = result.deadline.deadlineClass == DeadlineClass::Short ? 128 : 512;
        incumbentMaster.maximumCandidates = 1;
        incumbentMaster.deadline = seedDeadline;
        MasterDiagnostics incumbentDiagnostics;
        try {
            incumbent = greedy_.build_incumbent(
                state,
                ledger,
                incumbentGeneration,
                incumbentMaster,
                incumbentDiagnostics);
            incumbent.trafficSafety = counterfactual_traffic_safety(config_, belief_, result.manifest, incumbent.simulation);
        } catch (const std::runtime_error& error) {
            if (std::string(error.what()) != "the exact wait column portfolio did not yield a valid incumbent" &&
                std::chrono::steady_clock::now() < seedDeadline) {
                throw;
            }
        }
    }
    std::string incumbentId = incumbent.stableId;

    const std::chrono::steady_clock::time_point fastViabilityDeadline =
        started + result.deadline.seed + result.deadline.fastViability;
    result.viability = viabilityAnalyzer_.analyze(state, ledger, fastViabilityDeadline);
    result.timing.incumbent = elapsed();
    CacheRepairResult cacheRepair = repair_cached_contingencies(
        config_,
        ledger_,
        state,
        ledger,
        master_,
        result.viability,
        fastViabilityDeadline);
    result.cacheRepair = cacheRepair.diagnostics;
    result.timing.fastPath = elapsed() - result.timing.incumbent;

    ColumnGenerationOptions generationOptions;
    generationOptions.maximumPathsPerTarget = result.deadline.deadlineClass == DeadlineClass::Short ? 2 : 4;
    generationOptions.maximumColumnsPerAgent = result.deadline.deadlineClass == DeadlineClass::Short ? 6 :
        16;
    generationOptions.maximumTargetSpots = result.deadline.deadlineClass == DeadlineClass::Short ? 8 : 12;
    generationOptions.maximumEscorts = result.deadline.deadlineClass == DeadlineClass::Short ? 4 : 16;
    generationOptions.maximumSeedPlans = result.deadline.deadlineClass == DeadlineClass::Short ? 1 : 2;
    generationOptions.enableHarvestExtensions = harvestExtensionMode_ > 0;
    generationOptions.allowUncachedHarvestTargets = harvestExtensionMode_ > 1;
    const bool highFuelOrienteering =
        static_cast<std::int64_t>(config_.fuelLimit) >=
        3LL * config_.steps_for_day(state.dayNumber);
    const bool exactFuelOrienteering =
        static_cast<std::int64_t>(config_.fuelLimit) >=
        2LL * config_.steps_for_day(state.dayNumber);
    const bool hasFuelConstrainedPatrol = std::any_of(
        state.agents.begin(),
        state.agents.end(),
        [this, &state](const AgentState& agent) {
            return agent.kind == AgentKind::Patrol &&
                static_cast<std::int64_t>(agent.fuel) <
                    2LL * config_.steps_for_day(state.dayNumber);
        });
    generationOptions.enableHarvestOrienteering =
        harvestExtensionMode_ > 5 &&
        result.deadline.search >= std::chrono::milliseconds{1500} &&
        highFuelOrienteering;
    generationOptions.enableExactHarvestOrienteering =
        harvestExtensionMode_ > 5 &&
        (exactFuelOrienteering ||
         (harvestExtensionMode_ > 6 &&
          state.dayNumber == config_.day_count())) &&
        result.deadline.search >= std::chrono::milliseconds{400} &&
        (harvestExtensionMode_ > 6 || state.dayNumber == config_.day_count());
    generationOptions.enableFuelConstrainedExactHarvestOrienteering =
        hasFuelConstrainedPatrol &&
        harvestExtensionMode_ > 6 &&
        state.dayNumber == config_.day_count();
    generationOptions.enableAnytimeFuelConstrainedHarvestOrienteering =
        generationOptions.enableFuelConstrainedExactHarvestOrienteering;
    generationOptions.maximumHarvestExtensionSources =
        harvestExtensionMode_ > 2 ? 4 : 1;
    generationOptions.maximumHarvestExtensionDepth =
        harvest_extension_depth(
            config_,
            state.dayNumber,
            harvestExtensionMode_);
    generationOptions.mandatoryReservations = result.viability.reservations;
    generationOptions.seedPlans.push_back(incumbent.plan);
    generationOptions.seedPlans.insert(
        generationOptions.seedPlans.end(),
        cacheRepair.seedPlans.begin(),
        cacheRepair.seedPlans.end());
    const DeadlineCalibration& deadlineCalibration = deadlineScheduler_.calibration();
    const std::chrono::steady_clock::time_point searchPhaseStart =
        started + result.deadline.seed + result.deadline.fastViability;
    const std::chrono::milliseconds f0Guard = std::min(
        result.deadline.search,
        std::max(
            deadlineCalibration.validationFloor,
            std::min(
                deadlineCalibration.f0BoundaryGuard,
                result.deadline.search / 10)));
    const std::chrono::milliseconds f0Window = std::max(
        std::chrono::milliseconds{0},
        result.deadline.search - f0Guard);
    const std::chrono::steady_clock::time_point f0Deadline = searchPhaseStart + f0Window;
    const std::chrono::milliseconds f0Reserve = std::chrono::milliseconds{
        f0Window.count() * deadlineCalibration.f0SearchReservePercent / 100};
    const std::chrono::steady_clock::time_point searchSoftDeadline =
        searchPhaseStart + result.deadline.searchSoft;
    const std::chrono::steady_clock::time_point masterSearchDeadline = std::min(
        f0Deadline - f0Reserve,
        searchSoftDeadline);
    const std::chrono::milliseconds masterSearchWindow =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            masterSearchDeadline - searchPhaseStart);
    const bool feedbackRoutePool = routePoolSearch_ == RoutePoolSearch::Feedback;
    const std::chrono::milliseconds routePoolReserve =
        result.deadline.deadlineClass == DeadlineClass::Short
        ? std::chrono::milliseconds{0}
        : std::chrono::milliseconds{std::max<std::int64_t>(
            5,
            masterSearchWindow.count() /
                (feedbackRoutePool ? 4 : 8))};
    const std::chrono::steady_clock::time_point preRecombinationDeadline =
        masterSearchDeadline - routePoolReserve;
    const std::chrono::steady_clock::time_point firstRecombinationDeadline =
        feedbackRoutePool
        ? preRecombinationDeadline + routePoolReserve / 3
        : masterSearchDeadline;
    const std::chrono::steady_clock::time_point feedbackAlnsDeadline =
        feedbackRoutePool
        ? preRecombinationDeadline + routePoolReserve * 2 / 3
        : masterSearchDeadline;
    const std::chrono::steady_clock::time_point initialAlnsDeadline =
        feedbackRoutePool
        ? preRecombinationDeadline
        : preRecombinationDeadline + routePoolReserve / 2;
    const std::int32_t f0OperationCap = result.deadline.deadlineClass == DeadlineClass::Short
        ? deadlineCalibration.shortF0OperationCap
        : (result.deadline.deadlineClass == DeadlineClass::Long
            ? deadlineCalibration.longF0OperationCap
            : deadlineCalibration.normalF0OperationCap);
    const std::int32_t f0CandidateLimit = result.deadline.deadlineClass == DeadlineClass::Short
        ? deadlineCalibration.shortF0CandidateLimit
        : (result.deadline.deadlineClass == DeadlineClass::Long
            ? deadlineCalibration.longF0CandidateLimit
            : deadlineCalibration.normalF0CandidateLimit);
    const std::int32_t searchCandidateLimit = result.deadline.deadlineClass == DeadlineClass::Short
        ? std::max(f0CandidateLimit, 16)
        : (result.deadline.deadlineClass == DeadlineClass::Long
            ? std::max(f0CandidateLimit, 32)
            : std::max(f0CandidateLimit, 32));
    MasterOptions masterOptions;
    masterOptions.maximumCombinations = result.deadline.deadlineClass == DeadlineClass::Short ? 5000 : 40000;
    masterOptions.maximumCandidates = searchCandidateLimit;
    masterOptions.diversityCandidates = result.deadline.deadlineClass == DeadlineClass::Short
        ? 2
        : std::max(4, searchCandidateLimit / 4);
    masterOptions.preferBaselineHarvestSources = harvestExtensionMode_ > 2;
    masterOptions.mandatoryReservations = result.viability.reservations;
    masterOptions.deadline = preRecombinationDeadline;
    generationOptions.deadline = masterOptions.deadline;
    std::optional<MasterCandidate> independentCandidate;
    if (result.deadline.deadlineClass != DeadlineClass::Short &&
        masterSearchWindow >= std::chrono::milliseconds{40} &&
        std::chrono::steady_clock::now() < preRecombinationDeadline) {
        const std::chrono::milliseconds independentBudgetCap =
            deadlineCalibration.networkFloor >= std::chrono::milliseconds{1000}
            ? std::chrono::milliseconds{180}
            : std::chrono::milliseconds{450};
        const std::chrono::milliseconds independentBudget = std::min(
            independentBudgetCap,
            std::max(
                std::chrono::milliseconds{40},
                std::chrono::milliseconds{
                    masterSearchWindow.count() / 5}));
        const std::chrono::milliseconds beforeIndependent = elapsed();
        blank_slate::Diagnostics independentDiagnostics;
        blank_slate::Planner independentPlanner(
            config_,
            blank_slate::Method::Portfolio);
        const DayPlan independentPlan = independentPlanner.solve_day(
            state,
            ledger,
            std::min(
                independentBudget,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    preRecombinationDeadline -
                    std::chrono::steady_clock::now())),
            independentDiagnostics);
        result.timing.independentGenerators =
            elapsed() - beforeIndependent;
        result.audit.independentRoutesGenerated =
            independentDiagnostics.routesGenerated;
        result.audit.independentPlansEvaluated =
            independentDiagnostics.plansEvaluated;
        result.audit.independentSearchNodes =
            independentDiagnostics.searchNodes;
        result.audit.independentRollouts =
            independentDiagnostics.rollouts;
        result.audit.independentDeadlineReached =
            independentDiagnostics.deadlineReached;
        independentCandidate = master_.evaluate_exact_plan(
            state,
            ledger,
            independentPlan,
            result.viability.reservations);
        result.audit.independentCandidateAccepted =
            independentCandidate.has_value();
    }
    const std::chrono::milliseconds beforeColumnGeneration = elapsed();
    const std::chrono::steady_clock::time_point portfolioPhaseStarted =
        std::chrono::steady_clock::now();
    const std::chrono::milliseconds portfolioPhaseWindow = std::max(
        std::chrono::milliseconds{0},
        std::chrono::duration_cast<std::chrono::milliseconds>(
            preRecombinationDeadline - portfolioPhaseStarted));
    const bool stagedDeepHarvestSearch =
        generationOptions.maximumHarvestExtensionDepth >= 4;
    ColumnGenerationOptions legacyGenerationOptions = generationOptions;
    legacyGenerationOptions.allowUncachedHarvestTargets = false;
    if (result.deadline.deadlineClass == DeadlineClass::Normal) {
        legacyGenerationOptions.maximumColumnsPerAgent = 12;
    }
    if (stagedDeepHarvestSearch) {
        legacyGenerationOptions.deadline =
            portfolioPhaseStarted + portfolioPhaseWindow * 50 / 100;
    }
    RoutePortfolio portfolio = generator_.generate(
        state,
        ledger,
        legacyGenerationOptions,
        &result.audit.columnGeneration);
    const RoutePortfolio legacyPortfolio = portfolio;
    const std::chrono::milliseconds afterLegacyGeneration = elapsed();
    std::chrono::milliseconds columnGenerationDuration =
        afterLegacyGeneration - beforeColumnGeneration;
    std::chrono::milliseconds stagedLegacyMasterDuration{0};
    const auto solve_legacy_until = [this,
                                     &state,
                                     &ledger,
                                     &legacyPortfolio,
                                     &masterOptions,
                                     &result](
                                        std::chrono::steady_clock::time_point deadline) {
        MasterOptions legacyMasterOptions = masterOptions;
        legacyMasterOptions.maximumCombinations =
            std::max(256, masterOptions.maximumCombinations / 2);
        legacyMasterOptions.maximumCandidates =
            std::max(8, masterOptions.maximumCandidates / 2);
        legacyMasterOptions.diversityCandidates =
            std::max(2, legacyMasterOptions.maximumCandidates / 4);
        legacyMasterOptions.deadline = deadline;
        MasterDiagnostics legacyDiagnostics;
        std::vector<MasterCandidate> candidates = master_.solve(
            state,
            ledger,
            legacyPortfolio,
            legacyMasterOptions,
            legacyDiagnostics);
        merge_master_diagnostics(result.diagnostics, legacyDiagnostics);
        return candidates;
    };
    std::vector<MasterCandidate> legacyCandidates;
    if (stagedDeepHarvestSearch &&
        !generationOptions.enableExactHarvestOrienteering) {
        const std::chrono::steady_clock::time_point legacyMasterDeadline =
            portfolioPhaseStarted + portfolioPhaseWindow * 60 / 100;
        if (std::chrono::steady_clock::now() < legacyMasterDeadline) {
            const std::chrono::milliseconds beforeLegacyMaster = elapsed();
            legacyCandidates = solve_legacy_until(legacyMasterDeadline);
            stagedLegacyMasterDuration = elapsed() - beforeLegacyMaster;
        }
    }
    if (harvestExtensionMode_ > 1 &&
        (!generationOptions.deadline.has_value() ||
         std::chrono::steady_clock::now() < *generationOptions.deadline)) {
        ColumnGenerationOptions expandedGenerationOptions = generationOptions;
        if (stagedDeepHarvestSearch) {
            expandedGenerationOptions.deadline =
                portfolioPhaseStarted + portfolioPhaseWindow *
                    (generationOptions.enableExactHarvestOrienteering ? 95 : 85) /
                    100;
        }
        const std::chrono::milliseconds beforeExpandedGeneration = elapsed();
        ColumnGenerationDiagnostics expandedDiagnostics;
        RoutePortfolio expandedPortfolio = generator_.generate(
            state,
            ledger,
            expandedGenerationOptions,
            &expandedDiagnostics);
        result.audit.columnGeneration.exactOrienteeringSupportedAgents +=
            expandedDiagnostics.exactOrienteeringSupportedAgents;
        result.audit.columnGeneration.exactOrienteeringCompleteAgents +=
            expandedDiagnostics.exactOrienteeringCompleteAgents;
        result.audit.columnGeneration.exactOrienteeringCacheHits +=
            expandedDiagnostics.exactOrienteeringCacheHits;
        result.audit.columnGeneration.exactOrienteeringSettledStates +=
            expandedDiagnostics.exactOrienteeringSettledStates;
        result.audit.columnGeneration.exactOrienteeringTerminalVariants +=
            expandedDiagnostics.exactOrienteeringTerminalVariants;
        result.audit.columnGeneration.exactOrienteeringBundles +=
            expandedDiagnostics.exactOrienteeringBundles;
        result.audit.columnGeneration.exactOrienteeringSeedServings =
            expandedDiagnostics.exactOrienteeringSeedServings;
        result.audit.columnGeneration.exactOrienteeringLocalServings =
            expandedDiagnostics.exactOrienteeringLocalServings;
        result.audit.columnGeneration.exactOrienteeringFeasibilityNodes +=
            expandedDiagnostics.exactOrienteeringFeasibilityNodes;
        result.audit.columnGeneration
            .exactOrienteeringOverlapFeasibilityNodes +=
            expandedDiagnostics
                .exactOrienteeringOverlapFeasibilityNodes;
        result.audit.columnGeneration
            .exactOrienteeringFeasibilityImprovements +=
            expandedDiagnostics
                .exactOrienteeringFeasibilityImprovements;
        result.audit.columnGeneration.exactOrienteeringFeasibilityImproved =
            result.audit.columnGeneration
                .exactOrienteeringFeasibilityImproved ||
            expandedDiagnostics.exactOrienteeringFeasibilityImproved;
        result.audit.columnGeneration
            .exactOrienteeringOverlapFeasibilityImproved =
            result.audit.columnGeneration
                .exactOrienteeringOverlapFeasibilityImproved ||
            expandedDiagnostics
                .exactOrienteeringOverlapFeasibilityImproved;
        result.audit.columnGeneration.exactOrienteeringMilliseconds +=
            expandedDiagnostics.exactOrienteeringMilliseconds;
        result.audit.columnGeneration
            .exactOrienteeringEnumerationMilliseconds +=
            expandedDiagnostics
                .exactOrienteeringEnumerationMilliseconds;
        result.audit.columnGeneration
            .exactOrienteeringFinalizationMilliseconds +=
            expandedDiagnostics
                .exactOrienteeringFinalizationMilliseconds;
        result.audit.columnGeneration
            .exactOrienteeringDeadlineRemainingAtStartMilliseconds =
            expandedDiagnostics
                .exactOrienteeringDeadlineRemainingAtStartMilliseconds;
        result.audit.columnGeneration
            .exactOrienteeringDeadlineOverrunMilliseconds =
            expandedDiagnostics
                .exactOrienteeringDeadlineOverrunMilliseconds;
        result.audit.columnGeneration.deadlineReached =
            result.audit.columnGeneration.deadlineReached ||
            expandedDiagnostics.deadlineReached;
        columnGenerationDuration += elapsed() - beforeExpandedGeneration;
        std::int32_t nextColumnId = 0;
        for (const std::vector<RouteColumn>& columns :
             portfolio.columnsByAgent) {
            for (const RouteColumn& column : columns) {
                nextColumnId = std::max(nextColumnId, column.columnId + 1);
            }
        }
        const auto same_actions = [](const AgentPlan& left, const AgentPlan& right) {
            if (left.size() != right.size()) {
                return false;
            }
            for (std::size_t actionIndex = 0;
                 actionIndex < left.size();
                 ++actionIndex) {
                if (left.at(actionIndex).kind != right.at(actionIndex).kind ||
                    left.at(actionIndex).value != right.at(actionIndex).value) {
                    return false;
                }
            }
            return true;
        };
        for (std::size_t agentOffset = 0;
             agentOffset < portfolio.columnsByAgent.size();
            ++agentOffset) {
            std::vector<RouteColumn>& retained =
                portfolio.columnsByAgent.at(agentOffset);
            for (RouteColumn& column :
                 expandedPortfolio.columnsByAgent.at(agentOffset)) {
                if (!column.harvestExtension ||
                    std::any_of(
                        retained.begin(),
                        retained.end(),
                        [&column, &same_actions](const RouteColumn& existing) {
                            return same_actions(existing.actions, column.actions) &&
                                (!column.exactOrienteering ||
                                 (existing.exactOrienteering &&
                                  existing.contingencyBundle == column.contingencyBundle));
                        })) {
                    continue;
                }
                column.columnId = nextColumnId++;
                retained.push_back(std::move(column));
            }
        }
    }
    result.timing.columnGeneration = columnGenerationDuration;
    result.audit.portfolioColumnsByAgent.reserve(portfolio.columnsByAgent.size());
    result.audit.portfolioBrandCountsByAgent.reserve(portfolio.columnsByAgent.size());
    result.audit.portfolioMaximumServingsByAgent.reserve(
        portfolio.columnsByAgent.size());
    result.audit.portfolioHarvestExtensionsByAgent.reserve(
        portfolio.columnsByAgent.size());
    result.audit.portfolioTerminalCellsByAgent.reserve(
        portfolio.columnsByAgent.size());
    for (const std::vector<RouteColumn>& columns : portfolio.columnsByAgent) {
        BrandMask brands;
        std::int32_t maximumServings = 0;
        std::int32_t harvestExtensions = 0;
        std::vector<CellId> terminalCells;
        terminalCells.reserve(columns.size());
        for (const RouteColumn& column : columns) {
            brands |= column.estimatedBrands;
            maximumServings = std::max(maximumServings, column.estimatedServings);
            harvestExtensions += column.harvestExtension ? 1 : 0;
            terminalCells.push_back(column.terminalCell);
        }
        result.audit.portfolioColumnsByAgent.push_back(static_cast<std::int32_t>(columns.size()));
        result.audit.portfolioBrandCountsByAgent.push_back(brand_count(brands));
        result.audit.portfolioMaximumServingsByAgent.push_back(maximumServings);
        result.audit.portfolioHarvestExtensionsByAgent.push_back(harvestExtensions);
        result.audit.portfolioTerminalCellsByAgent.push_back(
            std::move(terminalCells));
    }
    const std::chrono::milliseconds beforeInitialMaster = elapsed();
    if (!stagedDeepHarvestSearch && harvestExtensionMode_ > 1 &&
        std::chrono::steady_clock::now() < preRecombinationDeadline) {
        const std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();
        legacyCandidates = solve_legacy_until(
            now + (preRecombinationDeadline - now) / 2);
    }
    if (!legacyCandidates.empty() &&
        better_search_candidate(legacyCandidates.front(), incumbent)) {
        incumbent = legacyCandidates.front();
        incumbentId = incumbent.stableId;
    }
    MasterDiagnostics initialDiagnostics;
    std::vector<MasterCandidate> generatedCandidates = master_.solve(
        state,
        ledger,
        portfolio,
        masterOptions,
        initialDiagnostics);
    merge_master_diagnostics(result.diagnostics, initialDiagnostics);
    generatedCandidates.insert(
        generatedCandidates.end(),
        std::make_move_iterator(legacyCandidates.begin()),
        std::make_move_iterator(legacyCandidates.end()));
    std::sort(
        generatedCandidates.begin(),
        generatedCandidates.end(),
        better_search_candidate);
    result.timing.initialMaster =
        stagedLegacyMasterDuration + elapsed() - beforeInitialMaster;
    if (independentCandidate.has_value()) {
        const bool duplicate = std::any_of(
            generatedCandidates.begin(),
            generatedCandidates.end(),
            [&independentCandidate](const MasterCandidate& candidate) {
                return candidate.stableId == independentCandidate->stableId;
            });
        if (!duplicate) {
            generatedCandidates.push_back(std::move(*independentCandidate));
            std::sort(
                generatedCandidates.begin(),
                generatedCandidates.end(),
                better_search_candidate);
        }
    }
    RoutePortfolio alnsPortfolio = portfolio;
    if (!generatedCandidates.empty() && std::chrono::steady_clock::now() < *masterOptions.deadline) {
        const std::vector<CellId> promoted = promoted_critical_roads(
            config_,
            belief_,
            result.manifest,
            generatedCandidates);
        if (!promoted.empty()) {
            ColumnGenerationOptions promotedGeneration = generationOptions;
            promotedGeneration.criticalRoadHints = promoted;
            MasterOptions promotedMaster = masterOptions;
            promotedMaster.maximumCombinations = std::max(
                256,
                masterOptions.maximumCombinations / 3);
            MasterDiagnostics promotedDiagnostics;
            RoutePortfolio promotedPortfolio = generator_.generate(state, ledger, promotedGeneration);
            std::vector<MasterCandidate> promotedCandidates = master_.solve(
                state,
                ledger,
                promotedPortfolio,
                promotedMaster,
                promotedDiagnostics);
            merge_master_diagnostics(result.diagnostics, promotedDiagnostics);
            result.diagnostics.criticalRoadPromotions += static_cast<std::int32_t>(promoted.size());
            result.audit.promotedCriticalRoads = promoted;
            const std::vector<TrafficScenario> privateSensitivity =
                scenarioGenerator_.private_sensitivity_scenarios(state, belief_, promoted);
            for (const TrafficScenario& scenario : privateSensitivity) {
                for (const CellId road : promoted) {
                    if (scenario.opponentCurrentFootprint.at(static_cast<std::size_t>(road)) > 0 &&
                        std::find(
                            result.audit.privateSensitivityRoads.begin(),
                            result.audit.privateSensitivityRoads.end(),
                            road) == result.audit.privateSensitivityRoads.end()) {
                        result.audit.privateSensitivityRoads.push_back(road);
                    }
                }
            }
            generatedCandidates.insert(
                generatedCandidates.end(),
                std::make_move_iterator(promotedCandidates.begin()),
                std::make_move_iterator(promotedCandidates.end()));
        }
    }
    AlnsOptions alnsOptions;
    alnsOptions.maximumIterations =
        result.deadline.deadlineClass == DeadlineClass::Short
        ? 32
        : (result.deadline.deadlineClass == DeadlineClass::Long
               ? 160
               : 96);
    alnsOptions.maximumProofGuidedIterations =
        result.deadline.deadlineClass == DeadlineClass::Short
        ? 0
        : config_.agent_count() *
            (result.deadline.deadlineClass == DeadlineClass::Long
                ? deadlineCalibration.longProofGuidedPasses
                : deadlineCalibration.normalProofGuidedPasses);
    alnsOptions.maximumAlternativesPerIteration =
        result.deadline.deadlineClass == DeadlineClass::Short ? 4 :
        (result.deadline.deadlineClass == DeadlineClass::Long ? 8 : 6);
    alnsOptions.maximumCandidates = masterOptions.maximumCandidates;
    alnsOptions.diversityCandidates = masterOptions.diversityCandidates;
    alnsOptions.mandatoryReservations = result.viability.reservations;
    alnsOptions.criticalRoads = result.audit.promotedCriticalRoads;
    alnsOptions.brandSlack = result.viability.slackByBrand;
    alnsOptions.latestSafeDayByBrand = result.viability.latestSafeDayByBrand;
    alnsOptions.proofUpperBound =
        result.diagnostics.searchGuidanceUpperBound;
    if (alnsOptions.criticalRoads.empty()) {
        alnsOptions.criticalRoads = baseline_critical_roads(config_, state);
    }
    alnsOptions.deadline = initialAlnsDeadline;
    const std::chrono::milliseconds beforeAlns = elapsed();
    generatedCandidates = alns_.improve(
        state,
        ledger,
        alnsPortfolio,
        std::move(generatedCandidates),
        alnsOptions,
        result.alns);
    result.timing.alns = elapsed() - beforeAlns;

    const std::chrono::milliseconds beforeRecombination = elapsed();
    if (routePoolReserve.count() > 0 && !generatedCandidates.empty()) {
        if (std::chrono::steady_clock::now() < masterSearchDeadline) {
            RoutePoolAugmentation augmentation = generator_.augment_with_candidate_routes(
                state,
                std::move(alnsPortfolio),
                generatedCandidates,
                generationOptions.maximumColumnsPerAgent +
                    (result.deadline.deadlineClass == DeadlineClass::Long ? 16 : 8));
            result.alns.poolRoutesConsidered = augmentation.routesConsidered;
            result.alns.poolNovelRoutes = augmentation.novelRoutes;
            result.alns.poolRetainedRoutes = augmentation.retainedNovelRoutes;
            if (augmentation.retainedNovelRoutes > 0) {
                MasterOptions recombinationOptions = masterOptions;
                recombinationOptions.deadline = firstRecombinationDeadline;
                recombinationOptions.maximumCombinations =
                    result.deadline.deadlineClass == DeadlineClass::Long ? 40000 : 20000;
                recombinationOptions.maximumResolveRounds = 2;
                MasterDiagnostics recombinationDiagnostics;
                std::vector<MasterCandidate> recombined = master_.solve(
                    state,
                    ledger,
                    augmentation.portfolio,
                    recombinationOptions,
                    recombinationDiagnostics);
                result.alns.recombinationCandidates = static_cast<std::int32_t>(recombined.size());
                if (!recombined.empty()) {
                    const MasterCandidate& previousBest = generatedCandidates.front();
                    const MasterCandidate& recombinedBest = recombined.front();
                    if (better_search_candidate(recombinedBest, previousBest)) {
                        result.alns.recombinationImprovements = 1;
                    }
                    generatedCandidates.insert(
                        generatedCandidates.end(),
                        std::make_move_iterator(recombined.begin()),
                        std::make_move_iterator(recombined.end()));
                }
                merge_master_diagnostics(result.diagnostics, recombinationDiagnostics);
            }
            if (feedbackRoutePool &&
                std::chrono::steady_clock::now() < feedbackAlnsDeadline) {
                AlnsOptions feedbackOptions = alnsOptions;
                feedbackOptions.maximumIterations =
                    std::max(16, alnsOptions.maximumIterations / 2);
                feedbackOptions.deadline = feedbackAlnsDeadline;
                AlnsDiagnostics feedbackDiagnostics;
                generatedCandidates = alns_.improve(
                    state,
                    ledger,
                    augmentation.portfolio,
                    std::move(generatedCandidates),
                    feedbackOptions,
                    feedbackDiagnostics);
                merge_alns_diagnostics(result.alns, feedbackDiagnostics);
            }
            if (feedbackRoutePool &&
                std::chrono::steady_clock::now() < masterSearchDeadline) {
                RoutePoolAugmentation finalAugmentation =
                    generator_.augment_with_candidate_routes(
                        state,
                        std::move(augmentation.portfolio),
                        generatedCandidates,
                        generationOptions.maximumColumnsPerAgent +
                            (result.deadline.deadlineClass == DeadlineClass::Long ? 16 : 8));
                result.alns.poolRoutesConsidered += finalAugmentation.routesConsidered;
                result.alns.poolNovelRoutes += finalAugmentation.novelRoutes;
                result.alns.poolRetainedRoutes += finalAugmentation.retainedNovelRoutes;
                if (finalAugmentation.retainedNovelRoutes > 0) {
                    MasterOptions finalOptions = masterOptions;
                    finalOptions.deadline = masterSearchDeadline;
                    finalOptions.maximumCombinations =
                        result.deadline.deadlineClass == DeadlineClass::Long ? 40000 : 20000;
                    finalOptions.maximumResolveRounds = 2;
                    MasterDiagnostics finalDiagnostics;
                    std::vector<MasterCandidate> finalCandidates = master_.solve(
                        state,
                        ledger,
                        finalAugmentation.portfolio,
                        finalOptions,
                        finalDiagnostics);
                    result.alns.recombinationCandidates +=
                        static_cast<std::int32_t>(finalCandidates.size());
                    if (!finalCandidates.empty() &&
                        better_search_candidate(finalCandidates.front(), generatedCandidates.front())) {
                        ++result.alns.recombinationImprovements;
                    }
                    generatedCandidates.insert(
                        generatedCandidates.end(),
                        std::make_move_iterator(finalCandidates.begin()),
                        std::make_move_iterator(finalCandidates.end()));
                    merge_master_diagnostics(result.diagnostics, finalDiagnostics);
                }
            } else if (feedbackRoutePool) {
                result.alns.recombinationDeadlineSkipped = true;
            }
        } else {
            result.alns.recombinationDeadlineSkipped = true;
        }
    }
    result.timing.recombination = elapsed() - beforeRecombination;
    result.timing.search = elapsed() - result.timing.incumbent - result.timing.fastPath;

    std::vector<MasterCandidate> candidates;
    candidates.reserve(1U + cacheRepair.candidates.size() + generatedCandidates.size() + 1U);
    std::set<std::string> candidateIds;
    const auto append_candidate = [this, &result, &candidates, &candidateIds](MasterCandidate candidate) {
        candidate.trafficSafety = counterfactual_traffic_safety(config_, belief_, result.manifest, candidate.simulation);
        if (candidateIds.insert(candidate.stableId).second) {
            candidates.push_back(std::move(candidate));
        }
    };
    append_candidate(std::move(incumbent));
    for (MasterCandidate& cachedCandidate : cacheRepair.candidates) {
        append_candidate(std::move(cachedCandidate));
    }
    for (MasterCandidate& generatedCandidate : generatedCandidates) {
        append_candidate(std::move(generatedCandidate));
    }

    std::optional<std::string> lastSentId;
    if (ledger_.lastCandidate.has_value() && ledger_.lastProfileDay.has_value() &&
        *ledger_.lastProfileDay == state.dayNumber) {
        std::optional<MasterCandidate> lastSent = master_.evaluate_exact_plan(
            state,
            ledger,
            ledger_.lastCandidate->plan,
            result.viability.reservations);
        if (lastSent.has_value()) {
            lastSentId = lastSent->stableId;
            append_candidate(std::move(*lastSent));
        }
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const MasterCandidate& left, const MasterCandidate& right) {
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
        });
    const bool deterministicNoRoad = config_.roadCells.empty();
    std::map<std::string, OfficialScore> f0UpperBounds;
    if (deterministicNoRoad) {
        for (const MasterCandidate& candidate : candidates) {
            f0UpperBounds.emplace(
                candidate.stableId,
                candidate_valid_upper_bound(
                    viabilityAnalyzer_,
                    config_,
                    state,
                    ledger,
                    candidate,
                    f0Deadline));
        }
    }
    std::set<std::string> f0Ids;
    f0Ids.insert(incumbentId);
    if (lastSentId.has_value()) {
        f0Ids.insert(*lastSentId);
    }
    const std::int32_t diversitySlots = std::max(1, f0CandidateLimit / 4);
    const std::int32_t qualityTarget = std::max(
        static_cast<std::int32_t>(f0Ids.size()),
        f0CandidateLimit - diversitySlots);
    for (const MasterCandidate& candidate : candidates) {
        if (static_cast<std::int32_t>(f0Ids.size()) >= qualityTarget) {
            break;
        }
        f0Ids.insert(candidate.stableId);
    }
    while (static_cast<std::int32_t>(f0Ids.size()) < f0CandidateLimit) {
        const MasterCandidate* selectedDiverse = nullptr;
        OfficialScore selectedUpper;
        std::int32_t bestMinimumDistance = -1;
        for (const MasterCandidate& candidate : candidates) {
            if (f0Ids.contains(candidate.stableId)) {
                continue;
            }
            std::int32_t minimumDistance =
                std::numeric_limits<std::int32_t>::max();
            for (const MasterCandidate& selectedCandidate : candidates) {
                if (!f0Ids.contains(selectedCandidate.stableId)) {
                    continue;
                }
                minimumDistance = std::min(
                    minimumDistance,
                    search_plan_distance(
                        candidate,
                        selectedCandidate));
            }
            const std::int32_t upperOrder = !deterministicNoRoad ||
                    selectedDiverse == nullptr
                ? 0
                : compare_lexicographic(
                    f0UpperBounds.at(candidate.stableId),
                    selectedUpper);
            if ((deterministicNoRoad && selectedDiverse == nullptr) ||
                upperOrder > 0 ||
                (upperOrder == 0 && minimumDistance > bestMinimumDistance) ||
                (upperOrder == 0 && minimumDistance == bestMinimumDistance &&
                 (selectedDiverse == nullptr ||
                  better_search_candidate(candidate, *selectedDiverse)))) {
                if (deterministicNoRoad) {
                    selectedUpper = f0UpperBounds.at(candidate.stableId);
                }
                bestMinimumDistance = minimumDistance;
                selectedDiverse = &candidate;
            }
        }
        if (selectedDiverse == nullptr) {
            break;
        }
        f0Ids.insert(selectedDiverse->stableId);
    }
    std::vector<MasterCandidate> f0Candidates;
    f0Candidates.reserve(f0Ids.size());
    for (MasterCandidate& candidate : candidates) {
        if (f0Ids.contains(candidate.stableId)) {
            f0Candidates.push_back(std::move(candidate));
        }
    }
    std::sort(
        f0Candidates.begin(),
        f0Candidates.end(),
        [](const MasterCandidate& left, const MasterCandidate& right) {
            return left.stableId < right.stableId;
        });
    candidates = std::move(f0Candidates);

    std::vector<CandidateEvaluation> evaluations;
    evaluations.reserve(candidates.size());
    for (MasterCandidate& candidate : candidates) {
        const OfficialScore validUpperBound = deterministicNoRoad
            ? f0UpperBounds.at(candidate.stableId)
            : candidate_valid_upper_bound(
                viabilityAnalyzer_,
                config_,
                state,
                ledger,
                candidate,
                f0Deadline);
        CandidateProfile provisional = witnessRepairer_.provisional_profile(
            candidate,
            state,
            ledger,
            belief_,
            result.manifest,
            validUpperBound,
            f0OperationCap,
            f0Deadline);
        const auto cachedSuffix = cacheRepair.certifiedSuffixes.find(
            candidate.stableId);
        if (cachedSuffix != cacheRepair.certifiedSuffixes.end()) {
            for (std::size_t outcomeIndex = 0;
                 outcomeIndex < provisional.outcomes.size();
                 ++outcomeIndex) {
                const OfficialScore scenarioUpper =
                    provisional.scenarioValidUpperBounds.at(outcomeIndex);
                ScenarioOutcome& outcome =
                    provisional.outcomes.at(outcomeIndex);
                if (compare_lexicographic(
                        cachedSuffix->second.score,
                        scenarioUpper) <= 0 &&
                    (!outcome.witness.certified ||
                     outcome.score < cachedSuffix->second.score)) {
                    outcome = ScenarioOutcome{
                        cachedSuffix->second.score,
                        cachedSuffix->second,
                    };
                }
            }
            provisional.provisional =
                !all_outcomes_certified(provisional);
        }
        evaluations.push_back(CandidateEvaluation{std::move(candidate), std::move(provisional)});
    }
    comparator_.finalize_profiles(
        evaluations,
        result.manifest,
        &result.audit.profileFinalization);
    result.audit.selectionReason.clear();
    result.audit.candidates.clear();
    result.audit.candidates.reserve(evaluations.size());
    for (const CandidateEvaluation& evaluation : evaluations) {
        CandidateAuditRecord record;
        record.stableId = evaluation.candidate.stableId;
        record.scoreAfterToday = evaluation.candidate.scoreAfterToday;
        record.provisionalLowerBound = evaluation.profile.provisionalLowerBound;
        record.validUpperBound = evaluation.profile.validUpperBound;
        record.terminalCells.reserve(
            evaluation.candidate.simulation.finalAgents.size());
        record.terminalFuel.reserve(
            evaluation.candidate.simulation.finalAgents.size());
        for (const AgentState& finalAgent :
             evaluation.candidate.simulation.finalAgents) {
            record.terminalCells.push_back(finalAgent.position);
            record.terminalFuel.push_back(finalAgent.fuel);
        }
        record.disposition = "not-shortlisted";
        result.audit.candidates.push_back(std::move(record));
    }
    result.timing.candidatePreparation =
        elapsed() - result.timing.incumbent - result.timing.fastPath - result.timing.search;
    const std::size_t floorLeader = comparator_.choose_provisional(evaluations);
    std::vector<std::size_t> repairIndices;
    const auto enqueue_repair = [&repairIndices](std::size_t candidateIndex) {
        if (std::find(repairIndices.begin(), repairIndices.end(), candidateIndex) == repairIndices.end()) {
            repairIndices.push_back(candidateIndex);
        }
    };
    enqueue_repair(floorLeader);
    append_w1_role(result.audit.candidates.at(floorLeader), "floor-leader");
    result.audit.candidates.at(floorLeader).disposition = "w1-repair";
    if (const std::optional<std::size_t> upside = select_upside_challenger(evaluations, floorLeader);
        upside.has_value()) {
        enqueue_repair(*upside);
        append_w1_role(result.audit.candidates.at(*upside), "upside-challenger");
        result.audit.candidates.at(*upside).disposition = "w1-repair";
    }
    const std::string& incumbentOrLastSentId = lastSentId.has_value() ? *lastSentId : incumbentId;
    for (std::size_t candidateIndex = 0; candidateIndex < evaluations.size(); ++candidateIndex) {
        if (evaluations.at(candidateIndex).candidate.stableId == incumbentOrLastSentId) {
            enqueue_repair(candidateIndex);
            append_w1_role(
                result.audit.candidates.at(candidateIndex),
                lastSentId.has_value() ? "last-sent" : "incumbent");
            result.audit.candidates.at(candidateIndex).disposition = "w1-repair";
            break;
        }
    }
    const std::int32_t repairCap = result.deadline.deadlineClass == DeadlineClass::Short ? 50 : 200;
    const std::chrono::milliseconds finalValidationFloor = std::min(
        result.deadline.certification,
        deadlineCalibration.validationFloor);
    const std::chrono::steady_clock::time_point certificationDeadline =
        started + result.deadline.total - result.deadline.network - finalValidationFloor;
    for (std::size_t repairOffset = 0;
         repairOffset < repairIndices.size();
         ++repairOffset) {
        const std::size_t candidateIndex = repairIndices.at(repairOffset);
        if (all_outcomes_certified(evaluations.at(candidateIndex).profile)) {
            continue;
        }
        const std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();
        const std::int32_t remainingCandidates =
            static_cast<std::int32_t>(repairIndices.size() - repairOffset);
        const std::chrono::steady_clock::time_point candidateDeadline =
            now >= certificationDeadline
            ? certificationDeadline
            : now + (certificationDeadline - now) /
                  std::max(1, remainingCandidates);
        witnessRepairer_.repair_profile(
            evaluations.at(candidateIndex).profile,
            evaluations.at(candidateIndex).candidate,
            state,
            ledger,
            belief_,
            result.manifest,
            repairCap,
            candidateDeadline);
    }
    std::vector<CandidateEvaluation> certifiedPool;
    std::vector<std::size_t> certifiedIndices;
    certifiedPool.reserve(repairIndices.size());
    certifiedIndices.reserve(repairIndices.size());
    for (const std::size_t candidateIndex : repairIndices) {
        const bool certified = all_outcomes_certified(evaluations.at(candidateIndex).profile);
        result.audit.candidates.at(candidateIndex).certified = certified;
        if (certified) {
            certifiedPool.push_back(std::move(evaluations.at(candidateIndex)));
            certifiedIndices.push_back(candidateIndex);
        } else {
            result.audit.candidates.at(candidateIndex).disposition = "w1-deadline-or-invalid";
        }
    }
    if (certifiedPool.empty()) {
        result.manifest = emergency_manifest(config_);
        bind_manifest_policy(result.manifest, result.riskPolicy);
        result.candidate = make_valid_emergency_candidate(config_, simulator_, validator_, state, ledger);
        result.candidate.trafficSafety = counterfactual_traffic_safety(
            config_, belief_, result.manifest, result.candidate.simulation);
        result.profile = witnessRepairer_.provisional_profile(
            result.candidate,
            state,
            ledger,
            belief_,
            result.manifest,
            result.candidate.scoreAfterToday,
            1);
        std::vector<CandidateEvaluation> emergencyEvaluations;
        emergencyEvaluations.push_back(CandidateEvaluation{result.candidate, result.profile});
        comparator_.finalize_profiles(
            emergencyEvaluations,
            result.manifest,
            &result.audit.profileFinalization);
        result.profile = std::move(emergencyEvaluations.front().profile);
        result.emergency = true;
        record_emergency_audit(result, "w1-budget-exhausted-emergency", true);
        finalize_optimality_gap(config_, state, ledger, result);
        result.timing.certification = elapsed() - result.timing.incumbent - result.timing.fastPath -
            result.timing.search - result.timing.candidatePreparation;
        result.timing.total = elapsed();
        return result;
    }
    comparator_.finalize_profiles(
        certifiedPool,
        result.manifest,
        &result.audit.profileFinalization);
    for (std::size_t candidateIndex = 0;
         candidateIndex < certifiedPool.size();
         ++candidateIndex) {
        CandidateAuditRecord& record =
            result.audit.candidates.at(certifiedIndices.at(candidateIndex));
        record.finalQuantile50 =
            certifiedPool.at(candidateIndex).profile.quantiles.at(2);
        record.finalCertifiedLowerBound =
            certifiedPool.at(candidateIndex).profile.certifiedLowerBound;
    }
    std::vector<bool> dominated(certifiedPool.size(), false);
    for (std::size_t candidateIndex = 0; candidateIndex < certifiedPool.size(); ++candidateIndex) {
        for (std::size_t challengerIndex = 0; challengerIndex < certifiedPool.size(); ++challengerIndex) {
            if (candidateIndex == challengerIndex) {
                continue;
            }
            if (comparator_.certified_dominates(
                    certifiedPool.at(challengerIndex).profile,
                    certifiedPool.at(candidateIndex).profile)) {
                dominated.at(candidateIndex) = true;
                result.audit.candidates.at(certifiedIndices.at(candidateIndex)).disposition = "certified-dominated";
                break;
            }
        }
    }
    std::vector<CandidateEvaluation> undominatedPool;
    std::vector<std::size_t> undominatedIndices;
    undominatedPool.reserve(certifiedPool.size());
    undominatedIndices.reserve(certifiedIndices.size());
    for (std::size_t candidateIndex = 0; candidateIndex < certifiedPool.size(); ++candidateIndex) {
        if (!dominated.at(candidateIndex)) {
            undominatedPool.push_back(std::move(certifiedPool.at(candidateIndex)));
            undominatedIndices.push_back(certifiedIndices.at(candidateIndex));
        }
    }
    if (undominatedPool.empty()) {
        throw std::runtime_error("all certified candidates were unexpectedly dominated");
    }
    certifiedPool = std::move(undominatedPool);
    const std::size_t selected = comparator_.choose(
        certifiedPool,
        requireUndominatedCurrentFloor_);
    for (std::size_t candidateIndex = 0; candidateIndex < undominatedIndices.size(); ++candidateIndex) {
        CandidateAuditRecord& record = result.audit.candidates.at(undominatedIndices.at(candidateIndex));
        if (candidateIndex == selected) {
            record.selected = true;
            record.disposition = "selected";
        } else {
            record.disposition = "certified-not-selected";
        }
    }
    result.audit.selectionReason = requireUndominatedCurrentFloor_
        ? "certified-undominated-current-floor"
        : "certified-lexicographic";
    for (const ScenarioOutcome& outcome : certifiedPool.at(selected).profile.outcomes) {
        if (!outcome.witness.certified) {
            throw std::runtime_error("selected candidate lacks a certified future witness");
        }
    }
    result.candidate = std::move(certifiedPool.at(selected).candidate);
    const SimulationResult detailed = simulator_.simulate(state, result.candidate.plan, true);
    const SimulationResult validation = validator_.validate(state, result.candidate.plan, true);
    std::string mismatch;
    if (!detailed.valid || !validator_.agrees_with(detailed, validation, mismatch)) {
        throw std::runtime_error("final candidate failed independent validation: " + mismatch);
    }
    result.candidate.simulation = detailed;
    result.profile = std::move(certifiedPool.at(selected).profile);
    finalize_optimality_gap(config_, state, ledger, result);
    result.timing.certification = elapsed() - result.timing.incumbent - result.timing.fastPath -
        result.timing.search - result.timing.candidatePreparation;
    result.timing.total = elapsed();
    return result;
}

DecisionResult UdonShieldEngine::solve_day_until(
    const DayState& state,
    const MatchLedger& ledger,
    std::chrono::system_clock::time_point receivedAt) {
    const std::chrono::system_clock::time_point endsAt =
        std::chrono::system_clock::time_point{std::chrono::seconds{state.endsAt}};
    const std::chrono::milliseconds available = std::max(
        std::chrono::milliseconds{0},
        std::chrono::duration_cast<std::chrono::milliseconds>(endsAt - receivedAt));
    return solve_day(state, ledger, available);
}

bool UdonShieldEngine::may_submit(const DecisionResult& decision) const {
    if (decision.dayNumber < 1 || decision.dayNumber > config_.day_count()) {
        return false;
    }
    if (ledger_.lastValidResponseByDay.empty() ||
        !ledger_.lastValidResponseByDay.at(static_cast<std::size_t>(decision.dayNumber - 1)).has_value()) {
        return true;
    }
    if (!ledger_.lastProfile.has_value() || !ledger_.lastCandidate.has_value() ||
        !ledger_.lastProfileDay.has_value() || *ledger_.lastProfileDay != decision.dayNumber ||
        !ledger_.lastProfile->hasValidUpperBound) {
        return false;
    }
    if (ledger_.lastCandidate->stableId == decision.candidate.stableId) {
        return false;
    }
    if (decision.emergency || !all_outcomes_certified(decision.profile)) {
        return false;
    }
    if (comparator_.certified_dominates(decision.profile, *ledger_.lastProfile)) {
        return true;
    }
    const std::int32_t currentOrder = compare_lexicographic(
        decision.candidate.scoreAfterToday,
        ledger_.lastCandidate->scoreAfterToday);
    if (currentOrder <= 0) {
        return false;
    }
    std::size_t firstDifference = 0;
    while (firstDifference < 3U &&
           score_component(decision.candidate.scoreAfterToday, firstDifference) ==
               score_component(ledger_.lastCandidate->scoreAfterToday, firstDifference)) {
        ++firstDifference;
    }
    for (std::size_t component = 0; component < firstDifference; ++component) {
        if (score_component(decision.profile.certifiedLowerBound, component) <
            score_component(ledger_.lastProfile->validUpperBound, component)) {
            return false;
        }
    }
    return firstDifference < 3U &&
        score_component(
            decision.profile.certifiedLowerBound,
            firstDifference) >
        score_component(
            ledger_.lastProfile->validUpperBound,
            firstDifference);
}

void UdonShieldEngine::record_submitted(
    const DecisionResult& decision,
    std::chrono::milliseconds responseTime) {
    if (responseTime.count() < 0) {
        throw std::invalid_argument("response time cannot be negative");
    }
    if (decision.dayNumber < 1 || decision.dayNumber > config_.day_count()) {
        throw std::invalid_argument("submitted decision has an invalid day number");
    }
    if (!may_submit(decision)) {
        throw std::invalid_argument("submission is not a certified improvement over the latest response");
    }
    if (lastSubmittedDay_.has_value() && decision.dayNumber < *lastSubmittedDay_) {
        throw std::invalid_argument("submitted decision cannot move backward in time");
    }
    if (ledger_.lastValidResponseByDay.empty()) {
        ledger_.lastValidResponseByDay.resize(static_cast<std::size_t>(config_.day_count()));
    }
    std::optional<std::chrono::milliseconds>& previous =
        ledger_.lastValidResponseByDay.at(static_cast<std::size_t>(decision.dayNumber - 1));
    if (previous.has_value()) {
        ledger_.totalResponse -= *previous;
    }
    previous = responseTime;
    ledger_.totalResponse += responseTime;
    ledger_.lastProfile = decision.profile;
    ledger_.lastCandidate = decision.candidate;
    ledger_.lastProfileDay = decision.dayNumber;
    ledger_.cachedContingencies.clear();
    ledger_.strongProofs.clear();
    if (decision.dayNumber < config_.day_count()) {
        std::set<std::string> cachedPlanIds;
        for (std::size_t outcomeIndex = 0; outcomeIndex < decision.profile.outcomes.size(); ++outcomeIndex) {
            const FutureWitness& witness = decision.profile.outcomes.at(outcomeIndex).witness;
            if (!witness.certified || witness.futurePlans.empty()) {
                continue;
            }
            const DayPlan& plan = witness.futurePlans.front();
            if (plan.actions.size() != static_cast<std::size_t>(config_.agent_count()) ||
                !cachedPlanIds.insert(canonical_plan_bytes(plan)).second) {
                continue;
            }
            ResponseLedger::CachedContingency contingency;
            contingency.dayNumber = decision.dayNumber + 1;
            contingency.plan = plan;
            if (config_.roadCells.empty()) {
                contingency.certifiedSuffix = witness;
            }
            if (outcomeIndex < decision.manifest.scenarios.size()) {
                contingency.scenarioId = decision.manifest.scenarios.at(outcomeIndex).scenarioId;
                contingency.scenarioClass = decision.manifest.scenarios.at(outcomeIndex).scenarioClass;
            } else {
                contingency.scenarioId = static_cast<std::int32_t>(outcomeIndex);
                contingency.scenarioClass = "certified-cache";
            }
            ledger_.cachedContingencies.push_back(std::move(contingency));
        }
    }
    previousOwnFootprintBeforeLastSubmission_ = belief_.previous_own_footprint();
    belief_.record_own_footprint(decision.dayNumber, decision.candidate.simulation.roadFootprint);
    lastSubmittedDay_ = decision.dayNumber;
    expectedNextAgents_ = decision.candidate.simulation.finalAgents;
    remainingPostAckComputeBudget_ =
        kCompetitionComputeHardCap -
        competition_compute_budget(decision.timing.total);
}

void UdonShieldEngine::record_applied_transition(
    const DayState& state,
    const SimulationResult& simulation) {
    if (!simulation.valid) {
        throw std::invalid_argument("cannot record an invalid applied transition");
    }
    if (state.dayNumber < 1 || state.dayNumber > config_.day_count()) {
        throw std::invalid_argument("applied transition has an invalid day number");
    }
    if (simulation.finalAgents.size() != state.agents.size()) {
        throw std::invalid_argument("applied transition has an invalid final agent count");
    }
    belief_.observe(state);
    previousOwnFootprintBeforeLastSubmission_ = belief_.previous_own_footprint();
    belief_.record_own_footprint(state.dayNumber, simulation.roadFootprint);
    lastSubmittedDay_ = state.dayNumber;
    expectedNextAgents_ = simulation.finalAgents;
    ledger_.lastProfile.reset();
    ledger_.lastCandidate.reset();
    ledger_.lastProfileDay.reset();
    ledger_.cachedContingencies.clear();
    ledger_.strongProofs.clear();
    remainingPostAckComputeBudget_ = std::chrono::milliseconds{0};
}

void UdonShieldEngine::restore_response_artifacts(
    std::vector<ResponseLedger::CachedContingency> cachedContingencies,
    std::vector<ResponseLedger::StrongProofRecord> strongProofs) {
    for (const ResponseLedger::CachedContingency& contingency : cachedContingencies) {
        if (contingency.dayNumber < 1 || contingency.dayNumber > config_.day_count() ||
            contingency.plan.actions.size() != static_cast<std::size_t>(config_.agent_count())) {
            throw std::invalid_argument("restored contingency is outside the match contract");
        }
    }
    for (const ResponseLedger::StrongProofRecord& proof : strongProofs) {
        if (proof.dayNumber < 1 || proof.dayNumber > config_.day_count()) {
            throw std::invalid_argument("restored strong proof is outside the match contract");
        }
    }
    ledger_.cachedContingencies = std::move(cachedContingencies);
    ledger_.strongProofs = std::move(strongProofs);
}

namespace {

void consume_background_compute_budget(
    std::chrono::milliseconds& remaining,
    std::chrono::steady_clock::time_point started) {
    const std::chrono::milliseconds elapsed = std::chrono::ceil<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started);
    remaining -= std::min(remaining, std::max(std::chrono::milliseconds{0}, elapsed));
}

} // namespace

std::int32_t UdonShieldEngine::precompute_next_day_contingencies(
    const DayState& state,
    const MatchLedger& ledger,
    const DecisionResult& decision,
    std::chrono::steady_clock::time_point deadline) {
    const std::chrono::steady_clock::time_point started =
        std::chrono::steady_clock::now();
    deadline = std::min(
        deadline,
        started + remainingPostAckComputeBudget_);
    if (state.dayNumber != decision.dayNumber || decision.dayNumber >= config_.day_count() ||
        remainingPostAckComputeBudget_.count() <= 0 || started >= deadline) {
        return 0;
    }
    if (!ledger_.lastCandidate.has_value() || !ledger_.lastProfileDay.has_value() ||
        *ledger_.lastProfileDay != decision.dayNumber ||
        ledger_.lastCandidate->stableId != decision.candidate.stableId) {
        throw std::invalid_argument("contingency precompute requires the acknowledged decision");
    }
    const std::size_t cellCount = static_cast<std::size_t>(config_.map.cell_count());
    std::vector<std::int32_t> previousOwn(cellCount, 0);
    if (previousOwnFootprintBeforeLastSubmission_.has_value()) {
        if (previousOwnFootprintBeforeLastSubmission_->size() != cellCount) {
            throw std::runtime_error("stored pre-submission footprint has invalid size");
        }
        previousOwn = *previousOwnFootprintBeforeLastSubmission_;
    }
    MatchLedger futureLedger = ledger;
    futureLedger.apply(decision.candidate.simulation.score);
    DayState futureState;
    futureState.dayNumber = decision.dayNumber + 1;
    futureState.agents = decision.candidate.simulation.finalAgents;
    futureState.others = state.others;
    std::set<std::string> cachedPlanIds;
    for (const ResponseLedger::CachedContingency& contingency : ledger_.cachedContingencies) {
        if (contingency.dayNumber == futureState.dayNumber) {
            cachedPlanIds.insert(canonical_plan_bytes(contingency.plan));
        }
    }
    std::int32_t added = 0;
    std::vector<const TrafficScenario*> scenarioOrder;
    scenarioOrder.reserve(decision.manifest.scenarios.size());
    for (const TrafficScenario& scenario : decision.manifest.scenarios) {
        if (scenario.pessimisticFallback) {
            scenarioOrder.push_back(&scenario);
        }
    }
    for (const TrafficScenario& scenario : decision.manifest.scenarios) {
        if (!scenario.pessimisticFallback) {
            scenarioOrder.push_back(&scenario);
        }
    }
    for (const TrafficScenario* scenarioPointer : scenarioOrder) {
        if (std::chrono::steady_clock::now() >= deadline) {
            break;
        }
        const TrafficScenario& scenario = *scenarioPointer;
        futureState.roadStatuses = predict_with_components(
            config_,
            previousOwn,
            decision.candidate.simulation.roadFootprint,
            scenario.opponentCarryFootprint,
            scenario.opponentCurrentFootprint);
        DayPlan plan;
        if (scenario.pessimisticFallback) {
            plan = emergency_wait_plan(config_, futureState);
        } else {
            const ViabilityBounds futureViability = viabilityAnalyzer_.analyze(futureState, futureLedger, deadline);
            ColumnGenerationOptions generationOptions;
            generationOptions.maximumPathsPerTarget = 2;
            generationOptions.maximumColumnsPerAgent = 4;
            generationOptions.maximumTargetSpots = 8;
            generationOptions.maximumEscorts = 6;
            generationOptions.maximumSeedPlans = 1;
            generationOptions.mandatoryReservations = futureViability.reservations;
            generationOptions.deadline = deadline;
            MasterOptions masterOptions;
            masterOptions.maximumCombinations = 512;
            masterOptions.maximumCandidates = 1;
            masterOptions.maximumResolveRounds = 1;
            masterOptions.mandatoryReservations = futureViability.reservations;
            masterOptions.deadline = deadline;
            MasterDiagnostics diagnostics;
            const RoutePortfolio portfolio = generator_.generate(futureState, futureLedger, generationOptions);
            std::vector<MasterCandidate> candidates = master_.solve(
                futureState,
                futureLedger,
                portfolio,
                masterOptions,
                diagnostics);
            plan = candidates.empty() ? emergency_wait_plan(config_, futureState) : candidates.front().plan;
        }
        const std::optional<MasterCandidate> exact = master_.evaluate_exact_plan(futureState, futureLedger, plan);
        if (!exact.has_value()) {
            continue;
        }
        const std::string planId = exact->stableId;
        if (!cachedPlanIds.insert(planId).second) {
            continue;
        }
        ResponseLedger::CachedContingency contingency;
        contingency.dayNumber = futureState.dayNumber;
        contingency.scenarioId = scenario.scenarioId;
        contingency.scenarioClass = scenario.scenarioClass;
        contingency.plan = std::move(plan);
        ledger_.cachedContingencies.push_back(std::move(contingency));
        ++added;
    }
    consume_background_compute_budget(remainingPostAckComputeBudget_, started);
    return added;
}

std::int32_t UdonShieldEngine::prove_remaining_horizon(
    const DayState& state,
    const MatchLedger& ledger,
    const DecisionResult& decision,
    std::chrono::steady_clock::time_point deadline) {
    const std::chrono::steady_clock::time_point started =
        std::chrono::steady_clock::now();
    deadline = std::min(
        deadline,
        started + remainingPostAckComputeBudget_);
    if (state.dayNumber != decision.dayNumber || decision.dayNumber >= config_.day_count() ||
        remainingPostAckComputeBudget_.count() <= 0 || started >= deadline) {
        return 0;
    }
    if (!ledger_.lastCandidate.has_value() || !ledger_.lastProfileDay.has_value() ||
        *ledger_.lastProfileDay != decision.dayNumber ||
        ledger_.lastCandidate->stableId != decision.candidate.stableId) {
        throw std::invalid_argument("strong proof search requires the acknowledged decision");
    }
    const std::size_t cellCount = static_cast<std::size_t>(config_.map.cell_count());
    std::vector<std::int32_t> previousOwn(cellCount, 0);
    if (previousOwnFootprintBeforeLastSubmission_.has_value()) {
        if (previousOwnFootprintBeforeLastSubmission_->size() != cellCount) {
            throw std::runtime_error("stored pre-submission footprint has invalid size");
        }
        previousOwn = *previousOwnFootprintBeforeLastSubmission_;
    }
    MatchLedger futureLedger = ledger;
    futureLedger.apply(decision.candidate.simulation.score);
    const std::int64_t stockPerDay = std::accumulate(
        config_.spots.begin(),
        config_.spots.end(),
        std::int64_t{0},
        [](std::int64_t total, const Spot& spot) { return total + spot.stock; });
    const auto bounded_int = [](std::int64_t value) {
        return static_cast<std::int32_t>(std::min<std::int64_t>(
            std::numeric_limits<std::int32_t>::max(),
            std::max<std::int64_t>(0, value)));
    };
    const auto horizon_upper_bound = [&](const MatchLedger& branchLedger, std::int32_t nextDay) {
        const std::int64_t remainingDays = std::max(0, config_.day_count() - nextDay + 1);
        return OfficialScore{
            config_.brand_count(),
            bounded_int(
                static_cast<std::int64_t>(branchLedger.totalDailyDistinct) +
                remainingDays * config_.brand_count()),
            bounded_int(
                static_cast<std::int64_t>(branchLedger.totalServings) +
                remainingDays * stockPerDay),
        };
    };
    std::int32_t completed = 0;
    for (const TrafficScenario& scenario : decision.manifest.scenarios) {
        if (std::chrono::steady_clock::now() >= deadline) {
            break;
        }
        const auto existing = std::find_if(
            ledger_.strongProofs.begin(),
            ledger_.strongProofs.end(),
            [&](const ResponseLedger::StrongProofRecord& proof) {
                return proof.dayNumber == decision.dayNumber + 1 &&
                    proof.scenarioId == scenario.scenarioId;
            });
        if (existing != ledger_.strongProofs.end() && existing->complete) {
            continue;
        }
        ResponseLedger::StrongProofRecord proof;
        proof.dayNumber = decision.dayNumber + 1;
        proof.scenarioId = scenario.scenarioId;
        proof.scenarioClass = scenario.scenarioClass;
        proof.upperBound = horizon_upper_bound(futureLedger, proof.dayNumber);
        DayState rootState;
        rootState.dayNumber = proof.dayNumber;
        rootState.agents = decision.candidate.simulation.finalAgents;
        rootState.others = state.others;
        rootState.roadStatuses = predict_with_components(
            config_,
            previousOwn,
            decision.candidate.simulation.roadFootprint,
            scenario.opponentCarryFootprint,
            scenario.opponentCurrentFootprint);
        OfficialScore bestScore;
        bool hasBest = false;
        std::int64_t combinationsVisited = 0;
        std::int64_t branchesPruned = 0;
        std::function<bool(const DayState&, const MatchLedger&, const std::vector<std::int32_t>&)> search;
        search = [&](const DayState& branchState,
                     const MatchLedger& branchLedger,
                     const std::vector<std::int32_t>& currentOwn) {
            if (std::chrono::steady_clock::now() >= deadline) {
                return false;
            }
            if (branchState.dayNumber > config_.day_count()) {
                const OfficialScore score = current_score(branchLedger);
                if (!hasBest || compare_lexicographic(score, bestScore) > 0) {
                    bestScore = score;
                    hasBest = true;
                }
                return true;
            }
            const OfficialScore branchUpperBound = horizon_upper_bound(branchLedger, branchState.dayNumber);
            if (hasBest && compare_lexicographic(branchUpperBound, bestScore) <= 0) {
                ++branchesPruned;
                return true;
            }
            ColumnGenerationOptions generationOptions;
            generationOptions.deadline = deadline;
            if (branchState.dayNumber == proof.dayNumber) {
                for (const ResponseLedger::CachedContingency& contingency : ledger_.cachedContingencies) {
                    if (contingency.dayNumber == branchState.dayNumber &&
                        contingency.scenarioId == scenario.scenarioId) {
                        generationOptions.seedPlans.push_back(contingency.plan);
                    }
                }
            }
            const RoutePortfolio portfolio = generator_.generate(branchState, branchLedger, generationOptions);
            if (std::chrono::steady_clock::now() >= deadline) {
                return false;
            }
            MasterOptions masterOptions;
            masterOptions.maximumCombinations = std::numeric_limits<std::int32_t>::max();
            masterOptions.maximumCandidates = 4096;
            masterOptions.maximumResolveRounds = 1;
            masterOptions.enableLexicographicBranchAndBound = false;
            masterOptions.deadline = deadline;
            MasterDiagnostics diagnostics;
            const std::vector<MasterCandidate> candidates = master_.solve(
                branchState,
                branchLedger,
                portfolio,
                masterOptions,
                diagnostics);
            combinationsVisited += diagnostics.combinationsVisited;
            branchesPruned += diagnostics.branchesPruned;
            if (!diagnostics.searchComplete ||
                diagnostics.simulatorValidCombinations >
                    static_cast<std::int32_t>(candidates.size())) {
                return false;
            }
            for (const MasterCandidate& candidate : candidates) {
                if (std::chrono::steady_clock::now() >= deadline) {
                    return false;
                }
                MatchLedger nextLedger = branchLedger;
                nextLedger.apply(candidate.simulation.score);
                DayState nextState;
                nextState.dayNumber = branchState.dayNumber + 1;
                nextState.agents = candidate.simulation.finalAgents;
                nextState.others = branchState.others;
                if (nextState.dayNumber <= config_.day_count()) {
                    nextState.roadStatuses = predict_with_components(
                        config_,
                        currentOwn,
                        candidate.simulation.roadFootprint,
                        scenario.opponentCurrentFootprint,
                        scenario.opponentCurrentFootprint);
                }
                if (!search(nextState, nextLedger, candidate.simulation.roadFootprint)) {
                    return false;
                }
            }
            return true;
        };
        proof.complete = search(
            rootState,
            futureLedger,
            decision.candidate.simulation.roadFootprint);
        proof.infeasible = proof.complete && !hasBest;
        proof.bestScore = hasBest ? bestScore : current_score(futureLedger);
        proof.upperBound = proof.complete && hasBest ? bestScore : proof.upperBound;
        proof.combinationsVisited = bounded_int(combinationsVisited);
        proof.branchesPruned = bounded_int(branchesPruned);
        if (existing == ledger_.strongProofs.end()) {
            ledger_.strongProofs.push_back(proof);
        } else {
            *existing = proof;
        }
        if (proof.complete) {
            ++completed;
        } else {
            break;
        }
    }
    consume_background_compute_budget(remainingPostAckComputeBudget_, started);
    return completed;
}

const ResponseLedger& UdonShieldEngine::response_ledger() const {
    return ledger_;
}

const std::vector<std::int32_t>& UdonShieldEngine::previous_own_footprint() const {
    return belief_.previous_own_footprint();
}

std::chrono::milliseconds UdonShieldEngine::remaining_post_ack_compute_budget() const {
    return remainingPostAckComputeBudget_;
}

}
