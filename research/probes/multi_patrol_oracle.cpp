#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "udon/decision.hpp"
#include "udon/json.hpp"
#include "udon/orienteering.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"

namespace {

constexpr std::chrono::milliseconds kProductionBudget{5000};
constexpr std::int32_t kHarvestMode = 7;
constexpr std::int32_t kFutureHarvestMode = 7;
constexpr std::size_t kTrackedRoadCapacity = 2U;
using TrafficFootprint = std::array<std::uint8_t, kTrackedRoadCapacity>;

struct ManifestRow {
    std::string experimentId;
    std::string family;
    std::string fuelProfile;
    std::int32_t horizon = 0;
    std::uint64_t firstSeed = 0;
    std::int32_t count = 0;
};

struct Options {
    std::string manifest = "research/holdouts/CEILING-MULTI-PATROL-085.csv";
    std::string split = "development";
    std::int32_t maximumMatches = 0;
    std::uint64_t onlySeed = 0;
    bool details = false;
    bool headOnly = false;
};

struct Fixture {
    udon::MatchConfig config;
    std::string family;
    std::string fuelProfile;
    std::uint64_t seed = 0;
    bool trafficAware = false;
    std::vector<TrafficFootprint> opponentFootprints;
};

struct DayOutcome {
    udon::CellId position = udon::kInvalidCell;
    std::int32_t fuel = 0;
    std::uint32_t spotMask = 0;
    udon::AgentPlan actions;
    TrafficFootprint roadFootprint{};
};

using MatchKey = std::tuple<
    udon::CellId,
    std::int32_t,
    udon::CellId,
    std::int32_t,
    std::uint64_t,
    TrafficFootprint,
    TrafficFootprint>;

struct LayerEntry {
    MatchKey key{};
    std::int32_t totalDailyDistinct = 0;
    std::int32_t totalServings = 0;
    std::int32_t parentIndex = -1;
    udon::AgentPlan firstPlan;
    udon::AgentPlan secondPlan;
    bool swapForNextDay = false;
};

struct OracleResult {
    bool valid = false;
    udon::OfficialScore score;
    std::vector<udon::DayPlan> plans;
    std::vector<udon::OfficialScore> cumulative;
    std::vector<std::vector<udon::AgentState>> terminalAgents;
    std::size_t maximumFrontier = 0;
    std::size_t dailyEnumerations = 0;
};

struct HeadResult {
    bool valid = false;
    udon::OfficialScore score;
    std::vector<udon::DayPlan> plans;
    std::vector<udon::OfficialScore> cumulative;
    std::vector<std::vector<udon::AgentState>> terminalAgents;
    std::vector<udon::DecisionAudit> audits;
    struct CachePathAudit {
        std::int32_t cached = 0;
        std::int32_t eligible = 0;
        std::int32_t reused = 0;
        std::int32_t rejected = 0;
        std::int32_t retained = 0;
        bool selected = false;
        bool cachedValid = false;
        udon::OfficialScore cachedCurrent;
        udon::OfficialScore cachedUpper;
        udon::OfficialScore selectedCurrent;
        udon::OfficialScore selectedUpper;
        std::int32_t sourceMultiDayWitnesses = 0;
        std::int32_t sourceMaximumPlans = 0;
        udon::OfficialScore sourceFinal;
        bool suffixAttempted = false;
        bool suffixValid = false;
        bool suffixScoreMatches = false;
        udon::OfficialScore suffixReplayScore;
    };
    std::vector<CachePathAudit> cachePaths;
};

struct Summary {
    std::int32_t cases = 0;
    std::int32_t oracleWins = 0;
    std::int32_t ties = 0;
    std::int32_t headWins = 0;
    std::int32_t invalid = 0;
    std::int32_t tier1 = 0;
    std::int32_t tier2 = 0;
    std::int32_t tier3 = 0;
    std::int32_t maximumGain = 0;
    std::uint64_t resultHash = 1469598103934665603ULL;
};

[[nodiscard]] std::uint64_t mix64(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

void hash_value(std::uint64_t& hash, std::uint64_t value) {
    for (std::int32_t byte = 0; byte < 8; ++byte) {
        hash ^= (value >> static_cast<std::uint32_t>(byte * 8)) & 0xffU;
        hash *= 1099511628211ULL;
    }
}

[[nodiscard]] std::string evaluator_contract_hash() {
    constexpr std::string_view contract =
        "lexicographic-risk-comparator-v1|exact-step-simulator-v1|independent-validator-v1";
    std::uint64_t hash = 14695981039346656037ULL;
    for (const char byte : contract) {
        hash ^= static_cast<std::uint8_t>(byte);
        hash *= 1099511628211ULL;
    }
    return "fnv1a64:" + std::to_string(hash);
}

[[nodiscard]] std::vector<std::string> split_csv(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream input(line);
    std::string field;
    while (std::getline(input, field, ',')) {
        fields.push_back(field);
    }
    return fields;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
    Options options;
    for (std::int32_t index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--manifest" && index + 1 < argc) {
            options.manifest = argv[++index];
        } else if (argument == "--split" && index + 1 < argc) {
            options.split = argv[++index];
        } else if (argument == "--maximum-matches" && index + 1 < argc) {
            options.maximumMatches = std::stoi(argv[++index]);
        } else if (argument == "--only-seed" && index + 1 < argc) {
            options.onlySeed = std::stoull(argv[++index]);
        } else if (argument == "--details") {
            options.details = true;
        } else if (argument == "--head-only") {
            options.headOnly = true;
        } else {
            throw std::invalid_argument("unknown or incomplete option: " + argument);
        }
    }
    if (options.split != "development" && options.split != "holdout") {
        throw std::invalid_argument("split must be development or holdout");
    }
    if (options.maximumMatches < 0) {
        throw std::invalid_argument("maximum matches cannot be negative");
    }
    return options;
}

[[nodiscard]] std::vector<ManifestRow> load_manifest(
    const std::string& path,
    const std::string& split) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open manifest: " + path);
    }
    std::string line;
    if (!std::getline(input, line) ||
        line != "experiment_id,split,family,fuel_profile,horizon,first_seed,count,active_agents,total_agents,spot_count,role_mode,oracle_scope") {
        throw std::runtime_error("unexpected multi-patrol oracle manifest schema");
    }
    std::vector<ManifestRow> rows;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::vector<std::string> fields = split_csv(line);
        if (fields.size() != 12U ||
            (fields.at(0) != "CEILING-MULTI-PATROL-085" &&
             fields.at(0) != "CEILING-BRANCH-PATROL-089" &&
             fields.at(0) != "CEILING-CYCLE-PATROL-095" &&
             fields.at(0) != "CEILING-TRAFFIC-PATROL-097" &&
             fields.at(0) != "SCORE-MASTER-ADDITIVE-100" &&
             fields.at(0) != "CEILING-LADDER-PATROL-101" &&
             fields.at(0) != "SCORE-W1-TERMINAL-FRONTIER-109" &&
             fields.at(0) != "ATTR-W0-CACHE-RETENTION-110" &&
             fields.at(0) != "SCORE-W0-SUFFIX-PRESERVE-113" &&
             fields.at(0) != "SCORE-W1-CLOSED-LOOP-114")) {
            throw std::runtime_error("invalid manifest row: " + line);
        }
        if (fields.at(1) != split) {
            continue;
        }
        const std::string expectedSpots = fields.at(0) == "CEILING-MULTI-PATROL-085"
            ? "5"
            : "6";
        const std::string expectedScope =
            fields.at(0) == "CEILING-TRAFFIC-PATROL-097"
            ? "complete-two-active-patrol-own-traffic-full-match-dp"
            : "complete-two-active-patrol-full-match-dp";
        if (fields.at(7) != "2" || fields.at(8) != "3" ||
            fields.at(9) != expectedSpots ||
            fields.at(10) != "all-patrol" ||
            fields.at(11) != expectedScope) {
            throw std::runtime_error("manifest row violates oracle scope: " + line);
        }
        rows.push_back(ManifestRow{
            fields.at(0),
            fields.at(2),
            fields.at(3),
            std::stoi(fields.at(4)),
            std::stoull(fields.at(5)),
            std::stoi(fields.at(6)),
        });
    }
    if (rows.empty()) {
        throw std::runtime_error("manifest contains no rows for split " + split);
    }
    return rows;
}

[[nodiscard]] std::vector<std::int32_t> base_brands(const std::string& family) {
    if (family == "duplicate-brand") {
        return {100, 100, 200, 300, 400};
    }
    if (family == "coverage-trap") {
        return {100, 100, 200, 200, 999};
    }
    if (family == "terminal-separation") {
        return {100, 200, 200, 300, 900};
    }
    if (family == "balanced" || family == "stock-contention" ||
        family == "fuel-allocation") {
        return {100, 200, 300, 400, 500};
    }
    if (family == "branched-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "rare-fork") {
        return {100, 100, 200, 200, 300, 999};
    }
    if (family == "terminal-fork") {
        return {100, 200, 300, 300, 400, 900};
    }
    if (family == "branched-balanced" || family == "stock-race" ||
        family == "fuel-split") {
        return {100, 200, 300, 400, 500, 600};
    }
    if (family == "cycle-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "rare-loop") {
        return {100, 100, 200, 200, 300, 999};
    }
    if (family == "terminal-loop") {
        return {100, 200, 300, 300, 400, 900};
    }
    if (family == "cycle-balanced" || family == "stock-crossing" ||
        family == "fuel-circuit") {
        return {100, 200, 300, 400, 500, 600};
    }
    if (family == "perimeter-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "rare-perimeter") {
        return {100, 100, 200, 200, 300, 999};
    }
    if (family == "terminal-perimeter") {
        return {100, 200, 300, 300, 400, 900};
    }
    if (family == "perimeter-balanced" || family == "stock-perimeter" ||
        family == "fuel-perimeter") {
        return {100, 200, 300, 400, 500, 600};
    }
    if (family == "ladder-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "rare-ladder") {
        return {100, 100, 200, 200, 300, 999};
    }
    if (family == "terminal-ladder") {
        return {100, 200, 300, 300, 400, 900};
    }
    if (family == "ladder-balanced" || family == "stock-ladder" ||
        family == "fuel-ladder") {
        return {100, 200, 300, 400, 500, 600};
    }
    if (family == "traffic-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "traffic-terminal") {
        return {100, 200, 300, 300, 400, 900};
    }
    if (family == "traffic-balanced" || family == "threshold-loop" ||
        family == "jam-loop" || family == "traffic-stock") {
        return {100, 200, 300, 400, 500, 600};
    }
    if (family == "diamond-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "rare-diamond") {
        return {100, 100, 200, 200, 300, 999};
    }
    if (family == "terminal-diamond") {
        return {100, 200, 200, 300, 400, 900};
    }
    if (family == "diamond-balanced" || family == "stock-diamond" ||
        family == "fuel-diamond") {
        return {100, 200, 300, 400, 500, 600};
    }
    throw std::invalid_argument("unknown family: " + family);
}

