#include <algorithm>
#include <array>
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
#include <utility>
#include <vector>

#include "udon/decision.hpp"
#include "udon/graph.hpp"
#include "udon/json.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"
#include "../../strategies/blank_slate/planners.hpp"

namespace {

constexpr std::chrono::milliseconds kProductionBudget{5000};
constexpr std::int32_t kHarvestMode = 7;
constexpr std::int32_t kFutureHarvestMode = 7;
constexpr std::size_t kAgentCount = 3U;
constexpr std::size_t kPatrolCount = 2U;
constexpr std::size_t kSpotCount = 4U;

struct ManifestRow {
    std::string split;
    std::string family;
    std::string fuelProfile;
    std::uint64_t firstSeed = 0;
    std::int32_t count = 0;
};

struct Options {
    std::string manifest = "research/holdouts/CEILING-TANKER-TERMINAL-138.csv";
    std::string split = "development";
    bool details = false;
    std::uint64_t onlySeed = std::numeric_limits<std::uint64_t>::max();
    std::string onlyFuel;
    bool attribute = false;
    bool headOnly = false;
};

struct Fixture {
    udon::MatchConfig config;
    udon::DayState state;
    udon::MatchLedger ledger;
};

struct PendingAction {
    std::uint8_t kind = 0U; // 0 free, 1 wait, 2 move
    std::uint8_t remaining = 0U;
    std::uint8_t target = 0U;
    std::uint8_t fuelCost = 0U;

    [[nodiscard]] friend bool operator==(
        const PendingAction& left,
        const PendingAction& right) = default;
};

struct JointState {
    std::array<std::uint8_t, kAgentCount> positions{};
    std::array<std::uint8_t, kPatrolCount> fuels{};
    std::array<PendingAction, kAgentCount> pending{};
    std::array<std::uint8_t, kPatrolCount> visited{};
    std::array<std::uint8_t, kSpotCount> stocks{};
    std::array<std::uint8_t, kPatrolCount> coLocatedPrevious{};

