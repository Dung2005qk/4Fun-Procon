// ATTR-COVERAGE-REGIME-208 read-only attribution probe.
//
// Reproduces the column-generation starvation attributed in the consumed BTC
// replays m-3986 (32 brands == 32 spots: tier-2 collapse) and m-3897
// (30 spots, players=2, 6 brands: tier-3 servings loss on starved days) on
// the registered SYN-COLGEN-208 synthetic family. The probe runs the
// unchanged production planner day loop (harvest mode 7, single-pass pool,
// fixed roles) and dumps per-day column-generation diagnostics so the
// earliest failing stage can be attributed under controlled traffic,
// spot-density, day-steps and brand-scarcity axes. No production source is
// touched; fixtures follow research/holdouts/ATTR-COVERAGE-REGIME-208.csv.
//
// Usage: colgen_starvation_probe <seed> <spotCount> <daySteps>
//            <brandMode:mod6|distinct> <players:2|4> [budgetMs=3375]

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "udon/decision.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"

namespace {

constexpr std::int32_t kHarvestMode = 7;
constexpr std::int32_t kFutureHarvestMode = 7;

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
    std::vector<udon::CellId> starts;
    std::vector<std::int32_t> daySteps;
    std::int32_t fuelLimit = 16;
    std::int32_t players = 3;
    std::int32_t busyThreshold = 3;
    std::int32_t jammedThreshold = 6;
};

[[nodiscard]] std::vector<std::int32_t> plain_map(std::size_t cellCount) {
    return std::vector<std::int32_t>(
        cellCount,
        static_cast<std::int32_t>(udon::Terrain::Plain));
}