[[nodiscard]] Fixture make_fixture(const ManifestRow& row, std::uint64_t seed) {
    constexpr std::int32_t side = 8;
    constexpr std::int32_t cells = side * side;
    std::vector<std::int32_t> terrain(
        static_cast<std::size_t>(cells),
        static_cast<std::int32_t>(udon::Terrain::Pond));
    terrain.at(0) = static_cast<std::int32_t>(udon::Terrain::Plain);
    const bool branched =
        row.experimentId == "CEILING-BRANCH-PATROL-089" ||
        row.experimentId == "SCORE-W1-TERMINAL-FRONTIER-109" ||
        row.experimentId == "ATTR-W0-CACHE-RETENTION-110";
    const bool cyclic =
        row.experimentId == "CEILING-CYCLE-PATROL-095" ||
        row.experimentId == "SCORE-W0-SUFFIX-PRESERVE-113";
    const bool trafficAware = row.experimentId == "CEILING-TRAFFIC-PATROL-097";
    const bool perimeter = row.experimentId == "SCORE-MASTER-ADDITIVE-100";
    const bool ladder = row.experimentId == "CEILING-LADDER-PATROL-101";
    const bool diamond = row.experimentId == "SCORE-W1-CLOSED-LOOP-114";
    std::vector<udon::CellId> spotCells;
    if (diamond) {
        for (const udon::CellId cell :
             {17, 18, 19, 20, 25, 26, 27, 28, 34, 35, 36}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        terrain.at(27) = static_cast<std::int32_t>(udon::Terrain::Mountain);
        spotCells = {17, 20, 25, 28, 34, 36};
    } else if (trafficAware) {
        for (const udon::CellId cell : {17, 18, 19, 20, 25, 27, 28, 33, 34, 35, 36}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        terrain.at(18) = static_cast<std::int32_t>(udon::Terrain::Road);
        terrain.at(27) = static_cast<std::int32_t>(udon::Terrain::Mountain);
        terrain.at(35) = static_cast<std::int32_t>(udon::Terrain::Road);
        spotCells = {17, 19, 20, 33, 34, 36};
    } else if (ladder) {
        for (const udon::CellId cell : {18, 19, 20, 21, 26, 27, 28, 29}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        spotCells = {19, 20, 21, 26, 27, 28};
    } else if (perimeter) {
        for (const udon::CellId cell : {18, 19, 20, 21, 26, 29, 34, 35, 36, 37}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        terrain.at(26) = static_cast<std::int32_t>(udon::Terrain::Mountain);
        spotCells = {19, 20, 21, 29, 35, 36};
    } else if (cyclic) {
        for (const udon::CellId cell : {18, 19, 20, 26, 27, 28, 34, 35, 36}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        terrain.at(27) = static_cast<std::int32_t>(udon::Terrain::Mountain);
        spotCells = {19, 20, 26, 28, 34, 35};
    } else if (branched) {
        for (const udon::CellId cell : {10, 11, 16, 17, 18, 19, 20, 21, 26}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        terrain.at(11) = static_cast<std::int32_t>(udon::Terrain::Mountain);
        spotCells = {10, 17, 18, 19, 20, 26};
    } else {
        for (udon::CellId cell = 16; cell <= 22; ++cell) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        spotCells = {17, 18, 19, 20, 21};
    }
    std::sort(
        spotCells.begin(),
        spotCells.end(),
        [seed](udon::CellId left, udon::CellId right) {
            return mix64(seed ^ (static_cast<std::uint64_t>(left) << 17U)) <
                mix64(seed ^ (static_cast<std::uint64_t>(right) << 17U));
        });
    std::vector<std::int32_t> brands = base_brands(row.family);
    if (row.family == "terminal-separation" || row.family == "terminal-fork" ||
        row.family == "terminal-loop" || row.family == "traffic-terminal" ||
        row.family == "terminal-perimeter" || row.family == "terminal-ladder" ||
        row.family == "terminal-diamond") {
        const udon::CellId protectedTerminal = trafficAware
            ? 33
            : (diamond ? 36 : (ladder ? 28 : (perimeter ? 36 : (cyclic ? 35 : (branched ? 26 : 19)))));
        const auto center = std::find(
            spotCells.begin(),
            spotCells.end(),
            protectedTerminal);
        if (center != spotCells.end()) {
            std::iter_swap(center, spotCells.end() - 1);
        }
    }

    std::vector<std::int32_t> daySteps;
    for (std::int32_t day = 0; day < row.horizon; ++day) {
        daySteps.push_back(16 + static_cast<std::int32_t>(
            mix64(seed ^ (static_cast<std::uint64_t>(day) << 39U)) % 3U));
    }
    std::int32_t fuelLimit =
        (branched || cyclic || trafficAware || perimeter || diamond) ? 24 : 20;
    if (row.fuelProfile == "low") {
        fuelLimit =
            (branched || cyclic || trafficAware || perimeter || diamond) ? 12 : 10;
    } else if (row.fuelProfile == "high") {
        fuelLimit = 8 * row.horizon;
    } else if (row.fuelProfile != "default") {
        throw std::invalid_argument("unknown fuel profile: " + row.fuelProfile);
    }

    std::ostringstream document;
    document << "{\"startsAt\":1778227200,\"daySeconds\":[";
    for (std::int32_t day = 0; day < row.horizon; ++day) {
        if (day != 0) {
            document << ',';
        }
        document << 5;
    }
    document << "],\"daySteps\":[";
    for (std::size_t day = 0; day < daySteps.size(); ++day) {
        if (day != 0U) {
            document << ',';
        }
        document << daySteps.at(day);
    }
    document << "],\"map\":{\"height\":8,\"width\":8,\"cells\":[";
    for (std::int32_t mapRow = 0; mapRow < side; ++mapRow) {
        if (mapRow != 0) {
            document << ',';
        }
        document << '[';
        for (std::int32_t column = 0; column < side; ++column) {
            if (column != 0) {
                document << ',';
            }
            document << terrain.at(static_cast<std::size_t>(mapRow * side + column));
        }
        document << ']';
    }
    document << "]},\"spots\":[";
    for (std::size_t spot = 0; spot < spotCells.size(); ++spot) {
        if (spot != 0U) {
            document << ',';
        }
        std::int32_t stock = 2;
        if (row.family == "stock-contention" || row.family == "coverage-trap" ||
            row.family == "stock-race" || row.family == "rare-fork" ||
            row.family == "stock-crossing" || row.family == "rare-loop" ||
            row.family == "traffic-stock" || row.family == "stock-perimeter" ||
            row.family == "rare-perimeter" || row.family == "stock-ladder" ||
            row.family == "rare-ladder" || row.family == "stock-diamond" ||
            row.family == "rare-diamond") {
            stock = 1;
        } else if (row.family == "fuel-allocation" || row.family == "fuel-split" ||
            row.family == "fuel-circuit" || row.family == "fuel-perimeter" ||
            row.family == "fuel-ladder" || row.family == "fuel-diamond") {
            stock = 1 + static_cast<std::int32_t>((spot + seed) % 2U);
        }
        document << "{\"brand\":" << brands.at(spot)
                 << ",\"pos\":" << spotCells.at(spot)
                 << ",\"stocks\":" << stock << '}';
    }
    document << "],\"agents\":[" << (diamond ? 18 : (trafficAware ? 25 : (ladder ? 18 : (perimeter ? 18 : (cyclic ? 18 : 16))))) << ','
             << (diamond ? 35 : (trafficAware ? 28 : (ladder ? 29 : (perimeter ? 37 : (cyclic ? 36 : (branched ? 21 : 22))))))
             << ",0],\"fuelLimits\":" << fuelLimit
             << ",\"players\":4,\"busyThreshold\":2,\"jammedThreshold\":4}";

    Fixture fixture;
    fixture.config = udon::parse_match_config(udon::JsonValue::parse(document.str()));
    fixture.family = row.family;
    fixture.fuelProfile = row.fuelProfile;
    fixture.seed = seed;
    fixture.trafficAware = trafficAware;
    fixture.opponentFootprints.assign(
        static_cast<std::size_t>(row.horizon),
        TrafficFootprint{});
    if (fixture.config.roadCells.size() > kTrackedRoadCapacity) {
        throw std::runtime_error("traffic oracle road capacity exceeded");
    }
    if (trafficAware) {
        for (std::int32_t day = 0; day < row.horizon; ++day) {
            for (std::size_t roadIndex = 0;
                 roadIndex < fixture.config.roadCells.size();
                 ++roadIndex) {
                const udon::CellId road = fixture.config.roadCells.at(roadIndex);
                const std::uint64_t draw = mix64(
                    seed ^ (static_cast<std::uint64_t>(day) << 41U) ^
                    (static_cast<std::uint64_t>(road) << 13U));
                std::int32_t stays = static_cast<std::int32_t>(draw % 4U);
                if (row.family == "threshold-loop") {
                    stays = fixture.config.players * fixture.config.busyThreshold - 3 +
                        static_cast<std::int32_t>(draw % 3U);
                } else if (row.family == "jam-loop") {
                    stays = fixture.config.players * fixture.config.jammedThreshold - 4 +
                        static_cast<std::int32_t>(draw % 4U);
                }
                fixture.opponentFootprints.at(static_cast<std::size_t>(day)).at(
                    roadIndex) = static_cast<std::uint8_t>(std::clamp(
                        stays,
                        0,
                        fixture.config.players * fixture.config.jammedThreshold));
            }
        }
    }
    if (!trafficAware && !fixture.config.roadCells.empty()) {
        throw std::runtime_error("multi-patrol exact domain unexpectedly contains roads");
    }
    return fixture;
}

[[nodiscard]] udon::DayState day_state(
    const udon::MatchConfig& config,
    std::int32_t day,
    const std::vector<udon::AgentState>& agents,
    const std::vector<udon::RoadStatus>& roadStatuses) {
    udon::DayState state;
    state.endsAt = config.startsAt + static_cast<std::int64_t>(day) * 5;
    state.dayNumber = day;
    state.agents = agents;
    state.roadStatuses = roadStatuses;
    return state;
}

[[nodiscard]] udon::DayState day_state(
    const udon::MatchConfig& config,
    std::int32_t day,
    const std::vector<udon::AgentState>& agents) {
    return day_state(
        config,
        day,
        agents,
        std::vector<udon::RoadStatus>(
            static_cast<std::size_t>(config.map.cell_count()),
            udon::RoadStatus::Smooth));
}

[[nodiscard]] std::vector<udon::RoadStatus> exact_road_statuses(
    const Fixture& fixture,
    std::int32_t day,
    const TrafficFootprint& previousOwn,
    const TrafficFootprint& priorOwn) {
    std::vector<udon::RoadStatus> statuses(
        static_cast<std::size_t>(fixture.config.map.cell_count()),
        udon::RoadStatus::Smooth);
    if (!fixture.trafficAware) {
        return statuses;
    }
    for (std::size_t roadIndex = 0;
         roadIndex < fixture.config.roadCells.size();
         ++roadIndex) {
        const udon::CellId road = fixture.config.roadCells.at(roadIndex);
        const std::size_t cellOffset = static_cast<std::size_t>(road);
        std::int32_t stays = previousOwn.at(roadIndex) + priorOwn.at(roadIndex);
        for (std::int32_t history = 1; history <= 2; ++history) {
            const std::int32_t completedDay = day - history;
            if (completedDay > 0) {
                stays += fixture.opponentFootprints
                    .at(static_cast<std::size_t>(completedDay - 1))
                    .at(roadIndex);
            }
        }
        if (stays >= fixture.config.players * fixture.config.jammedThreshold) {
            statuses.at(cellOffset) = udon::RoadStatus::Jammed;
        } else if (stays >= fixture.config.players * fixture.config.busyThreshold) {
            statuses.at(cellOffset) = udon::RoadStatus::Busy;
        }
    }
    return statuses;
}

[[nodiscard]] std::size_t road_offset(
    const udon::MatchConfig& config,
    udon::CellId road) {
    const auto found = std::find(
        config.roadCells.begin(),
        config.roadCells.end(),
        road);
    if (found == config.roadCells.end()) {
        throw std::runtime_error("road footprint requested for a non-road cell");
    }
    return static_cast<std::size_t>(found - config.roadCells.begin());
}

[[nodiscard]] TrafficFootprint compact_road_footprint(
    const udon::MatchConfig& config,
    const std::vector<std::int32_t>& fullFootprint) {
    TrafficFootprint compact{};
    const std::int32_t saturation = config.players * config.jammedThreshold;
    for (std::size_t roadIndex = 0;
         roadIndex < config.roadCells.size();
         ++roadIndex) {
        compact.at(roadIndex) = static_cast<std::uint8_t>(std::clamp(
            fullFootprint.at(
                static_cast<std::size_t>(config.roadCells.at(roadIndex))),
            0,
            saturation));
    }
    return compact;
}

void add_traffic_stays(
    const udon::MatchConfig& config,
    TrafficFootprint& footprint,
    std::size_t roadIndex,
    std::int32_t stays) {
    const std::int32_t saturation = config.players * config.jammedThreshold;
    footprint.at(roadIndex) = static_cast<std::uint8_t>(std::min(
        saturation,
        static_cast<std::int32_t>(footprint.at(roadIndex)) + stays));
}

[[nodiscard]] bool validates(
    const udon::MatchConfig& config,
    const udon::DayState& state,
    const udon::DayPlan& plan,
    udon::SimulationResult& simulation,
    std::string& mismatch) {
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    simulation = simulator.simulate(state, plan, false);
    const udon::SimulationResult independent = validator.validate(state, plan, false);
    return simulation.valid && independent.valid &&
        validator.agrees_with(simulation, independent, mismatch);
}

[[nodiscard]] std::vector<DayOutcome> enumerate_day(
    const udon::MatchConfig& config,
    std::int32_t day,
    udon::CellId start,
    std::int32_t availableFuel,
    const std::vector<udon::RoadStatus>& roadStatuses) {
    struct Node {
        std::int32_t steps = 0;
        std::int32_t fuelUsed = 0;
        udon::CellId cell = udon::kInvalidCell;
        std::uint32_t spotMask = 0;
        TrafficFootprint roadFootprint{};
        std::int32_t parent = -1;
        udon::PlanAction incoming = udon::PlanAction::wait(1);
    };
    const std::int32_t limit = config.steps_for_day(day);
    std::vector<Node> nodes{
        Node{
            0,
            0,
            start,
            0U,
            TrafficFootprint{},
            -1,
            udon::PlanAction::wait(1)},
    };
    using NodeKey = std::tuple<
        std::int32_t,
        std::int32_t,
        udon::CellId,
        std::uint32_t,
        TrafficFootprint>;
    std::set<NodeKey> seen;
    seen.emplace(0, 0, start, 0U, nodes.front().roadFootprint);
    const auto push = [&nodes, &seen](const Node& node) {
        if (seen.emplace(
                node.steps,
                node.fuelUsed,
                node.cell,
                node.spotMask,
                node.roadFootprint).second) {
            nodes.push_back(node);
        }
    };
    for (std::size_t cursor = 0; cursor < nodes.size(); ++cursor) {
        const Node current = nodes.at(cursor);
        const udon::SpotIndex currentSpot =
            config.spotAtCell.at(static_cast<std::size_t>(current.cell));
        if (currentSpot != udon::kInvalidSpot && current.steps < limit &&
            (current.spotMask &
             (std::uint32_t{1} << static_cast<std::uint32_t>(currentSpot))) == 0U) {
            Node waited = current;
            ++waited.steps;
            if (config.map.terrain.at(static_cast<std::size_t>(waited.cell)) ==
                udon::Terrain::Road) {
                add_traffic_stays(
                    config,
                    waited.roadFootprint,
                    road_offset(config, waited.cell),
                    1);
            }
            waited.spotMask |=
                std::uint32_t{1} << static_cast<std::uint32_t>(currentSpot);
            waited.parent = static_cast<std::int32_t>(cursor);
            waited.incoming = udon::PlanAction::wait(1);
            push(waited);
        }
        const udon::MoveCost cost = config.move_cost(
            current.cell,
            roadStatuses.at(static_cast<std::size_t>(current.cell)));
        if (cost.steps <= 0 || current.steps + cost.steps > limit ||
            current.fuelUsed + cost.patrolFuel > availableFuel) {
            continue;
        }
        for (std::int32_t direction = 0; direction < udon::kDirectionCount; ++direction) {
            const udon::CellId destination = config.map.neighbors
                .at(static_cast<std::size_t>(current.cell))
                .at(static_cast<std::size_t>(direction));
            if (destination == udon::kInvalidCell ||
                config.map.terrain.at(static_cast<std::size_t>(destination)) ==
                    udon::Terrain::Pond) {
                continue;
            }
            Node moved = current;
            moved.steps += cost.steps;
            moved.fuelUsed += cost.patrolFuel;
            moved.cell = destination;
            if (config.map.terrain.at(static_cast<std::size_t>(current.cell)) ==
                udon::Terrain::Road) {
                add_traffic_stays(
                    config,
                    moved.roadFootprint,
                    road_offset(config, current.cell),
                    cost.steps - 1);
            }
            if (config.map.terrain.at(static_cast<std::size_t>(destination)) ==
                udon::Terrain::Road) {
                add_traffic_stays(
                    config,
                    moved.roadFootprint,
                    road_offset(config, destination),
                    1);
            }
            const udon::SpotIndex spot =
                config.spotAtCell.at(static_cast<std::size_t>(destination));
            if (spot != udon::kInvalidSpot) {
                moved.spotMask |=
                    std::uint32_t{1} << static_cast<std::uint32_t>(spot);
            }
            moved.parent = static_cast<std::int32_t>(cursor);
            moved.incoming = udon::PlanAction::move(direction);
            push(moved);
        }
    }

    using OutcomeKey = std::tuple<
        udon::CellId,
        std::int32_t,
        std::uint32_t,
        TrafficFootprint>;
    std::map<OutcomeKey, std::int32_t> compressed;
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const Node& node = nodes.at(index);
        TrafficFootprint finalFootprint = node.roadFootprint;
        if (config.map.terrain.at(static_cast<std::size_t>(node.cell)) ==
            udon::Terrain::Road) {
            add_traffic_stays(
                config,
                finalFootprint,
                road_offset(config, node.cell),
                limit - node.steps);
        }
        const OutcomeKey key{
            node.cell,
            availableFuel - node.fuelUsed,
            node.spotMask,
            std::move(finalFootprint)};
        const auto found = compressed.find(key);
        if (found == compressed.end() ||
            nodes.at(static_cast<std::size_t>(found->second)).steps > node.steps) {
            compressed[key] = static_cast<std::int32_t>(index);
        }
    }

    std::vector<DayOutcome> outcomes;
    for (const auto& [key, witness] : compressed) {
        const Node& terminal = nodes.at(static_cast<std::size_t>(witness));
        udon::AgentPlan reversed;
        std::int32_t current = witness;
        while (nodes.at(static_cast<std::size_t>(current)).parent >= 0) {
            reversed.push_back(nodes.at(static_cast<std::size_t>(current)).incoming);
            current = nodes.at(static_cast<std::size_t>(current)).parent;
        }
        std::reverse(reversed.begin(), reversed.end());
        if (terminal.steps < limit) {
            reversed.push_back(udon::PlanAction::wait(limit - terminal.steps));
        }
        outcomes.push_back(DayOutcome{
            std::get<0>(key),
            std::get<1>(key),
            std::get<2>(key),
            std::move(reversed),
            std::get<3>(key),
        });
    }

    std::vector<bool> dominated(outcomes.size(), false);
    for (std::size_t left = 0; left < outcomes.size(); ++left) {
        for (std::size_t right = 0; right < outcomes.size(); ++right) {
            if (left == right || outcomes.at(left).position != outcomes.at(right).position) {
                continue;
            }
            const DayOutcome& candidate = outcomes.at(left);
            const DayOutcome& other = outcomes.at(right);
            const bool spotSuperset =
                (other.spotMask | candidate.spotMask) == other.spotMask;
            const bool footprintNoWorse = std::equal(
                other.roadFootprint.begin(),
                other.roadFootprint.end(),
                candidate.roadFootprint.begin(),
                candidate.roadFootprint.end(),
                [](std::int32_t otherStays, std::int32_t candidateStays) {
                    return otherStays <= candidateStays;
                });
            if (spotSuperset && other.fuel >= candidate.fuel &&
                footprintNoWorse &&
                (other.spotMask != candidate.spotMask || other.fuel > candidate.fuel ||
                 other.roadFootprint != candidate.roadFootprint)) {
                dominated.at(left) = true;
                break;
            }
        }
    }
    std::vector<DayOutcome> frontier;
    for (std::size_t index = 0; index < outcomes.size(); ++index) {
        if (!dominated.at(index)) {
            frontier.push_back(std::move(outcomes.at(index)));
        }
    }
    return frontier;
}

[[nodiscard]] std::tuple<udon::CellId, std::int32_t, udon::CellId, std::int32_t, bool>
canonical_pair(const DayOutcome& first, const DayOutcome& second) {
    if (std::tie(second.position, second.fuel) < std::tie(first.position, first.fuel)) {
        return {second.position, second.fuel, first.position, first.fuel, true};
    }
    return {first.position, first.fuel, second.position, second.fuel, false};
}

[[nodiscard]] std::pair<std::uint64_t, std::int32_t> joint_day_score(
    const udon::MatchConfig& config,
    std::uint32_t firstMask,
    std::uint32_t secondMask) {
    std::uint64_t brands = 0U;
    std::int32_t servings = 0;
    for (std::size_t spotIndex = 0; spotIndex < config.spots.size(); ++spotIndex) {
        const std::uint32_t bit =
            std::uint32_t{1} << static_cast<std::uint32_t>(spotIndex);
        const std::int32_t claims =
            ((firstMask & bit) != 0U ? 1 : 0) +
            ((secondMask & bit) != 0U ? 1 : 0);
        if (claims == 0) {
            continue;
        }
        const udon::Spot& spot = config.spots.at(spotIndex);
        servings += std::min(claims, spot.stock);
        brands |= udon::brand_bit(spot.brandIndex);
    }
    return {brands, servings};
}

[[nodiscard]] bool accumulated_better(
    std::int32_t dailyDistinct,
    std::int32_t servings,
    const LayerEntry& incumbent) {
    return dailyDistinct > incumbent.totalDailyDistinct ||
        (dailyDistinct == incumbent.totalDailyDistinct &&
         servings > incumbent.totalServings);
}

[[nodiscard]] std::vector<LayerEntry> prune_layer(std::vector<LayerEntry> entries) {
    std::vector<bool> dominated(entries.size(), false);
    using PhysicalKey = std::tuple<
        udon::CellId,
        std::int32_t,
        udon::CellId,
        std::int32_t>;
    std::map<PhysicalKey, std::vector<std::size_t>> groups;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto& [p0, f0, p1, f1, lifetime, previousOwn, priorOwn] =
            entries.at(index).key;
        static_cast<void>(lifetime);
        static_cast<void>(previousOwn);
        static_cast<void>(priorOwn);
        groups[{p0, f0, p1, f1}].push_back(index);
    }
    for (const auto& [physical, indices] : groups) {
        static_cast<void>(physical);
        for (const std::size_t left : indices) {
            const std::uint64_t leftLifetime = std::get<4>(entries.at(left).key);
            for (const std::size_t right : indices) {
            if (left == right) {
                continue;
            }
            const std::uint64_t rightLifetime = std::get<4>(entries.at(right).key);
            const TrafficFootprint& leftPrevious =
                std::get<5>(entries.at(left).key);
            const TrafficFootprint& rightPrevious =
                std::get<5>(entries.at(right).key);
            const TrafficFootprint& leftPrior =
                std::get<6>(entries.at(left).key);
            const TrafficFootprint& rightPrior =
                std::get<6>(entries.at(right).key);
            const bool lifetimeSuperset =
                (rightLifetime | leftLifetime) == rightLifetime;
            const bool trafficNoWorse =
                std::equal(
                    rightPrevious.begin(),
                    rightPrevious.end(),
                    leftPrevious.begin(),
                    leftPrevious.end(),
                    std::less_equal<std::uint8_t>{}) &&
                std::equal(
                    rightPrior.begin(),
                    rightPrior.end(),
                    leftPrior.begin(),
                    leftPrior.end(),
                    std::less_equal<std::uint8_t>{});
            const bool scoreNoWorse =
                entries.at(right).totalDailyDistinct >= entries.at(left).totalDailyDistinct &&
                entries.at(right).totalServings >= entries.at(left).totalServings;
            const bool strict = rightLifetime != leftLifetime ||
                rightPrevious != leftPrevious || rightPrior != leftPrior ||
                entries.at(right).totalDailyDistinct > entries.at(left).totalDailyDistinct ||
                entries.at(right).totalServings > entries.at(left).totalServings;
            if (lifetimeSuperset && trafficNoWorse && scoreNoWorse && strict) {
                dominated.at(left) = true;
                break;
            }
        }
        }
    }
    std::vector<LayerEntry> frontier;
    frontier.reserve(entries.size());
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (!dominated.at(index)) {
            frontier.push_back(std::move(entries.at(index)));
        }
    }
    return frontier;
}

[[nodiscard]] OracleResult solve_oracle(const Fixture& fixture) {
    const auto initialFirst = std::pair{
        fixture.config.initialAgents.at(0),
        fixture.config.fuelLimit,
    };
    const auto initialSecond = std::pair{
        fixture.config.initialAgents.at(1),
        fixture.config.fuelLimit,
    };
    const bool initialSwap = initialSecond < initialFirst;
    const auto first = initialSwap ? initialSecond : initialFirst;
    const auto second = initialSwap ? initialFirst : initialSecond;

    std::vector<std::vector<LayerEntry>> layers;
    const TrafficFootprint emptyFootprint{};
    layers.push_back(std::vector<LayerEntry>{LayerEntry{
        MatchKey{
            first.first,
            first.second,
            second.first,
            second.second,
            0U,
            emptyFootprint,
            emptyFootprint},
    }});
    using DayCacheKey = std::tuple<
        std::int32_t,
        udon::CellId,
        std::int32_t,
        std::vector<udon::RoadStatus>>;
    std::map<DayCacheKey, std::vector<DayOutcome>> cache;
    OracleResult result;
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const std::vector<LayerEntry>& current = layers.back();
        std::vector<LayerEntry> next;
        std::map<MatchKey, std::size_t> retained;
        for (std::size_t parentIndex = 0; parentIndex < current.size(); ++parentIndex) {
            const LayerEntry& parent = current.at(parentIndex);
            const auto& [
                firstPosition,
                firstFuel,
                secondPosition,
                secondFuel,
                lifetime,
                previousOwn,
                priorOwn] = parent.key;
            const std::vector<udon::RoadStatus> roadStatuses = exact_road_statuses(
                fixture,
                day,
                previousOwn,
                priorOwn);
            const DayCacheKey firstCacheKey{
                day,
                firstPosition,
                firstFuel,
                roadStatuses};
            auto firstFound = cache.find(firstCacheKey);
            if (firstFound == cache.end()) {
                firstFound = cache.emplace(
                    firstCacheKey,
                    enumerate_day(
                        fixture.config,
                        day,
                        firstPosition,
                        firstFuel,
                        roadStatuses)).first;
                ++result.dailyEnumerations;
            }
            const DayCacheKey secondCacheKey{
                day,
                secondPosition,
                secondFuel,
                roadStatuses};
            auto secondFound = cache.find(secondCacheKey);
            if (secondFound == cache.end()) {
                secondFound = cache.emplace(
                    secondCacheKey,
                    enumerate_day(
                        fixture.config,
                        day,
                        secondPosition,
                        secondFuel,
                        roadStatuses)).first;
                ++result.dailyEnumerations;
            }
            for (const DayOutcome& firstOutcome : firstFound->second) {
                for (const DayOutcome& secondOutcome : secondFound->second) {
                    const auto [dailyBrands, dailyServings] = joint_day_score(
                        fixture.config,
                        firstOutcome.spotMask,
                        secondOutcome.spotMask);
                    const auto [nextP0, nextF0, nextP1, nextF1, swap] =
                        canonical_pair(firstOutcome, secondOutcome);
                    TrafficFootprint currentOwn{};
                    for (std::size_t roadIndex = 0;
                         roadIndex < fixture.config.roadCells.size();
                         ++roadIndex) {
                        add_traffic_stays(
                            fixture.config,
                            currentOwn,
                            roadIndex,
                            static_cast<std::int32_t>(
                                firstOutcome.roadFootprint.at(roadIndex)) +
                                static_cast<std::int32_t>(
                                    secondOutcome.roadFootprint.at(roadIndex)));
                    }
                    const MatchKey key{
                        nextP0,
                        nextF0,
                        nextP1,
                        nextF1,
                        lifetime | dailyBrands,
                        std::move(currentOwn),
                        previousOwn,
                    };
                    const std::int32_t nextDaily = parent.totalDailyDistinct +
                        static_cast<std::int32_t>(std::popcount(dailyBrands));
                    const std::int32_t nextServings =
                        parent.totalServings + dailyServings;
                    const auto found = retained.find(key);
                    if (found != retained.end() &&
                        !accumulated_better(nextDaily, nextServings, next.at(found->second))) {
                        continue;
                    }
                    LayerEntry candidate;
                    candidate.key = key;
                    candidate.totalDailyDistinct = nextDaily;
                    candidate.totalServings = nextServings;
                    candidate.parentIndex = static_cast<std::int32_t>(parentIndex);
                    candidate.firstPlan = firstOutcome.actions;
                    candidate.secondPlan = secondOutcome.actions;
                    candidate.swapForNextDay = swap;
                    if (found == retained.end()) {
                        retained.emplace(key, next.size());
                        next.push_back(std::move(candidate));
                    } else {
                        next.at(found->second) = std::move(candidate);
                    }
                }
            }
        }
        next = prune_layer(std::move(next));
        if (next.empty()) {
            return result;
        }
        result.maximumFrontier = std::max(result.maximumFrontier, next.size());
        layers.push_back(std::move(next));
    }

    const std::vector<LayerEntry>& finalLayer = layers.back();
    std::size_t bestIndex = 0;
    udon::OfficialScore bestScore{
        static_cast<std::int32_t>(std::popcount(std::get<4>(finalLayer.front().key))),
        finalLayer.front().totalDailyDistinct,
        finalLayer.front().totalServings,
    };
    for (std::size_t index = 1; index < finalLayer.size(); ++index) {
        const LayerEntry& candidate = finalLayer.at(index);
        const udon::OfficialScore score{
            static_cast<std::int32_t>(std::popcount(std::get<4>(candidate.key))),
            candidate.totalDailyDistinct,
            candidate.totalServings,
        };
        if (bestScore < score) {
            bestScore = score;
            bestIndex = index;
        }
    }

    struct AbstractDay {
        udon::AgentPlan first;
        udon::AgentPlan second;
        bool swap = false;
        MatchKey terminal{};
    };
    std::vector<AbstractDay> abstractDays(
        static_cast<std::size_t>(fixture.config.day_count()));
    std::size_t currentIndex = bestIndex;
    for (std::int32_t day = fixture.config.day_count(); day >= 1; --day) {
        const LayerEntry& entry = layers.at(static_cast<std::size_t>(day)).at(currentIndex);
        abstractDays.at(static_cast<std::size_t>(day - 1)) = AbstractDay{
            entry.firstPlan,
            entry.secondPlan,
            entry.swapForNextDay,
            entry.key,
        };
        currentIndex = static_cast<std::size_t>(entry.parentIndex);
    }

    std::vector<udon::AgentState> agents;
    for (const udon::CellId start : fixture.config.initialAgents) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            start,
            fixture.config.fuelLimit,
        });
    }
    std::array<std::size_t, 2> abstractToPhysical = initialSwap
        ? std::array<std::size_t, 2>{1U, 0U}
        : std::array<std::size_t, 2>{0U, 1U};
    udon::MatchLedger ledger;
    TrafficFootprint previousOwn = emptyFootprint;
    TrafficFootprint priorOwn = emptyFootprint;
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const AbstractDay& abstract = abstractDays.at(static_cast<std::size_t>(day - 1));
        udon::DayPlan plan;
        plan.actions.resize(static_cast<std::size_t>(fixture.config.agent_count()));
        plan.actions.at(abstractToPhysical.at(0)) = abstract.first;
        plan.actions.at(abstractToPhysical.at(1)) = abstract.second;
        plan.actions.at(2) = udon::AgentPlan{
            udon::PlanAction::wait(fixture.config.steps_for_day(day)),
        };
        const std::vector<udon::RoadStatus> roadStatuses = exact_road_statuses(
            fixture,
            day,
            previousOwn,
            priorOwn);
        const udon::DayState state = day_state(
            fixture.config,
            day,
            agents,
            roadStatuses);
        udon::SimulationResult simulation;
        std::string mismatch;
        if (!validates(fixture.config, state, plan, simulation, mismatch)) {
            throw std::runtime_error("oracle witness validation failed: " + mismatch);
        }
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
        if (abstract.swap) {
            std::swap(abstractToPhysical.at(0), abstractToPhysical.at(1));
        }
        const auto& [
            expectedP0,
            expectedF0,
            expectedP1,
            expectedF1,
            expectedLifetime,
            expectedPreviousOwn,
            expectedPriorOwn] = abstract.terminal;
        if (agents.at(abstractToPhysical.at(0)).position != expectedP0 ||
            agents.at(abstractToPhysical.at(0)).fuel != expectedF0 ||
            agents.at(abstractToPhysical.at(1)).position != expectedP1 ||
            agents.at(abstractToPhysical.at(1)).fuel != expectedF1 ||
            ledger.lifetimeBrands != expectedLifetime ||
            compact_road_footprint(fixture.config, simulation.roadFootprint) !=
                expectedPreviousOwn ||
            previousOwn != expectedPriorOwn) {
            throw std::runtime_error("oracle witness disagrees with canonical DP state");
        }
        priorOwn = previousOwn;
        previousOwn = compact_road_footprint(
            fixture.config,
            simulation.roadFootprint);
        result.plans.push_back(std::move(plan));
        result.cumulative.push_back(udon::OfficialScore{
            ledger.lifetime_distinct(),
            ledger.totalDailyDistinct,
            ledger.totalServings,
        });
        result.terminalAgents.push_back(agents);
    }
    result.score = udon::OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
    if (!(result.score == bestScore)) {
        throw std::runtime_error("oracle witness score disagrees with complete DP");
    }
    result.valid = true;
    return result;
}

