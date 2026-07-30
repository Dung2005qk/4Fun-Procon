#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "blank_slate/planners.hpp"
#include "udon/decision.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"

namespace {

enum class Strategy {
    WaitOnly,
    GreedyOneDay,
    TopFcWeighted,
    RollingAlnsUnshielded,
    RollingHalnsRoutePool,
    RollingHalnsFeedback,
    RollingHalnsFeedbackDiverse,
    EventConflict,
    BackwardDeadline,
    MacroMcts,
    BlankPortfolio,
    UdonShieldFixedRoles,
    UdonShieldFeedback,
    UdonShieldProofExpanded,
    UdonShield,
    UdonShieldMultiHarvest,
    UdonShieldDeepHarvest,
    UdonShieldAdaptiveDepth4,
    UdonShieldOrienteering,
    UdonShieldDeepHarvestCentralRoles,
    UdonShieldDeepHarvestAdaptiveRoles3,
    UdonShieldDeepHarvestAdaptiveRoles,
    DeterministicAlns,
    DeterministicProofAlns,
    DeterministicProofAlns4x,
    DeterministicProofAlns8x,
    DeterministicProofRoutePool,
};

enum class AlnsPipeline {
    AlnsOnly,
    RoutePool,
    RoutePoolFeedback,
    RoutePoolFeedbackDiverse,
    Deterministic,
    DeterministicProof,
    DeterministicProof4x,
    DeterministicProof8x,
    DeterministicProofRoutePool,
};

constexpr std::array<Strategy, 14> kStrategies = {
    Strategy::WaitOnly,
    Strategy::GreedyOneDay,
    Strategy::TopFcWeighted,
    Strategy::RollingAlnsUnshielded,
    Strategy::RollingHalnsRoutePool,
    Strategy::RollingHalnsFeedback,
    Strategy::RollingHalnsFeedbackDiverse,
    Strategy::EventConflict,
    Strategy::BackwardDeadline,
    Strategy::MacroMcts,
    Strategy::BlankPortfolio,
    Strategy::UdonShieldFixedRoles,
    Strategy::UdonShieldFeedback,
    Strategy::UdonShield,
};

struct SpotSpec {
    std::int32_t brand = 0;
    udon::CellId position = udon::kInvalidCell;
    std::int32_t stock = 1;
};

struct FixtureSpec {
    std::string name;
    std::string family = "fixed";
    std::uint64_t seed = 0;
    std::int32_t width = 8;
    std::int32_t height = 8;
    std::vector<std::int32_t> terrain;
    std::vector<SpotSpec> spots;
    std::vector<udon::CellId> starts{8, 9, 10, 11};
    std::vector<std::int32_t> daySteps{16, 16, 16, 16};
    std::int32_t fuelLimit = 16;
    std::int32_t players = 3;
    std::int32_t busyThreshold = 3;
    std::int32_t jammedThreshold = 6;
};

struct PlanOutcome {
    udon::MasterCandidate candidate;
    udon::MasterDiagnostics master;
    udon::AlnsDiagnostics alns;
    udon::blank_slate::Diagnostics blank;
    std::optional<udon::DecisionResult> fullDecision;
    std::int32_t feedbackRounds = 0;
    std::int32_t feedbackImprovements = 0;
    bool emergency = false;
    bool staticManifest = false;
};

struct RunMetrics {
    std::string fixture;
    std::string family;
    std::uint64_t seed = 0;
    std::string strategy;
    udon::OfficialScore score;
    std::vector<std::int64_t> responseTimes;
    std::int64_t roleTime = 0;
    std::uint32_t roleMask = 0;
    std::int32_t patrolCount = 0;
    std::int64_t combinationsVisited = 0;
    std::int64_t branchesPruned = 0;
    std::int64_t capCuts = 0;
    std::int64_t prefixCuts = 0;
    std::int64_t hotspotPromotions = 0;
    std::int64_t synthesizedRoutes = 0;
    std::int64_t pooledRoutes = 0;
    std::int64_t recombinationImprovements = 0;
    std::int64_t feedbackRounds = 0;
    std::int64_t feedbackImprovements = 0;
    std::int64_t blankRoutes = 0;
    std::int64_t blankPlans = 0;
    std::int64_t blankConflicts = 0;
    std::int64_t blankNodes = 0;
    std::int64_t blankRollouts = 0;
    std::int32_t invalidPlans = 0;
    std::int32_t emergencyDays = 0;
    std::int32_t staticManifestDays = 0;
    std::int32_t currentScoreSacrificeDays = 0;
    std::array<std::int64_t, 4> todayOpenTierCounts{};
    std::array<std::int64_t, 4> horizonOpenTierCounts{};
};

[[nodiscard]] std::int32_t score_component(const udon::OfficialScore& score, std::size_t tier) {
    switch (tier) {
    case 0:
        return score.lifetimeDistinct;
    case 1:
        return score.totalDailyDistinct;
    default:
        return score.totalServings;
    }
}

[[nodiscard]] std::int32_t first_open_tier(
    const udon::OfficialScore& lowerBound,
    const udon::OfficialScore& upperBound) {
    if (udon::compare_lexicographic(upperBound, lowerBound) < 0) {
        return 0;
    }
    for (std::size_t tier = 0; tier < 3U; ++tier) {
        if (score_component(upperBound, tier) > score_component(lowerBound, tier)) {
            return static_cast<std::int32_t>(tier + 1U);
        }
    }
    return 0;
}

[[nodiscard]] const char* strategy_name(Strategy strategy) {
    switch (strategy) {
    case Strategy::WaitOnly:
        return "wait-only";
    case Strategy::GreedyOneDay:
        return "greedy-one-day";
    case Strategy::TopFcWeighted:
        return "top-fc-weighted-rendezvous";
    case Strategy::RollingAlnsUnshielded:
        return "rolling-alns-unshielded";
    case Strategy::RollingHalnsRoutePool:
        return "rolling-halns-route-pool";
    case Strategy::RollingHalnsFeedback:
        return "rolling-halns-feedback";
    case Strategy::RollingHalnsFeedbackDiverse:
        return "rolling-halns-feedback-diverse";
    case Strategy::EventConflict:
        return "blank-event-conflict";
    case Strategy::BackwardDeadline:
        return "blank-backward-deadline";
    case Strategy::MacroMcts:
        return "blank-macro-mcts";
    case Strategy::BlankPortfolio:
        return "blank-portfolio";
    case Strategy::UdonShieldFixedRoles:
        return "udon-shield-fixed-roles";
    case Strategy::UdonShieldFeedback:
        return "udon-shield-feedback";
    case Strategy::UdonShieldProofExpanded:
        return "udon-shield-proof-8x";
    case Strategy::UdonShield:
        return "udon-shield";
    case Strategy::UdonShieldMultiHarvest:
        return "udon-shield-multi-harvest";
    case Strategy::UdonShieldDeepHarvest:
        return "udon-shield-deep-harvest";
    case Strategy::UdonShieldAdaptiveDepth4:
        return "udon-shield-adaptive-depth-4";
    case Strategy::UdonShieldOrienteering:
        return "udon-shield-orienteering";
    case Strategy::UdonShieldDeepHarvestCentralRoles:
        return "udon-shield-deep-harvest-central-roles";
    case Strategy::UdonShieldDeepHarvestAdaptiveRoles3:
        return "udon-shield-deep-harvest-adaptive-roles-3";
    case Strategy::UdonShieldDeepHarvestAdaptiveRoles:
        return "udon-shield-deep-harvest-adaptive-roles";
    case Strategy::DeterministicAlns:
        return "deterministic-alns";
    case Strategy::DeterministicProofAlns:
        return "deterministic-proof-alns";
    case Strategy::DeterministicProofAlns4x:
        return "deterministic-proof-alns-4x";
    case Strategy::DeterministicProofAlns8x:
        return "deterministic-proof-alns-8x";
    case Strategy::DeterministicProofRoutePool:
        return "deterministic-proof-route-pool";
    }
    return "unknown";
}

[[nodiscard]] bool is_blank_strategy(Strategy strategy) {
    return strategy == Strategy::EventConflict ||
        strategy == Strategy::BackwardDeadline ||
        strategy == Strategy::MacroMcts ||
        strategy == Strategy::BlankPortfolio;
}

[[nodiscard]] udon::DeadlineCalibration deadline_calibration(Strategy strategy) {
    udon::DeadlineCalibration calibration;
    if (strategy == Strategy::UdonShieldProofExpanded) {
        calibration.normalProofGuidedPasses = 8;
        calibration.longProofGuidedPasses = 8;
    }
    return calibration;
}

[[nodiscard]] udon::blank_slate::Method blank_method(Strategy strategy) {
    switch (strategy) {
    case Strategy::BackwardDeadline:
        return udon::blank_slate::Method::BackwardDeadline;
    case Strategy::MacroMcts:
        return udon::blank_slate::Method::MacroMcts;
    case Strategy::BlankPortfolio:
        return udon::blank_slate::Method::Portfolio;
    default:
        return udon::blank_slate::Method::EventConflict;
    }
}

[[nodiscard]] std::vector<std::int32_t> plain_map(std::size_t cellCount = 64U) {
    return std::vector<std::int32_t>(cellCount, static_cast<std::int32_t>(udon::Terrain::Plain));
}

void preserve_plain_cells(FixtureSpec& fixture) {
    for (const udon::CellId start : fixture.starts) {
        fixture.terrain.at(static_cast<std::size_t>(start)) = static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    for (const SpotSpec& spot : fixture.spots) {
        fixture.terrain.at(static_cast<std::size_t>(spot.position)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }
}

[[nodiscard]] FixtureSpec balanced_fixture() {
    FixtureSpec fixture;
    fixture.name = "balanced-random-seed-17";
    fixture.family = "balanced";
    fixture.terrain = plain_map();
    fixture.spots = {
        {10, 16, 2},
        {11, 19, 2},
        {12, 23, 1},
        {13, 34, 2},
        {14, 38, 1},
        {15, 45, 2},
        {10, 54, 2},
        {11, 61, 2},
    };
    for (std::int32_t cell = 0; cell < 64; ++cell) {
        if ((cell * 17 + 5) % 19 == 0) {
            fixture.terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Mountain);
        } else if ((cell * 11 + 3) % 13 == 0) {
            fixture.terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Road);
        }
    }
    preserve_plain_cells(fixture);
    return fixture;
}

[[nodiscard]] FixtureSpec rare_brand_fixture() {
    FixtureSpec fixture;
    fixture.name = "rare-brand-dead-end";
    fixture.family = "rare-brand";
    fixture.terrain = plain_map();
    fixture.spots = {
        {10, 16, 2},
        {11, 20, 2},
        {12, 27, 1},
        {13, 36, 2},
        {10, 44, 2},
        {11, 52, 1},
        {99, 63, 1},
    };
    for (const udon::CellId cell : {17, 25, 33, 41, 49, 55}) {
        fixture.terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Road);
    }
    fixture.terrain.at(54U) = static_cast<std::int32_t>(udon::Terrain::Pond);
    fixture.terrain.at(62U) = static_cast<std::int32_t>(udon::Terrain::Pond);
    preserve_plain_cells(fixture);
    return fixture;
}

