#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
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
#include "udon/orienteering.hpp"
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
    std::uint64_t firstSeed = 0;
    std::int32_t count = 0;
};

struct Options {
    std::string manifest = "research/holdouts/CEILING-ORACLE-070.csv";
    std::string split = "development";
    bool details = false;
};

struct SpotSpec {
    std::int32_t brand = 0;
    udon::CellId position = udon::kInvalidCell;
    std::int32_t stock = 0;
};

struct Fixture {
    udon::MatchConfig config;
    udon::DayState state;
    udon::MatchLedger ledger;
};

struct OracleResult {
    bool complete = false;
    udon::DayPlan plan;
    udon::SimulationResult simulation;
    udon::OfficialScore score;
    std::uint64_t settledStates = 0;
    std::size_t teamStates = 0;
};

struct Summary {
    std::int32_t cases = 0;
    std::int32_t oracleWins = 0;
    std::int32_t ties = 0;
    std::int32_t headWins = 0;
    std::int32_t incomplete = 0;
    std::int32_t invalid = 0;
    std::int32_t tier1Wins = 0;
    std::int32_t tier2Wins = 0;
    std::int32_t tier3Wins = 0;
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
        } else if (argument == "--details") {
            options.details = true;
        } else {
            throw std::invalid_argument("unknown or incomplete option: " + argument);
        }
    }
    if (options.split != "development" && options.split != "holdout") {
        throw std::invalid_argument("split must be development or holdout");
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
        line != "experiment_id,split,family,fuel_profile,first_seed,count,agent_count,spot_count,day_count,terminal_day,role_mode,oracle_scope") {
        throw std::runtime_error("unexpected CEILING-ORACLE-070 manifest schema");
    }
    std::vector<ManifestRow> rows;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::vector<std::string> fields = split_csv(line);
        if (fields.size() != 12U || fields.at(0) != "CEILING-ORACLE-070") {
            throw std::runtime_error("invalid manifest row: " + line);
        }
        if (fields.at(1) != requestedSplit) {
            continue;
        }
        if (fields.at(6) != "4" || fields.at(7) != "8" ||
            fields.at(8) != "5" || fields.at(9) != "5" ||
            fields.at(10) != "all-patrol" ||
            fields.at(11) != "complete-terminal-resource-team-dp") {
            throw std::runtime_error("manifest row violates oracle scope: " + line);
        }
        rows.push_back(ManifestRow{
            fields.at(1),
            fields.at(2),
            fields.at(3),
            std::stoull(fields.at(4)),
            std::stoi(fields.at(5)),
        });
    }
    if (rows.empty()) {
        throw std::runtime_error("manifest contains no rows for split " + requestedSplit);
    }
    return rows;
}

[[nodiscard]] std::vector<std::int32_t> brand_values_for(
    const std::string& family) {
    if (family == "rare-brand") {
        return {100, 100, 200, 200, 300, 300, 400, 999};
    }
    if (family == "fuel-tight") {
        return {100, 100, 200, 300, 400, 500, 600, 700};
    }
    if (family == "overnight") {
        return {100, 100, 200, 200, 300, 400, 500, 900};
    }
    return {100, 200, 300, 400, 500, 600, 700, 800};
}

