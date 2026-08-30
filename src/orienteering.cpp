#include "udon/orienteering.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace udon {

namespace {

constexpr std::uint16_t kUnreachable = std::numeric_limits<std::uint16_t>::max();
constexpr std::size_t kMaximumExactStates = 8U * 1024U * 1024U;

struct ResourceLabel {
    std::uint32_t state = 0;
    std::uint16_t usedSteps = 0;
    std::uint16_t usedFuel = 0;
    std::uint32_t parent = std::numeric_limits<std::uint32_t>::max();
    std::int32_t nextAtState = -1;
    std::uint8_t incoming = std::numeric_limits<std::uint8_t>::max();
    bool active = true;
};

[[nodiscard]] std::vector<std::uint32_t> inclusion_maximal_masks(
    const std::vector<bool>& reachable,
    const std::optional<std::chrono::steady_clock::time_point>& deadline,
    bool& complete) {
    std::vector<std::uint32_t> maximal;
    const std::uint32_t maskCount = static_cast<std::uint32_t>(reachable.size());
    const std::uint32_t fullMask = maskCount - 1U;
    std::vector<std::uint8_t> reachableSuperset(reachable.size(), 0U);
    for (std::uint32_t mask = 0; mask < maskCount; ++mask) {
        reachableSuperset.at(mask) = reachable.at(mask) ? 1U : 0U;
    }
    const std::uint32_t bitCount = std::bit_width(fullMask);
    for (std::uint32_t bit = 0; bit < bitCount; ++bit) {
        const std::uint32_t flag = std::uint32_t{1} << bit;
        for (std::uint32_t mask = 0; mask < maskCount; ++mask) {
            if ((mask & 1023U) == 0U && deadline.has_value() &&
                std::chrono::steady_clock::now() >= *deadline) {
                complete = false;
                return {};
            }
            if ((mask & flag) == 0U) {
                reachableSuperset.at(mask) = static_cast<std::uint8_t>(
                    reachableSuperset.at(mask) |
                    reachableSuperset.at(mask | flag));
            }
        }
    }
    for (std::uint32_t mask = 0; mask < maskCount; ++mask) {
        if ((mask & 1023U) == 0U && deadline.has_value() &&
            std::chrono::steady_clock::now() >= *deadline) {
            complete = false;
            return {};
        }
        if (!reachable.at(mask)) {
            continue;
        }
        bool dominated = false;
        const std::uint32_t complement = fullMask ^ mask;
        for (std::uint32_t remaining = complement;
             remaining != 0U;
             remaining &= remaining - 1U) {
            const std::uint32_t added = remaining & (~remaining + 1U);
            if (reachableSuperset.at(mask | added) != 0U) {
                dominated = true;
                break;
            }
        }
        if (!dominated) {
            maximal.push_back(mask);
        }
    }
    std::sort(
        maximal.begin(),
        maximal.end(),
        [](std::uint32_t left, std::uint32_t right) {
            return std::pair{std::popcount(left), left} >
                std::pair{std::popcount(right), right};
        });
    return maximal;
}

}

bool exact_orienteering_dense_state_supported(
    const MatchConfig& config) noexcept {
    if (config.spots.size() > 16U || config.map.cell_count() <= 0) {
        return false;
    }
    const std::size_t maskCount = std::size_t{1} << config.spots.size();
    const std::size_t cellCount = static_cast<std::size_t>(
        config.map.cell_count());
    return maskCount <= kMaximumExactStates / cellCount;
}

