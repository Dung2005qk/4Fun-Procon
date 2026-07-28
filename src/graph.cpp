#include "udon/graph.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <tuple>

namespace udon {

namespace {

struct Label {
    CellId cell = kInvalidCell;
    std::int32_t travelSteps = 0;
    std::int32_t patrolFuel = 0;
    std::int32_t parent = -1;
    std::int32_t incomingDirection = -1;
    std::vector<std::int32_t> criticalFootprint;
    bool active = true;
};

struct QueueItem {
    std::int32_t travelSteps = 0;
    std::int32_t patrolFuel = 0;
    std::int32_t labelIndex = -1;
};

struct QueueOrder {
    [[nodiscard]] bool operator()(const QueueItem& left, const QueueItem& right) const {
        return std::tie(left.travelSteps, left.patrolFuel, left.labelIndex) >
            std::tie(right.travelSteps, right.patrolFuel, right.labelIndex);
    }
};

[[nodiscard]] bool dominates(const Label& left, const Label& right) {
    if (left.travelSteps > right.travelSteps || left.patrolFuel > right.patrolFuel ||
        left.criticalFootprint.size() != right.criticalFootprint.size()) {
        return false;
    }
    bool strict = left.travelSteps < right.travelSteps || left.patrolFuel < right.patrolFuel;
    for (std::size_t roadIndex = 0; roadIndex < left.criticalFootprint.size(); ++roadIndex) {
        if (left.criticalFootprint.at(roadIndex) > right.criticalFootprint.at(roadIndex)) {
            return false;
        }
        strict = strict || left.criticalFootprint.at(roadIndex) < right.criticalFootprint.at(roadIndex);
    }
    return strict || (left.travelSteps == right.travelSteps && left.patrolFuel == right.patrolFuel &&
                      left.criticalFootprint == right.criticalFootprint);
}

[[nodiscard]] std::int32_t critical_sum(const Label& label) {
    std::int32_t result = 0;
    for (const std::int32_t count : label.criticalFootprint) {
        result += count;
    }
    return result;
}

[[nodiscard]] ParetoPath reconstruct_path(
    const MatchConfig& config,
    const std::vector<Label>& labels,
    std::int32_t terminalLabel) {
    ParetoPath result;
    const Label& terminal = labels.at(static_cast<std::size_t>(terminalLabel));
    result.travelSteps = terminal.travelSteps;
    result.patrolFuel = terminal.patrolFuel;
    std::int32_t current = terminalLabel;
    while (labels.at(static_cast<std::size_t>(current)).parent >= 0) {
        const Label& label = labels.at(static_cast<std::size_t>(current));
        const Label& parent = labels.at(static_cast<std::size_t>(label.parent));
        result.directions.push_back(label.incomingDirection);
        if (config.map.terrain.at(static_cast<std::size_t>(parent.cell)) == Terrain::Road) {
            result.heuristicFootprint.add(parent.cell, label.travelSteps - parent.travelSteps);
        }
        current = label.parent;
    }
    std::reverse(result.directions.begin(), result.directions.end());
    return result;
}

} 

void SparseRoadFootprint::add(CellId road, std::int32_t stays) {
    if (stays <= 0) {
        return;
    }
    const auto iterator = std::lower_bound(
        entries.begin(),
        entries.end(),
        road,
        [](const std::pair<CellId, std::int32_t>& entry, CellId value) { return entry.first < value; });
    if (iterator != entries.end() && iterator->first == road) {
        iterator->second += stays;
        return;
    }
    entries.insert(iterator, std::pair<CellId, std::int32_t>{road, stays});
}

std::int32_t SparseRoadFootprint::at(CellId road) const {
    const auto iterator = std::lower_bound(
        entries.begin(),
        entries.end(),
        road,
        [](const std::pair<CellId, std::int32_t>& entry, CellId value) { return entry.first < value; });
    return iterator != entries.end() && iterator->first == road ? iterator->second : 0;
}

ParetoRouter::ParetoRouter(const MatchConfig& config)
    : config_(config) {}

std::size_t ParetoRouter::RouteQueryKeyHash::operator()(const RouteQueryKey& key) const {
    std::size_t seed = 0;
    const auto mix = [&seed](std::size_t value) {
        seed ^= value + std::size_t{0x9e3779b9} + (seed << 6U) + (seed >> 2U);
    };
    mix(std::hash<CellId>{}(key.source));
    mix(std::hash<CellId>{}(key.target));
    mix(std::hash<std::int32_t>{}(key.maximumTravelSteps));
    mix(std::hash<std::int32_t>{}(key.maximumPatrolFuel));
    mix(std::hash<std::int32_t>{}(key.maximumLabelsPerCell));
    mix(std::hash<std::int32_t>{}(key.maximumPaths));
    mix(std::hash<bool>{}(key.patrol));
    mix(std::hash<bool>{}(key.useGeometricLowerBound));
    for (const RoadStatus status : key.roadStatuses) {
        mix(std::hash<std::int32_t>{}(static_cast<std::int32_t>(status)));
    }
    for (const CellId road : key.criticalRoads) {
        mix(std::hash<CellId>{}(road));
    }
    return seed;
}

std::vector<ParetoPath> ParetoRouter::find_paths(
    CellId source,
    CellId target,
    const std::vector<RoadStatus>& roadStatuses,
    const ParetoSearchOptions& options) const {
    if (!config_.map.contains(source) || !config_.map.contains(target) ||
        roadStatuses.size() != static_cast<std::size_t>(config_.map.cell_count()) || options.maximumPaths <= 0 ||
        options.maximumLabelsPerCell <= 0) {
        return {};
    }
    RouteQueryKey cacheKey;
    cacheKey.source = source;
    cacheKey.target = target;
    cacheKey.maximumTravelSteps = options.maximumTravelSteps;
    cacheKey.maximumPatrolFuel = options.maximumPatrolFuel;
    cacheKey.maximumLabelsPerCell = options.maximumLabelsPerCell;
    cacheKey.maximumPaths = options.maximumPaths;
    cacheKey.patrol = options.patrol;
    cacheKey.useGeometricLowerBound = options.useGeometricLowerBound;
    cacheKey.roadStatuses = roadStatuses;
    cacheKey.criticalRoads = options.criticalRoads;
    if (const auto cached = routeCache_.find(cacheKey); cached != routeCache_.end()) {
        return cached->second;
    }
    if (options.deadline.has_value() &&
        std::chrono::steady_clock::now() >= *options.deadline) {
        return {};
    }
    const std::int32_t maximumSteps = options.maximumTravelSteps > 0
        ? options.maximumTravelSteps
        : std::numeric_limits<std::int32_t>::max();
    if (options.patrol && options.maximumPatrolFuel < 0) {
        return {};
    }
    const std::int32_t maximumFuel = options.patrol
        ? options.maximumPatrolFuel
        : std::numeric_limits<std::int32_t>::max();
    const auto cannot_reach_target = [&](CellId cell, std::int32_t usedSteps, std::int32_t usedFuel) {
        if (!options.useGeometricLowerBound) {
            return false;
        }
        const std::int32_t remainingHops = config_.map.hex_distance(cell, target);
        if (remainingHops > maximumSteps - usedSteps) {
            return true;
        }
        return options.patrol && remainingHops > maximumFuel - usedFuel;
    };
    if (cannot_reach_target(source, 0, 0)) {
        if (routeCache_.size() >= kMaximumCachedQueries) {
            routeCache_.clear();
        }
        routeCache_.emplace(std::move(cacheKey), std::vector<ParetoPath>{});
        return {};
    }

    std::vector<std::int32_t> criticalIndex(static_cast<std::size_t>(config_.map.cell_count()), -1);
    for (std::size_t roadIndex = 0; roadIndex < options.criticalRoads.size(); ++roadIndex) {
        const CellId road = options.criticalRoads.at(roadIndex);
        if (config_.map.contains(road) &&
            config_.map.terrain.at(static_cast<std::size_t>(road)) == Terrain::Road) {
            criticalIndex.at(static_cast<std::size_t>(road)) = static_cast<std::int32_t>(roadIndex);
        }
    }

    std::vector<Label> labels;
    labels.reserve(static_cast<std::size_t>(config_.map.cell_count() * options.maximumLabelsPerCell));
    std::vector<std::vector<std::int32_t>> labelsAtCell(static_cast<std::size_t>(config_.map.cell_count()));
    Label root;
    root.cell = source;
    root.criticalFootprint.assign(options.criticalRoads.size(), 0);
    labels.push_back(root);
    labelsAtCell.at(static_cast<std::size_t>(source)).push_back(0);

    std::priority_queue<QueueItem, std::vector<QueueItem>, QueueOrder> queue;
    queue.push(QueueItem{0, 0, 0});
    std::vector<ParetoPath> results;
    bool deadlineReached = false;

    while (!queue.empty() && static_cast<std::int32_t>(results.size()) < options.maximumPaths) {
        if (options.deadline.has_value() &&
            std::chrono::steady_clock::now() >= *options.deadline) {
            deadlineReached = true;
            break;
        }
        const QueueItem item = queue.top();
        queue.pop();
        const Label current = labels.at(static_cast<std::size_t>(item.labelIndex));
        if (!current.active || current.travelSteps != item.travelSteps || current.patrolFuel != item.patrolFuel) {
            continue;
        }
        if (current.cell == target) {
            results.push_back(reconstruct_path(config_, labels, item.labelIndex));
            continue;
        }
        for (std::int32_t direction = 0; direction < kDirectionCount; ++direction) {
            const CellId destination = config_.map.neighbors.at(static_cast<std::size_t>(current.cell)).at(static_cast<std::size_t>(direction));
            if (destination == kInvalidCell || config_.map.terrain.at(static_cast<std::size_t>(destination)) == Terrain::Pond) {
                continue;
            }
            const MoveCost move = config_.move_cost(
                current.cell,
                roadStatuses.at(static_cast<std::size_t>(current.cell)));
            const std::int32_t candidateSteps = current.travelSteps + move.steps;
            const std::int32_t candidateFuel = current.patrolFuel + (options.patrol ? move.patrolFuel : 0);
            if (candidateSteps > maximumSteps || candidateFuel > maximumFuel) {
                continue;
            }
            if (cannot_reach_target(destination, candidateSteps, candidateFuel)) {
                continue;
            }
            Label candidate;
            candidate.cell = destination;
            candidate.travelSteps = candidateSteps;
            candidate.patrolFuel = candidateFuel;
            candidate.parent = item.labelIndex;
            candidate.incomingDirection = direction;
            candidate.criticalFootprint = current.criticalFootprint;
            const std::int32_t criticalRoad = criticalIndex.at(static_cast<std::size_t>(current.cell));
            if (criticalRoad >= 0) {
                candidate.criticalFootprint.at(static_cast<std::size_t>(criticalRoad)) += move.steps;
            }

            std::vector<std::int32_t>& existing = labelsAtCell.at(static_cast<std::size_t>(destination));
            bool discard = false;
            for (const std::int32_t existingIndex : existing) {
                const Label& prior = labels.at(static_cast<std::size_t>(existingIndex));
                if (prior.active && dominates(prior, candidate)) {
                    discard = true;
                    break;
                }
            }
            if (discard) {
                continue;
            }
            for (const std::int32_t existingIndex : existing) {
                Label& prior = labels.at(static_cast<std::size_t>(existingIndex));
                if (prior.active && dominates(candidate, prior)) {
                    prior.active = false;
                }
            }
            const std::int32_t candidateIndex = static_cast<std::int32_t>(labels.size());
            labels.push_back(std::move(candidate));
            existing.push_back(candidateIndex);

            std::vector<std::int32_t> activeLabels;
            activeLabels.reserve(existing.size());
            for (const std::int32_t existingIndex : existing) {
                if (labels.at(static_cast<std::size_t>(existingIndex)).active) {
                    activeLabels.push_back(existingIndex);
                }
            }
            if (static_cast<std::int32_t>(activeLabels.size()) > options.maximumLabelsPerCell) {
                std::sort(
                    activeLabels.begin(),
                    activeLabels.end(),
                    [&labels](std::int32_t leftIndex, std::int32_t rightIndex) {
                        const Label& left = labels.at(static_cast<std::size_t>(leftIndex));
                        const Label& right = labels.at(static_cast<std::size_t>(rightIndex));
                        return std::tuple{left.travelSteps, left.patrolFuel, critical_sum(left), leftIndex} <
                            std::tuple{right.travelSteps, right.patrolFuel, critical_sum(right), rightIndex};
                    });
                for (std::size_t labelOffset = static_cast<std::size_t>(options.maximumLabelsPerCell);
                     labelOffset < activeLabels.size();
                     ++labelOffset) {
                    labels.at(static_cast<std::size_t>(activeLabels.at(labelOffset))).active = false;
                }
            }
            if (labels.at(static_cast<std::size_t>(candidateIndex)).active) {
                const Label& accepted = labels.at(static_cast<std::size_t>(candidateIndex));
                queue.push(QueueItem{accepted.travelSteps, accepted.patrolFuel, candidateIndex});
            }
        }
    }
    if (!deadlineReached) {
        if (routeCache_.size() >= kMaximumCachedQueries) {
            routeCache_.clear();
        }
        routeCache_.emplace(std::move(cacheKey), results);
    }
    return results;
}

}