[[nodiscard]] FixtureSpec threshold_fixture() {
    FixtureSpec fixture;
    fixture.name = "road-threshold-crossing";
    fixture.family = "threshold-corridor";
    fixture.terrain = plain_map();
    fixture.spots = {
        {10, 16, 2},
        {11, 22, 2},
        {12, 32, 2},
        {13, 39, 2},
        {14, 48, 2},
        {15, 55, 2},
        {16, 62, 1},
    };
    for (udon::CellId cell = 24; cell <= 31; ++cell) {
        fixture.terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Road);
    }
    fixture.busyThreshold = 2;
    fixture.jammedThreshold = 4;
    preserve_plain_cells(fixture);
    return fixture;
}

[[nodiscard]] FixtureSpec fuel_tight_fixture() {
    FixtureSpec fixture;
    fixture.name = "fuel-tight-rendezvous";
    fixture.family = "fuel-tight";
    fixture.terrain = plain_map();
    fixture.spots = {
        {10, 16, 1},
        {11, 23, 1},
        {12, 31, 1},
        {13, 39, 1},
        {14, 47, 1},
        {15, 55, 1},
        {16, 63, 1},
    };
    for (const udon::CellId cell : {18, 26, 34, 42, 50, 58}) {
        fixture.terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Mountain);
    }
    for (const udon::CellId cell : {17, 25, 33, 41, 49, 57}) {
        fixture.terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Road);
    }
    fixture.fuelLimit = 8;
    preserve_plain_cells(fixture);
    return fixture;
}

[[nodiscard]] FixtureSpec high_stock_fixture() {
    FixtureSpec fixture;
    fixture.name = "high-stock-multi-patrol";
    fixture.family = "high-stock";
    fixture.terrain = plain_map();
    fixture.spots = {
        {10, 16, 4},
        {11, 21, 4},
        {12, 30, 3},
        {13, 39, 4},
        {14, 48, 3},
        {15, 57, 4},
    };
    for (const udon::CellId cell : {18, 19, 26, 27, 34, 35, 42, 43, 50, 51}) {
        fixture.terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Road);
    }
    preserve_plain_cells(fixture);
    return fixture;
}

[[nodiscard]] FixtureSpec overnight_fixture() {
    FixtureSpec fixture;
    fixture.name = "overnight-terminal-value";
    fixture.family = "overnight";
    fixture.terrain = plain_map();
    fixture.daySteps = {16, 16, 16, 16};
    fixture.spots = {
        {10, 16, 1},
        {11, 24, 1},
        {12, 33, 2},
        {13, 42, 2},
        {14, 51, 2},
        {15, 60, 2},
        {16, 63, 1},
    };
    for (const udon::CellId cell : {17, 25, 26, 34, 35, 43, 44, 52, 53, 61}) {
        fixture.terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Road);
    }
    fixture.fuelLimit = 12;
    preserve_plain_cells(fixture);
    return fixture;
}

[[nodiscard]] std::vector<FixtureSpec> fixtures() {
    return {
        balanced_fixture(),
        rare_brand_fixture(),
        threshold_fixture(),
        fuel_tight_fixture(),
        high_stock_fixture(),
        overnight_fixture(),
    };
}

[[nodiscard]] FixtureSpec generated_fixture(std::uint64_t seed) {
    static constexpr std::array<const char*, 6> families{
        "balanced",
        "rare-brand",
        "threshold-corridor",
        "fuel-tight",
        "high-stock",
        "overnight",
    };
    FixtureSpec fixture;
    fixture.seed = seed;
    fixture.family = families.at(static_cast<std::size_t>(seed % families.size()));
    fixture.name = fixture.family + "-seed-" + std::to_string(seed);
    fixture.terrain = plain_map();

    std::mt19937_64 random(seed ^ 0x9e3779b97f4a7c15ULL);
    std::vector<udon::CellId> cells(64U);
    std::iota(cells.begin(), cells.end(), 0);
    std::shuffle(cells.begin(), cells.end(), random);
    fixture.starts.assign(cells.begin(), cells.begin() + 4);

    const std::int32_t spotCount = 6 + static_cast<std::int32_t>(random() % 3U);
    const std::int32_t brandCount = 5 + static_cast<std::int32_t>(random() % 3U);
    fixture.spots.reserve(static_cast<std::size_t>(spotCount));
    for (std::int32_t spotIndex = 0; spotIndex < spotCount; ++spotIndex) {
        std::int32_t brandIndex = spotIndex % brandCount;
        if (fixture.family == "rare-brand") {
            brandIndex = spotIndex == spotCount - 1
                ? brandCount - 1
                : spotIndex % std::max(1, brandCount - 1);
        }
        std::int32_t stock = 1 + static_cast<std::int32_t>(random() % 2U);
        if (fixture.family == "high-stock") {
            stock = 3 + static_cast<std::int32_t>(random() % 3U);
        }
        fixture.spots.push_back(SpotSpec{
            100 + brandIndex,
            cells.at(static_cast<std::size_t>(4 + spotIndex)),
            stock,
        });
    }

    std::int32_t roadPercent = 16;
    std::int32_t mountainPercent = 10;
    if (fixture.family == "threshold-corridor") {
        roadPercent = 34;
        mountainPercent = 6;
    } else if (fixture.family == "fuel-tight") {
        roadPercent = 24;
        mountainPercent = 22;
    } else if (fixture.family == "overnight") {
        roadPercent = 22;
        mountainPercent = 16;
    }
    for (std::int32_t cell = 0; cell < 64; ++cell) {
        const std::int32_t draw = static_cast<std::int32_t>(random() % 100U);
        if (draw < roadPercent) {
            fixture.terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Road);
        } else if (draw < roadPercent + mountainPercent) {
            fixture.terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Mountain);
        }
    }

    fixture.daySteps.assign(
        fixture.family == "overnight" ? 5U : 4U,
        16 + static_cast<std::int32_t>(random() % 5U));
    fixture.fuelLimit = fixture.family == "fuel-tight"
        ? 8 + static_cast<std::int32_t>(random() % 3U)
        : 14 + static_cast<std::int32_t>(random() % 7U);
    fixture.players = 2 + static_cast<std::int32_t>(random() % 4U);
    fixture.busyThreshold = 2 + static_cast<std::int32_t>(random() % 3U);
    fixture.jammedThreshold = fixture.busyThreshold + 2 +
        static_cast<std::int32_t>(random() % 3U);
    preserve_plain_cells(fixture);
    return fixture;
}

[[nodiscard]] FixtureSpec generated_btc_large_fixture(std::uint64_t seed) {
    static constexpr std::array<const char*, 6> families{
        "balanced",
        "rare-brand",
        "threshold-corridor",
        "fuel-tight",
        "high-stock",
        "overnight",
    };
    constexpr std::int32_t side = 32;
    constexpr std::int32_t cellCount = side * side;
    FixtureSpec fixture;
    fixture.seed = seed;
    fixture.family = families.at(static_cast<std::size_t>(seed % families.size()));
    fixture.name = "btc-large-" + fixture.family + "-seed-" + std::to_string(seed);
    fixture.width = side;
    fixture.height = side;
    fixture.terrain = plain_map(static_cast<std::size_t>(cellCount));

    std::mt19937_64 random(seed ^ 0xd1b54a32d192ed03ULL);
    std::vector<udon::CellId> roadCells;
    roadCells.reserve(63U);
    const auto add_road = [&fixture, &roadCells](std::int32_t row, std::int32_t column) {
        const udon::CellId cell = row * side + column;
        if (fixture.terrain.at(static_cast<std::size_t>(cell)) !=
            static_cast<std::int32_t>(udon::Terrain::Road)) {
            fixture.terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Road);
            roadCells.push_back(cell);
        }
    };
    std::int32_t column = 7 + static_cast<std::int32_t>(random() % 18U);
    for (std::int32_t row = 0; row < side; ++row) {
        column = std::clamp(
            column + static_cast<std::int32_t>(random() % 3U) - 1,
            1,
            side - 2);
        add_road(row, column);
    }
    std::int32_t row = 7 + static_cast<std::int32_t>(random() % 18U);
    for (column = 0; column < side; ++column) {
        row = std::clamp(
            row + static_cast<std::int32_t>(random() % 3U) - 1,
            1,
            side - 2);
        add_road(row, column);
    }
    for (std::int32_t cell = 0;
         roadCells.size() < 63U && cell < cellCount;
         ++cell) {
        const udon::CellId candidate = static_cast<udon::CellId>(
            (cell * 37 + static_cast<std::int32_t>(seed % 31U)) % cellCount);
        add_road(candidate / side, candidate % side);
    }

    std::vector<udon::CellId> cells(static_cast<std::size_t>(cellCount));
    std::iota(cells.begin(), cells.end(), 0);
    std::shuffle(cells.begin(), cells.end(), random);
    std::int32_t mountains = 0;
    std::int32_t ponds = 0;
    for (const udon::CellId cell : cells) {
        std::int32_t& terrain = fixture.terrain.at(static_cast<std::size_t>(cell));
        if (terrain != static_cast<std::int32_t>(udon::Terrain::Plain)) {
            continue;
        }
        if (mountains < 56) {
            terrain = static_cast<std::int32_t>(udon::Terrain::Mountain);
            ++mountains;
        } else if (ponds < 5) {
            terrain = static_cast<std::int32_t>(udon::Terrain::Pond);
            ++ponds;
        } else {
            break;
        }
    }

    std::shuffle(cells.begin(), cells.end(), random);
    std::vector<udon::CellId> plainCells;
    plainCells.reserve(cells.size());
    for (const udon::CellId cell : cells) {
        if (fixture.terrain.at(static_cast<std::size_t>(cell)) ==
            static_cast<std::int32_t>(udon::Terrain::Plain)) {
            plainCells.push_back(cell);
        }
    }
    fixture.starts.assign(plainCells.begin(), plainCells.begin() + 8);
    fixture.spots.reserve(12U);
    for (std::int32_t spotIndex = 0; spotIndex < 12; ++spotIndex) {
        std::int32_t brandIndex = spotIndex % 6;
        if (fixture.family == "rare-brand" && spotIndex < 11) {
            brandIndex = spotIndex % 5;
        }
        std::int32_t stock = 1 + static_cast<std::int32_t>(random() % 8U);
        if (fixture.family == "high-stock") {
            stock = 5 + static_cast<std::int32_t>(random() % 4U);
        }
        fixture.spots.push_back(SpotSpec{
            brandIndex,
            plainCells.at(static_cast<std::size_t>(8 + spotIndex)),
            stock,
        });
    }
    fixture.daySteps.assign(10U, 100);
    fixture.fuelLimit = fixture.family == "fuel-tight" ? 120 : 200;
    fixture.players = 4;
    fixture.busyThreshold = 5;
    fixture.jammedThreshold = 10;
    return fixture;
}

[[nodiscard]] FixtureSpec generated_btc_highfuel_fixture(std::uint64_t seed) {
    FixtureSpec fixture = generated_btc_large_fixture(seed);
    fixture.family = "high-fuel-" + fixture.family;
    fixture.name = "btc-highfuel-" + fixture.name;
    fixture.fuelLimit = 3 * fixture.daySteps.front();
    return fixture;
}