ExactOrienteeringReachability enumerate_exact_high_fuel_routes(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agentIndex,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
    ExactOrienteeringReachability result;
    if (agentIndex < 0 || agentIndex >= static_cast<AgentIndex>(state.agents.size()) ||
        state.dayNumber < 1 || state.dayNumber > config.day_count() ||
        state.roadStatuses.size() != static_cast<std::size_t>(config.map.cell_count())) {
        return result;
    }
    const AgentState& agent = state.agents.at(static_cast<std::size_t>(agentIndex));
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    if (agent.kind != AgentKind::Patrol || daySteps <= 0 || daySteps >= kUnreachable ||
        daySteps > static_cast<std::int32_t>(
            std::numeric_limits<std::uint16_t>::max()) ||
        !exact_orienteering_dense_state_supported(config) ||
        static_cast<std::int64_t>(agent.fuel) <
            2LL * static_cast<std::int64_t>(daySteps)) {
        return result;
    }
    const std::uint32_t maskCount = std::uint32_t{1} <<
        static_cast<std::uint32_t>(config.spots.size());
    const std::uint32_t cellCount = static_cast<std::uint32_t>(config.map.cell_count());
    const std::size_t stateCount = static_cast<std::size_t>(maskCount) * cellCount;
    if (cellCount == 0U || stateCount > kMaximumExactStates) {
        return result;
    }
    result.supported = true;
    const auto deadline_expired = [&deadline]() {
        return deadline.has_value() &&
            std::chrono::steady_clock::now() >= *deadline;
    };
    if (deadline_expired()) {
        return result;
    }

    std::vector<std::uint16_t> distance;
    distance.reserve(stateCount);
    if (deadline_expired()) {
        return result;
    }
    std::vector<std::uint16_t> patrolFuel;
    patrolFuel.reserve(stateCount);
    if (deadline_expired()) {
        return result;
    }
    std::vector<std::uint32_t> parent;
    parent.reserve(stateCount);
    if (deadline_expired()) {
        return result;
    }
    std::vector<std::uint8_t> incoming;
    incoming.reserve(stateCount);
    constexpr std::size_t kInitializationChunkStates = 65536U;
    while (distance.size() < stateCount) {
        if (deadline_expired()) {
            return result;
        }
        const std::size_t initialized = std::min(
            stateCount,
            distance.size() + kInitializationChunkStates);
        distance.resize(initialized, kUnreachable);
        patrolFuel.resize(initialized, kUnreachable);
        parent.resize(
            initialized,
            std::numeric_limits<std::uint32_t>::max());
        incoming.resize(
            initialized,
            std::numeric_limits<std::uint8_t>::max());
    }
    std::vector<std::vector<std::uint64_t>> buckets(
        static_cast<std::size_t>(daySteps) + 1U);
    const auto state_id = [cellCount](std::uint32_t mask, CellId cell) {
        return mask * cellCount + static_cast<std::uint32_t>(cell);
    };
    const auto relax = [&distance, &patrolFuel, &parent, &incoming, &buckets](
                           std::uint32_t id,
                           std::uint16_t candidateSteps,
                           std::uint16_t candidateFuel,
                           std::uint32_t predecessor,
                           std::uint8_t action) {
        if (std::pair{candidateSteps, candidateFuel} >=
            std::pair{distance.at(id), patrolFuel.at(id)}) {
            return;
        }
        distance.at(id) = candidateSteps;
        patrolFuel.at(id) = candidateFuel;
        parent.at(id) = predecessor;
        incoming.at(id) = action;
        buckets.at(candidateSteps).push_back(
            (static_cast<std::uint64_t>(candidateFuel) << 32U) | id);
    };

    const std::uint32_t root = state_id(0U, agent.position);
    relax(
        root,
        0U,
        0U,
        std::numeric_limits<std::uint32_t>::max(),
        std::numeric_limits<std::uint8_t>::max());
    const SpotIndex startSpot =
        config.spotAtCell.at(static_cast<std::size_t>(agent.position));
    if (startSpot != kInvalidSpot) {
        relax(
            state_id(
                std::uint32_t{1} << static_cast<std::uint32_t>(startSpot),
                agent.position),
            1U,
            0U,
            root,
            static_cast<std::uint8_t>(kDirectionCount));
    }

    std::vector<bool> reachableMasks(maskCount, false);
    std::vector<MoveCost> moveCosts(cellCount);
    std::vector<std::uint32_t> destinationSpotBits(cellCount, 0U);
    for (std::uint32_t cell = 0; cell < cellCount; ++cell) {
        moveCosts.at(cell) = config.move_cost(
            static_cast<CellId>(cell),
            state.roadStatuses.at(cell));
        const SpotIndex spot = config.spotAtCell.at(cell);
        if (spot != kInvalidSpot) {
            destinationSpotBits.at(cell) =
                std::uint32_t{1} << static_cast<std::uint32_t>(spot);
        }
    }
    std::vector<std::vector<std::uint16_t>> spotTerminalDistances(
        config.spots.size(),
        std::vector<std::uint16_t>(cellCount, 0U));
    for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
        for (std::uint32_t cell = 0; cell < cellCount; ++cell) {
            spotTerminalDistances.at(spot).at(cell) =
                static_cast<std::uint16_t>(config.map.hex_distance(
                    static_cast<CellId>(cell),
                    config.spots.at(spot).position));
        }
    }
    std::vector<std::int32_t> terminalBrandDistanceByCell(cellCount, 0);
    for (std::uint32_t cell = 0; cell < cellCount; ++cell) {
        for (std::int32_t brand = 0; brand < config.brand_count(); ++brand) {
            std::uint16_t nearestBrand = kUnreachable;
            for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
                if (config.spots.at(spot).brandIndex == brand) {
                    nearestBrand = std::min(
                        nearestBrand,
                        spotTerminalDistances.at(spot).at(cell));
                }
            }
            if (nearestBrand != kUnreachable) {
                terminalBrandDistanceByCell.at(cell) += nearestBrand;
            }
        }
    }
    std::vector<std::pair<CellId, std::vector<std::uint16_t>>>
        tankerTerminalDistances;
    for (const AgentState& target : state.agents) {
        if (target.kind != AgentKind::Tanker) {
            continue;
        }
        std::vector<std::uint16_t> distances(cellCount, 0U);
        for (std::uint32_t cell = 0; cell < cellCount; ++cell) {
            distances.at(cell) = static_cast<std::uint16_t>(
                config.map.hex_distance(
                    static_cast<CellId>(cell),
                    target.position));
        }
        tankerTerminalDistances.emplace_back(
            target.position,
            std::move(distances));
    }
    std::uint64_t processedBucketEntries = 0U;
    for (std::uint16_t usedSteps = 0;
         usedSteps <= static_cast<std::uint16_t>(daySteps);
         ++usedSteps) {
        std::vector<std::uint64_t>& bucket = buckets.at(usedSteps);
        for (std::size_t offset = 0; offset < bucket.size(); ++offset) {
            if ((processedBucketEntries++ & 4095U) == 0U &&
                deadline_expired()) {
                return result;
            }
            const std::uint64_t entry = bucket.at(offset);
            const std::uint16_t usedFuel = static_cast<std::uint16_t>(entry >> 32U);
            const std::uint32_t id = static_cast<std::uint32_t>(entry);
            if (distance.at(id) != usedSteps || patrolFuel.at(id) != usedFuel) {
                continue;
            }
            ++result.settledStates;
            const std::uint32_t mask = id / cellCount;
            const CellId cell = static_cast<CellId>(id % cellCount);
            reachableMasks.at(mask) = true;
            const MoveCost move = moveCosts.at(static_cast<std::size_t>(cell));
            const std::int32_t candidateSteps =
                static_cast<std::int32_t>(usedSteps) + move.steps;
            const std::int32_t candidateFuel =
                static_cast<std::int32_t>(usedFuel) + move.patrolFuel;
            if (candidateSteps > daySteps || candidateFuel > agent.fuel) {
                continue;
            }
            for (std::int32_t direction = 0;
                 direction < kDirectionCount;
                 ++direction) {
                const CellId destination = config.map.neighbors
                    .at(static_cast<std::size_t>(cell))
                    .at(static_cast<std::size_t>(direction));
                if (destination == kInvalidCell ||
                    config.map.terrain.at(static_cast<std::size_t>(destination)) ==
                        Terrain::Pond) {
                    continue;
                }
                const std::uint32_t candidateMask = mask |
                    destinationSpotBits.at(static_cast<std::size_t>(destination));
                relax(
                    state_id(candidateMask, destination),
                    static_cast<std::uint16_t>(candidateSteps),
                    static_cast<std::uint16_t>(candidateFuel),
                    id,
                    static_cast<std::uint8_t>(direction));
            }
        }
    }

    const auto reconstruct_high_fuel_route =
        [&config,
         &distance,
         &patrolFuel,
         &parent,
         &incoming,
         &terminalBrandDistanceByCell,
         cellCount,
         daySteps,
         root](std::uint32_t witness, std::uint32_t mask) {
            ExactOrienteeringRoute route;
            route.spotMask = mask;
            route.usedSteps = distance.at(witness);
            route.patrolFuel = patrolFuel.at(witness);
            route.terminalCell = static_cast<CellId>(witness % cellCount);
            route.terminalOnSpot =
                config.spotAtCell.at(static_cast<std::size_t>(route.terminalCell)) !=
                kInvalidSpot;
            route.terminalBrandDistance = terminalBrandDistanceByCell.at(
                static_cast<std::size_t>(route.terminalCell));
            std::vector<PlanAction> reversed;
            std::uint32_t current = witness;
            while (current != root) {
                const std::uint8_t action = incoming.at(current);
                if (action == static_cast<std::uint8_t>(kDirectionCount)) {
                    reversed.push_back(PlanAction::wait(1));
                } else if (action < static_cast<std::uint8_t>(kDirectionCount)) {
                    reversed.push_back(PlanAction::move(action));
                } else {
                    throw std::runtime_error(
                        "exact orienteering predecessor chain is broken");
                }
                current = parent.at(current);
            }
            std::reverse(reversed.begin(), reversed.end());
            if (route.usedSteps < daySteps) {
                reversed.push_back(PlanAction::wait(daySteps - route.usedSteps));
            }
            route.actions = std::move(reversed);
            return route;
        };

    bool maximalComplete = true;
    const std::vector<std::uint32_t> maximalMasks = inclusion_maximal_masks(
        reachableMasks,
        deadline,
        maximalComplete);
    if (!maximalComplete) {
        return result;
    }
    result.maximalRoutes.reserve(maximalMasks.size());
    result.terminalVariants.reserve(maximalMasks.size() * (config.spots.size() + 1U));
    for (std::size_t maskOffset = 0; maskOffset < maximalMasks.size(); ++maskOffset) {
        if ((maskOffset & 15U) == 0U && deadline_expired()) {
            return result;
        }
        const std::uint32_t mask = maximalMasks.at(maskOffset);
        std::vector<std::uint32_t> candidateStates;
        const auto add_candidate = [&candidateStates](std::uint32_t id) {
            if (id != std::numeric_limits<std::uint32_t>::max()) {
                candidateStates.push_back(id);
            }
        };
        std::uint32_t fastest = std::numeric_limits<std::uint32_t>::max();
        std::uint32_t lowestFuel = std::numeric_limits<std::uint32_t>::max();
        for (std::uint32_t cell = 0; cell < cellCount; ++cell) {
            if ((cell & 255U) == 0U && deadline_expired()) {
                return result;
            }
            const std::uint32_t id = state_id(mask, static_cast<CellId>(cell));
            if (distance.at(id) == kUnreachable) {
                continue;
            }
            if (fastest == std::numeric_limits<std::uint32_t>::max() ||
                std::tuple{distance.at(id), patrolFuel.at(id), id} <
                    std::tuple{
                        distance.at(fastest),
                        patrolFuel.at(fastest),
                        fastest}) {
                fastest = id;
            }
            if (lowestFuel == std::numeric_limits<std::uint32_t>::max() ||
                std::tuple{patrolFuel.at(id), distance.at(id), id} <
                    std::tuple{
                        patrolFuel.at(lowestFuel),
                        distance.at(lowestFuel),
                        lowestFuel}) {
                lowestFuel = id;
            }
        }
        add_candidate(fastest);
        add_candidate(lowestFuel);
        const auto add_nearest_terminal = [&](
                CellId targetCell,
                const std::vector<std::uint16_t>& terminalDistances,
                bool excludeTarget) {
            std::uint32_t nearest = std::numeric_limits<std::uint32_t>::max();
            std::tuple<std::int32_t, std::uint16_t, std::uint16_t, std::uint32_t>
                nearestRank;
            for (std::uint32_t cell = 0; cell < cellCount; ++cell) {
                if ((cell & 255U) == 0U && deadline_expired()) {
                    return false;
                }
                const std::uint32_t id = state_id(mask, static_cast<CellId>(cell));
                if (distance.at(id) == kUnreachable ||
                    (excludeTarget && static_cast<CellId>(cell) == targetCell)) {
                    continue;
                }
                const auto rank = std::tuple{
                    static_cast<std::int32_t>(terminalDistances.at(cell)),
                    patrolFuel.at(id),
                    distance.at(id),
                    id};
                if (nearest == std::numeric_limits<std::uint32_t>::max() ||
                    rank < nearestRank) {
                    nearest = id;
                    nearestRank = rank;
                }
            }
            add_candidate(nearest);
            return true;
        };
        for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
            if (!add_nearest_terminal(
                    config.spots.at(spot).position,
                    spotTerminalDistances.at(spot),
                    false)) {
                return result;
            }
        }
        for (const auto& [targetCell, terminalDistances] :
             tankerTerminalDistances) {
            if (!add_nearest_terminal(
                    targetCell,
                    terminalDistances,
                    true)) {
                return result;
            }
        }
        std::sort(candidateStates.begin(), candidateStates.end());
        candidateStates.erase(
            std::unique(candidateStates.begin(), candidateStates.end()),
            candidateStates.end());
        for (std::size_t witnessOffset = 0;
             witnessOffset < candidateStates.size();
             ++witnessOffset) {
            if ((witnessOffset & 15U) == 0U && deadline_expired()) {
                return result;
            }
            const std::uint32_t witness = candidateStates.at(witnessOffset);
            ExactOrienteeringRoute route =
                reconstruct_high_fuel_route(witness, mask);
            if (witness == fastest) {
                result.maximalRoutes.push_back(std::move(route));
            } else {
                result.terminalVariants.push_back(std::move(route));
            }
        }
    }
    result.servedSpotFuelRoutes.reserve(config.spots.size());
    for (std::size_t targetSpot = 0;
         targetSpot < config.spots.size();
         ++targetSpot) {
        if (deadline_expired()) {
            return result;
        }
        const CellId targetCell = config.spots.at(targetSpot).position;
        const std::uint32_t targetBit =
            std::uint32_t{1} << static_cast<std::uint32_t>(targetSpot);
        std::uint32_t best = std::numeric_limits<std::uint32_t>::max();
        std::tuple<
            std::int32_t,
            std::int32_t,
            std::int32_t,
            std::int32_t,
            std::int32_t,
            std::uint32_t,
            std::uint32_t> bestRank;
        for (std::uint32_t mask = 1U; mask < maskCount; ++mask) {
            if ((mask & 1023U) == 0U && deadline_expired()) {
                return result;
            }
            if ((mask & targetBit) == 0U) {
                continue;
            }
            const std::uint32_t id = state_id(mask, targetCell);
            if (distance.at(id) == kUnreachable) {
                continue;
            }
            BrandMask brands;
            std::int32_t servingPotential = 0;
            for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
                if ((mask & (std::uint32_t{1} << spot)) == 0U) {
                    continue;
                }
                brands |= brand_bit(config.spots.at(spot).brandIndex);
                servingPotential += config.spots.at(spot).stock > 0 ? 1 : 0;
            }
            const auto rank = std::tuple{
                static_cast<std::int32_t>(patrolFuel.at(id)),
                -brand_count(brands),
                -servingPotential,
                -static_cast<std::int32_t>(std::popcount(mask)),
                static_cast<std::int32_t>(distance.at(id)),
                mask,
                id};
            if (best == std::numeric_limits<std::uint32_t>::max() ||
                rank < bestRank) {
                best = id;
                bestRank = rank;
            }
        }
        if (best != std::numeric_limits<std::uint32_t>::max()) {
            result.servedSpotFuelRoutes.push_back(
                reconstruct_high_fuel_route(best, best / cellCount));
        }
    }
    result.complete = true;
    return result;
}

