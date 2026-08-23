// SEM-REFUEL-MIDMOVE-205 research probe (read-only attribution).
//
// Mode "semantics": pins the exact simulator and independent validator
// behavior for mid-move tanker co-location on synthetic micro-fixtures.
// Mode "scan": replays archived BTC matches, finds occurrences where a
// mid-move refuel changes final fuel versus a stationary-only counterfactual,
// and compares simulated end-of-day agents against the next authoritative
// day state. No production behavior is touched.

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "udon/btc_protocol.hpp"
#include "udon/json.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/types.hpp"
#include "udon/validator.hpp"

namespace {

using namespace udon;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct BoundaryState {
    CellId position = kInvalidCell;
    bool midMove = false;
};

// Re-expands one agent plan into per-boundary position and pending-move
// state, mirroring ExactStepSimulator scheduling. Cross-checked against the
// captured trace by the caller so any divergence aborts the probe.
[[nodiscard]] std::vector<BoundaryState> expand_agent_schedule(
    const MatchConfig& config,
    const DayState& state,
    const AgentPlan& actions,
    AgentIndex agentIndex) {
    const std::int32_t stepCount = config.steps_for_day(state.dayNumber);
    std::vector<BoundaryState> boundaries(static_cast<std::size_t>(stepCount) + 1U);
    CellId position = state.agents.at(static_cast<std::size_t>(agentIndex)).position;
    std::size_t cursor = 0;
    std::optional<std::int32_t> completion;
    CellId target = kInvalidCell;
    bool pendingIsMove = false;
    const auto schedule = [&](std::int32_t startStep) {
        require(cursor < actions.size(), "schedule past plan end");
        const PlanAction& action = actions.at(cursor++);
        if (action.kind == ActionKind::Wait) {
            completion = startStep + action.value;
            target = position;
            pendingIsMove = false;
            return;
        }
        const CellId destination = config.map.neighbors
            .at(static_cast<std::size_t>(position))
            .at(static_cast<std::size_t>(action.value));
        require(destination != kInvalidCell, "expansion move into invalid cell");
        const MoveCost cost = config.move_cost(
            position,
            state.roadStatuses.at(static_cast<std::size_t>(position)));
        completion = startStep + cost.steps;
        target = destination;
        pendingIsMove = true;
    };
    schedule(0);
    boundaries.at(0) = BoundaryState{position, false};
    for (std::int32_t step = 1; step <= stepCount; ++step) {
        if (completion.has_value() && *completion == step) {
            if (pendingIsMove) {
                position = target;
            }
            completion.reset();
        }
        const bool midMove = completion.has_value() && pendingIsMove;
        boundaries.at(static_cast<std::size_t>(step)) = BoundaryState{position, midMove};
        if (step < stepCount && !completion.has_value()) {
            schedule(step);
        }
    }
    return boundaries;
}

struct RefuelFiring {
    AgentIndex agent = kInvalidAgent;
    std::int32_t step = 0;
    CellId cell = kInvalidCell;
    bool midMoveNow = false;
    bool midMovePrevious = false;
};

struct DayAnalysis {
    std::vector<RefuelFiring> firings;
    std::vector<std::int32_t> counterfactualFinalFuel;
    bool counterfactualFuelViolation = false;
};

// Recomputes fuel trajectories twice: once with the simulator rule (refuel on
// two consecutive co-located boundaries regardless of pending moves) and once
// with a stationary-only rule (no refuel while the patrol is mid-move). The
// simulator-rule trajectory is asserted equal to the captured trace fuels.
[[nodiscard]] DayAnalysis analyze_day(
    const MatchConfig& config,
    const DayState& state,
    const DayPlan& plan,
    const SimulationResult& simulation) {
    const std::int32_t stepCount = config.steps_for_day(state.dayNumber);
    const std::int32_t agentCount = config.agent_count();
    std::vector<std::vector<BoundaryState>> schedules;
    schedules.reserve(static_cast<std::size_t>(agentCount));
    for (AgentIndex agent = 0; agent < agentCount; ++agent) {
        schedules.push_back(expand_agent_schedule(
            config,
            state,
            plan.actions.at(static_cast<std::size_t>(agent)),
            agent));
        for (std::int32_t step = 0; step <= stepCount; ++step) {
            require(
                schedules.back().at(static_cast<std::size_t>(step)).position ==
                    simulation.trace.position_at(step, agent),
                "schedule expansion diverged from simulator trace");
        }
    }
    const auto tanker_at = [&](CellId cell, std::int32_t step) {
        for (AgentIndex agent = 0; agent < agentCount; ++agent) {
            if (state.agents.at(static_cast<std::size_t>(agent)).kind == AgentKind::Tanker &&
                schedules.at(static_cast<std::size_t>(agent))
                        .at(static_cast<std::size_t>(step))
                        .position == cell) {
                return true;
            }
        }
        return false;
    };

    DayAnalysis analysis;
    analysis.counterfactualFinalFuel.assign(static_cast<std::size_t>(agentCount), 0);
    for (AgentIndex agent = 0; agent < agentCount; ++agent) {
        const AgentState& initial = state.agents.at(static_cast<std::size_t>(agent));
        if (initial.kind != AgentKind::Patrol) {
            analysis.counterfactualFinalFuel.at(static_cast<std::size_t>(agent)) = initial.fuel;
            continue;
        }
        const std::vector<BoundaryState>& schedule = schedules.at(static_cast<std::size_t>(agent));
        std::int32_t simulatorFuel = initial.fuel;
        std::int32_t counterfactualFuel = initial.fuel;
        bool previousCoLocated = tanker_at(schedule.at(0).position, 0);
        for (std::int32_t step = 1; step <= stepCount; ++step) {
            const BoundaryState& now = schedule.at(static_cast<std::size_t>(step));
            const BoundaryState& before = schedule.at(static_cast<std::size_t>(step) - 1U);
            // Fuel deduction happens when a move completes at this step.
            if (before.midMove && !now.midMove && now.position != before.position) {
                const MoveCost cost = config.move_cost(
                    before.position,
                    state.roadStatuses.at(static_cast<std::size_t>(before.position)));
                simulatorFuel -= cost.patrolFuel;
                counterfactualFuel -= cost.patrolFuel;
            } else if (!before.midMove && now.position != before.position) {
                const MoveCost cost = config.move_cost(
                    before.position,
                    state.roadStatuses.at(static_cast<std::size_t>(before.position)));
                simulatorFuel -= cost.patrolFuel;
                counterfactualFuel -= cost.patrolFuel;
            }
            if (counterfactualFuel < 0) {
                analysis.counterfactualFuelViolation = true;
            }
            const bool coLocatedNow = tanker_at(now.position, step);
            if (coLocatedNow && previousCoLocated) {
                simulatorFuel = config.fuelLimit;
                if (!now.midMove) {
                    counterfactualFuel = config.fuelLimit;
                } else {
                    analysis.firings.push_back(RefuelFiring{
                        agent,
                        step,
                        now.position,
                        now.midMove,
                        before.midMove,
                    });
                }
            }
            previousCoLocated = coLocatedNow;
            if (step == stepCount && coLocatedNow) {
                // Day-boundary terminal refuel (SEM-REFUEL-002): the
                // simulator re-records the final boundary after topping up
                // every patrol that ends the day on a tanker cell.
                simulatorFuel = config.fuelLimit;
            }
            if (simulatorFuel != simulation.trace.fuel_at(step, agent)) {
                std::ostringstream detail;
                detail << "probe fuel accounting diverged at step " << step
                       << " agent " << agent
                       << ": expected=" << simulatorFuel
                       << " trace=" << simulation.trace.fuel_at(step, agent);
                for (std::int32_t k = std::max(0, step - 3); k <= step; ++k) {
                    const BoundaryState& b = schedule.at(static_cast<std::size_t>(k));
                    detail << " | k=" << k
                           << " pos=" << b.position
                           << " mid=" << (b.midMove ? 1 : 0)
                           << " tank=" << (tanker_at(b.position, k) ? 1 : 0)
                           << " traceFuel=" << simulation.trace.fuel_at(k, agent);
                    detail << " tankers=";
                    for (AgentIndex other = 0; other < agentCount; ++other) {
                        if (state.agents.at(static_cast<std::size_t>(other)).kind ==
                            AgentKind::Tanker) {
                            detail << other << "@"
                                   << simulation.trace.position_at(k, other) << ";";
                        }
                    }
                }
                throw std::runtime_error(detail.str());
            }
        }
        // Day-boundary terminal refuel (SEM-REFUEL-002, server-confirmed):
        // identical under both semantics.
        if (tanker_at(schedule.at(static_cast<std::size_t>(stepCount)).position, stepCount)) {
            counterfactualFuel = config.fuelLimit;
        }
        analysis.counterfactualFinalFuel.at(static_cast<std::size_t>(agent)) = counterfactualFuel;
    }
    return analysis;
}

[[nodiscard]] SimulationResult dual_engine_simulate(
    const MatchConfig& config,
    const DayState& state,
    const DayPlan& plan) {
    const ExactStepSimulator simulator(config);
    const IndependentDayValidator validator(config);
    const SimulationResult simulation = simulator.simulate(state, plan, true);
    const SimulationResult validation = validator.validate(state, plan, true);
    require(simulation.valid, "simulation invalid: " +
        (simulation.error.has_value() ? simulation.error->message : "unknown"));
    std::string mismatch;
    require(
        validator.agrees_with(simulation, validation, mismatch),
        "engines disagree: " + mismatch);
    return simulation;
}

[[nodiscard]] MatchConfig semantics_config() {
    return parse_match_config(JsonValue::parse(R"({
        "startsAt":1778227200,
        "daySeconds":[5,5,5,5],
        "daySteps":[16,16,16,16],
        "map":{"height":8,"width":8,"cells":[
            [1,1,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,2,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0]
        ]},
        "spots":[
            {"brand":10,"pos":16,"stocks":1},
            {"brand":11,"pos":17,"stocks":1},
            {"brand":12,"pos":24,"stocks":1}
        ],
        "agents":[8,9,10],
        "fuelLimits":20,
        "players":2,
        "busyThreshold":2,
        "jammedThreshold":4
    })"));
}

[[nodiscard]] DayState semantics_state(
    const MatchConfig& config,
    const std::vector<AgentState>& agents) {
    DayState state;
    state.endsAt = 1778227205;
    state.dayNumber = 1;
    state.agents = agents;
    state.roadStatuses.assign(
        static_cast<std::size_t>(config.map.cell_count()),
        RoadStatus::Smooth);
    return state;
}

[[nodiscard]] std::int32_t move_direction(
    const MatchConfig& config,
    CellId from,
    CellId to) {
    for (std::int32_t direction = 0; direction < kDirectionCount; ++direction) {
        if (config.map.neighbors.at(static_cast<std::size_t>(from))
                .at(static_cast<std::size_t>(direction)) == to) {
            return direction;
        }
    }
    throw std::runtime_error(
        "cells are not adjacent: " + std::to_string(from) + "->" + std::to_string(to));
}

// Builds a plan of moves along `path` (consecutive adjacent cells) padded
// with an exact-fill trailing wait; `leadWait` optionally waits first.
[[nodiscard]] AgentPlan path_plan(
    const MatchConfig& config,
    const DayState& state,
    CellId start,
    const std::vector<CellId>& path,
    std::int32_t leadWait = 0) {
    AgentPlan plan;
    std::int32_t used = leadWait;
    if (leadWait > 0) {
        plan.push_back(PlanAction::wait(leadWait));
    }
    CellId current = start;
    for (const CellId next : path) {
        plan.push_back(PlanAction::move(move_direction(config, current, next)));
        used += config
            .move_cost(current, state.roadStatuses.at(static_cast<std::size_t>(current)))
            .steps;
        current = next;
    }
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    require(used <= daySteps, "path plan exceeds the day");
    if (used < daySteps) {
        plan.push_back(PlanAction::wait(daySteps - used));
    }
    return plan;
}

void report_case(
    const std::string& name,
    const MatchConfig& config,
    const DayState& state,
    const DayPlan& plan,
    AgentIndex patrol,
    std::int32_t boundary) {
    const SimulationResult simulation = dual_engine_simulate(config, state, plan);
    const bool refueled =
        simulation.trace.fuel_at(boundary, patrol) == config.fuelLimit;
    std::cout << "timeline=" << name << ",fuel0..6=";
    for (std::int32_t k = 0; k <= 6; ++k) {
        std::cout << simulation.trace.fuel_at(k, patrol) << (k < 6 ? "/" : "");
    }
    std::cout << '\n';
    std::cout << "case=" << name
              << ",boundary=" << boundary
              << ",fuel_at_boundary=" << simulation.trace.fuel_at(boundary, patrol)
              << ",fuel_limit=" << config.fuelLimit
              << ",refueled=" << (refueled ? 1 : 0)
              << ",final_fuel=" << simulation.finalAgents.at(static_cast<std::size_t>(patrol)).fuel
              << ",engines_agree=1" << '\n';
}

// Picks a plain, spot-free neighbor chain start -> hop1 -> hop2 for the
// rendezvous cases so terrain costs are the two-step plain kind.
[[nodiscard]] std::pair<CellId, CellId> plain_hops(const MatchConfig& config, CellId start) {
    const auto plain_free = [&](CellId cell) {
        return cell != kInvalidCell &&
            config.map.terrain.at(static_cast<std::size_t>(cell)) == Terrain::Plain &&
            config.spotAtCell.at(static_cast<std::size_t>(cell)) == kInvalidSpot;
    };
    for (std::int32_t first = 0; first < kDirectionCount; ++first) {
        const CellId hop1 = config.map.neighbors.at(static_cast<std::size_t>(start))
            .at(static_cast<std::size_t>(first));
        if (!plain_free(hop1)) {
            continue;
        }
        for (std::int32_t second = 0; second < kDirectionCount; ++second) {
            const CellId hop2 = config.map.neighbors.at(static_cast<std::size_t>(hop1))
                .at(static_cast<std::size_t>(second));
            if (hop2 != start && plain_free(hop2)) {
                return {hop1, hop2};
            }
        }
    }
    throw std::runtime_error("no plain hop pair found");
}

int run_semantics() {
    const MatchConfig config = semantics_config();
    const CellId patrolStart = 9;
    const auto [meet, away] = plain_hops(config, patrolStart);

    // Case A: immediate departure. Patrol moves start->meet (two plain
    // steps, arrives boundary 2), immediately departs meet->away (two plain
    // steps, completes boundary 4). Tanker waits on meet all day. The patrol
    // stands on meet at boundaries 2 and 3 while mid-move.
    {
        const DayState state = semantics_state(config, {
            AgentState{AgentKind::Patrol, patrolStart, 10},
            AgentState{AgentKind::Tanker, meet, config.fuelLimit},
            AgentState{AgentKind::Patrol, 10, config.fuelLimit},
        });
        DayPlan plan;
        plan.actions.push_back(path_plan(config, state, patrolStart, {meet, away}));
        plan.actions.push_back(AgentPlan{PlanAction::wait(16)});
        plan.actions.push_back(AgentPlan{PlanAction::wait(16)});
        report_case("A-immediate-departure", config, state, plan, 0, 3);
    }

    // Case B: mid-move interception from a mountain source. Patrol starts a
    // three-step mountain move at boundary 0; tanker sits on the mountain.
    {
        const CellId mountain = 26;
        CellId exit = kInvalidCell;
        for (std::int32_t direction = 0; direction < kDirectionCount; ++direction) {
            const CellId next = config.map.neighbors.at(static_cast<std::size_t>(mountain))
                .at(static_cast<std::size_t>(direction));
            if (next != kInvalidCell &&
                config.map.terrain.at(static_cast<std::size_t>(next)) == Terrain::Plain) {
                exit = next;
                break;
            }
        }
        require(exit != kInvalidCell, "mountain has no plain exit");
        const DayState state = semantics_state(config, {
            AgentState{AgentKind::Patrol, mountain, 10},
            AgentState{AgentKind::Tanker, mountain, config.fuelLimit},
            AgentState{AgentKind::Patrol, 10, config.fuelLimit},
        });
        DayPlan plan;
        plan.actions.push_back(path_plan(config, state, mountain, {exit}));
        plan.actions.push_back(AgentPlan{PlanAction::wait(16)});
        plan.actions.push_back(AgentPlan{PlanAction::wait(16)});
        report_case("B-midmove-interception", config, state, plan, 0, 1);
    }

    // Case C: smooth-road one-step departure. Patrol arrives at road cell 0
    // from cell 8 (two plain steps, boundary 2), departs 0->1 in one smooth
    // road step. Only boundary 2 is shared with the tanker: no refuel.
    {
        const DayState state = semantics_state(config, {
            AgentState{AgentKind::Patrol, 8, 10},
            AgentState{AgentKind::Tanker, 0, config.fuelLimit},
            AgentState{AgentKind::Patrol, 10, config.fuelLimit},
        });
        DayPlan plan;
        plan.actions.push_back(path_plan(config, state, 8, {0, 1}));
        plan.actions.push_back(AgentPlan{PlanAction::wait(16)});
        plan.actions.push_back(AgentPlan{PlanAction::wait(16)});
        report_case("C-road-one-step-departure", config, state, plan, 0, 3);
    }

    // Case D: tanker mid-move provision. Patrol waits on the meet cell; the
    // tanker starts a two-step plain move out of it at boundary 0 and stands
    // there through boundary 1.
    {
        const DayState state = semantics_state(config, {
            AgentState{AgentKind::Patrol, meet, 10},
            AgentState{AgentKind::Tanker, meet, config.fuelLimit},
            AgentState{AgentKind::Patrol, 10, config.fuelLimit},
        });
        DayPlan plan;
        plan.actions.push_back(AgentPlan{PlanAction::wait(16)});
        plan.actions.push_back(path_plan(config, state, meet, {away}));
        plan.actions.push_back(AgentPlan{PlanAction::wait(16)});
        report_case("D-tanker-midmove-provision", config, state, plan, 0, 1);
    }
    // Case E: arrival onto a waiting tanker at the destination (the m-3811
    // pattern). Patrol moves start->meet->away; tanker waits on away.
    {
        const DayState state = semantics_state(config, {
            AgentState{AgentKind::Patrol, patrolStart, 10},
            AgentState{AgentKind::Tanker, away, config.fuelLimit},
            AgentState{AgentKind::Patrol, 10, config.fuelLimit},
        });
        DayPlan plan;
        plan.actions.push_back(path_plan(config, state, patrolStart, {meet, away}));
        plan.actions.push_back(AgentPlan{PlanAction::wait(16)});
        plan.actions.push_back(AgentPlan{PlanAction::wait(16)});
        report_case("E-arrival-onto-tanker", config, state, plan, 0, 4);
    }
    return 0;
}

struct ReplayDay {
    DayState state;
    DayPlan plan;
    bool hasPlan = false;
};

int run_inspect(
    const std::string& path,
    std::int32_t dayNumber,
    AgentIndex agent) {
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "cannot open replay " + path);
    std::optional<MatchConfig> config;
    std::map<std::int32_t, DayState> states;
    std::map<std::int32_t, DayPlan> plans;
    std::optional<DayPlan> pendingPlan;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const JsonValue event = JsonValue::parse(line);
        const std::string kind = event.at("kind").string();
        if (kind == "setup") {
            config = parse_btc_setup(event.at("body"));
        } else if (kind == "day_state") {
            const DayState state = parse_btc_day_state(
                *config,
                event.at("body"),
                std::chrono::system_clock::time_point{});
            states[state.dayNumber] = state;
        } else if (kind == "actions") {
            pendingPlan = parse_day_plan(*config, event.at("body"));
        } else if (kind == "action_result") {
            if (pendingPlan.has_value() && btc_action_result_accepted(event.at("body"))) {
                const std::optional<std::int32_t> day = btc_action_result_day(event.at("body"));
                plans[*day] = *pendingPlan;
            }
            pendingPlan.reset();
        }
    }
    const DayState& state = states.at(dayNumber);
    const SimulationResult simulation =
        dual_engine_simulate(*config, state, plans.at(dayNumber));
    std::cout << "trajectory,day=" << dayNumber << ",agent=" << agent << '\n';
    const std::int32_t stepCount = config->steps_for_day(dayNumber);
    for (std::int32_t step = 0; step <= stepCount; ++step) {
        std::cout << step << ":" << simulation.trace.position_at(step, agent)
                  << "@" << simulation.trace.fuel_at(step, agent)
                  << ((step + 1) % 10 == 0 ? "\n" : " ");
    }
    std::cout << '\n';
    const auto print_others = [](const DayState& snapshot, const std::string& label) {
        std::cout << label;
        for (const OtherTeamState& team : snapshot.others) {
            for (std::size_t index = 0; index < team.agents.size(); ++index) {
                const AgentState& other = team.agents.at(index);
                if (other.kind == AgentKind::Tanker) {
                    std::cout << " team" << team.teamId << "[" << index << "]@"
                              << other.position;
                }
            }
        }
        std::cout << '\n';
    };
    print_others(state, "opponent_tankers_day_start:");
    const auto next = states.find(dayNumber + 1);
    if (next != states.end()) {
        print_others(next->second, "opponent_tankers_day_end:");
    }
    return 0;
}