[[nodiscard]] HeadResult solve_head(const Fixture& fixture) {
    udon::UdonShieldEngine engine(
        fixture.config,
        {},
        {},
        udon::RoutePoolSearch::SinglePass,
        kHarvestMode,
        false,
        kFutureHarvestMode);
    const udon::ExactStepSimulator cacheSimulator(fixture.config);
    const udon::IndependentDayValidator cacheValidator(fixture.config);
    const udon::RouteMaster cacheMaster(
        fixture.config,
        cacheSimulator,
        cacheValidator);
    const udon::FastViabilityAnalyzer cacheViability(fixture.config);
    std::vector<udon::AgentState> agents;
    for (const udon::CellId start : fixture.config.initialAgents) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            start,
            fixture.config.fuelLimit,
        });
    }
    udon::MatchLedger ledger;
    TrafficFootprint previousOwn{};
    TrafficFootprint priorOwn{};
    HeadResult result;
    std::int32_t sourceMultiDayWitnesses = 0;
    std::int32_t sourceMaximumPlans = 0;
    udon::OfficialScore sourceFinal;
    std::optional<udon::FutureWitness> sourceWitness;
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const std::vector<udon::RoadStatus> roadStatuses = exact_road_statuses(
            fixture,
            day,
            previousOwn,
            priorOwn);
        const udon::DayState state = day_state(
            fixture.config,
            day,
            agents,
            roadStatuses);
        bool suffixAttempted = false;
        bool suffixValid = false;
        bool suffixScoreMatches = false;
        udon::OfficialScore suffixReplayScore;
        if (sourceWitness.has_value() &&
            sourceWitness->futurePlans.size() > 1U &&
            fixture.config.roadCells.empty()) {
            suffixAttempted = true;
            suffixValid = true;
            udon::DayState suffixState = state;
            udon::MatchLedger suffixLedger = ledger;
            for (std::size_t planOffset = 0;
                 planOffset < sourceWitness->futurePlans.size();
                 ++planOffset) {
                udon::SimulationResult suffixSimulation;
                std::string suffixMismatch;
                if (!validates(
                        fixture.config,
                        suffixState,
                        sourceWitness->futurePlans.at(planOffset),
                        suffixSimulation,
                        suffixMismatch)) {
                    suffixValid = false;
                    break;
                }
                suffixLedger.apply(suffixSimulation.score);
                if (planOffset + 1U < sourceWitness->futurePlans.size()) {
                    suffixState = day_state(
                        fixture.config,
                        suffixState.dayNumber + 1,
                        suffixSimulation.finalAgents);
                }
            }
            suffixReplayScore = udon::OfficialScore{
                suffixLedger.lifetime_distinct(),
                suffixLedger.totalDailyDistinct,
                suffixLedger.totalServings,
            };
            suffixScoreMatches = suffixValid &&
                suffixReplayScore == sourceWitness->score;
        }
        std::set<std::string> cachedPlanIds;
        bool cachedValid = false;
        udon::OfficialScore cachedCurrent;
        udon::OfficialScore cachedUpper;
        for (const udon::ResponseLedger::CachedContingency& contingency :
             engine.response_ledger().cachedContingencies) {
            if (contingency.dayNumber == day) {
                cachedPlanIds.insert(
                    udon::serialize_day_plan(contingency.plan).dump());
                const std::optional<udon::MasterCandidate> exact =
                    cacheMaster.evaluate_exact_plan(
                        state,
                        ledger,
                        contingency.plan);
                if (!exact.has_value()) {
                    continue;
                }
                udon::OfficialScore upper = exact->scoreAfterToday;
                if (day < fixture.config.day_count()) {
                    udon::MatchLedger futureLedger = ledger;
                    futureLedger.apply(exact->simulation.score);
                    upper = cacheViability.analyze(
                        day_state(
                            fixture.config,
                            day + 1,
                            exact->simulation.finalAgents),
                        futureLedger).upperBound;
                }
                if (!cachedValid || cachedUpper < upper ||
                    (cachedUpper == upper &&
                     cachedCurrent < exact->scoreAfterToday)) {
                    cachedValid = true;
                    cachedCurrent = exact->scoreAfterToday;
                    cachedUpper = upper;
                }
            }
        }
        const auto started = std::chrono::steady_clock::now();
        const udon::DecisionResult decision =
            engine.solve_day(state, ledger, kProductionBudget);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started);
        udon::SimulationResult simulation;
        std::string mismatch;
        if (!validates(
                fixture.config,
                state,
                decision.candidate.plan,
                simulation,
                mismatch)) {
            return result;
        }
        engine.record_submitted(decision, elapsed);
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
        result.plans.push_back(decision.candidate.plan);
        result.cumulative.push_back(udon::OfficialScore{
            ledger.lifetime_distinct(),
            ledger.totalDailyDistinct,
            ledger.totalServings,
        });
        result.terminalAgents.push_back(agents);
        result.audits.push_back(decision.audit);
        HeadResult::CachePathAudit cachePath;
        cachePath.cached = static_cast<std::int32_t>(cachedPlanIds.size());
        cachePath.eligible = decision.cacheRepair.eligibleContingencies;
        cachePath.reused = decision.cacheRepair.reusedContingencies;
        cachePath.rejected = decision.cacheRepair.rejectedContingencies;
        cachePath.retained = static_cast<std::int32_t>(std::count_if(
            decision.audit.candidates.begin(),
            decision.audit.candidates.end(),
            [&cachedPlanIds](const udon::CandidateAuditRecord& record) {
                return cachedPlanIds.contains(record.stableId);
            }));
        cachePath.selected = cachedPlanIds.contains(
            decision.candidate.stableId);
        cachePath.cachedValid = cachedValid;
        cachePath.cachedCurrent = cachedCurrent;
        cachePath.cachedUpper = cachedUpper;
        const auto selectedAudit = std::find_if(
            decision.audit.candidates.begin(),
            decision.audit.candidates.end(),
            [](const udon::CandidateAuditRecord& record) {
                return record.selected;
            });
        if (selectedAudit != decision.audit.candidates.end()) {
            cachePath.selectedCurrent = selectedAudit->scoreAfterToday;
            cachePath.selectedUpper = selectedAudit->validUpperBound;
        }
        cachePath.sourceMultiDayWitnesses = sourceMultiDayWitnesses;
        cachePath.sourceMaximumPlans = sourceMaximumPlans;
        cachePath.sourceFinal = sourceFinal;
        cachePath.suffixAttempted = suffixAttempted;
        cachePath.suffixValid = suffixValid;
        cachePath.suffixScoreMatches = suffixScoreMatches;
        cachePath.suffixReplayScore = suffixReplayScore;
        result.cachePaths.push_back(cachePath);
        sourceMultiDayWitnesses = 0;
        sourceMaximumPlans = 0;
        sourceFinal = {};
        sourceWitness.reset();
        for (const udon::ScenarioOutcome& outcome : decision.profile.outcomes) {
            if (!outcome.witness.certified ||
                outcome.witness.futurePlans.size() <= 1U) {
                continue;
            }
            ++sourceMultiDayWitnesses;
            sourceMaximumPlans = std::max(
                sourceMaximumPlans,
                static_cast<std::int32_t>(
                    outcome.witness.futurePlans.size()));
            sourceFinal = std::max(sourceFinal, outcome.score);
            if (!sourceWitness.has_value() ||
                sourceWitness->score < outcome.score) {
                sourceWitness = outcome.witness;
            }
        }
        priorOwn = previousOwn;
        previousOwn = compact_road_footprint(
            fixture.config,
            simulation.roadFootprint);
    }
    result.score = udon::OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
    result.valid = true;
    return result;
}