namespace {

struct SparseResourceLabel {
    std::uint32_t mask = 0U;
    CellId cell = kInvalidCell;
    std::uint16_t usedSteps = 0U;
    std::uint16_t usedFuel = 0U;
    std::uint32_t parent = std::numeric_limits<std::uint32_t>::max();
    std::int32_t nextAtState = -1;
    std::uint8_t incoming = std::numeric_limits<std::uint8_t>::max();
    bool active = true;
};

using SparseRouteRank = std::tuple<
    std::int32_t,
    std::int32_t,
    std::int32_t,
    std::int32_t,
    std::int32_t,
    std::int32_t,
    std::int32_t,
    std::int32_t,
    std::uint32_t>;

struct SparseRouteChoice {
    SparseRouteRank rank;
    std::uint32_t label = 0U;
};

using SparseTerminalMarginalRank = std::tuple<
    std::int32_t,
    std::int32_t,
    std::int32_t,
    SparseRouteRank>;

struct SparseTerminalMarginalChoice {
    SparseTerminalMarginalRank rank;
    std::uint32_t label = 0U;
};

ExactOrienteeringReachability enumerate_sparse_anytime_resource_routes_impl(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agentIndex,
    std::int32_t minimumSpots,
    std::size_t maximumRoutes,
    std::uint64_t maximumSettledStates,
    BrandMask preferredBrands,
    std::optional<CellId> requiredTerminal,
    std::optional<std::chrono::steady_clock::time_point> deadline,
    const TerminalMarginalRouteContext* terminalMarginalContext) {
    ExactOrienteeringReachability result;
    if (agentIndex < 0 ||
        agentIndex >= static_cast<AgentIndex>(state.agents.size()) ||
        state.dayNumber < 1 || state.dayNumber > config.day_count() ||
        state.roadStatuses.size() !=
            static_cast<std::size_t>(config.map.cell_count()) ||
        config.spots.empty() || config.spots.size() > 32U ||
        minimumSpots <= 0 || maximumRoutes == 0U ||
        (requiredTerminal.has_value() &&
         (*requiredTerminal < 0 ||
          *requiredTerminal >= config.map.cell_count() ||
          config.map.terrain.at(
              static_cast<std::size_t>(*requiredTerminal)) == Terrain::Pond)) ||
        maximumSettledStates == 0U) {
        return result;
    }
    const AgentState& agent = state.agents.at(
        static_cast<std::size_t>(agentIndex));
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    if (agent.kind != AgentKind::Patrol || daySteps <= 0 ||
        daySteps >= kUnreachable || agent.fuel < 0 ||
        agent.fuel >= kUnreachable ||
        daySteps > static_cast<std::int32_t>(
            std::numeric_limits<std::uint16_t>::max()) ||
        agent.fuel > static_cast<std::int32_t>(
            std::numeric_limits<std::uint16_t>::max())) {
        return result;
    }
    result.supported = true;
    const auto deadline_expired = [&deadline]() {
        return deadline.has_value() &&
            std::chrono::steady_clock::now() >= *deadline;
    };
    if (deadline_expired()) {
        return result;
    }

    const auto state_key = [](std::uint32_t mask, CellId cell) {
        return (static_cast<std::uint64_t>(mask) << 32U) |
            static_cast<std::uint32_t>(cell);
    };
    using QueueEntry = std::tuple<
        std::uint8_t,
        std::uint16_t,
        std::uint16_t,
        std::uint32_t>;
    std::priority_queue<
        QueueEntry,
        std::vector<QueueEntry>,
        std::greater<>> queue;
    std::unordered_map<std::uint64_t, std::int32_t> firstLabelAtState;
    firstLabelAtState.reserve(
        static_cast<std::size_t>(std::min<std::uint64_t>(
            maximumSettledStates,
            1U << 20U)));
    std::vector<SparseResourceLabel> labels;
    labels.reserve(
        static_cast<std::size_t>(std::min<std::uint64_t>(
            maximumSettledStates,
            1U << 20U)));
    const auto relax = [&firstLabelAtState,
                        &labels,
                        &queue,
                        &state_key](
                           std::uint32_t mask,
                           CellId cell,
                           std::uint16_t candidateSteps,
                           std::uint16_t candidateFuel,
                           std::uint32_t parent,
                           std::uint8_t incoming) {
        const std::uint64_t key = state_key(mask, cell);
        const auto iterator = firstLabelAtState.find(key);
        const std::int32_t first = iterator == firstLabelAtState.end()
            ? -1
            : iterator->second;
        for (std::int32_t labelIndex = first;
             labelIndex >= 0;
             labelIndex = labels.at(
                 static_cast<std::size_t>(labelIndex)).nextAtState) {
            const SparseResourceLabel& existing = labels.at(
                static_cast<std::size_t>(labelIndex));
            if (existing.active &&
                existing.usedSteps <= candidateSteps &&
                existing.usedFuel <= candidateFuel) {
                return;
            }
        }
        for (std::int32_t labelIndex = first;
             labelIndex >= 0;
             labelIndex = labels.at(
                 static_cast<std::size_t>(labelIndex)).nextAtState) {
            SparseResourceLabel& existing = labels.at(
                static_cast<std::size_t>(labelIndex));
            if (existing.active &&
                candidateSteps <= existing.usedSteps &&
                candidateFuel <= existing.usedFuel) {
                existing.active = false;
            }
        }
        const std::uint32_t labelIndex = static_cast<std::uint32_t>(
            labels.size());
        labels.push_back(SparseResourceLabel{
            mask,
            cell,
            candidateSteps,
            candidateFuel,
            parent,
            first,
            incoming,
            true,
        });
        firstLabelAtState[key] = static_cast<std::int32_t>(labelIndex);
        queue.emplace(
            static_cast<std::uint8_t>(32U - std::popcount(mask)),
            candidateSteps,
            candidateFuel,
            labelIndex);
    };

    relax(
        0U,
        agent.position,
        0U,
        0U,
        std::numeric_limits<std::uint32_t>::max(),
        std::numeric_limits<std::uint8_t>::max());
    const std::uint32_t rootLabel = 0U;
    const SpotIndex startSpot = config.spotAtCell.at(
        static_cast<std::size_t>(agent.position));
    if (startSpot != kInvalidSpot && daySteps >= 1) {
        relax(
            std::uint32_t{1} << static_cast<std::uint32_t>(startSpot),
            agent.position,
            1U,
            0U,
            rootLabel,
            static_cast<std::uint8_t>(kDirectionCount));
    }

    const std::uint32_t cellCount = static_cast<std::uint32_t>(
        config.map.cell_count());
    std::vector<MoveCost> moveCosts(cellCount);
    std::vector<std::uint32_t> destinationSpotBits(cellCount, 0U);
    for (std::uint32_t cell = 0U; cell < cellCount; ++cell) {
        moveCosts.at(cell) = config.move_cost(
            static_cast<CellId>(cell),
            state.roadStatuses.at(cell));
        const SpotIndex spot = config.spotAtCell.at(cell);
        if (spot != kInvalidSpot) {
            destinationSpotBits.at(cell) =
                std::uint32_t{1} << static_cast<std::uint32_t>(spot);
        }
    }
    const auto route_rank = [&config, preferredBrands](
                                const SparseResourceLabel& label) {
        BrandMask brands;
        std::int32_t servingPotential = 0;
        for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
            if ((label.mask &
                 (std::uint32_t{1} << static_cast<std::uint32_t>(spot))) ==
                0U) {
                continue;
            }
            brands |= brand_bit(config.spots.at(spot).brandIndex);
            servingPotential += config.spots.at(spot).stock > 0 ? 1 : 0;
        }
        std::int32_t terminalBrandDistance = 0;
        for (std::int32_t brand = 0;
             brand < config.brand_count();
             ++brand) {
            std::int32_t nearest = std::numeric_limits<std::int32_t>::max();
            for (const Spot& spot : config.spots) {
                if (spot.brandIndex == brand) {
                    nearest = std::min(
                        nearest,
                        config.map.hex_distance(label.cell, spot.position));
                }
            }
            if (nearest != std::numeric_limits<std::int32_t>::max()) {
                terminalBrandDistance += nearest;
            }
        }
        return SparseRouteRank{
            brand_intersection_count(brands, preferredBrands),
            brand_count(brands),
            servingPotential,
            static_cast<std::int32_t>(std::popcount(label.mask)),
            -static_cast<std::int32_t>(label.usedSteps),
            -static_cast<std::int32_t>(label.usedFuel),
            -terminalBrandDistance,
            -label.cell,
             std::numeric_limits<std::uint32_t>::max() - label.mask};
    };
    const bool terminalMarginalEnabled =
        terminalMarginalContext != nullptr &&
        terminalMarginalContext->incumbentClaimsWithoutAgent.size() ==
            config.spots.size();
    const auto terminal_marginal_rank =
        [&config,
         terminalMarginalContext,
         &route_rank](const SparseResourceLabel& label) {
            BrandMask dayBrands;
            std::int32_t servings = 0;
            for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
                const std::int32_t claims =
                    terminalMarginalContext->incumbentClaimsWithoutAgent.at(spot) +
                    (((label.mask &
                       (std::uint32_t{1} << static_cast<std::uint32_t>(spot))) !=
                      0U)
                         ? 1
                         : 0);
                if (claims <= 0) {
                    continue;
                }
                dayBrands |= brand_bit(config.spots.at(spot).brandIndex);
                servings += std::min(config.spots.at(spot).stock, claims);
            }
            return SparseTerminalMarginalRank{
                brand_count(
                    terminalMarginalContext->lifetimeBrands | dayBrands),
                brand_count(dayBrands),
                servings,
                route_rank(label)};
        };
    const auto retain = [maximumRoutes](
                            std::vector<SparseRouteChoice>& choices,
                            SparseRouteChoice candidate) {
        if (choices.size() < maximumRoutes) {
            choices.push_back(std::move(candidate));
            return;
        }
        const auto worst = std::min_element(
            choices.begin(),
            choices.end(),
            [](const SparseRouteChoice& left, const SparseRouteChoice& right) {
                return left.rank < right.rank;
            });
        if (worst->rank < candidate.rank) {
            *worst = std::move(candidate);
        }
    };

    std::unordered_set<std::uint32_t> emittedMasks;
    emittedMasks.reserve(1U << 16U);
    std::vector<SparseRouteChoice> retained;
    std::vector<SparseRouteChoice> supplemental;
    std::vector<SparseTerminalMarginalChoice> terminalMarginal;
    retained.reserve(maximumRoutes);
    supplemental.reserve(maximumRoutes);
    terminalMarginal.reserve(maximumRoutes);
    std::uint64_t processedEntries = 0U;
    while (!queue.empty() && result.settledStates < maximumSettledStates) {
        if ((processedEntries++ & 4095U) == 0U && deadline_expired()) {
            break;
        }
        const auto [
            queuedCardinality,
            queuedSteps,
            queuedFuel,
            labelIndex] = queue.top();
        queue.pop();
        static_cast<void>(queuedCardinality);
        const SparseResourceLabel current = labels.at(
            static_cast<std::size_t>(labelIndex));
        if (!current.active || current.usedSteps != queuedSteps ||
            current.usedFuel != queuedFuel) {
            continue;
        }
        ++result.settledStates;
        const bool eligibleTerminal = requiredTerminal.has_value()
            ? current.cell == *requiredTerminal
            : config.spotAtCell.at(static_cast<std::size_t>(current.cell)) !=
                kInvalidSpot;
        if (static_cast<std::int32_t>(std::popcount(current.mask)) >=
                minimumSpots &&
            eligibleTerminal &&
            emittedMasks.insert(current.mask).second) {
            const SparseRouteRank rank = route_rank(current);
            retain(retained, SparseRouteChoice{rank, labelIndex});
            if (preferredBrands != 0U) {
                retain(supplemental, SparseRouteChoice{rank, labelIndex});
            }
            if (terminalMarginalEnabled) {
                const SparseTerminalMarginalChoice candidate{
                    terminal_marginal_rank(current),
                    labelIndex};
                if (terminalMarginal.size() < maximumRoutes) {
                    terminalMarginal.push_back(candidate);
                } else {
                    const auto worst = std::min_element(
                        terminalMarginal.begin(),
                        terminalMarginal.end(),
                        [](const SparseTerminalMarginalChoice& left,
                           const SparseTerminalMarginalChoice& right) {
                            return left.rank < right.rank;
                        });
                    if (worst->rank < candidate.rank) {
                        *worst = candidate;
                    }
                }
            }
        }

        const MoveCost move = moveCosts.at(
            static_cast<std::size_t>(current.cell));
        const std::int32_t nextSteps =
            static_cast<std::int32_t>(current.usedSteps) + move.steps;
        const std::int32_t nextFuel =
            static_cast<std::int32_t>(current.usedFuel) + move.patrolFuel;
        if (nextSteps > daySteps || nextFuel > agent.fuel) {
            continue;
        }
        for (std::int32_t direction = 0;
             direction < kDirectionCount;
             ++direction) {
            const CellId destination = config.map.neighbors
                .at(static_cast<std::size_t>(current.cell))
                .at(static_cast<std::size_t>(direction));
            if (destination == kInvalidCell ||
                config.map.terrain.at(static_cast<std::size_t>(destination)) ==
                    Terrain::Pond) {
                continue;
            }
            relax(
                current.mask | destinationSpotBits.at(
                    static_cast<std::size_t>(destination)),
                destination,
                static_cast<std::uint16_t>(nextSteps),
                static_cast<std::uint16_t>(nextFuel),
                labelIndex,
                static_cast<std::uint8_t>(direction));
        }
    }

    const auto reconstruct = [&config, &labels, rootLabel, daySteps](
                                 std::uint32_t witness) {
        const SparseResourceLabel& terminal = labels.at(
            static_cast<std::size_t>(witness));
        ExactOrienteeringRoute route;
        route.spotMask = terminal.mask;
        route.usedSteps = terminal.usedSteps;
        route.patrolFuel = terminal.usedFuel;
        route.terminalCell = terminal.cell;
        route.terminalOnSpot =
            config.spotAtCell.at(static_cast<std::size_t>(terminal.cell)) !=
            kInvalidSpot;
        std::vector<PlanAction> reversed;
        std::uint32_t current = witness;
        while (current != rootLabel) {
            const SparseResourceLabel& label = labels.at(
                static_cast<std::size_t>(current));
            if (label.incoming == static_cast<std::uint8_t>(kDirectionCount)) {
                reversed.push_back(PlanAction::wait(1));
            } else if (label.incoming <
                       static_cast<std::uint8_t>(kDirectionCount)) {
                reversed.push_back(PlanAction::move(label.incoming));
            } else {
                throw std::runtime_error(
                    "sparse resource predecessor chain is broken");
            }
            current = label.parent;
        }
        std::reverse(reversed.begin(), reversed.end());
        if (route.usedSteps < daySteps) {
            reversed.push_back(PlanAction::wait(daySteps - route.usedSteps));
        }
        route.actions = std::move(reversed);
        return route;
    };
    const auto append_choices = [&reconstruct](
                                    std::vector<SparseRouteChoice>& choices,
                                    std::vector<ExactOrienteeringRoute>& routes) {
        std::sort(
            choices.begin(),
            choices.end(),
            [](const SparseRouteChoice& left, const SparseRouteChoice& right) {
                return left.rank > right.rank;
            });
        routes.reserve(choices.size());
        for (const SparseRouteChoice& choice : choices) {
            routes.push_back(reconstruct(choice.label));
        }
    };
    append_choices(retained, result.maximalRoutes);
    append_choices(supplemental, result.supplementalRoutes);
    std::sort(
        terminalMarginal.begin(),
        terminalMarginal.end(),
        [](const SparseTerminalMarginalChoice& left,
           const SparseTerminalMarginalChoice& right) {
            return left.rank > right.rank;
        });
    result.terminalMarginalRoutes.reserve(terminalMarginal.size());
    for (const SparseTerminalMarginalChoice& choice : terminalMarginal) {
        result.terminalMarginalRoutes.push_back(reconstruct(choice.label));
    }
    return result;
}

