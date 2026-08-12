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
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "udon/decision.hpp"
#include "udon/json.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"

namespace {

constexpr std::chrono::milliseconds kProductionBudget{5000};
constexpr std::int32_t kHarvestMode = 7;
constexpr std::int32_t kFutureHarvestMode = 7;

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
};

struct Fixture {
    udon::MatchConfig config;
    std::string family;
    std::string fuelProfile;
    std::uint64_t seed = 0;
};

struct DayOutcome {
    udon::CellId position = udon::kInvalidCell;
    std::int32_t fuel = 0;
    std::uint32_t spotMask = 0;
    udon::AgentPlan actions;
};

using MatchKey = std::tuple<
    udon::CellId,
    std::int32_t,
    udon::CellId,
    std::int32_t,
    std::uint64_t>;

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
             fields.at(0) != "CEILING-BRANCH-PATROL-089")) {
            throw std::runtime_error("invalid manifest row: " + line);
        }
        if (fields.at(1) != split) {
            continue;
        }
        const std::string expectedSpots = fields.at(0) == "CEILING-MULTI-PATROL-085"
            ? "5"
            : "6";
        if (fields.at(7) != "2" || fields.at(8) != "3" ||
            fields.at(9) != expectedSpots ||
            fields.at(10) != "all-patrol" ||
            fields.at(11) != "complete-two-active-patrol-full-match-dp") {
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
    throw std::invalid_argument("unknown family: " + family);
}