[[nodiscard]] std::int32_t terrain_for(
    const std::string& family,
    std::int32_t row,
    std::int32_t column,
    std::uint64_t draw) {
    const std::uint64_t bucket = draw % 100U;
    if (family == "threshold-corridor" && (column == 3 || column == 4)) {
        return static_cast<std::int32_t>(udon::Terrain::Road);
    }
    if (family == "fuel-tight") {
        if (bucket < 34U) {
            return static_cast<std::int32_t>(udon::Terrain::Mountain);
        }
        if (bucket < 48U) {
            return static_cast<std::int32_t>(udon::Terrain::Road);
        }
        if (bucket < 53U) {
            return static_cast<std::int32_t>(udon::Terrain::Pond);
        }
        return static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    const std::uint64_t roadCutoff = family == "threshold-corridor" ? 30U : 22U;
    const std::uint64_t mountainCutoff = roadCutoff + 15U;
    const std::uint64_t pondCutoff = mountainCutoff +
        (family == "high-stock" ? 3U : 5U);
    if (bucket < roadCutoff) {
        return static_cast<std::int32_t>(udon::Terrain::Road);
    }
    if (bucket < mountainCutoff) {
        return static_cast<std::int32_t>(udon::Terrain::Mountain);
    }
    if (bucket < pondCutoff && row > 0 && row < 7) {
        return static_cast<std::int32_t>(udon::Terrain::Pond);
    }
    return static_cast<std::int32_t>(udon::Terrain::Plain);
}

[[nodiscard]] Fixture make_fixture(
    const std::string& family,
    const std::string& fuelProfile,
    std::uint64_t seed) {
    constexpr std::int32_t side = 8;
    constexpr std::int32_t cells = side * side;
    const std::vector<udon::CellId> starts{0, 7, 56, 63};
    const std::int32_t daySteps = 16 + static_cast<std::int32_t>(mix64(seed) % 5U);
    std::int32_t fuelLimit = daySteps;
    if (fuelProfile == "low") {
        fuelLimit = std::max(4, (2 * daySteps) / 3);
    } else if (fuelProfile == "high") {
        fuelLimit = 2 * daySteps;
    } else if (fuelProfile != "default") {
        throw std::invalid_argument("unknown fuel profile: " + fuelProfile);
    }

    std::vector<std::int32_t> terrain(static_cast<std::size_t>(cells));
    for (std::int32_t cell = 0; cell < cells; ++cell) {
        terrain.at(static_cast<std::size_t>(cell)) = terrain_for(
            family,
            cell / side,
            cell % side,
            mix64(seed ^ (static_cast<std::uint64_t>(cell) << 18U)));
    }

    std::unordered_set<udon::CellId> reserved(starts.begin(), starts.end());
    std::vector<udon::CellId> spotCells;
    spotCells.reserve(8U);
    std::uint64_t cursor = seed ^ 0x91e10da5c79e7b1dULL;
    while (spotCells.size() < 8U) {
        cursor = mix64(cursor);
        const udon::CellId cell = static_cast<udon::CellId>(cursor % cells);
        if (reserved.insert(cell).second) {
            spotCells.push_back(cell);
        }
    }
    for (const udon::CellId cell : starts) {
        terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    for (const udon::CellId cell : spotCells) {
        terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }

    const std::vector<std::int32_t> brands = brand_values_for(family);
    std::vector<SpotSpec> spots;
    spots.reserve(8U);
    for (std::size_t index = 0; index < 8U; ++index) {
        std::int32_t stock = 1 + static_cast<std::int32_t>(
            mix64(seed ^ (static_cast<std::uint64_t>(index) << 32U)) % 2U);
        if (family == "high-stock") {
            stock = 3 + static_cast<std::int32_t>(
                mix64(seed + index) % 2U);
        }
        if (family == "rare-brand" && index == 7U) {
            stock = 1;
        }
        spots.push_back(SpotSpec{brands.at(index), spotCells.at(index), stock});
    }

    std::ostringstream document;
    document << "{\"startsAt\":1778227200,\"daySeconds\":[5,5,5,5,5],"
             << "\"daySteps\":[" << daySteps << ',' << daySteps << ','
             << daySteps << ',' << daySteps << ',' << daySteps << "],"
             << "\"map\":{\"height\":8,\"width\":8,\"cells\":[";
    for (std::int32_t row = 0; row < side; ++row) {
        if (row != 0) {
            document << ',';
        }
        document << '[';
        for (std::int32_t column = 0; column < side; ++column) {
            if (column != 0) {
                document << ',';
            }
            document << terrain.at(static_cast<std::size_t>(row * side + column));
        }
        document << ']';
    }
    document << "]},\"spots\":[";
    for (std::size_t index = 0; index < spots.size(); ++index) {
        if (index != 0U) {
            document << ',';
        }
        const SpotSpec& spot = spots.at(index);
        document << "{\"brand\":" << spot.brand << ",\"pos\":"
                 << spot.position << ",\"stocks\":" << spot.stock << '}';
    }
    document << "],\"agents\":[0,7,56,63],\"fuelLimits\":" << fuelLimit
             << ",\"players\":4,\"busyThreshold\":2,\"jammedThreshold\":4}";

    Fixture fixture;
    fixture.config = udon::parse_match_config(udon::JsonValue::parse(document.str()));
    fixture.state.endsAt = 1778227225;
    fixture.state.dayNumber = 5;
    fixture.state.roadStatuses.assign(
        static_cast<std::size_t>(fixture.config.map.cell_count()),
        udon::RoadStatus::Smooth);
    for (const udon::CellId road : fixture.config.roadCells) {
        if (family == "threshold-corridor") {
            const std::uint64_t draw = mix64(seed ^ static_cast<std::uint64_t>(road));
            fixture.state.roadStatuses.at(static_cast<std::size_t>(road)) =
                draw % 3U == 0U ? udon::RoadStatus::Jammed : udon::RoadStatus::Busy;
        } else if (mix64(seed + static_cast<std::uint64_t>(road)) % 7U == 0U) {
            fixture.state.roadStatuses.at(static_cast<std::size_t>(road)) =
                udon::RoadStatus::Busy;
        }
    }
    for (std::int32_t agent = 0; agent < fixture.config.agent_count(); ++agent) {
        const udon::CellId position = family == "overnight"
            ? fixture.config.spots.at(static_cast<std::size_t>(agent)).position
            : fixture.config.initialAgents.at(static_cast<std::size_t>(agent));
        fixture.state.agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            position,
            fuelLimit,
        });
    }
    for (std::int32_t brand = 0; brand < fixture.config.brand_count(); ++brand) {
        if (mix64(seed ^ (static_cast<std::uint64_t>(brand) << 44U)) % 4U == 0U) {
            fixture.ledger.lifetimeBrands |= udon::brand_bit(brand);
        }
    }
    fixture.ledger.totalDailyDistinct = 8 + static_cast<std::int32_t>(mix64(seed) % 9U);
    fixture.ledger.totalServings = 20 + static_cast<std::int32_t>(mix64(seed + 1U) % 17U);
    return fixture;
}

