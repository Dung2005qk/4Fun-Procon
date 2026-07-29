#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "udon/types.hpp"

namespace udon {

struct SparseRoadFootprint {
    std::vector<std::pair<CellId, std::int32_t>> entries;

    void add(CellId road, std::int32_t stays);
    [[nodiscard]] std::int32_t at(CellId road) const;
};

struct ParetoPath {
    std::vector<std::int32_t> directions;
    std::int32_t travelSteps = 0;
    std::int32_t patrolFuel = 0;
    SparseRoadFootprint heuristicFootprint;
};

struct ParetoSearchOptions {
    std::int32_t maximumTravelSteps = 0;
    std::int32_t maximumPatrolFuel = std::numeric_limits<std::int32_t>::max();
    std::int32_t maximumLabelsPerCell = 32;
    std::int32_t maximumPaths = 8;
    bool patrol = true;
    bool useGeometricLowerBound = true;
    std::vector<CellId> criticalRoads;
    std::optional<std::chrono::steady_clock::time_point> deadline;
};

struct ParetoSearchDiagnostics {
    std::int32_t queries = 0;
    std::int32_t cacheHits = 0;
    std::int32_t cacheMisses = 0;
    std::int32_t cacheClears = 0;
    std::int32_t resourceBoundCacheHits = 0;
    std::int32_t resourceBoundCacheMisses = 0;
    std::int32_t deadlineRejectedQueries = 0;
    std::int32_t deadlineInterruptedQueries = 0;
    std::int64_t queuePops = 0;
    std::int64_t labelsGenerated = 0;
    std::int64_t labelsDominanceRejected = 0;
    std::int64_t labelsDominated = 0;
    std::int64_t labelsPrunedByCap = 0;
    std::int64_t labelsPrunedByResourceBound = 0;
};

class ParetoRouter {
public:
    explicit ParetoRouter(const MatchConfig& config);

    [[nodiscard]] std::vector<ParetoPath> find_paths(
        CellId source,
        CellId target,
        const std::vector<RoadStatus>& roadStatuses,
        const ParetoSearchOptions& options,
        ParetoSearchDiagnostics* diagnostics = nullptr) const;

private:
    struct RouteQueryKey {
        CellId source = kInvalidCell;
        CellId target = kInvalidCell;
        std::int32_t maximumTravelSteps = 0;
        std::int32_t maximumPatrolFuel = 0;
        std::int32_t maximumLabelsPerCell = 0;
        std::int32_t maximumPaths = 0;
        bool patrol = true;
        bool useGeometricLowerBound = true;
        std::vector<RoadStatus> roadStatuses;
        std::vector<CellId> criticalRoads;

        [[nodiscard]] bool operator==(const RouteQueryKey& other) const = default;
    };

    struct RouteQueryKeyHash {
        [[nodiscard]] std::size_t operator()(const RouteQueryKey& key) const;
    };

    struct ResourceLowerBoundKey {
        CellId target = kInvalidCell;
        std::vector<RoadStatus> roadStatuses;

        [[nodiscard]] bool operator==(const ResourceLowerBoundKey& other) const = default;
    };

    struct ResourceLowerBoundKeyHash {
        [[nodiscard]] std::size_t operator()(const ResourceLowerBoundKey& key) const;
    };

    struct ResourceLowerBounds {
        std::vector<std::int32_t> travelSteps;
        std::vector<std::int32_t> patrolFuel;
    };

    static constexpr std::size_t kMaximumCachedQueries = 4096;
    static constexpr std::size_t kMaximumCachedBytes = 64U * 1024U * 1024U;
    static constexpr std::size_t kMaximumResourceLowerBounds = 256;

    const MatchConfig& config_;
    mutable std::unordered_map<RouteQueryKey, std::vector<ParetoPath>, RouteQueryKeyHash> routeCache_;
    mutable std::size_t routeCacheBytes_ = 0;
    mutable std::unordered_map<
        ResourceLowerBoundKey,
        ResourceLowerBounds,
        ResourceLowerBoundKeyHash> resourceLowerBoundCache_;
};

} 
