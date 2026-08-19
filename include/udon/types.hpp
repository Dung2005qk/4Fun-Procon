#pragma once

#include <algorithm>
#include <bit>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace udon {

using CellId = std::int32_t;
using AgentIndex = std::int32_t;
using SpotIndex = std::int32_t;

constexpr CellId kInvalidCell = -1;
constexpr AgentIndex kInvalidAgent = -1;
constexpr SpotIndex kInvalidSpot = -1;
constexpr std::int32_t kDirectionCount = 6;
constexpr std::int32_t kMaximumAgents = 8;
constexpr std::int32_t kMaximumMapSide = 32;
constexpr std::int32_t kMaximumCells = kMaximumMapSide * kMaximumMapSide;
inline constexpr std::chrono::milliseconds kCompetitionComputeHardCap{5000};

[[nodiscard]] constexpr std::chrono::milliseconds competition_compute_budget(
    std::chrono::milliseconds requested) noexcept {
    return requested <= std::chrono::milliseconds{0}
        ? std::chrono::milliseconds{0}
        : (requested < kCompetitionComputeHardCap
               ? requested
               : kCompetitionComputeHardCap);
}

enum class Terrain : std::uint8_t {
    Plain = 0,
    Road = 1,
    Mountain = 2,
    Pond = 3,
};

enum class RoadStatus : std::uint8_t {
    Smooth = 0,
    Busy = 1,
    Jammed = 2,
};

enum class AgentKind : std::uint8_t {
    Patrol = 0,
    Tanker = 1,
};

enum class ActionKind : std::uint8_t {
    Move,
    Wait,
};

enum class SimulationErrorCode : std::uint8_t {
    None,
    AgentCountMismatch,
    EmptyActionPlan,
    InvalidDirection,
    InvalidWaitDuration,
    InvalidDestination,
    DurationMismatch,
    MovementPastDeadline,
    InsufficientFuel,
    InvalidDay,
    InternalInvariant,
};

struct MoveCost {
    std::int32_t steps = 0;
    std::int32_t patrolFuel = 0;
};

struct PlanAction {
    ActionKind kind = ActionKind::Wait;
    std::int32_t value = 1;

    [[nodiscard]] static PlanAction move(std::int32_t direction) {
        return PlanAction{ActionKind::Move, direction};
    }

    [[nodiscard]] static PlanAction wait(std::int32_t duration) {
        return PlanAction{ActionKind::Wait, duration};
    }

    [[nodiscard]] std::int32_t wire_value() const {
        return kind == ActionKind::Move ? value : -value;
    }
};

using AgentPlan = std::vector<PlanAction>;

struct DayPlan {
    std::vector<AgentPlan> actions;
};

struct Spot {
    std::int32_t brandValue = 0;
    std::int32_t brandIndex = 0;
    CellId position = kInvalidCell;
    std::int32_t stock = 0;
};

struct CubeCoordinate {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t z = 0;

    [[nodiscard]] friend bool operator==(const CubeCoordinate& left, const CubeCoordinate& right) = default;
};

struct GridMap {
    std::int32_t height = 0;
    std::int32_t width = 0;
    std::vector<Terrain> terrain;
    std::vector<std::array<CellId, kDirectionCount>> neighbors;

    [[nodiscard]] std::int32_t cell_count() const {
        return height * width;
    }

    [[nodiscard]] bool contains(CellId cell) const {
        return cell >= 0 && cell < cell_count();
    }

    [[nodiscard]] std::int32_t row_of(CellId cell) const {
        return cell / width;
    }

    [[nodiscard]] std::int32_t column_of(CellId cell) const {
        return cell % width;
    }

    [[nodiscard]] CubeCoordinate cube_coordinate(CellId cell) const {
        const std::int32_t row = row_of(cell);
        const std::int32_t column = column_of(cell);
        const std::int32_t x = column - (row + (row & 1)) / 2;
        const std::int32_t z = row;
        return CubeCoordinate{x, -x - z, z};
    }

    [[nodiscard]] std::int32_t hex_distance(CellId left, CellId right) const {
        if (!contains(left) || !contains(right)) {
            return std::numeric_limits<std::int32_t>::max();
        }
        const CubeCoordinate leftCube = cube_coordinate(left);
        const CubeCoordinate rightCube = cube_coordinate(right);
        return std::max({
            std::abs(leftCube.x - rightCube.x),
            std::abs(leftCube.y - rightCube.y),
            std::abs(leftCube.z - rightCube.z),
        });
    }
};

struct MatchConfig {
    std::int64_t startsAt = 0;
    std::vector<std::int32_t> daySeconds;
    std::vector<std::int32_t> daySteps;
    GridMap map;
    std::vector<Spot> spots;
    std::vector<CellId> initialAgents;
    std::int32_t fuelLimit = 0;
    std::int32_t players = 0;
    std::int32_t busyThreshold = 0;
    std::int32_t jammedThreshold = 0;
    std::vector<SpotIndex> spotAtCell;
    std::vector<CellId> roadCells;
    std::vector<std::int32_t> brandValues;
    std::map<std::int32_t, std::int32_t> brandToIndex;

    [[nodiscard]] std::int32_t day_count() const {
        return static_cast<std::int32_t>(daySteps.size());
    }

