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
#include "udon/json.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"

namespace {

constexpr std::chrono::milliseconds kProductionBudget{5000};
constexpr std::int32_t kHarvestMode = 7;
constexpr std::int32_t kFutureHarvestMode = 7;
constexpr std::size_t kAgentCount = 3U;
constexpr std::size_t kPatrolCount = 2U;
constexpr std::size_t kSpotCount = 3U;

struct ManifestRow {
    std::string family;
    std::string fuelProfile;
    std::int32_t horizon = 0;
    std::uint64_t firstSeed = 0;
    std::int32_t count = 0;
};

struct Options {
    std::string manifest = "research/holdouts/CEILING-TANKER-MATCH-150.csv";
    std::string split = "development";
    std::uint64_t onlySeed = 0;
    std::int32_t diagnosticSeconds = 0;
    bool headOnly = false;
    bool details = false;
    bool convoyAttribution = false;
    bool sharedColumnAttribution = false;
    bool dayOneOnly = false;
};

struct Fixture {
    udon::MatchConfig config;
    std::string family;
    std::string fuelProfile;
    std::uint64_t seed = 0;
};

struct PendingAction {
    std::uint8_t kind = 0U;
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

struct WitnessNode {
    std::uint64_t parent = std::numeric_limits<std::uint64_t>::max();
    std::array<std::int8_t, kAgentCount> wireActions{};
    std::uint8_t scheduledMask = 0U;
};

struct StateWitness {
    JointState state;
    std::uint64_t witness = std::numeric_limits<std::uint64_t>::max();
};

using DominanceFrontier =
    std::unordered_map<JointState, std::vector<StateWitness>, JointStateHash>;

struct DayOutcome {
    std::array<std::uint8_t, kAgentCount> positions{};
    std::array<std::uint8_t, kPatrolCount> fuels{};
    std::uint8_t brands = 0U;
    std::int32_t servings = 0;
    udon::DayPlan plan;
};

struct MatchKey {
    std::array<std::uint8_t, kAgentCount> positions{};
    std::array<std::uint8_t, kPatrolCount> fuels{};
    std::uint8_t lifetimeBrands = 0U;