[[nodiscard]] std::string config_document(const FixtureSpec& fixture) {
    std::ostringstream output;
    output << "{\"startsAt\":1778227200,\"daySeconds\":[";
    for (std::size_t day = 0; day < fixture.daySteps.size(); ++day) {
        if (day != 0U) {
            output << ',';
        }
        output << 5;
    }
    output << "],\"daySteps\":[";
    for (std::size_t day = 0; day < fixture.daySteps.size(); ++day) {
        if (day != 0U) {
            output << ',';
        }
        output << fixture.daySteps.at(day);
    }
    output << "],\"map\":{\"height\":" << fixture.height
           << ",\"width\":" << fixture.width << ",\"cells\":[";
    for (std::int32_t row = 0; row < fixture.height; ++row) {
        if (row != 0) {
            output << ',';
        }
        output << '[';
        for (std::int32_t column = 0; column < fixture.width; ++column) {
            if (column != 0) {
                output << ',';
            }
            output << fixture.terrain.at(static_cast<std::size_t>(row * fixture.width + column));
        }
        output << ']';
    }
    output << "]},\"spots\":[";
    for (std::size_t index = 0; index < fixture.spots.size(); ++index) {
        if (index != 0U) {
            output << ',';
        }
        const SpotSpec& spot = fixture.spots.at(index);
        output << "{\"brand\":" << spot.brand
               << ",\"pos\":" << spot.position
               << ",\"stocks\":" << spot.stock << '}';
    }
    output << "],\"agents\":[";
    for (std::size_t index = 0; index < fixture.starts.size(); ++index) {
        if (index != 0U) {
            output << ',';
        }
        output << fixture.starts.at(index);
    }
    output << "],\"fuelLimits\":" << fixture.fuelLimit
           << ",\"players\":" << fixture.players << ",\"busyThreshold\":" << fixture.busyThreshold
           << ",\"jammedThreshold\":" << fixture.jammedThreshold << '}';
    return output.str();
}

