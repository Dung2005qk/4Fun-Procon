#include "udon/graph.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <queue>
#include <tuple>

namespace udon {

namespace {

struct LabelCriticalFootprint {
    static constexpr std::size_t kInlineCapacity = 16;

    void assign(std::size_t count, std::int32_t value) {
        count_ = count;
        inlineValues_.fill(value);
        overflowValues_.assign(
            count > kInlineCapacity ? count - kInlineCapacity : 0,
            value);
    }

    [[nodiscard]] std::size_t size() const {
        return count_;
    }

    [[nodiscard]] std::int32_t at(std::size_t index) const {
        return index < kInlineCapacity
            ? inlineValues_.at(index)
            : overflowValues_.at(index - kInlineCapacity);
    }

    [[nodiscard]] std::int32_t& at(std::size_t index) {
        return index < kInlineCapacity
            ? inlineValues_.at(index)
            : overflowValues_.at(index - kInlineCapacity);
    }

    [[nodiscard]] bool operator==(const LabelCriticalFootprint& other) const {
        if (count_ != other.count_) {
            return false;
        }
        for (std::size_t index = 0; index < count_; ++index) {
            if (at(index) != other.at(index)) {
                return false;
            }
        }
        return true;
    }

private:
    std::array<std::int32_t, kInlineCapacity> inlineValues_{};
    std::vector<std::int32_t> overflowValues_;
    std::size_t count_ = 0;
};

struct Label {
    CellId cell = kInvalidCell;
    std::int32_t travelSteps = 0;
    std::int32_t patrolFuel = 0;
    std::int32_t parent = -1;
    std::int32_t incomingDirection = -1;
    LabelCriticalFootprint criticalFootprint;
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
    for (std::size_t index = 0; index < label.criticalFootprint.size(); ++index) {
        result += label.criticalFootprint.at(index);
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

std::size_t ParetoRouter::ResourceLowerBoundKeyHash::operator()(
    const ResourceLowerBoundKey& key) const {
    std::size_t seed = std::hash<CellId>{}(key.target);
    for (const RoadStatus status : key.roadStatuses) {
        seed ^= std::hash<std::int32_t>{}(static_cast<std::int32_t>(status)) +
            std::size_t{0x9e3779b9} + (seed << 6U) + (seed >> 2U);
    }
    return seed;
}

std::vector<ParetoPath> ParetoRouter::find_paths(
    CellId source,
    CellId target,
    const std::vector<RoadStatus>& roadStatuses,
    const ParetoSearchOptions& options,
    ParetoSearchDiagnostics* diagnostics) const {
    if (diagnostics != nullptr) {
        ++diagnostics->queries;
    }
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
    const auto cache_result = [this, diagnostics](
                                  RouteQueryKey&& key,
                                  const std::vector<ParetoPath>& paths) {
        std::size_t entryBytes =
            sizeof(RouteQueryKey) + sizeof(std::vector<ParetoPath>) +
            key.roadStatuses.size() * sizeof(RoadStatus) +
            key.criticalRoads.size() * sizeof(CellId) +
            paths.size() * sizeof(ParetoPath);
        for (const ParetoPath& path : paths) {
            entryBytes += path.directions.size() * sizeof(std::int32_t);
            entryBytes += path.heuristicFootprint.entries.size() *
                sizeof(std::pair<CellId, std::int32_t>);
        }
        if (entryBytes > kMaximumCachedBytes) {
            return;
        }
        if (routeCache_.size() >= kMaximumCachedQueries ||
            routeCacheBytes_ > kMaximumCachedBytes - entryBytes) {
            routeCache_.clear();
            routeCacheBytes_ = 0;
            if (diagnostics != nullptr) {
                ++diagnostics->cacheClears;
            }
        }
        routeCacheBytes_ += entryBytes;
        routeCache_.emplace(std::move(key), paths);
    };
    if (const auto cached = routeCache_.find(cacheKey); cached != routeCache_.end()) {
        if (diagnostics != nullptr) {
            ++diagnostics->cacheHits;
        }
        return cached->second;
    }
    if (diagnostics != nullptr) {
        ++diagnostics->cacheMisses;
    }
    if (options.deadline.has_value() &&
        std::chrono::steady_clock::now() >= *options.deadline) {
        if (diagnostics != nullptr) {
            ++diagnostics->deadlineRejectedQueries;
        }
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
    const ResourceLowerBounds* resourceLowerBounds = nullptr;
    if (options.useGeometricLowerBound) {
        ResourceLowerBoundKey lowerBoundKey;
        lowerBoundKey.target = target;
        lowerBoundKey.roadStatuses = roadStatuses;
        auto lowerBoundIterator = resourceLowerBoundCache_.find(lowerBoundKey);
        if (lowerBoundIterator == resourceLowerBoundCache_.end()) {
            if (diagnostics != nullptr) {
                ++diagnostics->resourceBoundCacheMisses;
            }
            if (resourceLowerBoundCache_.size() >= kMaximumResourceLowerBounds) {
                resourceLowerBoundCache_.clear();
            }
            ResourceLowerBounds bounds;
            const auto reverse_shortest_paths =
                [this, target, &roadStatuses](bool patrolFuel) {
                    std::vector<std::int32_t> distances(
                        static_cast<std::size_t>(config_.map.cell_count()),
                        std::numeric_limits<std::int32_t>::max());
                    using DistanceEntry = std::pair<std::int32_t, CellId>;
                    std::priority_queue<
                        DistanceEntry,
                        std::vector<DistanceEntry>,
                        std::greater<>> queue;
                    distances.at(static_cast<std::size_t>(target)) = 0;
                    queue.push(DistanceEntry{0, target});
                    while (!queue.empty()) {
                        const auto [distance, cell] = queue.top();
                        queue.pop();
                        if (distance != distances.at(static_cast<std::size_t>(cell))) {
                            continue;
                        }
                        for (const CellId predecessor :
                             config_.map.neighbors.at(static_cast<std::size_t>(cell))) {
                            if (predecessor == kInvalidCell ||
                                config_.map.terrain.at(static_cast<std::size_t>(predecessor)) ==
                                    Terrain::Pond) {
                                continue;
                            }
                            const MoveCost move = config_.move_cost(
                                predecessor,
                                roadStatuses.at(static_cast<std::size_t>(predecessor)));
                            const std::int32_t edgeCost = patrolFuel
                                ? move.patrolFuel
                                : move.steps;
                            const std::int32_t candidate = distance + edgeCost;
                            std::int32_t& incumbent =
                                distances.at(static_cast<std::size_t>(predecessor));
                            if (candidate < incumbent) {
                                incumbent = candidate;
                                queue.push(DistanceEntry{candidate, predecessor});
                            }
                        }
                    }
                    return distances;
                };
            bounds.travelSteps = reverse_shortest_paths(false);
            bounds.patrolFuel = reverse_shortest_paths(true);
            lowerBoundIterator = resourceLowerBoundCache_.emplace(
                std::move(lowerBoundKey),
                std::move(bounds)).first;
        } else if (diagnostics != nullptr) {
            ++diagnostics->resourceBoundCacheHits;
        }
        resourceLowerBounds = &lowerBoundIterator->second;
    }
    const auto cannot_reach_target = [&](CellId cell, std::int32_t usedSteps, std::int32_t usedFuel) {
        if (!options.useGeometricLowerBound) {
            return false;
        }
        const std::size_t cellOffset = static_cast<std::size_t>(cell);
        const bool unreachable =
            resourceLowerBounds->travelSteps.at(cellOffset) > maximumSteps - usedSteps ||
            (options.patrol &&
             resourceLowerBounds->patrolFuel.at(cellOffset) > maximumFuel - usedFuel);
        if (unreachable && diagnostics != nullptr) {
            ++diagnostics->labelsPrunedByResourceBound;
        }
        return unreachable;
    };
    if (cannot_reach_target(source, 0, 0)) {
        cache_result(std::move(cacheKey), {});
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
    if (diagnostics != nullptr) {
        ++diagnostics->labelsGenerated;
    }
    labelsAtCell.at(static_cast<std::size_t>(source)).push_back(0);

    std::priority_queue<QueueItem, std::vector<QueueItem>, QueueOrder> queue;
    queue.push(QueueItem{0, 0, 0});
    std::vector<ParetoPath> results;
    bool deadlineReached = false;

    while (!queue.empty() && static_cast<std::int32_t>(results.size()) < options.maximumPaths) {
        if (options.deadline.has_value() &&
            std::chrono::steady_clock::now() >= *options.deadline) {
            deadlineReached = true;
            if (diagnostics != nullptr) {
                ++diagnostics->deadlineInterruptedQueries;
            }
            break;
        }
        const QueueItem item = queue.top();
        queue.pop();
        if (diagnostics != nullptr) {
            ++diagnostics->queuePops;
        }
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
                if (diagnostics != nullptr) {
                    ++diagnostics->labelsDominanceRejected;
                }
                continue;
            }
            for (const std::int32_t existingIndex : existing) {
                Label& prior = labels.at(static_cast<std::size_t>(existingIndex));
                if (prior.active && dominates(candidate, prior)) {
                    prior.active = false;
                    if (diagnostics != nullptr) {
                        ++diagnostics->labelsDominated;
                    }
                }
            }
            const std::int32_t candidateIndex = static_cast<std::int32_t>(labels.size());
            labels.push_back(std::move(candidate));
            if (diagnostics != nullptr) {
                ++diagnostics->labelsGenerated;
            }
            existing.push_back(candidateIndex);

            existing.erase(
                std::remove_if(
                    existing.begin(),
                    existing.end(),
                    [&labels](std::int32_t existingIndex) {
                        return !labels.at(static_cast<std::size_t>(existingIndex)).active;
                    }),
                existing.end());
            if (static_cast<std::int32_t>(existing.size()) > options.maximumLabelsPerCell) {
                std::vector<std::int32_t> activeLabels = existing;
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
                    if (diagnostics != nullptr) {
                        ++diagnostics->labelsPrunedByCap;
                    }
                }
                existing.erase(
                    std::remove_if(
                        existing.begin(),
                        existing.end(),
                        [&labels](std::int32_t existingIndex) {
                            return !labels.at(static_cast<std::size_t>(existingIndex)).active;
                        }),
                    existing.end());
            }
            if (labels.at(static_cast<std::size_t>(candidateIndex)).active) {
                const Label& accepted = labels.at(static_cast<std::size_t>(candidateIndex));
                queue.push(QueueItem{accepted.travelSteps, accepted.patrolFuel, candidateIndex});
            }
        }
    }
    if (!deadlineReached) {
        cache_result(std::move(cacheKey), results);
    }
    return results;
}

}
