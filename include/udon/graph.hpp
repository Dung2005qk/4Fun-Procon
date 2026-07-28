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

class ParetoRouter {
public:
    explicit ParetoRouter(const MatchConfig& config);

    [[nodiscard]] std::vector<ParetoPath> find_paths(
        CellId source,
        CellId target,
        const std::vector<RoadStatus>& roadStatuses,
        const ParetoSearchOptions& options) const;

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

    static constexpr std::size_t kMaximumCachedQueries = 4096;

    const MatchConfig& config_;
    mutable std::unordered_map<RouteQueryKey, std::vector<ParetoPath>, RouteQueryKeyHash> routeCache_;
};

} 