[[nodiscard]] std::uint32_t add_route_to_key(
    const udon::MatchConfig& config,
    std::uint32_t key,
    std::uint32_t spotMask) {
    for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
        if ((spotMask & (std::uint32_t{1} << static_cast<std::uint32_t>(spot))) == 0U) {
            continue;
        }
        const std::uint32_t shift = static_cast<std::uint32_t>(spot * 3U);
        const std::uint32_t count = (key >> shift) & 7U;
        const std::uint32_t cap = static_cast<std::uint32_t>(config.spots.at(spot).stock);
        if (count < cap) {
            key += std::uint32_t{1} << shift;
        }
    }
    return key;
}

[[nodiscard]] udon::DayScore score_for_key(
    const udon::MatchConfig& config,
    std::uint32_t key) {
    udon::DayScore score;
    for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
        const std::uint32_t shift = static_cast<std::uint32_t>(spot * 3U);
        const std::int32_t count = static_cast<std::int32_t>((key >> shift) & 7U);
        if (count <= 0) {
            continue;
        }
        score.brands |= udon::brand_bit(config.spots.at(spot).brandIndex);
        score.servings += count;
    }
    score.dailyDistinct = udon::brand_count(score.brands);
    return score;
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

[[nodiscard]] OracleResult solve_oracle(
    const Fixture& fixture) {
    std::vector<std::vector<udon::ExactOrienteeringRoute>> routesByAgent;
    routesByAgent.reserve(static_cast<std::size_t>(fixture.config.agent_count()));
    OracleResult result;
    for (udon::AgentIndex agent = 0; agent < fixture.config.agent_count(); ++agent) {
        udon::ExactOrienteeringReachability reachability =
            udon::enumerate_exact_resource_routes(
                fixture.config,
                fixture.state,
                agent,
                std::nullopt);
        result.settledStates += reachability.settledStates;
        if (!reachability.supported || !reachability.complete ||
            reachability.maximalRoutes.empty()) {
            return result;
        }
        std::unordered_set<std::uint32_t> masks;
        std::vector<udon::ExactOrienteeringRoute> routes;
        for (udon::ExactOrienteeringRoute& route : reachability.maximalRoutes) {
            if (masks.insert(route.spotMask).second) {
                routes.push_back(std::move(route));
            }
        }
        routesByAgent.push_back(std::move(routes));
    }

    struct TeamState {
        std::vector<std::size_t> choices;
    };
    std::unordered_map<std::uint32_t, TeamState> frontier;
    frontier.emplace(0U, TeamState{});
    for (std::size_t agent = 0; agent < routesByAgent.size(); ++agent) {
        std::unordered_map<std::uint32_t, TeamState> next;
        for (const auto& [key, teamState] : frontier) {
            for (std::size_t route = 0; route < routesByAgent.at(agent).size(); ++route) {
                const std::uint32_t nextKey = add_route_to_key(
                    fixture.config,
                    key,
                    routesByAgent.at(agent).at(route).spotMask);
                if (next.contains(nextKey)) {
                    continue;
                }
                TeamState witness = teamState;
                witness.choices.push_back(route);
                next.emplace(nextKey, std::move(witness));
            }
        }
        frontier = std::move(next);
    }
    result.teamStates = frontier.size();
    if (frontier.empty()) {
        return result;
    }

    auto best = frontier.begin();
    udon::DayScore bestDayScore = score_for_key(fixture.config, best->first);
    udon::OfficialScore bestScore =
        udon::OfficialScore::after_day(fixture.ledger, bestDayScore);
    for (auto candidate = std::next(frontier.begin()); candidate != frontier.end(); ++candidate) {
        const udon::DayScore dayScore = score_for_key(fixture.config, candidate->first);
        const udon::OfficialScore score =
            udon::OfficialScore::after_day(fixture.ledger, dayScore);
        if (bestScore < score) {
            best = candidate;
            bestDayScore = dayScore;
            bestScore = score;
        }
    }

    result.plan.actions.resize(routesByAgent.size());
    for (std::size_t agent = 0; agent < routesByAgent.size(); ++agent) {
        result.plan.actions.at(agent) = routesByAgent.at(agent)
            .at(best->second.choices.at(agent)).actions;
    }
    std::string mismatch;
    if (!validates(
            fixture.config,
            fixture.state,
            result.plan,
            result.simulation,
            mismatch)) {
        throw std::runtime_error("oracle witness validation failed: " + mismatch);
    }
    result.score = udon::OfficialScore::after_day(
        fixture.ledger,
        result.simulation.score);
    if (!(result.score == bestScore) || !(result.simulation.score.brands == bestDayScore.brands) ||
        result.simulation.score.servings != bestDayScore.servings) {
        throw std::runtime_error("oracle DP score disagrees with exact simulation");
    }
    result.complete = true;
    return result;
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
              << ",incomplete=" << summary.incomplete
              << ",invalid=" << summary.invalid
              << ",tier1=" << summary.tier1Wins
              << ",tier2=" << summary.tier2Wins
              << ",tier3=" << summary.tier3Wins
              << ",max_gain=" << summary.maximumGain
              << ",result_hash=" << std::hex << summary.resultHash << std::dec
              << '\n';
}

} // namespace