    [[nodiscard]] friend bool operator==(
        const JointState& left,
        const JointState& right) = default;
};

struct JointStateHash {
    [[nodiscard]] std::size_t operator()(const JointState& state) const noexcept {
        std::uint64_t hash = 1469598103934665603ULL;
        const auto add = [&hash](std::uint8_t value) {
            hash ^= value;
            hash *= 1099511628211ULL;
        };
        for (const std::uint8_t value : state.positions) {
            add(value);
        }
        for (const std::uint8_t value : state.fuels) {
            add(value);
        }
        for (const PendingAction& pending : state.pending) {
            add(pending.kind);
            add(pending.remaining);
            add(pending.target);
            add(pending.fuelCost);
        }
        for (const std::uint8_t value : state.visited) {
            add(value);
        }
        for (const std::uint8_t value : state.stocks) {
            add(value);
        }
        for (const std::uint8_t value : state.coLocatedPrevious) {
            add(value);
        }
        return static_cast<std::size_t>(hash ^ (hash >> 32U));
    }
};

struct OracleResult {
    bool complete = false;
    udon::DayPlan plan;
    udon::SimulationResult simulation;
    udon::OfficialScore score;
    std::uint64_t settledStates = 0;
    std::size_t peakStates = 0;
};

struct WitnessNode {
    std::uint64_t parent = std::numeric_limits<std::uint64_t>::max();
    std::array<std::int8_t, kAgentCount> wireActions{};
    std::uint8_t scheduledMask = 0U;
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
        } else if (argument == "--seed" && index + 1 < argc) {
            options.onlySeed = std::stoull(argv[++index]);
        } else if (argument == "--fuel" && index + 1 < argc) {
            options.onlyFuel = argv[++index];
        } else if (argument == "--attribute") {
            options.attribute = true;
        } else if (argument == "--head-only") {
            options.headOnly = true;
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
        (line != "experiment_id,split,family,fuel_profile,first_seed,count,agent_count,spot_count,day_count,terminal_day,role_mode,oracle_scope" &&
         line != "experiment_id,split,family,fuel_profile,first_seed,count,agent_count,spot_count,day_count,terminal_day,role_mode,scope")) {
        throw std::runtime_error("unexpected tanker-terminal manifest schema");
    }
    std::vector<ManifestRow> rows;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::vector<std::string> fields = split_csv(line);
        if (fields.size() != 12U ||
            (fields.at(0) != "CEILING-TANKER-TERMINAL-138" &&
             fields.at(0) != "CEILING-TANKER-DURABLE-147" &&
             fields.at(0) != "SCORE-TANKER-MOBILE-WINDOWS-140" &&
             fields.at(0) != "SCORE-TANKER-PARENT-FIRST-141" &&
             fields.at(0) != "SCORE-TANKER-POST-PARENT-142" &&
             fields.at(0) != "SCORE-TANKER-TERMINAL-ONLY-144")) {
            throw std::runtime_error("invalid manifest row: " + line);
        }
        if (fields.at(1) != requestedSplit) {
            continue;
        }
        if (fields.at(6) != "3" || fields.at(7) != "4" ||
            fields.at(8) != "4" || fields.at(9) != "4" ||
            fields.at(10) != "two-patrol-one-tanker" ||
            (fields.at(11) != "complete-terminal-joint-step-dp" &&
             fields.at(11) != "paired-parent-candidate")) {
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

[[nodiscard]] std::int32_t fuel_limit_for(const std::string& profile) {
    if (profile == "low") {
        return 2;
    }
    if (profile == "default") {
        return 4;
    }
    if (profile == "high") {
        return 8;
    }
    throw std::invalid_argument("unknown fuel profile: " + profile);
}

[[nodiscard]] Fixture make_fixture(
    const std::string& family,
    const std::string& fuelProfile,
    std::uint64_t seed) {
    constexpr std::int32_t side = 8;
    constexpr std::int32_t cells = side * side;
    constexpr std::int32_t daySteps = 16;
    const std::int32_t fuelLimit = fuel_limit_for(fuelProfile);

    std::vector<udon::CellId> active;
    std::array<udon::CellId, kAgentCount> starts{};
    std::array<udon::CellId, kSpotCount> spotCells{};
    std::array<std::int32_t, kSpotCount> brands{100, 200, 300, 400};
    std::array<std::int32_t, kSpotCount> stocks{1, 1, 1, 1};
    std::vector<udon::CellId> specialTerrain;
    udon::Terrain specialKind = udon::Terrain::Plain;

    active = {27, 28, 29, 35, 36, 37, 43, 44, 45};
    starts = {27, 45, 36};
    if (family == "rendezvous") {
        spotCells = {28, 29, 43, 44};
        stocks = {2, 1, 1, 2};
    } else if (family == "split-frontier") {
        spotCells = {29, 35, 37, 43};
    } else if (family == "stock-contention") {
        spotCells = {28, 35, 37, 44};
        brands = {100, 100, 200, 200};
        stocks = {1, 2, 1, 2};
    } else if (family == "rare-brand") {
        spotCells = {28, 29, 43, 44};
        brands = {100, 100, 200, 999};
        stocks = {2, 2, 1, 1};
    } else if (family == "mountain-bridge") {
        spotCells = {28, 29, 43, 44};
        specialTerrain = {35, 37};
        specialKind = udon::Terrain::Mountain;
    } else if (family == "delayed-claim") {
        spotCells = {28, 29, 43, 44};
        stocks = {2, 1, 1, 2};
        specialTerrain = {35, 37};
        specialKind = udon::Terrain::Road;
    } else {
        throw std::invalid_argument("unknown family: " + family);
    }

    if ((seed & 1U) != 0U) {
        std::swap(starts.at(0), starts.at(1));
        std::reverse(spotCells.begin(), spotCells.end());
    }
    if (seed % 3U == 2U) {
        std::rotate(brands.begin(), brands.begin() + 1, brands.end());
    }
    if (seed % 3U == 0U && family != "split-frontier" && family != "mountain-bridge") {
        stocks.at(static_cast<std::size_t>(mix64(seed) % kSpotCount)) = 2;
    }

    std::vector<std::int32_t> terrain(static_cast<std::size_t>(cells),
        static_cast<std::int32_t>(udon::Terrain::Pond));
    for (const udon::CellId cell : active) {
        terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    for (const udon::CellId cell : specialTerrain) {
        terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(specialKind);
    }
    for (const udon::CellId cell : starts) {
        terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    for (const udon::CellId cell : spotCells) {
        terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }

    std::ostringstream document;
    document << "{\"startsAt\":1786579200,\"daySeconds\":[5,5,5,5],"
             << "\"daySteps\":[16,16,16,16],\"map\":{\"height\":8,\"width\":8,\"cells\":[";
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
    for (std::size_t index = 0; index < kSpotCount; ++index) {
        if (index != 0U) {
            document << ',';
        }
        document << "{\"brand\":" << brands.at(index)
                 << ",\"pos\":" << spotCells.at(index)
                 << ",\"stocks\":" << stocks.at(index) << '}';
    }
    document << "],\"agents\":[" << starts.at(0) << ',' << starts.at(1) << ','
             << starts.at(2) << "],\"fuelLimits\":" << fuelLimit
             << ",\"players\":4,\"busyThreshold\":2,\"jammedThreshold\":4}";

    Fixture fixture;
    fixture.config = udon::parse_match_config(udon::JsonValue::parse(document.str()));
    fixture.state.endsAt = 1786579220;
    fixture.state.dayNumber = 4;
    fixture.state.roadStatuses.assign(
        static_cast<std::size_t>(fixture.config.map.cell_count()),
        udon::RoadStatus::Smooth);
    for (const udon::CellId road : fixture.config.roadCells) {
        fixture.state.roadStatuses.at(static_cast<std::size_t>(road)) =
            seed % 2U == 0U ? udon::RoadStatus::Busy : udon::RoadStatus::Jammed;
    }
    fixture.state.agents = {
        udon::AgentState{udon::AgentKind::Patrol, starts.at(0), fuelLimit},
        udon::AgentState{udon::AgentKind::Patrol, starts.at(1), fuelLimit},
        udon::AgentState{udon::AgentKind::Tanker, starts.at(2), fuelLimit},
    };
    for (std::int32_t brand = 0; brand < fixture.config.brand_count(); ++brand) {
        if (mix64(seed ^ (static_cast<std::uint64_t>(brand) << 41U)) % 3U == 0U) {
            fixture.ledger.lifetimeBrands |= udon::brand_bit(brand);
        }
    }
    fixture.ledger.totalDailyDistinct = 9 + static_cast<std::int32_t>(mix64(seed) % 7U);
    fixture.ledger.totalServings = 20 + static_cast<std::int32_t>(mix64(seed + 1U) % 13U);
    return fixture;
}

[[nodiscard]] bool validates(
    const udon::MatchConfig& config,
    const udon::DayState& state,
    const udon::DayPlan& plan,
    udon::SimulationResult& simulation,
    std::string& mismatch) {
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    simulation = simulator.simulate(state, plan, true);
    const udon::SimulationResult independent = validator.validate(state, plan, true);
    return simulation.valid && independent.valid &&
        validator.agrees_with(simulation, independent, mismatch);
}

[[nodiscard]] udon::DayScore score_from_stocks(
    const udon::MatchConfig& config,
    const JointState& state) {
    udon::DayScore score;
    for (std::size_t spot = 0; spot < kSpotCount; ++spot) {
        const std::int32_t consumed = config.spots.at(spot).stock -
            static_cast<std::int32_t>(state.stocks.at(spot));
        if (consumed <= 0) {
            continue;
        }
        score.brands |= udon::brand_bit(config.spots.at(spot).brandIndex);
        score.servings += consumed;
    }
    score.dailyDistinct = udon::brand_count(score.brands);
    return score;
}

[[nodiscard]] std::vector<std::pair<udon::PlanAction, PendingAction>> choices_for(
    const Fixture& fixture,
    const JointState& state,
    std::size_t agent,
    std::int32_t currentStep) {
    const std::int32_t remainingDay =
        fixture.config.steps_for_day(fixture.state.dayNumber) - currentStep;
    std::vector<std::pair<udon::PlanAction, PendingAction>> choices;
    choices.reserve(static_cast<std::size_t>(1 + udon::kDirectionCount));
    // wait(N) is score-dominated by N consecutive wait(1) actions. Both keep
    // position and co-location identical at every step and can start the same
    // successor action at step N. Earlier completion can only claim the same
    // team-owned spot stock earlier; it cannot reduce team brands or servings,
    // and the long wait must mark that patrol visited before it becomes free.
    choices.push_back({
        udon::PlanAction::wait(1),
        PendingAction{1U, 1U, state.positions.at(agent), 0U},
    });
    const udon::CellId source = static_cast<udon::CellId>(state.positions.at(agent));
    for (std::int32_t direction = 0; direction < udon::kDirectionCount; ++direction) {
        const udon::CellId target = fixture.config.map.neighbors
            .at(static_cast<std::size_t>(source)).at(static_cast<std::size_t>(direction));
        if (target == udon::kInvalidCell ||
            fixture.config.map.terrain.at(static_cast<std::size_t>(target)) == udon::Terrain::Pond) {
            continue;
        }
        const udon::MoveCost cost = fixture.config.move_cost(
            source,
            fixture.state.roadStatuses.at(static_cast<std::size_t>(source)));
        if (cost.steps <= 0 || cost.steps > remainingDay ||
            (agent < kPatrolCount &&
             static_cast<std::int32_t>(state.fuels.at(agent)) < cost.patrolFuel)) {
            continue;
        }
        choices.push_back({
            udon::PlanAction::move(direction),
            PendingAction{2U, static_cast<std::uint8_t>(cost.steps),
                static_cast<std::uint8_t>(target),
                static_cast<std::uint8_t>(cost.patrolFuel)},
        });
    }
    return choices;
}

[[nodiscard]] JointState advance_one_step(
    const Fixture& fixture,
    JointState state,
    std::int32_t nextStep) {
    std::array<bool, kAgentCount> completed{};
    for (std::size_t agent = 0; agent < kAgentCount; ++agent) {
        PendingAction& pending = state.pending.at(agent);
        if (pending.kind == 0U || pending.remaining == 0U) {
            throw std::logic_error("exact DP found an unscheduled agent");
        }
        --pending.remaining;
        if (pending.remaining != 0U) {
            continue;
        }
        if (pending.kind == 2U) {
            if (agent < kPatrolCount) {
                if (state.fuels.at(agent) < pending.fuelCost) {
                    throw std::logic_error("exact DP patrol fuel underflow");
                }
                state.fuels.at(agent) = static_cast<std::uint8_t>(
                    state.fuels.at(agent) - pending.fuelCost);
            }
            state.positions.at(agent) = pending.target;
        }
        completed.at(agent) = true;
        pending = PendingAction{};
    }

    for (std::size_t patrol = 0; patrol < kPatrolCount; ++patrol) {
        if (!completed.at(patrol)) {
            continue;
        }
        const udon::SpotIndex spot = fixture.config.spotAtCell.at(
            static_cast<std::size_t>(state.positions.at(patrol)));
        if (spot == udon::kInvalidSpot) {
            continue;
        }
        const std::uint8_t bit = static_cast<std::uint8_t>(
            std::uint8_t{1} << static_cast<std::uint32_t>(spot));
        if ((state.visited.at(patrol) & bit) != 0U) {
            continue;
        }
        state.visited.at(patrol) = static_cast<std::uint8_t>(
            state.visited.at(patrol) | bit);
        if (state.stocks.at(static_cast<std::size_t>(spot)) > 0U) {
            --state.stocks.at(static_cast<std::size_t>(spot));
        }
    }

    const std::uint8_t tankerPosition = state.positions.at(2);
    for (std::size_t patrol = 0; patrol < kPatrolCount; ++patrol) {
        const bool coLocatedNow = state.positions.at(patrol) == tankerPosition;
        if (coLocatedNow && state.coLocatedPrevious.at(patrol) != 0U) {
            state.fuels.at(patrol) = static_cast<std::uint8_t>(fixture.config.fuelLimit);
        }
        state.coLocatedPrevious.at(patrol) = coLocatedNow ? 1U : 0U;
    }

    if (nextStep == fixture.config.steps_for_day(fixture.state.dayNumber)) {
        for (std::size_t patrol = 0; patrol < kPatrolCount; ++patrol) {
            if (state.positions.at(patrol) == tankerPosition) {
                state.fuels.at(patrol) = static_cast<std::uint8_t>(fixture.config.fuelLimit);
            }
        }
    }
    return state;
}

struct StateWitness {
    JointState state;
    std::uint64_t witness = std::numeric_limits<std::uint64_t>::max();
};

using DominanceFrontier =
    std::unordered_map<JointState, std::vector<StateWitness>, JointStateHash>;

[[nodiscard]] JointState structural_key(JointState state) {
    state.fuels.fill(0U);
    state.visited.fill(0U);
    return state;
}

[[nodiscard]] bool dominates(
    const JointState& left,
    const JointState& right) {
    for (std::size_t patrol = 0; patrol < kPatrolCount; ++patrol) {
        if (left.fuels.at(patrol) < right.fuels.at(patrol) ||
            (left.visited.at(patrol) & right.visited.at(patrol)) !=
                left.visited.at(patrol)) {
            return false;
        }
    }
    return true;
}

void canonicalize_empty_stock_visits(JointState& state) {
    for (std::size_t spot = 0; spot < kSpotCount; ++spot) {
        if (state.stocks.at(spot) != 0U) {
            continue;
        }
        const std::uint8_t keepMask = static_cast<std::uint8_t>(
            ~(std::uint8_t{1} << static_cast<std::uint32_t>(spot)));
        for (std::size_t patrol = 0; patrol < kPatrolCount; ++patrol) {
            state.visited.at(patrol) = static_cast<std::uint8_t>(
                state.visited.at(patrol) & keepMask);
        }
    }
}

void insert_nondominated(
    JointState state,
    std::uint64_t sourceWitness,
    const std::array<std::int8_t, kAgentCount>& scheduledActions,
    std::uint8_t scheduledMask,
    DominanceFrontier& frontier,
    std::vector<WitnessNode>& witnesses) {
    canonicalize_empty_stock_visits(state);
    std::vector<StateWitness>& group = frontier[structural_key(state)];
    for (const StateWitness& incumbent : group) {
        if (dominates(incumbent.state, state)) {
            return;
        }
    }
    group.erase(
        std::remove_if(
            group.begin(),
            group.end(),
            [&state](const StateWitness& incumbent) {
                return dominates(state, incumbent.state);
            }),
        group.end());
    const std::uint64_t witness = scheduledMask == 0U
        ? sourceWitness
        : static_cast<std::uint64_t>(witnesses.size());
    if (scheduledMask != 0U) {
        witnesses.push_back(WitnessNode{
            sourceWitness,
            scheduledActions,
            scheduledMask,
        });
    }
    group.push_back(StateWitness{std::move(state), witness});
}

void enumerate_joint_schedules(
    const Fixture& fixture,
    std::int32_t currentStep,
    std::size_t agent,
    JointState& state,
    std::uint64_t sourceWitness,
    std::array<std::int8_t, kAgentCount>& scheduledActions,
    std::uint8_t scheduledMask,
    DominanceFrontier& next,
    std::vector<WitnessNode>& witnesses) {
    if (agent == kAgentCount) {
        insert_nondominated(
            advance_one_step(fixture, state, currentStep + 1),
            sourceWitness,
            scheduledActions,
            scheduledMask,
            next,
            witnesses);
        return;
    }
    if (state.pending.at(agent).kind != 0U) {
        enumerate_joint_schedules(
            fixture,
            currentStep,
            agent + 1U,
            state,
            sourceWitness,
            scheduledActions,
            scheduledMask,
            next,
            witnesses);
        return;
    }
    const auto choices = choices_for(fixture, state, agent, currentStep);
    for (const auto& [action, pending] : choices) {
        state.pending.at(agent) = pending;
        scheduledActions.at(agent) = static_cast<std::int8_t>(action.wire_value());
        enumerate_joint_schedules(
            fixture,
            currentStep,
            agent + 1U,
            state,
            sourceWitness,
            scheduledActions,
            static_cast<std::uint8_t>(scheduledMask |
                (std::uint8_t{1} << static_cast<std::uint32_t>(agent))),
            next,
            witnesses);
        state.pending.at(agent) = PendingAction{};
    }
}

[[nodiscard]] OracleResult solve_oracle(const Fixture& fixture) {
    JointState initial;
    for (std::size_t agent = 0; agent < kAgentCount; ++agent) {
        initial.positions.at(agent) = static_cast<std::uint8_t>(
            fixture.state.agents.at(agent).position);
    }
    for (std::size_t patrol = 0; patrol < kPatrolCount; ++patrol) {
        initial.fuels.at(patrol) = static_cast<std::uint8_t>(
            fixture.state.agents.at(patrol).fuel);
        initial.coLocatedPrevious.at(patrol) =
            initial.positions.at(patrol) == initial.positions.at(2) ? 1U : 0U;
    }
    for (std::size_t spot = 0; spot < kSpotCount; ++spot) {
        initial.stocks.at(spot) = static_cast<std::uint8_t>(
            fixture.config.spots.at(spot).stock);
    }
    std::vector<StateWitness> frontier;
    constexpr std::uint64_t noWitness = std::numeric_limits<std::uint64_t>::max();
    frontier.push_back(StateWitness{initial, noWitness});
    std::vector<WitnessNode> witnesses;
    witnesses.reserve(65536U);

    OracleResult result;
    const std::int32_t finalStep = fixture.config.steps_for_day(fixture.state.dayNumber);
    for (std::int32_t currentStep = 0; currentStep < finalStep; ++currentStep) {
        result.settledStates += frontier.size();
        result.peakStates = std::max(result.peakStates, frontier.size());
        DominanceFrontier next;
        next.reserve(frontier.size() * 2U);
        for (const StateWitness& source : frontier) {
            JointState state = source.state;
            std::array<std::int8_t, kAgentCount> scheduledActions{};
            enumerate_joint_schedules(
                fixture,
                currentStep,
                0U,
                state,
                source.witness,
                scheduledActions,
                0U,
                next,
                witnesses);
        }
        frontier.clear();
        std::size_t stateCount = 0U;
        for (const auto& [ignored, group] : next) {
            static_cast<void>(ignored);
            stateCount += group.size();
        }
        frontier.reserve(stateCount);
        for (auto& [ignored, group] : next) {
            static_cast<void>(ignored);
            for (StateWitness& state : group) {
                frontier.push_back(std::move(state));
            }
        }
        if (frontier.empty()) {
            return result;
        }
    }
    result.settledStates += frontier.size();
    result.peakStates = std::max(result.peakStates, frontier.size());

    auto best = frontier.begin();
    udon::DayScore bestDayScore = score_from_stocks(fixture.config, best->state);
    udon::OfficialScore bestScore =
        udon::OfficialScore::after_day(fixture.ledger, bestDayScore);
    for (auto candidate = std::next(frontier.begin()); candidate != frontier.end(); ++candidate) {
        const udon::DayScore dayScore = score_from_stocks(fixture.config, candidate->state);
        const udon::OfficialScore score =
            udon::OfficialScore::after_day(fixture.ledger, dayScore);
        if (bestScore < score) {
            best = candidate;
            bestDayScore = dayScore;
            bestScore = score;
        }
    }
    std::vector<std::uint64_t> witnessChain;
    for (std::uint64_t witness = best->witness; witness != noWitness;) {
        witnessChain.push_back(witness);
        witness = witnesses.at(static_cast<std::size_t>(witness)).parent;
    }
    std::reverse(witnessChain.begin(), witnessChain.end());
    result.plan.actions.resize(kAgentCount);
    for (const std::uint64_t witness : witnessChain) {
        const WitnessNode& node = witnesses.at(static_cast<std::size_t>(witness));
        for (std::size_t agent = 0; agent < kAgentCount; ++agent) {
            if ((node.scheduledMask &
                 (std::uint8_t{1} << static_cast<std::uint32_t>(agent))) == 0U) {
                continue;
            }
            const std::int32_t wire = node.wireActions.at(agent);
            result.plan.actions.at(agent).push_back(
                wire >= 0 ? udon::PlanAction::move(wire) : udon::PlanAction::wait(-wire));
        }
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
    if (!(result.score == bestScore) ||
        result.simulation.score.brands != bestDayScore.brands ||
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

void print_plan_trace(
    const std::string& label,
    const udon::DayPlan& plan,
    const udon::SimulationResult& simulation) {
    for (std::size_t agent = 0; agent < plan.actions.size(); ++agent) {
        std::cout << label << "_plan,agent=" << agent << ",wire=";
        for (std::size_t action = 0; action < plan.actions.at(agent).size(); ++action) {
            if (action != 0U) {
                std::cout << ':';
            }
            std::cout << plan.actions.at(agent).at(action).wire_value();
        }
        std::cout << ",trace=";
        for (std::int32_t step = 0; step < simulation.trace.stepCount; ++step) {
            if (step != 0) {
                std::cout << ':';
            }
            std::cout << simulation.trace.position_at(step, static_cast<udon::AgentIndex>(agent))
                      << '/'
                      << simulation.trace.fuel_at(step, static_cast<udon::AgentIndex>(agent));
        }
        std::cout << '\n';
    }
    std::cout << label << "_claims";
    for (const udon::ClaimEvent& claim : simulation.claims) {
        std::cout << ",a" << claim.agent << "s" << claim.spot
                  << "t" << claim.step << "=" << (claim.served ? 1 : 0);
    }
    std::cout << '\n';
}

[[nodiscard]] bool same_actions(
    const udon::AgentPlan& left,
    const udon::AgentPlan& right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (left.at(index).kind != right.at(index).kind ||
            left.at(index).value != right.at(index).value) {
            return false;
        }
    }
    return true;
}

void attribute_generation_and_master(
    const Fixture& fixture,
    const OracleResult& oracle,
    const udon::OfficialScore& headScore) {
    const udon::ParetoRouter router(fixture.config);
    const udon::RouteColumnGenerator generator(fixture.config, router);
    const udon::ExactStepSimulator simulator(fixture.config);
    const udon::IndependentDayValidator validator(fixture.config);
    const udon::RouteMaster master(fixture.config, simulator, validator);

    udon::ColumnGenerationOptions generation;
    generation.maximumPathsPerTarget = 4;
    generation.maximumColumnsPerAgent = 16;
    generation.maximumTargetSpots = 12;
    generation.maximumEscorts = 16;
    generation.maximumSeedPlans = 1;
    generation.enableHarvestExtensions = true;
    generation.allowUncachedHarvestTargets = true;
    generation.enableHarvestOrienteering = false;
    generation.enableExactHarvestOrienteering = true;
    generation.enableFuelConstrainedExactHarvestOrienteering = true;
    generation.enableAnytimeFuelConstrainedHarvestOrienteering = true;
    generation.maximumHarvestExtensionSources = 4;
    generation.maximumHarvestExtensionDepth = 3;

    udon::ColumnGenerationDiagnostics baselineDiagnostics;
    const udon::RoutePortfolio baseline = generator.generate(
        fixture.state,
        fixture.ledger,
        generation,
        &baselineDiagnostics);
    std::uint32_t exactRouteMask = 0U;
    std::cout << "attribute_portfolio";
    for (std::size_t agent = 0; agent < kAgentCount; ++agent) {
        const auto& columns = baseline.columnsByAgent.at(agent);
        const bool present = std::any_of(
            columns.begin(),
            columns.end(),
            [&oracle, agent](const udon::RouteColumn& column) {
                return same_actions(column.actions, oracle.plan.actions.at(agent));
            });
        if (present) {
            exactRouteMask |= std::uint32_t{1} << static_cast<std::uint32_t>(agent);
        }
        std::cout << ",a" << agent << "_width=" << columns.size()
                  << ",a" << agent << "_exact=" << (present ? 1 : 0);
    }
    std::cout << ",mask=" << exactRouteMask
              << ",exact_supported=" << baselineDiagnostics.exactOrienteeringSupportedAgents
              << ",exact_complete=" << baselineDiagnostics.exactOrienteeringCompleteAgents
              << '\n';

    udon::MasterOptions masterOptions;
    masterOptions.maximumCombinations = 40000;
    masterOptions.maximumCandidates = 32;
    masterOptions.diversityCandidates = 8;
    udon::MasterDiagnostics baselineMasterDiagnostics;
    const std::vector<udon::MasterCandidate> baselineCandidates = master.solve(
        fixture.state,
        fixture.ledger,
        baseline,
        masterOptions,
        baselineMasterDiagnostics);
    const udon::OfficialScore baselineBest = baselineCandidates.empty()
        ? headScore
        : baselineCandidates.front().scoreAfterToday;
    std::cout << "attribute_baseline_master,score="
              << baselineBest.lifetimeDistinct << '/'
              << baselineBest.totalDailyDistinct << '/'
              << baselineBest.totalServings
              << ",candidates=" << baselineCandidates.size()
              << ",nodes=" << baselineMasterDiagnostics.combinationsVisited
              << ",complete=" << (baselineMasterDiagnostics.searchComplete ? 1 : 0)
              << '\n';

    generation.seedPlans.push_back(oracle.plan);
    udon::ColumnGenerationDiagnostics injectedDiagnostics;
    const udon::RoutePortfolio injected = generator.generate(
        fixture.state,
        fixture.ledger,
        generation,
        &injectedDiagnostics);
    udon::MasterDiagnostics injectedMasterDiagnostics;
    const std::vector<udon::MasterCandidate> injectedCandidates = master.solve(
        fixture.state,
        fixture.ledger,
        injected,
        masterOptions,
        injectedMasterDiagnostics);
    const bool exactRetained = std::any_of(
        injectedCandidates.begin(),
        injectedCandidates.end(),
        [&oracle](const udon::MasterCandidate& candidate) {
            return candidate.scoreAfterToday == oracle.score;
        });
    const udon::OfficialScore injectedBest = injectedCandidates.empty()
        ? headScore
        : injectedCandidates.front().scoreAfterToday;
    std::cout << "attribute_injected_master,score="
              << injectedBest.lifetimeDistinct << '/'
              << injectedBest.totalDailyDistinct << '/'
              << injectedBest.totalServings
              << ",exact_retained=" << (exactRetained ? 1 : 0)
              << ",candidates=" << injectedCandidates.size()
              << ",nodes=" << injectedMasterDiagnostics.combinationsVisited
              << ",complete=" << (injectedMasterDiagnostics.searchComplete ? 1 : 0)
              << '\n';

    const std::array<std::pair<const char*, udon::blank_slate::Method>, 4> methods{{
        {"event", udon::blank_slate::Method::EventConflict},
        {"backward", udon::blank_slate::Method::BackwardDeadline},
        {"mcts", udon::blank_slate::Method::MacroMcts},
        {"portfolio", udon::blank_slate::Method::Portfolio},
    }};
    for (const auto& [methodName, method] : methods) {
        udon::blank_slate::Diagnostics independentDiagnostics;
        udon::blank_slate::Planner independent(fixture.config, method);
        const udon::DayPlan independentPlan = independent.solve_day(
            fixture.state,
            fixture.ledger,
            kProductionBudget,
            independentDiagnostics);
        udon::SimulationResult independentSimulation;
        std::string independentMismatch;
        const bool independentValid = validates(
            fixture.config,
            fixture.state,
            independentPlan,
            independentSimulation,
            independentMismatch);
        const udon::OfficialScore independentScore = independentValid
            ? udon::OfficialScore::after_day(
                  fixture.ledger,
                  independentSimulation.score)
            : udon::OfficialScore{};
        std::cout << "attribute_independent_" << methodName
                  << ",valid=" << (independentValid ? 1 : 0)
                  << ",score=" << independentScore.lifetimeDistinct << '/'
                  << independentScore.totalDailyDistinct << '/'
                  << independentScore.totalServings
                  << ",routes=" << independentDiagnostics.routesGenerated
                  << ",plans=" << independentDiagnostics.plansEvaluated
                  << ",nodes=" << independentDiagnostics.searchNodes
                  << ",rollouts=" << independentDiagnostics.rollouts
                  << ",deadline=" << (independentDiagnostics.deadlineReached ? 1 : 0)
                  << '\n';
    }
}

void add_result(
    Summary& summary,
    std::uint64_t seed,
    std::int32_t comparison,
    std::int32_t tier,
    std::int32_t delta) {
    if (comparison > 0) {
        ++summary.oracleWins;
        if (tier == 1) {
            ++summary.tier1Wins;
        } else if (tier == 2) {
            ++summary.tier2Wins;
        } else {
            ++summary.tier3Wins;
        }
        summary.maximumGain = std::max(summary.maximumGain, delta);
    } else if (comparison == 0) {
        ++summary.ties;
    } else {
        ++summary.headWins;
    }
    hash_value(summary.resultHash, seed);
    hash_value(summary.resultHash, static_cast<std::uint64_t>(comparison + 1));
    hash_value(summary.resultHash, static_cast<std::uint64_t>(tier));
    hash_value(summary.resultHash, static_cast<std::uint64_t>(delta));
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
    std::cout << std::unitbuf;
    const Options options = parse_options(argc, argv);
    const std::vector<ManifestRow> rows = load_manifest(options.manifest, options.split);
    Summary total;
    std::map<std::pair<std::string, std::string>, Summary> strata;
    for (const ManifestRow& row : rows) {
        if (!options.onlyFuel.empty() && row.fuelProfile != options.onlyFuel) {
            continue;
        }
        Summary& stratum = strata[{row.family, row.fuelProfile}];
        for (std::int32_t offset = 0; offset < row.count; ++offset) {
            const std::uint64_t seed = row.firstSeed + static_cast<std::uint64_t>(offset);
            if (options.onlySeed != std::numeric_limits<std::uint64_t>::max() &&
                seed != options.onlySeed) {
                continue;
            }
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
            const udon::OfficialScore headScore = udon::OfficialScore::after_day(
                fixture.ledger,
                headSimulation.score);
            if (options.headOnly) {
                std::cout << "head_only,split=" << options.split
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",seed=" << seed
                          << ",score=" << headScore.lifetimeDistinct << '/'
                          << headScore.totalDailyDistinct << '/'
                          << headScore.totalServings << '\n';
                hash_value(stratum.resultHash, seed);
                hash_value(stratum.resultHash, static_cast<std::uint64_t>(headScore.lifetimeDistinct));
                hash_value(stratum.resultHash, static_cast<std::uint64_t>(headScore.totalDailyDistinct));
                hash_value(stratum.resultHash, static_cast<std::uint64_t>(headScore.totalServings));
                hash_value(total.resultHash, seed);
                hash_value(total.resultHash, static_cast<std::uint64_t>(headScore.lifetimeDistinct));
                hash_value(total.resultHash, static_cast<std::uint64_t>(headScore.totalDailyDistinct));
                hash_value(total.resultHash, static_cast<std::uint64_t>(headScore.totalServings));
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
            const std::int32_t comparison =
                udon::compare_lexicographic(oracle.score, headScore);
            std::int32_t tier = 0;
            std::int32_t delta = 0;
            if (comparison > 0) {
                std::tie(tier, delta) = first_tier_delta(oracle.score, headScore);
            } else if (comparison < 0) {
                std::tie(tier, delta) = first_tier_delta(headScore, oracle.score);
            }
            add_result(stratum, seed, comparison, tier, delta);
            add_result(total, seed, comparison, tier, delta);
            if (options.details || options.attribute || comparison != 0) {
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
                          << ",peak=" << oracle.peakStates
                          << '\n';
                if (options.details &&
                    options.onlySeed != std::numeric_limits<std::uint64_t>::max()) {
                    print_plan_trace("head", decision.candidate.plan, headSimulation);
                    print_plan_trace("oracle", oracle.plan, oracle.simulation);
                }
                if (options.attribute) {
                    attribute_generation_and_master(fixture, oracle, headScore);
                }
            }
        }
    }
    for (const auto& [key, summary] : strata) {
        print_summary("stratum", options.split, key.first, key.second, summary);
    }
    print_summary("summary", options.split, {}, {}, total);
    return total.invalid == 0 && total.incomplete == 0 && total.headWins == 0 ? 0 : 2;
} catch (const std::exception& error) {
    std::cerr << "tanker_terminal_oracle: " << error.what() << '\n';
    return 1;
}
