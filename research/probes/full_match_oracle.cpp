#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
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
    std::string split;
    std::string family;
    std::string fuelProfile;
    std::int32_t horizon = 0;
    std::uint64_t firstSeed = 0;
    std::int32_t count = 0;
};

struct Options {
    std::string manifest = "research/holdouts/CEILING-MATCH-071.csv";
    std::string split = "development";
    bool details = false;
    bool scoresOnly = false;
    bool attributeWitness = false;
    bool attributeRouteClosure = false;
    std::int32_t maximumMatches = 0;
    std::uint64_t onlySeed = 0;
    bool verifyAllOutcomes = false;
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
    udon::BrandMask brands;
    std::int32_t servings = 0;
    udon::AgentPlan actions;
};

struct MatchValue {
    std::int32_t totalDailyDistinct = 0;
    std::int32_t totalServings = 0;
    std::vector<udon::AgentPlan> activePlans;
};

using MatchKey = std::tuple<udon::CellId, std::int32_t, udon::BrandMask>;

struct MatchResult {
    bool valid = false;
    udon::OfficialScore score;
    std::vector<udon::OfficialScore> cumulative;
    std::vector<udon::DayPlan> plans;
    std::vector<udon::CellId> activePositions;
    std::vector<std::int32_t> activeFuels;
    std::vector<std::string> candidateAudits;
    std::size_t maximumFrontier = 0;
    std::size_t dailyEnumerations = 0;
    std::size_t verifiedOutcomes = 0;
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
        } else if (argument == "--verify-all-outcomes") {
            options.verifyAllOutcomes = true;
        } else if (argument == "--details") {
            options.details = true;
        } else if (argument == "--scores-only") {
            options.scoresOnly = true;
        } else if (argument == "--attribute-witness") {
            options.attributeWitness = true;
        } else if (argument == "--attribute-route-closure") {
            options.attributeRouteClosure = true;
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
    const std::string& requestedSplit) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open manifest: " + path);
    }
    std::string line;
    if (!std::getline(input, line) ||
        line != "experiment_id,split,family,fuel_profile,horizon,first_seed,count,active_agents,total_agents,spot_count,role_mode,oracle_scope") {
        throw std::runtime_error("unexpected CEILING-MATCH-071 manifest schema");
    }
    std::vector<ManifestRow> rows;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::vector<std::string> fields = split_csv(line);
        if (fields.size() != 12U || fields.at(0) != "CEILING-MATCH-071") {
            throw std::runtime_error("invalid manifest row: " + line);
        }
        if (fields.at(1) != requestedSplit) {
            continue;
        }
        if (fields.at(7) != "1" || fields.at(8) != "3" || fields.at(9) != "5" ||
            fields.at(10) != "all-patrol" ||
            fields.at(11) != "complete-one-active-patrol-full-match-dp") {
            throw std::runtime_error("manifest row violates oracle scope: " + line);
        }
        rows.push_back(ManifestRow{
            fields.at(1),
            fields.at(2),
            fields.at(3),
            std::stoi(fields.at(4)),
            std::stoull(fields.at(5)),
            std::stoi(fields.at(6)),
        });
    }
    if (rows.empty()) {
        throw std::runtime_error("manifest contains no rows for split " + requestedSplit);
    }
    return rows;
}

[[nodiscard]] std::vector<std::int32_t> brands_for(const std::string& family) {
    if (family == "rare-late") {
        return {100, 100, 200, 200, 999};
    }
    if (family == "daily-choice") {
        return {100, 100, 200, 300, 300};
    }
    if (family == "terminal-position") {
        return {100, 200, 200, 300, 900};
    }
    return {100, 200, 300, 400, 500};
}