[[nodiscard]] Fixture make_fixture(const ManifestRow& row, std::uint64_t seed) {
    constexpr std::int32_t side = 8;
    constexpr std::int32_t cells = side * side;
    std::vector<std::int32_t> terrain(
        static_cast<std::size_t>(cells),
        static_cast<std::int32_t>(udon::Terrain::Pond));
    terrain.at(0) = static_cast<std::int32_t>(udon::Terrain::Plain);
    const bool branched = row.experimentId == "CEILING-BRANCH-PATROL-089";
    std::vector<udon::CellId> spotCells;
    if (branched) {
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
    if (row.family == "terminal-separation" || row.family == "terminal-fork") {
        const udon::CellId protectedTerminal = branched ? 26 : 19;
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
    std::int32_t fuelLimit = branched ? 24 : 20;
    if (row.fuelProfile == "low") {
        fuelLimit = branched ? 12 : 10;
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
            row.family == "stock-race" || row.family == "rare-fork") {
            stock = 1;
        } else if (row.family == "fuel-allocation" || row.family == "fuel-split") {
            stock = 1 + static_cast<std::int32_t>((spot + seed) % 2U);
        }
        document << "{\"brand\":" << brands.at(spot)
                 << ",\"pos\":" << spotCells.at(spot)
                 << ",\"stocks\":" << stock << '}';
    }
    document << "],\"agents\":[16," << (branched ? 21 : 22)
             << ",0],\"fuelLimits\":" << fuelLimit
             << ",\"players\":4,\"busyThreshold\":2,\"jammedThreshold\":4}";

    Fixture fixture;
    fixture.config = udon::parse_match_config(udon::JsonValue::parse(document.str()));
    fixture.family = row.family;
    fixture.fuelProfile = row.fuelProfile;
    fixture.seed = seed;
    if (!fixture.config.roadCells.empty()) {
        throw std::runtime_error("multi-patrol exact domain unexpectedly contains roads");
    }
    return fixture;
}

[[nodiscard]] udon::DayState day_state(
    const udon::MatchConfig& config,
    std::int32_t day,
    const std::vector<udon::AgentState>& agents) {
    udon::DayState state;
    state.endsAt = config.startsAt + static_cast<std::int64_t>(day) * 5;
    state.dayNumber = day;
    state.agents = agents;
    state.roadStatuses.assign(
        static_cast<std::size_t>(config.map.cell_count()),
        udon::RoadStatus::Smooth);
    return state;
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
    std::int32_t availableFuel) {
    struct Node {
        std::int32_t steps = 0;
        std::int32_t fuelUsed = 0;
        udon::CellId cell = udon::kInvalidCell;
        std::uint32_t spotMask = 0;
        std::int32_t parent = -1;
        udon::PlanAction incoming = udon::PlanAction::wait(1);
    };
    const std::int32_t limit = config.steps_for_day(day);
    std::vector<Node> nodes{
        Node{0, 0, start, 0U, -1, udon::PlanAction::wait(1)},
    };
    std::set<std::tuple<std::int32_t, std::int32_t, udon::CellId, std::uint32_t>> seen;
    seen.emplace(0, 0, start, 0U);
    const auto push = [&nodes, &seen](const Node& node) {
        if (seen.emplace(node.steps, node.fuelUsed, node.cell, node.spotMask).second) {
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
            waited.spotMask |=
                std::uint32_t{1} << static_cast<std::uint32_t>(currentSpot);
            waited.parent = static_cast<std::int32_t>(cursor);
            waited.incoming = udon::PlanAction::wait(1);
            push(waited);
        }
        const udon::MoveCost cost = config.move_cost(current.cell, udon::RoadStatus::Smooth);
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

    using OutcomeKey = std::tuple<udon::CellId, std::int32_t, std::uint32_t>;
    std::map<OutcomeKey, std::int32_t> compressed;
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const Node& node = nodes.at(index);
        const OutcomeKey key{node.cell, availableFuel - node.fuelUsed, node.spotMask};
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
            if (spotSuperset && other.fuel >= candidate.fuel &&
                (other.spotMask != candidate.spotMask || other.fuel > candidate.fuel)) {
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
        const auto [p0, f0, p1, f1, lifetime] = entries.at(index).key;
        static_cast<void>(lifetime);
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
            const bool lifetimeSuperset =
                (rightLifetime | leftLifetime) == rightLifetime;
            const bool scoreNoWorse =
                entries.at(right).totalDailyDistinct >= entries.at(left).totalDailyDistinct &&
                entries.at(right).totalServings >= entries.at(left).totalServings;
            const bool strict = rightLifetime != leftLifetime ||
                entries.at(right).totalDailyDistinct > entries.at(left).totalDailyDistinct ||
                entries.at(right).totalServings > entries.at(left).totalServings;
            if (lifetimeSuperset && scoreNoWorse && strict) {
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
    layers.push_back(std::vector<LayerEntry>{LayerEntry{
        MatchKey{first.first, first.second, second.first, second.second, 0U},
    }});
    std::map<std::tuple<std::int32_t, udon::CellId, std::int32_t>, std::vector<DayOutcome>> cache;
    OracleResult result;
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const std::vector<LayerEntry>& current = layers.back();
        std::vector<LayerEntry> next;
        std::map<MatchKey, std::size_t> retained;
        for (std::size_t parentIndex = 0; parentIndex < current.size(); ++parentIndex) {
            const LayerEntry& parent = current.at(parentIndex);
            const auto [firstPosition, firstFuel, secondPosition, secondFuel, lifetime] =
                parent.key;
            const auto firstCacheKey = std::tuple{day, firstPosition, firstFuel};
            auto firstFound = cache.find(firstCacheKey);
            if (firstFound == cache.end()) {
                firstFound = cache.emplace(
                    firstCacheKey,
                    enumerate_day(fixture.config, day, firstPosition, firstFuel)).first;
                ++result.dailyEnumerations;
            }
            const auto secondCacheKey = std::tuple{day, secondPosition, secondFuel};
            auto secondFound = cache.find(secondCacheKey);
            if (secondFound == cache.end()) {
                secondFound = cache.emplace(
                    secondCacheKey,
                    enumerate_day(fixture.config, day, secondPosition, secondFuel)).first;
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
                    const MatchKey key{
                        nextP0,
                        nextF0,
                        nextP1,
                        nextF1,
                        lifetime | dailyBrands,
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
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const AbstractDay& abstract = abstractDays.at(static_cast<std::size_t>(day - 1));
        udon::DayPlan plan;
        plan.actions.resize(static_cast<std::size_t>(fixture.config.agent_count()));
        plan.actions.at(abstractToPhysical.at(0)) = abstract.first;
        plan.actions.at(abstractToPhysical.at(1)) = abstract.second;
        plan.actions.at(2) = udon::AgentPlan{
            udon::PlanAction::wait(fixture.config.steps_for_day(day)),
        };
        const udon::DayState state = day_state(fixture.config, day, agents);
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
        const auto [expectedP0, expectedF0, expectedP1, expectedF1, expectedLifetime] =
            abstract.terminal;
        if (agents.at(abstractToPhysical.at(0)).position != expectedP0 ||
            agents.at(abstractToPhysical.at(0)).fuel != expectedF0 ||
            agents.at(abstractToPhysical.at(1)).position != expectedP1 ||
            agents.at(abstractToPhysical.at(1)).fuel != expectedF1 ||
            ledger.lifetimeBrands != expectedLifetime) {
            throw std::runtime_error("oracle witness disagrees with canonical DP state");
        }
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
    std::vector<udon::AgentState> agents;
    for (const udon::CellId start : fixture.config.initialAgents) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            start,
            fixture.config.fuelLimit,
        });
    }
    udon::MatchLedger ledger;
    HeadResult result;
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const udon::DayState state = day_state(fixture.config, day, agents);
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
    const udon::DayPlan& oraclePlan) {
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
    const OracleResult& oracle) {
    std::vector<udon::AgentState> agents;
    for (const udon::CellId start : fixture.config.initialAgents) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            start,
            fixture.config.fuelLimit,
        });
    }
    udon::MatchLedger ledger;
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const udon::DayState state = day_state(fixture.config, day, agents);
        const udon::DayPlan& plan = oracle.plans.at(
            static_cast<std::size_t>(day - 1));
        attribute_oracle_day(fixture, day, state, ledger, plan);
        udon::SimulationResult simulation;
        std::string mismatch;
        if (!validates(fixture.config, state, plan, simulation, mismatch)) {
            throw std::runtime_error(
                "oracle attribution replay failed: " + mismatch);
        }
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        const std::vector<ManifestRow> rows = load_manifest(options.manifest, options.split);
        Summary summary;
        std::map<std::pair<std::string, std::string>, Summary> strata;
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
                    attribute_oracle_path(fixture, oracle);
                }
            }
            if (options.maximumMatches > 0 && summary.cases >= options.maximumMatches) {
                break;
            }
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