[[nodiscard]] std::uint64_t mix64(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

[[nodiscard]] std::vector<std::vector<std::int32_t>> opponent_footprints(
    const FixtureSpec& fixture,
    const udon::MatchConfig& config) {
    std::vector<std::vector<std::int32_t>> footprints(
        static_cast<std::size_t>(config.day_count()),
        std::vector<std::int32_t>(static_cast<std::size_t>(config.map.cell_count()), 0));
    for (std::int32_t day = 1; day <= config.day_count(); ++day) {
        for (std::size_t roadIndex = 0; roadIndex < config.roadCells.size(); ++roadIndex) {
            const udon::CellId road = config.roadCells.at(roadIndex);
            const std::uint64_t draw = mix64(
                fixture.seed ^
                (static_cast<std::uint64_t>(day) << 48U) ^
                (static_cast<std::uint64_t>(road) << 16U) ^
                static_cast<std::uint64_t>(roadIndex));
            const std::int32_t busyMass = config.players * config.busyThreshold;
            const std::int32_t jammedMass = config.players * config.jammedThreshold;
            std::int32_t stays = static_cast<std::int32_t>(
                draw % static_cast<std::uint64_t>(std::max(1, jammedMass + 3)));
            if (fixture.family == "threshold-corridor") {
                stays = busyMass - 2 + static_cast<std::int32_t>(draw % 7U);
                if ((roadIndex + static_cast<std::size_t>(day)) % 4U == 0U) {
                    stays = jammedMass - 2 + static_cast<std::int32_t>(draw % 5U);
                }
            } else if (fixture.family == "balanced") {
                stays /= 2;
            } else if (fixture.family == "overnight" && day % 2 == 0) {
                stays += config.players;
            }
            footprints.at(static_cast<std::size_t>(day - 1)).at(static_cast<std::size_t>(road)) =
                std::max(0, stays);
        }
    }
    return footprints;
}

[[nodiscard]] std::vector<udon::RoadStatus> road_statuses(
    const udon::MatchConfig& config,
    std::int32_t dayNumber,
    const std::vector<std::vector<std::int32_t>>& ownFootprints,
    const std::vector<std::vector<std::int32_t>>& opponentFootprints) {
    std::vector<udon::RoadStatus> statuses(
        static_cast<std::size_t>(config.map.cell_count()),
        udon::RoadStatus::Smooth);
    for (const udon::CellId road : config.roadCells) {
        std::int32_t stays = 0;
        for (std::int32_t offset = 1; offset <= 2; ++offset) {
            const std::int32_t completedDay = dayNumber - offset;
            if (completedDay <= 0) {
                continue;
            }
            const std::size_t historyIndex = static_cast<std::size_t>(completedDay - 1);
            stays += ownFootprints.at(historyIndex).at(static_cast<std::size_t>(road));
            stays += opponentFootprints.at(historyIndex).at(static_cast<std::size_t>(road));
        }
        udon::RoadStatus status = udon::RoadStatus::Smooth;
        if (stays >= config.players * config.jammedThreshold) {
            status = udon::RoadStatus::Jammed;
        } else if (stays >= config.players * config.busyThreshold) {
            status = udon::RoadStatus::Busy;
        }
        statuses.at(static_cast<std::size_t>(road)) = status;
    }
    return statuses;
}

[[nodiscard]] std::vector<udon::AgentKind> fixed_roles(const udon::MatchConfig& config, bool tanker) {
    std::vector<udon::AgentKind> roles(
        static_cast<std::size_t>(config.agent_count()),
        udon::AgentKind::Patrol);
    if (tanker && !roles.empty()) {
        roles.back() = udon::AgentKind::Tanker;
    }
    return roles;
}

[[nodiscard]] std::int64_t weighted_value(const udon::MasterCandidate& candidate) {
    return static_cast<std::int64_t>(candidate.scoreAfterToday.lifetimeDistinct) * 1000 +
        static_cast<std::int64_t>(candidate.scoreAfterToday.totalDailyDistinct) * 40 +
        candidate.scoreAfterToday.totalServings;
}

[[nodiscard]] bool better_candidate(
    const udon::MasterCandidate& left,
    const udon::MasterCandidate& right) {
    const std::int32_t scoreOrder = udon::compare_lexicographic(left.scoreAfterToday, right.scoreAfterToday);
    if (scoreOrder != 0) {
        return scoreOrder > 0;
    }
    const std::int32_t slackOrder = udon::compare_terminal_slack(left.terminalSlack, right.terminalSlack);
    if (slackOrder != 0) {
        return slackOrder > 0;
    }
    return left.stableId < right.stableId;
}

void merge_master_diagnostics(
    udon::MasterDiagnostics& target,
    const udon::MasterDiagnostics& addition) {
    target.combinationsVisited += addition.combinationsVisited;
    target.simulatorValidCombinations += addition.simulatorValidCombinations;
    target.branchesPruned += addition.branchesPruned;
    target.stockCapacityConflicts += addition.stockCapacityConflicts;
    target.prefixConflicts += addition.prefixConflicts;
    target.hotspotPromotions += addition.hotspotPromotions;
    target.capCuts += addition.capCuts;
    target.prefixCuts += addition.prefixCuts;
    target.cutRounds += addition.cutRounds;
    target.stockCreditDenials += addition.stockCreditDenials;
    target.exactCreditMismatches += addition.exactCreditMismatches;
    target.criticalRoadPromotions += addition.criticalRoadPromotions;
    target.deadlineReached = target.deadlineReached || addition.deadlineReached;
    target.searchComplete = target.searchComplete && addition.searchComplete;
    if (udon::compare_lexicographic(addition.optimisticUpperBound, target.optimisticUpperBound) > 0) {
        target.optimisticUpperBound = addition.optimisticUpperBound;
    }
}

void merge_alns_diagnostics(
    udon::AlnsDiagnostics& target,
    const udon::AlnsDiagnostics& addition) {
    target.iterations += addition.iterations;
    target.accepted += addition.accepted;
    target.improvements += addition.improvements;
    target.synthesizedRoutes += addition.synthesizedRoutes;
    target.synthesizedAccepted += addition.synthesizedAccepted;
    target.proofGuidedIterations += addition.proofGuidedIterations;
    target.proofGuidedRoutes += addition.proofGuidedRoutes;
    target.proofGuidedAccepted += addition.proofGuidedAccepted;
    target.proofGuidedImprovements += addition.proofGuidedImprovements;
    target.poolRoutesConsidered += addition.poolRoutesConsidered;
    target.poolNovelRoutes += addition.poolNovelRoutes;
    target.poolRetainedRoutes += addition.poolRetainedRoutes;
    target.recombinationCandidates += addition.recombinationCandidates;
    target.recombinationImprovements += addition.recombinationImprovements;
    target.recombinationDeadlineSkipped =
        target.recombinationDeadlineSkipped || addition.recombinationDeadlineSkipped;
    for (std::size_t index = 0; index < target.attemptedByOperator.size(); ++index) {
        target.attemptedByOperator.at(index) += addition.attemptedByOperator.at(index);
        target.acceptedByOperator.at(index) += addition.acceptedByOperator.at(index);
    }
}

void merge_candidates(
    std::vector<udon::MasterCandidate>& candidates,
    std::vector<udon::MasterCandidate> additions,
    std::int32_t maximumCandidates) {
    candidates.insert(
        candidates.end(),
        std::make_move_iterator(additions.begin()),
        std::make_move_iterator(additions.end()));
    std::sort(candidates.begin(), candidates.end(), better_candidate);
    candidates.erase(
        std::unique(
            candidates.begin(),
            candidates.end(),
            [](const udon::MasterCandidate& left, const udon::MasterCandidate& right) {
                return left.stableId == right.stableId;
            }),
        candidates.end());
    if (static_cast<std::int32_t>(candidates.size()) > maximumCandidates) {
        candidates.resize(static_cast<std::size_t>(maximumCandidates));
    }
}

class StrategyPlanner {
public:
    StrategyPlanner(
        const udon::MatchConfig& config,
        Strategy strategy,
        std::chrono::milliseconds budget)
        : config_(config),
          strategy_(strategy),
          budget_(budget),
          router_(config_),
          generator_(config_, router_),
          simulator_(config_),
          validator_(config_),
          master_(config_, simulator_, validator_),
          alns_(config_, master_),
          greedy_(config_, generator_, master_),
          blank_(config_, blank_method(strategy_)),
          full_(
              config_,
              {},
              deadline_calibration(strategy_),
              strategy_ == Strategy::UdonShieldFeedback
                  ? udon::RoutePoolSearch::Feedback
                  : udon::RoutePoolSearch::SinglePass,
              (strategy_ == Strategy::UdonShieldDeepHarvest ||
               strategy_ == Strategy::UdonShieldAdaptiveDepth4 ||
               strategy_ == Strategy::UdonShieldOrienteering ||
               strategy_ == Strategy::UdonShieldDeepHarvestCentralRoles ||
               strategy_ == Strategy::UdonShieldDeepHarvestAdaptiveRoles3 ||
               strategy_ == Strategy::UdonShieldDeepHarvestAdaptiveRoles)
                  ? (strategy_ == Strategy::UdonShieldOrienteering
                         ? 6
                         : (strategy_ == Strategy::UdonShieldAdaptiveDepth4
                                ? 5
                                : 4))
                  : (strategy_ == Strategy::UdonShieldMultiHarvest ? 3 : 2),
              false,
              strategy_ == Strategy::UdonShieldOrienteering ? 5 : -1) {}

    [[nodiscard]] std::vector<udon::AgentKind> select_roles(std::int64_t& elapsedMs) {
        const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
        std::vector<udon::AgentKind> roles;
        if (is_blank_strategy(strategy_)) {
            roles = blank_.select_roles();
        } else if (strategy_ == Strategy::UdonShieldDeepHarvestCentralRoles) {
            roles = blank_.select_roles();
        } else if (strategy_ == Strategy::UdonShieldDeepHarvestAdaptiveRoles3) {
            const std::vector<udon::RoleAssignment> assignments =
                full_.select_roles_until(
                    std::min(budget_, std::chrono::milliseconds{4000}),
                    3);
            roles = assignments.empty()
                ? blank_.select_roles()
                : assignments.front().roles;
        } else if (strategy_ == Strategy::UdonShieldDeepHarvestAdaptiveRoles) {
            const std::vector<udon::RoleAssignment> assignments =
                full_.select_roles_until(
                    std::min(budget_, std::chrono::milliseconds{4000}),
                    8);
            roles = assignments.empty()
                ? blank_.select_roles()
                : assignments.front().roles;
        } else if (strategy_ == Strategy::UdonShield ||
            strategy_ == Strategy::UdonShieldMultiHarvest ||
            strategy_ == Strategy::UdonShieldDeepHarvest ||
            strategy_ == Strategy::UdonShieldAdaptiveDepth4 ||
            strategy_ == Strategy::UdonShieldOrienteering ||
            strategy_ == Strategy::UdonShieldFeedback ||
            strategy_ == Strategy::UdonShieldProofExpanded) {
            const std::vector<udon::RoleAssignment> assignments = full_.select_roles_until(
                std::min(budget_, std::chrono::milliseconds{500}),
                3);
            roles = assignments.empty() ? fixed_roles(config_, true) : assignments.front().roles;
        } else {
            const bool tanker =
                strategy_ == Strategy::TopFcWeighted ||
                strategy_ == Strategy::RollingAlnsUnshielded ||
                strategy_ == Strategy::RollingHalnsRoutePool ||
                strategy_ == Strategy::RollingHalnsFeedback ||
                strategy_ == Strategy::RollingHalnsFeedbackDiverse ||
                strategy_ == Strategy::UdonShieldFixedRoles ||
                strategy_ == Strategy::DeterministicAlns ||
                strategy_ == Strategy::DeterministicProofAlns ||
                strategy_ == Strategy::DeterministicProofAlns4x ||
                strategy_ == Strategy::DeterministicProofAlns8x ||
                strategy_ == Strategy::DeterministicProofRoutePool;
            roles = fixed_roles(config_, tanker);
        }
        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
        return roles;
    }

    [[nodiscard]] PlanOutcome plan(const udon::DayState& state, const udon::MatchLedger& ledger) {
        switch (strategy_) {
        case Strategy::WaitOnly:
            return wait_plan(state, ledger);
        case Strategy::GreedyOneDay:
            return greedy_plan(state, ledger);
        case Strategy::TopFcWeighted:
            return weighted_plan(state, ledger);
        case Strategy::RollingAlnsUnshielded:
            return alns_plan(state, ledger, AlnsPipeline::AlnsOnly);
        case Strategy::RollingHalnsRoutePool:
            return alns_plan(state, ledger, AlnsPipeline::RoutePool);
        case Strategy::RollingHalnsFeedback:
            return alns_plan(state, ledger, AlnsPipeline::RoutePoolFeedback);
        case Strategy::RollingHalnsFeedbackDiverse:
            return alns_plan(state, ledger, AlnsPipeline::RoutePoolFeedbackDiverse);
        case Strategy::EventConflict:
            return blank_plan(state, ledger);
        case Strategy::BackwardDeadline:
            return blank_plan(state, ledger);
        case Strategy::MacroMcts:
            return blank_plan(state, ledger);
        case Strategy::BlankPortfolio:
            return blank_plan(state, ledger);
        case Strategy::UdonShieldFixedRoles:
            return full_plan(state, ledger);
        case Strategy::UdonShieldFeedback:
            return full_plan(state, ledger);
        case Strategy::UdonShieldProofExpanded:
            return full_plan(state, ledger);
        case Strategy::UdonShield:
        case Strategy::UdonShieldMultiHarvest:
        case Strategy::UdonShieldDeepHarvest:
        case Strategy::UdonShieldAdaptiveDepth4:
        case Strategy::UdonShieldOrienteering:
        case Strategy::UdonShieldDeepHarvestCentralRoles:
        case Strategy::UdonShieldDeepHarvestAdaptiveRoles3:
        case Strategy::UdonShieldDeepHarvestAdaptiveRoles:
            return full_plan(state, ledger);
        case Strategy::DeterministicAlns:
            return alns_plan(state, ledger, AlnsPipeline::Deterministic);
        case Strategy::DeterministicProofAlns:
            return alns_plan(state, ledger, AlnsPipeline::DeterministicProof);
        case Strategy::DeterministicProofAlns4x:
            return alns_plan(state, ledger, AlnsPipeline::DeterministicProof4x);
        case Strategy::DeterministicProofAlns8x:
            return alns_plan(state, ledger, AlnsPipeline::DeterministicProof8x);
        case Strategy::DeterministicProofRoutePool:
            return alns_plan(state, ledger, AlnsPipeline::DeterministicProofRoutePool);
        }
        throw std::logic_error("unknown benchmark strategy");
    }

    void acknowledge(const PlanOutcome& outcome, std::chrono::milliseconds responseTime) {
        if (outcome.fullDecision.has_value()) {
            full_.record_submitted(*outcome.fullDecision, responseTime);
        }
    }

private:
    [[nodiscard]] PlanOutcome wait_plan(
        const udon::DayState& state,
        const udon::MatchLedger& ledger) const {
        PlanOutcome outcome;
        const std::optional<udon::MasterCandidate> wait = master_.evaluate_exact_plan(
            state,
            ledger,
            udon::emergency_wait_plan(config_, state));
        if (!wait.has_value()) {
            throw std::runtime_error("wait baseline failed exact validation");
        }
        outcome.candidate = *wait;
        return outcome;
    }

    [[nodiscard]] PlanOutcome exact_wait(
        const udon::DayState& state,
        const udon::MatchLedger& ledger,
        PlanOutcome outcome = {}) const {
        const std::optional<udon::MasterCandidate> wait = master_.evaluate_exact_plan(
            state,
            ledger,
            udon::emergency_wait_plan(config_, state));
        if (!wait.has_value()) {
            throw std::runtime_error("exact wait fallback failed");
        }
        outcome.candidate = *wait;
        outcome.emergency = true;
        return outcome;
    }

    [[nodiscard]] PlanOutcome greedy_plan(
        const udon::DayState& state,
        const udon::MatchLedger& ledger) const {
        PlanOutcome outcome;
        udon::ColumnGenerationOptions generation;
        generation.maximumPathsPerTarget = 1;
        generation.maximumColumnsPerAgent = 1;
        generation.maximumTargetSpots = 8;
        generation.maximumEscorts = 0;
        generation.maximumSeedPlans = 0;
        generation.deadline = std::chrono::steady_clock::now() + budget_;
        udon::MasterOptions masterOptions;
        masterOptions.maximumCombinations = 1;
        masterOptions.maximumCandidates = 1;
        masterOptions.maximumResolveRounds = 1;
        masterOptions.deadline = generation.deadline;
        try {
            outcome.candidate = greedy_.build_incumbent(
                state,
                ledger,
                generation,
                masterOptions,
                outcome.master);
            return outcome;
        } catch (const std::runtime_error&) {
            return exact_wait(state, ledger, outcome);
        }
    }

    [[nodiscard]] PlanOutcome weighted_plan(
        const udon::DayState& state,
        const udon::MatchLedger& ledger) const {
        PlanOutcome outcome;
        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + budget_;
        udon::ColumnGenerationOptions generation;
        generation.maximumPathsPerTarget = 3;
        generation.maximumColumnsPerAgent = 8;
        generation.maximumTargetSpots = 12;
        generation.maximumEscorts = 16;
        generation.deadline = deadline;
        const udon::RoutePortfolio portfolio = generator_.generate(state, ledger, generation);
        udon::MasterOptions masterOptions;
        masterOptions.maximumCombinations = 12000;
        masterOptions.maximumCandidates = 12000;
        masterOptions.maximumResolveRounds = 1;
        masterOptions.enableLexicographicBranchAndBound = false;
        masterOptions.deadline = deadline;
        const std::vector<udon::MasterCandidate> candidates = master_.solve(
            state,
            ledger,
            portfolio,
            masterOptions,
            outcome.master);
        if (candidates.empty()) {
            return exact_wait(state, ledger, outcome);
        }
        outcome.candidate = *std::max_element(
            candidates.begin(),
            candidates.end(),
            [](const udon::MasterCandidate& left, const udon::MasterCandidate& right) {
                const std::int64_t leftValue = weighted_value(left);
                const std::int64_t rightValue = weighted_value(right);
                if (leftValue != rightValue) {
                    return leftValue < rightValue;
                }
                return udon::compare_terminal_slack(left.terminalSlack, right.terminalSlack) < 0;
            });
        return outcome;
    }

    [[nodiscard]] PlanOutcome alns_plan(
        const udon::DayState& state,
        const udon::MatchLedger& ledger,
        AlnsPipeline pipeline) const {
        PlanOutcome outcome;
        const bool recombineRoutePool =
            pipeline == AlnsPipeline::RoutePool ||
            pipeline == AlnsPipeline::RoutePoolFeedback ||
            pipeline == AlnsPipeline::RoutePoolFeedbackDiverse ||
            pipeline == AlnsPipeline::DeterministicProofRoutePool;
        const bool deterministic =
            pipeline == AlnsPipeline::Deterministic ||
            pipeline == AlnsPipeline::DeterministicProof ||
            pipeline == AlnsPipeline::DeterministicProof4x ||
            pipeline == AlnsPipeline::DeterministicProof8x ||
            pipeline == AlnsPipeline::DeterministicProofRoutePool;
        const bool deterministicProof =
            pipeline == AlnsPipeline::DeterministicProof ||
            pipeline == AlnsPipeline::DeterministicProof4x ||
            pipeline == AlnsPipeline::DeterministicProof8x ||
            pipeline == AlnsPipeline::DeterministicProofRoutePool;
        const std::int32_t proofMultiplier =
            pipeline == AlnsPipeline::DeterministicProof8x ? 8 :
            (pipeline == AlnsPipeline::DeterministicProof4x ? 4 : 2);
        const bool deterministicRoutePool = pipeline == AlnsPipeline::DeterministicProofRoutePool;
        const bool boundedRecombination = recombineRoutePool && !deterministicRoutePool;
        const bool feedback =
            pipeline == AlnsPipeline::RoutePoolFeedback ||
            pipeline == AlnsPipeline::RoutePoolFeedbackDiverse;
        const bool diverse = pipeline == AlnsPipeline::RoutePoolFeedbackDiverse;
        const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
        const std::chrono::steady_clock::time_point masterDeadline =
            started + budget_ * (feedback ? 3 : (recombineRoutePool ? 2 : 1)) /
                (feedback ? 10 : (recombineRoutePool ? 5 : 2));
        const std::chrono::steady_clock::time_point firstAlnsDeadline =
            feedback ? started + budget_ * 11 / 20 :
            (recombineRoutePool ? started + budget_ * 3 / 4 : started + budget_);
        const std::chrono::steady_clock::time_point firstRecombinationDeadline =
            feedback ? started + budget_ * 7 / 10 : started + budget_;
        const std::chrono::steady_clock::time_point feedbackAlnsDeadline =
            started + budget_ * 9 / 10;
        const std::chrono::steady_clock::time_point deadline = started + budget_;
        udon::ColumnGenerationOptions generation;
        generation.maximumPathsPerTarget = 3;
        generation.maximumColumnsPerAgent = 8;
        generation.maximumTargetSpots = 12;
        generation.maximumEscorts = 16;
        generation.deadline = deterministic
            ? std::nullopt
            : std::optional<std::chrono::steady_clock::time_point>{masterDeadline};
        udon::RoutePortfolio portfolio = generator_.generate(state, ledger, generation);
        udon::MasterOptions masterOptions;
        masterOptions.maximumCombinations = 20000;
        masterOptions.maximumCandidates = 32;
        masterOptions.deadline = deterministic
            ? std::nullopt
            : std::optional<std::chrono::steady_clock::time_point>{masterDeadline};
        std::vector<udon::MasterCandidate> candidates = master_.solve(
            state,
            ledger,
            portfolio,
            masterOptions,
            outcome.master);
        if (candidates.empty()) {
            return exact_wait(state, ledger, outcome);
        }
        udon::AlnsOptions alnsOptions;
        alnsOptions.maximumIterations = 96;
        alnsOptions.maximumAlternativesPerIteration = 6;
        alnsOptions.maximumCandidates = 32;
        alnsOptions.diversityCandidates = diverse ? 8 : 0;
        alnsOptions.maximumProofGuidedIterations = deterministicProof
            ? config_.agent_count() * proofMultiplier
            : 0;
        alnsOptions.proofUpperBound = outcome.master.optimisticUpperBound;
        alnsOptions.deadline = deterministic
            ? std::nullopt
            : std::optional<std::chrono::steady_clock::time_point>{firstAlnsDeadline};
        for (const udon::CellId road : config_.roadCells) {
            if (state.roadStatuses.at(static_cast<std::size_t>(road)) != udon::RoadStatus::Smooth) {
                alnsOptions.criticalRoads.push_back(road);
            }
        }
        candidates = alns_.improve(
            state,
            ledger,
            portfolio,
            std::move(candidates),
            alnsOptions,
            outcome.alns);
        if (deterministicRoutePool ||
            (boundedRecombination && std::chrono::steady_clock::now() < firstRecombinationDeadline)) {
            udon::RoutePoolAugmentation augmentation = generator_.augment_with_candidate_routes(
                state,
                std::move(portfolio),
                candidates,
                16);
            outcome.alns.poolRoutesConsidered = augmentation.routesConsidered;
            outcome.alns.poolNovelRoutes = augmentation.novelRoutes;
            outcome.alns.poolRetainedRoutes = augmentation.retainedNovelRoutes;
            if (augmentation.retainedNovelRoutes > 0) {
                udon::MasterOptions recombinationOptions = masterOptions;
                recombinationOptions.maximumCombinations = 20000;
                recombinationOptions.maximumResolveRounds = 2;
                recombinationOptions.deadline = deterministicRoutePool
                    ? std::nullopt
                    : std::optional<std::chrono::steady_clock::time_point>{firstRecombinationDeadline};
                udon::MasterDiagnostics recombinationDiagnostics;
                std::vector<udon::MasterCandidate> recombined = master_.solve(
                    state,
                    ledger,
                    augmentation.portfolio,
                    recombinationOptions,
                    recombinationDiagnostics);
                outcome.alns.recombinationCandidates = static_cast<std::int32_t>(recombined.size());
                if (!recombined.empty() && better_candidate(recombined.front(), candidates.front())) {
                    outcome.alns.recombinationImprovements = 1;
                }
                merge_candidates(
                    candidates,
                    std::move(recombined),
                    alnsOptions.maximumCandidates + (diverse ? alnsOptions.diversityCandidates : 0));
                merge_master_diagnostics(outcome.master, recombinationDiagnostics);
            }
            if (feedback && std::chrono::steady_clock::now() < feedbackAlnsDeadline) {
                const udon::MasterCandidate previousBest = candidates.front();
                udon::AlnsOptions feedbackOptions = alnsOptions;
                feedbackOptions.maximumIterations = 64;
                feedbackOptions.deadline = feedbackAlnsDeadline;
                udon::AlnsDiagnostics feedbackDiagnostics;
                candidates = alns_.improve(
                    state,
                    ledger,
                    augmentation.portfolio,
                    std::move(candidates),
                    feedbackOptions,
                    feedbackDiagnostics);
                ++outcome.feedbackRounds;
                outcome.feedbackImprovements +=
                    better_candidate(candidates.front(), previousBest) ? 1 : 0;
                merge_alns_diagnostics(outcome.alns, feedbackDiagnostics);
            }
            if (feedback && std::chrono::steady_clock::now() < deadline) {
                udon::RoutePoolAugmentation finalAugmentation =
                    generator_.augment_with_candidate_routes(
                        state,
                        std::move(augmentation.portfolio),
                        candidates,
                        16);
                outcome.alns.poolRoutesConsidered += finalAugmentation.routesConsidered;
                outcome.alns.poolNovelRoutes += finalAugmentation.novelRoutes;
                outcome.alns.poolRetainedRoutes += finalAugmentation.retainedNovelRoutes;
                if (finalAugmentation.retainedNovelRoutes > 0) {
                    udon::MasterOptions finalOptions = masterOptions;
                    finalOptions.maximumCombinations = 20000;
                    finalOptions.maximumResolveRounds = 2;
                    finalOptions.deadline = deadline;
                    udon::MasterDiagnostics finalDiagnostics;
                    std::vector<udon::MasterCandidate> finalCandidates = master_.solve(
                        state,
                        ledger,
                        finalAugmentation.portfolio,
                        finalOptions,
                        finalDiagnostics);
                    outcome.alns.recombinationCandidates +=
                        static_cast<std::int32_t>(finalCandidates.size());
                    if (!finalCandidates.empty() &&
                        better_candidate(finalCandidates.front(), candidates.front())) {
                        ++outcome.alns.recombinationImprovements;
                    }
                    merge_candidates(
                        candidates,
                        std::move(finalCandidates),
                        alnsOptions.maximumCandidates);
                    merge_master_diagnostics(outcome.master, finalDiagnostics);
                }
            } else if (feedback) {
                outcome.alns.recombinationDeadlineSkipped = true;
            }
        } else if (boundedRecombination) {
            outcome.alns.recombinationDeadlineSkipped = true;
        }
        outcome.candidate = candidates.front();
        return outcome;
    }

    [[nodiscard]] PlanOutcome full_plan(
        const udon::DayState& state,
        const udon::MatchLedger& ledger) {
        PlanOutcome outcome;
        udon::DecisionResult decision = full_.solve_day(state, ledger, budget_);
        outcome.candidate = decision.candidate;
        outcome.master = decision.diagnostics;
        outcome.alns = decision.alns;
        outcome.emergency = decision.emergency;
        outcome.staticManifest = decision.manifest.usedStaticFallback;
        outcome.fullDecision = std::move(decision);
        return outcome;
    }

    [[nodiscard]] PlanOutcome blank_plan(
        const udon::DayState& state,
        const udon::MatchLedger& ledger) {
        PlanOutcome outcome;
        const udon::DayPlan plan = blank_.solve_day(
            state,
            ledger,
            budget_,
            outcome.blank);
        const std::optional<udon::MasterCandidate> evaluated =
            master_.evaluate_exact_plan(state, ledger, plan);
        if (!evaluated.has_value()) {
            return exact_wait(state, ledger, outcome);
        }
        outcome.candidate = *evaluated;
        return outcome;
    }

    const udon::MatchConfig& config_;
    Strategy strategy_;
    std::chrono::milliseconds budget_;
    udon::ParetoRouter router_;
    udon::RouteColumnGenerator generator_;
    udon::ExactStepSimulator simulator_;
    udon::IndependentDayValidator validator_;
    udon::RouteMaster master_;
    udon::AdaptiveRouteImprover alns_;
    udon::GreedyPlanner greedy_;
    udon::blank_slate::Planner blank_;
    udon::UdonShieldEngine full_;
};

[[nodiscard]] RunMetrics run_strategy(
    const FixtureSpec& fixture,
    const udon::MatchConfig& config,
    Strategy strategy,
    std::chrono::milliseconds budget,
    const std::vector<udon::AgentKind>* commonRoles = nullptr) {
    RunMetrics metrics;
    metrics.fixture = fixture.name;
    metrics.family = fixture.family;
    metrics.seed = fixture.seed;
    metrics.strategy = strategy_name(strategy);
    StrategyPlanner planner(config, strategy, budget);
    std::vector<udon::AgentKind> roles = commonRoles == nullptr
        ? planner.select_roles(metrics.roleTime)
        : *commonRoles;
    metrics.patrolCount = static_cast<std::int32_t>(std::count(
        roles.begin(),
        roles.end(),
        udon::AgentKind::Patrol));
    for (std::size_t agentIndex = 0; agentIndex < roles.size(); ++agentIndex) {
        if (roles.at(agentIndex) == udon::AgentKind::Tanker) {
            metrics.roleMask |= std::uint32_t{1} <<
                static_cast<std::uint32_t>(agentIndex);
        }
    }
    std::vector<udon::AgentState> agents;
    agents.reserve(static_cast<std::size_t>(config.agent_count()));
    for (udon::AgentIndex agent = 0; agent < config.agent_count(); ++agent) {
        agents.push_back(udon::AgentState{
            roles.at(static_cast<std::size_t>(agent)),
            config.initialAgents.at(static_cast<std::size_t>(agent)),
            config.fuelLimit,
        });
    }
    udon::MatchLedger ledger;
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    std::vector<std::vector<std::int32_t>> ownFootprints(
        static_cast<std::size_t>(config.day_count()),
        std::vector<std::int32_t>(static_cast<std::size_t>(config.map.cell_count()), 0));
    const std::vector<std::vector<std::int32_t>> opponentFootprints =
        opponent_footprints(fixture, config);
    for (std::int32_t day = 1; day <= config.day_count(); ++day) {
        udon::DayState state;
        state.endsAt = config.startsAt + static_cast<std::int64_t>(day) * 5;
        state.dayNumber = day;
        state.agents = agents;
        state.roadStatuses = road_statuses(config, day, ownFootprints, opponentFootprints);
        const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
        PlanOutcome outcome = planner.plan(state, ledger);
        const std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started);
        metrics.responseTimes.push_back(elapsed.count());
        udon::SimulationResult detailed = simulator.simulate(state, outcome.candidate.plan, false);
        const udon::SimulationResult independent = validator.validate(state, outcome.candidate.plan, false);
        std::string mismatch;
        const bool acceptedOutcome = detailed.valid && validator.agrees_with(detailed, independent, mismatch);
        if (!acceptedOutcome) {
            ++metrics.invalidPlans;
            const udon::DayPlan wait = udon::emergency_wait_plan(config, state);
            detailed = simulator.simulate(state, wait, false);
            const udon::SimulationResult waitValidation = validator.validate(state, wait, false);
            if (!detailed.valid || !validator.agrees_with(detailed, waitValidation, mismatch)) {
                throw std::runtime_error("benchmark fallback validation failed: " + mismatch);
            }
        } else {
            planner.acknowledge(outcome, elapsed);
        }
        ledger.apply(detailed.score);
        agents = detailed.finalAgents;
        ownFootprints.at(static_cast<std::size_t>(day - 1)) = detailed.roadFootprint;
        metrics.combinationsVisited += outcome.master.combinationsVisited;
        metrics.branchesPruned += outcome.master.branchesPruned;
        metrics.capCuts += outcome.master.capCuts;
        metrics.prefixCuts += outcome.master.prefixCuts;
        metrics.hotspotPromotions += outcome.master.hotspotPromotions;
        metrics.synthesizedRoutes += outcome.alns.synthesizedRoutes;
        metrics.pooledRoutes += outcome.alns.poolRetainedRoutes;
        metrics.recombinationImprovements += outcome.alns.recombinationImprovements;
        metrics.feedbackRounds += outcome.feedbackRounds;
        metrics.feedbackImprovements += outcome.feedbackImprovements;
        metrics.blankRoutes += outcome.blank.routesGenerated;
        metrics.blankPlans += outcome.blank.plansEvaluated;
        metrics.blankConflicts += outcome.blank.resourceConflicts;
        metrics.blankNodes += outcome.blank.searchNodes;
        metrics.blankRollouts += outcome.blank.rollouts;
        metrics.emergencyDays += outcome.emergency ? 1 : 0;
        metrics.staticManifestDays += outcome.staticManifest ? 1 : 0;
        if (acceptedOutcome && outcome.fullDecision.has_value()) {
            const udon::OptimalityGapDiagnostics& gap = outcome.fullDecision->audit.optimalityGap;
            metrics.todayOpenTierCounts.at(gap.todayPortfolio.firstOpenTier) += 1;
            metrics.horizonOpenTierCounts.at(gap.viabilityHorizon.firstOpenTier) += 1;
            udon::OfficialScore bestAuditedCurrent = outcome.candidate.scoreAfterToday;
            for (const udon::CandidateAuditRecord& record : outcome.fullDecision->audit.candidates) {
                if (udon::compare_lexicographic(record.scoreAfterToday, bestAuditedCurrent) > 0) {
                    bestAuditedCurrent = record.scoreAfterToday;
                }
            }
            metrics.currentScoreSacrificeDays +=
                udon::compare_lexicographic(bestAuditedCurrent, outcome.candidate.scoreAfterToday) > 0 ? 1 : 0;
        } else if (acceptedOutcome) {
            const std::int32_t openTier = first_open_tier(
                outcome.candidate.scoreAfterToday,
                outcome.master.optimisticUpperBound);
            metrics.todayOpenTierCounts.at(static_cast<std::size_t>(openTier)) += 1;
        }
    }
    metrics.score = udon::OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
    return metrics;
}

