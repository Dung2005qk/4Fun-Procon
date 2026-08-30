#pragma once

#include <algorithm>
#include <bit>
#include <array>
#include <chrono>
#include <compare>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
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
constexpr std::int32_t kMaximumBrands = kMaximumCells;
inline constexpr std::chrono::milliseconds kCompetitionComputeHardCap{5000};

class BrandMask {
public:
    static constexpr std::size_t kWordBits = 64U;
    static constexpr std::size_t kWordCount =
        static_cast<std::size_t>(kMaximumBrands) / kWordBits;
    static constexpr std::size_t kHighWordCount = kWordCount - 1U;
    using HighWords = std::array<std::uint64_t, kHighWordCount>;

    BrandMask() = default;
    BrandMask(std::uint64_t low) noexcept : low_(low) {}
    BrandMask(const BrandMask& other)
        : low_(other.low_),
          high_(other.high_ == nullptr
              ? nullptr
              : std::make_unique<HighWords>(*other.high_)) {}
    BrandMask(BrandMask&&) noexcept = default;

    BrandMask& operator=(const BrandMask& other) {
        if (this == &other) {
            return *this;
        }
        low_ = other.low_;
        high_ = other.high_ == nullptr
            ? nullptr
            : std::make_unique<HighWords>(*other.high_);
        return *this;
    }

    BrandMask& operator=(BrandMask&&) noexcept = default;

    [[nodiscard]] bool test(std::int32_t index) const noexcept {
        if (index < 0 || index >= kMaximumBrands) {
            return false;
        }
        const std::size_t word = static_cast<std::size_t>(index) / kWordBits;
        const std::uint64_t bit =
            std::uint64_t{1} << (static_cast<std::size_t>(index) % kWordBits);
        return word == 0U
            ? (low_ & bit) != 0U
            : high_ != nullptr && (high_->at(word - 1U) & bit) != 0U;
    }

    void set(std::int32_t index) {
        if (index < 0 || index >= kMaximumBrands) {
            throw std::out_of_range("brand index exceeds the official map capacity");
        }
        const std::size_t word = static_cast<std::size_t>(index) / kWordBits;
        const std::uint64_t bit =
            std::uint64_t{1} << (static_cast<std::size_t>(index) % kWordBits);
        if (word == 0U) {
            low_ |= bit;
            return;
        }
        ensure_unique_high();
        high_->at(word - 1U) |= bit;
    }

    [[nodiscard]] std::int32_t count() const noexcept {
        std::int32_t result = static_cast<std::int32_t>(std::popcount(low_));
        if (high_ != nullptr) {
            for (const std::uint64_t word : *high_) {
                result += static_cast<std::int32_t>(std::popcount(word));
            }
        }
        return result;
    }

    [[nodiscard]] bool any() const noexcept {
        if (low_ != 0U) {
            return true;
        }
        return high_ != nullptr && std::any_of(
            high_->begin(),
            high_->end(),
            [](std::uint64_t word) { return word != 0U; });
    }