// Byte-copy of old/harness/historical_tournament.cpp generated_btc_large_fixture
// at commit 690728a with the three registered SYN-COLGEN-208 overrides:
// dayStepsValue, brandMode (mod6|distinct) and players; fuelLimit is
// 3 * dayStepsValue to mirror the consumed replays.
[[nodiscard]] FixtureSpec syn_colgen_fixture(
    std::uint64_t seed,
    std::int32_t spotCount,
    std::int32_t dayStepsValue,
    const std::string& brandMode,
    std::int32_t players) {
    static constexpr std::array<const char*, 6> families{
        "balanced",
        "rare-brand",
        "threshold-corridor",
        "fuel-tight",
        "high-stock",
        "overnight",
    };
    constexpr std::int32_t side = 32;
    constexpr std::int32_t days = 10;
    constexpr std::int32_t agentCount = 8;
    const std::int32_t cellCount = side * side;
    FixtureSpec fixture;
    fixture.seed = seed;
    fixture.family = families.at(static_cast<std::size_t>(seed % families.size()));
    fixture.name = "syn-colgen-208-" + fixture.family + "-seed-" + std::to_string(seed);
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
    const auto addRoad = [&fixture, &roadCells](
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
    if (spotCount <= 0 ||
        static_cast<std::size_t>(agentCount + spotCount) > plainCells.size()) {
        throw std::invalid_argument("fixture spot count exceeds plain cells");
    }
    fixture.spots.reserve(static_cast<std::size_t>(spotCount));
    for (std::int32_t spotIndex = 0; spotIndex < spotCount; ++spotIndex) {
        std::int32_t brandIndex = spotIndex % 6;
        if (brandMode == "distinct") {
            brandIndex = spotIndex;
        } else if (fixture.family == "rare-brand") {
            brandIndex = spotIndex == spotCount - 1
                ? 5
                : spotIndex % 5;
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
    fixture.daySteps.assign(static_cast<std::size_t>(days), dayStepsValue);
    fixture.fuelLimit = 3 * dayStepsValue;
    fixture.players = players;
    fixture.busyThreshold = 5;
    fixture.jammedThreshold = 10;
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

template <typename Value>
void print_list(std::ostream& output, const std::vector<Value>& values) {
    output << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0U) {
            output << ' ';
        }
        output << values.at(index);
    }
    output << ']';
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 6) {
            std::cerr << "usage: colgen_starvation_probe <seed> <spotCount> "
                         "<daySteps> <brandMode:mod6|distinct> <players:2|4> "
                         "[budgetMs=3375]\n";
            return 2;
        }
        const std::uint64_t seed = std::stoull(argv[1]);
        const std::int32_t spotCount = std::stoi(argv[2]);
        const std::int32_t dayStepsValue = std::stoi(argv[3]);
        const std::string brandMode = argv[4];
        const std::int32_t players = std::stoi(argv[5]);
        const std::chrono::milliseconds dayBudget(
            argc > 6 ? std::stoi(argv[6]) : 3375);
        if (brandMode != "mod6" && brandMode != "distinct") {
            throw std::invalid_argument("brand mode must be mod6 or distinct");
        }

        const FixtureSpec fixture = syn_colgen_fixture(
            seed, spotCount, dayStepsValue, brandMode, players);
        const udon::MatchConfig config = udon::parse_match_config(
            udon::JsonValue::parse(config_document(fixture)));
        udon::UdonShieldEngine engine(
            config,
            {},
            {},
            udon::RoutePoolSearch::SinglePass,
            kHarvestMode,
            false,
            kFutureHarvestMode);

        // Fixed roles, tanker = agent 0 (mirrors consumed m-3986).
        std::vector<udon::AgentKind> roles(
            static_cast<std::size_t>(config.agent_count()),
            udon::AgentKind::Patrol);
        roles.at(0) = udon::AgentKind::Tanker;

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
            std::vector<std::int32_t>(
                static_cast<std::size_t>(config.map.cell_count()),
                0));
        const std::vector<std::vector<std::int32_t>> opponentFootprints =
            opponent_footprints(fixture, config);

        std::cout << "probe_begin,fixture=" << fixture.name
                  << ",family=" << fixture.family
                  << ",spots=" << spotCount
                  << ",daySteps=" << dayStepsValue
                  << ",brandMode=" << brandMode
                  << ",players=" << players
                  << ",fuel=" << fixture.fuelLimit
                  << ",budgetMs=" << dayBudget.count() << '\n';

        std::int32_t starvedDays = 0;
        std::int32_t stubDays = 0;
        std::int32_t previousDaily = 0;
        std::int64_t previousServings = 0;
        for (std::int32_t day = 1; day <= config.day_count(); ++day) {
            udon::DayState state;
            state.endsAt = config.startsAt + static_cast<std::int64_t>(day) * 5;
            state.dayNumber = day;
            state.agents = agents;
            state.roadStatuses =
                road_statuses(config, day, ownFootprints, opponentFootprints);

            const auto started = std::chrono::steady_clock::now();
            const udon::DecisionResult decision =
                engine.solve_day(state, ledger, dayBudget);
            const std::chrono::milliseconds elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - started);

            udon::SimulationResult detailed =
                simulator.simulate(state, decision.candidate.plan, false);
            const udon::SimulationResult independent =
                validator.validate(state, decision.candidate.plan, false);
            std::string mismatch;
            if (!detailed.valid ||
                !validator.agrees_with(detailed, independent, mismatch)) {
                throw std::runtime_error(
                    "plan invalid on day " + std::to_string(day) + ": " + mismatch);
            }
            engine.record_submitted(decision, elapsed);
            ledger.apply(detailed.score);

            const udon::DecisionAudit& audit = decision.audit;
            const udon::ColumnGenerationDiagnostics& colgen = audit.columnGeneration;
            std::vector<std::int32_t> starvedAgents;
            std::vector<std::int32_t> stubAgents;
            for (std::size_t index = 0; index < colgen.agentParetoQueries.size(); ++index) {
                if (colgen.agentParetoQueries.at(index) == 0 &&
                    roles.at(index) == udon::AgentKind::Patrol) {
                    starvedAgents.push_back(static_cast<std::int32_t>(index));
                }
            }
            for (std::size_t index = 0; index < audit.portfolioColumnsByAgent.size(); ++index) {
                if (audit.portfolioColumnsByAgent.at(index) <= 3 &&
                    roles.at(index) == udon::AgentKind::Patrol) {
                    stubAgents.push_back(static_cast<std::int32_t>(index));
                }
            }
            starvedDays += starvedAgents.empty() ? 0 : 1;
            stubDays += stubAgents.empty() ? 0 : 1;

            const std::int32_t dailyDelta =
                ledger.totalDailyDistinct - previousDaily;
            const std::int64_t servingsDelta =
                ledger.totalServings - previousServings;
            previousDaily = ledger.totalDailyDistinct;
            previousServings = ledger.totalServings;

            std::cout << "day=" << day
                      << ",dailyDelta=" << dailyDelta
                      << ",servDelta=" << servingsDelta
                      << ",cum=" << ledger.lifetime_distinct()
                      << '/' << ledger.totalDailyDistinct
                      << '/' << ledger.totalServings
                      << ",cgDeadline=" << (colgen.deadlineReached ? 1 : 0)
                      << ",starved=";
            print_list(std::cout, starvedAgents);
            std::cout << ",stub=";
            print_list(std::cout, stubAgents);
            std::cout << ",cols=";
            print_list(std::cout, audit.portfolioColumnsByAgent);
            std::cout << ",brands=";
            print_list(std::cout, audit.portfolioBrandCountsByAgent);
            std::cout << ",agentMs=";
            print_list(std::cout, colgen.agentMilliseconds);
            std::cout << ",agentQ=";
            print_list(std::cout, colgen.agentParetoQueries);
            std::cout << ",cacheHits=" << colgen.pareto.cacheHits
                      << ",cacheMisses=" << colgen.pareto.cacheMisses
                      << ",labels=" << colgen.pareto.labelsGenerated
                      << ",paretoQueries=" << colgen.pareto.queries
                      << ",combos=" << decision.diagnostics.combinationsVisited
                      << ",masterDeadline="
                      << (decision.diagnostics.deadlineReached ? 1 : 0)
                      << ",colGenMs=" << decision.timing.columnGeneration.count()
                      << ",searchMs=" << decision.timing.search.count()
                      << ",totalMs=" << decision.timing.total.count()
                      << '\n';

            agents = detailed.finalAgents;
            ownFootprints.at(static_cast<std::size_t>(day - 1)) =
                detailed.roadFootprint;
        }

        std::cout << "probe_result,fixture=" << fixture.name
                  << ",spots=" << spotCount
                  << ",daySteps=" << dayStepsValue
                  << ",brandMode=" << brandMode
                  << ",players=" << players
                  << ",score=" << ledger.lifetime_distinct()
                  << '/' << ledger.totalDailyDistinct
                  << '/' << ledger.totalServings
                  << ",starvedDays=" << starvedDays
                  << ",stubDays=" << stubDays << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "colgen starvation probe failed: " << error.what() << '\n';
        return 1;
    }
}