[[nodiscard]] std::int64_t percentile(std::vector<std::int64_t> values, std::int32_t percent) {
    if (values.empty()) {
        return 0;
    }
    std::sort(values.begin(), values.end());
    const std::size_t index = std::min(
        values.size() - 1U,
        static_cast<std::size_t>(
            (static_cast<std::int64_t>(percent) * static_cast<std::int64_t>(values.size()) + 99) / 100 - 1));
    return values.at(index);
}

[[nodiscard]] std::pair<double, double> wilson_interval(
    std::int32_t successes,
    std::int32_t trials) {
    if (trials <= 0) {
        return {0.0, 1.0};
    }
    constexpr double z = 1.959963984540054;
    const double count = static_cast<double>(trials);
    const double rate = static_cast<double>(successes) / count;
    const double denominator = 1.0 + z * z / count;
    const double center = (rate + z * z / (2.0 * count)) / denominator;
    const double radius = z * std::sqrt(
        rate * (1.0 - rate) / count + z * z / (4.0 * count * count)) / denominator;
    return {std::max(0.0, center - radius), std::min(1.0, center + radius)};
}

[[nodiscard]] double two_sided_sign_p(std::int32_t wins, std::int32_t losses) {
    const std::int32_t trials = wins + losses;
    if (trials <= 0) {
        return 1.0;
    }
    const std::int32_t tail = std::min(wins, losses);
    long double term = std::pow(0.5L, trials);
    long double cumulative = term;
    for (std::int32_t count = 0; count < tail; ++count) {
        term *= static_cast<long double>(trials - count) /
            static_cast<long double>(count + 1);
        cumulative += term;
    }
    return static_cast<double>(std::min(1.0L, 2.0L * cumulative));
}