int run_scan(const std::vector<std::string>& paths) {
    for (const std::string& path : paths) {
        std::ifstream input(path, std::ios::binary);
        require(static_cast<bool>(input), "cannot open replay " + path);
        std::optional<MatchConfig> config;
        std::map<std::int32_t, ReplayDay> days;
        std::optional<std::int32_t> pendingActionsDay;
        std::optional<DayPlan> pendingPlan;
        std::string line;
        while (std::getline(input, line)) {
            if (line.empty()) {
                continue;
            }
            const JsonValue event = JsonValue::parse(line);
            const std::string kind = event.at("kind").string();
            if (kind == "setup") {
                config = parse_btc_setup(event.at("body"));
            } else if (kind == "day_state") {
                require(config.has_value(), "day_state before setup");
                const DayState state = parse_btc_day_state(
                    *config,
                    event.at("body"),
                    std::chrono::system_clock::time_point{});
                days[state.dayNumber].state = state;
            } else if (kind == "actions") {
                require(config.has_value(), "actions before setup");
                pendingPlan = parse_day_plan(*config, event.at("body"));
                pendingActionsDay.reset();
            } else if (kind == "action_result") {
                const JsonValue& body = event.at("body");
                if (pendingPlan.has_value() && btc_action_result_accepted(body)) {
                    const std::optional<std::int32_t> day = btc_action_result_day(body);
                    require(day.has_value(), "accepted action_result without day");
                    days[*day].plan = *pendingPlan;
                    days[*day].hasPlan = true;
                }
                pendingPlan.reset();
            }
        }
        require(config.has_value(), "replay has no setup: " + path);

        std::int32_t analyzedDays = 0;
        std::int32_t firingsTotal = 0;
        std::int32_t distinguishingDays = 0;
        std::int32_t fuelCriticalDays = 0;
        std::int32_t authoritativeMatches = 0;
        std::int32_t authoritativeChecked = 0;
        for (const auto& [dayNumber, day] : days) {
            if (!day.hasPlan) {
                continue;
            }
            const SimulationResult simulation =
                dual_engine_simulate(*config, day.state, day.plan);
            const DayAnalysis analysis =
                analyze_day(*config, day.state, day.plan, simulation);
            ++analyzedDays;
            firingsTotal += static_cast<std::int32_t>(analysis.firings.size());
            bool distinguishing = false;
            for (AgentIndex agent = 0; agent < config->agent_count(); ++agent) {
                if (analysis.counterfactualFinalFuel.at(static_cast<std::size_t>(agent)) !=
                    simulation.finalAgents.at(static_cast<std::size_t>(agent)).fuel) {
                    distinguishing = true;
                }
            }
            if (distinguishing) {
                ++distinguishingDays;
            }
            if (analysis.counterfactualFuelViolation) {
                ++fuelCriticalDays;
            }
            for (const RefuelFiring& firing : analysis.firings) {
                std::cout << "firing,replay=" << path
                          << ",day=" << dayNumber
                          << ",agent=" << firing.agent
                          << ",step=" << firing.step
                          << ",cell=" << firing.cell
                          << ",mid_move_now=" << (firing.midMoveNow ? 1 : 0)
                          << '\n';
            }
            const auto next = days.find(dayNumber + 1);
            if (next != days.end()) {
                ++authoritativeChecked;
                bool matches = true;
                for (AgentIndex agent = 0; agent < config->agent_count(); ++agent) {
                    const AgentState& predicted =
                        simulation.finalAgents.at(static_cast<std::size_t>(agent));
                    const AgentState& authoritative =
                        next->second.state.agents.at(static_cast<std::size_t>(agent));
                    if (predicted.position != authoritative.position ||
                        predicted.fuel != authoritative.fuel) {
                        matches = false;
                        std::cout << "authoritative_mismatch,replay=" << path
                                  << ",day=" << dayNumber
                                  << ",agent=" << agent
                                  << ",predicted=" << predicted.position << "@" << predicted.fuel
                                  << ",authoritative=" << authoritative.position
                                  << "@" << authoritative.fuel << '\n';
                    }
                }
                if (matches) {
                    ++authoritativeMatches;
                }
                if (distinguishing) {
                    std::cout << "distinguishing_day,replay=" << path
                              << ",day=" << dayNumber
                              << ",authoritative_matches_simulator=" << (matches ? 1 : 0)
                              << '\n';
                }
            }
        }
        std::cout << "replay_summary,replay=" << path
                  << ",analyzed_days=" << analyzedDays
                  << ",midmove_firings=" << firingsTotal
                  << ",distinguishing_days=" << distinguishingDays
                  << ",fuel_critical_days=" << fuelCriticalDays
                  << ",authoritative_checked=" << authoritativeChecked
                  << ",authoritative_matches=" << authoritativeMatches
                  << '\n';
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::vector<std::string> arguments(argv + 1, argv + argc);
        if (!arguments.empty() && arguments.front() == "semantics") {
            return run_semantics();
        }
        if (arguments.size() >= 2U && arguments.front() == "scan") {
            return run_scan({arguments.begin() + 1, arguments.end()});
        }
        if (arguments.size() == 4U && arguments.front() == "inspect") {
            return run_inspect(
                arguments.at(1),
                std::stoi(arguments.at(2)),
                std::stoi(arguments.at(3)));
        }
        std::cerr << "usage: refuel_midmove_probe semantics | scan <replay.jsonl>...\n";
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "probe failure: " << error.what() << '\n';
        return 1;
    }
}