[[nodiscard]] Fixture make_fixture(
    const ManifestRow& row,
    std::uint64_t seed) {
    constexpr std::int32_t side = 8;
    constexpr std::int32_t cells = side * side;
    const std::vector<udon::CellId> connected{
        16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31,
        32, 33,
    };
    std::vector<std::int32_t> terrain(
        static_cast<std::size_t>(cells),
        static_cast<std::int32_t>(udon::Terrain::Pond));
    terrain.at(0) = static_cast<std::int32_t>(udon::Terrain::Plain);
    terrain.at(7) = static_cast<std::int32_t>(udon::Terrain::Plain);
    for (const udon::CellId cell : connected) {
        const std::uint64_t draw = mix64(
            seed ^ (static_cast<std::uint64_t>(cell) << 22U));
        std::uint64_t mountainCutoff = 25U;
        if (row.family == "fuel-allocation" || row.family == "mountain-detour") {
            mountainCutoff = 48U;
        }
        terrain.at(static_cast<std::size_t>(cell)) = draw % 100U < mountainCutoff
            ? static_cast<std::int32_t>(udon::Terrain::Mountain)
            : static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    terrain.at(16) = static_cast<std::int32_t>(udon::Terrain::Plain);

    std::vector<udon::CellId> candidates;
    for (const udon::CellId cell : connected) {
        if (cell != 16) {
            candidates.push_back(cell);
        }
    }
    std::sort(
        candidates.begin(),
        candidates.end(),
        [seed](udon::CellId left, udon::CellId right) {
            return mix64(seed ^ static_cast<std::uint64_t>(left)) <
                mix64(seed ^ static_cast<std::uint64_t>(right));
        });
    std::vector<udon::CellId> spotCells(candidates.begin(), candidates.begin() + 5);
    if (row.family == "rare-late") {
        const auto farthest = std::max_element(
            candidates.begin(),
            candidates.end());
        spotCells.clear();
        for (const udon::CellId candidate : candidates) {
            if (candidate != *farthest && spotCells.size() < 4U) {
                spotCells.push_back(candidate);
            }
        }
        spotCells.push_back(*farthest);
    } else if (row.family == "terminal-position") {
        spotCells = {17, 18, 29, 31, 33};
    } else if (row.family == "fuel-allocation") {
        spotCells = {20, 22, 25, 30, 33};
    }
    std::sort(spotCells.begin(), spotCells.end());
    if (std::adjacent_find(spotCells.begin(), spotCells.end()) != spotCells.end()) {
        throw std::runtime_error("fixture generator selected duplicate spots");
    }
    for (const udon::CellId cell : spotCells) {
        terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }

    std::vector<std::int32_t> daySteps;
    daySteps.reserve(static_cast<std::size_t>(row.horizon));
    for (std::int32_t day = 0; day < row.horizon; ++day) {
        daySteps.push_back(16 + static_cast<std::int32_t>(
            mix64(seed ^ (static_cast<std::uint64_t>(day) << 40U)) % 5U));
    }
    std::int32_t fuelLimit = 20;
    if (row.fuelProfile == "low") {
        fuelLimit = 10;
    } else if (row.fuelProfile == "high") {
        fuelLimit = 8 * row.horizon;
    } else if (row.fuelProfile != "default") {
        throw std::invalid_argument("unknown fuel profile: " + row.fuelProfile);
    }

    const std::vector<std::int32_t> brands = brands_for(row.family);
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
        document << "{\"brand\":" << brands.at(spot)
                 << ",\"pos\":" << spotCells.at(spot)
                 << ",\"stocks\":"
                 << (row.family == "daily-choice" && spot % 2U == 0U ? 2 : 1)
                 << '}';
    }
    document << "],\"agents\":[16,0,7],\"fuelLimits\":" << fuelLimit
             << ",\"players\":4,\"busyThreshold\":2,\"jammedThreshold\":4}";

    Fixture fixture;
    fixture.config = udon::parse_match_config(udon::JsonValue::parse(document.str()));
    fixture.family = row.family;
    fixture.fuelProfile = row.fuelProfile;
    fixture.seed = seed;
    if (!fixture.config.roadCells.empty()) {
        throw std::runtime_error("full-match exact domain unexpectedly contains roads");
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

[[nodiscard]] std::uint64_t search_key(
    std::int32_t steps,
    std::int32_t fuelUsed,
    udon::CellId cell,
    std::uint32_t spotMask) {
    return static_cast<std::uint64_t>(steps) |
        (static_cast<std::uint64_t>(fuelUsed) << 7U) |
        (static_cast<std::uint64_t>(cell) << 19U) |
        (static_cast<std::uint64_t>(spotMask) << 25U);
}

[[nodiscard]] std::vector<DayOutcome> enumerate_day(
    const udon::MatchConfig& config,
    std::int32_t day,
    udon::CellId start,
    std::int32_t availableFuel,
    bool verifyAllOutcomes) {
    struct Node {
        std::int32_t steps = 0;
        std::int32_t fuelUsed = 0;
        udon::CellId cell = udon::kInvalidCell;
        std::uint32_t spotMask = 0;
        std::int32_t parent = -1;
        udon::PlanAction incoming = udon::PlanAction::wait(1);
    };
    const std::int32_t limit = config.steps_for_day(day);
    std::vector<Node> nodes;
    nodes.push_back(Node{0, 0, start, 0U, -1, udon::PlanAction::wait(1)});
    std::unordered_set<std::uint64_t> seen;
    seen.insert(search_key(0, 0, start, 0U));
    const auto push = [&nodes, &seen](const Node& node) {
        if (seen.insert(search_key(
                node.steps,
                node.fuelUsed,
                node.cell,
                node.spotMask)).second) {
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
        const udon::MoveCost cost = config.move_cost(
            current.cell,
            udon::RoadStatus::Smooth);
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

    using OutcomeKey = std::tuple<udon::CellId, std::int32_t, udon::BrandMask>;
    struct OutcomeWitness {
        std::int32_t servings = 0;
        std::int32_t node = -1;
    };
    std::map<OutcomeKey, OutcomeWitness> compressed;
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const Node& node = nodes.at(index);
        udon::BrandMask brands;
        for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
            if ((node.spotMask &
                 (std::uint32_t{1} << static_cast<std::uint32_t>(spot))) != 0U) {
                brands |= udon::brand_bit(config.spots.at(spot).brandIndex);
            }
        }
        const OutcomeKey key{
            node.cell,
            availableFuel - node.fuelUsed,
            brands,
        };
        const std::int32_t servings =
            static_cast<std::int32_t>(std::popcount(node.spotMask));
        const auto existing = compressed.find(key);
        if (existing == compressed.end() ||
            existing->second.servings < servings ||
            (existing->second.servings == servings &&
             nodes.at(static_cast<std::size_t>(existing->second.node)).steps > node.steps)) {
            compressed[key] = OutcomeWitness{
                servings,
                static_cast<std::int32_t>(index),
            };
        }
    }

    std::vector<DayOutcome> outcomes;
    outcomes.reserve(compressed.size());
    for (const auto& [key, witness] : compressed) {
        const Node& terminal = nodes.at(static_cast<std::size_t>(witness.node));
        udon::AgentPlan reversed;
        std::int32_t current = witness.node;
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
            witness.servings,
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
            const bool brandSuperset =
                (other.brands | candidate.brands) == other.brands;
            if (brandSuperset && other.fuel >= candidate.fuel &&
                other.servings >= candidate.servings &&
                (other.brands != candidate.brands || other.fuel > candidate.fuel ||
                 other.servings > candidate.servings)) {
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
    if (verifyAllOutcomes) {
        std::vector<udon::AgentState> agents{
            udon::AgentState{udon::AgentKind::Patrol, start, availableFuel},
            udon::AgentState{
                udon::AgentKind::Patrol,
                config.initialAgents.at(1),
                config.fuelLimit,
            },
            udon::AgentState{
                udon::AgentKind::Patrol,
                config.initialAgents.at(2),
                config.fuelLimit,
            },
        };
        const udon::DayState state = day_state(config, day, agents);
        for (const DayOutcome& outcome : frontier) {
            udon::DayPlan plan;
            plan.actions.push_back(outcome.actions);
            const std::int32_t steps = config.steps_for_day(day);
            plan.actions.push_back(udon::AgentPlan{udon::PlanAction::wait(steps)});
            plan.actions.push_back(udon::AgentPlan{udon::PlanAction::wait(steps)});
            udon::SimulationResult simulation;
            std::string mismatch;
            if (!validates(config, state, plan, simulation, mismatch) ||
                simulation.finalAgents.at(0).position != outcome.position ||
                simulation.finalAgents.at(0).fuel != outcome.fuel ||
                simulation.score.brands != outcome.brands ||
                simulation.score.dailyDistinct !=
                    udon::brand_count(outcome.brands) ||
                simulation.score.servings != outcome.servings) {
                throw std::runtime_error(
                    "enumerated day outcome disagrees with independent engines: " + mismatch);
            }
        }
    }
    return frontier;
}

[[nodiscard]] bool value_better(
    const MatchValue& left,
    const MatchValue& right) {
    if (left.totalDailyDistinct != right.totalDailyDistinct) {
        return left.totalDailyDistinct > right.totalDailyDistinct;
    }
    return left.totalServings > right.totalServings;
}

[[nodiscard]] MatchResult solve_oracle(
    const Fixture& fixture,
    bool verifyAllOutcomes) {
    std::map<MatchKey, MatchValue> frontier;
    frontier.emplace(
        MatchKey{fixture.config.initialAgents.at(0), fixture.config.fuelLimit, 0U},
        MatchValue{});
    std::map<std::tuple<std::int32_t, udon::CellId, std::int32_t>, std::vector<DayOutcome>> cache;
    MatchResult result;
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        std::map<MatchKey, MatchValue> next;
        for (const auto& [key, value] : frontier) {
            const udon::CellId position = std::get<0>(key);
            const std::int32_t fuel = std::get<1>(key);
            const udon::BrandMask lifetime = std::get<2>(key);
            const auto cacheKey = std::tuple{day, position, fuel};
            auto found = cache.find(cacheKey);
            if (found == cache.end()) {
                std::vector<DayOutcome> outcomes = enumerate_day(
                    fixture.config,
                    day,
                    position,
                    fuel,
                    verifyAllOutcomes);
                if (verifyAllOutcomes) {
                    result.verifiedOutcomes += outcomes.size();
                }
                found = cache.emplace(cacheKey, std::move(outcomes)).first;
                ++result.dailyEnumerations;
            }
            for (const DayOutcome& outcome : found->second) {
                const MatchKey nextKey{
                    outcome.position,
                    outcome.fuel,
                    lifetime | outcome.brands,
                };
                MatchValue candidate = value;
                candidate.totalDailyDistinct +=
                    udon::brand_count(outcome.brands);
                candidate.totalServings += outcome.servings;
                candidate.activePlans.push_back(outcome.actions);
                const auto incumbent = next.find(nextKey);
                if (incumbent == next.end() || value_better(candidate, incumbent->second)) {
                    next[nextKey] = std::move(candidate);
                }
            }
        }
        frontier = std::move(next);
        result.maximumFrontier = std::max(result.maximumFrontier, frontier.size());
        if (frontier.empty()) {
            return result;
        }
    }

    auto best = frontier.begin();
    udon::OfficialScore bestScore{
        udon::brand_count(std::get<2>(best->first)),
        best->second.totalDailyDistinct,
        best->second.totalServings,
    };
    for (auto candidate = std::next(frontier.begin()); candidate != frontier.end(); ++candidate) {
        const udon::OfficialScore score{
            udon::brand_count(std::get<2>(candidate->first)),
            candidate->second.totalDailyDistinct,
            candidate->second.totalServings,
        };
        if (bestScore < score) {
            best = candidate;
            bestScore = score;
        }
    }

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
        udon::DayPlan plan;
        plan.actions.push_back(best->second.activePlans.at(static_cast<std::size_t>(day - 1)));
        const std::int32_t steps = fixture.config.steps_for_day(day);
        plan.actions.push_back(udon::AgentPlan{udon::PlanAction::wait(steps)});
        plan.actions.push_back(udon::AgentPlan{udon::PlanAction::wait(steps)});
        const udon::DayState state = day_state(fixture.config, day, agents);
        udon::SimulationResult simulation;
        std::string mismatch;
        if (!validates(fixture.config, state, plan, simulation, mismatch)) {
            throw std::runtime_error("oracle witness validation failed: " + mismatch);
        }
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
        result.activePositions.push_back(agents.at(0).position);
        result.activeFuels.push_back(agents.at(0).fuel);
        result.cumulative.push_back(udon::OfficialScore{
            ledger.lifetime_distinct(),
            ledger.totalDailyDistinct,
            ledger.totalServings,
        });
        result.plans.push_back(std::move(plan));
    }
    result.score = result.cumulative.back();
    if (!(result.score == bestScore)) {
        throw std::runtime_error("full-match DP score disagrees with exact simulation");
    }
    result.valid = true;
    return result;
}

[[nodiscard]] MatchResult solve_head(const Fixture& fixture) {
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
    MatchResult result;
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
        std::ostringstream audit;
        audit << "d" << day << '{';
        for (std::size_t scenario = 0; scenario < decision.manifest.scenarios.size(); ++scenario) {
            if (scenario != 0U) {
                audit << '|';
            }
            const udon::TrafficScenario& item = decision.manifest.scenarios.at(scenario);
            audit << item.scenarioClass << ':' << item.weight << ':'
                  << item.pessimisticFallback;
        }
        audit << "}[";
        for (std::size_t index = 0; index < decision.audit.candidates.size(); ++index) {
            if (index != 0U) {
                audit << '|';
            }
            const udon::CandidateAuditRecord& record =
                decision.audit.candidates.at(index);
            audit << (record.selected ? '*' : '-')
                  << record.scoreAfterToday.lifetimeDistinct << '/'
                  << record.scoreAfterToday.totalDailyDistinct << '/'
                  << record.scoreAfterToday.totalServings << ':'
                  << record.provisionalLowerBound.lifetimeDistinct << '/'
                  << record.provisionalLowerBound.totalDailyDistinct << '/'
                  << record.provisionalLowerBound.totalServings << ':'
                  << record.validUpperBound.lifetimeDistinct << '/'
                  << record.validUpperBound.totalDailyDistinct << '/'
                  << record.validUpperBound.totalServings << ':'
                  << record.finalCertifiedLowerBound.lifetimeDistinct << '/'
                  << record.finalCertifiedLowerBound.totalDailyDistinct << '/'
                  << record.finalCertifiedLowerBound.totalServings << ':';
            if (!record.terminalCells.empty() && !record.terminalFuel.empty()) {
                audit << record.terminalCells.front() << '@'
                      << record.terminalFuel.front();
            } else {
                audit << "na";
            }
            audit << ':' << record.disposition;
        }
        audit << ']';
        result.candidateAudits.push_back(audit.str());
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
        result.activePositions.push_back(agents.at(0).position);
        result.activeFuels.push_back(agents.at(0).fuel);
        result.cumulative.push_back(udon::OfficialScore{
            ledger.lifetime_distinct(),
            ledger.totalDailyDistinct,
            ledger.totalServings,
        });
        result.plans.push_back(decision.candidate.plan);
    }
    result.score = result.cumulative.back();
    result.valid = true;
    return result;
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

[[nodiscard]] std::string plan_wire(const udon::AgentPlan& plan) {
    std::ostringstream output;
    for (std::size_t action = 0U; action < plan.size(); ++action) {
        if (action != 0U) {
            output << '.';
        }
        output << plan.at(action).wire_value();
    }
    return output.str();
}

struct WideRolloutResult {
    bool valid = false;
    udon::OfficialScore score;
    std::string trace;
};

[[nodiscard]] WideRolloutResult run_wide_rollout(
    const Fixture& fixture,
    const udon::MasterCandidate& first,
    const udon::RouteColumnGenerator& generator,
    const udon::RouteMaster& master,
    const udon::FastViabilityAnalyzer& viabilityAnalyzer,
    bool upperGuided) {
    std::vector<udon::AgentState> agents = first.simulation.finalAgents;
    udon::MatchLedger ledger;
    ledger.apply(first.simulation.score);
    WideRolloutResult result;
    std::ostringstream trace;
    trace << first.scoreAfterToday.lifetimeDistinct << '/'
          << first.scoreAfterToday.totalDailyDistinct << '/'
          << first.scoreAfterToday.totalServings << '@'
          << agents.front().position << ':' << agents.front().fuel;
    for (std::int32_t day = 2; day <= fixture.config.day_count(); ++day) {
        const udon::DayState state = day_state(fixture.config, day, agents);
        udon::ColumnGenerationOptions generationOptions;
        generationOptions.maximumPathsPerTarget = 1;
        generationOptions.maximumColumnsPerAgent = 16;
        generationOptions.enableHarvestExtensions = kFutureHarvestMode > 0;
        generationOptions.allowUncachedHarvestTargets = kFutureHarvestMode > 1;
        generationOptions.enableHarvestOrienteering =
            kFutureHarvestMode > 5 &&
            static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                3LL * fixture.config.steps_for_day(day);
        generationOptions.enableExactHarvestOrienteering =
            kFutureHarvestMode > 5 &&
            static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                2LL * fixture.config.steps_for_day(day) &&
            (kFutureHarvestMode > 6 || day == fixture.config.day_count());
        generationOptions.maximumHarvestExtensionSources =
            kFutureHarvestMode > 2 ? 4 : 1;
        generationOptions.maximumHarvestExtensionDepth =
            kFutureHarvestMode > 4 &&
                static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                    3LL * fixture.config.steps_for_day(day)
            ? 4
            : kFutureHarvestMode > 3 ? 3 : 2;
        const bool detailed = day <= std::min(3, fixture.config.day_count());
        udon::MasterOptions masterOptions;
        masterOptions.maximumCandidates = upperGuided ? 32 : 1;
        masterOptions.maximumResolveRounds = 1;
        if (detailed) {
            generationOptions.maximumTargetSpots = 6;
            generationOptions.maximumEscorts = 4;
            masterOptions.maximumCombinations = 100;
        } else {
            const udon::ViabilityBounds viability = viabilityAnalyzer.analyze(
                state,
                ledger);
            generationOptions.maximumTargetSpots = 4;
            generationOptions.maximumEscorts = 2;
            generationOptions.maximumSeedPlans = 1;
            generationOptions.mandatoryReservations = viability.reservations;
            masterOptions.maximumCombinations = 25;
            masterOptions.mandatoryReservations = viability.reservations;
        }
        const udon::RoutePortfolio portfolio = generator.generate(
            state,
            ledger,
            generationOptions);
        udon::MasterDiagnostics diagnostics;
        std::vector<udon::MasterCandidate> choices = master.solve(
            state,
            ledger,
            portfolio,
            masterOptions,
            diagnostics);
        if (choices.empty()) {
            return result;
        }
        std::size_t selectedIndex = 0U;
        if (upperGuided) {
            udon::OfficialScore selectedUpper;
            bool hasSelected = false;
            for (std::size_t choiceIndex = 0U;
                 choiceIndex < choices.size();
                 ++choiceIndex) {
                const udon::MasterCandidate& choice = choices.at(choiceIndex);
                udon::MatchLedger trialLedger = ledger;
                trialLedger.apply(choice.simulation.score);
                const udon::OfficialScore trialScore{
                    trialLedger.lifetime_distinct(),
                    trialLedger.totalDailyDistinct,
                    trialLedger.totalServings,
                };
                udon::OfficialScore trialUpper = trialScore;
                if (day < fixture.config.day_count()) {
                    const udon::DayState nextState = day_state(
                        fixture.config,
                        day + 1,
                        choice.simulation.finalAgents);
                    trialUpper = viabilityAnalyzer.analyze(
                        nextState,
                        trialLedger).upperBound;
                }
                bool better = !hasSelected;
                if (hasSelected) {
                    const std::int32_t upperOrder = udon::compare_lexicographic(
                        trialUpper,
                        selectedUpper);
                    const std::int32_t scoreOrder = udon::compare_lexicographic(
                        choice.scoreAfterToday,
                        choices.at(selectedIndex).scoreAfterToday);
                    const std::int32_t slackOrder = udon::compare_terminal_slack(
                        choice.terminalSlack,
                        choices.at(selectedIndex).terminalSlack);
                    better = upperOrder > 0 ||
                        (upperOrder == 0 && scoreOrder > 0) ||
                        (upperOrder == 0 && scoreOrder == 0 && slackOrder > 0) ||
                        (upperOrder == 0 && scoreOrder == 0 && slackOrder == 0 &&
                         choice.stableId < choices.at(selectedIndex).stableId);
                }
                if (better) {
                    selectedIndex = choiceIndex;
                    selectedUpper = trialUpper;
                    hasSelected = true;
                }
            }
        }
        const udon::MasterCandidate& selected = choices.at(selectedIndex);
        udon::SimulationResult simulation;
        std::string mismatch;
        if (!validates(fixture.config, state, selected.plan, simulation, mismatch)) {
            return result;
        }
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
        const udon::OfficialScore score{
            ledger.lifetime_distinct(),
            ledger.totalDailyDistinct,
            ledger.totalServings,
        };
        trace << ';' << score.lifetimeDistinct << '/'
              << score.totalDailyDistinct << '/'
              << score.totalServings << '@'
              << agents.front().position << ':' << agents.front().fuel;
    }
    result.valid = true;
    result.score = udon::OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
    result.trace = trace.str();
    return result;
}

void attribute_oracle_witness(
    const Fixture& fixture,
    const MatchResult& oracle) {
    const udon::ExactStepSimulator simulator(fixture.config);
    const udon::IndependentDayValidator validator(fixture.config);
    const udon::ParetoRouter router(fixture.config);
    const udon::RouteColumnGenerator generator(fixture.config, router);
    const udon::RouteMaster master(fixture.config, simulator, validator);
    const udon::FutureWitnessRepairer repairer(
        fixture.config,
        generator,
        master,
        simulator,
        validator,
        kFutureHarvestMode);
    const udon::FastViabilityAnalyzer viabilityAnalyzer(fixture.config);
    const udon::ScenarioGenerator scenarioGenerator(fixture.config);

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
    const std::optional<udon::MasterCandidate> exactFirst = master.evaluate_exact_plan(
        firstState,
        ledger,
        oracle.plans.front());
    if (!exactFirst.has_value()) {
        std::cout << "attribute,seed=" << fixture.seed
                  << ",status=oracle-day1-not-dual-valid\n";
        return;
    }
    udon::TrafficBelief belief(fixture.config);
    belief.observe(firstState);
    const udon::ScenarioManifest manifest = scenarioGenerator.freeze_manifest(
        firstState,
        belief);
    udon::MatchLedger afterFirstLedger = ledger;
    afterFirstLedger.apply(exactFirst->simulation.score);
    udon::OfficialScore validUpper = exactFirst->scoreAfterToday;
    if (fixture.config.day_count() > 1) {
        const udon::DayState afterFirstState = day_state(
            fixture.config,
            2,
            exactFirst->simulation.finalAgents);
        validUpper = viabilityAnalyzer.analyze(
            afterFirstState,
            afterFirstLedger).upperBound;
    }
    const auto structuralDeadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(60);
    udon::CandidateProfile profile = repairer.provisional_profile(
        *exactFirst,
        firstState,
        ledger,
        belief,
        manifest,
        validUpper,
        24,
        structuralDeadline);
    repairer.repair_profile(
        profile,
        *exactFirst,
        firstState,
        ledger,
        belief,
        manifest,
        200,
        structuralDeadline);
    const udon::ScenarioOutcome& outcome = profile.outcomes.front();
    std::cout << "attribute,seed=" << fixture.seed
              << ",oracle_day1=" << exactFirst->scoreAfterToday.lifetimeDistinct << '/'
              << exactFirst->scoreAfterToday.totalDailyDistinct << '/'
              << exactFirst->scoreAfterToday.totalServings
              << ",w1_final=" << outcome.score.lifetimeDistinct << '/'
              << outcome.score.totalDailyDistinct << '/'
              << outcome.score.totalServings
              << ",oracle_final=" << oracle.score.lifetimeDistinct << '/'
              << oracle.score.totalDailyDistinct << '/'
              << oracle.score.totalServings
              << ",certified=" << outcome.witness.certified
              << ",future_plans=" << outcome.witness.futurePlans.size()
              << '\n';
    const WideRolloutResult wideGreedy = run_wide_rollout(
        fixture,
        *exactFirst,
        generator,
        master,
        viabilityAnalyzer,
        false);
    const WideRolloutResult wideGuided = run_wide_rollout(
        fixture,
        *exactFirst,
        generator,
        master,
        viabilityAnalyzer,
        true);
    std::cout << "attribute_wide,seed=" << fixture.seed
              << ",baseline=" << outcome.score.lifetimeDistinct << '/'
              << outcome.score.totalDailyDistinct << '/'
              << outcome.score.totalServings
              << ",greedy_valid=" << wideGreedy.valid
              << ",greedy=" << wideGreedy.score.lifetimeDistinct << '/'
              << wideGreedy.score.totalDailyDistinct << '/'
              << wideGreedy.score.totalServings
              << ",guided_valid=" << wideGuided.valid
              << ",guided=" << wideGuided.score.lifetimeDistinct << '/'
              << wideGuided.score.totalDailyDistinct << '/'
              << wideGuided.score.totalServings
              << ",oracle=" << oracle.score.lifetimeDistinct << '/'
              << oracle.score.totalDailyDistinct << '/'
              << oracle.score.totalServings
              << ",greedy_trace=" << wideGreedy.trace
              << ",guided_trace=" << wideGuided.trace
              << '\n';

    agents = exactFirst->simulation.finalAgents;
    ledger = afterFirstLedger;
    for (std::int32_t day = 2; day <= fixture.config.day_count(); ++day) {
        const udon::DayState state = day_state(fixture.config, day, agents);
        const udon::DayPlan& oraclePlan = oracle.plans.at(
            static_cast<std::size_t>(day - 1));
        const std::optional<udon::MasterCandidate> exact = master.evaluate_exact_plan(
            state,
            ledger,
            oraclePlan);

        udon::ColumnGenerationOptions generationOptions;
        generationOptions.maximumPathsPerTarget = 1;
        generationOptions.enableHarvestExtensions = kFutureHarvestMode > 0;
        generationOptions.allowUncachedHarvestTargets = kFutureHarvestMode > 1;
        generationOptions.enableHarvestOrienteering =
            kFutureHarvestMode > 5 &&
            static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                3LL * fixture.config.steps_for_day(day);
        generationOptions.enableExactHarvestOrienteering =
            kFutureHarvestMode > 5 &&
            static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                2LL * fixture.config.steps_for_day(day) &&
            (kFutureHarvestMode > 6 || day == fixture.config.day_count());
        generationOptions.maximumHarvestExtensionSources =
            kFutureHarvestMode > 2 ? 4 : 1;
        generationOptions.maximumHarvestExtensionDepth =
            kFutureHarvestMode > 4 &&
                static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                    3LL * fixture.config.steps_for_day(day)
            ? 4
            : kFutureHarvestMode > 3 ? 3 : 2;
        generationOptions.deadline = structuralDeadline;
        const bool detailed = day <= std::min(3, fixture.config.day_count());
        udon::MasterOptions masterOptions;
        masterOptions.maximumResolveRounds = 1;
        masterOptions.deadline = structuralDeadline;
        if (detailed) {
            generationOptions.maximumColumnsPerAgent = 3;
            generationOptions.maximumTargetSpots = 6;
            generationOptions.maximumEscorts = 4;
            masterOptions.maximumCombinations = 100;
        } else {
            const udon::ViabilityBounds viability = viabilityAnalyzer.analyze(
                state,
                ledger);
            generationOptions.maximumColumnsPerAgent = 2;
            generationOptions.maximumTargetSpots = 4;
            generationOptions.maximumEscorts = 2;
            generationOptions.maximumSeedPlans = 1;
            generationOptions.mandatoryReservations = viability.reservations;
            masterOptions.maximumCombinations = 25;
            masterOptions.mandatoryReservations = viability.reservations;
        }
        const udon::RoutePortfolio portfolio = generator.generate(
            state,
            ledger,
            generationOptions);
        udon::ColumnGenerationOptions wideColumnOptions = generationOptions;
        wideColumnOptions.maximumColumnsPerAgent = 32;
        const udon::RoutePortfolio wideColumnPortfolio = generator.generate(
            state,
            ledger,
            wideColumnOptions);
        udon::ColumnGenerationOptions widePathOptions = wideColumnOptions;
        widePathOptions.maximumPathsPerTarget = 4;
        const udon::RoutePortfolio widePathPortfolio = generator.generate(
            state,
            ledger,
            widePathOptions);
        const auto activePlanPresent = [&oraclePlan](const udon::RoutePortfolio& candidatePortfolio) {
            return std::any_of(
                candidatePortfolio.columnsByAgent.front().begin(),
                candidatePortfolio.columnsByAgent.front().end(),
                [&oraclePlan](const udon::RouteColumn& column) {
                    return same_agent_plan(
                        column.actions,
                        oraclePlan.actions.front());
                });
        };
        const bool activeWideColumns = activePlanPresent(wideColumnPortfolio);
        const bool activeWidePaths = activePlanPresent(widePathPortfolio);
        std::int32_t firstActiveColumnCap = -1;
        for (const std::int32_t columnCap : {4, 6, 8, 10, 12, 16, 24, 32}) {
            udon::ColumnGenerationOptions capOptions = generationOptions;
            capOptions.maximumColumnsPerAgent = columnCap;
            const udon::RoutePortfolio capPortfolio = generator.generate(
                state,
                ledger,
                capOptions);
            if (activePlanPresent(capPortfolio)) {
                firstActiveColumnCap = columnCap;
                break;
            }
        }
        std::int32_t portfolioAgents = 0;
        std::string portfolioMask;
        for (std::size_t agent = 0U; agent < oraclePlan.actions.size(); ++agent) {
            const bool present = std::any_of(
                portfolio.columnsByAgent.at(agent).begin(),
                portfolio.columnsByAgent.at(agent).end(),
                [&oraclePlan, agent](const udon::RouteColumn& column) {
                    return same_agent_plan(
                        column.actions,
                        oraclePlan.actions.at(agent));
                });
            portfolioAgents += present ? 1 : 0;
            portfolioMask += present ? '1' : '0';
        }
        udon::MasterDiagnostics oneDiagnostics;
        masterOptions.maximumCandidates = 1;
        const std::vector<udon::MasterCandidate> one = master.solve(
            state,
            ledger,
            portfolio,
            masterOptions,
            oneDiagnostics);
        udon::MasterDiagnostics broadDiagnostics;
        masterOptions.maximumCandidates = 32;
        const std::vector<udon::MasterCandidate> broad = master.solve(
            state,
            ledger,
            portfolio,
            masterOptions,
            broadDiagnostics);
        const bool retainedOne = exact.has_value() && std::any_of(
            one.begin(),
            one.end(),
            [&exact](const udon::MasterCandidate& candidate) {
                return candidate.stableId == exact->stableId;
            });
        const bool retainedBroad = exact.has_value() && std::any_of(
            broad.begin(),
            broad.end(),
            [&exact](const udon::MasterCandidate& candidate) {
                return candidate.stableId == exact->stableId;
            });
        const auto sameOutcome = [&exact](const udon::MasterCandidate& candidate) {
            if (!exact.has_value() ||
                !(candidate.scoreAfterToday == exact->scoreAfterToday) ||
                candidate.simulation.finalAgents.size() !=
                    exact->simulation.finalAgents.size()) {
                return false;
            }
            for (std::size_t agent = 0U;
                 agent < candidate.simulation.finalAgents.size();
                 ++agent) {
                const udon::AgentState& left =
                    candidate.simulation.finalAgents.at(agent);
                const udon::AgentState& right =
                    exact->simulation.finalAgents.at(agent);
                if (left.kind != right.kind || left.position != right.position ||
                    left.fuel != right.fuel) {
                    return false;
                }
            }
            return true;
        };
        const bool outcomeOne = std::any_of(one.begin(), one.end(), sameOutcome);
        const bool outcomeBroad = std::any_of(broad.begin(), broad.end(), sameOutcome);
        std::cout << "attribute_day,seed=" << fixture.seed
                  << ",day=" << day
                  << ",portfolio_agents=" << portfolioAgents << '/'
                  << oraclePlan.actions.size()
                  << ",portfolio_mask=" << portfolioMask
                  << ",active_wide32=" << activeWideColumns
                  << ",active_paths4=" << activeWidePaths
                  << ",first_active_cap=" << firstActiveColumnCap
                  << ",exact_valid=" << exact.has_value()
                  << ",retained_1=" << retainedOne
                  << ",retained_32=" << retainedBroad
                  << ",outcome_1=" << outcomeOne
                  << ",outcome_32=" << outcomeBroad
                  << ",master_1_count=" << one.size()
                  << ",master_32_count=" << broad.size();
        if (exact.has_value()) {
            std::cout << ",exact=" << exact->scoreAfterToday.lifetimeDistinct << '/'
                      << exact->scoreAfterToday.totalDailyDistinct << '/'
                      << exact->scoreAfterToday.totalServings
                      << ",exact_terminal=" << exact->simulation.finalAgents.front().position
                      << '@' << exact->simulation.finalAgents.front().fuel;
        }
        if (!one.empty()) {
            std::cout << ",front=" << one.front().scoreAfterToday.lifetimeDistinct << '/'
                      << one.front().scoreAfterToday.totalDailyDistinct << '/'
                      << one.front().scoreAfterToday.totalServings
                      << ",front_terminal=" << one.front().simulation.finalAgents.front().position
                      << '@' << one.front().simulation.finalAgents.front().fuel;
        }
        std::cout << '\n';
        if (!portfolioMask.empty() && portfolioMask.front() == '0') {
            std::cout << "attribute_routes,seed=" << fixture.seed
                      << ",day=" << day
                      << ",oracle=" << plan_wire(oraclePlan.actions.front());
            for (const udon::RouteColumn& column : portfolio.columnsByAgent.front()) {
                std::cout << ",column=" << column.columnId << ':'
                          << plan_wire(column.actions) << ':'
                          << column.terminalCell << '@' << column.terminalFuel << ':'
                          << column.estimatedBrands << '/' << column.estimatedServings << ':'
                          << column.harvestExtension << ':'
                          << column.exactOrienteering << ':'
                          << column.priority;
            }
            std::cout << '\n';
        }

        udon::SimulationResult simulation;
        std::string mismatch;
        if (!validates(fixture.config, state, oraclePlan, simulation, mismatch)) {
            throw std::runtime_error("oracle attribution replay failed: " + mismatch);
        }
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
    }
}

void attribute_route_closure(
    const Fixture& fixture,
    const MatchResult& oracle) {
    const udon::ExactStepSimulator simulator(fixture.config);
    const udon::IndependentDayValidator validator(fixture.config);
    const udon::ParetoRouter router(fixture.config);
    const udon::RouteColumnGenerator generator(fixture.config, router);
    const udon::RouteMaster master(fixture.config, simulator, validator);
    const udon::FastViabilityAnalyzer viabilityAnalyzer(fixture.config);

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
        const udon::DayPlan& oraclePlan = oracle.plans.at(
            static_cast<std::size_t>(day - 1));
        const std::optional<udon::MasterCandidate> exact = master.evaluate_exact_plan(
            state,
            ledger,
            oraclePlan);
        if (!exact.has_value()) {
            std::cout << "route_closure,seed=" << fixture.seed
                      << ",family=" << fixture.family
                      << ",fuel=" << fixture.fuelProfile
                      << ",day=" << day
                      << ",status=oracle-not-dual-valid\n";
            return;
        }

        udon::ColumnGenerationOptions base;
        base.maximumPathsPerTarget = 1;
        base.enableHarvestExtensions = kFutureHarvestMode > 0;
        base.allowUncachedHarvestTargets = kFutureHarvestMode > 1;
        base.enableHarvestOrienteering =
            kFutureHarvestMode > 5 &&
            static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                3LL * fixture.config.steps_for_day(day);
        base.enableExactHarvestOrienteering =
            kFutureHarvestMode > 5 &&
            static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                2LL * fixture.config.steps_for_day(day) &&
            (kFutureHarvestMode > 6 || day == fixture.config.day_count());
        base.maximumHarvestExtensionSources =
            kFutureHarvestMode > 2 ? 4 : 1;
        base.maximumHarvestExtensionDepth =
            kFutureHarvestMode > 4 &&
                static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                    3LL * fixture.config.steps_for_day(day)
            ? 4
            : kFutureHarvestMode > 3 ? 3 : 2;
        const bool detailed = day <= std::min(3, fixture.config.day_count());
        if (detailed) {
            base.maximumColumnsPerAgent = 3;
            base.maximumTargetSpots = 6;
            base.maximumEscorts = 4;
        } else {
            const udon::ViabilityBounds viability = viabilityAnalyzer.analyze(
                state,
                ledger);
            base.maximumColumnsPerAgent = 2;
            base.maximumTargetSpots = 4;
            base.maximumEscorts = 2;
            base.maximumSeedPlans = 1;
            base.mandatoryReservations = viability.reservations;
        }

        struct CapabilityStage {
            std::string name;
            udon::ColumnGenerationOptions options;
        };
        std::vector<CapabilityStage> stages;
        stages.push_back(CapabilityStage{"w1", base});

        udon::ColumnGenerationOptions cap16 = base;
        cap16.maximumColumnsPerAgent = 16;
        stages.push_back(CapabilityStage{"cap16", cap16});

        udon::ColumnGenerationOptions cap32Paths4 = cap16;
        cap32Paths4.maximumColumnsPerAgent = 32;
        cap32Paths4.maximumPathsPerTarget = 4;
        stages.push_back(CapabilityStage{"cap32-paths4", cap32Paths4});

        udon::ColumnGenerationOptions cap128Paths8 = cap32Paths4;
        cap128Paths8.maximumColumnsPerAgent = 128;
        cap128Paths8.maximumPathsPerTarget = 8;
        stages.push_back(CapabilityStage{"cap128-paths8", cap128Paths8});

        udon::ColumnGenerationOptions harvest = cap128Paths8;
        harvest.enableHarvestExtensions = true;
        harvest.allowUncachedHarvestTargets = true;
        harvest.enableHarvestOrienteering = true;
        harvest.enableExactHarvestOrienteering = false;
        harvest.enableFuelConstrainedExactHarvestOrienteering = false;
        harvest.enableAnytimeFuelConstrainedHarvestOrienteering = false;
        harvest.maximumHarvestExtensionSources = static_cast<std::int32_t>(
            fixture.config.spots.size());
        harvest.maximumHarvestExtensionDepth = static_cast<std::int32_t>(
            fixture.config.spots.size());
        stages.push_back(CapabilityStage{"harvest-orienteering", harvest});

        udon::ColumnGenerationOptions exactHighFuel = harvest;
        exactHighFuel.enableExactHarvestOrienteering = true;
        stages.push_back(CapabilityStage{"exact-highfuel", exactHighFuel});

        udon::ColumnGenerationOptions fuelExact = exactHighFuel;
        fuelExact.enableFuelConstrainedExactHarvestOrienteering = true;
        stages.push_back(CapabilityStage{"fuel-exact", fuelExact});

        udon::ColumnGenerationOptions anytime = fuelExact;
        anytime.enableAnytimeFuelConstrainedHarvestOrienteering = true;
        stages.push_back(CapabilityStage{"fuel-anytime", anytime});

        std::string firstStage = "absent";
        bool routeMatched = false;
        bool matchedHarvest = false;
        bool matchedExact = false;
        std::int32_t matchedSourceRank = 0;
        udon::ColumnGenerationDiagnostics matchedDiagnostics;
        std::size_t matchedColumns = 0U;
        for (CapabilityStage& stage : stages) {
            stage.options.deadline = std::chrono::steady_clock::now() +
                std::chrono::seconds(60);
            udon::ColumnGenerationDiagnostics diagnostics;
            const udon::RoutePortfolio portfolio = generator.generate(
                state,
                ledger,
                stage.options,
                &diagnostics);
            const std::vector<udon::RouteColumn>& activeColumns =
                portfolio.columnsByAgent.front();
            const auto iterator = std::find_if(
                activeColumns.begin(),
                activeColumns.end(),
                [&oraclePlan](const udon::RouteColumn& column) {
                    return same_agent_plan(
                        column.actions,
                        oraclePlan.actions.front());
                });
            if (iterator != activeColumns.end()) {
                firstStage = stage.name;
                routeMatched = true;
                matchedHarvest = iterator->harvestExtension;
                matchedExact = iterator->exactOrienteering;
                matchedSourceRank = iterator->harvestExtensionSourceRank;
                matchedDiagnostics = diagnostics;
                matchedColumns = activeColumns.size();
                break;
            }
        }

        std::cout << "route_closure,seed=" << fixture.seed
                  << ",family=" << fixture.family
                  << ",fuel=" << fixture.fuelProfile
                  << ",day=" << day
                  << ",first_stage=" << firstStage
                  << ",oracle=" << plan_wire(oraclePlan.actions.front())
                  << ",daily=" << exact->simulation.score.dailyDistinct << '/'
                  << exact->simulation.score.servings
                  << ",terminal="
                  << exact->simulation.finalAgents.front().position << '@'
                  << exact->simulation.finalAgents.front().fuel;
        if (routeMatched) {
            std::cout << ",columns=" << matchedColumns
                      << ",matched_harvest=" << matchedHarvest
                      << ",matched_exact=" << matchedExact
                      << ",matched_source_rank="
                      << matchedSourceRank
                      << ",exact_complete="
                      << matchedDiagnostics.exactOrienteeringCompleteAgents
                      << ",exact_states="
                      << matchedDiagnostics.exactOrienteeringSettledStates;
        }
        std::cout << '\n';

        udon::SimulationResult validation;
        std::string mismatch;
        if (!validates(
                fixture.config,
                state,
                oraclePlan,
                validation,
                mismatch)) {
            throw std::runtime_error(
                "route-closure oracle replay failed: " + mismatch);
        }
        agents = validation.finalAgents;
        ledger.apply(validation.score);
    }
}

[[nodiscard]] std::pair<std::int32_t, std::int32_t> first_tier_delta(
    const udon::OfficialScore& better,
    const udon::OfficialScore& worse) {
    if (better.lifetimeDistinct != worse.lifetimeDistinct) {
        return {1, better.lifetimeDistinct - worse.lifetimeDistinct};
    }
    if (better.totalDailyDistinct != worse.totalDailyDistinct) {
        return {2, better.totalDailyDistinct - worse.totalDailyDistinct};
    }
    return {3, better.totalServings - worse.totalServings};
}

void print_summary(
    const std::string& label,
    const std::string& split,
    const std::string& family,
    const std::string& fuel,
    const Summary& summary) {
    std::cout << label << ",split=" << split;
    if (!family.empty()) {
        std::cout << ",family=" << family;
    }
    if (!fuel.empty()) {
        std::cout << ",fuel=" << fuel;
    }
    std::cout << ",cases=" << summary.cases
              << ",oracle_wins=" << summary.oracleWins
              << ",ties=" << summary.ties
              << ",head_wins=" << summary.headWins
              << ",invalid=" << summary.invalid
              << ",tier1=" << summary.tier1
              << ",tier2=" << summary.tier2
              << ",tier3=" << summary.tier3
              << ",max_gain=" << summary.maximumGain
              << ",result_hash=" << std::hex << summary.resultHash << std::dec
              << '\n';
}

[[nodiscard]] std::string trace_string(const MatchResult& result) {
    std::ostringstream output;
    for (std::size_t day = 0; day < result.cumulative.size(); ++day) {
        if (day != 0U) {
            output << ';';
        }
        const udon::OfficialScore& score = result.cumulative.at(day);
        output << score.lifetimeDistinct << '/' << score.totalDailyDistinct
               << '/' << score.totalServings << '@'
               << result.activePositions.at(day) << ':' << result.activeFuels.at(day);
    }
    return output.str();
}

[[nodiscard]] std::string audit_string(const MatchResult& result) {
    std::ostringstream output;
    for (std::size_t index = 0; index < result.candidateAudits.size(); ++index) {
        if (index != 0U) {
            output << ';';
        }
        output << result.candidateAudits.at(index);
    }
    return output.str();
}

[[nodiscard]] std::int32_t first_plan_difference(
    const MatchResult& left,
    const MatchResult& right) {
    const std::size_t days = std::min(left.plans.size(), right.plans.size());
    for (std::size_t day = 0; day < days; ++day) {
        const udon::AgentPlan& leftPlan = left.plans.at(day).actions.at(0);
        const udon::AgentPlan& rightPlan = right.plans.at(day).actions.at(0);
        if (leftPlan.size() != rightPlan.size()) {
            return static_cast<std::int32_t>(day + 1U);
        }
        for (std::size_t action = 0; action < leftPlan.size(); ++action) {
            if (leftPlan.at(action).wire_value() != rightPlan.at(action).wire_value()) {
                return static_cast<std::int32_t>(day + 1U);
            }
        }
    }
    return left.plans.size() == right.plans.size()
        ? 0
        : static_cast<std::int32_t>(days + 1U);
}

} // namespace

