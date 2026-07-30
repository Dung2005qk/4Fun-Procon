#include "udon/orienteering.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace udon {

namespace {

constexpr std::uint16_t kUnreachable = std::numeric_limits<std::uint16_t>::max();
constexpr std::size_t kMaximumExactStates = 8U * 1024U * 1024U;

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
        config.spots.size() > 16U || agent.fuel < 2 * daySteps) {
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
    if (deadline.has_value() && std::chrono::steady_clock::now() >= *deadline) {
        return result;
    }

    std::vector<std::uint16_t> distance(stateCount, kUnreachable);
    std::vector<std::uint16_t> patrolFuel(stateCount, kUnreachable);
    std::vector<std::uint32_t> parent(
        stateCount,
        std::numeric_limits<std::uint32_t>::max());
    std::vector<std::uint8_t> incoming(
        stateCount,
        std::numeric_limits<std::uint8_t>::max());
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
    for (std::uint16_t usedSteps = 0;
         usedSteps <= static_cast<std::uint16_t>(daySteps);
         ++usedSteps) {
        std::vector<std::uint64_t>& bucket = buckets.at(usedSteps);
        for (std::size_t offset = 0; offset < bucket.size(); ++offset) {
            if ((result.settledStates & 4095U) == 0U && deadline.has_value() &&
                std::chrono::steady_clock::now() >= *deadline) {
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
    for (const std::uint32_t mask : maximalMasks) {
        std::vector<std::uint32_t> candidateStates;
        const auto add_candidate = [&candidateStates](std::uint32_t id) {
            if (id != std::numeric_limits<std::uint32_t>::max()) {
                candidateStates.push_back(id);
            }
        };
        std::uint32_t fastest = std::numeric_limits<std::uint32_t>::max();
        std::uint32_t lowestFuel = std::numeric_limits<std::uint32_t>::max();
        for (std::uint32_t cell = 0; cell < cellCount; ++cell) {
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
        for (const Spot& target : config.spots) {
            std::uint32_t nearest = std::numeric_limits<std::uint32_t>::max();
            for (std::uint32_t cell = 0; cell < cellCount; ++cell) {
                const std::uint32_t id = state_id(mask, static_cast<CellId>(cell));
                if (distance.at(id) == kUnreachable) {
                    continue;
                }
                const auto rank = std::tuple{
                    config.map.hex_distance(static_cast<CellId>(cell), target.position),
                    patrolFuel.at(id),
                    distance.at(id),
                    id};
                if (nearest == std::numeric_limits<std::uint32_t>::max()) {
                    nearest = id;
                    continue;
                }
                const CellId nearestCell = static_cast<CellId>(nearest % cellCount);
                const auto nearestRank = std::tuple{
                    config.map.hex_distance(nearestCell, target.position),
                    patrolFuel.at(nearest),
                    distance.at(nearest),
                    nearest};
                if (rank < nearestRank) {
                    nearest = id;
                }
            }
            add_candidate(nearest);
        }
        std::sort(candidateStates.begin(), candidateStates.end());
        candidateStates.erase(
            std::unique(candidateStates.begin(), candidateStates.end()),
            candidateStates.end());
        for (std::uint32_t witness : candidateStates) {
            ExactOrienteeringRoute route;
            route.spotMask = mask;
            route.usedSteps = distance.at(witness);
            route.patrolFuel = patrolFuel.at(witness);
            route.terminalCell = static_cast<CellId>(witness % cellCount);
            route.terminalOnSpot =
                config.spotAtCell.at(static_cast<std::size_t>(route.terminalCell)) !=
                kInvalidSpot;
            for (std::int32_t brand = 0; brand < config.brand_count(); ++brand) {
                std::int32_t nearestBrand = std::numeric_limits<std::int32_t>::max();
                for (const Spot& spot : config.spots) {
                    if (spot.brandIndex == brand) {
                        nearestBrand = std::min(
                            nearestBrand,
                            config.map.hex_distance(route.terminalCell, spot.position));
                    }
                }
                if (nearestBrand != std::numeric_limits<std::int32_t>::max()) {
                    route.terminalBrandDistance += nearestBrand;
                }
            }
            std::vector<PlanAction> reversed;
            std::uint32_t current = witness;
            while (current != root) {
                const std::uint8_t action = incoming.at(current);
                if (action == static_cast<std::uint8_t>(kDirectionCount)) {
                    reversed.push_back(PlanAction::wait(1));
                } else if (action < static_cast<std::uint8_t>(kDirectionCount)) {
                    reversed.push_back(PlanAction::move(action));
                } else {
                    throw std::runtime_error("exact orienteering predecessor chain is broken");
                }
                current = parent.at(current);
            }
            std::reverse(reversed.begin(), reversed.end());
            if (route.usedSteps < daySteps) {
                reversed.push_back(PlanAction::wait(daySteps - route.usedSteps));
            }
            route.actions = std::move(reversed);
            if (witness == fastest) {
                result.maximalRoutes.push_back(std::move(route));
            } else {
                result.terminalVariants.push_back(std::move(route));
            }
        }
    }
    result.complete = true;
    return result;
}

}
