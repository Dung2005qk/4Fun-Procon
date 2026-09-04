#include <algorithm>
#include <bit>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "udon/json.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"

namespace {

constexpr const char* kExperiment = "ATTR-BIDIRECTIONAL-RCSP-FRONTIER-297";
constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

struct Options {
    std::string manifest = "research/holdouts/ATTR-BIDIRECTIONAL-RCSP-FRONTIER-297.csv";
    std::string split = "development";
    std::optional<std::uint64_t> onlySeed;
    bool details = false;
};

struct ManifestRow {
    std::string family;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t agentCount = 0;
    std::int32_t spotCount = 0;
    std::int32_t players = 0;
    std::int32_t daySteps = 0;
    std::string fuelProfile;
    std::uint64_t firstSeed = 0;
    std::int32_t count = 0;
};

struct Fixture {
    udon::MatchConfig config;
    udon::DayState state;
    udon::CellId start = udon::kInvalidCell;
};

struct Label {
    udon::CellId cell = udon::kInvalidCell;
    std::uint32_t mask = 0U;
    std::int32_t steps = 0;
    std::int32_t fuel = 0;
    std::int32_t parent = -1;
    std::int32_t direction = -1;
    bool active = true;
};

struct Work {
    std::uint64_t settled = 0;
    std::uint64_t arcScans = 0;
    std::uint64_t dominanceChecks = 0;
    std::uint64_t joinPairs = 0;

    [[nodiscard]] std::uint64_t total() const {
        return settled + arcScans + dominanceChecks + joinPairs;
    }

    Work& operator+=(const Work& other) {
        settled += other.settled;
        arcScans += other.arcScans;
        dominanceChecks += other.dominanceChecks;
        joinPairs += other.joinPairs;
        return *this;
    }
};

struct SearchResult {
    std::vector<Label> labels;
    std::vector<std::vector<std::int32_t>> atState;
    Work work;
};

struct TargetBounds {
    std::vector<std::int32_t> steps;
    std::vector<std::int32_t> fuel;
    Work work;
};

struct FrontierEntry {
    std::uint32_t mask = 0U;
    std::int32_t steps = 0;
    std::int32_t fuel = 0;
    std::vector<std::int32_t> directions;

    [[nodiscard]] friend bool operator==(const FrontierEntry& left, const FrontierEntry& right) {
        return left.mask == right.mask && left.steps == right.steps && left.fuel == right.fuel;
    }
};

struct TargetResult {
    std::vector<FrontierEntry> frontier;
    Work work;
    std::int32_t invalid = 0;
    std::int32_t reverseCostFailures = 0;
    std::int32_t reconstructionFailures = 0;
};

struct SolverRun {
    std::vector<TargetResult> targets;
    Work work;
    std::uint64_t frontierHash = kFnvOffset;
    std::int32_t invalid = 0;
    std::int32_t reverseCostFailures = 0;
    std::int32_t reconstructionFailures = 0;
};

struct QueueItem {
    std::int32_t steps = 0;
    std::int32_t fuel = 0;
    std::int32_t label = -1;

    [[nodiscard]] friend bool operator>(const QueueItem& left, const QueueItem& right) {
        return std::tie(left.steps, left.fuel, left.label) >
            std::tie(right.steps, right.fuel, right.label);
    }
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
        hash *= kFnvPrime;
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

[[nodiscard]] Options parse_options(std::int32_t argc, char** argv) {
    Options options;
    for (std::int32_t index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--manifest" && index + 1 < argc) {
            options.manifest = argv[++index];
        } else if (argument == "--split" && index + 1 < argc) {
            options.split = argv[++index];
        } else if (argument == "--only-seed" && index + 1 < argc) {
            options.onlySeed = std::stoull(argv[++index]);
        } else if (argument == "--details") {
            options.details = true;
        } else {
            throw std::invalid_argument("unknown or incomplete option: " + argument);
        }
    }
    if (options.split != "development" && options.split != "verification") {
        throw std::invalid_argument("split must be development or verification");
    }
    return options;
}

[[nodiscard]] std::vector<ManifestRow> load_manifest(
    const std::string& path,
    const std::string& requestedSplit) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open manifest: " + path);
    }
    std::string line;
    constexpr const char* expectedHeader =
        "experiment_id,split,family,width,height,agent_count,spot_count,players,day_steps,fuel_profile,first_seed,count,scope";
    if (!std::getline(input, line) || line != expectedHeader) {
        throw std::runtime_error("unexpected ATTR-BIDIRECTIONAL-RCSP-FRONTIER-297 manifest schema");
    }
    std::vector<ManifestRow> rows;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::vector<std::string> fields = split_csv(line);
        if (fields.size() != 13U || fields.at(0) != kExperiment ||
            fields.at(12) != "targeted-forward-versus-bidirectional-exact-frontier") {
            throw std::runtime_error("invalid manifest row: " + line);
        }
        if (fields.at(1) != requestedSplit) {
            continue;
        }
        ManifestRow row;
        row.family = fields.at(2);
        row.width = std::stoi(fields.at(3));
        row.height = std::stoi(fields.at(4));
        row.agentCount = std::stoi(fields.at(5));
        row.spotCount = std::stoi(fields.at(6));
        row.players = std::stoi(fields.at(7));
        row.daySteps = std::stoi(fields.at(8));
        row.fuelProfile = fields.at(9);
        row.firstSeed = std::stoull(fields.at(10));
        row.count = std::stoi(fields.at(11));
        if (row.width < 8 || row.width > 32 || row.height < 8 || row.height > 32 ||
            row.agentCount < 3 || row.agentCount > 8 || row.spotCount < 1 ||
            row.spotCount > 31 || row.players < 8 || row.players > 10 ||
            row.daySteps <= 0 || row.count <= 0) {
            throw std::runtime_error("manifest row violates official-domain scope: " + line);
        }
        rows.push_back(std::move(row));
    }
    if (rows.empty()) {
        throw std::runtime_error("manifest contains no rows for split " + requestedSplit);
    }
    return rows;
}