    [[nodiscard]] bool is_subset_of(const BrandMask& other) const noexcept {
        if ((low_ & ~other.low_) != 0U) {
            return false;
        }
        if (high_ == nullptr) {
            return true;
        }
        for (std::size_t index = 0; index < kHighWordCount; ++index) {
            const std::uint64_t otherWord = other.high_ == nullptr
                ? 0U
                : other.high_->at(index);
            if ((high_->at(index) & ~otherWord) != 0U) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] std::int32_t intersection_count(const BrandMask& other) const noexcept {
        std::int32_t result = static_cast<std::int32_t>(
            std::popcount(low_ & other.low_));
        if (high_ == nullptr || other.high_ == nullptr) {
            return result;
        }
        for (std::size_t index = 0; index < kHighWordCount; ++index) {
            result += static_cast<std::int32_t>(std::popcount(
                high_->at(index) & other.high_->at(index)));
        }
        return result;
    }

    [[nodiscard]] std::int32_t difference_count(const BrandMask& other) const noexcept {
        std::int32_t result = static_cast<std::int32_t>(
            std::popcount(low_ & ~other.low_));
        if (high_ == nullptr) {
            return result;
        }
        for (std::size_t index = 0; index < kHighWordCount; ++index) {
            const std::uint64_t otherWord = other.high_ == nullptr
                ? 0U
                : other.high_->at(index);
            result += static_cast<std::int32_t>(std::popcount(
                high_->at(index) & ~otherWord));
        }
        return result;
    }

    [[nodiscard]] BrandMask without(const BrandMask& other) const {
        BrandMask result{low_ & ~other.low_};
        if (high_ == nullptr) {
            return result;
        }
        for (std::size_t index = 0; index < kHighWordCount; ++index) {
            const std::uint64_t otherWord = other.high_ == nullptr
                ? 0U
                : other.high_->at(index);
            const std::uint64_t difference = high_->at(index) & ~otherWord;
            if (difference != 0U) {
                result.ensure_unique_high();
                result.high_->at(index) = difference;
            }
        }
        return result;
    }

    BrandMask& operator|=(const BrandMask& other) {
        low_ |= other.low_;
        if (other.high_ == nullptr) {
            return *this;
        }
        ensure_unique_high();
        for (std::size_t index = 0; index < kHighWordCount; ++index) {
            high_->at(index) |= other.high_->at(index);
        }
        return *this;
    }

    BrandMask& operator&=(const BrandMask& other) {
        low_ &= other.low_;
        if (high_ == nullptr) {
            return *this;
        }
        if (other.high_ == nullptr) {
            high_.reset();
            return *this;
        }
        ensure_unique_high();
        bool anyHigh = false;
        for (std::size_t index = 0; index < kHighWordCount; ++index) {
            high_->at(index) &= other.high_->at(index);
            anyHigh = anyHigh || high_->at(index) != 0U;
        }
        if (!anyHigh) {
            high_.reset();
        }
        return *this;
    }

    [[nodiscard]] std::string wire_string() const {
        if (high_ == nullptr) {
            return std::to_string(low_);
        }
        std::string result = std::to_string(low_);
        for (const std::uint64_t word : *high_) {
            result.push_back(':');
            result += std::to_string(word);
        }
        return result;
    }

    [[nodiscard]] friend BrandMask operator|(BrandMask left, const BrandMask& right) {
        left |= right;
        return left;
    }

    [[nodiscard]] friend BrandMask operator&(BrandMask left, const BrandMask& right) {
        left &= right;
        return left;
    }

    [[nodiscard]] friend bool operator==(const BrandMask& left, const BrandMask& right) noexcept {
        if (left.low_ != right.low_) {
            return false;
        }
        if (left.high_ == nullptr || right.high_ == nullptr) {
            const HighWords* present = left.high_ != nullptr
                ? left.high_.get()
                : right.high_.get();
            return present == nullptr || std::all_of(
                present->begin(),
                present->end(),
                [](std::uint64_t word) { return word == 0U; });
        }
        return *left.high_ == *right.high_;
    }

    [[nodiscard]] friend std::strong_ordering operator<=> (
        const BrandMask& left,
        const BrandMask& right) noexcept {
        for (std::size_t offset = kHighWordCount; offset > 0U; --offset) {
            const std::uint64_t leftWord = left.high_ == nullptr
                ? 0U
                : left.high_->at(offset - 1U);
            const std::uint64_t rightWord = right.high_ == nullptr
                ? 0U
                : right.high_->at(offset - 1U);
            if (leftWord < rightWord) {
                return std::strong_ordering::less;
            }
            if (leftWord > rightWord) {
                return std::strong_ordering::greater;
            }
        }
        return left.low_ <=> right.low_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const BrandMask& mask) {
        return stream << mask.wire_string();
    }

private:
    void ensure_unique_high() {
        if (high_ == nullptr) {
            high_ = std::make_unique<HighWords>();
        }
    }

    std::uint64_t low_ = 0U;
    std::unique_ptr<HighWords> high_;
};

[[nodiscard]] inline BrandMask brand_difference(
    const BrandMask& left,
    const BrandMask& right) {
    return left.without(right);
}

[[nodiscard]] inline std::int32_t brand_count(const BrandMask& mask) noexcept {
    return mask.count();
}

[[nodiscard]] inline std::int32_t brand_intersection_count(
    const BrandMask& left,
    const BrandMask& right) noexcept {
    return left.intersection_count(right);
}

[[nodiscard]] inline std::int32_t brand_difference_count(
    const BrandMask& left,
    const BrandMask& right) noexcept {
    return left.difference_count(right);
}

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
    BrandMask brands;
    std::int32_t dailyDistinct = 0;
    std::int32_t servings = 0;
};

struct MatchLedger {
    BrandMask lifetimeBrands;
    std::int32_t totalDailyDistinct = 0;
    std::int32_t totalServings = 0;

    [[nodiscard]] std::int32_t lifetime_distinct() const {
        return brand_count(lifetimeBrands);
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
            brand_count(ledger.lifetimeBrands | dayScore.brands),
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

[[nodiscard]] inline bool has_brand(const BrandMask& mask, std::int32_t brandIndex) {
    return mask.test(brandIndex);
}

[[nodiscard]] inline BrandMask brand_bit(std::int32_t brandIndex) {
    BrandMask result;
    result.set(brandIndex);
    return result;
}

}
