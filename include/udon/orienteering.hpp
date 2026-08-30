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
    std::vector<ExactOrienteeringRoute> supplementalRoutes;
    // SCORE-TERMINAL-STOCK-MARGINAL-RESERVOIR-258 research candidate: an
    // additive terminal-only pool ranked against the incumbent team's public
    // stock claims. Canonical maximal/supplemental retention is unchanged.
    std::vector<ExactOrienteeringRoute> terminalMarginalRoutes;
    std::vector<ExactOrienteeringRoute> terminalVariants;
    std::vector<ExactOrienteeringRoute> servedSpotFuelRoutes;
};

struct TerminalMarginalRouteContext {
    BrandMask lifetimeBrands;
    std::vector<std::int32_t> incumbentClaimsWithoutAgent;
};

[[nodiscard]] bool exact_orienteering_dense_state_supported(
    const MatchConfig& config) noexcept;

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
    std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt,
    BrandMask preferredBrands = {});

[[nodiscard]] ExactOrienteeringReachability enumerate_sparse_anytime_resource_routes(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agent,
    std::int32_t minimumSpots,
    std::size_t maximumRoutes,
    std::uint64_t maximumSettledStates,
    std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt,
    BrandMask preferredBrands = {},
    const TerminalMarginalRouteContext* terminalMarginalContext = nullptr);

// Target-terminal sparse entry point introduced by attribution experiment 214
// and consumed by the SCORE-MIDDAY-TARGET-FOLLOWUP-215 suffix.
// It uses the same sparse label search and Pareto dominance as the canonical
// enumerator, but retains labels at one caller-supplied terminal instead of
// globally retaining only routes whose terminal is an udon spot.
[[nodiscard]] ExactOrienteeringReachability
enumerate_sparse_anytime_resource_routes_to_terminal(
    const MatchConfig& config,
    const DayState& state,
    AgentIndex agent,
    CellId requiredTerminal,
    std::int32_t minimumSpots,
    std::size_t maximumRoutes,
    std::uint64_t maximumSettledStates,
    std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt,
    BrandMask preferredBrands = {},
    const TerminalMarginalRouteContext* terminalMarginalContext = nullptr);

}