ExactOrienteeringReachability enumerate_resource_routes(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agentIndex,
    std::optional<std::int32_t> minimumSpots,
    std::size_t maximumRoutes,
    std::uint64_t maximumSettledStates,
    BrandMask preferredBrands,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
    ExactOrienteeringReachability result;
    if (agentIndex < 0 ||
        agentIndex >= static_cast<AgentIndex>(state.agents.size()) ||
        state.dayNumber < 1 || state.dayNumber > config.day_count() ||
        state.roadStatuses.size() !=
            static_cast<std::size_t>(config.map.cell_count())) {
        return result;
    }
    const AgentState& agent =
        state.agents.at(static_cast<std::size_t>(agentIndex));
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    if (agent.kind != AgentKind::Patrol || daySteps <= 0 ||
        daySteps >= kUnreachable || agent.fuel < 0 ||
        agent.fuel >= kUnreachable || config.spots.size() > 16U ||
        daySteps > static_cast<std::int32_t>(
            std::numeric_limits<std::uint16_t>::max()) ||
        agent.fuel > static_cast<std::int32_t>(
            std::numeric_limits<std::uint16_t>::max())) {
        return result;
    }
    if (!minimumSpots.has_value() &&
        static_cast<std::int64_t>(agent.fuel) >=
            2LL * static_cast<std::int64_t>(daySteps)) {
        return enumerate_exact_high_fuel_routes(
            config,
            state,
            agentIndex,
            deadline);
    }

    const std::uint32_t maskCount = std::uint32_t{1} <<
        static_cast<std::uint32_t>(config.spots.size());
    const std::uint32_t cellCount =
        static_cast<std::uint32_t>(config.map.cell_count());
    const std::size_t stateCount =
        static_cast<std::size_t>(maskCount) * cellCount;
    if (cellCount == 0U || stateCount > kMaximumExactStates) {
        return result;
    }
    result.supported = true;
    const auto deadline_expired = [&deadline]() {
        return deadline.has_value() &&
            std::chrono::steady_clock::now() >= *deadline;
    };
    if (deadline_expired()) {
        return result;
    }

    const auto state_id = [cellCount](std::uint32_t mask, CellId cell) {
        return mask * cellCount + static_cast<std::uint32_t>(cell);
    };
    const bool cardinalityFirst = minimumSpots.has_value();
    using QueueEntry = std::tuple<
        std::uint8_t,
        std::uint16_t,
        std::uint16_t,
        std::uint32_t>;
    std::priority_queue<
        QueueEntry,
        std::vector<QueueEntry>,
        std::greater<>> queue;
    std::vector<std::int32_t> firstLabelAtState(stateCount, -1);
    std::vector<ResourceLabel> labels;
    labels.reserve(std::min<std::size_t>(stateCount, 1U << 20U));
    const auto relax =
        [&firstLabelAtState,
         &labels,
         &queue,
         cardinalityFirst,
         cellCount](
            std::uint32_t stateId,
            std::uint16_t candidateSteps,
            std::uint16_t candidateFuel,
            std::uint32_t parent,
            std::uint8_t incoming) {
            for (std::int32_t labelIndex = firstLabelAtState.at(stateId);
                 labelIndex >= 0;
                 labelIndex = labels.at(
                     static_cast<std::size_t>(labelIndex)).nextAtState) {
                const ResourceLabel& existing =
                    labels.at(static_cast<std::size_t>(labelIndex));
                if (existing.active &&
                    existing.usedSteps <= candidateSteps &&
                    existing.usedFuel <= candidateFuel) {
                    return;
                }
            }
            for (std::int32_t labelIndex = firstLabelAtState.at(stateId);
                 labelIndex >= 0;
                 labelIndex = labels.at(
                     static_cast<std::size_t>(labelIndex)).nextAtState) {
                ResourceLabel& existing =
                    labels.at(static_cast<std::size_t>(labelIndex));
                if (existing.active &&
                    candidateSteps <= existing.usedSteps &&
                    candidateFuel <= existing.usedFuel) {
                    existing.active = false;
                }
            }
            const std::uint32_t labelIndex =
                static_cast<std::uint32_t>(labels.size());
            ResourceLabel label;
            label.state = stateId;
            label.usedSteps = candidateSteps;
            label.usedFuel = candidateFuel;
            label.parent = parent;
            label.nextAtState = firstLabelAtState.at(stateId);
            label.incoming = incoming;
            labels.push_back(label);
            firstLabelAtState.at(stateId) =
                static_cast<std::int32_t>(labelIndex);
            const std::uint8_t cardinalityPriority = cardinalityFirst
                ? static_cast<std::uint8_t>(
                    16U - std::popcount(stateId / cellCount))
                : 0U;
            queue.emplace(
                cardinalityPriority,
                candidateSteps,
                candidateFuel,
                labelIndex);
        };

    const std::uint32_t rootState = state_id(0U, agent.position);
    relax(
        rootState,
        0U,
        0U,
        std::numeric_limits<std::uint32_t>::max(),
        std::numeric_limits<std::uint8_t>::max());
    const std::uint32_t rootLabel = 0U;
    const SpotIndex startSpot =
        config.spotAtCell.at(static_cast<std::size_t>(agent.position));
    if (startSpot != kInvalidSpot && daySteps >= 1) {
        relax(
            state_id(
                std::uint32_t{1} << static_cast<std::uint32_t>(startSpot),
                agent.position),
            1U,
            0U,
            rootLabel,
            static_cast<std::uint8_t>(kDirectionCount));
    }

    std::vector<bool> reachableMasks(maskCount, false);
    std::unordered_set<std::uint32_t> anytimeMasks;
    std::vector<MoveCost> moveCosts(cellCount);
    std::vector<std::uint32_t> destinationSpotBits(cellCount, 0U);
    std::vector<BrandMask> brandMaskBySpotMask;
    std::vector<std::int32_t> servingPotentialBySpotMask;
    if (preferredBrands != 0U) {
        brandMaskBySpotMask.assign(maskCount, BrandMask{});
        servingPotentialBySpotMask.assign(maskCount, 0);
        for (std::uint32_t mask = 1U; mask < maskCount; ++mask) {
            const std::uint32_t bit = std::countr_zero(mask);
            const std::uint32_t prior = mask & (mask - 1U);
            const Spot& spot = config.spots.at(bit);
            brandMaskBySpotMask.at(mask) =
                brandMaskBySpotMask.at(prior) |
                brand_bit(spot.brandIndex);
            servingPotentialBySpotMask.at(mask) =
                servingPotentialBySpotMask.at(prior) +
                (spot.stock > 0 ? 1 : 0);
        }
    }
    for (std::uint32_t cell = 0; cell < cellCount; ++cell) {
        moveCosts.at(cell) = config.move_cost(
            static_cast<CellId>(cell),
            state.roadStatuses.at(cell));
        const SpotIndex spot = config.spotAtCell.at(cell);
        if (spot != kInvalidSpot) {
            destinationSpotBits.at(cell) =
                std::uint32_t{1} << static_cast<std::uint32_t>(spot);
        }
    }

    const auto reconstruct_route =
        [&config, &labels, cellCount, daySteps, rootLabel](
            std::uint32_t witness,
            std::uint32_t mask) {
            const ResourceLabel& terminal = labels.at(witness);
            ExactOrienteeringRoute route;
            route.spotMask = mask;
            route.usedSteps = terminal.usedSteps;
            route.patrolFuel = terminal.usedFuel;
            route.terminalCell =
                static_cast<CellId>(terminal.state % cellCount);
            route.terminalOnSpot =
                config.spotAtCell.at(
                    static_cast<std::size_t>(route.terminalCell)) !=
                kInvalidSpot;
            for (std::int32_t brand = 0;
                 brand < config.brand_count();
                 ++brand) {
                std::int32_t nearestBrand =
                    std::numeric_limits<std::int32_t>::max();
                for (const Spot& spot : config.spots) {
                    if (spot.brandIndex == brand) {
                        nearestBrand = std::min(
                            nearestBrand,
                            config.map.hex_distance(
                                route.terminalCell,
                                spot.position));
                    }
                }
                if (nearestBrand !=
                    std::numeric_limits<std::int32_t>::max()) {
                    route.terminalBrandDistance += nearestBrand;
                }
            }
            std::vector<PlanAction> reversed;
            std::uint32_t current = witness;
            while (current != rootLabel) {
                const ResourceLabel& label = labels.at(current);
                if (label.incoming ==
                    static_cast<std::uint8_t>(kDirectionCount)) {
                    reversed.push_back(PlanAction::wait(1));
                } else if (label.incoming <
                           static_cast<std::uint8_t>(kDirectionCount)) {
                    reversed.push_back(
                        PlanAction::move(label.incoming));
                } else {
                    throw std::runtime_error(
                        "resource orienteering predecessor chain is broken");
                }
                current = label.parent;
            }
            std::reverse(reversed.begin(), reversed.end());
            if (route.usedSteps < daySteps) {
                reversed.push_back(
                    PlanAction::wait(daySteps - route.usedSteps));
            }
            route.actions = std::move(reversed);
            return route;
        };

    std::uint64_t processedQueueEntries = 0U;
    while (!queue.empty()) {
        if ((processedQueueEntries++ & 4095U) == 0U &&
            deadline_expired()) {
            return result;
        }
        const auto [
            queuedCardinality,
            queuedSteps,
            queuedFuel,
            labelIndex] = queue.top();
        queue.pop();
        static_cast<void>(queuedCardinality);
        const ResourceLabel current =
            labels.at(static_cast<std::size_t>(labelIndex));
        if (!current.active || current.usedSteps != queuedSteps ||
            current.usedFuel != queuedFuel) {
            continue;
        }
        ++result.settledStates;
        const std::uint32_t mask = current.state / cellCount;
        const CellId cell =
            static_cast<CellId>(current.state % cellCount);
        reachableMasks.at(mask) = true;
        if (minimumSpots.has_value() &&
            static_cast<std::int32_t>(std::popcount(mask)) >=
                *minimumSpots &&
            config.spotAtCell.at(static_cast<std::size_t>(cell)) !=
                kInvalidSpot &&
            anytimeMasks.insert(mask).second) {
            std::int32_t terminalBrandDistance = 0;
            for (std::int32_t brand = 0;
                 brand < config.brand_count();
                 ++brand) {
                std::int32_t nearestBrand =
                    std::numeric_limits<std::int32_t>::max();
                for (const Spot& spot : config.spots) {
                    if (spot.brandIndex == brand) {
                        nearestBrand = std::min(
                            nearestBrand,
                            config.map.hex_distance(cell, spot.position));
                    }
                }
                if (nearestBrand !=
                    std::numeric_limits<std::int32_t>::max()) {
                    terminalBrandDistance += nearestBrand;
                }
            }
            const auto lexicographic_rank =
                [&brandMaskBySpotMask,
                 &servingPotentialBySpotMask,
                 preferredBrands](
                    std::uint32_t routeMask,
                    std::int32_t usedSteps,
                    std::int32_t usedFuel,
                    std::int32_t brandDistance,
                    CellId terminalCell) {
                    const BrandMask brands =
                        brandMaskBySpotMask.at(routeMask);
                    return std::tuple{
                        brand_intersection_count(brands, preferredBrands),
                        brand_count(brands),
                        servingPotentialBySpotMask.at(routeMask),
                        static_cast<std::int32_t>(
                            std::popcount(routeMask)),
                        -usedSteps,
                        -usedFuel,
                        -brandDistance,
                        -terminalCell,
                        -static_cast<std::int32_t>(routeMask)};
                };
            const auto legacy_rank =
                [](const ExactOrienteeringRoute& route) {
                    return std::tuple{
                        static_cast<std::int32_t>(
                            std::popcount(route.spotMask)),
                        -route.usedSteps,
                        -route.patrolFuel,
                        -route.terminalBrandDistance,
                        -route.terminalCell,
                        -static_cast<std::int32_t>(route.spotMask)};
                };
            const auto legacyCandidateRank = std::tuple{
                static_cast<std::int32_t>(std::popcount(mask)),
                -static_cast<std::int32_t>(current.usedSteps),
                -static_cast<std::int32_t>(current.usedFuel),
                -terminalBrandDistance,
                -cell,
                -static_cast<std::int32_t>(mask)};
            if (result.maximalRoutes.size() < maximumRoutes) {
                result.maximalRoutes.push_back(
                    reconstruct_route(labelIndex, mask));
            } else {
                const auto worst = std::min_element(
                    result.maximalRoutes.begin(),
                    result.maximalRoutes.end(),
                    [&legacy_rank](
                        const ExactOrienteeringRoute& left,
                        const ExactOrienteeringRoute& right) {
                        return legacy_rank(left) < legacy_rank(right);
                    });
                if (legacy_rank(*worst) < legacyCandidateRank) {
                    *worst = reconstruct_route(labelIndex, mask);
                }
            }
            if (preferredBrands != 0U) {
                const auto candidateRank = lexicographic_rank(
                    mask,
                    current.usedSteps,
                    current.usedFuel,
                    terminalBrandDistance,
                    cell);
                if (result.supplementalRoutes.size() < maximumRoutes) {
                    result.supplementalRoutes.push_back(
                        reconstruct_route(labelIndex, mask));
                } else {
                    std::vector<std::int32_t> retainedBrandCounts(
                        static_cast<std::size_t>(config.brand_count()),
                        0);
                    BrandMask retainedBrands;
                    for (const ExactOrienteeringRoute& route :
                         result.supplementalRoutes) {
                        const BrandMask brands =
                            brandMaskBySpotMask.at(route.spotMask);
                        retainedBrands |= brands;
                        for (std::int32_t brand = 0;
                             brand < config.brand_count();
                             ++brand) {
                            if (has_brand(brands, brand)) {
                                ++retainedBrandCounts.at(
                                    static_cast<std::size_t>(brand));
                            }
                        }
                    }
                    const BrandMask candidateBrands =
                        brandMaskBySpotMask.at(mask);
                    const bool expandsCoverage =
                        brand_difference(candidateBrands, retainedBrands).any();
                    auto worstSafe = result.supplementalRoutes.end();
                    for (auto existing =
                             result.supplementalRoutes.begin();
                         existing != result.supplementalRoutes.end();
                         ++existing) {
                        const BrandMask existingBrands =
                            brandMaskBySpotMask.at(existing->spotMask);
                        bool safe = true;
                        for (std::int32_t brand = 0;
                             brand < config.brand_count();
                             ++brand) {
                            if (has_brand(existingBrands, brand) &&
                                retainedBrandCounts.at(
                                    static_cast<std::size_t>(brand)) == 1 &&
                                !has_brand(candidateBrands, brand)) {
                                safe = false;
                                break;
                            }
                        }
                        if (!safe) {
                            continue;
                        }
                        if (worstSafe ==
                                result.supplementalRoutes.end() ||
                            lexicographic_rank(
                                existing->spotMask,
                                existing->usedSteps,
                                existing->patrolFuel,
                                existing->terminalBrandDistance,
                                existing->terminalCell) <
                                lexicographic_rank(
                                    worstSafe->spotMask,
                                    worstSafe->usedSteps,
                                    worstSafe->patrolFuel,
                                    worstSafe->terminalBrandDistance,
                                    worstSafe->terminalCell)) {
                            worstSafe = existing;
                        }
                    }
                    if (worstSafe !=
                            result.supplementalRoutes.end() &&
                        (expandsCoverage ||
                         lexicographic_rank(
                             worstSafe->spotMask,
                             worstSafe->usedSteps,
                             worstSafe->patrolFuel,
                             worstSafe->terminalBrandDistance,
                             worstSafe->terminalCell) < candidateRank)) {
                        *worstSafe =
                            reconstruct_route(labelIndex, mask);
                    } else if (worstSafe ==
                               result.supplementalRoutes.end()) {
                        const auto worst = std::min_element(
                            result.supplementalRoutes.begin(),
                            result.supplementalRoutes.end(),
                            [&lexicographic_rank](
                                const ExactOrienteeringRoute& left,
                                const ExactOrienteeringRoute& right) {
                                return lexicographic_rank(
                                           left.spotMask,
                                           left.usedSteps,
                                           left.patrolFuel,
                                           left.terminalBrandDistance,
                                           left.terminalCell) <
                                    lexicographic_rank(
                                           right.spotMask,
                                           right.usedSteps,
                                           right.patrolFuel,
                                           right.terminalBrandDistance,
                                           right.terminalCell);
                            });
                        if (lexicographic_rank(
                                worst->spotMask,
                                worst->usedSteps,
                                worst->patrolFuel,
                                worst->terminalBrandDistance,
                                worst->terminalCell) < candidateRank) {
                            *worst =
                                reconstruct_route(labelIndex, mask);
                        }
                    }
                }
            }
        }
        if (minimumSpots.has_value() &&
            result.settledStates >= maximumSettledStates) {
            return result;
        }

        const MoveCost move =
            moveCosts.at(static_cast<std::size_t>(cell));
        const std::int32_t candidateSteps =
            static_cast<std::int32_t>(current.usedSteps) + move.steps;
        const std::int32_t candidateFuel =
            static_cast<std::int32_t>(current.usedFuel) + move.patrolFuel;
        if (candidateSteps > daySteps || candidateFuel > agent.fuel) {
            continue;
        }
        for (std::int32_t direction = 0;
             direction < kDirectionCount;
             ++direction) {
            const CellId destination = config.map.neighbors
                .at(static_cast<std::size_t>(cell))
                .at(static_cast<std::size_t>(direction));
            if (destination == kInvalidCell ||
                config.map.terrain.at(
                    static_cast<std::size_t>(destination)) ==
                    Terrain::Pond) {
                continue;
            }
            relax(
                state_id(
                    mask | destinationSpotBits.at(
                        static_cast<std::size_t>(destination)),
                    destination),
                static_cast<std::uint16_t>(candidateSteps),
                static_cast<std::uint16_t>(candidateFuel),
                labelIndex,
                static_cast<std::uint8_t>(direction));
        }
    }

    bool maximalComplete = true;
    const std::vector<std::uint32_t> maximalMasks =
        inclusion_maximal_masks(
            reachableMasks,
            deadline,
            maximalComplete);
    if (!maximalComplete) {
        return result;
    }
    result.maximalRoutes.reserve(maximalMasks.size());
    result.terminalVariants.reserve(
        maximalMasks.size() * (config.spots.size() + 1U));
    for (std::size_t maskOffset = 0;
         maskOffset < maximalMasks.size();
         ++maskOffset) {
        if ((maskOffset & 15U) == 0U && deadline_expired()) {
            return result;
        }
        const std::uint32_t mask = maximalMasks.at(maskOffset);
        std::vector<std::uint32_t> candidateLabels;
        const auto add_candidate =
            [&candidateLabels](std::uint32_t labelIndex) {
                if (labelIndex !=
                    std::numeric_limits<std::uint32_t>::max()) {
                    candidateLabels.push_back(labelIndex);
                }
            };
        bool terminalDeadlineReached = false;
        const auto select_label =
            [&labels,
             &firstLabelAtState,
             cellCount,
             mask,
             &config,
             &deadline_expired,
             &terminalDeadlineReached](
                const std::function<std::tuple<
                    std::int32_t,
                    std::int32_t,
                    std::int32_t,
                    std::uint32_t>(
                    const ResourceLabel&,
                    CellId,
                    std::uint32_t)>& rank) {
                std::uint32_t best =
                    std::numeric_limits<std::uint32_t>::max();
                std::tuple<
                    std::int32_t,
                    std::int32_t,
                    std::int32_t,
                    std::uint32_t> bestRank;
                for (std::uint32_t cell = 0;
                     cell < cellCount;
                     ++cell) {
                    if ((cell & 255U) == 0U && deadline_expired()) {
                        terminalDeadlineReached = true;
                        return std::numeric_limits<std::uint32_t>::max();
                    }
                    const std::uint32_t id = mask * cellCount + cell;
                    for (std::int32_t labelIndex =
                             firstLabelAtState.at(id);
                         labelIndex >= 0;
                         labelIndex = labels.at(
                             static_cast<std::size_t>(labelIndex))
                                          .nextAtState) {
                        const ResourceLabel& label = labels.at(
                            static_cast<std::size_t>(labelIndex));
                        if (!label.active) {
                            continue;
                        }
                        const auto candidateRank = rank(
                            label,
                            static_cast<CellId>(cell),
                            static_cast<std::uint32_t>(labelIndex));
                        if (best ==
                                std::numeric_limits<std::uint32_t>::max() ||
                            candidateRank < bestRank) {
                            best = static_cast<std::uint32_t>(labelIndex);
                            bestRank = candidateRank;
                        }
                    }
                }
                return best;
            };
        const std::uint32_t fastest = select_label(
            [](const ResourceLabel& label,
               CellId,
               std::uint32_t labelIndex) {
                return std::tuple{
                    static_cast<std::int32_t>(label.usedSteps),
                    static_cast<std::int32_t>(label.usedFuel),
                    static_cast<std::int32_t>(label.state),
                    labelIndex};
            });
        if (terminalDeadlineReached) {
            return result;
        }
        add_candidate(fastest);
        add_candidate(select_label(
            [](const ResourceLabel& label,
               CellId,
               std::uint32_t labelIndex) {
                return std::tuple{
                    static_cast<std::int32_t>(label.usedFuel),
                    static_cast<std::int32_t>(label.usedSteps),
                    static_cast<std::int32_t>(label.state),
                    labelIndex};
            }));
        if (terminalDeadlineReached) {
            return result;
        }
        const auto add_nearest_terminal =
            [&select_label, &add_candidate, &config](
                CellId targetCell,
                bool excludeTarget) {
                add_candidate(select_label(
                    [&config, targetCell, excludeTarget](
                        const ResourceLabel& label,
                        CellId cell,
                        std::uint32_t labelIndex) {
                        return std::tuple{
                            excludeTarget && cell == targetCell
                                ? std::numeric_limits<std::int32_t>::max()
                                : config.map.hex_distance(
                                    cell,
                                    targetCell),
                            static_cast<std::int32_t>(label.usedFuel),
                            static_cast<std::int32_t>(label.usedSteps),
                            labelIndex};
                    }));
            };
        for (const Spot& target : config.spots) {
            add_nearest_terminal(target.position, false);
            if (terminalDeadlineReached) {
                return result;
            }
        }
        for (const AgentState& target : state.agents) {
            if (target.kind == AgentKind::Tanker) {
                add_nearest_terminal(target.position, true);
                if (terminalDeadlineReached) {
                    return result;
                }
            }
        }
        std::sort(candidateLabels.begin(), candidateLabels.end());
        candidateLabels.erase(
            std::unique(candidateLabels.begin(), candidateLabels.end()),
            candidateLabels.end());
        for (std::size_t witnessOffset = 0;
             witnessOffset < candidateLabels.size();
             ++witnessOffset) {
            if ((witnessOffset & 15U) == 0U && deadline_expired()) {
                return result;
            }
            const std::uint32_t witness = candidateLabels.at(witnessOffset);
            ExactOrienteeringRoute route =
                reconstruct_route(witness, mask);
            if (witness == fastest) {
                result.maximalRoutes.push_back(std::move(route));
            } else {
                result.terminalVariants.push_back(std::move(route));
            }
        }
    }
    result.servedSpotFuelRoutes.reserve(config.spots.size());
    for (std::size_t targetSpot = 0;
         targetSpot < config.spots.size();
         ++targetSpot) {
        if (deadline_expired()) {
            return result;
        }
        const CellId targetCell = config.spots.at(targetSpot).position;
        const std::uint32_t targetBit =
            std::uint32_t{1} << static_cast<std::uint32_t>(targetSpot);
        std::uint32_t best = std::numeric_limits<std::uint32_t>::max();
        std::tuple<
            std::int32_t,
            std::int32_t,
            std::int32_t,
            std::int32_t,
            std::int32_t,
            std::uint32_t,
            std::uint32_t> bestRank;
        for (std::uint32_t mask = 1U; mask < maskCount; ++mask) {
            if ((mask & 1023U) == 0U && deadline_expired()) {
                return result;
            }
            if ((mask & targetBit) == 0U) {
                continue;
            }
            const std::uint32_t id = state_id(mask, targetCell);
            BrandMask brands;
            std::int32_t servingPotential = 0;
            for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
                if ((mask & (std::uint32_t{1} << spot)) == 0U) {
                    continue;
                }
                brands |= brand_bit(config.spots.at(spot).brandIndex);
                servingPotential += config.spots.at(spot).stock > 0 ? 1 : 0;
            }
            for (std::int32_t labelIndex = firstLabelAtState.at(id);
                 labelIndex >= 0;
                 labelIndex = labels.at(static_cast<std::size_t>(labelIndex))
                                  .nextAtState) {
                const ResourceLabel& label = labels.at(
                    static_cast<std::size_t>(labelIndex));
                if (!label.active) {
                    continue;
                }
                const auto rank = std::tuple{
                    static_cast<std::int32_t>(label.usedFuel),
                    -brand_count(brands),
                    -servingPotential,
                    -static_cast<std::int32_t>(std::popcount(mask)),
                    static_cast<std::int32_t>(label.usedSteps),
                    mask,
                    static_cast<std::uint32_t>(labelIndex)};
                if (best == std::numeric_limits<std::uint32_t>::max() ||
                    rank < bestRank) {
                    best = static_cast<std::uint32_t>(labelIndex);
                    bestRank = rank;
                }
            }
        }
        if (best != std::numeric_limits<std::uint32_t>::max()) {
            const std::uint32_t mask =
                labels.at(static_cast<std::size_t>(best)).state / cellCount;
            result.servedSpotFuelRoutes.push_back(
                reconstruct_route(best, mask));
        }
    }
    result.complete = true;
    return result;
}

}