void print_result(const RunMetrics& metrics) {
    const std::int64_t totalTime = std::accumulate(
        metrics.responseTimes.begin(),
        metrics.responseTimes.end(),
        std::int64_t{0});
    const std::int64_t meanTime = metrics.responseTimes.empty()
        ? 0
        : totalTime / static_cast<std::int64_t>(metrics.responseTimes.size());
    std::cout << "result"
              << ",fixture=" << metrics.fixture
              << ",family=" << metrics.family
              << ",seed=" << metrics.seed
              << ",strategy=" << metrics.strategy
              << ",lifetime=" << metrics.score.lifetimeDistinct
              << ",daily=" << metrics.score.totalDailyDistinct
              << ",servings=" << metrics.score.totalServings
              << ",invalid=" << metrics.invalidPlans
              << ",role_ms=" << metrics.roleTime
              << ",role_mask=" << metrics.roleMask
              << ",patrols=" << metrics.patrolCount
              << ",mean_ms=" << meanTime
              << ",p50_ms=" << percentile(metrics.responseTimes, 50)
              << ",p95_ms=" << percentile(metrics.responseTimes, 95)
              << ",max_ms=" << percentile(metrics.responseTimes, 100)
              << ",combinations=" << metrics.combinationsVisited
              << ",branches_pruned=" << metrics.branchesPruned
              << ",cap_cuts=" << metrics.capCuts
              << ",prefix_cuts=" << metrics.prefixCuts
              << ",hotspot_promotions=" << metrics.hotspotPromotions
              << ",alns_synthesized=" << metrics.synthesizedRoutes
              << ",pooled_routes=" << metrics.pooledRoutes
              << ",recombination_improvements=" << metrics.recombinationImprovements
              << ",feedback_rounds=" << metrics.feedbackRounds
              << ",feedback_improvements=" << metrics.feedbackImprovements
              << ",blank_routes=" << metrics.blankRoutes
              << ",blank_plans=" << metrics.blankPlans
              << ",blank_conflicts=" << metrics.blankConflicts
              << ",blank_nodes=" << metrics.blankNodes
              << ",blank_rollouts=" << metrics.blankRollouts
              << ",emergency_days=" << metrics.emergencyDays
              << ",static_manifest_days=" << metrics.staticManifestDays
              << ",current_score_sacrifice_days=" << metrics.currentScoreSacrificeDays
              << ",today_gap_closed_t1_t2_t3="
              << metrics.todayOpenTierCounts.at(0) << '/'
              << metrics.todayOpenTierCounts.at(1) << '/'
              << metrics.todayOpenTierCounts.at(2) << '/'
              << metrics.todayOpenTierCounts.at(3)
              << ",horizon_gap_closed_t1_t2_t3="
              << metrics.horizonOpenTierCounts.at(0) << '/'
              << metrics.horizonOpenTierCounts.at(1) << '/'
              << metrics.horizonOpenTierCounts.at(2) << '/'
              << metrics.horizonOpenTierCounts.at(3)
              << '\n';
}

void print_summary(
    const std::vector<RunMetrics>& results,
    std::size_t fixtureCount,
    const std::vector<Strategy>& strategies,
    Strategy referenceStrategy) {
    for (const Strategy strategy : strategies) {
        std::int32_t wins = 0;
        std::int32_t ties = 0;
        std::int32_t tierOneDrops = 0;
        std::int32_t tierOneGains = 0;
        std::int32_t invalid = 0;
        std::int32_t pairWins = 0;
        std::int32_t pairTies = 0;
        std::int32_t pairLosses = 0;
        std::int32_t currentScoreSacrifices = 0;
        std::int64_t lifetimeDelta = 0;
        std::int64_t dailyDelta = 0;
        std::int64_t servingsDelta = 0;
        std::vector<std::int64_t> tiedScoreTimes;
        std::vector<std::int64_t> allTimes;
        std::array<std::int64_t, 4> todayOpenTierCounts{};
        std::array<std::int64_t, 4> horizonOpenTierCounts{};
        for (std::size_t fixtureIndex = 0; fixtureIndex < fixtureCount; ++fixtureIndex) {
            const RunMetrics* selected = nullptr;
            const RunMetrics* full = nullptr;
            udon::OfficialScore best;
            bool hasBest = false;
            std::int32_t bestCount = 0;
            for (const RunMetrics& result : results) {
                if (result.fixture != results.at(fixtureIndex * strategies.size()).fixture) {
                    continue;
                }
                if (!hasBest || udon::compare_lexicographic(result.score, best) > 0) {
                    best = result.score;
                    hasBest = true;
                    bestCount = 1;
                } else if (result.score == best) {
                    ++bestCount;
                }
                if (result.strategy == strategy_name(strategy)) {
                    selected = &result;
                }
                if (result.strategy == strategy_name(referenceStrategy)) {
                    full = &result;
                }
            }
            if (selected == nullptr || full == nullptr) {
                throw std::runtime_error("benchmark summary lost a strategy result");
            }
            if (selected->score == best) {
                if (bestCount == 1) {
                    ++wins;
                } else {
                    ++ties;
                }
            }
            tierOneDrops += selected->score.lifetimeDistinct < full->score.lifetimeDistinct ? 1 : 0;
            tierOneGains += selected->score.lifetimeDistinct > full->score.lifetimeDistinct ? 1 : 0;
            lifetimeDelta += selected->score.lifetimeDistinct - full->score.lifetimeDistinct;
            dailyDelta += selected->score.totalDailyDistinct - full->score.totalDailyDistinct;
            servingsDelta += selected->score.totalServings - full->score.totalServings;
            const std::int32_t pairOrder = udon::compare_lexicographic(selected->score, full->score);
            pairWins += pairOrder > 0 ? 1 : 0;
            pairTies += pairOrder == 0 ? 1 : 0;
            pairLosses += pairOrder < 0 ? 1 : 0;
            invalid += selected->invalidPlans;
            currentScoreSacrifices += selected->currentScoreSacrificeDays;
            for (std::size_t tier = 0; tier < todayOpenTierCounts.size(); ++tier) {
                todayOpenTierCounts.at(tier) += selected->todayOpenTierCounts.at(tier);
                horizonOpenTierCounts.at(tier) += selected->horizonOpenTierCounts.at(tier);
            }
            allTimes.insert(
                allTimes.end(),
                selected->responseTimes.begin(),
                selected->responseTimes.end());
            if (selected->score == best) {
                tiedScoreTimes.insert(
                    tiedScoreTimes.end(),
                    selected->responseTimes.begin(),
                    selected->responseTimes.end());
            }
        }
        const auto [intervalLow, intervalHigh] = wilson_interval(pairWins, pairWins + pairLosses);
        std::cout << "summary"
                  << ",strategy=" << strategy_name(strategy)
                  << ",lex_wins=" << wins << '/' << fixtureCount
                  << ",lex_ties=" << ties << '/' << fixtureCount
                  << ",tier1_drops_vs_full=" << tierOneDrops
                  << ",tier1_gains_vs_full=" << tierOneGains
                  << ",score_delta_vs_full=" << lifetimeDelta << '/' << dailyDelta << '/' << servingsDelta
                  << ",pair_vs_full=" << pairWins << '/' << pairTies << '/' << pairLosses
                  << ",decisive_win_ci95=" << std::fixed << std::setprecision(4)
                  << intervalLow << ':' << intervalHigh
                  << ",sign_p=" << two_sided_sign_p(pairWins, pairLosses)
                  << ",invalid=" << invalid
                  << ",current_score_sacrifices=" << currentScoreSacrifices
                  << ",p50_ms=" << percentile(allTimes, 50)
                  << ",p95_ms=" << percentile(allTimes, 95)
                  << ",p99_ms=" << percentile(allTimes, 99)
                  << ",max_ms=" << percentile(allTimes, 100)
                  << ",equal_best_p95_ms=" << percentile(tiedScoreTimes, 95)
                  << ",today_gap_closed_t1_t2_t3="
                  << todayOpenTierCounts.at(0) << '/'
                  << todayOpenTierCounts.at(1) << '/'
                  << todayOpenTierCounts.at(2) << '/'
                  << todayOpenTierCounts.at(3)
                  << ",horizon_gap_closed_t1_t2_t3="
                  << horizonOpenTierCounts.at(0) << '/'
                  << horizonOpenTierCounts.at(1) << '/'
                  << horizonOpenTierCounts.at(2) << '/'
                  << horizonOpenTierCounts.at(3)
                  << '\n';
    }
}