[[nodiscard]] std::int32_t first_tier(
    const udon::OfficialScore& oracle,
    const udon::OfficialScore& head) {
    if (oracle.lifetimeDistinct != head.lifetimeDistinct) {
        return 1;
    }
    if (oracle.totalDailyDistinct != head.totalDailyDistinct) {
        return 2;
    }
    if (oracle.totalServings != head.totalServings) {
        return 3;
    }
    return 0;
}

[[nodiscard]] std::int32_t tier_gain(
    const udon::OfficialScore& oracle,
    const udon::OfficialScore& head,
    std::int32_t tier) {
    if (tier == 1) {
        return oracle.lifetimeDistinct - head.lifetimeDistinct;
    }
    if (tier == 2) {
        return oracle.totalDailyDistinct - head.totalDailyDistinct;
    }
    if (tier == 3) {
        return oracle.totalServings - head.totalServings;
    }
    return 0;
}

void record_summary(
    Summary& summary,
    const OracleResult& oracle,
    const HeadResult& head,
    std::uint64_t seed) {
    ++summary.cases;
    if (!oracle.valid || !head.valid) {
        ++summary.invalid;
        return;
    }
    if (head.score < oracle.score) {
        ++summary.oracleWins;
    } else if (oracle.score < head.score) {
        ++summary.headWins;
    } else {
        ++summary.ties;
    }
    const std::int32_t tier = first_tier(oracle.score, head.score);
    if (tier == 1) {
        ++summary.tier1;
    } else if (tier == 2) {
        ++summary.tier2;
    } else if (tier == 3) {
        ++summary.tier3;
    }
    summary.maximumGain = std::max(summary.maximumGain, tier_gain(oracle.score, head.score, tier));
    for (const std::int32_t value : {
             static_cast<std::int32_t>(seed & 0x7fffffffU),
             oracle.score.lifetimeDistinct,
             oracle.score.totalDailyDistinct,
             oracle.score.totalServings,
             head.score.lifetimeDistinct,
             head.score.totalDailyDistinct,
             head.score.totalServings,
         }) {
        hash_value(summary.resultHash, static_cast<std::uint64_t>(value));
    }
}