ExactOrienteeringReachability enumerate_exact_resource_routes(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agentIndex,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
    return enumerate_resource_routes(
        config,
        state,
        agentIndex,
        std::nullopt,
        0U,
        std::numeric_limits<std::uint64_t>::max(),
        0U,
        deadline);
}

ExactOrienteeringReachability enumerate_anytime_resource_routes(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agentIndex,
    std::int32_t minimumSpots,
    std::size_t maximumRoutes,
    std::uint64_t maximumSettledStates,
    std::optional<std::chrono::steady_clock::time_point> deadline,
    BrandMask preferredBrands) {
    if (minimumSpots <= 0 || maximumRoutes == 0U ||
        maximumSettledStates == 0U) {
        return {};
    }
    ExactOrienteeringReachability result = enumerate_resource_routes(
        config,
        state,
        agentIndex,
        minimumSpots,
        maximumRoutes,
        maximumSettledStates,
        preferredBrands,
        deadline);
    if (deadline.has_value() &&
        std::chrono::steady_clock::now() >= *deadline) {
        return result;
    }
    const auto route_brand_mask =
        [&config](std::uint32_t spotMask) {
            BrandMask brands;
            for (std::size_t spot = 0;
                 spot < config.spots.size();
                 ++spot) {
                if ((spotMask & (std::uint32_t{1} << spot)) != 0U) {
                    brands |= brand_bit(config.spots.at(spot).brandIndex);
                }
            }
            return brands;
        };
    const auto serving_potential =
        [&config](std::uint32_t spotMask) {
            std::int32_t servings = 0;
            for (std::size_t spot = 0;
                 spot < config.spots.size();
                 ++spot) {
                if ((spotMask & (std::uint32_t{1} << spot)) != 0U &&
                    config.spots.at(spot).stock > 0) {
                    ++servings;
                }
            }
            return servings;
        };
    const auto legacy_rank =
        [](const ExactOrienteeringRoute& route) {
            return std::tuple{
                static_cast<std::int32_t>(
                    std::popcount(route.spotMask)),
                -route.usedSteps,
                -route.patrolFuel,
                -route.terminalBrandDistance,
                -route.terminalCell,
                -static_cast<std::int32_t>(route.spotMask)};
        };
    const auto supplemental_rank =
        [&route_brand_mask,
         &serving_potential,
         preferredBrands](const ExactOrienteeringRoute& route) {
            const BrandMask brands =
                route_brand_mask(route.spotMask);
            return std::tuple{
                brand_intersection_count(brands, preferredBrands),
                brand_count(brands),
                serving_potential(route.spotMask),
                static_cast<std::int32_t>(
                    std::popcount(route.spotMask)),
                -route.usedSteps,
                -route.patrolFuel,
                -route.terminalBrandDistance,
                -route.terminalCell,
                -static_cast<std::int32_t>(route.spotMask)};
        };
    std::sort(
        result.maximalRoutes.begin(),
        result.maximalRoutes.end(),
        [&legacy_rank](const ExactOrienteeringRoute& left,
                       const ExactOrienteeringRoute& right) {
            return legacy_rank(left) > legacy_rank(right);
        });
    std::unordered_set<std::uint32_t> retainedMasks;
    std::erase_if(
        result.maximalRoutes,
        [&retainedMasks](const ExactOrienteeringRoute& route) {
            return !retainedMasks.insert(route.spotMask).second;
        });
    if (result.maximalRoutes.size() > maximumRoutes) {
        result.maximalRoutes.resize(maximumRoutes);
    }
    retainedMasks.clear();
    for (const ExactOrienteeringRoute& route : result.maximalRoutes) {
        retainedMasks.insert(route.spotMask);
    }
    std::sort(
        result.supplementalRoutes.begin(),
        result.supplementalRoutes.end(),
        [&supplemental_rank](
            const ExactOrienteeringRoute& left,
            const ExactOrienteeringRoute& right) {
            return supplemental_rank(left) >
                supplemental_rank(right);
        });
    std::unordered_set<std::uint32_t> supplementalMasks =
        retainedMasks;
    std::erase_if(
        result.supplementalRoutes,
        [&supplementalMasks](const ExactOrienteeringRoute& route) {
            return !supplementalMasks.insert(route.spotMask).second;
        });
    if (result.supplementalRoutes.size() > maximumRoutes) {
        result.supplementalRoutes.resize(maximumRoutes);
    }
    result.complete = false;
    return result;
}