[[nodiscard]] std::int32_t terrain_for(
    const ManifestRow& row,
    std::int32_t mapRow,
    std::int32_t column,
    std::uint64_t draw) {
    if (row.family == "plain-open") {
        return static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    if (row.family == "pond-bottleneck") {
        const std::int32_t wall = row.width / 2;
        if (column == wall && mapRow != row.height / 3 && mapRow != (2 * row.height) / 3) {
            return static_cast<std::int32_t>(udon::Terrain::Pond);
        }
        return draw % 9U == 0U
            ? static_cast<std::int32_t>(udon::Terrain::Mountain)
            : static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    if (row.family == "road-threshold") {
        if (column == row.width / 3 || column == (2 * row.width) / 3) {
            return static_cast<std::int32_t>(udon::Terrain::Road);
        }
        const std::uint64_t bucket = draw % 100U;
        if (bucket < 28U) {
            return static_cast<std::int32_t>(udon::Terrain::Road);
        }
        if (bucket < 42U) {
            return static_cast<std::int32_t>(udon::Terrain::Mountain);
        }
        if (bucket < 46U && mapRow > 0 && mapRow + 1 < row.height) {
            return static_cast<std::int32_t>(udon::Terrain::Pond);
        }
        return static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    if (row.family != "mixed-terrain") {
        throw std::invalid_argument("unknown family: " + row.family);
    }
    const std::uint64_t bucket = draw % 100U;
    if (bucket < 18U) {
        return static_cast<std::int32_t>(udon::Terrain::Road);
    }
    if (bucket < 36U) {
        return static_cast<std::int32_t>(udon::Terrain::Mountain);
    }
    if (bucket < 41U && mapRow > 0 && mapRow + 1 < row.height) {
        return static_cast<std::int32_t>(udon::Terrain::Pond);
    }
    return static_cast<std::int32_t>(udon::Terrain::Plain);
}

[[nodiscard]] Fixture make_fixture(const ManifestRow& row, std::uint64_t seed) {
    const std::int32_t cells = row.width * row.height;
    std::vector<std::int32_t> terrain(static_cast<std::size_t>(cells));
    for (std::int32_t cell = 0; cell < cells; ++cell) {
        terrain.at(static_cast<std::size_t>(cell)) = terrain_for(
            row,
            cell / row.width,
            cell % row.width,
            mix64(seed ^ (static_cast<std::uint64_t>(cell) << 21U)));
    }
    for (std::int32_t column = 0; column < row.width; ++column) {
        terrain.at(static_cast<std::size_t>(column)) = static_cast<std::int32_t>(udon::Terrain::Plain);
        terrain.at(static_cast<std::size_t>((row.height - 1) * row.width + column)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    for (std::int32_t mapRow = 0; mapRow < row.height; ++mapRow) {
        terrain.at(static_cast<std::size_t>(mapRow * row.width)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }

    std::vector<udon::CellId> starts;
    starts.reserve(static_cast<std::size_t>(row.agentCount));
    const std::vector<udon::CellId> anchors{
        0,
        row.width - 1,
        (row.height - 1) * row.width,
        cells - 1,
        row.width / 2,
        (row.height - 1) * row.width + row.width / 2,
        (row.height / 2) * row.width,
        (row.height / 2) * row.width + row.width - 1,
    };
    std::unordered_set<udon::CellId> reserved;
    for (std::int32_t index = 0; index < row.agentCount; ++index) {
        udon::CellId cell = anchors.at(static_cast<std::size_t>(index));
        if (!reserved.insert(cell).second) {
            for (cell = 0; cell < cells && !reserved.insert(cell).second; ++cell) {
            }
        }
        starts.push_back(cell);
        terrain.at(static_cast<std::size_t>(cell)) = static_cast<std::int32_t>(udon::Terrain::Plain);
    }

    std::vector<udon::CellId> spotCells;
    spotCells.reserve(static_cast<std::size_t>(row.spotCount));
    std::uint64_t cursor = seed ^ 0xd6e8feb86659fd93ULL;
    while (static_cast<std::int32_t>(spotCells.size()) < row.spotCount) {
        cursor = mix64(cursor);
        const udon::CellId cell = static_cast<udon::CellId>(cursor % static_cast<std::uint64_t>(cells));
        if (reserved.insert(cell).second) {
            spotCells.push_back(cell);
            terrain.at(static_cast<std::size_t>(cell)) = static_cast<std::int32_t>(udon::Terrain::Plain);
        }
    }

    std::int32_t fuelLimit = row.daySteps;
    if (row.fuelProfile == "low") {
        fuelLimit = std::max(3, (2 * row.daySteps) / 3);
    } else if (row.fuelProfile == "high") {
        fuelLimit = 2 * row.daySteps;
    } else if (row.fuelProfile != "default") {
        throw std::invalid_argument("unknown fuel profile: " + row.fuelProfile);
    }

    std::ostringstream document;
    document << "{\"startsAt\":1788480000,\"daySeconds\":[30,30,30,30],\"daySteps\":["
             << row.daySteps << ',' << row.daySteps << ',' << row.daySteps << ',' << row.daySteps
             << "],\"map\":{\"height\":" << row.height << ",\"width\":" << row.width
             << ",\"cells\":[";
    for (std::int32_t mapRow = 0; mapRow < row.height; ++mapRow) {
        if (mapRow != 0) {
            document << ',';
        }
        document << '[';
        for (std::int32_t column = 0; column < row.width; ++column) {
            if (column != 0) {
                document << ',';
            }
            document << terrain.at(static_cast<std::size_t>(mapRow * row.width + column));
        }
        document << ']';
    }
    document << "]},\"spots\":[";
    for (std::int32_t index = 0; index < row.spotCount; ++index) {
        if (index != 0) {
            document << ',';
        }
        const std::int32_t stock = 1 + static_cast<std::int32_t>(
            mix64(seed ^ (static_cast<std::uint64_t>(index) << 37U)) %
            static_cast<std::uint64_t>(row.agentCount));
        document << "{\"brand\":" << 1000 + index << ",\"pos\":"
                 << spotCells.at(static_cast<std::size_t>(index)) << ",\"stocks\":" << stock << '}';
    }
    document << "],\"agents\":[";
    for (std::size_t index = 0; index < starts.size(); ++index) {
        if (index != 0U) {
            document << ',';
        }
        document << starts.at(index);
    }
    document << "],\"fuelLimits\":" << fuelLimit << ",\"players\":" << row.players
             << ",\"busyThreshold\":2,\"jammedThreshold\":5}";

    Fixture fixture;
    fixture.config = udon::parse_match_config(udon::JsonValue::parse(document.str()));
    fixture.state.endsAt = 1788480030;
    fixture.state.dayNumber = 3;
    fixture.state.roadStatuses.assign(static_cast<std::size_t>(cells), udon::RoadStatus::Smooth);
    for (const udon::CellId road : fixture.config.roadCells) {
        const std::uint64_t draw = mix64(seed ^ (static_cast<std::uint64_t>(road) << 7U));
        if (row.family == "road-threshold") {
            fixture.state.roadStatuses.at(static_cast<std::size_t>(road)) =
                draw % 3U == 0U ? udon::RoadStatus::Jammed : udon::RoadStatus::Busy;
        } else if (draw % 7U == 0U) {
            fixture.state.roadStatuses.at(static_cast<std::size_t>(road)) = udon::RoadStatus::Busy;
        }
    }
    for (const udon::CellId start : starts) {
        fixture.state.agents.push_back(udon::AgentState{udon::AgentKind::Patrol, start, fuelLimit});
    }
    fixture.start = starts.front();
    return fixture;
}

[[nodiscard]] std::uint32_t mask_at(const udon::MatchConfig& config, udon::CellId cell) {
    const udon::SpotIndex spot = config.spotAtCell.at(static_cast<std::size_t>(cell));
    return spot == udon::kInvalidSpot
        ? 0U
        : (std::uint32_t{1} << static_cast<std::uint32_t>(spot));
}

[[nodiscard]] std::int32_t state_index(
    const udon::MatchConfig& config,
    std::uint32_t mask,
    udon::CellId cell) {
    return static_cast<std::int32_t>(mask) * config.map.cell_count() + cell;
}

[[nodiscard]] bool insert_label(
    const udon::MatchConfig& config,
    SearchResult& result,
    Label candidate,
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>>& queue) {
    std::vector<std::int32_t>& bucket = result.atState.at(static_cast<std::size_t>(
        state_index(config, candidate.mask, candidate.cell)));
    for (const std::int32_t labelIndex : bucket) {
        Label& existing = result.labels.at(static_cast<std::size_t>(labelIndex));
        if (!existing.active) {
            continue;
        }
        ++result.work.dominanceChecks;
        if (existing.steps <= candidate.steps && existing.fuel <= candidate.fuel) {
            return false;
        }
    }
    for (const std::int32_t labelIndex : bucket) {
        Label& existing = result.labels.at(static_cast<std::size_t>(labelIndex));
        if (!existing.active) {
            continue;
        }
        ++result.work.dominanceChecks;
        if (candidate.steps <= existing.steps && candidate.fuel <= existing.fuel) {
            existing.active = false;
        }
    }
    const std::int32_t index = static_cast<std::int32_t>(result.labels.size());
    result.labels.push_back(std::move(candidate));
    bucket.push_back(index);
    const Label& inserted = result.labels.back();
    queue.push(QueueItem{inserted.steps, inserted.fuel, index});
    return true;
}

[[nodiscard]] std::pair<std::vector<std::int32_t>, Work> lower_bound_to_target(
    const Fixture& fixture,
    udon::CellId target,
    bool fuelResource) {
    const udon::MatchConfig& config = fixture.config;
    const std::int32_t cells = config.map.cell_count();
    const std::int32_t infinity = std::numeric_limits<std::int32_t>::max() / 4;
    std::vector<std::vector<std::pair<udon::CellId, std::int32_t>>> incoming(
        static_cast<std::size_t>(cells));
    Work work;
    for (udon::CellId source = 0; source < cells; ++source) {
        if (config.map.terrain.at(static_cast<std::size_t>(source)) == udon::Terrain::Pond) {
            continue;
        }
        for (std::int32_t direction = 0; direction < udon::kDirectionCount; ++direction) {
            ++work.arcScans;
            const udon::CellId destination = config.map.neighbors.at(static_cast<std::size_t>(source))
                .at(static_cast<std::size_t>(direction));
            if (destination != udon::kInvalidCell &&
                config.map.terrain.at(static_cast<std::size_t>(destination)) != udon::Terrain::Pond) {
                incoming.at(static_cast<std::size_t>(destination)).emplace_back(source, direction);
            }
        }
    }
    std::vector<std::int32_t> distance(static_cast<std::size_t>(cells), infinity);
    using DistanceCell = std::pair<std::int32_t, udon::CellId>;
    std::priority_queue<DistanceCell, std::vector<DistanceCell>, std::greater<>> queue;
    distance.at(static_cast<std::size_t>(target)) = 0;
    queue.emplace(0, target);
    while (!queue.empty()) {
        const auto [currentDistance, cell] = queue.top();
        queue.pop();
        if (currentDistance != distance.at(static_cast<std::size_t>(cell))) {
            continue;
        }
        ++work.settled;
        for (const auto& [predecessor, direction] : incoming.at(static_cast<std::size_t>(cell))) {
            static_cast<void>(direction);
            ++work.arcScans;
            const udon::MoveCost cost = config.move_cost(
                predecessor,
                fixture.state.roadStatuses.at(static_cast<std::size_t>(predecessor)));
            const std::int32_t weight = fuelResource ? cost.patrolFuel : cost.steps;
            if (currentDistance + weight < distance.at(static_cast<std::size_t>(predecessor))) {
                distance.at(static_cast<std::size_t>(predecessor)) = currentDistance + weight;
                queue.emplace(currentDistance + weight, predecessor);
            }
        }
    }
    return {std::move(distance), work};
}

[[nodiscard]] TargetBounds target_bounds(const Fixture& fixture, udon::CellId target) {
    TargetBounds result;
    auto [stepBounds, stepWork] = lower_bound_to_target(fixture, target, false);
    auto [fuelBounds, fuelWork] = lower_bound_to_target(fixture, target, true);
    result.steps = std::move(stepBounds);
    result.fuel = std::move(fuelBounds);
    result.work += stepWork;
    result.work += fuelWork;
    return result;
}

[[nodiscard]] SearchResult search_forward(
    const Fixture& fixture,
    std::int32_t stepCap,
    const TargetBounds* bounds = nullptr) {
    const udon::MatchConfig& config = fixture.config;
    const std::size_t stateCount =
        (std::size_t{1} << config.spots.size()) * static_cast<std::size_t>(config.map.cell_count());
    SearchResult result;
    result.atState.resize(stateCount);
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>> queue;
    static_cast<void>(insert_label(
        config,
        result,
        Label{fixture.start, mask_at(config, fixture.start)},
        queue));
    while (!queue.empty()) {
        const QueueItem item = queue.top();
        queue.pop();
        const Label label = result.labels.at(static_cast<std::size_t>(item.label));
        if (!label.active) {
            continue;
        }
        ++result.work.settled;
        const udon::MoveCost cost = config.move_cost(
            label.cell,
            fixture.state.roadStatuses.at(static_cast<std::size_t>(label.cell)));
        for (std::int32_t direction = 0; direction < udon::kDirectionCount; ++direction) {
            ++result.work.arcScans;
            const udon::CellId next = config.map.neighbors.at(static_cast<std::size_t>(label.cell))
                .at(static_cast<std::size_t>(direction));
            if (next == udon::kInvalidCell ||
                config.map.terrain.at(static_cast<std::size_t>(next)) == udon::Terrain::Pond) {
                continue;
            }
            const std::int32_t nextSteps = label.steps + cost.steps;
            const std::int32_t nextFuel = label.fuel + cost.patrolFuel;
            if (nextSteps > stepCap || nextFuel > config.fuelLimit) {
                continue;
            }
            if (bounds != nullptr) {
                const std::int32_t minimumRemainingSteps =
                    bounds->steps.at(static_cast<std::size_t>(next));
                const std::int32_t minimumRemainingFuel =
                    bounds->fuel.at(static_cast<std::size_t>(next));
                if (minimumRemainingSteps > stepCap - nextSteps ||
                    minimumRemainingFuel > config.fuelLimit - nextFuel) {
                    continue;
                }
            }
            static_cast<void>(insert_label(config, result, Label{
                next,
                label.mask | mask_at(config, next),
                nextSteps,
                nextFuel,
                item.label,
                direction,
                true,
            }, queue));
        }
    }
    return result;
}

[[nodiscard]] SearchResult search_backward(
    const Fixture& fixture,
    udon::CellId target,
    std::int32_t stepCap,
    std::int32_t& reverseCostFailures) {
    const udon::MatchConfig& config = fixture.config;
    const std::size_t stateCount =
        (std::size_t{1} << config.spots.size()) * static_cast<std::size_t>(config.map.cell_count());
    SearchResult result;
    result.atState.resize(stateCount);
    std::vector<std::vector<std::pair<udon::CellId, std::int32_t>>> incoming(
        static_cast<std::size_t>(config.map.cell_count()));
    for (udon::CellId predecessor = 0; predecessor < config.map.cell_count(); ++predecessor) {
        if (config.map.terrain.at(static_cast<std::size_t>(predecessor)) == udon::Terrain::Pond) {
            continue;
        }
        for (std::int32_t direction = 0; direction < udon::kDirectionCount; ++direction) {
            ++result.work.arcScans;
            const udon::CellId destination = config.map.neighbors.at(static_cast<std::size_t>(predecessor))
                .at(static_cast<std::size_t>(direction));
            if (destination != udon::kInvalidCell &&
                config.map.terrain.at(static_cast<std::size_t>(destination)) != udon::Terrain::Pond) {
                incoming.at(static_cast<std::size_t>(destination)).emplace_back(predecessor, direction);
            }
        }
    }
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>> queue;
    static_cast<void>(insert_label(
        config,
        result,
        Label{target, mask_at(config, target)},
        queue));
    while (!queue.empty()) {
        const QueueItem item = queue.top();
        queue.pop();
        const Label label = result.labels.at(static_cast<std::size_t>(item.label));
        if (!label.active) {
            continue;
        }
        ++result.work.settled;
        for (const auto& [predecessor, forwardDirection] :
             incoming.at(static_cast<std::size_t>(label.cell))) {
            ++result.work.arcScans;
            const udon::MoveCost sourceCost = config.move_cost(
                predecessor,
                fixture.state.roadStatuses.at(static_cast<std::size_t>(predecessor)));
            if (config.map.neighbors.at(static_cast<std::size_t>(predecessor))
                    .at(static_cast<std::size_t>(forwardDirection)) != label.cell ||
                sourceCost.steps <= 0 || sourceCost.patrolFuel < 0) {
                ++reverseCostFailures;
            }
            const std::int32_t nextSteps = label.steps + sourceCost.steps;
            const std::int32_t nextFuel = label.fuel + sourceCost.patrolFuel;
            if (nextSteps > stepCap || nextFuel > config.fuelLimit) {
                continue;
            }
            static_cast<void>(insert_label(config, result, Label{
                predecessor,
                label.mask | mask_at(config, predecessor),
                nextSteps,
                nextFuel,
                item.label,
                forwardDirection,
                true,
            }, queue));
        }
    }
    return result;
}

[[nodiscard]] std::vector<std::int32_t> reconstruct_forward(
    const SearchResult& search,
    std::int32_t index) {
    std::vector<std::int32_t> reversed;
    while (index >= 0) {
        const Label& label = search.labels.at(static_cast<std::size_t>(index));
        if (label.parent >= 0) {
            reversed.push_back(label.direction);
        }
        index = label.parent;
    }
    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}

[[nodiscard]] std::vector<std::int32_t> reconstruct_backward(
    const SearchResult& search,
    std::int32_t index) {
    std::vector<std::int32_t> directions;
    while (index >= 0) {
        const Label& label = search.labels.at(static_cast<std::size_t>(index));
        if (label.parent >= 0) {
            directions.push_back(label.direction);
        }
        index = label.parent;
    }
    return directions;
}

[[nodiscard]] std::vector<FrontierEntry> pareto_frontier(std::vector<FrontierEntry> candidates) {
    std::sort(candidates.begin(), candidates.end(), [](const FrontierEntry& left, const FrontierEntry& right) {
        return std::tie(left.mask, left.steps, left.fuel, left.directions) <
            std::tie(right.mask, right.steps, right.fuel, right.directions);
    });
    std::vector<FrontierEntry> frontier;
    for (FrontierEntry& candidate : candidates) {
        bool dominated = false;
        for (const FrontierEntry& existing : frontier) {
            if (existing.mask == candidate.mask && existing.steps <= candidate.steps &&
                existing.fuel <= candidate.fuel) {
                dominated = true;
                break;
            }
        }
        if (dominated) {
            continue;
        }
        frontier.erase(std::remove_if(frontier.begin(), frontier.end(), [&](const FrontierEntry& existing) {
            return existing.mask == candidate.mask && candidate.steps <= existing.steps &&
                candidate.fuel <= existing.fuel;
        }), frontier.end());
        frontier.push_back(std::move(candidate));
    }
    std::sort(frontier.begin(), frontier.end(), [](const FrontierEntry& left, const FrontierEntry& right) {
        return std::tie(left.mask, left.steps, left.fuel) < std::tie(right.mask, right.steps, right.fuel);
    });
    return frontier;
}

[[nodiscard]] std::vector<FrontierEntry> frontier_at_target(
    const Fixture& fixture,
    const SearchResult& search,
    udon::CellId target) {
    std::vector<FrontierEntry> candidates;
    for (std::int32_t index = 0; index < static_cast<std::int32_t>(search.labels.size()); ++index) {
        const Label& label = search.labels.at(static_cast<std::size_t>(index));
        if (label.active && label.cell == target) {
            candidates.push_back(FrontierEntry{
                label.mask,
                label.steps,
                label.fuel,
                reconstruct_forward(search, index),
            });
        }
    }
    return pareto_frontier(std::move(candidates));
}

[[nodiscard]] TargetResult bidirectional_target(
    const Fixture& fixture,
    const SearchResult& forwardHalf,
    udon::CellId target,
    std::int32_t backwardCap) {
    TargetResult result;
    SearchResult backward = search_backward(
        fixture,
        target,
        backwardCap,
        result.reverseCostFailures);
    result.work += backward.work;
    std::vector<FrontierEntry> candidates;
    const udon::MatchConfig& config = fixture.config;
    std::vector<std::vector<std::int32_t>> backwardAtCell(
        static_cast<std::size_t>(config.map.cell_count()));
    for (std::int32_t index = 0; index < static_cast<std::int32_t>(backward.labels.size()); ++index) {
        const Label& label = backward.labels.at(static_cast<std::size_t>(index));
        if (label.active) {
            backwardAtCell.at(static_cast<std::size_t>(label.cell)).push_back(index);
        }
    }
    for (std::int32_t forwardIndex = 0;
         forwardIndex < static_cast<std::int32_t>(forwardHalf.labels.size());
         ++forwardIndex) {
        const Label& forward = forwardHalf.labels.at(static_cast<std::size_t>(forwardIndex));
        if (!forward.active) {
            continue;
        }
        for (const std::int32_t backwardIndex :
             backwardAtCell.at(static_cast<std::size_t>(forward.cell))) {
            const Label& reverse = backward.labels.at(static_cast<std::size_t>(backwardIndex));
            ++result.work.joinPairs;
            const std::int32_t steps = forward.steps + reverse.steps;
            const std::int32_t fuel = forward.fuel + reverse.fuel;
            if (steps > fixture.config.steps_for_day(fixture.state.dayNumber) ||
                fuel > fixture.config.fuelLimit) {
                continue;
            }
            std::vector<std::int32_t> directions = reconstruct_forward(forwardHalf, forwardIndex);
            std::vector<std::int32_t> suffix = reconstruct_backward(backward, backwardIndex);
            directions.insert(directions.end(), suffix.begin(), suffix.end());
            candidates.push_back(FrontierEntry{
                forward.mask | reverse.mask,
                steps,
                fuel,
                std::move(directions),
            });
        }
    }
    result.frontier = pareto_frontier(std::move(candidates));
    return result;
}

[[nodiscard]] bool validate_entry(
    const Fixture& fixture,
    const FrontierEntry& entry,
    udon::CellId target,
    std::string& mismatch) {
    udon::DayPlan plan;
    plan.actions.resize(static_cast<std::size_t>(fixture.config.agent_count()));
    for (const std::int32_t direction : entry.directions) {
        plan.actions.front().push_back(udon::PlanAction::move(direction));
    }
    const std::int32_t remaining =
        fixture.config.steps_for_day(fixture.state.dayNumber) - entry.steps;
    if (remaining > 0) {
        plan.actions.front().push_back(udon::PlanAction::wait(remaining));
    }
    for (std::int32_t agent = 1; agent < fixture.config.agent_count(); ++agent) {
        plan.actions.at(static_cast<std::size_t>(agent)).push_back(
            udon::PlanAction::wait(fixture.config.steps_for_day(fixture.state.dayNumber)));
    }
    const udon::ExactStepSimulator simulator(fixture.config);
    const udon::IndependentDayValidator validator(fixture.config);
    const udon::SimulationResult simulation = simulator.simulate(fixture.state, plan, false);
    const udon::SimulationResult independent = validator.validate(fixture.state, plan, false);
    if (!simulation.valid || !independent.valid ||
        !validator.agrees_with(simulation, independent, mismatch)) {
        return false;
    }
    if (simulation.finalAgents.front().position != target ||
        simulation.finalAgents.front().fuel != fixture.config.fuelLimit - entry.fuel) {
        mismatch = "terminal state differs from frontier tuple";
        return false;
    }
    std::uint32_t observedMask = 0U;
    for (const udon::ClaimEvent& claim : simulation.claims) {
        if (claim.agent == 0 && claim.served) {
            observedMask |= std::uint32_t{1} << static_cast<std::uint32_t>(claim.spot);
        }
    }
    if (observedMask != entry.mask) {
        mismatch = "simulated claim mask differs from frontier tuple";
        return false;
    }
    return true;
}

void hash_frontier(std::uint64_t& hash, const std::vector<FrontierEntry>& frontier) {
    hash_value(hash, frontier.size());
    for (const FrontierEntry& entry : frontier) {
        hash_value(hash, entry.mask);
        hash_value(hash, static_cast<std::uint64_t>(entry.steps));
        hash_value(hash, static_cast<std::uint64_t>(entry.fuel));
    }
}

[[nodiscard]] SolverRun run_forward_targets(
    const Fixture& fixture,
    const std::vector<udon::CellId>& targets) {
    SolverRun run;
    run.targets.reserve(targets.size());
    for (const udon::CellId target : targets) {
        const TargetBounds bounds = target_bounds(fixture, target);
        SearchResult search = search_forward(
            fixture,
            fixture.config.steps_for_day(fixture.state.dayNumber),
            &bounds);
        TargetResult targetResult;
        targetResult.work = search.work;
        targetResult.work += bounds.work;
        targetResult.frontier = frontier_at_target(fixture, search, target);
        for (const FrontierEntry& entry : targetResult.frontier) {
            std::string mismatch;
            if (!validate_entry(fixture, entry, target, mismatch)) {
                ++targetResult.invalid;
            }
        }
        run.work += targetResult.work;
        run.invalid += targetResult.invalid;
        hash_frontier(run.frontierHash, targetResult.frontier);
        run.targets.push_back(std::move(targetResult));
    }
    return run;
}

[[nodiscard]] SolverRun run_bidirectional_targets(
    const Fixture& fixture,
    const std::vector<udon::CellId>& targets) {
    SolverRun run;
    run.targets.reserve(targets.size());
    const std::int32_t budget = fixture.config.steps_for_day(fixture.state.dayNumber);
    constexpr std::int32_t maxEdgeSteps = 4;
    const std::int32_t forwardCap = budget / 2 + maxEdgeSteps;
    const std::int32_t backwardCap = budget - budget / 2;
    SearchResult forwardHalf = search_forward(fixture, forwardCap);
    run.work += forwardHalf.work;
    for (const udon::CellId target : targets) {
        TargetResult targetResult = bidirectional_target(
            fixture,
            forwardHalf,
            target,
            backwardCap);
        for (const FrontierEntry& entry : targetResult.frontier) {
            std::string mismatch;
            if (!validate_entry(fixture, entry, target, mismatch)) {
                ++targetResult.invalid;
            }
        }
        run.work += targetResult.work;
        run.invalid += targetResult.invalid;
        run.reverseCostFailures += targetResult.reverseCostFailures;
        run.reconstructionFailures += targetResult.reconstructionFailures;
        hash_frontier(run.frontierHash, targetResult.frontier);
        run.targets.push_back(std::move(targetResult));
    }
    return run;
}

[[nodiscard]] std::vector<std::tuple<std::uint32_t, std::int32_t, std::int32_t>> keys(
    const std::vector<FrontierEntry>& frontier) {
    std::vector<std::tuple<std::uint32_t, std::int32_t, std::int32_t>> result;
    result.reserve(frontier.size());
    for (const FrontierEntry& entry : frontier) {
        result.emplace_back(entry.mask, entry.steps, entry.fuel);
    }
    return result;
}

[[nodiscard]] std::tuple<std::int32_t, std::int32_t, std::int32_t> best_rank(
    const std::vector<FrontierEntry>& frontier) {
    std::tuple<std::int32_t, std::int32_t, std::int32_t> best{
        std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::min(),
    };
    for (const FrontierEntry& entry : frontier) {
        best = std::max(best, std::tuple{
            static_cast<std::int32_t>(std::popcount(entry.mask)),
            -entry.steps,
            -entry.fuel,
        });
    }
    return best;
}

[[nodiscard]] std::uint64_t canonical_case_hash(const SolverRun& run) {
    std::vector<std::vector<std::tuple<std::uint32_t, std::int32_t, std::int32_t>>> all;
    all.reserve(run.targets.size());
    for (const TargetResult& target : run.targets) {
        all.push_back(keys(target.frontier));
    }
    std::sort(all.begin(), all.end());
    std::uint64_t hash = kFnvOffset;
    hash_value(hash, all.size());
    for (const auto& frontier : all) {
        hash_value(hash, frontier.size());
        for (const auto& [mask, steps, fuel] : frontier) {
            hash_value(hash, mask);
            hash_value(hash, static_cast<std::uint64_t>(steps));
            hash_value(hash, static_cast<std::uint64_t>(fuel));
        }
    }
    return hash;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        const std::vector<ManifestRow> rows = load_manifest(options.manifest, options.split);
        std::int32_t cases = 0;
        std::int32_t frontierMismatches = 0;
        std::int32_t bestRankMismatches = 0;
        std::int32_t deterministicFailures = 0;
        std::int32_t invalid = 0;
        std::int32_t reverseCostFailures = 0;
        std::int32_t reconstructionFailures = 0;
        std::set<std::string> reducedFamilies;
        std::vector<double> workRatios;
        std::uint64_t summaryHash = kFnvOffset;

        for (const ManifestRow& row : rows) {
            for (std::int32_t offset = 0; offset < row.count; ++offset) {
                const std::uint64_t seed = row.firstSeed + static_cast<std::uint64_t>(offset);
                if (options.onlySeed.has_value() && seed != *options.onlySeed) {
                    continue;
                }
                const Fixture fixture = make_fixture(row, seed);
                std::vector<udon::CellId> targets;
                targets.reserve(fixture.config.spots.size());
                for (const udon::Spot& spot : fixture.config.spots) {
                    targets.push_back(spot.position);
                }
                const SolverRun forward = run_forward_targets(fixture, targets);
                const SolverRun bidirectional = run_bidirectional_targets(fixture, targets);
                std::reverse(targets.begin(), targets.end());
                const SolverRun forwardReverse = run_forward_targets(fixture, targets);
                const SolverRun bidirectionalReverse = run_bidirectional_targets(fixture, targets);

                std::int32_t caseFrontierMismatch = 0;
                std::int32_t caseBestMismatch = 0;
                for (std::size_t target = 0; target < forward.targets.size(); ++target) {
                    if (keys(forward.targets.at(target).frontier) !=
                        keys(bidirectional.targets.at(target).frontier)) {
                        ++caseFrontierMismatch;
                    }
                    if (best_rank(forward.targets.at(target).frontier) !=
                        best_rank(bidirectional.targets.at(target).frontier)) {
                        ++caseBestMismatch;
                    }
                }
                const bool deterministic =
                    canonical_case_hash(forward) == canonical_case_hash(forwardReverse) &&
                    canonical_case_hash(bidirectional) == canonical_case_hash(bidirectionalReverse) &&
                    forward.work.total() == forwardReverse.work.total() &&
                    bidirectional.work.total() == bidirectionalReverse.work.total();
                const double ratio = forward.work.total() == 0U
                    ? 1.0
                    : static_cast<double>(bidirectional.work.total()) /
                        static_cast<double>(forward.work.total());
                if (ratio < 1.0) {
                    reducedFamilies.insert(row.family);
                }
                workRatios.push_back(ratio);
                ++cases;
                frontierMismatches += caseFrontierMismatch;
                bestRankMismatches += caseBestMismatch;
                deterministicFailures += deterministic ? 0 : 1;
                invalid += forward.invalid + bidirectional.invalid +
                    forwardReverse.invalid + bidirectionalReverse.invalid;
                reverseCostFailures += bidirectional.reverseCostFailures +
                    bidirectionalReverse.reverseCostFailures;
                reconstructionFailures += bidirectional.reconstructionFailures +
                    bidirectionalReverse.reconstructionFailures;
                hash_value(summaryHash, seed);
                hash_value(summaryHash, canonical_case_hash(forward));
                hash_value(summaryHash, canonical_case_hash(bidirectional));
                hash_value(summaryHash, forward.work.total());
                hash_value(summaryHash, bidirectional.work.total());

                std::cout << "case"
                          << " split=" << options.split
                          << " family=" << row.family
                          << " seed=" << seed
                          << " targets=" << fixture.config.spots.size()
                          << " forward_work=" << forward.work.total()
                          << " bidirectional_work=" << bidirectional.work.total()
                          << " work_ratio=" << std::fixed << std::setprecision(6) << ratio
                          << " frontier_mismatch=" << caseFrontierMismatch
                          << " best_rank_mismatch=" << caseBestMismatch
                          << " deterministic_failure=" << (deterministic ? 0 : 1)
                          << " invalid=" << (forward.invalid + bidirectional.invalid +
                              forwardReverse.invalid + bidirectionalReverse.invalid)
                          << " reverse_cost_failure=" << (bidirectional.reverseCostFailures +
                              bidirectionalReverse.reverseCostFailures)
                          << " reconstruction_failure=" << (bidirectional.reconstructionFailures +
                              bidirectionalReverse.reconstructionFailures)
                          << '\n';
                if (options.details) {
                    std::cout << "detail seed=" << seed
                              << " forward_settled=" << forward.work.settled
                              << " forward_arcs=" << forward.work.arcScans
                              << " forward_dominance=" << forward.work.dominanceChecks
                              << " bidirectional_settled=" << bidirectional.work.settled
                              << " bidirectional_arcs=" << bidirectional.work.arcScans
                              << " bidirectional_dominance=" << bidirectional.work.dominanceChecks
                              << " bidirectional_join=" << bidirectional.work.joinPairs
                              << " hash=" << std::hex << canonical_case_hash(forward) << std::dec
                              << '\n';
                }
            }
        }
        if (cases == 0) {
            throw std::runtime_error("no manifest case selected");
        }
        std::sort(workRatios.begin(), workRatios.end());
        const double medianRatio = workRatios.size() % 2U == 1U
            ? workRatios.at(workRatios.size() / 2U)
            : (workRatios.at(workRatios.size() / 2U - 1U) +
               workRatios.at(workRatios.size() / 2U)) / 2.0;
        const bool parityPass = frontierMismatches == 0 && bestRankMismatches == 0 &&
            deterministicFailures == 0 && invalid == 0 && reverseCostFailures == 0 &&
            reconstructionFailures == 0;
        std::cout << "summary"
                  << " experiment=" << kExperiment
                  << " split=" << options.split
                  << " cases=" << cases
                  << " frontier_mismatch=" << frontierMismatches
                  << " best_rank_mismatch=" << bestRankMismatches
                  << " deterministic_failure=" << deterministicFailures
                  << " invalid=" << invalid
                  << " reverse_cost_failure=" << reverseCostFailures
                  << " reconstruction_failure=" << reconstructionFailures
                  << " median_work_ratio=" << std::fixed << std::setprecision(6) << medianRatio
                  << " reduced_families=" << reducedFamilies.size()
                  << " parity_pass=" << (parityPass ? 1 : 0)
                  << " result_hash=" << std::hex << summaryHash << std::dec
                  << '\n';
        return parityPass ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "bidirectional_rcsp_probe error: " << error.what() << '\n';
        return 1;
    }
}