    [[nodiscard]] friend bool operator<(
        const MatchKey& left,
        const MatchKey& right) {
        return std::tie(left.positions, left.fuels, left.lifetimeBrands) <
            std::tie(right.positions, right.fuels, right.lifetimeBrands);
    }
};

struct LayerEntry {
    MatchKey key;
    std::int32_t totalDailyDistinct = 0;
    std::int32_t totalServings = 0;
    std::int32_t parentIndex = -1;
    udon::DayPlan plan;
    bool swapForNextDay = false;
};

struct OracleResult {
    bool complete = false;
    udon::OfficialScore score;
    std::vector<udon::DayPlan> plans;
    std::vector<udon::OfficialScore> cumulative;
    std::vector<std::vector<udon::AgentState>> terminalAgents;
    std::size_t maximumStepFrontier = 0U;
    std::size_t maximumMatchFrontier = 0U;
    std::uint64_t settledStepStates = 0U;
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
    std::int32_t incomplete = 0;
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
        } else if (argument == "--only-seed" && index + 1 < argc) {
            options.onlySeed = std::stoull(argv[++index]);
        } else if (argument == "--diagnostic-seconds" && index + 1 < argc) {
            options.diagnosticSeconds = std::stoi(argv[++index]);
            if (options.diagnosticSeconds <= 0) {
                throw std::invalid_argument(
                    "diagnostic-seconds must be positive");
            }
        } else if (argument == "--details") {
            options.details = true;
        } else if (argument == "--head-only") {
            options.headOnly = true;
        } else if (argument == "--convoy-attribution") {
            options.convoyAttribution = true;
        } else if (argument == "--shared-column-attribution") {
            options.sharedColumnAttribution = true;
        } else if (argument == "--day-one-only") {
            options.dayOneOnly = true;
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
        (line != "experiment_id,split,family,fuel_profile,horizon,first_seed,count,active_agents,total_agents,spot_count,role_mode,oracle_scope" &&
         line != "experiment_id,split,family,fuel_profile,horizon,first_seed,count,active_agents,total_agents,spot_count,role_mode,scope")) {
        throw std::runtime_error("unexpected tanker-match manifest schema");
    }
    std::vector<ManifestRow> rows;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::vector<std::string> fields = split_csv(line);
        const bool exactSweep =
            fields.size() == 12U &&
            fields.at(0) == "CEILING-TANKER-MATCH-150" &&
            fields.at(11) == "complete-three-agent-full-match-step-dp";
        const bool caravanCandidate =
            fields.size() == 12U &&
            (fields.at(0) == "SCORE-TANKER-CARAVAN-152" ||
             fields.at(0) == "SCORE-TANKER-JOINT-CARAVAN-153" ||
             fields.at(0) == "SCORE-TANKER-CARAVAN-PRESEED-155" ||
             fields.at(0) == "SCORE-TANKER-CARAVAN-OVERNIGHT-156" ||
             fields.at(0) == "SCORE-TANKER-SHARED-REFUEL-COLUMNS-157" ||
             fields.at(0) == "SCORE-TANKER-CANONICAL-CARAVAN-158" ||
             fields.at(0) == "SCORE-TANKER-TRAFFIC-NEUTRAL-159" ||
             fields.at(0) == "SCORE-TANKER-PARENT-TIE-PROTECTION-160") &&
            (fields.at(11) == "consumed-exact-anchor" ||
             fields.at(11) == "fresh-candidate-vs-exact" ||
             fields.at(11) == "sealed-candidate-vs-exact");
        if (!exactSweep && !caravanCandidate) {
            throw std::runtime_error("invalid tanker-match manifest row: " + line);
        }
        if (fields.at(1) != requestedSplit) {
            continue;
        }
        if (fields.at(4) != "4" || fields.at(7) != "3" ||
            fields.at(8) != "3" || fields.at(9) != "3" ||
            fields.at(10) != "two-patrol-one-tanker") {
            throw std::runtime_error("manifest row violates tanker-match scope: " + line);
        }
        rows.push_back(ManifestRow{
            fields.at(2),
            fields.at(3),
            std::stoi(fields.at(4)),
            std::stoull(fields.at(5)),
            std::stoi(fields.at(6)),
        });
    }
    if (rows.empty()) {
        throw std::runtime_error(
            "manifest contains no rows for split " + requestedSplit);
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
    const ManifestRow& row,
    std::uint64_t seed) {
    constexpr std::int32_t side = 8;
    constexpr std::int32_t cells = side * side;
    const std::array<udon::CellId, 6> active{26, 27, 28, 34, 35, 36};
    std::array<udon::CellId, kAgentCount> starts{26, 28, 35};
    std::array<udon::CellId, kSpotCount> spotCells{};
    std::array<std::int32_t, kSpotCount> brands{100, 200, 300};
    std::array<std::int32_t, kSpotCount> stocks{1, 1, 1};
    if (row.family == "rendezvous-chain") {
        spotCells = {27, 34, 36};
    } else if (row.family == "split-duty") {
        spotCells = {27, 34, 36};
    } else if (row.family == "stock-cycle") {
        spotCells = {27, 34, 36};
        brands = {100, 100, 200};
        stocks = {2, 1, 2};
    } else if (row.family == "rare-return") {
        spotCells = {27, 34, 36};
        brands = {100, 100, 999};
        stocks = {2, 2, 1};
    } else {
        throw std::invalid_argument("unknown family: " + row.family);
    }
    if ((seed & 1U) != 0U) {
        std::swap(starts.at(0), starts.at(1));
        std::reverse(spotCells.begin(), spotCells.end());
    }
    if (seed % 3U == 2U) {
        std::rotate(brands.begin(), brands.begin() + 1, brands.end());
    }
    if (seed % 3U == 0U) {
        stocks.at(static_cast<std::size_t>(mix64(seed) % kSpotCount)) = 2;
    }

    std::vector<std::int32_t> terrain(
        static_cast<std::size_t>(cells),
        static_cast<std::int32_t>(udon::Terrain::Pond));
    for (const udon::CellId cell : active) {
        terrain.at(static_cast<std::size_t>(cell)) =
            static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    std::ostringstream document;
    document << "{\"startsAt\":1786665600,\"daySeconds\":[5,5,5,5],"
             << "\"daySteps\":[16,16,16,16],\"map\":{\"height\":8,\"width\":8,\"cells\":[";
    for (std::int32_t mapRow = 0; mapRow < side; ++mapRow) {
        if (mapRow != 0) {
            document << ',';
        }
        document << '[';
        for (std::int32_t column = 0; column < side; ++column) {
            if (column != 0) {
                document << ',';
            }
            document << terrain.at(
                static_cast<std::size_t>(mapRow * side + column));
        }
        document << ']';
    }
    document << "]},\"spots\":[";
    for (std::size_t spot = 0; spot < kSpotCount; ++spot) {
        if (spot != 0U) {
            document << ',';
        }
        document << "{\"brand\":" << brands.at(spot)
                 << ",\"pos\":" << spotCells.at(spot)
                 << ",\"stocks\":" << stocks.at(spot) << '}';
    }
    document << "],\"agents\":[" << starts.at(0) << ',' << starts.at(1)
             << ',' << starts.at(2) << "],\"fuelLimits\":"
             << fuel_limit_for(row.fuelProfile)
             << ",\"players\":4,\"busyThreshold\":2,\"jammedThreshold\":4}";

    Fixture fixture;
    fixture.config =
        udon::parse_match_config(udon::JsonValue::parse(document.str()));
    fixture.family = row.family;
    fixture.fuelProfile = row.fuelProfile;
    fixture.seed = seed;
    return fixture;
}

[[nodiscard]] udon::DayState day_state(
    const Fixture& fixture,
    std::int32_t day,
    const std::vector<udon::AgentState>& agents) {
    udon::DayState state;
    state.endsAt = fixture.config.startsAt + static_cast<std::int64_t>(day) * 5;
    state.dayNumber = day;
    state.agents = agents;
    state.roadStatuses.assign(
        static_cast<std::size_t>(fixture.config.map.cell_count()),
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
    simulation = simulator.simulate(state, plan, true);
    const udon::SimulationResult independent =
        validator.validate(state, plan, true);
    return simulation.valid && independent.valid &&
        validator.agrees_with(simulation, independent, mismatch);
}

[[nodiscard]] udon::DayPlan compact_convoy_plan() {
    udon::DayPlan plan;
    plan.actions = {
        {
            udon::PlanAction::move(3),
            udon::PlanAction::move(2),
            udon::PlanAction::move(2),
            udon::PlanAction::move(0),
            udon::PlanAction::move(5),
            udon::PlanAction::move(5),
            udon::PlanAction::wait(4),
        },
        {
            udon::PlanAction::wait(2),
            udon::PlanAction::move(4),
            udon::PlanAction::move(2),
            udon::PlanAction::move(0),
            udon::PlanAction::move(5),
            udon::PlanAction::move(5),
            udon::PlanAction::wait(4),
        },
        {
            udon::PlanAction::move(5),
            udon::PlanAction::move(2),
            udon::PlanAction::move(2),
            udon::PlanAction::move(0),
            udon::PlanAction::move(5),
            udon::PlanAction::move(5),
            udon::PlanAction::wait(4),
        },
    };
    return plan;
}

[[nodiscard]] std::vector<std::pair<udon::PlanAction, PendingAction>>
choices_for(
    const Fixture& fixture,
    std::int32_t day,
    const JointState& state,
    std::size_t agent,
    std::int32_t currentStep) {
    const std::int32_t remainingDay =
        fixture.config.steps_for_day(day) - currentStep;
    std::vector<std::pair<udon::PlanAction, PendingAction>> choices;
    choices.push_back({
        udon::PlanAction::wait(1),
        PendingAction{1U, 1U, state.positions.at(agent), 0U},
    });
    const udon::CellId source =
        static_cast<udon::CellId>(state.positions.at(agent));
    for (std::int32_t direction = 0;
         direction < udon::kDirectionCount;
         ++direction) {
        const udon::CellId target = fixture.config.map.neighbors
            .at(static_cast<std::size_t>(source))
            .at(static_cast<std::size_t>(direction));
        if (target == udon::kInvalidCell ||
            fixture.config.map.terrain.at(static_cast<std::size_t>(target)) ==
                udon::Terrain::Pond) {
            continue;
        }
        const udon::MoveCost cost =
            fixture.config.move_cost(source, udon::RoadStatus::Smooth);
        if (cost.steps <= 0 || cost.steps > remainingDay ||
            (agent < kPatrolCount &&
             static_cast<std::int32_t>(state.fuels.at(agent)) <
                 cost.patrolFuel)) {
            continue;
        }
        choices.push_back({
            udon::PlanAction::move(direction),
            PendingAction{
                2U,
                static_cast<std::uint8_t>(cost.steps),
                static_cast<std::uint8_t>(target),
                static_cast<std::uint8_t>(cost.patrolFuel),
            },
        });
    }
    return choices;
}

[[nodiscard]] JointState advance_one_step(
    const Fixture& fixture,
    std::int32_t day,
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
        state.visited.at(patrol) =
            static_cast<std::uint8_t>(state.visited.at(patrol) | bit);
        if (state.stocks.at(static_cast<std::size_t>(spot)) > 0U) {
            --state.stocks.at(static_cast<std::size_t>(spot));
        }
    }
    const std::uint8_t tankerPosition = state.positions.at(2);
    for (std::size_t patrol = 0; patrol < kPatrolCount; ++patrol) {
        const bool coLocatedNow =
            state.positions.at(patrol) == tankerPosition;
        if (coLocatedNow && state.coLocatedPrevious.at(patrol) != 0U) {
            state.fuels.at(patrol) =
                static_cast<std::uint8_t>(fixture.config.fuelLimit);
        }
        state.coLocatedPrevious.at(patrol) = coLocatedNow ? 1U : 0U;
    }
    if (nextStep == fixture.config.steps_for_day(day)) {
        for (std::size_t patrol = 0; patrol < kPatrolCount; ++patrol) {
            if (state.positions.at(patrol) == tankerPosition) {
                state.fuels.at(patrol) =
                    static_cast<std::uint8_t>(fixture.config.fuelLimit);
            }
        }
    }
    return state;
}

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
            state.visited.at(patrol) =
                static_cast<std::uint8_t>(
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
    const std::uint64_t witness =
        scheduledMask == 0U
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
    std::int32_t day,
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
            advance_one_step(fixture, day, state, currentStep + 1),
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
            day,
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
    for (const auto& [action, pending] :
         choices_for(fixture, day, state, agent, currentStep)) {
        state.pending.at(agent) = pending;
        scheduledActions.at(agent) =
            static_cast<std::int8_t>(action.wire_value());
        enumerate_joint_schedules(
            fixture,
            day,
            currentStep,
            agent + 1U,
            state,
            sourceWitness,
            scheduledActions,
            static_cast<std::uint8_t>(
                scheduledMask |
                (std::uint8_t{1} << static_cast<std::uint32_t>(agent))),
            next,
            witnesses);
        state.pending.at(agent) = PendingAction{};
    }
}

[[nodiscard]] std::vector<DayOutcome> enumerate_day(
    const Fixture& fixture,
    std::int32_t day,
    const MatchKey& start,
    OracleResult& diagnostics,
    std::int32_t diagnosticSeconds,
    const std::chrono::steady_clock::time_point diagnosticStart) {
    JointState initial;
    initial.positions = start.positions;
    initial.fuels = start.fuels;
    for (std::size_t patrol = 0; patrol < kPatrolCount; ++patrol) {
        initial.coLocatedPrevious.at(patrol) =
            initial.positions.at(patrol) == initial.positions.at(2)
            ? 1U
            : 0U;
    }
    for (std::size_t spot = 0; spot < kSpotCount; ++spot) {
        initial.stocks.at(spot) = static_cast<std::uint8_t>(
            fixture.config.spots.at(spot).stock);
    }
    constexpr std::uint64_t noWitness =
        std::numeric_limits<std::uint64_t>::max();
    std::vector<StateWitness> frontier{StateWitness{initial, noWitness}};
    std::vector<WitnessNode> witnesses;
    witnesses.reserve(32768U);
    const std::int32_t finalStep = fixture.config.steps_for_day(day);
    if (diagnosticSeconds > 0) {
        std::cerr << "oracle-progress,day=" << day
                  << ",phase=start"
                  << ",positions="
                  << static_cast<std::int32_t>(start.positions.at(0)) << '/'
                  << static_cast<std::int32_t>(start.positions.at(1)) << '/'
                  << static_cast<std::int32_t>(start.positions.at(2))
                  << ",fuels="
                  << static_cast<std::int32_t>(start.fuels.at(0)) << '/'
                  << static_cast<std::int32_t>(start.fuels.at(1)) << '\n';
    }
    for (std::int32_t currentStep = 0;
         currentStep < finalStep;
         ++currentStep) {
        diagnostics.settledStepStates += frontier.size();
        diagnostics.maximumStepFrontier =
            std::max(diagnostics.maximumStepFrontier, frontier.size());
        DominanceFrontier next;
        next.reserve(frontier.size() * 2U);
        for (const StateWitness& source : frontier) {
            JointState state = source.state;
            std::array<std::int8_t, kAgentCount> scheduledActions{};
            enumerate_joint_schedules(
                fixture,
                day,
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
            for (StateWitness& candidate : group) {
                frontier.push_back(std::move(candidate));
            }
        }
        if (diagnosticSeconds > 0) {
            std::cerr << "oracle-progress,day=" << day
                      << ",step=" << (currentStep + 1)
                      << ",frontier=" << frontier.size()
                      << ",settled=" << diagnostics.settledStepStates << '\n';
        }
        if (diagnosticSeconds > 0 &&
            std::chrono::steady_clock::now() - diagnosticStart >=
                std::chrono::seconds(diagnosticSeconds)) {
            throw std::runtime_error("diagnostic cutoff after completed step");
        }
        if (frontier.empty()) {
            return {};
        }
    }
    diagnostics.settledStepStates += frontier.size();
    diagnostics.maximumStepFrontier =
        std::max(diagnostics.maximumStepFrontier, frontier.size());

    using OutcomeKey = std::tuple<
        std::array<std::uint8_t, kAgentCount>,
        std::array<std::uint8_t, kPatrolCount>,
        std::uint8_t>;
    std::map<OutcomeKey, std::pair<std::int32_t, std::uint64_t>> retained;
    for (const StateWitness& terminal : frontier) {
        std::uint8_t brands = 0U;
        std::int32_t servings = 0;
        for (std::size_t spot = 0; spot < kSpotCount; ++spot) {
            const std::int32_t consumed =
                fixture.config.spots.at(spot).stock -
                static_cast<std::int32_t>(terminal.state.stocks.at(spot));
            if (consumed <= 0) {
                continue;
            }
            brands = static_cast<std::uint8_t>(
                brands |
                (std::uint8_t{1} << static_cast<std::uint32_t>(
                    fixture.config.spots.at(spot).brandIndex)));
            servings += consumed;
        }
        const OutcomeKey key{
            terminal.state.positions,
            terminal.state.fuels,
            brands,
        };
        const auto found = retained.find(key);
        if (found == retained.end() || found->second.first < servings) {
            retained[key] = {servings, terminal.witness};
        }
    }

    std::vector<DayOutcome> outcomes;
    outcomes.reserve(retained.size());
    for (const auto& [key, value] : retained) {
        std::vector<std::uint64_t> chain;
        for (std::uint64_t witness = value.second;
             witness != noWitness;) {
            chain.push_back(witness);
            witness = witnesses.at(static_cast<std::size_t>(witness)).parent;
        }
        std::reverse(chain.begin(), chain.end());
        udon::DayPlan plan;
        plan.actions.resize(kAgentCount);
        for (const std::uint64_t witness : chain) {
            const WitnessNode& node =
                witnesses.at(static_cast<std::size_t>(witness));
            for (std::size_t agent = 0; agent < kAgentCount; ++agent) {
                if ((node.scheduledMask &
                     (std::uint8_t{1} <<
                      static_cast<std::uint32_t>(agent))) == 0U) {
                    continue;
                }
                const std::int32_t wire = node.wireActions.at(agent);
                plan.actions.at(agent).push_back(
                    wire >= 0
                    ? udon::PlanAction::move(wire)
                    : udon::PlanAction::wait(-wire));
            }
        }
        outcomes.push_back(DayOutcome{
            std::get<0>(key),
            std::get<1>(key),
            std::get<2>(key),
            value.first,
            std::move(plan),
        });
    }
    return outcomes;
}

[[nodiscard]] bool accumulated_better(
    std::int32_t daily,
    std::int32_t servings,
    const LayerEntry& incumbent) {
    return std::tie(daily, servings) >
        std::tie(incumbent.totalDailyDistinct, incumbent.totalServings);
}

[[nodiscard]] OracleResult solve_day_one_oracle(
    const Fixture& fixture,
    std::int32_t diagnosticSeconds) {
    OracleResult result;
    MatchKey initial;
    for (std::size_t agent = 0; agent < kAgentCount; ++agent) {
        initial.positions.at(agent) = static_cast<std::uint8_t>(
            fixture.config.initialAgents.at(agent));
    }
    initial.fuels.fill(static_cast<std::uint8_t>(fixture.config.fuelLimit));
    const bool initialSwap =
        std::tie(initial.positions.at(1), initial.fuels.at(1)) <
        std::tie(initial.positions.at(0), initial.fuels.at(0));
    if (initialSwap) {
        std::swap(initial.positions.at(0), initial.positions.at(1));
        std::swap(initial.fuels.at(0), initial.fuels.at(1));
    }
    const std::vector<DayOutcome> outcomes = enumerate_day(
        fixture,
        1,
        initial,
        result,
        diagnosticSeconds,
        std::chrono::steady_clock::now());
    if (outcomes.empty()) {
        return result;
    }
    const DayOutcome* best = &outcomes.front();
    for (const DayOutcome& candidate : outcomes) {
        if (std::tuple{std::popcount(candidate.brands), candidate.servings} >
            std::tuple{std::popcount(best->brands), best->servings}) {
            best = &candidate;
        }
    }
    udon::DayPlan plan = best->plan;
    if (initialSwap) {
        std::swap(plan.actions.at(0), plan.actions.at(1));
    }
    std::vector<udon::AgentState> agents{
        udon::AgentState{
            udon::AgentKind::Patrol,
            fixture.config.initialAgents.at(0),
            fixture.config.fuelLimit,
        },
        udon::AgentState{
            udon::AgentKind::Patrol,
            fixture.config.initialAgents.at(1),
            fixture.config.fuelLimit,
        },
        udon::AgentState{
            udon::AgentKind::Tanker,
            fixture.config.initialAgents.at(2),
            fixture.config.fuelLimit,
        },
    };
    const udon::DayState state = day_state(fixture, 1, agents);
    udon::SimulationResult simulation;
    std::string mismatch;
    if (!validates(fixture.config, state, plan, simulation, mismatch)) {
        throw std::runtime_error(
            "day-one oracle witness validation failed: " + mismatch);
    }
    const std::int32_t brands = std::popcount(best->brands);
    result.score = udon::OfficialScore{brands, brands, best->servings};
    if (simulation.score.dailyDistinct != brands ||
        simulation.score.servings != best->servings) {
        throw std::runtime_error(
            "day-one oracle witness score disagrees with complete DP");
    }
    result.plans.push_back(std::move(plan));
    result.cumulative.push_back(result.score);
    result.terminalAgents.push_back(simulation.finalAgents);
    result.complete = true;
    return result;
}

[[nodiscard]] OracleResult solve_oracle(
    const Fixture& fixture,
    std::int32_t diagnosticSeconds) {
    OracleResult result;
    const auto diagnosticStart = std::chrono::steady_clock::now();
    LayerEntry initial;
    for (std::size_t agent = 0; agent < kAgentCount; ++agent) {
        initial.key.positions.at(agent) = static_cast<std::uint8_t>(
            fixture.config.initialAgents.at(agent));
    }
    initial.key.fuels.fill(
        static_cast<std::uint8_t>(fixture.config.fuelLimit));
    const bool initialSwap =
        std::tie(initial.key.positions.at(1), initial.key.fuels.at(1)) <
        std::tie(initial.key.positions.at(0), initial.key.fuels.at(0));
    if (initialSwap) {
        std::swap(initial.key.positions.at(0), initial.key.positions.at(1));
        std::swap(initial.key.fuels.at(0), initial.key.fuels.at(1));
    }
    std::vector<std::vector<LayerEntry>> layers{
        std::vector<LayerEntry>{initial},
    };
    using DayCacheKey = std::tuple<
        std::array<std::uint8_t, kAgentCount>,
        std::array<std::uint8_t, kPatrolCount>>;
    std::map<DayCacheKey, std::vector<DayOutcome>> dayCache;
    const std::int32_t invariantDaySteps = fixture.config.steps_for_day(1);
    for (std::int32_t day = 2;
         day <= fixture.config.day_count();
         ++day) {
        if (fixture.config.steps_for_day(day) != invariantDaySteps) {
            throw std::runtime_error(
                "time-homogeneous day cache requires equal day steps");
        }
    }
    for (std::int32_t day = 1;
         day <= fixture.config.day_count();
         ++day) {
        const std::vector<LayerEntry>& current = layers.back();
        std::vector<LayerEntry> next;
        std::map<MatchKey, std::size_t> retained;
        for (std::size_t parentIndex = 0;
             parentIndex < current.size();
             ++parentIndex) {
            const LayerEntry& parent = current.at(parentIndex);
            const DayCacheKey cacheKey{
                parent.key.positions,
                parent.key.fuels,
            };
            auto foundOutcomes = dayCache.find(cacheKey);
            if (foundOutcomes == dayCache.end()) {
                foundOutcomes = dayCache.emplace(
                    cacheKey,
                    enumerate_day(
                        fixture,
                        1,
                        parent.key,
                        result,
                        diagnosticSeconds,
                        diagnosticStart)).first;
            }
            for (const DayOutcome& outcome : foundOutcomes->second) {
                LayerEntry candidate;
                candidate.key.positions = outcome.positions;
                candidate.key.fuels = outcome.fuels;
                const bool swapForNextDay =
                    std::tie(
                        candidate.key.positions.at(1),
                        candidate.key.fuels.at(1)) <
                    std::tie(
                        candidate.key.positions.at(0),
                        candidate.key.fuels.at(0));
                if (swapForNextDay) {
                    std::swap(
                        candidate.key.positions.at(0),
                        candidate.key.positions.at(1));
                    std::swap(
                        candidate.key.fuels.at(0),
                        candidate.key.fuels.at(1));
                }
                candidate.key.lifetimeBrands = static_cast<std::uint8_t>(
                    parent.key.lifetimeBrands | outcome.brands);
                candidate.totalDailyDistinct =
                    parent.totalDailyDistinct +
                    static_cast<std::int32_t>(
                        std::popcount(outcome.brands));
                candidate.totalServings =
                    parent.totalServings + outcome.servings;
                candidate.parentIndex =
                    static_cast<std::int32_t>(parentIndex);
                candidate.plan = outcome.plan;
                candidate.swapForNextDay = swapForNextDay;
                const auto found = retained.find(candidate.key);
                if (found != retained.end() &&
                    !accumulated_better(
                        candidate.totalDailyDistinct,
                        candidate.totalServings,
                        next.at(found->second))) {
                    continue;
                }
                if (found == retained.end()) {
                    retained.emplace(candidate.key, next.size());
                    next.push_back(std::move(candidate));
                } else {
                    next.at(found->second) = std::move(candidate);
                }
            }
        }
        if (next.empty()) {
            return result;
        }
        result.maximumMatchFrontier =
            std::max(result.maximumMatchFrontier, next.size());
        layers.push_back(std::move(next));
    }
    const std::vector<LayerEntry>& finalLayer = layers.back();
    std::size_t bestIndex = 0U;
    udon::OfficialScore bestScore{
        static_cast<std::int32_t>(
            std::popcount(finalLayer.front().key.lifetimeBrands)),
        finalLayer.front().totalDailyDistinct,
        finalLayer.front().totalServings,
    };
    for (std::size_t index = 1; index < finalLayer.size(); ++index) {
        const LayerEntry& candidate = finalLayer.at(index);
        const udon::OfficialScore score{
            static_cast<std::int32_t>(
                std::popcount(candidate.key.lifetimeBrands)),
            candidate.totalDailyDistinct,
            candidate.totalServings,
        };
        if (bestScore < score) {
            bestScore = score;
            bestIndex = index;
        }
    }
    struct AbstractDay {
        udon::DayPlan plan;
        bool swap = false;
    };
    std::vector<AbstractDay> abstractDays(
        static_cast<std::size_t>(fixture.config.day_count()));
    std::size_t currentIndex = bestIndex;
    for (std::int32_t day = fixture.config.day_count();
         day >= 1;
         --day) {
        const LayerEntry& entry =
            layers.at(static_cast<std::size_t>(day)).at(currentIndex);
        abstractDays.at(static_cast<std::size_t>(day - 1)) =
            AbstractDay{entry.plan, entry.swapForNextDay};
        currentIndex = static_cast<std::size_t>(entry.parentIndex);
    }
    result.plans.resize(
        static_cast<std::size_t>(fixture.config.day_count()));
    std::array<std::size_t, kPatrolCount> abstractToPhysical =
        initialSwap
        ? std::array<std::size_t, kPatrolCount>{1U, 0U}
        : std::array<std::size_t, kPatrolCount>{0U, 1U};
    for (std::int32_t day = 1;
         day <= fixture.config.day_count();
         ++day) {
        const AbstractDay& abstract =
            abstractDays.at(static_cast<std::size_t>(day - 1));
        udon::DayPlan& physical =
            result.plans.at(static_cast<std::size_t>(day - 1));
        physical.actions.resize(kAgentCount);
        physical.actions.at(abstractToPhysical.at(0)) =
            abstract.plan.actions.at(0);
        physical.actions.at(abstractToPhysical.at(1)) =
            abstract.plan.actions.at(1);
        physical.actions.at(2) = abstract.plan.actions.at(2);
        if (abstract.swap) {
            std::swap(abstractToPhysical.at(0), abstractToPhysical.at(1));
        }
    }

    std::vector<udon::AgentState> agents{
        udon::AgentState{
            udon::AgentKind::Patrol,
            fixture.config.initialAgents.at(0),
            fixture.config.fuelLimit,
        },
        udon::AgentState{
            udon::AgentKind::Patrol,
            fixture.config.initialAgents.at(1),
            fixture.config.fuelLimit,
        },
        udon::AgentState{
            udon::AgentKind::Tanker,
            fixture.config.initialAgents.at(2),
            fixture.config.fuelLimit,
        },
    };
    udon::MatchLedger ledger;
    for (std::int32_t day = 1;
         day <= fixture.config.day_count();
         ++day) {
        const udon::DayState state = day_state(fixture, day, agents);
        udon::SimulationResult simulation;
        std::string mismatch;
        if (!validates(
                fixture.config,
                state,
                result.plans.at(static_cast<std::size_t>(day - 1)),
                simulation,
                mismatch)) {
            throw std::runtime_error(
                "oracle witness validation failed: " + mismatch);
        }
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
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
        throw std::runtime_error(
            "oracle witness score disagrees with complete match DP");
    }
    result.complete = true;
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
    std::vector<udon::AgentState> agents{
        udon::AgentState{
            udon::AgentKind::Patrol,
            fixture.config.initialAgents.at(0),
            fixture.config.fuelLimit,
        },
        udon::AgentState{
            udon::AgentKind::Patrol,
            fixture.config.initialAgents.at(1),
            fixture.config.fuelLimit,
        },
        udon::AgentState{
            udon::AgentKind::Tanker,
            fixture.config.initialAgents.at(2),
            fixture.config.fuelLimit,
        },
    };
    udon::MatchLedger ledger;
    HeadResult result;
    for (std::int32_t day = 1;
         day <= fixture.config.day_count();
         ++day) {
        const udon::DayState state = day_state(fixture, day, agents);
        const auto started = std::chrono::steady_clock::now();
        const udon::DecisionResult decision =
            engine.solve_day(state, ledger, kProductionBudget);
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
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

[[nodiscard]] std::string score_text(const udon::OfficialScore& score) {
    return std::to_string(score.lifetimeDistinct) + '/' +
        std::to_string(score.totalDailyDistinct) + '/' +
        std::to_string(score.totalServings);
}

[[nodiscard]] std::string plan_text(const udon::DayPlan& plan) {
    std::ostringstream output;
    for (std::size_t agent = 0; agent < plan.actions.size(); ++agent) {
        if (agent != 0U) {
            output << '|';
        }
        for (std::size_t action = 0;
             action < plan.actions.at(agent).size();
             ++action) {
            if (action != 0U) {
                output << '.';
            }
            output << plan.actions.at(agent).at(action).wire_value();
        }
    }
    return output.str();
}

[[nodiscard]] std::string agents_text(
    const std::vector<udon::AgentState>& agents) {
    std::ostringstream output;
    for (std::size_t agent = 0; agent < agents.size(); ++agent) {
        if (agent != 0U) {
            output << '|';
        }
        output << agents.at(agent).position << ':' << agents.at(agent).fuel;
    }
    return output.str();
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
            ++summary.tier1;
        } else if (tier == 2) {
            ++summary.tier2;
        } else {
            ++summary.tier3;
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
    const std::string& split,
    const Summary& summary) {
    std::cout << "summary,split=" << split
              << ",cases=" << summary.cases
              << ",oracle_wins=" << summary.oracleWins
              << ",ties=" << summary.ties
              << ",head_wins=" << summary.headWins
              << ",incomplete=" << summary.incomplete
              << ",invalid=" << summary.invalid
              << ",tier1=" << summary.tier1
              << ",tier2=" << summary.tier2
              << ",tier3=" << summary.tier3
              << ",max_gain=" << summary.maximumGain
              << ",result_hash=" << std::hex << summary.resultHash
              << std::dec << '\n';
}

} // namespace

int main(int argc, char** argv) try {
    std::cout << std::unitbuf;
    const Options options = parse_options(argc, argv);
    const std::vector<ManifestRow> rows =
        load_manifest(options.manifest, options.split);
    Summary summary;
    for (const ManifestRow& row : rows) {
        for (std::int32_t offset = 0; offset < row.count; ++offset) {
            const std::uint64_t seed =
                row.firstSeed + static_cast<std::uint64_t>(offset);
            if (options.onlySeed != 0U && options.onlySeed != seed) {
                continue;
            }
            ++summary.cases;
            const Fixture fixture = make_fixture(row, seed);
            if (options.sharedColumnAttribution) {
                if (seed != 3200100U) {
                    throw std::invalid_argument(
                        "shared-column attribution is restricted to consumed seed 3200100");
                }
                const std::vector<udon::AgentState> agents{
                    udon::AgentState{
                        udon::AgentKind::Patrol,
                        fixture.config.initialAgents.at(0),
                        fixture.config.fuelLimit,
                    },
                    udon::AgentState{
                        udon::AgentKind::Patrol,
                        fixture.config.initialAgents.at(1),
                        fixture.config.fuelLimit,
                    },
                    udon::AgentState{
                        udon::AgentKind::Tanker,
                        fixture.config.initialAgents.at(2),
                        fixture.config.fuelLimit,
                    },
                };
                const udon::DayState state = day_state(fixture, 1, agents);
                const udon::ParetoRouter router(fixture.config);
                const udon::RouteColumnGenerator generator(
                    fixture.config,
                    router);
                udon::ColumnGenerationOptions generation;
                generation.maximumPathsPerTarget = 4;
                generation.maximumColumnsPerAgent = 12;
                generation.maximumTargetSpots = 12;
                generation.maximumEscorts = 16;
                const udon::RoutePortfolio portfolio = generator.generate(
                    state,
                    udon::MatchLedger{},
                    generation);
                for (std::size_t agent = 0;
                     agent < portfolio.columnsByAgent.size();
                     ++agent) {
                    for (const udon::RouteColumn& column :
                         portfolio.columnsByAgent.at(agent)) {
                        if (column.requiredRefuels.empty() &&
                            (agent != 2U || column.escortGroup >= 0 ||
                             column.contingencyBundle >= 0)) {
                            continue;
                        }
                        std::cout << "shared-column,agent=" << agent
                                  << ",id=" << column.columnId
                                  << ",servings=" << column.estimatedServings
                                  << ",brands=" << udon::brand_count(
                                         column.estimatedBrands)
                                  << ",escort=" << column.escortGroup
                                  << ",required=";
                        for (std::size_t event = 0;
                             event < column.requiredRefuels.size();
                             ++event) {
                            if (event != 0U) {
                                std::cout << '|';
                            }
                            std::cout << column.requiredRefuels.at(event).cell
                                      << '@'
                                      << column.requiredRefuels.at(event).step;
                        }
                        std::cout << ",actions=";
                        for (std::size_t action = 0;
                             action < column.actions.size();
                             ++action) {
                            if (action != 0U) {
                                std::cout << '.';
                            }
                            std::cout << column.actions.at(action).wire_value();
                        }
                        std::cout << '\n';
                    }
                }
                const udon::ExactStepSimulator simulator(fixture.config);
                const udon::IndependentDayValidator validator(fixture.config);
                const udon::RouteMaster master(
                    fixture.config,
                    simulator,
                    validator);
                udon::MasterOptions masterOptions;
                masterOptions.maximumCombinations = 1000000;
                masterOptions.maximumCandidates = 64;
                masterOptions.maximumResolveRounds = 3;
                udon::MasterDiagnostics diagnostics;
                const std::vector<udon::MasterCandidate> candidates =
                    master.solve(
                        state,
                        udon::MatchLedger{},
                        portfolio,
                        masterOptions,
                        diagnostics);
                std::cout << "shared-column-master,candidates="
                          << candidates.size()
                          << ",combinations="
                          << diagnostics.combinationsVisited;
                if (!candidates.empty()) {
                    std::cout << ",best="
                              << score_text(candidates.front().scoreAfterToday)
                              << ",plan=" << plan_text(candidates.front().plan);
                }
                std::cout << '\n';
                continue;
            }
            if (options.convoyAttribution) {
                if (seed != 3200100U) {
                    throw std::invalid_argument(
                        "convoy attribution is restricted to consumed seed 3200100");
                }
                const std::vector<udon::AgentState> agents{
                    udon::AgentState{
                        udon::AgentKind::Patrol,
                        fixture.config.initialAgents.at(0),
                        fixture.config.fuelLimit,
                    },
                    udon::AgentState{
                        udon::AgentKind::Patrol,
                        fixture.config.initialAgents.at(1),
                        fixture.config.fuelLimit,
                    },
                    udon::AgentState{
                        udon::AgentKind::Tanker,
                        fixture.config.initialAgents.at(2),
                        fixture.config.fuelLimit,
                    },
                };
                const udon::DayState state = day_state(fixture, 1, agents);
                const udon::DayPlan plan = compact_convoy_plan();
                udon::SimulationResult simulation;
                std::string mismatch;
                const bool valid = validates(
                    fixture.config,
                    state,
                    plan,
                    simulation,
                    mismatch);
                std::cout << "convoy-attribution,seed=" << seed
                          << ",valid=" << valid
                          << ",score="
                          << simulation.score.dailyDistinct << '/'
                          << simulation.score.servings
                          << ",agents="
                          << agents_text(simulation.finalAgents)
                          << ",plan=" << plan_text(plan)
                          << ",mismatch=" << mismatch << '\n';
                if (!valid) {
                    ++summary.invalid;
                }
                continue;
            }
            if (options.dayOneOnly) {
                const OracleResult oracle =
                    solve_day_one_oracle(fixture, options.diagnosticSeconds);
                if (!oracle.complete) {
                    ++summary.incomplete;
                    std::cout << "day-one,split=" << options.split
                              << ",family=" << row.family
                              << ",fuel=" << row.fuelProfile
                              << ",seed=" << seed
                              << ",verdict=incomplete\n";
                    continue;
                }
                const HeadResult head = solve_head(fixture);
                if (!head.valid || head.cumulative.empty()) {
                    ++summary.invalid;
                    std::cout << "day-one,split=" << options.split
                              << ",family=" << row.family
                              << ",fuel=" << row.fuelProfile
                              << ",seed=" << seed
                              << ",verdict=head-invalid\n";
                    continue;
                }
                const udon::OfficialScore& headDay = head.cumulative.front();
                const std::int32_t comparison =
                    udon::compare_lexicographic(oracle.score, headDay);
                std::int32_t tier = 0;
                std::int32_t delta = 0;
                if (comparison != 0) {
                    std::tie(tier, delta) = comparison > 0
                        ? first_tier_delta(oracle.score, headDay)
                        : first_tier_delta(headDay, oracle.score);
                }
                add_result(summary, seed, comparison, tier, delta);
                std::cout << "day-one,split=" << options.split
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",seed=" << seed
                          << ",verdict="
                          << (comparison > 0
                              ? "oracle-win"
                              : comparison < 0 ? "head-win" : "tie")
                          << ",tier=" << tier
                          << ",delta=" << delta
                          << ",head=" << score_text(headDay)
                          << ",oracle=" << score_text(oracle.score)
                          << ",step_frontier=" << oracle.maximumStepFrontier
                          << ",settled=" << oracle.settledStepStates << '\n';
                if (options.details) {
                    std::cout << "day-one-trace,seed=" << seed
                              << ",oracle_agents="
                              << agents_text(oracle.terminalAgents.front())
                              << ",head_agents="
                              << agents_text(head.terminalAgents.front())
                              << ",oracle_plan="
                              << plan_text(oracle.plans.front())
                              << ",head_plan="
                              << plan_text(head.plans.front()) << '\n';
                }
                continue;
            }
            if (options.headOnly) {
                const HeadResult head = solve_head(fixture);
                std::cout << "head-only,split=" << options.split
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",seed=" << seed
                          << ",valid=" << head.valid
                          << ",score=" << score_text(head.score) << '\n';
                if (options.details && head.valid) {
                    for (std::size_t day = 0;
                         day < head.plans.size();
                         ++day) {
                        std::cout << "head-trace,seed=" << seed
                                  << ",day=" << (day + 1U)
                                  << ",score="
                                  << score_text(head.cumulative.at(day))
                                  << ",agents="
                                  << agents_text(head.terminalAgents.at(day))
                                  << ",plan="
                                  << plan_text(head.plans.at(day))
                                  << ",audit_candidates="
                                  << head.audits.at(day).candidates.size()
                                  << ",columns=";
                        for (std::size_t agent = 0;
                             agent < head.audits.at(day)
                                         .portfolioColumnsByAgent.size();
                             ++agent) {
                            if (agent != 0U) {
                                std::cout << '|';
                            }
                            std::cout << head.audits.at(day)
                                             .portfolioColumnsByAgent.at(agent);
                        }
                        std::cout << ",max_servings=";
                        for (std::size_t agent = 0;
                             agent < head.audits.at(day)
                                         .portfolioMaximumServingsByAgent.size();
                             ++agent) {
                            if (agent != 0U) {
                                std::cout << '|';
                            }
                            std::cout << head.audits.at(day)
                                             .portfolioMaximumServingsByAgent.at(agent);
                        }
                        std::cout << ",candidate_scores=";
                        for (std::size_t candidate = 0;
                             candidate < head.audits.at(day).candidates.size();
                             ++candidate) {
                            if (candidate != 0U) {
                                std::cout << '|';
                            }
                            std::cout << score_text(
                                head.audits.at(day)
                                    .candidates.at(candidate)
                                    .scoreAfterToday);
                        }
                        std::cout
                                  << ",selection="
                                  << head.audits.at(day).selectionReason
                                  << '\n';
                    }
                }
                if (!head.valid) {
                    ++summary.invalid;
                }
                continue;
            }
            const OracleResult oracle =
                solve_oracle(fixture, options.diagnosticSeconds);
            if (!oracle.complete) {
                ++summary.incomplete;
                std::cout << "case,split=" << options.split
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",seed=" << seed
                          << ",verdict=incomplete\n";
                continue;
            }
            const HeadResult head = solve_head(fixture);
            if (!head.valid) {
                ++summary.invalid;
                std::cout << "case,split=" << options.split
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",seed=" << seed
                          << ",verdict=head-invalid\n";
                continue;
            }
            const std::int32_t comparison =
                udon::compare_lexicographic(oracle.score, head.score);
            std::int32_t tier = 0;
            std::int32_t delta = 0;
            if (comparison > 0) {
                std::tie(tier, delta) =
                    first_tier_delta(oracle.score, head.score);
            } else if (comparison < 0) {
                std::tie(tier, delta) =
                    first_tier_delta(head.score, oracle.score);
            }
            add_result(summary, seed, comparison, tier, delta);
            if (options.details || comparison != 0) {
                std::cout << "case,split=" << options.split
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",seed=" << seed
                          << ",verdict="
                          << (comparison > 0
                              ? "oracle-win"
                              : comparison < 0 ? "head-win" : "tie")
                          << ",tier=" << tier
                          << ",delta=" << delta
                          << ",head=" << head.score.lifetimeDistinct << '/'
                          << head.score.totalDailyDistinct << '/'
                          << head.score.totalServings
                          << ",oracle=" << oracle.score.lifetimeDistinct << '/'
                          << oracle.score.totalDailyDistinct << '/'
                          << oracle.score.totalServings
                          << ",step_frontier=" << oracle.maximumStepFrontier
                          << ",match_frontier=" << oracle.maximumMatchFrontier
                          << ",settled=" << oracle.settledStepStates
                          << '\n';
            }
            if (options.details) {
                for (std::size_t day = 0;
                     day < oracle.plans.size();
                     ++day) {
                    const std::string oracleId =
                        udon::serialize_day_plan(oracle.plans.at(day)).dump();
                    const std::string headId =
                        udon::serialize_day_plan(head.plans.at(day)).dump();
                    const udon::DecisionAudit& audit = head.audits.at(day);
                    const auto oracleAudit = std::find_if(
                        audit.candidates.begin(),
                        audit.candidates.end(),
                        [&oracleId](const udon::CandidateAuditRecord& record) {
                            return record.stableId == oracleId;
                        });
                    std::cout << "trace,seed=" << seed
                              << ",day=" << (day + 1U)
                              << ",oracle_score="
                              << score_text(oracle.cumulative.at(day))
                              << ",head_score="
                              << score_text(head.cumulative.at(day))
                              << ",oracle_agents="
                              << agents_text(oracle.terminalAgents.at(day))
                              << ",head_agents="
                              << agents_text(head.terminalAgents.at(day))
                              << ",oracle_plan="
                              << plan_text(oracle.plans.at(day))
                              << ",head_plan="
                              << plan_text(head.plans.at(day))
                              << ",oracle_in_audit="
                              << (oracleAudit != audit.candidates.end())
                              << ",oracle_disposition="
                              << (oracleAudit != audit.candidates.end()
                                  ? oracleAudit->disposition
                                  : "absent")
                              << ",oracle_w1="
                              << (oracleAudit != audit.candidates.end()
                                  ? oracleAudit->w1Role
                                  : "absent")
                              << ",audit_candidates="
                              << audit.candidates.size()
                              << ",selection=" << audit.selectionReason
                              << ",oracle_id=" << oracleId
                              << ",head_id=" << headId
                              << '\n';
                }
            }
        }
    }
    print_summary(options.split, summary);
    return summary.incomplete == 0 &&
        summary.invalid == 0 &&
        summary.headWins == 0
        ? 0
        : 2;
} catch (const std::exception& error) {
    std::cerr << "tanker_match_oracle: " << error.what() << '\n';
    return 1;
}