ExactOrienteeringReachability enumerate_sparse_anytime_resource_routes(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agentIndex,
    std::int32_t minimumSpots,
    std::size_t maximumRoutes,
    std::uint64_t maximumSettledStates,
    std::optional<std::chrono::steady_clock::time_point> deadline,
    BrandMask preferredBrands,
    const TerminalMarginalRouteContext* terminalMarginalContext) {
    return enumerate_sparse_anytime_resource_routes_impl(
        config,
        state,
        agentIndex,
        minimumSpots,
        maximumRoutes,
        maximumSettledStates,
        preferredBrands,
        std::nullopt,
        deadline,
        terminalMarginalContext);
}

ExactOrienteeringReachability
enumerate_sparse_anytime_resource_routes_to_terminal(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agentIndex,
    CellId requiredTerminal,
    std::int32_t minimumSpots,
    std::size_t maximumRoutes,
    std::uint64_t maximumSettledStates,
    std::optional<std::chrono::steady_clock::time_point> deadline,
    BrandMask preferredBrands,
    const TerminalMarginalRouteContext* terminalMarginalContext) {
    return enumerate_sparse_anytime_resource_routes_impl(
        config,
        state,
        agentIndex,
        minimumSpots,
        maximumRoutes,
        maximumSettledStates,
        preferredBrands,
        requiredTerminal,
        deadline,
        terminalMarginalContext);
}

}