int main(int argc, char** argv) try {
    const Options options = parse_options(argc, argv);
    const std::vector<ManifestRow> rows = load_manifest(options.manifest, options.split);
    Summary total;
    std::map<std::pair<std::string, std::string>, Summary> strata;
    bool stop = false;
    for (const ManifestRow& row : rows) {
        if (stop) {
            break;
        }
        Summary& stratum = strata[{row.family, row.fuelProfile}];
        for (std::int32_t offset = 0; offset < row.count; ++offset) {
            if (options.maximumMatches > 0 && total.cases >= options.maximumMatches) {
                stop = true;
                break;
            }
            const std::uint64_t seed = row.firstSeed + static_cast<std::uint64_t>(offset);
            if (options.onlySeed != 0U && seed != options.onlySeed) {
                continue;
            }
            ++stratum.cases;
            ++total.cases;
            const Fixture fixture = make_fixture(row, seed);
            const MatchResult oracle = solve_oracle(fixture, options.verifyAllOutcomes);
            const MatchResult head = solve_head(fixture);
            if (!oracle.valid || !head.valid) {
                ++stratum.invalid;
                ++total.invalid;
                std::cout << "match,split=" << options.split
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",seed=" << seed
                          << ",verdict=invalid,oracle_valid=" << oracle.valid
                          << ",head_valid=" << head.valid << '\n';
                continue;
            }
            const std::int32_t comparison =
                udon::compare_lexicographic(oracle.score, head.score);
            std::int32_t tier = 0;
            std::int32_t delta = 0;
            if (comparison > 0) {
                ++stratum.oracleWins;
                ++total.oracleWins;
                std::tie(tier, delta) = first_tier_delta(oracle.score, head.score);
                if (tier == 1) {
                    ++stratum.tier1;
                    ++total.tier1;
                } else if (tier == 2) {
                    ++stratum.tier2;
                    ++total.tier2;
                } else {
                    ++stratum.tier3;
                    ++total.tier3;
                }
                stratum.maximumGain = std::max(stratum.maximumGain, delta);
                total.maximumGain = std::max(total.maximumGain, delta);
            } else if (comparison == 0) {
                ++stratum.ties;
                ++total.ties;
            } else {
                ++stratum.headWins;
                ++total.headWins;
                std::tie(tier, delta) = first_tier_delta(head.score, oracle.score);
            }
            hash_value(stratum.resultHash, seed);
            hash_value(stratum.resultHash, static_cast<std::uint64_t>(comparison + 1));
            hash_value(stratum.resultHash, static_cast<std::uint64_t>(tier));
            hash_value(stratum.resultHash, static_cast<std::uint64_t>(delta));
            hash_value(total.resultHash, seed);
            hash_value(total.resultHash, static_cast<std::uint64_t>(comparison + 1));
            hash_value(total.resultHash, static_cast<std::uint64_t>(tier));
            hash_value(total.resultHash, static_cast<std::uint64_t>(delta));
            if (options.scoresOnly) {
                std::cout << "score,split=" << options.split
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",seed=" << seed
                          << ",valid=1,head="
                          << head.score.lifetimeDistinct << '/'
                          << head.score.totalDailyDistinct << '/'
                          << head.score.totalServings
                          << ",oracle="
                          << oracle.score.lifetimeDistinct << '/'
                          << oracle.score.totalDailyDistinct << '/'
                          << oracle.score.totalServings << '\n';
            } else if (options.details || comparison != 0) {
                std::cout << "match,split=" << options.split
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",seed=" << seed
                          << ",verdict="
                          << (comparison > 0 ? "oracle-win" : comparison < 0 ? "head-win" : "tie")
                          << ",tier=" << tier
                          << ",delta=" << delta
                          << ",head=" << head.score.lifetimeDistinct << '/'
                          << head.score.totalDailyDistinct << '/' << head.score.totalServings
                          << ",oracle=" << oracle.score.lifetimeDistinct << '/'
                          << oracle.score.totalDailyDistinct << '/' << oracle.score.totalServings
                          << ",oracle_frontier=" << oracle.maximumFrontier
                          << ",day_enumerations=" << oracle.dailyEnumerations
                          << ",verified_outcomes=" << oracle.verifiedOutcomes
                          << ",first_plan_diff_day=" << first_plan_difference(head, oracle)
                          << ",head_trace=" << trace_string(head)
                          << ",oracle_trace=" << trace_string(oracle)
                          << ",head_audit=" << audit_string(head)
                          << '\n';
            }
            if (options.attributeWitness && comparison > 0) {
                attribute_oracle_witness(fixture, oracle);
            }
            if (options.attributeRouteClosure) {
                attribute_route_closure(fixture, oracle);
            }
        }
    }
    for (const auto& [key, summary] : strata) {
        print_summary("stratum", options.split, key.first, key.second, summary);
    }
    print_summary("summary", options.split, "", "", total);
    if (total.invalid != 0 || total.headWins != 0) {
        return 2;
    }
    return 0;
} catch (const std::exception& error) {
    std::cerr << "full-match oracle failed: " << error.what() << '\n';
    return 1;
}