void print_family_ablation(const std::vector<RunMetrics>& results) {
    std::vector<std::string> families;
    for (const RunMetrics& result : results) {
        if (std::find(families.begin(), families.end(), result.family) == families.end()) {
            families.push_back(result.family);
        }
    }
    for (const std::string& family : families) {
        std::int32_t fullWins = 0;
        std::int32_t ties = 0;
        std::int32_t routePoolWins = 0;
        std::int32_t fullFeedbackWins = 0;
        std::int32_t feedbackTies = 0;
        std::int32_t feedbackWins = 0;
        for (const RunMetrics& full : results) {
            if (full.family != family || full.strategy != strategy_name(Strategy::UdonShield)) {
                continue;
            }
            const auto routePool = std::find_if(
                results.begin(),
                results.end(),
                [&full](const RunMetrics& candidate) {
                    return candidate.fixture == full.fixture &&
                        candidate.strategy == strategy_name(Strategy::RollingHalnsRoutePool);
                });
            if (routePool == results.end()) {
                throw std::runtime_error("family ablation lost route-pool result");
            }
            const std::int32_t order = udon::compare_lexicographic(full.score, routePool->score);
            fullWins += order > 0 ? 1 : 0;
            ties += order == 0 ? 1 : 0;
            routePoolWins += order < 0 ? 1 : 0;
            const auto feedback = std::find_if(
                results.begin(),
                results.end(),
                [&full](const RunMetrics& candidate) {
                    return candidate.fixture == full.fixture &&
                        candidate.strategy == strategy_name(Strategy::RollingHalnsFeedback);
                });
            if (feedback == results.end()) {
                throw std::runtime_error("family ablation lost feedback result");
            }
            const std::int32_t feedbackOrder =
                udon::compare_lexicographic(full.score, feedback->score);
            fullFeedbackWins += feedbackOrder > 0 ? 1 : 0;
            feedbackTies += feedbackOrder == 0 ? 1 : 0;
            feedbackWins += feedbackOrder < 0 ? 1 : 0;
        }
        std::cout << "family"
                  << ",name=" << family
                  << ",full_vs_route_pool=" << fullWins << '/' << ties << '/' << routePoolWins
                  << ",full_vs_feedback=" << fullFeedbackWins << '/' << feedbackTies << '/' << feedbackWins
                  << '\n';
    }
}

void print_pairwise(
    const std::vector<RunMetrics>& results,
    Strategy leftStrategy,
    Strategy rightStrategy) {
    std::int32_t wins = 0;
    std::int32_t ties = 0;
    std::int32_t losses = 0;
    for (const RunMetrics& left : results) {
        if (left.strategy != strategy_name(leftStrategy)) {
            continue;
        }
        const auto right = std::find_if(
            results.begin(),
            results.end(),
            [&left, rightStrategy](const RunMetrics& candidate) {
                return candidate.fixture == left.fixture &&
                    candidate.strategy == strategy_name(rightStrategy);
            });
        if (right == results.end()) {
            throw std::runtime_error("pairwise summary lost a strategy result");
        }
        const std::int32_t order = udon::compare_lexicographic(left.score, right->score);
        wins += order > 0 ? 1 : 0;
        ties += order == 0 ? 1 : 0;
        losses += order < 0 ? 1 : 0;
    }
    const auto [low, high] = wilson_interval(wins, wins + losses);
    std::cout << "pair"
              << ",left=" << strategy_name(leftStrategy)
              << ",right=" << strategy_name(rightStrategy)
              << ",win_tie_loss=" << wins << '/' << ties << '/' << losses
              << ",decisive_win_ci95=" << low << ':' << high
              << ",sign_p=" << two_sided_sign_p(wins, losses)
              << '\n';
}

void print_family_pairwise(
    const std::vector<RunMetrics>& results,
    Strategy leftStrategy,
    Strategy rightStrategy) {
    std::vector<std::string> families;
    for (const RunMetrics& result : results) {
        if (std::find(families.begin(), families.end(), result.family) == families.end()) {
            families.push_back(result.family);
        }
    }
    for (const std::string& family : families) {
        std::int32_t wins = 0;
        std::int32_t ties = 0;
        std::int32_t losses = 0;
        for (const RunMetrics& left : results) {
            if (left.family != family ||
                left.strategy != strategy_name(leftStrategy)) {
                continue;
            }
            const auto right = std::find_if(
                results.begin(),
                results.end(),
                [&left, rightStrategy](const RunMetrics& candidate) {
                    return candidate.fixture == left.fixture &&
                        candidate.strategy == strategy_name(rightStrategy);
                });
            if (right == results.end()) {
                throw std::runtime_error("family pairwise lost a strategy result");
            }
            const std::int32_t order =
                udon::compare_lexicographic(left.score, right->score);
            wins += order > 0 ? 1 : 0;
            ties += order == 0 ? 1 : 0;
            losses += order < 0 ? 1 : 0;
        }
        std::cout << "family_pair"
                  << ",family=" << family
                  << ",left=" << strategy_name(leftStrategy)
                  << ",right=" << strategy_name(rightStrategy)
                  << ",win_tie_loss=" << wins << '/' << ties << '/' << losses
                  << '\n';
    }
}

} 