    [[nodiscard]] std::int32_t agent_count() const {
        return static_cast<std::int32_t>(initialAgents.size());
    }

    [[nodiscard]] std::int32_t brand_count() const {
        return static_cast<std::int32_t>(brandValues.size());
    }

    [[nodiscard]] std::int32_t steps_for_day(std::int32_t dayNumber) const {
        return daySteps.at(static_cast<std::size_t>(dayNumber - 1));
    }

    [[nodiscard]] MoveCost move_cost(CellId source, RoadStatus roadStatus) const {
        const Terrain sourceTerrain = map.terrain.at(static_cast<std::size_t>(source));
        switch (sourceTerrain) {
        case Terrain::Plain:
            return MoveCost{2, 1};
        case Terrain::Mountain:
            return MoveCost{3, 2};
        case Terrain::Road:
            switch (roadStatus) {
            case RoadStatus::Smooth:
                return MoveCost{1, 2};
            case RoadStatus::Busy:
                return MoveCost{2, 2};
            case RoadStatus::Jammed:
                return MoveCost{4, 2};
            }
            break;
        case Terrain::Pond:
            break;
        }
        return MoveCost{};
    }
};

struct AgentState {
    AgentKind kind = AgentKind::Patrol;
    CellId position = kInvalidCell;
    std::int32_t fuel = 0;
};

struct OtherTeamState {
    std::int32_t teamId = 0;
    std::vector<AgentState> agents;
};

struct DayState {
    std::int64_t endsAt = 0;
    std::int32_t dayNumber = 0;
    std::vector<AgentState> agents;
    std::vector<OtherTeamState> others;
    std::vector<RoadStatus> roadStatuses;
};

struct DayScore {
    std::uint64_t brands = 0;
    std::int32_t dailyDistinct = 0;
    std::int32_t servings = 0;
};

struct MatchLedger {
    std::uint64_t lifetimeBrands = 0;
    std::int32_t totalDailyDistinct = 0;
    std::int32_t totalServings = 0;

    [[nodiscard]] std::int32_t lifetime_distinct() const {
        return static_cast<std::int32_t>(std::popcount(lifetimeBrands));
    }

    void apply(const DayScore& score) {
        lifetimeBrands |= score.brands;
        totalDailyDistinct += score.dailyDistinct;
        totalServings += score.servings;
    }
};

struct OfficialScore {
    std::int32_t lifetimeDistinct = 0;
    std::int32_t totalDailyDistinct = 0;
    std::int32_t totalServings = 0;

    [[nodiscard]] static OfficialScore after_day(const MatchLedger& ledger, const DayScore& dayScore) {
        return OfficialScore{
            static_cast<std::int32_t>(std::popcount(ledger.lifetimeBrands | dayScore.brands)),
            ledger.totalDailyDistinct + dayScore.dailyDistinct,
            ledger.totalServings + dayScore.servings,
        };
    }

    [[nodiscard]] friend bool operator==(const OfficialScore& left, const OfficialScore& right) = default;

    [[nodiscard]] friend bool operator<(const OfficialScore& left, const OfficialScore& right) {
        if (left.lifetimeDistinct != right.lifetimeDistinct) {
            return left.lifetimeDistinct < right.lifetimeDistinct;
        }
        if (left.totalDailyDistinct != right.totalDailyDistinct) {
            return left.totalDailyDistinct < right.totalDailyDistinct;
        }
        return left.totalServings < right.totalServings;
    }
};

struct ClaimEvent {
    AgentIndex agent = kInvalidAgent;
    SpotIndex spot = kInvalidSpot;
    std::int32_t step = 0;
    bool served = false;
};

struct StepTrace {
    std::int32_t stepCount = 0;
    std::int32_t agentCount = 0;
    std::vector<CellId> positions;
    std::vector<std::int32_t> fuels;

    [[nodiscard]] CellId position_at(std::int32_t step, AgentIndex agent) const {
        const std::size_t offset = static_cast<std::size_t>(step * agentCount + agent);
        return positions.at(offset);
    }

    [[nodiscard]] std::int32_t fuel_at(std::int32_t step, AgentIndex agent) const {
        const std::size_t offset = static_cast<std::size_t>(step * agentCount + agent);
        return fuels.at(offset);
    }
};

struct SimulationError {
    SimulationErrorCode code = SimulationErrorCode::None;
    AgentIndex agent = kInvalidAgent;
    std::int32_t step = -1;
    std::string message;
};

struct SimulationResult {
    bool valid = false;
    std::optional<SimulationError> error;
    std::vector<AgentState> finalAgents;
    DayScore score;
    std::vector<std::int32_t> roadFootprint;
    std::vector<ClaimEvent> claims;
    StepTrace trace;
};

[[nodiscard]] inline std::int32_t compare_lexicographic(
    const OfficialScore& left,
    const OfficialScore& right) {
    if (left < right) {
        return -1;
    }
    if (right < left) {
        return 1;
    }
    return 0;
}

[[nodiscard]] inline bool has_brand(std::uint64_t mask, std::int32_t brandIndex) {
    return (mask & (std::uint64_t{1} << static_cast<std::uint32_t>(brandIndex))) != 0;
}

[[nodiscard]] inline std::uint64_t brand_bit(std::int32_t brandIndex) {
    return std::uint64_t{1} << static_cast<std::uint32_t>(brandIndex);
}

}