[[nodiscard]] std::string score_text(const udon::OfficialScore& score) {
    return std::to_string(score.lifetimeDistinct) + '/' +
        std::to_string(score.totalDailyDistinct) + '/' +
        std::to_string(score.totalServings);
}

[[nodiscard]] std::string agents_text(const std::vector<udon::AgentState>& agents) {
    std::ostringstream output;
    for (std::size_t index = 0; index < agents.size(); ++index) {
        if (index != 0U) {
            output << '|';
        }
        output << agents.at(index).position << '@' << agents.at(index).fuel;
    }
    return output.str();
}

[[nodiscard]] std::string plan_text(const udon::DayPlan& plan) {
    std::ostringstream output;
    for (std::size_t agent = 0; agent < plan.actions.size(); ++agent) {
        if (agent != 0U) {
            output << '|';
        }
        for (std::size_t action = 0; action < plan.actions.at(agent).size(); ++action) {
            if (action != 0U) {
                output << '.';
            }
            output << plan.actions.at(agent).at(action).wire_value();
        }
    }
    return output.str();
}

[[nodiscard]] std::uint64_t plan_sequence_hash(
    const std::vector<udon::DayPlan>& plans) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const udon::DayPlan& plan : plans) {
        for (const udon::AgentPlan& actions : plan.actions) {
            for (const udon::PlanAction& action : actions) {
                hash_value(
                    hash,
                    static_cast<std::uint64_t>(action.wire_value()));
            }
            hash_value(hash, std::numeric_limits<std::uint64_t>::max());
        }
        hash_value(hash, std::numeric_limits<std::uint64_t>::max() - 1U);
    }
    return hash;
}