int main(int argumentCount, char** arguments) {
    try {
        std::chrono::milliseconds budget{1200};
        bool smoke = false;
        bool summaryOnly = false;
        bool commonRoles = false;
        std::string split = "fixed";
        std::string focus = "all";
        std::int32_t seedCount = 100;
        std::int32_t seedOffset = 0;
        for (int argument = 1; argument < argumentCount; ++argument) {
            const std::string value = arguments[argument];
            if (value == "--smoke") {
                smoke = true;
                budget = std::chrono::milliseconds{350};
            } else if (value == "--summary-only") {
                summaryOnly = true;
            } else if (value == "--common-roles") {
                commonRoles = true;
            } else if (value == "--split" && argument + 1 < argumentCount) {
                split = arguments[++argument];
            } else if (value == "--focus" && argument + 1 < argumentCount) {
                focus = arguments[++argument];
            } else if (value == "--seeds" && argument + 1 < argumentCount) {
                seedCount = std::stoi(arguments[++argument]);
            } else if (value == "--seed-offset" && argument + 1 < argumentCount) {
                seedOffset = std::stoi(arguments[++argument]);
            } else if (value == "--budget-ms" && argument + 1 < argumentCount) {
                budget = std::chrono::milliseconds{std::stoll(arguments[++argument])};
            } else {
                throw std::invalid_argument(
                    "usage: udonshield_strategy_bench [--smoke] [--summary-only] "
                    "[--split fixed|train|validation|heldout|research-train|research-validation|"
                    "research-confirm|future-holdout|harvest-holdout|btc-large-holdout|btc-highfuel-holdout|proof-holdout|proof-production-holdout|blank-train|blank-validation|blank-confirm|"
                    "blank-holdout] [--focus all|halns-feedback|full-feedback|harvest-ablation|deep-harvest-ablation|depth4-ablation|orienteering-ablation|role-ablation|blank-slate|blank-champion|proof-ablation|proof-production] "
                    "[--common-roles] [--seeds N] [--seed-offset N] [--budget-ms N]");
            }
        }
        if (budget.count() <= 0 || seedCount <= 0 || seedOffset < 0) {
            throw std::invalid_argument("benchmark budget and seed count must be positive");
        }
        std::vector<FixtureSpec> suite;
        if (split == "fixed") {
            suite = fixtures();
        } else {
            std::uint64_t firstSeed = 0;
            bool btcLarge = false;
            bool btcHighFuel = false;
            if (split == "validation") {
                firstSeed = 10000;
            } else if (split == "heldout") {
                firstSeed = 100000;
            } else if (split == "research-train") {
                firstSeed = 1000;
            } else if (split == "research-validation") {
                firstSeed = 20000;
            } else if (split == "research-confirm") {
                firstSeed = 30000;
            } else if (split == "future-holdout") {
                firstSeed = 200000;
            } else if (split == "harvest-holdout") {
                firstSeed = 600000;
            } else if (split == "btc-large-holdout") {
                firstSeed = 700000;
                btcLarge = true;
            } else if (split == "btc-highfuel-holdout") {
                firstSeed = 800000;
                btcHighFuel = true;
            } else if (split == "proof-holdout") {
                firstSeed = 400000;
            } else if (split == "proof-production-holdout") {
                firstSeed = 500000;
            } else if (split == "blank-train") {
                firstSeed = 40000;
            } else if (split == "blank-validation") {
                firstSeed = 50000;
            } else if (split == "blank-confirm") {
                firstSeed = 60000;
            } else if (split == "blank-holdout") {
                firstSeed = 300000;
            } else if (split != "train") {
                throw std::invalid_argument("unknown benchmark split: " + split);
            }
            suite.reserve(static_cast<std::size_t>(seedCount));
            for (std::int32_t offset = 0; offset < seedCount; ++offset) {
                const std::uint64_t seed = firstSeed +
                    static_cast<std::uint64_t>(seedOffset) +
                    static_cast<std::uint64_t>(offset);
                suite.push_back(btcHighFuel
                    ? generated_btc_highfuel_fixture(seed)
                    : (btcLarge
                        ? generated_btc_large_fixture(seed)
                        : generated_fixture(seed)));
            }
        }
        if (smoke && suite.size() > 2U) {
            suite.resize(2);
        }
        std::vector<Strategy> strategies;
        if (focus == "all") {
            strategies.assign(kStrategies.begin(), kStrategies.end());
        } else if (focus == "halns-feedback") {
            strategies = {
                Strategy::RollingHalnsRoutePool,
                Strategy::RollingHalnsFeedback,
                Strategy::RollingHalnsFeedbackDiverse,
                Strategy::UdonShield,
            };
        } else if (focus == "full-feedback") {
            strategies = {
                Strategy::UdonShieldFeedback,
                Strategy::UdonShield,
            };
        } else if (focus == "harvest-ablation") {
            strategies = {
                Strategy::UdonShield,
                Strategy::UdonShieldMultiHarvest,
            };
        } else if (focus == "deep-harvest-ablation") {
            strategies = {
                Strategy::UdonShieldMultiHarvest,
                Strategy::UdonShieldDeepHarvest,
            };
        } else if (focus == "depth4-ablation") {
            strategies = {
                Strategy::UdonShieldDeepHarvest,
                Strategy::UdonShieldAdaptiveDepth4,
            };
        } else if (focus == "orienteering-ablation") {
            strategies = {
                Strategy::UdonShieldAdaptiveDepth4,
                Strategy::UdonShieldOrienteering,
            };
        } else if (focus == "role-ablation") {
            strategies = {
                Strategy::UdonShieldDeepHarvestCentralRoles,
                Strategy::UdonShieldDeepHarvestAdaptiveRoles3,
                Strategy::UdonShieldDeepHarvestAdaptiveRoles,
            };
        } else if (focus == "blank-slate") {
            strategies = {
                Strategy::EventConflict,
                Strategy::BackwardDeadline,
                Strategy::MacroMcts,
                Strategy::BlankPortfolio,
                Strategy::UdonShield,
            };
        } else if (focus == "blank-champion") {
            strategies = {
                Strategy::BlankPortfolio,
                Strategy::UdonShield,
            };
        } else if (focus == "proof-ablation") {
            strategies = {
                Strategy::DeterministicAlns,
                Strategy::DeterministicProofAlns,
                Strategy::DeterministicProofAlns4x,
                Strategy::DeterministicProofAlns8x,
                Strategy::DeterministicProofRoutePool,
            };
        } else if (focus == "proof-production") {
            strategies = {
                Strategy::UdonShield,
                Strategy::UdonShieldProofExpanded,
            };
        } else {
            throw std::invalid_argument("unknown benchmark focus: " + focus);
        }
        std::vector<RunMetrics> results;
        results.reserve(suite.size() * strategies.size());
        const Strategy summaryReference = focus == "proof-ablation"
            ? Strategy::DeterministicAlns
            : (focus == "proof-production"
                ? Strategy::UdonShield
                : (focus == "deep-harvest-ablation"
                    ? Strategy::UdonShieldMultiHarvest
                    : (focus == "depth4-ablation"
                        ? Strategy::UdonShieldDeepHarvest
                        : (focus == "orienteering-ablation"
                            ? Strategy::UdonShieldAdaptiveDepth4
                            : (focus == "role-ablation"
                                ? Strategy::UdonShieldDeepHarvestAdaptiveRoles3
                                : Strategy::UdonShield)))));
        std::cout << "schema=udon-shield-strategy-benchmark-v3"
                  << ",split=" << split
                  << ",fixtures=" << suite.size()
                  << ",strategies=" << strategies.size()
                  << ",focus=" << focus
                  << ",reference=" << strategy_name(summaryReference)
                  << ",roles=" << (commonRoles ? "central-tanker-common" : "native")
                  << ",days=" << suite.front().daySteps.size()
                  << ",budget_ms=" << budget.count()
                  << ",traffic=endogenous-own-plus-common-opponents-two-day"
                  << '\n';
        for (const FixtureSpec& fixture : suite) {
            const udon::MatchConfig config = udon::parse_match_config(
                udon::JsonValue::parse(config_document(fixture)));
            std::vector<udon::AgentKind> sharedRoles;
            if (commonRoles) {
                sharedRoles = udon::blank_slate::Planner(
                    config,
                    udon::blank_slate::Method::EventConflict).select_roles();
            }
            for (const Strategy strategy : strategies) {
                RunMetrics metrics = run_strategy(
                    fixture,
                    config,
                    strategy,
                    budget,
                    commonRoles ? &sharedRoles : nullptr);
                if (!summaryOnly) {
                    print_result(metrics);
                }
                results.push_back(std::move(metrics));
            }
        }
        print_summary(results, suite.size(), strategies, summaryReference);
        const auto includes = [&strategies](Strategy strategy) {
            return std::find(strategies.begin(), strategies.end(), strategy) != strategies.end();
        };
        if (includes(Strategy::RollingHalnsRoutePool) &&
            includes(Strategy::RollingHalnsFeedback) &&
            includes(Strategy::UdonShield)) {
            print_family_ablation(results);
            print_pairwise(results, Strategy::RollingHalnsFeedback, Strategy::RollingHalnsRoutePool);
        }
        if (includes(Strategy::RollingHalnsFeedbackDiverse) &&
            includes(Strategy::RollingHalnsFeedback)) {
            print_pairwise(
                results,
                Strategy::RollingHalnsFeedbackDiverse,
                Strategy::RollingHalnsFeedback);
        }
        if (includes(Strategy::RollingHalnsFeedback) &&
            includes(Strategy::UdonShieldFixedRoles)) {
            print_pairwise(results, Strategy::RollingHalnsFeedback, Strategy::UdonShieldFixedRoles);
        }
        if (includes(Strategy::RollingHalnsRoutePool) &&
            includes(Strategy::UdonShieldFixedRoles)) {
            print_pairwise(results, Strategy::RollingHalnsRoutePool, Strategy::UdonShieldFixedRoles);
        }
        if (includes(Strategy::UdonShieldFixedRoles) &&
            includes(Strategy::UdonShield)) {
            print_pairwise(results, Strategy::UdonShieldFixedRoles, Strategy::UdonShield);
        }
        if (includes(Strategy::UdonShieldFeedback) &&
            includes(Strategy::UdonShield)) {
            print_pairwise(results, Strategy::UdonShieldFeedback, Strategy::UdonShield);
        }
        if (includes(Strategy::UdonShieldDeepHarvestCentralRoles) &&
            includes(Strategy::UdonShieldDeepHarvestAdaptiveRoles3)) {
            print_pairwise(
                results,
                Strategy::UdonShieldDeepHarvestAdaptiveRoles3,
                Strategy::UdonShieldDeepHarvestCentralRoles);
        }
        if (includes(Strategy::UdonShieldDeepHarvestAdaptiveRoles3) &&
            includes(Strategy::UdonShieldDeepHarvestAdaptiveRoles)) {
            print_pairwise(
                results,
                Strategy::UdonShieldDeepHarvestAdaptiveRoles,
                Strategy::UdonShieldDeepHarvestAdaptiveRoles3);
        }
        if (includes(Strategy::UdonShieldDeepHarvestCentralRoles) &&
            includes(Strategy::UdonShieldDeepHarvestAdaptiveRoles)) {
            print_pairwise(
                results,
                Strategy::UdonShieldDeepHarvestAdaptiveRoles,
                Strategy::UdonShieldDeepHarvestCentralRoles);
        }
        if (includes(Strategy::UdonShieldMultiHarvest) &&
            includes(Strategy::UdonShield)) {
            print_pairwise(results, Strategy::UdonShieldMultiHarvest, Strategy::UdonShield);
            print_family_pairwise(results, Strategy::UdonShieldMultiHarvest, Strategy::UdonShield);
        }
        if (includes(Strategy::UdonShieldDeepHarvest) &&
            includes(Strategy::UdonShieldMultiHarvest)) {
            print_pairwise(
                results,
                Strategy::UdonShieldDeepHarvest,
                Strategy::UdonShieldMultiHarvest);
            print_family_pairwise(
                results,
                Strategy::UdonShieldDeepHarvest,
                Strategy::UdonShieldMultiHarvest);
        }
        if (includes(Strategy::UdonShieldAdaptiveDepth4) &&
            includes(Strategy::UdonShieldDeepHarvest)) {
            print_pairwise(
                results,
                Strategy::UdonShieldAdaptiveDepth4,
                Strategy::UdonShieldDeepHarvest);
            print_family_pairwise(
                results,
                Strategy::UdonShieldAdaptiveDepth4,
                Strategy::UdonShieldDeepHarvest);
        }
        if (includes(Strategy::UdonShieldOrienteering) &&
            includes(Strategy::UdonShieldAdaptiveDepth4)) {
            print_pairwise(
                results,
                Strategy::UdonShieldOrienteering,
                Strategy::UdonShieldAdaptiveDepth4);
            print_family_pairwise(
                results,
                Strategy::UdonShieldOrienteering,
                Strategy::UdonShieldAdaptiveDepth4);
        }
        if (includes(Strategy::EventConflict) &&
            includes(Strategy::UdonShield)) {
            print_pairwise(results, Strategy::EventConflict, Strategy::UdonShield);
            print_family_pairwise(results, Strategy::EventConflict, Strategy::UdonShield);
        }
        if (includes(Strategy::BackwardDeadline) &&
            includes(Strategy::UdonShield)) {
            print_pairwise(results, Strategy::BackwardDeadline, Strategy::UdonShield);
        }
        if (includes(Strategy::MacroMcts) &&
            includes(Strategy::UdonShield)) {
            print_pairwise(results, Strategy::MacroMcts, Strategy::UdonShield);
            print_family_pairwise(results, Strategy::MacroMcts, Strategy::UdonShield);
        }
        if (includes(Strategy::BlankPortfolio) &&
            includes(Strategy::UdonShield)) {
            print_pairwise(results, Strategy::BlankPortfolio, Strategy::UdonShield);
            print_family_pairwise(results, Strategy::BlankPortfolio, Strategy::UdonShield);
        }
        if (includes(Strategy::BlankPortfolio) &&
            includes(Strategy::EventConflict)) {
            print_pairwise(results, Strategy::BlankPortfolio, Strategy::EventConflict);
        }
        if (includes(Strategy::BlankPortfolio) &&
            includes(Strategy::MacroMcts)) {
            print_pairwise(results, Strategy::BlankPortfolio, Strategy::MacroMcts);
        }
        if (includes(Strategy::EventConflict) &&
            includes(Strategy::MacroMcts)) {
            print_pairwise(results, Strategy::EventConflict, Strategy::MacroMcts);
        }
        if (includes(Strategy::EventConflict) &&
            includes(Strategy::BackwardDeadline)) {
            print_pairwise(results, Strategy::EventConflict, Strategy::BackwardDeadline);
        }
        if (includes(Strategy::DeterministicAlns) &&
            includes(Strategy::DeterministicProofAlns)) {
            print_pairwise(
                results,
                Strategy::DeterministicProofAlns,
                Strategy::DeterministicAlns);
            print_family_pairwise(
                results,
                Strategy::DeterministicProofAlns,
                Strategy::DeterministicAlns);
        }
        if (includes(Strategy::DeterministicProofAlns) &&
            includes(Strategy::DeterministicProofRoutePool)) {
            print_pairwise(
                results,
                Strategy::DeterministicProofRoutePool,
                Strategy::DeterministicProofAlns);
            print_family_pairwise(
                results,
                Strategy::DeterministicProofRoutePool,
                Strategy::DeterministicProofAlns);
        }
        if (includes(Strategy::UdonShieldProofExpanded) && includes(Strategy::UdonShield)) {
            print_pairwise(results, Strategy::UdonShieldProofExpanded, Strategy::UdonShield);
            print_family_pairwise(results, Strategy::UdonShieldProofExpanded, Strategy::UdonShield);
        }
        if (includes(Strategy::DeterministicProofAlns) &&
            includes(Strategy::DeterministicProofAlns4x)) {
            print_pairwise(
                results,
                Strategy::DeterministicProofAlns4x,
                Strategy::DeterministicProofAlns);
            print_family_pairwise(
                results,
                Strategy::DeterministicProofAlns4x,
                Strategy::DeterministicProofAlns);
        }
        if (includes(Strategy::DeterministicProofAlns4x) &&
            includes(Strategy::DeterministicProofAlns8x)) {
            print_pairwise(
                results,
                Strategy::DeterministicProofAlns8x,
                Strategy::DeterministicProofAlns4x);
            print_family_pairwise(
                results,
                Strategy::DeterministicProofAlns8x,
                Strategy::DeterministicProofAlns4x);
        }
        if (includes(Strategy::DeterministicProofAlns) &&
            includes(Strategy::DeterministicProofAlns8x)) {
            print_pairwise(
                results,
                Strategy::DeterministicProofAlns8x,
                Strategy::DeterministicProofAlns);
            print_family_pairwise(
                results,
                Strategy::DeterministicProofAlns8x,
                Strategy::DeterministicProofAlns);
        }
        const std::int32_t invalid = std::accumulate(
            results.begin(),
            results.end(),
            0,
            [](std::int32_t total, const RunMetrics& metrics) {
                return total + metrics.invalidPlans;
            });
        if (invalid != 0) {
            throw std::runtime_error("strategic benchmark invalid-rate gate failed");
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