int main(int argc, char** argv) try {
    const Options options = parse_options(argc, argv);
    const std::vector<ManifestRow> rows = load_manifest(options.manifest, options.split);
    Summary total;
    std::map<std::pair<std::string, std::string>, Summary> strata;
    for (const ManifestRow& row : rows) {
        Summary& stratum = strata[{row.family, row.fuelProfile}];
        for (std::int32_t offset = 0; offset < row.count; ++offset) {
            const std::uint64_t seed = row.firstSeed + static_cast<std::uint64_t>(offset);
            ++stratum.cases;
            ++total.cases;
            const Fixture fixture = make_fixture(row.family, row.fuelProfile, seed);
            udon::UdonShieldEngine engine(
                fixture.config,
                {},
                {},
                udon::RoutePoolSearch::SinglePass,
                kHarvestMode,
                false,
                kFutureHarvestMode);
            const udon::DecisionResult decision =
                engine.solve_day(fixture.state, fixture.ledger, kProductionBudget);
            udon::SimulationResult headSimulation;
            std::string mismatch;
            if (!validates(
                    fixture.config,
                    fixture.state,
                    decision.candidate.plan,
                    headSimulation,
                    mismatch)) {
                ++stratum.invalid;
                ++total.invalid;
                std::cout << "case,split=" << options.split
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",seed=" << seed
                          << ",verdict=head-invalid,mismatch=" << mismatch << '\n';
                continue;
            }
            const OracleResult oracle = solve_oracle(fixture);
            if (!oracle.complete) {
                ++stratum.incomplete;
                ++total.incomplete;
                std::cout << "case,split=" << options.split
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",seed=" << seed
                          << ",verdict=oracle-incomplete\n";
                continue;
            }
            const udon::OfficialScore headScore = udon::OfficialScore::after_day(
                fixture.ledger,
                headSimulation.score);
            const std::int32_t comparison =
                udon::compare_lexicographic(oracle.score, headScore);
            std::int32_t tier = 0;
            std::int32_t delta = 0;
            if (comparison > 0) {
                ++stratum.oracleWins;
                ++total.oracleWins;
                std::tie(tier, delta) = first_tier_delta(oracle.score, headScore);
                if (tier == 1) {
                    ++stratum.tier1Wins;
                    ++total.tier1Wins;
                } else if (tier == 2) {
                    ++stratum.tier2Wins;
                    ++total.tier2Wins;
                } else {
                    ++stratum.tier3Wins;
                    ++total.tier3Wins;
                }
                stratum.maximumGain = std::max(stratum.maximumGain, delta);
                total.maximumGain = std::max(total.maximumGain, delta);
            } else if (comparison == 0) {
                ++stratum.ties;
                ++total.ties;
            } else {
                ++stratum.headWins;
                ++total.headWins;
                std::tie(tier, delta) = first_tier_delta(headScore, oracle.score);
            }
            hash_value(stratum.resultHash, seed);
            hash_value(stratum.resultHash, static_cast<std::uint64_t>(comparison + 1));
            hash_value(stratum.resultHash, static_cast<std::uint64_t>(tier));
            hash_value(stratum.resultHash, static_cast<std::uint64_t>(delta));
            hash_value(total.resultHash, seed);
            hash_value(total.resultHash, static_cast<std::uint64_t>(comparison + 1));
            hash_value(total.resultHash, static_cast<std::uint64_t>(tier));
            hash_value(total.resultHash, static_cast<std::uint64_t>(delta));
            if (options.details || comparison != 0) {
                std::cout << "case,split=" << options.split
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",seed=" << seed
                          << ",verdict="
                          << (comparison > 0 ? "oracle-win" : comparison < 0 ? "head-win" : "tie")
                          << ",tier=" << tier
                          << ",delta=" << delta
                          << ",head=" << headScore.lifetimeDistinct << '/'
                          << headScore.totalDailyDistinct << '/' << headScore.totalServings
                          << ",oracle=" << oracle.score.lifetimeDistinct << '/'
                          << oracle.score.totalDailyDistinct << '/' << oracle.score.totalServings
                          << ",settled=" << oracle.settledStates
                          << ",team_states=" << oracle.teamStates
                          << '\n';
            }
        }
    }
    for (const auto& [key, summary] : strata) {
        print_summary("stratum", options.split, key.first, key.second, summary);
    }
    print_summary("summary", options.split, "", "", total);
    if (total.invalid != 0 || total.incomplete != 0 || total.headWins != 0) {
        return 2;
    }
    return 0;
} catch (const std::exception& error) {
    std::cerr << "ceiling oracle failed: " << error.what() << '\n';
    return 1;
}