[[nodiscard]] bool same_agent_plan(
    const udon::AgentPlan& left,
    const udon::AgentPlan& right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t action = 0U; action < left.size(); ++action) {
        if (left.at(action).wire_value() != right.at(action).wire_value()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::string portfolio_plan_mask(
    const udon::RoutePortfolio& portfolio,
    const udon::DayPlan& plan) {
    std::string mask;
    for (std::size_t agent = 0U; agent < plan.actions.size(); ++agent) {
        const bool present = agent < portfolio.columnsByAgent.size() &&
            std::any_of(
                portfolio.columnsByAgent.at(agent).begin(),
                portfolio.columnsByAgent.at(agent).end(),
                [&plan, agent](const udon::RouteColumn& column) {
                    return same_agent_plan(column.actions, plan.actions.at(agent));
                });
        mask += present ? '1' : '0';
    }
    return mask;
}

[[nodiscard]] bool contains_candidate(
    const std::vector<udon::MasterCandidate>& candidates,
    const udon::MasterCandidate& exact) {
    return std::any_of(
        candidates.begin(),
        candidates.end(),
        [&exact](const udon::MasterCandidate& candidate) {
            return candidate.stableId == exact.stableId;
        });
}

[[nodiscard]] bool contains_outcome(
    const std::vector<udon::MasterCandidate>& candidates,
    const udon::MasterCandidate& exact) {
    return std::any_of(
        candidates.begin(),
        candidates.end(),
        [&exact](const udon::MasterCandidate& candidate) {
            if (!(candidate.scoreAfterToday == exact.scoreAfterToday) ||
                candidate.simulation.finalAgents.size() !=
                    exact.simulation.finalAgents.size()) {
                return false;
            }
            for (std::size_t agent = 0U;
                 agent < candidate.simulation.finalAgents.size();
                 ++agent) {
                const udon::AgentState& left =
                    candidate.simulation.finalAgents.at(agent);
                const udon::AgentState& right =
                    exact.simulation.finalAgents.at(agent);
                if (left.kind != right.kind || left.position != right.position ||
                    left.fuel != right.fuel) {
                    return false;
                }
            }
            return true;
        });
}

[[nodiscard]] std::string best_candidate_score(
    const std::vector<udon::MasterCandidate>& candidates) {
    if (candidates.empty()) {
        return "none";
    }
    const udon::MasterCandidate* best = &candidates.front();
    for (const udon::MasterCandidate& candidate : candidates) {
        if (best->scoreAfterToday < candidate.scoreAfterToday) {
            best = &candidate;
        }
    }
    return score_text(best->scoreAfterToday);
}

[[nodiscard]] std::string slack_text(const udon::TerminalSlack& slack) {
    return std::to_string(slack.worstRemainingBrandSteps) + '/' +
        std::to_string(slack.totalRemainingBrandSteps) + '/' +
        std::to_string(slack.patrolFuelReserve) + '/' +
        std::to_string(slack.overnightSpotCount);
}

[[nodiscard]] bool profile_certified(const udon::CandidateProfile& profile) {
    return !profile.outcomes.empty() && std::all_of(
        profile.outcomes.begin(),
        profile.outcomes.end(),
        [](const udon::ScenarioOutcome& outcome) {
            return outcome.witness.certified;
        });
}

[[nodiscard]] std::string profile_support_text(
    const udon::CandidateProfile& profile) {
    std::ostringstream output;
    for (std::size_t index = 0; index < profile.outcomes.size(); ++index) {
        if (index != 0U) {
            output << '|';
        }
        output << score_text(profile.outcomes.at(index).score) << '@'
               << score_text(profile.scenarioValidUpperBounds.at(index)) << ':'
               << (profile.outcomes.at(index).witness.certified ? 1 : 0);
    }
    return output.str();
}

[[nodiscard]] std::string dominance_failure_text(
    const udon::CandidateProfile& challenger,
    const udon::CandidateProfile& incumbent) {
    if (!profile_certified(challenger) || !incumbent.hasValidUpperBound ||
        challenger.outcomes.size() != incumbent.outcomes.size() ||
        challenger.scenarioWeights != incumbent.scenarioWeights ||
        incumbent.scenarioValidUpperBounds.size() != incumbent.outcomes.size()) {
        return "profile-precondition";
    }
    std::vector<udon::OfficialScore> support;
    for (const udon::ScenarioOutcome& outcome : challenger.outcomes) {
        support.push_back(outcome.score);
    }
    support.insert(
        support.end(),
        incumbent.scenarioValidUpperBounds.begin(),
        incumbent.scenarioValidUpperBounds.end());
    std::sort(support.begin(), support.end());
    support.erase(std::unique(support.begin(), support.end()), support.end());
    bool strict = false;
    for (const udon::OfficialScore& threshold : support) {
        std::uint64_t challengerWeight = 0;
        std::uint64_t incumbentPossibleWeight = 0;
        for (std::size_t scenario = 0; scenario < challenger.outcomes.size(); ++scenario) {
            if (!(challenger.outcomes.at(scenario).score < threshold)) {
                challengerWeight += challenger.scenarioWeights.at(scenario);
            }
            if (!(incumbent.scenarioValidUpperBounds.at(scenario) < threshold)) {
                incumbentPossibleWeight += incumbent.scenarioWeights.at(scenario);
            }
        }
        if (challengerWeight < incumbentPossibleWeight) {
            return score_text(threshold) + ':' +
                std::to_string(challengerWeight) + '<' +
                std::to_string(incumbentPossibleWeight);
        }
        strict = strict || challengerWeight > incumbentPossibleWeight;
    }
    return strict ? "none" : "no-strict-threshold";
}

[[nodiscard]] const udon::MasterCandidate* best_same_prefix_slack(
    const std::vector<udon::MasterCandidate>& candidates,
    const udon::MasterCandidate& exact) {
    const udon::MasterCandidate* best = nullptr;
    for (const udon::MasterCandidate& candidate : candidates) {
        if (candidate.scoreAfterToday.lifetimeDistinct !=
                exact.scoreAfterToday.lifetimeDistinct ||
            candidate.scoreAfterToday.totalDailyDistinct !=
                exact.scoreAfterToday.totalDailyDistinct) {
            continue;
        }
        if (best == nullptr ||
            udon::compare_terminal_slack(
                candidate.terminalSlack,
                best->terminalSlack) > 0) {
            best = &candidate;
        }
    }
    return best;
}

void attribute_oracle_day(
    const Fixture& fixture,
    std::int32_t day,
    const udon::DayState& state,
    const udon::MatchLedger& ledger,
    const udon::DayPlan& oraclePlan,
    const udon::DayPlan& parentPlan) {
    const udon::ExactStepSimulator simulator(fixture.config);
    const udon::IndependentDayValidator validator(fixture.config);
    const udon::ParetoRouter router(fixture.config);
    const udon::RouteColumnGenerator generator(fixture.config, router);
    const udon::RouteMaster master(fixture.config, simulator, validator);
    const udon::GreedyPlanner greedy(fixture.config, generator, master);
    const udon::FastViabilityAnalyzer viabilityAnalyzer(fixture.config);

    const std::optional<udon::MasterCandidate> exact = master.evaluate_exact_plan(
        state,
        ledger,
        oraclePlan);
    if (!exact.has_value()) {
        std::cout << "attribute,seed=" << fixture.seed
                  << ",day=" << day
                  << ",status=oracle-plan-not-dual-valid\n";
        return;
    }
    const std::optional<udon::MasterCandidate> parent =
        master.evaluate_exact_plan(state, ledger, parentPlan);

    udon::ColumnGenerationOptions seedGeneration;
    seedGeneration.maximumPathsPerTarget = 1;
    seedGeneration.maximumColumnsPerAgent = 2;
    seedGeneration.maximumTargetSpots = 4;
    seedGeneration.maximumEscorts = 2;
    udon::MasterOptions seedMaster;
    seedMaster.maximumCombinations = 512;
    seedMaster.maximumCandidates = 1;
    udon::MasterDiagnostics seedDiagnostics;
    const udon::MasterCandidate incumbent = greedy.build_incumbent(
        state,
        ledger,
        seedGeneration,
        seedMaster,
        seedDiagnostics);

    const udon::ViabilityBounds viability = viabilityAnalyzer.analyze(state, ledger);
    udon::ColumnGenerationOptions generation;
    generation.maximumPathsPerTarget = 4;
    generation.maximumColumnsPerAgent = 16;
    generation.maximumTargetSpots = 12;
    generation.maximumEscorts = 16;
    generation.maximumSeedPlans = 2;
    generation.enableHarvestExtensions = true;
    generation.allowUncachedHarvestTargets = true;
    const bool exactFuelOrienteering =
        static_cast<std::int64_t>(fixture.config.fuelLimit) >=
        2LL * fixture.config.steps_for_day(day);
    const bool hasFuelConstrainedPatrol = std::any_of(
        state.agents.begin(),
        state.agents.end(),
        [&fixture, day](const udon::AgentState& agent) {
            return agent.kind == udon::AgentKind::Patrol &&
                static_cast<std::int64_t>(agent.fuel) <
                    2LL * fixture.config.steps_for_day(day);
        });
    generation.enableExactHarvestOrienteering =
        (exactFuelOrienteering || day == fixture.config.day_count());
    generation.enableFuelConstrainedExactHarvestOrienteering =
        hasFuelConstrainedPatrol && day == fixture.config.day_count();
    generation.enableAnytimeFuelConstrainedHarvestOrienteering =
        generation.enableFuelConstrainedExactHarvestOrienteering;
    generation.maximumHarvestExtensionSources = 4;
    generation.maximumHarvestExtensionDepth = 3;
    generation.mandatoryReservations = viability.reservations;
    generation.seedPlans.push_back(incumbent.plan);

    udon::ColumnGenerationOptions legacyGeneration = generation;
    legacyGeneration.maximumColumnsPerAgent = 12;
    legacyGeneration.allowUncachedHarvestTargets = false;
    udon::RoutePortfolio merged = generator.generate(
        state,
        ledger,
        legacyGeneration);
    const udon::RoutePortfolio legacy = merged;
    const udon::RoutePortfolio expanded = generator.generate(
        state,
        ledger,
        generation);

    std::int32_t nextColumnId = 0;
    for (const std::vector<udon::RouteColumn>& columns : merged.columnsByAgent) {
        for (const udon::RouteColumn& column : columns) {
            nextColumnId = std::max(nextColumnId, column.columnId + 1);
        }
    }
    for (std::size_t agent = 0U; agent < merged.columnsByAgent.size(); ++agent) {
        std::vector<udon::RouteColumn>& retained = merged.columnsByAgent.at(agent);
        for (const udon::RouteColumn& source : expanded.columnsByAgent.at(agent)) {
            if (!source.harvestExtension ||
                std::any_of(
                    retained.begin(),
                    retained.end(),
                    [&source](const udon::RouteColumn& existing) {
                        return same_agent_plan(existing.actions, source.actions);
                    })) {
                continue;
            }
            udon::RouteColumn added = source;
            added.columnId = nextColumnId++;
            retained.push_back(std::move(added));
        }
    }

    udon::ColumnGenerationOptions wideGeneration = generation;
    wideGeneration.maximumColumnsPerAgent = 32;
    const udon::RoutePortfolio wide32 = generator.generate(
        state,
        ledger,
        wideGeneration);
    wideGeneration.maximumColumnsPerAgent = 64;
    const udon::RoutePortfolio wide64 = generator.generate(
        state,
        ledger,
        wideGeneration);

    udon::MasterOptions masterOptions;
    masterOptions.maximumCombinations = 40000;
    masterOptions.maximumCandidates = 32;
    masterOptions.diversityCandidates = 8;
    masterOptions.preferBaselineHarvestSources = true;
    masterOptions.mandatoryReservations = viability.reservations;
    udon::MasterDiagnostics mergedDiagnostics;
    const std::vector<udon::MasterCandidate> mergedCandidates = master.solve(
        state,
        ledger,
        merged,
        masterOptions,
        mergedDiagnostics);

    udon::RoutePortfolio forcedBundle = merged;
    bool completeForcedBundle = true;
    constexpr std::int32_t forcedBundleId = 1000000000;
    for (std::size_t agent = 0U; agent < exact->plan.actions.size(); ++agent) {
        const auto found = std::find_if(
            forcedBundle.columnsByAgent.at(agent).begin(),
            forcedBundle.columnsByAgent.at(agent).end(),
            [&exact, agent](const udon::RouteColumn& column) {
                return same_agent_plan(
                    column.actions,
                    exact->plan.actions.at(agent));
            });
        if (found == forcedBundle.columnsByAgent.at(agent).end()) {
            completeForcedBundle = false;
            break;
        }
        found->contingencyBundle = forcedBundleId;
    }
    std::vector<udon::MasterCandidate> forcedCandidates;
    if (completeForcedBundle) {
        udon::MasterDiagnostics forcedDiagnostics;
        forcedCandidates = master.solve(
            state,
            ledger,
            forcedBundle,
            masterOptions,
            forcedDiagnostics);
    }

    udon::MasterOptions wideMasterOptions = masterOptions;
    wideMasterOptions.maximumCandidates = 256;
    wideMasterOptions.diversityCandidates = 64;
    wideMasterOptions.enableLexicographicBranchAndBound = false;
    udon::MasterDiagnostics wideMasterDiagnostics;
    const std::vector<udon::MasterCandidate> wideMasterCandidates = master.solve(
        state,
        ledger,
        merged,
        wideMasterOptions,
        wideMasterDiagnostics);

    const std::int32_t augmentationCap = static_cast<std::int32_t>(
        std::max({
            merged.columnsByAgent.at(0).size(),
            merged.columnsByAgent.at(1).size(),
            merged.columnsByAgent.at(2).size(),
        }) + 4U);
    const udon::RoutePoolAugmentation augmented = generator.augment_with_candidate_routes(
        state,
        merged,
        std::vector<udon::MasterCandidate>{*exact},
        augmentationCap);
    udon::MasterDiagnostics augmentedDiagnostics;
    const std::vector<udon::MasterCandidate> augmentedCandidates = master.solve(
        state,
        ledger,
        augmented.portfolio,
        masterOptions,
        augmentedDiagnostics);
    udon::MasterDiagnostics wideAugmentedDiagnostics;
    const std::vector<udon::MasterCandidate> wideAugmentedCandidates = master.solve(
        state,
        ledger,
        augmented.portfolio,
        wideMasterOptions,
        wideAugmentedDiagnostics);
    const auto candidateUpper = [
                                    &fixture,
                                    day,
                                    &ledger,
                                    &viabilityAnalyzer](
                                    const udon::MasterCandidate& candidate) {
        if (day == fixture.config.day_count()) {
            return candidate.scoreAfterToday;
        }
        udon::MatchLedger futureLedger = ledger;
        futureLedger.apply(candidate.simulation.score);
        const udon::DayState futureState = day_state(
            fixture.config,
            day + 1,
            candidate.simulation.finalAgents);
        return viabilityAnalyzer.analyze(futureState, futureLedger).upperBound;
    };
    const udon::OfficialScore exactUpper = candidateUpper(*exact);
    if (day == 1 && parent.has_value()) {
        udon::TrafficBelief belief(fixture.config);
        belief.observe(state);
        const udon::ScenarioGenerator scenarios(fixture.config);
        udon::ScenarioManifest manifest = scenarios.freeze_manifest(state, belief);
        const udon::RiskPolicy policy;
        manifest.generatorSeed = 0;
        manifest.evaluatorHash = evaluator_contract_hash();
        manifest.riskPolicyVersion = policy.version;
        manifest.confidenceBasisPoints = policy.confidenceBasisPoints;
        manifest.safetySlack = policy.safetySlack;
        manifest.resolutionBasisPoints = policy.resolutionBasisPoints;
        manifest.quantileBasisPoints = policy.quantileBasisPoints;
        const udon::FutureWitnessRepairer repairer(
            fixture.config,
            generator,
            master,
            simulator,
            validator,
            kFutureHarvestMode);
        const auto build_profile = [&](const udon::MasterCandidate& candidate) {
            const std::chrono::steady_clock::time_point deadline =
                std::chrono::steady_clock::now() + kProductionBudget;
            udon::CandidateProfile profile = repairer.provisional_profile(
                candidate,
                state,
                ledger,
                belief,
                manifest,
                candidateUpper(candidate),
                24,
                deadline);
            repairer.repair_profile(
                profile,
                candidate,
                state,
                ledger,
                belief,
                manifest,
                200,
                deadline);
            std::vector<udon::CandidateEvaluation> evaluations;
            evaluations.push_back(udon::CandidateEvaluation{candidate, std::move(profile)});
            udon::LexicographicRiskComparator comparator(policy);
            comparator.finalize_profiles(evaluations, manifest);
            return std::move(evaluations.front().profile);
        };
        const udon::CandidateProfile exactProfile = build_profile(*exact);
        const udon::CandidateProfile parentProfile = build_profile(*parent);
        const udon::LexicographicRiskComparator comparator(policy);
        std::cout << "profile_attribute,seed=" << fixture.seed
                  << ",day=" << day
                  << ",exact_current=" << score_text(exact->scoreAfterToday)
                  << ",parent_current=" << score_text(parent->scoreAfterToday)
                  << ",exact_lower=" << score_text(exactProfile.certifiedLowerBound)
                  << ",parent_lower=" << score_text(parentProfile.certifiedLowerBound)
                  << ",exact_upper=" << score_text(exactProfile.validUpperBound)
                  << ",parent_upper=" << score_text(parentProfile.validUpperBound)
                  << ",exact_certified=" << profile_certified(exactProfile)
                  << ",parent_certified=" << profile_certified(parentProfile)
                  << ",exact_dominates_parent="
                  << comparator.certified_dominates(exactProfile, parentProfile)
                  << ",parent_dominates_exact="
                  << comparator.certified_dominates(parentProfile, exactProfile)
                  << ",exact_failure="
                  << dominance_failure_text(exactProfile, parentProfile)
                  << ",parent_failure="
                  << dominance_failure_text(parentProfile, exactProfile)
                  << ",exact_support=" << profile_support_text(exactProfile)
                  << ",parent_support=" << profile_support_text(parentProfile)
                  << '\n';
    }
    std::int32_t upperRank = 1;
    for (const udon::MasterCandidate& candidate : wideAugmentedCandidates) {
        if (exactUpper < candidateUpper(candidate)) {
            ++upperRank;
        }
    }
    std::int32_t firstMasterCap = -1;
    for (const std::int32_t cap : {32, 48, 64, 96, 128, 192, 256}) {
        udon::MasterOptions capOptions = masterOptions;
        capOptions.maximumCandidates = cap;
        capOptions.diversityCandidates = std::max(1, cap / 4);
        udon::MasterDiagnostics capDiagnostics;
        const std::vector<udon::MasterCandidate> capCandidates = master.solve(
            state,
            ledger,
            augmented.portfolio,
            capOptions,
            capDiagnostics);
        if (contains_outcome(capCandidates, *exact)) {
            firstMasterCap = cap;
            break;
        }
    }
    std::string diversityRetention;
    for (const std::int32_t slots : {0, 8, 16, 24, 31}) {
        udon::MasterOptions diversityOptions = masterOptions;
        diversityOptions.diversityCandidates = slots;
        udon::MasterDiagnostics diversityDiagnostics;
        const std::vector<udon::MasterCandidate> diversityCandidates = master.solve(
            state,
            ledger,
            augmented.portfolio,
            diversityOptions,
            diversityDiagnostics);
        if (!diversityRetention.empty()) {
            diversityRetention += '/';
        }
        diversityRetention += std::to_string(slots) + ':' +
            (contains_outcome(diversityCandidates, *exact) ? '1' : '0');
    }
    std::string unboundedRetention;
    for (const std::int32_t slots : {0, 8, 16, 24, 31}) {
        udon::MasterOptions explorationOptions = masterOptions;
        explorationOptions.diversityCandidates = slots;
        explorationOptions.enableLexicographicBranchAndBound = false;
        udon::MasterDiagnostics explorationDiagnostics;
        const std::vector<udon::MasterCandidate> explorationCandidates = master.solve(
            state,
            ledger,
            augmented.portfolio,
            explorationOptions,
            explorationDiagnostics);
        if (!unboundedRetention.empty()) {
            unboundedRetention += '/';
        }
        unboundedRetention += std::to_string(slots) + ':' +
            (contains_outcome(explorationCandidates, *exact) ? '1' : '0');
    }
    const udon::MasterCandidate* prefixSlack = best_same_prefix_slack(
        wideAugmentedCandidates,
        *exact);

    std::cout << "attribute,seed=" << fixture.seed
              << ",day=" << day
              << ",exact=" << score_text(exact->scoreAfterToday)
              << ",legacy12_mask=" << portfolio_plan_mask(legacy, exact->plan)
              << ",expanded16_mask=" << portfolio_plan_mask(expanded, exact->plan)
              << ",merged_mask=" << portfolio_plan_mask(merged, exact->plan)
              << ",wide32_mask=" << portfolio_plan_mask(wide32, exact->plan)
              << ",wide64_mask=" << portfolio_plan_mask(wide64, exact->plan)
              << ",merged_master_exact=" << contains_candidate(mergedCandidates, *exact)
              << ",merged_master_outcome=" << contains_outcome(mergedCandidates, *exact)
              << ",merged_best=" << best_candidate_score(mergedCandidates)
              << ",forced_bundle=" << completeForcedBundle
              << ",forced_exact=" << contains_candidate(forcedCandidates, *exact)
              << ",wide_master_exact=" << contains_candidate(wideMasterCandidates, *exact)
              << ",wide_master_outcome=" << contains_outcome(wideMasterCandidates, *exact)
              << ",wide_master_count=" << wideMasterCandidates.size()
              << ",exact_slack=" << slack_text(exact->terminalSlack)
              << ",prefix_slack=" << (prefixSlack == nullptr
                    ? std::string{"none"}
                    : slack_text(prefixSlack->terminalSlack))
              << ",prefix_slack_exact=" << (prefixSlack != nullptr &&
                    prefixSlack->stableId == exact->stableId)
              << ",augmented_retained=" << augmented.retainedNovelRoutes
              << ",augmented_mask=" << portfolio_plan_mask(augmented.portfolio, exact->plan)
              << ",augmented_master_exact=" << contains_candidate(augmentedCandidates, *exact)
              << ",augmented_master_outcome=" << contains_outcome(augmentedCandidates, *exact)
              << ",augmented_best=" << best_candidate_score(augmentedCandidates)
              << ",wide_augmented_exact=" << contains_candidate(wideAugmentedCandidates, *exact)
              << ",wide_augmented_outcome=" << contains_outcome(wideAugmentedCandidates, *exact)
              << ",exact_upper=" << score_text(exactUpper)
              << ",upper_rank=" << upperRank
              << ",first_master_cap=" << firstMasterCap
              << ",diversity=" << diversityRetention
              << ",no_bnb=" << unboundedRetention
              << ",merged_nodes=" << mergedDiagnostics.combinationsVisited
              << ",augmented_nodes=" << augmentedDiagnostics.combinationsVisited
              << '\n';
}

void attribute_oracle_path(
    const Fixture& fixture,
    const OracleResult& oracle,
    const HeadResult& head) {
    std::vector<udon::AgentState> agents;
    for (const udon::CellId start : fixture.config.initialAgents) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            start,
            fixture.config.fuelLimit,
        });
    }
    udon::MatchLedger ledger;
    TrafficFootprint previousOwn{};
    TrafficFootprint priorOwn{};
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const std::vector<udon::RoadStatus> roadStatuses = exact_road_statuses(
            fixture,
            day,
            previousOwn,
            priorOwn);
        const udon::DayState state = day_state(
            fixture.config,
            day,
            agents,
            roadStatuses);
        const udon::DayPlan& plan = oracle.plans.at(
            static_cast<std::size_t>(day - 1));
        attribute_oracle_day(
            fixture,
            day,
            state,
            ledger,
            plan,
            head.plans.at(static_cast<std::size_t>(day - 1)));
        udon::SimulationResult simulation;
        std::string mismatch;
        if (!validates(fixture.config, state, plan, simulation, mismatch)) {
            throw std::runtime_error(
                "oracle attribution replay failed: " + mismatch);
        }
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
        priorOwn = previousOwn;
        previousOwn = compact_road_footprint(
            fixture.config,
            simulation.roadFootprint);
    }
}

