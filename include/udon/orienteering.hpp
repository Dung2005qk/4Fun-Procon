#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "udon/types.hpp"

namespace udon {

struct ExactOrienteeringRoute {
    std::uint32_t spotMask = 0;
    AgentPlan actions;
    std::int32_t usedSteps = 0;
    std::int32_t patrolFuel = 0;
    CellId terminalCell = kInvalidCell;
    std::int32_t terminalBrandDistance = 0;
    bool terminalOnSpot = false;
};

struct ExactOrienteeringReachability {
    bool supported = false;
    bool complete = false;
    std::uint64_t settledStates = 0;
    std::vector<ExactOrienteeringRoute> maximalRoutes;
    std::vector<ExactOrienteeringRoute> terminalVariants;
};

[[nodiscard]] ExactOrienteeringReachability enumerate_exact_high_fuel_routes(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agent,
    std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt);

[[nodiscard]] ExactOrienteeringReachability enumerate_exact_resource_routes(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agent,
    std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt);

[[nodiscard]] ExactOrienteeringReachability enumerate_anytime_resource_routes(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agent,
    std::int32_t minimumSpots,
    std::size_t maximumRoutes,
    std::uint64_t maximumSettledStates,
    std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt);

}
