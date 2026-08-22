#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "udon/decision.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/slack_refiner.hpp"
#include "udon/validator.hpp"

namespace {

constexpr std::int32_t kHarnessHarvestMode = 7;
constexpr std::int32_t kHarnessFutureHarvestMode = 7;

struct SpotSpec {
    std::int32_t brand = 0;
    udon::CellId position = udon::kInvalidCell;
    std::int32_t stock = 1;
};

struct FixtureSpec {
    std::string name;
    std::string family;
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

struct Options {
    std::string version;
    std::string track;
    std::string suite = "general";
    std::uint64_t firstSeed = 100000;
    std::int32_t seedCount = 6;
    std::chrono::milliseconds dayBudget{1200};
    std::chrono::milliseconds roleBudget{500};
    std::string roleMode = "exhaustive";
    std::uint32_t fixedRoleMask = 0;
    std::chrono::milliseconds protectedRefineBudget{0};
    std::chrono::milliseconds protectedWaitBudget{0};
    bool protectedWaitDetours = false;
    bool protectedWaitClosedLoop = false;
    bool dayDetails = false;
};

struct Metrics {
    udon::OfficialScore score;
    udon::OfficialScore protectedVirtualScore;
    std::vector<std::int64_t> responseTimes;
    std::int64_t roleMilliseconds = 0;
    std::int64_t combinationsVisited = 0;
    std::uint32_t roleMask = 0;
    std::int32_t invalidPlans = 0;
    std::int32_t emergencyDays = 0;
    std::int32_t searchCompleteDays = 0;
    std::int32_t searchDeadlineDays = 0;
    std::vector<udon::OfficialScore> cumulativeDayScores;
    std::vector<udon::OfficialScore> protectedVirtualDayScores;
    std::vector<udon::DayScore> exactDayScores;
    std::vector<std::uint64_t> planHashes;
    std::vector<bool> deadlineDays;
    std::vector<std::int32_t> exactSupportedAgents;
    std::vector<std::uint64_t> exactSettledStates;
    std::vector<std::int32_t> exactSeedServings;
    std::vector<std::int32_t> exactLocalServings;
    std::int32_t protectedTerminalAttempts = 0;
    std::int32_t protectedTerminalTakeovers = 0;
    std::int32_t protectedTerminalInvalid = 0;
    udon::OfficialScore protectedTerminalParentScore;
    udon::OfficialScore protectedTerminalRefinedScore;
    std::int64_t protectedWaitPlans = 0;
    std::int64_t protectedWaitValid = 0;
    std::int64_t protectedWaitLiftable = 0;
    std::int32_t protectedWaitBestDay = 0;
    udon::OfficialScore protectedWaitParentScore;
    udon::OfficialScore protectedWaitBestScore;
    std::int32_t protectedWaitTakeovers = 0;
    std::int32_t protectedWaitDeadlineDays = 0;
    std::int32_t protectedWaitWitnessAgent = -1;
    udon::CellId protectedWaitWitnessAnchor = udon::kInvalidCell;
    udon::CellId protectedWaitWitnessSpot = udon::kInvalidCell;
    std::int32_t protectedWaitWitnessDuration = 0;
    std::int32_t protectedWaitWitnessTravelSteps = 0;
    std::int32_t protectedWaitWitnessParentFuel = 0;
    std::int32_t protectedWaitWitnessCandidateFuel = 0;
    std::uint64_t protectedWaitWitnessPlanHash = 0;
};

[[nodiscard]] std::uint64_t plan_hash(const udon::DayPlan& plan) {
    std::uint64_t hash = 1469598103934665603ULL;
    const auto mix = [&hash](std::uint64_t value) {
        hash ^= value;
        hash *= 1099511628211ULL;
    };
    mix(plan.actions.size());
    for (const udon::AgentPlan& actions : plan.actions) {
        mix(actions.size());
        for (const udon::PlanAction& action : actions) {
            mix(static_cast<std::uint32_t>(action.wire_value()));
        }
    }
    return hash;
}

[[nodiscard]] std::vector<std::int32_t> plain_map(std::size_t cellCount) {
    return std::vector<std::int32_t>(
        cellCount,
        static_cast<std::int32_t>(udon::Terrain::Plain));
}

void preserve_plain_cells(FixtureSpec& fixture) {
    for (const udon::CellId start : fixture.starts) {
        fixture.terrain.at(static_cast<std::size_t>(start)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    for (const SpotSpec& spot : fixture.spots) {
        fixture.terrain.at(static_cast<std::size_t>(spot.position)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }
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
    fixture.terrain = plain_map(64U);

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
    fixture.jammedThreshold =
        fixture.busyThreshold + 2 + static_cast<std::int32_t>(random() % 3U);
    preserve_plain_cells(fixture);
    return fixture;
}

[[nodiscard]] FixtureSpec generated_btc_large_fixture(
    std::uint64_t seed,
    std::int32_t side = 32,
    std::int32_t days = 10,
    std::int32_t agentCount = 8) {
    static constexpr std::array<const char*, 6> families{
        "balanced",
        "rare-brand",
        "threshold-corridor",
        "fuel-tight",
        "high-stock",
        "overnight",
    };
    const std::int32_t cellCount = side * side;
    FixtureSpec fixture;
    fixture.seed = seed;
    fixture.family = families.at(static_cast<std::size_t>(seed % families.size()));
    fixture.name = "btc-large-" + fixture.family + "-seed-" + std::to_string(seed);
    fixture.width = side;
    fixture.height = side;
    fixture.terrain = plain_map(static_cast<std::size_t>(cellCount));

    std::mt19937_64 random(seed ^ 0xd1b54a32d192ed03ULL);
    std::vector<udon::CellId> roadCells;
    const std::size_t targetRoads = static_cast<std::size_t>(std::max(
        side,
        static_cast<std::int32_t>(
            std::llround(63.0 * static_cast<double>(cellCount) / 1024.0))));
    roadCells.reserve(targetRoads);
    const auto addRoad = [&fixture, &roadCells, side](
                             std::int32_t row,
                             std::int32_t column) {
        const udon::CellId cell = row * side + column;
        if (fixture.terrain.at(static_cast<std::size_t>(cell)) !=
            static_cast<std::int32_t>(udon::Terrain::Road)) {
            fixture.terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Road);
            roadCells.push_back(cell);
        }
    };
    const std::int32_t interiorSpan = std::max(1, side - 4);
    std::int32_t column = 2 +
        static_cast<std::int32_t>(random() %
            static_cast<std::uint64_t>(interiorSpan));
    for (std::int32_t row = 0;
         row < side && roadCells.size() < targetRoads;
         ++row) {
        column = std::clamp(
            column + static_cast<std::int32_t>(random() % 3U) - 1,
            1,
            side - 2);
        addRoad(row, column);
    }
    std::int32_t row = 2 +
        static_cast<std::int32_t>(random() %
            static_cast<std::uint64_t>(interiorSpan));
    for (column = 0;
         column < side && roadCells.size() < targetRoads;
         ++column) {
        row = std::clamp(
            row + static_cast<std::int32_t>(random() % 3U) - 1,
            1,
            side - 2);
        addRoad(row, column);
    }
    for (std::int32_t cell = 0;
         roadCells.size() < targetRoads && cell < cellCount;
         ++cell) {
        const udon::CellId candidate = static_cast<udon::CellId>(
            (cell * 37 + static_cast<std::int32_t>(seed % 31U)) % cellCount);
        addRoad(candidate / side, candidate % side);
    }

    std::vector<udon::CellId> cells(static_cast<std::size_t>(cellCount));
    std::iota(cells.begin(), cells.end(), 0);
    std::shuffle(cells.begin(), cells.end(), random);
    const std::int32_t targetMountains = std::max(
        1,
        static_cast<std::int32_t>(
            std::llround(56.0 * static_cast<double>(cellCount) / 1024.0)));
    const std::int32_t targetPonds = std::max(
        1,
        static_cast<std::int32_t>(
            std::llround(5.0 * static_cast<double>(cellCount) / 1024.0)));
    std::int32_t mountains = 0;
    std::int32_t ponds = 0;
    for (const udon::CellId cell : cells) {
        std::int32_t& terrain = fixture.terrain.at(static_cast<std::size_t>(cell));
        if (terrain != static_cast<std::int32_t>(udon::Terrain::Plain)) {
            continue;
        }
        if (mountains < targetMountains) {
            terrain = static_cast<std::int32_t>(udon::Terrain::Mountain);
            ++mountains;
        } else if (ponds < targetPonds) {
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
    fixture.starts.assign(
        plainCells.begin(),
        plainCells.begin() + agentCount);
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
            plainCells.at(static_cast<std::size_t>(agentCount + spotIndex)),
            stock,
        });
    }
    fixture.daySteps.assign(static_cast<std::size_t>(days), 100);
    fixture.fuelLimit = fixture.family == "fuel-tight" ? 120 : 200;
    fixture.players = 4;
    fixture.busyThreshold = 5;
    fixture.jammedThreshold = 10;
    return fixture;
}

[[nodiscard]] FixtureSpec make_fixture(const Options& options, std::uint64_t seed) {
    if (options.suite == "general") {
        return generated_fixture(seed);
    }
    if (options.suite == "general-roadless") {
        FixtureSpec fixture = generated_fixture(seed);
        fixture.family = "roadless-" + fixture.family;
        fixture.name = "roadless-" + fixture.name;
        for (std::int32_t& terrain : fixture.terrain) {
            if (terrain == static_cast<std::int32_t>(udon::Terrain::Road)) {
                terrain = static_cast<std::int32_t>(udon::Terrain::Plain);
            }
        }
        return fixture;
    }
    if (options.suite == "tanker-roadless-scale") {
        FixtureSpec fixture = generated_fixture(seed);
        fixture.family = "tanker-roadless-" + fixture.family;
        fixture.name = "tanker-roadless-" + fixture.name;
        for (std::int32_t& terrain : fixture.terrain) {
            if (terrain == static_cast<std::int32_t>(udon::Terrain::Road)) {
                terrain = static_cast<std::int32_t>(udon::Terrain::Plain);
            }
        }
        fixture.starts.resize(3U);
        static constexpr std::array<std::size_t, 4> spotCounts{3U, 4U, 5U, 6U};
        const std::size_t spotCount = spotCounts.at(
            static_cast<std::size_t>((seed / 6U) % spotCounts.size()));
        fixture.spots.resize(spotCount);
        static constexpr std::array<std::int32_t, 3> fuelProfiles{2, 8, 16};
        fixture.fuelLimit = fuelProfiles.at(
            static_cast<std::size_t>((seed / 24U) % fuelProfiles.size()));
        return fixture;
    }
    FixtureSpec fixture;
    if (options.suite == "stratified-easy") {
        fixture = generated_btc_large_fixture(seed, 14, 5, 4);
    } else if (options.suite == "stratified-medium") {
        fixture = generated_btc_large_fixture(seed, 20, 7, 4);
    } else if (options.suite == "stratified-hard") {
        fixture = generated_btc_large_fixture(seed, 26, 8, 6);
    } else if (options.suite == "stratified-very-hard") {
        fixture = generated_btc_large_fixture(seed, 32, 10, 8);
    } else {
        fixture = generated_btc_large_fixture(seed);
    }
    if (options.suite.starts_with("stratified-")) {
        fixture.name = options.suite + "-" + fixture.family +
            "-seed-" + std::to_string(seed);
    }
    if (options.suite == "btc-highfuel") {
        fixture.family = "high-fuel-" + fixture.family;
        fixture.name = "btc-highfuel-" + fixture.name;
        fixture.fuelLimit = 3 * fixture.daySteps.front();
    } else if (options.suite == "btc-lowfuel") {
        fixture.family = "low-fuel-" + fixture.family;
        fixture.name = "btc-lowfuel-" + fixture.name;
        fixture.fuelLimit = fixture.daySteps.front();
    } else if (options.suite == "stratified-easy" ||
               options.suite == "stratified-medium" ||
               options.suite == "stratified-hard" ||
               options.suite == "stratified-very-hard") {
        const std::uint64_t profile = seed % 9U;
        if (profile < 3U) {
            fixture.family = "low-fuel-" + fixture.family;
            fixture.name = "low-fuel-" + fixture.name;
            fixture.fuelLimit = fixture.daySteps.front();
        } else if (profile >= 6U) {
            fixture.family = "high-fuel-" + fixture.family;
            fixture.name = "high-fuel-" + fixture.name;
            fixture.fuelLimit = 3 * fixture.daySteps.front();
        }
    } else if (options.suite != "btc-large") {
        throw std::invalid_argument("unknown suite: " + options.suite);
    }
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
    for (std::int32_t mapRow = 0; mapRow < fixture.height; ++mapRow) {
        if (mapRow != 0) {
            output << ',';
        }
        output << '[';
        for (std::int32_t mapColumn = 0; mapColumn < fixture.width; ++mapColumn) {
            if (mapColumn != 0) {
                output << ',';
            }
            output << fixture.terrain.at(
                static_cast<std::size_t>(mapRow * fixture.width + mapColumn));
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
           << ",\"players\":" << fixture.players
           << ",\"busyThreshold\":" << fixture.busyThreshold
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
        std::vector<std::int32_t>(
            static_cast<std::size_t>(config.map.cell_count()),
            0));
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
            if (fixture.family == "threshold-corridor" ||
                fixture.family == "high-fuel-threshold-corridor") {
                stays = busyMass - 2 + static_cast<std::int32_t>(draw % 7U);
                if ((roadIndex + static_cast<std::size_t>(day)) % 4U == 0U) {
                    stays = jammedMass - 2 + static_cast<std::int32_t>(draw % 5U);
                }
            } else if (fixture.family == "balanced" ||
                fixture.family == "high-fuel-balanced") {
                stays /= 2;
            } else if ((fixture.family == "overnight" ||
                fixture.family == "high-fuel-overnight") && day % 2 == 0) {
                stays += config.players;
            }
            footprints.at(static_cast<std::size_t>(day - 1)).at(
                static_cast<std::size_t>(road)) = std::max(0, stays);
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

[[nodiscard]] std::vector<udon::AgentKind> fallback_roles(const udon::MatchConfig& config) {
    std::vector<udon::AgentKind> roles(
        static_cast<std::size_t>(config.agent_count()),
        udon::AgentKind::Patrol);
    if (!roles.empty()) {
        roles.back() = udon::AgentKind::Tanker;
    }
    return roles;
}

[[nodiscard]] std::vector<udon::AgentKind> roles_from_mask(
    const udon::MatchConfig& config,
    std::uint32_t mask) {
    std::vector<udon::AgentKind> roles(
        static_cast<std::size_t>(config.agent_count()),
        udon::AgentKind::Patrol);
    std::int32_t patrols = config.agent_count();
    for (std::int32_t agent = 0; agent < config.agent_count(); ++agent) {
        if ((mask & (std::uint32_t{1} << static_cast<std::uint32_t>(agent))) != 0U) {
            roles.at(static_cast<std::size_t>(agent)) = udon::AgentKind::Tanker;
            --patrols;
        }
    }
    if (patrols <= 0) {
        throw std::invalid_argument("fixed role mask must leave at least one patrol");
    }
    return roles;
}

[[nodiscard]] std::int64_t percentile(
    std::vector<std::int64_t> values,
    std::int32_t percent) {
    if (values.empty()) {
        return 0;
    }
    std::sort(values.begin(), values.end());
    const std::size_t index = std::min(
        values.size() - 1U,
        static_cast<std::size_t>(
            (static_cast<std::int64_t>(percent) *
                static_cast<std::int64_t>(values.size()) + 99) /
                100 -
            1));
    return values.at(index);
}

[[nodiscard]] Metrics run_fixture(
    const FixtureSpec& fixture,
    const Options& options) {
    const udon::MatchConfig config = udon::parse_match_config(
        udon::JsonValue::parse(config_document(fixture)));
    udon::UdonShieldEngine engine(
        config,
        {},
        {},
        udon::RoutePoolSearch::SinglePass,
        kHarnessHarvestMode,
        false,
        kHarnessFutureHarvestMode);
    const auto roleStarted = std::chrono::steady_clock::now();
    Metrics metrics;
    std::vector<udon::AgentKind> roles;
    if (options.roleMode == "fixed") {
        roles = roles_from_mask(config, options.fixedRoleMask);
    } else {
        const std::vector<udon::RoleAssignment> assignments =
            options.roleMode == "exhaustive"
            ? engine.select_roles_exhaustive_oracle(3)
            : engine.select_roles_until(
                options.roleBudget,
                3);
        roles = assignments.empty()
            ? fallback_roles(config)
            : assignments.front().roles;
    }
    metrics.roleMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - roleStarted).count();
    for (std::size_t index = 0; index < roles.size(); ++index) {
        if (roles.at(index) == udon::AgentKind::Tanker) {
            metrics.roleMask |= std::uint32_t{1}
                << static_cast<std::uint32_t>(index);
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
    udon::MatchLedger virtualLedger;
    std::vector<udon::AgentState> virtualAgents = agents;
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::ProtectedSlackRefiner slackRefiner(config);
    std::vector<std::vector<std::int32_t>> ownFootprints(
        static_cast<std::size_t>(config.day_count()),
        std::vector<std::int32_t>(
            static_cast<std::size_t>(config.map.cell_count()),
            0));
    const std::vector<std::vector<std::int32_t>> opponentFootprints =
        opponent_footprints(fixture, config);

    for (std::int32_t day = 1; day <= config.day_count(); ++day) {
        udon::DayState state;
        state.endsAt = config.startsAt + static_cast<std::int64_t>(day) * 5;
        state.dayNumber = day;
        state.agents = agents;
        state.roadStatuses =
            road_statuses(config, day, ownFootprints, opponentFootprints);
        udon::DayState planningState = state;
        if (options.protectedWaitClosedLoop) {
            planningState.agents = virtualAgents;
        }

        const auto started = std::chrono::steady_clock::now();
        udon::DecisionResult decision =
            engine.solve_day(
                planningState,
                options.protectedWaitClosedLoop ? virtualLedger : ledger,
                options.dayBudget);
        std::optional<udon::DecisionResult> refinementDecision;
        if (options.protectedRefineBudget.count() > 0 &&
            day == config.day_count()) {
            refinementDecision = engine.solve_day(
                planningState,
                options.protectedWaitClosedLoop ? virtualLedger : ledger,
                options.protectedRefineBudget);
        }
        const std::chrono::milliseconds elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - started);
        metrics.responseTimes.push_back(elapsed.count());
        metrics.emergencyDays += decision.emergency ? 1 : 0;
        metrics.combinationsVisited += decision.diagnostics.combinationsVisited;
        metrics.searchCompleteDays += decision.diagnostics.searchComplete ? 1 : 0;
        metrics.searchDeadlineDays += decision.diagnostics.deadlineReached ? 1 : 0;

        udon::SimulationResult detailed =
            simulator.simulate(state, decision.candidate.plan, false);
        const udon::SimulationResult independent =
            validator.validate(state, decision.candidate.plan, false);
        udon::SimulationResult virtualDetailed =
            simulator.simulate(
                planningState,
                decision.candidate.plan,
                false);
        const udon::SimulationResult virtualIndependent =
            validator.validate(
                planningState,
                decision.candidate.plan,
                false);
        std::string mismatch;
        const udon::DecisionResult* selectedDecision = &decision;
        std::uint64_t appliedPlanHash =
            plan_hash(decision.candidate.plan);
        std::string virtualMismatch;
        if (!detailed.valid ||
            !validator.agrees_with(detailed, independent, mismatch) ||
            !virtualDetailed.valid ||
            !validator.agrees_with(
                virtualDetailed,
                virtualIndependent,
                virtualMismatch)) {
            if (options.protectedWaitClosedLoop) {
                throw std::runtime_error(
                    "protected virtual-parent transition invalid for " +
                    fixture.name + ": actual=" + mismatch +
                    ", virtual=" + virtualMismatch);
            }
            ++metrics.invalidPlans;
            const udon::DayPlan wait = udon::emergency_wait_plan(config, state);
            detailed = simulator.simulate(state, wait, false);
            const udon::SimulationResult waitValidation =
                validator.validate(state, wait, false);
            if (!detailed.valid ||
                !validator.agrees_with(detailed, waitValidation, mismatch)) {
                throw std::runtime_error(
                    "fallback validation failed for " + fixture.name + ": " + mismatch);
            }
            virtualDetailed = detailed;
            appliedPlanHash = plan_hash(wait);
        } else {
            if (options.protectedWaitDetours &&
                day < config.day_count()) {
                const auto protectedDeadline =
                    options.protectedWaitBudget.count() > 0
                    ? std::chrono::steady_clock::now() +
                        options.protectedWaitBudget
                    : std::chrono::steady_clock::time_point::max();
                udon::ProtectedSlackResult protectedChoice =
                    slackRefiner.refine_wait_detours(
                    state,
                    ledger,
                    decision.candidate.plan,
                    detailed,
                    protectedDeadline);
                metrics.protectedWaitPlans +=
                    protectedChoice.diagnostics.generatedPlans;
                metrics.protectedWaitValid +=
                    protectedChoice.diagnostics.validPlans;
                metrics.protectedWaitLiftable +=
                    protectedChoice.diagnostics.liftablePlans;
                metrics.protectedWaitDeadlineDays +=
                    protectedChoice.diagnostics.deadlineReached ? 1 : 0;
                if (protectedChoice.improved) {
                    metrics.protectedWaitBestDay = state.dayNumber;
                    metrics.protectedWaitParentScore =
                        udon::OfficialScore::after_day(ledger, detailed.score);
                    metrics.protectedWaitBestScore =
                        protectedChoice.scoreAfterToday;
                    metrics.protectedWaitWitnessAgent =
                        protectedChoice.witnessAgent;
                    metrics.protectedWaitWitnessAnchor =
                        protectedChoice.witnessAnchor;
                    metrics.protectedWaitWitnessSpot =
                        protectedChoice.witnessSpot;
                    metrics.protectedWaitWitnessDuration =
                        protectedChoice.witnessDuration;
                    metrics.protectedWaitWitnessTravelSteps =
                        protectedChoice.witnessTravelSteps;
                    metrics.protectedWaitWitnessParentFuel =
                        protectedChoice.witnessParentFuel;
                    metrics.protectedWaitWitnessCandidateFuel =
                        protectedChoice.witnessCandidateFuel;
                    metrics.protectedWaitWitnessPlanHash =
                        plan_hash(protectedChoice.plan);
                }
                if (options.protectedWaitClosedLoop &&
                    protectedChoice.improved) {
                    detailed = protectedChoice.simulation;
                    appliedPlanHash = plan_hash(protectedChoice.plan);
                    ++metrics.protectedWaitTakeovers;
                }
            }
            if (refinementDecision.has_value() &&
                day == config.day_count()) {
                ++metrics.protectedTerminalAttempts;
                const udon::SimulationResult refinedDetailed =
                    simulator.simulate(
                        state,
                        refinementDecision->candidate.plan,
                        false);
                const udon::SimulationResult refinedIndependent =
                    validator.validate(
                        state,
                        refinementDecision->candidate.plan,
                        false);
                std::string refinedMismatch;
                metrics.protectedTerminalParentScore =
                    udon::OfficialScore::after_day(ledger, detailed.score);
                if (!refinedDetailed.valid ||
                    !validator.agrees_with(
                        refinedDetailed,
                        refinedIndependent,
                        refinedMismatch)) {
                    ++metrics.protectedTerminalInvalid;
                } else {
                    metrics.protectedTerminalRefinedScore =
                        udon::OfficialScore::after_day(
                            ledger,
                            refinedDetailed.score);
                    if (metrics.protectedTerminalParentScore <
                        metrics.protectedTerminalRefinedScore) {
                        detailed = refinedDetailed;
                        selectedDecision = &*refinementDecision;
                        ++metrics.protectedTerminalTakeovers;
                    }
                }
            }
            engine.record_submitted(*selectedDecision, elapsed);
        }
        if (options.protectedWaitClosedLoop &&
            day < config.day_count() &&
            !udon::protected_slack_transition_dominates(
                virtualDetailed,
                detailed)) {
            throw std::runtime_error(
                "protected virtual-parent state relation failed for " +
                fixture.name + " on day " + std::to_string(day));
        }
        ledger.apply(detailed.score);
        if (options.protectedWaitClosedLoop) {
            virtualLedger.apply(virtualDetailed.score);
            if (!udon::protected_slack_ledger_dominates(
                    virtualLedger,
                    ledger)) {
                throw std::runtime_error(
                    "protected virtual-parent ledger relation failed for " +
                    fixture.name + " on day " + std::to_string(day));
            }
            metrics.protectedVirtualDayScores.push_back(
                udon::OfficialScore{
                    virtualLedger.lifetime_distinct(),
                    virtualLedger.totalDailyDistinct,
                    virtualLedger.totalServings,
                });
        }
        metrics.exactDayScores.push_back(detailed.score);
        metrics.cumulativeDayScores.push_back(udon::OfficialScore{
            ledger.lifetime_distinct(),
            ledger.totalDailyDistinct,
            ledger.totalServings,
        });
        metrics.planHashes.push_back(appliedPlanHash);
        metrics.deadlineDays.push_back(decision.diagnostics.deadlineReached);
        metrics.exactSupportedAgents.push_back(
            decision.audit.columnGeneration.exactOrienteeringSupportedAgents);
        metrics.exactSettledStates.push_back(
            decision.audit.columnGeneration.exactOrienteeringSettledStates);
        metrics.exactSeedServings.push_back(
            decision.audit.columnGeneration.exactOrienteeringSeedServings);
        metrics.exactLocalServings.push_back(
            decision.audit.columnGeneration.exactOrienteeringLocalServings);
        agents = detailed.finalAgents;
        if (options.protectedWaitClosedLoop) {
            virtualAgents = virtualDetailed.finalAgents;
        }
        ownFootprints.at(static_cast<std::size_t>(day - 1)) =
            detailed.roadFootprint;
    }

    metrics.score = udon::OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
    if (options.protectedWaitClosedLoop) {
        metrics.protectedVirtualScore = udon::OfficialScore{
            virtualLedger.lifetime_distinct(),
            virtualLedger.totalDailyDistinct,
            virtualLedger.totalServings,
        };
    }
    return metrics;
}

[[nodiscard]] Options parse_options(int argumentCount, char** arguments) {
    Options options;
    for (int argument = 1; argument < argumentCount; ++argument) {
        const std::string value = arguments[argument];
        const auto next = [&]() -> std::string {
            if (argument + 1 >= argumentCount) {
                throw std::invalid_argument("missing value after " + value);
            }
            return arguments[++argument];
        };
        if (value == "--version") {
            options.version = next();
        } else if (value == "--track") {
            options.track = next();
        } else if (value == "--suite") {
            options.suite = next();
        } else if (value == "--first-seed") {
            options.firstSeed = std::stoull(next());
        } else if (value == "--seeds") {
            options.seedCount = std::stoi(next());
        } else if (value == "--budget-ms") {
            options.dayBudget = std::chrono::milliseconds{std::stoll(next())};
        } else if (value == "--role-ms") {
            options.roleBudget = std::chrono::milliseconds{std::stoll(next())};
        } else if (value == "--role-mode") {
            options.roleMode = next();
        } else if (value == "--role-mask") {
            options.fixedRoleMask = static_cast<std::uint32_t>(std::stoul(next()));
        } else if (value == "--protected-refine-ms") {
            options.protectedRefineBudget =
                std::chrono::milliseconds{std::stoll(next())};
        } else if (value == "--protected-wait-ms") {
            options.protectedWaitBudget =
                std::chrono::milliseconds{std::stoll(next())};
        } else if (value == "--protected-wait-detours") {
            options.protectedWaitDetours = true;
        } else if (value == "--protected-wait-closed-loop") {
            options.protectedWaitDetours = true;
            options.protectedWaitClosedLoop = true;
        } else if (value == "--day-details") {
            options.dayDetails = true;
        } else {
            throw std::invalid_argument("unknown argument: " + value);
        }
    }
    if (options.version.empty() || options.track.empty() ||
        options.seedCount <= 0 || options.dayBudget.count() <= 0 ||
        options.roleBudget.count() <= 0 ||
        options.protectedRefineBudget.count() < 0 ||
        options.protectedWaitBudget.count() < 0) {
        throw std::invalid_argument(
            "--version, --track, positive --seeds, --budget-ms and --role-ms are required");
    }
    if (options.roleMode != "exhaustive" &&
        options.roleMode != "deadline" &&
        options.roleMode != "fixed") {
        throw std::invalid_argument(
            "--role-mode must be exhaustive, deadline or fixed");
    }
    return options;
}

void print_result(
    const FixtureSpec& fixture,
    const Options& options,
    const Metrics& metrics) {
    const std::int64_t totalMilliseconds = std::accumulate(
        metrics.responseTimes.begin(),
        metrics.responseTimes.end(),
        std::int64_t{0});
    const std::int64_t meanMilliseconds = metrics.responseTimes.empty()
        ? 0
        : totalMilliseconds /
            static_cast<std::int64_t>(metrics.responseTimes.size());
    std::cout << "result"
              << ",version=" << options.version
              << ",track=" << options.track
              << ",suite=" << options.suite
              << ",budget_ms=" << options.dayBudget.count()
              << ",harvest_mode=" << kHarnessHarvestMode
              << ",future_harvest_mode=" << kHarnessFutureHarvestMode
              << ",role_mode=" << options.roleMode
              << ",fixture=" << fixture.name
              << ",family=" << fixture.family
              << ",seed=" << fixture.seed
              << ",lifetime=" << metrics.score.lifetimeDistinct
              << ",daily=" << metrics.score.totalDailyDistinct
              << ",servings=" << metrics.score.totalServings
              << ",invalid=" << metrics.invalidPlans
              << ",emergency=" << metrics.emergencyDays
              << ",search_complete_days=" << metrics.searchCompleteDays
              << ",search_deadline_days=" << metrics.searchDeadlineDays
              << ",combinations=" << metrics.combinationsVisited
              << ",role_mask=" << metrics.roleMask
              << ",exact_supported_agent_days="
              << std::accumulate(
                     metrics.exactSupportedAgents.begin(),
                     metrics.exactSupportedAgents.end(),
                     std::int64_t{0})
              << ",exact_settled="
              << std::accumulate(
                     metrics.exactSettledStates.begin(),
                     metrics.exactSettledStates.end(),
                     std::uint64_t{0})
              << ",role_ms=" << metrics.roleMilliseconds
              << ",protected_terminal_attempts="
              << metrics.protectedTerminalAttempts
              << ",protected_terminal_takeovers="
              << metrics.protectedTerminalTakeovers
              << ",protected_terminal_invalid="
              << metrics.protectedTerminalInvalid
              << ",protected_terminal_parent="
              << metrics.protectedTerminalParentScore.lifetimeDistinct << '/'
              << metrics.protectedTerminalParentScore.totalDailyDistinct << '/'
              << metrics.protectedTerminalParentScore.totalServings
              << ",protected_terminal_refined="
              << metrics.protectedTerminalRefinedScore.lifetimeDistinct << '/'
              << metrics.protectedTerminalRefinedScore.totalDailyDistinct << '/'
              << metrics.protectedTerminalRefinedScore.totalServings
              << ",protected_wait_plans=" << metrics.protectedWaitPlans
              << ",protected_wait_valid=" << metrics.protectedWaitValid
              << ",protected_wait_liftable=" << metrics.protectedWaitLiftable
              << ",protected_wait_best_day=" << metrics.protectedWaitBestDay
              << ",protected_wait_parent="
              << metrics.protectedWaitParentScore.lifetimeDistinct << '/'
              << metrics.protectedWaitParentScore.totalDailyDistinct << '/'
              << metrics.protectedWaitParentScore.totalServings
              << ",protected_wait_best="
              << metrics.protectedWaitBestScore.lifetimeDistinct << '/'
              << metrics.protectedWaitBestScore.totalDailyDistinct << '/'
              << metrics.protectedWaitBestScore.totalServings
              << ",protected_wait_takeovers="
              << metrics.protectedWaitTakeovers
              << ",protected_wait_deadline_days="
              << metrics.protectedWaitDeadlineDays
              << ",protected_virtual_score="
              << metrics.protectedVirtualScore.lifetimeDistinct << '/'
              << metrics.protectedVirtualScore.totalDailyDistinct << '/'
              << metrics.protectedVirtualScore.totalServings
              << ",protected_wait_witness_agent="
              << metrics.protectedWaitWitnessAgent
              << ",protected_wait_witness_anchor="
              << metrics.protectedWaitWitnessAnchor
              << ",protected_wait_witness_spot="
              << metrics.protectedWaitWitnessSpot
              << ",protected_wait_witness_duration="
              << metrics.protectedWaitWitnessDuration
              << ",protected_wait_witness_travel="
              << metrics.protectedWaitWitnessTravelSteps
              << ",protected_wait_witness_fuel="
              << metrics.protectedWaitWitnessParentFuel << '/'
              << metrics.protectedWaitWitnessCandidateFuel
              << ",protected_wait_witness_hash="
              << metrics.protectedWaitWitnessPlanHash
              << ",mean_ms=" << meanMilliseconds
              << ",p95_ms=" << percentile(metrics.responseTimes, 95)
              << ",max_ms=" << percentile(metrics.responseTimes, 100)
              << '\n';
    if (options.dayDetails) {
        for (std::size_t day = 0; day < metrics.exactDayScores.size(); ++day) {
            const udon::DayScore& exact = metrics.exactDayScores.at(day);
            const udon::OfficialScore& cumulative =
                metrics.cumulativeDayScores.at(day);
            std::cout << "day_detail"
                      << ",version=" << options.version
                      << ",track=" << options.track
                      << ",seed=" << fixture.seed
                      << ",day=" << day + 1U
                      << ",daily=" << exact.dailyDistinct
                      << '/' << exact.servings
                      << ",cumulative=" << cumulative.lifetimeDistinct
                      << '/' << cumulative.totalDailyDistinct
                      << '/' << cumulative.totalServings
                      << ",virtual_cumulative=";
            if (day < metrics.protectedVirtualDayScores.size()) {
                const udon::OfficialScore& virtualCumulative =
                    metrics.protectedVirtualDayScores.at(day);
                std::cout << virtualCumulative.lifetimeDistinct << '/'
                          << virtualCumulative.totalDailyDistinct << '/'
                          << virtualCumulative.totalServings;
            } else {
                std::cout << "0/0/0";
            }
            std::cout
                      << ",plan_hash=" << metrics.planHashes.at(day)
                      << ",response_ms=" << metrics.responseTimes.at(day)
                      << ",deadline=" << (metrics.deadlineDays.at(day) ? 1 : 0)
                      << ",exact_supported="
                      << metrics.exactSupportedAgents.at(day)
                      << ",exact_settled="
                      << metrics.exactSettledStates.at(day)
                      << ",exact_seed_servings="
                      << metrics.exactSeedServings.at(day)
                      << ",exact_local_servings="
                      << metrics.exactLocalServings.at(day)
                      << '\n';
        }
    }
}

}  // namespace

int main(int argumentCount, char** arguments) {
    try {
        const Options options = parse_options(argumentCount, arguments);
        for (std::int32_t offset = 0; offset < options.seedCount; ++offset) {
            const FixtureSpec fixture = make_fixture(
                options,
                options.firstSeed + static_cast<std::uint64_t>(offset));
            print_result(fixture, options, run_fixture(fixture, options));
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "historical tournament failed: " << error.what() << '\n';
        return 1;
    }
}