void attribute_w1_continuation(
    const Fixture& fixture,
    const OracleResult& oracle) {
    const udon::ExactStepSimulator simulator(fixture.config);
    const udon::IndependentDayValidator validator(fixture.config);
    const udon::ParetoRouter router(fixture.config);
    const udon::RouteColumnGenerator generator(fixture.config, router);
    const udon::RouteMaster master(fixture.config, simulator, validator);
    const udon::FastViabilityAnalyzer viability(fixture.config);
    std::vector<udon::AgentState> agents;
    for (const udon::CellId start : fixture.config.initialAgents) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            start,
            fixture.config.fuelLimit,
        });
    }
    udon::MatchLedger ledger;
    const udon::DayState firstState = day_state(fixture.config, 1, agents);
    const std::optional<udon::MasterCandidate> first = master.evaluate_exact_plan(
        firstState,
        ledger,
        oracle.plans.front());
    if (!first.has_value()) {
        throw std::runtime_error("W1 attribution exact day1 plan is invalid");
    }
    ledger.apply(first->simulation.score);
    agents = first->simulation.finalAgents;
    constexpr std::int32_t perDayRepairCap = 100;
    for (std::int32_t day = 2; day <= fixture.config.day_count(); ++day) {
        const udon::DayState state = day_state(fixture.config, day, agents);
        const udon::DayPlan& oraclePlan = oracle.plans.at(
            static_cast<std::size_t>(day - 1));
        const std::optional<udon::MasterCandidate> exact =
            master.evaluate_exact_plan(state, ledger, oraclePlan);
        if (!exact.has_value()) {
            std::cout << "w1_attribute,seed=" << fixture.seed
                      << ",day=" << day
                      << ",status=oracle-plan-invalid-after-divergence\n";
            break;
        }
        udon::ColumnGenerationOptions generation;
        generation.maximumPathsPerTarget = 1;
        generation.maximumColumnsPerAgent = day <= 3 ? 3 : 2;
        generation.maximumTargetSpots = day <= 3 ? 6 : 4;
        generation.maximumEscorts = day <= 3 ? 4 : 2;
        generation.maximumSeedPlans = day <= 3 ? 0 : 1;
        generation.enableHarvestExtensions = kFutureHarvestMode > 0;
        generation.allowUncachedHarvestTargets = kFutureHarvestMode > 1;
        generation.enableHarvestOrienteering =
            kFutureHarvestMode > 5 &&
            static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                3LL * fixture.config.steps_for_day(day);
        generation.enableExactHarvestOrienteering =
            kFutureHarvestMode > 5 &&
            static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                2LL * fixture.config.steps_for_day(day) &&
            (kFutureHarvestMode > 6 || day == fixture.config.day_count());
        generation.maximumHarvestExtensionSources =
            kFutureHarvestMode > 2 ? 4 : 1;
        generation.maximumHarvestExtensionDepth =
            kFutureHarvestMode > 4 &&
                static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                    3LL * fixture.config.steps_for_day(day)
            ? 4
            : (kFutureHarvestMode > 3 ? 3 : 2);
        udon::MasterOptions masterOptions;
        masterOptions.maximumCombinations = day <= 3
            ? perDayRepairCap
            : std::max(16, perDayRepairCap / 4);
        masterOptions.maximumCandidates = 1;
        masterOptions.maximumResolveRounds = 1;
        if (day > 3) {
            const udon::ViabilityBounds bounds = viability.analyze(state, ledger);
            generation.mandatoryReservations = bounds.reservations;
            masterOptions.mandatoryReservations = bounds.reservations;
        }
        const udon::RoutePortfolio portfolio = generator.generate(
            state,
            ledger,
            generation);
        if (day == 2) {
            for (udon::AgentIndex agent = 0; agent < 2; ++agent) {
                std::uint32_t oracleSpotMask = 0U;
                for (const udon::ClaimEvent& claim : exact->simulation.claims) {
                    if (claim.agent == agent) {
                        oracleSpotMask |= std::uint32_t{1} <<
                            static_cast<std::uint32_t>(claim.spot);
                    }
                }
                const udon::ExactOrienteeringReachability reachability =
                    udon::enumerate_exact_resource_routes(
                        fixture.config,
                        state,
                        agent);
                const auto route_mask_present = [oracleSpotMask](
                    const std::vector<udon::ExactOrienteeringRoute>& routes) {
                    return std::any_of(
                        routes.begin(),
                        routes.end(),
                        [oracleSpotMask](const udon::ExactOrienteeringRoute& route) {
                            return route.spotMask == oracleSpotMask;
                        });
                };
                std::int32_t strictSupersets = 0;
                std::int32_t sameTerminalFuelSupersets = 0;
                for (const udon::ExactOrienteeringRoute& route :
                     reachability.maximalRoutes) {
                    if (route.spotMask != oracleSpotMask &&
                        (route.spotMask & oracleSpotMask) == oracleSpotMask) {
                        ++strictSupersets;
                        const udon::AgentState& oracleTerminal =
                            exact->simulation.finalAgents.at(
                                static_cast<std::size_t>(agent));
                        if (route.terminalCell == oracleTerminal.position &&
                            route.patrolFuel == oracleTerminal.fuel) {
                            ++sameTerminalFuelSupersets;
                        }
                    }
                }
                std::cout << "w1_mask_attribute,seed=" << fixture.seed
                          << ",day=" << day
                          << ",agent=" << agent
                          << ",oracle_mask=" << oracleSpotMask
                          << ",oracle_terminal="
                          << exact->simulation.finalAgents.at(
                                 static_cast<std::size_t>(agent)).position
                          << ",oracle_fuel="
                          << exact->simulation.finalAgents.at(
                                 static_cast<std::size_t>(agent)).fuel
                          << ",maximal_present="
                          << route_mask_present(reachability.maximalRoutes)
                          << ",terminal_present="
                          << route_mask_present(reachability.terminalVariants)
                          << ",strict_supersets=" << strictSupersets
                          << ",same_terminal_fuel_supersets="
                          << sameTerminalFuelSupersets
                          << ",complete=" << reachability.complete
                          << '\n';
            }
            const std::vector<DayOutcome> firstOutcomes = enumerate_day(
                fixture.config,
                day,
                state.agents.at(0).position,
                state.agents.at(0).fuel,
                state.roadStatuses);
            const std::vector<DayOutcome> secondOutcomes = enumerate_day(
                fixture.config,
                day,
                state.agents.at(1).position,
                state.agents.at(1).fuel,
                state.roadStatuses);
            std::uint32_t oracleFirstMask = 0U;
            std::uint32_t oracleSecondMask = 0U;
            for (const udon::ClaimEvent& claim : exact->simulation.claims) {
                if (claim.agent == 0) {
                    oracleFirstMask |= std::uint32_t{1} <<
                        static_cast<std::uint32_t>(claim.spot);
                } else if (claim.agent == 1) {
                    oracleSecondMask |= std::uint32_t{1} <<
                        static_cast<std::uint32_t>(claim.spot);
                }
            }
            const DayOutcome* exactSecondOutcome = nullptr;
            for (const DayOutcome& outcome : secondOutcomes) {
                if (outcome.position == exact->simulation.finalAgents.at(1).position &&
                    outcome.fuel == exact->simulation.finalAgents.at(1).fuel &&
                    outcome.spotMask == oracleSecondMask) {
                    exactSecondOutcome = &outcome;
                    break;
                }
            }
            const auto conditional_rank =
                [&](const DayOutcome& outcome, bool fuelFirst) {
                    const auto [brands, servings] = joint_day_score(
                        fixture.config,
                        outcome.spotMask,
                        oracleSecondMask);
                    const udon::OfficialScore current{
                        static_cast<std::int32_t>(std::popcount(
                            ledger.lifetimeBrands | brands)),
                        ledger.totalDailyDistinct +
                            static_cast<std::int32_t>(std::popcount(brands)),
                        ledger.totalServings + servings,
                    };
                    return fuelFirst
                        ? std::tuple{
                              outcome.fuel,
                              current.lifetimeDistinct,
                              current.totalDailyDistinct,
                              current.totalServings,
                              -static_cast<std::int32_t>(outcome.spotMask)}
                        : std::tuple{
                              current.lifetimeDistinct,
                              current.totalDailyDistinct,
                              current.totalServings,
                              outcome.fuel,
                              -static_cast<std::int32_t>(outcome.spotMask)};
                };
            const udon::CellId oracleFirstTerminal =
                exact->simulation.finalAgents.at(0).position;
            const std::int32_t oracleFirstFuel =
                exact->simulation.finalAgents.at(0).fuel;
            std::int32_t sameTerminalOutcomes = 0;
            std::int32_t oracleOutcomeMatches = 0;
            std::int32_t fuelRank = 1;
            std::int32_t fuelTies = 0;
            std::int32_t currentRank = 1;
            std::int32_t currentTies = 0;
            std::int32_t upperRank = 1;
            std::int32_t upperTies = 0;
            udon::OfficialScore oracleConditionalUpper;
            bool oracleConditionalUpperSet = false;
            const DayOutcome oracleFirstOutcome{
                oracleFirstTerminal,
                oracleFirstFuel,
                oracleFirstMask,
                {},
                {},
            };
            const auto oracleFuelRank = conditional_rank(
                oracleFirstOutcome,
                true);
            const auto oracleCurrentRank = conditional_rank(
                oracleFirstOutcome,
                false);
            if (exactSecondOutcome != nullptr) {
                udon::MatchLedger futureLedger = ledger;
                futureLedger.apply(exact->simulation.score);
                const udon::DayState futureState = day_state(
                    fixture.config,
                    day + 1,
                    exact->simulation.finalAgents);
                oracleConditionalUpper = viability.analyze(
                    futureState,
                    futureLedger).upperBound;
                oracleConditionalUpperSet = true;
            }
            for (const DayOutcome& outcome : firstOutcomes) {
                if (outcome.position != oracleFirstTerminal ||
                    outcome.spotMask == 0U) {
                    continue;
                }
                ++sameTerminalOutcomes;
                oracleOutcomeMatches +=
                    outcome.fuel == oracleFirstFuel &&
                    outcome.spotMask == oracleFirstMask
                    ? 1
                    : 0;
                const auto candidateFuelRank = conditional_rank(outcome, true);
                const auto candidateCurrentRank = conditional_rank(outcome, false);
                fuelRank += oracleFuelRank < candidateFuelRank ? 1 : 0;
                fuelTies += oracleFuelRank == candidateFuelRank ? 1 : 0;
                currentRank += oracleCurrentRank < candidateCurrentRank ? 1 : 0;
                currentTies += oracleCurrentRank == candidateCurrentRank ? 1 : 0;
                if (exactSecondOutcome == nullptr) {
                    continue;
                }
                const auto [brands, servings] = joint_day_score(
                    fixture.config,
                    outcome.spotMask,
                    exactSecondOutcome->spotMask);
                udon::MatchLedger futureLedger = ledger;
                futureLedger.apply(udon::DayScore{
                    brands,
                    static_cast<std::int32_t>(std::popcount(brands)),
                    servings,
                });
                std::vector<udon::AgentState> futureAgents = state.agents;
                futureAgents.at(0).position = outcome.position;
                futureAgents.at(0).fuel = outcome.fuel;
                futureAgents.at(1).position = exactSecondOutcome->position;
                futureAgents.at(1).fuel = exactSecondOutcome->fuel;
                const udon::OfficialScore upper = viability.analyze(
                    day_state(fixture.config, day + 1, futureAgents),
                    futureLedger).upperBound;
                upperRank += oracleConditionalUpper < upper ? 1 : 0;
                upperTies += oracleConditionalUpper == upper ? 1 : 0;
            }
            std::cout << "w1_terminal_rank,seed=" << fixture.seed
                      << ",day=" << day
                      << ",agent=0"
                      << ",terminal=" << oracleFirstTerminal
                      << ",oracle_mask=" << oracleFirstMask
                      << ",oracle_fuel=" << oracleFirstFuel
                      << ",same_terminal_outcomes=" << sameTerminalOutcomes
                      << ",oracle_matches=" << oracleOutcomeMatches
                      << ",fuel_rank=" << fuelRank
                      << ",fuel_ties=" << fuelTies
                      << ",current_rank=" << currentRank
                      << ",current_ties=" << currentTies
                      << ",upper_rank="
                      << (oracleConditionalUpperSet ? upperRank : -1)
                      << ",upper_ties="
                      << (oracleConditionalUpperSet ? upperTies : -1)
                      << ",upper="
                      << (oracleConditionalUpperSet
                              ? score_text(oracleConditionalUpper)
                              : "unavailable")
                      << '\n';
            using JointState = std::tuple<
                udon::CellId,
                std::int32_t,
                udon::CellId,
                std::int32_t,
                std::uint64_t,
                std::int32_t>;
            std::set<JointState> jointStates;
            udon::MatchLedger exactFutureLedger = ledger;
            exactFutureLedger.apply(exact->simulation.score);
            const udon::DayState exactFutureState = day_state(
                fixture.config,
                day + 1,
                exact->simulation.finalAgents);
            const udon::OfficialScore oracleUpper = viability.analyze(
                exactFutureState,
                exactFutureLedger).upperBound;
            udon::OfficialScore maximumUpper;
            std::int32_t betterThanOracle = 0;
            std::int32_t tiedWithOracle = 0;
            bool oracleClassFound = false;
            for (const DayOutcome& firstOutcome : firstOutcomes) {
                for (const DayOutcome& secondOutcome : secondOutcomes) {
                    const auto [brands, servings] = joint_day_score(
                        fixture.config,
                        firstOutcome.spotMask,
                        secondOutcome.spotMask);
                    const JointState key{
                        firstOutcome.position,
                        firstOutcome.fuel,
                        secondOutcome.position,
                        secondOutcome.fuel,
                        brands,
                        servings,
                    };
                    if (!jointStates.insert(key).second) {
                        continue;
                    }
                    udon::MatchLedger futureLedger = ledger;
                    futureLedger.apply(udon::DayScore{
                        brands,
                        static_cast<std::int32_t>(std::popcount(brands)),
                        servings,
                    });
                    std::vector<udon::AgentState> futureAgents = state.agents;
                    futureAgents.at(0).position = firstOutcome.position;
                    futureAgents.at(0).fuel = firstOutcome.fuel;
                    futureAgents.at(1).position = secondOutcome.position;
                    futureAgents.at(1).fuel = secondOutcome.fuel;
                    const udon::DayState futureState = day_state(
                        fixture.config,
                        day + 1,
                        futureAgents);
                    const udon::OfficialScore upper = viability.analyze(
                        futureState,
                        futureLedger).upperBound;
                    if (maximumUpper < upper) {
                        maximumUpper = upper;
                    }
                    betterThanOracle += oracleUpper < upper ? 1 : 0;
                    tiedWithOracle += upper == oracleUpper ? 1 : 0;
                    oracleClassFound = oracleClassFound ||
                        (firstOutcome.position ==
                                exact->simulation.finalAgents.at(0).position &&
                         firstOutcome.fuel ==
                                exact->simulation.finalAgents.at(0).fuel &&
                         secondOutcome.position ==
                                exact->simulation.finalAgents.at(1).position &&
                         secondOutcome.fuel ==
                                exact->simulation.finalAgents.at(1).fuel &&
                         brands == exact->simulation.score.brands &&
                         servings == exact->simulation.score.servings);
                }
            }
            std::cout << "w1_upper_frontier,seed=" << fixture.seed
                      << ",day=" << day
                      << ",first_outcomes=" << firstOutcomes.size()
                      << ",second_outcomes=" << secondOutcomes.size()
                      << ",joint_states=" << jointStates.size()
                      << ",oracle_upper=" << score_text(oracleUpper)
                      << ",maximum_upper=" << score_text(maximumUpper)
                      << ",oracle_rank=" << (betterThanOracle + 1)
                      << ",oracle_ties=" << tiedWithOracle
                      << ",oracle_class_found=" << oracleClassFound
                      << '\n';
            udon::ColumnGenerationOptions anytime = generation;
            anytime.enableExactHarvestOrienteering = true;
            anytime.enableFuelConstrainedExactHarvestOrienteering = true;
            anytime.enableAnytimeFuelConstrainedHarvestOrienteering = true;
            udon::ColumnGenerationDiagnostics anytimeDiagnostics;
            const udon::RoutePortfolio anytimePortfolio = generator.generate(
                state,
                ledger,
                anytime,
                &anytimeDiagnostics);
            std::ostringstream anytimeWidths;
            for (std::size_t agent = 0;
                 agent < anytimePortfolio.columnsByAgent.size();
                 ++agent) {
                if (agent != 0U) {
                    anytimeWidths << '|';
                }
                anytimeWidths << anytimePortfolio.columnsByAgent.at(agent).size();
            }
            std::cout << "w1_anytime_attribute,seed=" << fixture.seed
                      << ",day=" << day
                      << ",oracle_mask="
                      << portfolio_plan_mask(anytimePortfolio, oraclePlan)
                      << ",columns=" << anytimeWidths.str()
                      << ",supported="
                      << anytimeDiagnostics.exactOrienteeringSupportedAgents
                      << ",complete="
                      << anytimeDiagnostics.exactOrienteeringCompleteAgents
                      << ",states="
                      << anytimeDiagnostics.exactOrienteeringSettledStates
                      << ",variants="
                      << anytimeDiagnostics.exactOrienteeringTerminalVariants
                      << ",bundles="
                      << anytimeDiagnostics.exactOrienteeringBundles
                      << ",deadline=" << anytimeDiagnostics.deadlineReached
                      << '\n';
            udon::ColumnGenerationOptions exactFuel = generation;
            exactFuel.enableExactHarvestOrienteering = true;
            exactFuel.enableFuelConstrainedExactHarvestOrienteering = true;
            exactFuel.enableAnytimeFuelConstrainedHarvestOrienteering = false;
            udon::ColumnGenerationDiagnostics exactFuelDiagnostics;
            const udon::RoutePortfolio exactFuelPortfolio = generator.generate(
                state,
                ledger,
                exactFuel,
                &exactFuelDiagnostics);
            std::ostringstream exactFuelWidths;
            for (std::size_t agent = 0;
                 agent < exactFuelPortfolio.columnsByAgent.size();
                 ++agent) {
                if (agent != 0U) {
                    exactFuelWidths << '|';
                }
                exactFuelWidths << exactFuelPortfolio.columnsByAgent.at(agent).size();
            }
            std::cout << "w1_exact_fuel_attribute,seed=" << fixture.seed
                      << ",day=" << day
                      << ",oracle_mask="
                      << portfolio_plan_mask(exactFuelPortfolio, oraclePlan)
                      << ",columns=" << exactFuelWidths.str()
                      << ",supported="
                      << exactFuelDiagnostics.exactOrienteeringSupportedAgents
                      << ",complete="
                      << exactFuelDiagnostics.exactOrienteeringCompleteAgents
                      << ",states="
                      << exactFuelDiagnostics.exactOrienteeringSettledStates
                      << ",variants="
                      << exactFuelDiagnostics.exactOrienteeringTerminalVariants
                      << ",bundles="
                      << exactFuelDiagnostics.exactOrienteeringBundles
                      << ",deadline=" << exactFuelDiagnostics.deadlineReached
                      << '\n';
        }
        udon::MasterDiagnostics diagnostics;
        const std::vector<udon::MasterCandidate> candidates = master.solve(
            state,
            ledger,
            portfolio,
            masterOptions,
            diagnostics);
        std::ostringstream widths;
        for (std::size_t agent = 0; agent < portfolio.columnsByAgent.size(); ++agent) {
            if (agent != 0U) {
                widths << '|';
            }
            widths << portfolio.columnsByAgent.at(agent).size();
        }
        const bool sameOutcome = contains_outcome(candidates, *exact);
        std::cout << "w1_attribute,seed=" << fixture.seed
                  << ",day=" << day
                  << ",oracle_mask=" << portfolio_plan_mask(portfolio, oraclePlan)
                  << ",columns=" << widths.str()
                  << ",nodes=" << diagnostics.combinationsVisited
                  << ",exact=" << score_text(exact->scoreAfterToday)
                  << ",chosen=" << best_candidate_score(candidates)
                  << ",exact_plan_retained=" << contains_candidate(candidates, *exact)
                  << ",exact_outcome_retained=" << sameOutcome
                  << '\n';
        if (!sameOutcome || candidates.empty()) {
            break;
        }
        ledger.apply(candidates.front().simulation.score);
        agents = candidates.front().simulation.finalAgents;
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        const std::vector<ManifestRow> rows = load_manifest(options.manifest, options.split);
        Summary summary;
        std::map<std::pair<std::string, std::string>, Summary> strata;
        std::int32_t headOnlyCases = 0;
        std::int32_t headOnlyInvalid = 0;
        for (const ManifestRow& row : rows) {
            for (std::int32_t offset = 0; offset < row.count; ++offset) {
                const std::uint64_t seed = row.firstSeed + static_cast<std::uint64_t>(offset);
                if (options.onlySeed != 0U && seed != options.onlySeed) {
                    continue;
                }
                if (options.maximumMatches > 0 && summary.cases >= options.maximumMatches) {
                    break;
                }
                const Fixture fixture = make_fixture(row, seed);
                if (options.headOnly) {
                    const HeadResult head = solve_head(fixture);
                    const std::int32_t strictTakeoverDays =
                        static_cast<std::int32_t>(std::count_if(
                            head.audits.begin(),
                            head.audits.end(),
                            [](const udon::DecisionAudit& audit) {
                                return audit.selectionReason ==
                                    "certified-supplemental-strict-dominance";
                            }));
                    ++headOnlyCases;
                    headOnlyInvalid += head.valid ? 0 : 1;
                    std::cout << "head,seed=" << seed
                              << ",family=" << row.family
                              << ",fuel=" << row.fuelProfile
                              << ",score=" << score_text(head.score)
                              << ",valid=" << head.valid
                              << ",plan_hash=" << std::hex
                              << plan_sequence_hash(head.plans) << std::dec
                              << ",strict_takeover_days=" << strictTakeoverDays
                              << '\n';
                    if (options.details) {
                        for (std::size_t day = 0;
                             day < head.audits.size();
                             ++day) {
                            const udon::DecisionAudit& audit =
                                head.audits.at(day);
                            const auto selected = std::find_if(
                                audit.candidates.begin(),
                                audit.candidates.end(),
                                [](const udon::CandidateAuditRecord& record) {
                                    return record.selected;
                                });
                            std::cout << "head_audit,seed=" << seed
                                      << ",day=" << (day + 1U)
                                      << ",reason=" << audit.selectionReason
                                      << ",candidate_count="
                                      << audit.candidates.size();
                            if (selected != audit.candidates.end()) {
                                std::cout << ",current="
                                          << score_text(
                                                 selected->scoreAfterToday)
                                          << ",lower="
                                          << score_text(
                                                 selected
                                                     ->finalCertifiedLowerBound)
                                          << ",upper="
                                          << score_text(
                                                 selected->validUpperBound)
                                          << ",w1=" << selected->w1Role
                                          << ",disposition="
                                          << selected->disposition;
                            }
                            std::cout << '\n';
                            if (day < head.cachePaths.size()) {
                                const HeadResult::CachePathAudit& cache =
                                    head.cachePaths.at(day);
                                std::cout << "cache_path,seed=" << seed
                                          << ",day=" << (day + 1U)
                                          << ",cached=" << cache.cached
                                          << ",eligible=" << cache.eligible
                                          << ",reused=" << cache.reused
                                          << ",rejected=" << cache.rejected
                                          << ",retained=" << cache.retained
                                          << ",selected=" << cache.selected
                                          << ",cached_current="
                                          << score_text(cache.cachedCurrent)
                                          << ",cached_upper="
                                          << score_text(cache.cachedUpper)
                                          << ",selected_current="
                                          << score_text(cache.selectedCurrent)
                                          << ",selected_upper="
                                          << score_text(cache.selectedUpper)
                                          << ",source_multi="
                                          << cache.sourceMultiDayWitnesses
                                          << ",source_plans="
                                          << cache.sourceMaximumPlans
                                          << ",source_final="
                                          << score_text(cache.sourceFinal)
                                          << ",suffix_attempted="
                                          << cache.suffixAttempted
                                          << ",suffix_valid="
                                          << cache.suffixValid
                                          << ",suffix_match="
                                          << cache.suffixScoreMatches
                                          << ",suffix_replay="
                                          << score_text(
                                                 cache.suffixReplayScore)
                                          << '\n';
                            }
                        }
                    }
                    continue;
                }
                const OracleResult oracle = solve_oracle(fixture);
                const HeadResult head = solve_head(fixture);
                record_summary(summary, oracle, head, seed);
                record_summary(strata[{row.family, row.fuelProfile}], oracle, head, seed);
                const std::int32_t tier = first_tier(oracle.score, head.score);
                std::cout << "case,seed=" << seed
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",oracle=" << score_text(oracle.score)
                          << ",head=" << score_text(head.score)
                          << ",tier=" << tier
                          << ",gain=" << tier_gain(oracle.score, head.score, tier)
                          << ",oracle_valid=" << oracle.valid
                          << ",head_valid=" << head.valid
                          << ",max_frontier=" << oracle.maximumFrontier
                          << ",day_enumerations=" << oracle.dailyEnumerations
                          << '\n';
                if (options.details) {
                    for (std::size_t day = 0; day < oracle.plans.size(); ++day) {
                        const std::string oracleId =
                            udon::serialize_day_plan(oracle.plans.at(day)).dump();
                        const std::string headId =
                            udon::serialize_day_plan(head.plans.at(day)).dump();
                        const std::int32_t oracleAuditMatches =
                            static_cast<std::int32_t>(std::count_if(
                                head.audits.at(day).candidates.begin(),
                                head.audits.at(day).candidates.end(),
                                [&oracleId](const udon::CandidateAuditRecord& candidate) {
                                    return candidate.stableId == oracleId;
                                }));
                        std::cout << "trace,seed=" << seed
                                  << ",day=" << (day + 1U)
                                  << ",oracle_score=" << score_text(oracle.cumulative.at(day))
                                  << ",head_score=" << score_text(head.cumulative.at(day))
                                  << ",oracle_agents=" << agents_text(oracle.terminalAgents.at(day))
                                  << ",head_agents=" << agents_text(head.terminalAgents.at(day))
                                  << ",oracle_plan=" << plan_text(oracle.plans.at(day))
                                  << ",head_plan=" << plan_text(head.plans.at(day))
                                  << ",oracle_audit_matches=" << oracleAuditMatches
                                  << ",audit_candidates=" << head.audits.at(day).candidates.size()
                                  << ",oracle_id=" << oracleId
                                  << ",head_id=" << headId
                                  << '\n';
                    }
                    attribute_oracle_path(fixture, oracle, head);
                    attribute_w1_continuation(fixture, oracle);
                }
            }
            if (options.maximumMatches > 0 && summary.cases >= options.maximumMatches) {
                break;
            }
        }
        if (options.headOnly) {
            std::cout << "head_summary,cases=" << headOnlyCases
                      << ",invalid=" << headOnlyInvalid
                      << '\n';
            return headOnlyInvalid == 0 ? 0 : 2;
        }
        for (const auto& [key, stratum] : strata) {
            std::cout << "stratum,family=" << key.first
                      << ",fuel=" << key.second
                      << ",cases=" << stratum.cases
                      << ",oracle_wins=" << stratum.oracleWins
                      << ",ties=" << stratum.ties
                      << ",head_wins=" << stratum.headWins
                      << ",invalid=" << stratum.invalid
                      << '\n';
        }
        std::cout << "summary,cases=" << summary.cases
                  << ",oracle_wins=" << summary.oracleWins
                  << ",ties=" << summary.ties
                  << ",head_wins=" << summary.headWins
                  << ",invalid=" << summary.invalid
                  << ",tier1=" << summary.tier1
                  << ",tier2=" << summary.tier2
                  << ",tier3=" << summary.tier3
                  << ",max_gain=" << summary.maximumGain
                  << ",hash=" << std::hex << summary.resultHash << std::dec
                  << '\n';
        return summary.invalid == 0 && summary.headWins == 0 ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "multi-patrol oracle failed: " << error.what() << '\n';
        return 1;
    }
}
