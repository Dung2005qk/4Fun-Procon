#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <unordered_set>

#include "udon/btc_protocol.hpp"
#include "udon/decision.hpp"
#include "udon/graph.hpp"
#include "udon/json.hpp"
#include "udon/orienteering.hpp"
#include "udon/planner.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/slack_refiner.hpp"
#include "udon/validator.hpp"

namespace {

[[nodiscard]] std::string read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open file: " + path);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

[[nodiscard]] std::vector<udon::JsonValue> read_replay(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open replay: " + path);
    }
    std::vector<udon::JsonValue> events;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            events.push_back(udon::JsonValue::parse(line));
        }
    }
    return events;
}

struct ReplayPrefix {
    udon::MatchConfig config;
    udon::DayState targetState;
    udon::MatchLedger ledger;
};

[[nodiscard]] udon::DayPlan recorded_plan_for_day(
    const std::vector<udon::JsonValue>& events,
    const udon::MatchConfig& config,
    std::int32_t targetDay) {
    bool targetStateSeen = false;
    for (const udon::JsonValue& event : events) {
        const std::string& kind = event.at("kind").string();
        if (kind == "day_state") {
            targetStateSeen =
                static_cast<std::int32_t>(event.at("body").at("day").integer()) + 1 ==
                targetDay;
            continue;
        }
        if (targetStateSeen &&
            (kind == "actions" || kind == "actions_fallback" ||
             kind == "actions_recovery_wait" || kind == "actions_server_wait")) {
            return udon::parse_day_plan(config, event.at("body"));
        }
    }
    throw std::runtime_error("recorded plan is absent for target day");
}

[[nodiscard]] ReplayPrefix reconstruct_prefix(
    const std::vector<udon::JsonValue>& events,
    std::int32_t targetDay) {
    const auto setupEvent = std::find_if(events.begin(), events.end(), [](const udon::JsonValue& event) {
        return event.at("kind").string() == "setup";
    });
    if (setupEvent == events.end()) {
        throw std::runtime_error("replay has no setup event");
    }

    const udon::BtcAdapterOptions options{5000};
    udon::MatchConfig config = udon::parse_btc_setup(setupEvent->at("body"), options);
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    udon::MatchLedger ledger;
    std::optional<udon::DayState> currentState;
    std::optional<udon::SimulationResult> pendingSimulation;
    std::int32_t pendingWireDay = -1;
    std::int32_t lastAcceptedWireDay = -1;

    for (const udon::JsonValue& event : events) {
        const std::string& kind = event.at("kind").string();
        if (kind == "day_state") {
            const std::int64_t atUnixMs = event.at("atUnixMs").integer();
            currentState = udon::parse_btc_day_state(
                config,
                event.at("body"),
                std::chrono::system_clock::time_point{std::chrono::milliseconds{atUnixMs}},
                options);
            if (currentState->dayNumber == targetDay) {
                return ReplayPrefix{std::move(config), std::move(*currentState), ledger};
            }
            pendingWireDay = currentState->dayNumber - 1;
            pendingSimulation.reset();
            continue;
        }
        if (kind == "actions" || kind == "actions_fallback" ||
            kind == "actions_recovery_wait" || kind == "actions_server_wait") {
            if (!currentState.has_value()) {
                throw std::runtime_error("actions precede day_state");
            }
            const udon::DayPlan plan = udon::parse_day_plan(config, event.at("body"));
            const udon::SimulationResult simulation = simulator.simulate(*currentState, plan, false);
            const udon::SimulationResult validation = validator.validate(*currentState, plan, false);
            std::string mismatch;
            if (!simulation.valid || !validator.agrees_with(simulation, validation, mismatch)) {
                throw std::runtime_error("recorded plan failed exact agreement: " + mismatch);
            }
            pendingSimulation = simulation;
            if (kind == "actions_server_wait" && pendingWireDay > lastAcceptedWireDay) {
                ledger.apply(pendingSimulation->score);
                lastAcceptedWireDay = pendingWireDay;
                pendingSimulation.reset();
            }
            continue;
        }
        if ((kind == "action_result" || kind == "action_result_recovery") &&
            pendingSimulation.has_value() && pendingWireDay > lastAcceptedWireDay &&
            udon::btc_action_result_accepted(event.at("body")) &&
            (!udon::btc_action_result_day(event.at("body")).has_value() ||
             *udon::btc_action_result_day(event.at("body")) == pendingWireDay + 1)) {
            ledger.apply(pendingSimulation->score);
            lastAcceptedWireDay = pendingWireDay;
            pendingSimulation.reset();
        }
    }
    throw std::runtime_error("target day is absent from replay");
}

void print_probe(
    const ReplayPrefix& prefix,
    const udon::DayPlan& plan,
    const udon::SimulationResult& simulation) {
    const udon::OfficialScore after = udon::OfficialScore::after_day(prefix.ledger, simulation.score);
    std::cout << "ledger_before=" << prefix.ledger.lifetime_distinct() << '/'
              << prefix.ledger.totalDailyDistinct << '/' << prefix.ledger.totalServings << '\n';
    std::cout << "day_score=" << udon::brand_count(simulation.score.brands) << '/'
              << simulation.score.dailyDistinct << '/' << simulation.score.servings << '\n';
    std::cout << "score_after=" << after.lifetimeDistinct << '/'
              << after.totalDailyDistinct << '/' << after.totalServings << '\n';

    const std::int32_t agentCount = prefix.config.agent_count();
    std::vector<std::vector<udon::SpotIndex>> claimedByAgent(static_cast<std::size_t>(agentCount));
    std::vector<std::vector<udon::SpotIndex>> servedByAgent(static_cast<std::size_t>(agentCount));
    std::vector<std::vector<udon::AgentIndex>> servedBySpot(prefix.config.spots.size());
    std::vector<std::vector<udon::AgentIndex>> deniedBySpot(prefix.config.spots.size());
    for (const udon::ClaimEvent& claim : simulation.claims) {
        claimedByAgent.at(static_cast<std::size_t>(claim.agent)).push_back(claim.spot);
        auto& bySpot = claim.served ? servedBySpot : deniedBySpot;
        bySpot.at(static_cast<std::size_t>(claim.spot)).push_back(claim.agent);
        if (claim.served) {
            servedByAgent.at(static_cast<std::size_t>(claim.agent)).push_back(claim.spot);
        }
    }

    for (udon::AgentIndex agent = 0; agent < agentCount; ++agent) {
        udon::CellId pathCell = prefix.targetState.agents.at(
            static_cast<std::size_t>(agent)).position;
        std::int32_t pathSteps = 0;
        std::int32_t rawPatrolFuel = 0;
        for (const udon::PlanAction& action : plan.actions.at(static_cast<std::size_t>(agent))) {
            if (action.kind == udon::ActionKind::Wait) {
                pathSteps += action.value;
                continue;
            }
            const udon::MoveCost move = prefix.config.move_cost(
                pathCell,
                prefix.targetState.roadStatuses.at(static_cast<std::size_t>(pathCell)));
            pathSteps += move.steps;
            rawPatrolFuel += move.patrolFuel;
            pathCell = prefix.config.map.neighbors.at(static_cast<std::size_t>(pathCell))
                .at(static_cast<std::size_t>(action.value));
        }
        std::cout << "agent=" << agent
                  << " kind=" << (prefix.targetState.agents.at(static_cast<std::size_t>(agent)).kind ==
                        udon::AgentKind::Patrol ? "patrol" : "tanker")
                  << " claims=" << claimedByAgent.at(static_cast<std::size_t>(agent)).size()
                  << " served=" << servedByAgent.at(static_cast<std::size_t>(agent)).size()
                  << " path_steps=" << pathSteps
                  << " raw_path_fuel=" << rawPatrolFuel
                  << " initial_fuel="
                  << prefix.targetState.agents.at(static_cast<std::size_t>(agent)).fuel
                  << " spots=";
        const auto& claims = claimedByAgent.at(static_cast<std::size_t>(agent));
        for (std::size_t offset = 0; offset < claims.size(); ++offset) {
            if (offset != 0U) {
                std::cout << ',';
            }
            const udon::Spot& spot = prefix.config.spots.at(static_cast<std::size_t>(claims.at(offset)));
            std::cout << claims.at(offset) << ':' << spot.brandValue;
        }
        const udon::AgentState& terminal = simulation.finalAgents.at(static_cast<std::size_t>(agent));
        std::cout << " terminal=" << terminal.position << ':' << terminal.fuel << '\n';
    }

    for (udon::SpotIndex spotIndex = 0;
         spotIndex < static_cast<udon::SpotIndex>(prefix.config.spots.size());
         ++spotIndex) {
        const auto& served = servedBySpot.at(static_cast<std::size_t>(spotIndex));
        const auto& denied = deniedBySpot.at(static_cast<std::size_t>(spotIndex));
        if (served.empty() && denied.empty()) {
            continue;
        }
        const udon::Spot& spot = prefix.config.spots.at(static_cast<std::size_t>(spotIndex));
        std::cout << "spot=" << spotIndex << " brand=" << spot.brandValue
                  << " cell=" << spot.position << " stock=" << spot.stock
                  << " served=" << served.size() << " denied=" << denied.size()
                  << " agents=";
        bool first = true;
        for (const udon::AgentIndex agent : served) {
            std::cout << (first ? "" : ",") << agent << 'S';
            first = false;
        }
        for (const udon::AgentIndex agent : denied) {
            std::cout << (first ? "" : ",") << agent << 'D';
            first = false;
        }
        std::cout << '\n';
    }
}

void print_frontier(const ReplayPrefix& prefix, udon::AgentIndex agent) {
    constexpr std::uint32_t kExactBundleMask = 0x999U;
    constexpr std::uint32_t kCanonicalWitnessMask = 0x9D8U;
    const udon::ExactOrienteeringReachability frontier =
        udon::enumerate_anytime_resource_routes(
            prefix.config,
            prefix.targetState,
            agent,
            5,
            32U,
            1250000U,
            std::nullopt,
            0U);
    std::cout << "frontier_agent=" << agent
              << " supported=" << (frontier.supported ? 1 : 0)
              << " complete=" << (frontier.complete ? 1 : 0)
              << " settled=" << frontier.settledStates
              << " maximal=" << frontier.maximalRoutes.size()
              << " supplemental=" << frontier.supplementalRoutes.size()
              << " terminal=" << frontier.terminalVariants.size() << '\n';

    const auto print_routes = [](const char* name,
                                 const std::vector<udon::ExactOrienteeringRoute>& routes) {
        for (std::size_t index = 0; index < routes.size(); ++index) {
            const udon::ExactOrienteeringRoute& route = routes.at(index);
            std::cout << "frontier=" << name << " index=" << index
                      << " mask=0x" << std::hex << std::uppercase << route.spotMask
                      << std::dec << std::nouppercase
                      << " spots=" << std::popcount(route.spotMask)
                      << " steps=" << route.usedSteps
                      << " fuel=" << route.patrolFuel
                      << " terminal=" << route.terminalCell << '\n';
        }
    };
    print_routes("maximal", frontier.maximalRoutes);
    print_routes("supplemental", frontier.supplementalRoutes);
    print_routes("terminal", frontier.terminalVariants);

    const auto contains = [](const std::vector<udon::ExactOrienteeringRoute>& routes,
                             std::uint32_t mask) {
        return std::any_of(
            routes.begin(),
            routes.end(),
            [mask](const udon::ExactOrienteeringRoute& route) {
                return route.spotMask == mask;
            });
    };
    const auto report_mask = [&](const char* name, std::uint32_t mask) {
        std::cout << "target=" << name << " mask=0x" << std::hex << std::uppercase
                  << mask << std::dec << std::nouppercase
                  << " in_maximal=" << (contains(frontier.maximalRoutes, mask) ? 1 : 0)
                  << " in_supplemental=" << (contains(frontier.supplementalRoutes, mask) ? 1 : 0)
                  << " in_terminal=" << (contains(frontier.terminalVariants, mask) ? 1 : 0)
                  << '\n';
    };
    report_mask("exact34", kExactBundleMask);
    report_mask("canonical35", kCanonicalWitnessMask);
}

void print_alns_diagnostics(
    const ReplayPrefix& prefix,
    const udon::DayPlan& frozenWitness) {
    udon::DeadlineCalibration calibration;
    calibration.version = "btc-http-local-budget-v8-idempotent-ack-resend";
    calibration.networkFloor = std::chrono::milliseconds{1600};
    calibration.networkPercent = 20;
    calibration.certificationPercent = 20;
    udon::UdonShieldEngine engine(
        prefix.config,
        {},
        calibration,
        udon::RoutePoolSearch::SinglePass,
        7);
    const udon::DecisionResult result = engine.solve_day(
        prefix.targetState,
        prefix.ledger,
        std::chrono::milliseconds{5000});
    const bool witnessEqual = udon::canonical_plan_bytes(result.candidate.plan) ==
        udon::canonical_plan_bytes(frozenWitness);
    std::cout << "pipeline_score=" << result.candidate.scoreAfterToday.lifetimeDistinct
              << '/' << result.candidate.scoreAfterToday.totalDailyDistinct
              << '/' << result.candidate.scoreAfterToday.totalServings
              << " witness_equal=" << (witnessEqual ? 1 : 0)
              << " exact_seed_servings="
              << result.audit.columnGeneration.exactOrienteeringSeedServings
              << " exact_local_servings="
              << result.audit.columnGeneration.exactOrienteeringLocalServings
              << " recombination_improvements="
              << result.alns.recombinationImprovements << '\n';
    std::cout << "alns_iterations=" << result.alns.iterations
              << " accepted=" << result.alns.accepted
              << " improvements=" << result.alns.improvements
              << " synthesized_routes=" << result.alns.synthesizedRoutes
              << " synthesized_accepted=" << result.alns.synthesizedAccepted
              << " proof_iterations=" << result.alns.proofGuidedIterations
              << " proof_routes=" << result.alns.proofGuidedRoutes
              << " proof_accepted=" << result.alns.proofGuidedAccepted
              << " proof_improvements=" << result.alns.proofGuidedImprovements
              << '\n';
    std::cout << "alns_attempted_by_operator=";
    for (std::size_t index = 0; index < result.alns.attemptedByOperator.size(); ++index) {
        std::cout << (index == 0U ? "" : ",")
                  << result.alns.attemptedByOperator.at(index);
    }
    std::cout << '\n';
    std::cout << "alns_accepted_by_operator=";
    for (std::size_t index = 0; index < result.alns.acceptedByOperator.size(); ++index) {
        std::cout << (index == 0U ? "" : ",")
                  << result.alns.acceptedByOperator.at(index);
    }
    std::cout << '\n';
}

void print_exact_one_exchange(
    const ReplayPrefix& prefix,
    const udon::DayPlan& frozenWitness) {
    const udon::ExactStepSimulator simulator(prefix.config);
    const udon::IndependentDayValidator validator(prefix.config);
    const udon::SimulationResult baseline = simulator.simulate(
        prefix.targetState,
        frozenWitness,
        false);
    const udon::OfficialScore baselineScore = udon::OfficialScore::after_day(
        prefix.ledger,
        baseline.score);
    udon::OfficialScore globalBest = baselineScore;
    udon::AgentIndex globalAgent = udon::kInvalidAgent;
    std::uint32_t globalMask = 0U;
    udon::DayPlan globalPlan = frozenWitness;

    for (udon::AgentIndex agent = 0; agent < prefix.config.agent_count(); ++agent) {
        if (prefix.targetState.agents.at(static_cast<std::size_t>(agent)).kind !=
            udon::AgentKind::Patrol) {
            continue;
        }
        const udon::ExactOrienteeringReachability frontier =
            udon::enumerate_anytime_resource_routes(
                prefix.config,
                prefix.targetState,
                agent,
                5,
                32U,
                1250000U,
                std::nullopt,
                0U);
        udon::OfficialScore agentBest = baselineScore;
        std::uint32_t agentBestMask = 0U;
        std::int32_t validMutations = 0;
        for (const udon::ExactOrienteeringRoute& route : frontier.maximalRoutes) {
            udon::DayPlan mutation = frozenWitness;
            mutation.actions.at(static_cast<std::size_t>(agent)) = route.actions;
            const udon::SimulationResult simulation = simulator.simulate(
                prefix.targetState,
                mutation,
                false);
            const udon::SimulationResult validation = validator.validate(
                prefix.targetState,
                mutation,
                false);
            std::string mismatch;
            if (!simulation.valid || !validator.agrees_with(simulation, validation, mismatch)) {
                continue;
            }
            ++validMutations;
            const udon::OfficialScore score = udon::OfficialScore::after_day(
                prefix.ledger,
                simulation.score);
            if (agentBest < score ||
                (agentBest == score && agentBestMask != 0U && route.spotMask < agentBestMask)) {
                agentBest = score;
                agentBestMask = route.spotMask;
            }
            if (globalBest < score ||
                (globalBest == score && globalAgent != udon::kInvalidAgent &&
                 std::pair{agent, route.spotMask} < std::pair{globalAgent, globalMask})) {
                globalBest = score;
                globalAgent = agent;
                globalMask = route.spotMask;
                globalPlan = std::move(mutation);
            }
        }
        std::cout << "exchange_agent=" << agent
                  << " settled=" << frontier.settledStates
                  << " routes=" << frontier.maximalRoutes.size()
                  << " valid_mutations=" << validMutations
                  << " best=" << agentBest.lifetimeDistinct << '/'
                  << agentBest.totalDailyDistinct << '/' << agentBest.totalServings
                  << " mask=0x" << std::hex << std::uppercase << agentBestMask
                  << std::dec << std::nouppercase << '\n';
    }
    std::cout << "exchange_global_best=" << globalBest.lifetimeDistinct << '/'
              << globalBest.totalDailyDistinct << '/' << globalBest.totalServings
              << " agent=" << globalAgent
              << " mask=0x" << std::hex << std::uppercase << globalMask
              << std::dec << std::nouppercase << '\n';
    if (baselineScore < globalBest) {
        std::cout << "exchange_best_plan="
                  << udon::serialize_day_plan(globalPlan).dump() << '\n';
    }
}

void print_complete_resource_exchange(
    const ReplayPrefix& prefix,
    const udon::DayPlan& frozenWitness) {
    const udon::ExactStepSimulator simulator(prefix.config);
    const udon::IndependentDayValidator validator(prefix.config);
    const udon::SimulationResult baseline = simulator.simulate(
        prefix.targetState,
        frozenWitness,
        false);
    const udon::OfficialScore baselineScore = udon::OfficialScore::after_day(
        prefix.ledger,
        baseline.score);
    udon::OfficialScore globalBest = baselineScore;
    udon::AgentIndex globalAgent = udon::kInvalidAgent;
    std::uint32_t globalMask = 0U;
    udon::DayPlan globalPlan = frozenWitness;
    constexpr std::array<udon::AgentIndex, 2> kUniqueClaimantAgents{2, 4};

    for (const udon::AgentIndex agent : kUniqueClaimantAgents) {
        const udon::ExactOrienteeringReachability frontier =
            udon::enumerate_exact_resource_routes(
                prefix.config,
                prefix.targetState,
                agent,
                std::nullopt);
        udon::OfficialScore agentBest = baselineScore;
        std::uint32_t agentBestMask = 0U;
        std::int32_t validMutations = 0;
        for (const udon::ExactOrienteeringRoute& route : frontier.maximalRoutes) {
            udon::DayPlan mutation = frozenWitness;
            mutation.actions.at(static_cast<std::size_t>(agent)) = route.actions;
            const udon::SimulationResult simulation = simulator.simulate(
                prefix.targetState,
                mutation,
                false);
            const udon::SimulationResult validation = validator.validate(
                prefix.targetState,
                mutation,
                false);
            std::string mismatch;
            if (!simulation.valid || !validator.agrees_with(simulation, validation, mismatch)) {
                continue;
            }
            ++validMutations;
            const udon::OfficialScore score = udon::OfficialScore::after_day(
                prefix.ledger,
                simulation.score);
            if (agentBest < score ||
                (agentBest == score && agentBestMask != 0U && route.spotMask < agentBestMask)) {
                agentBest = score;
                agentBestMask = route.spotMask;
            }
            if (globalBest < score ||
                (globalBest == score && globalAgent != udon::kInvalidAgent &&
                 std::pair{agent, route.spotMask} < std::pair{globalAgent, globalMask})) {
                globalBest = score;
                globalAgent = agent;
                globalMask = route.spotMask;
                globalPlan = std::move(mutation);
            }
        }
        std::cout << "complete_exchange_agent=" << agent
                  << " supported=" << (frontier.supported ? 1 : 0)
                  << " complete=" << (frontier.complete ? 1 : 0)
                  << " settled=" << frontier.settledStates
                  << " routes=" << frontier.maximalRoutes.size()
                  << " valid_mutations=" << validMutations
                  << " best=" << agentBest.lifetimeDistinct << '/'
                  << agentBest.totalDailyDistinct << '/' << agentBest.totalServings
                  << " mask=0x" << std::hex << std::uppercase << agentBestMask
                  << std::dec << std::nouppercase << '\n';
    }
    std::cout << "complete_exchange_global_best=" << globalBest.lifetimeDistinct << '/'
              << globalBest.totalDailyDistinct << '/' << globalBest.totalServings
              << " agent=" << globalAgent
              << " mask=0x" << std::hex << std::uppercase << globalMask
              << std::dec << std::nouppercase << '\n';
    if (baselineScore < globalBest) {
        std::cout << "complete_exchange_best_plan="
                  << udon::serialize_day_plan(globalPlan).dump() << '\n';
    }
}

struct SparseLabel {
    std::uint32_t mask = 0U;
    udon::CellId cell = udon::kInvalidCell;
    std::uint16_t usedSteps = 0U;
    std::uint16_t usedFuel = 0U;
    std::uint32_t parent = std::numeric_limits<std::uint32_t>::max();
    std::int32_t nextAtState = -1;
    std::uint8_t incoming = std::numeric_limits<std::uint8_t>::max();
    bool active = true;
};

struct SparseRouteChoice {
    std::uint32_t label = 0U;
    std::uint32_t mask = 0U;
    std::tuple<
        std::int32_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        std::uint32_t> rank;
};

[[nodiscard]] udon::DayPlan print_sparse_exchange(
    const ReplayPrefix& prefix,
    const udon::DayPlan& frozenWitness,
    std::uint64_t maximumSettledStates = 1250000U,
    udon::AgentIndex selectedAgent = udon::kInvalidAgent,
    std::size_t maximumRoutes = 32U,
    bool enableTerminalMarginal = false) {
    const udon::ExactStepSimulator simulator(prefix.config);
    const udon::IndependentDayValidator validator(prefix.config);
    const udon::SimulationResult baseline = simulator.simulate(
        prefix.targetState,
        frozenWitness,
        false);
    const udon::OfficialScore baselineScore = udon::OfficialScore::after_day(
        prefix.ledger,
        baseline.score);
    udon::OfficialScore globalBest = baselineScore;
    udon::OfficialScore protectedGlobalBest = baselineScore;
    udon::OfficialScore roadEquivalentGlobalBest = baselineScore;
    udon::OfficialScore trafficNonIncreasingGlobalBest = baselineScore;
    udon::AgentIndex globalAgent = udon::kInvalidAgent;
    udon::AgentIndex protectedGlobalAgent = udon::kInvalidAgent;
    udon::AgentIndex roadEquivalentGlobalAgent = udon::kInvalidAgent;
    udon::AgentIndex trafficNonIncreasingGlobalAgent = udon::kInvalidAgent;
    std::uint32_t globalMask = 0U;
    std::uint32_t protectedGlobalMask = 0U;
    std::uint32_t roadEquivalentGlobalMask = 0U;
    std::uint32_t trafficNonIncreasingGlobalMask = 0U;
    udon::DayPlan globalPlan = frozenWitness;
    std::int64_t exactRoadEquivalent = 0;
    std::int64_t strictRoadEquivalent = 0;
    std::int64_t strictRoadEquivalentChangedTerminal = 0;
    std::int32_t roadEquivalentTerminalDistance = 0;
    std::int32_t roadEquivalentFuelDelta = 0;
    std::int64_t trafficNonIncreasing = 0;
    std::int64_t strictTrafficNonIncreasing = 0;
    std::int64_t strictTrafficNonIncreasingChangedTerminal = 0;
    std::int32_t trafficNonIncreasingTerminalDistance = 0;
    std::int32_t trafficNonIncreasingFuelDelta = 0;
    std::int64_t trafficNonIncreasingRoadReduction = 0;

    udon::BrandMask preferredBrands;
    for (std::int32_t brand = 0; brand < prefix.config.brand_count(); ++brand) {
        if (!udon::has_brand(prefix.ledger.lifetimeBrands, brand)) {
            preferredBrands |= udon::brand_bit(brand);
        }
    }
    const std::int32_t minimumSpots = preferredBrands.any()
        ? 1
        : std::min<std::int32_t>(
              std::max(1, prefix.config.brand_count() - 1),
              static_cast<std::int32_t>(prefix.config.spots.size()));
    const std::int32_t daySteps = prefix.config.steps_for_day(
        prefix.targetState.dayNumber);
    const std::uint32_t cellCount = static_cast<std::uint32_t>(
        prefix.config.map.cell_count());
    const auto state_key = [](std::uint32_t mask, udon::CellId cell) {
        return (static_cast<std::uint64_t>(mask) << 32U) |
            static_cast<std::uint32_t>(cell);
    };

    for (udon::AgentIndex agent = 0;
         agent < prefix.config.agent_count();
         ++agent) {
        if (selectedAgent != udon::kInvalidAgent &&
            agent != selectedAgent) {
            continue;
        }
        const udon::AgentState& agentState = prefix.targetState.agents.at(
            static_cast<std::size_t>(agent));
        if (agentState.kind != udon::AgentKind::Patrol) {
            continue;
        }
        using QueueEntry = std::tuple<
            std::uint8_t,
            std::uint16_t,
            std::uint16_t,
            std::uint32_t>;
        std::priority_queue<
            QueueEntry,
            std::vector<QueueEntry>,
            std::greater<>> queue;
        std::unordered_map<std::uint64_t, std::int32_t> firstLabelAtState;
        firstLabelAtState.reserve(1U << 20U);
        std::vector<SparseLabel> labels;
        labels.reserve(1U << 20U);
        const auto relax = [&firstLabelAtState, &labels, &queue, &state_key](
                               std::uint32_t mask,
                               udon::CellId cell,
                               std::uint16_t candidateSteps,
                               std::uint16_t candidateFuel,
                               std::uint32_t parent,
                               std::uint8_t incoming) {
            const std::uint64_t key = state_key(mask, cell);
            const auto iterator = firstLabelAtState.find(key);
            const std::int32_t first = iterator == firstLabelAtState.end()
                ? -1
                : iterator->second;
            for (std::int32_t labelIndex = first;
                 labelIndex >= 0;
                 labelIndex = labels.at(
                     static_cast<std::size_t>(labelIndex)).nextAtState) {
                const SparseLabel& existing = labels.at(
                    static_cast<std::size_t>(labelIndex));
                if (existing.active &&
                    existing.usedSteps <= candidateSteps &&
                    existing.usedFuel <= candidateFuel) {
                    return;
                }
            }
            for (std::int32_t labelIndex = first;
                 labelIndex >= 0;
                 labelIndex = labels.at(
                     static_cast<std::size_t>(labelIndex)).nextAtState) {
                SparseLabel& existing = labels.at(
                    static_cast<std::size_t>(labelIndex));
                if (existing.active &&
                    candidateSteps <= existing.usedSteps &&
                    candidateFuel <= existing.usedFuel) {
                    existing.active = false;
                }
            }
            const std::uint32_t labelIndex = static_cast<std::uint32_t>(
                labels.size());
            labels.push_back(SparseLabel{
                mask,
                cell,
                candidateSteps,
                candidateFuel,
                parent,
                first,
                incoming,
                true,
            });
            firstLabelAtState[key] = static_cast<std::int32_t>(labelIndex);
            const std::uint8_t cardinalityPriority = static_cast<std::uint8_t>(
                32U - std::popcount(mask));
            queue.emplace(
                cardinalityPriority,
                candidateSteps,
                candidateFuel,
                labelIndex);
        };

        relax(
            0U,
            agentState.position,
            0U,
            0U,
            std::numeric_limits<std::uint32_t>::max(),
            std::numeric_limits<std::uint8_t>::max());
        const std::uint32_t rootLabel = 0U;
        const udon::SpotIndex startSpot = prefix.config.spotAtCell.at(
            static_cast<std::size_t>(agentState.position));
        if (startSpot != udon::kInvalidSpot && daySteps >= 1) {
            relax(
                std::uint32_t{1} << static_cast<std::uint32_t>(startSpot),
                agentState.position,
                1U,
                0U,
                rootLabel,
                static_cast<std::uint8_t>(udon::kDirectionCount));
        }

        std::vector<udon::MoveCost> moveCosts(cellCount);
        std::vector<std::uint32_t> destinationSpotBits(cellCount, 0U);
        for (std::uint32_t cell = 0U; cell < cellCount; ++cell) {
            moveCosts.at(cell) = prefix.config.move_cost(
                static_cast<udon::CellId>(cell),
                prefix.targetState.roadStatuses.at(cell));
            const udon::SpotIndex spot = prefix.config.spotAtCell.at(cell);
            if (spot != udon::kInvalidSpot) {
                destinationSpotBits.at(cell) =
                    std::uint32_t{1} << static_cast<std::uint32_t>(spot);
            }
        }

        std::unordered_set<std::uint32_t> emittedMasks;
        emittedMasks.reserve(1U << 16U);
        std::unordered_set<std::uint32_t> protectedEmittedMasks;
        protectedEmittedMasks.reserve(1U << 16U);
        std::vector<SparseRouteChoice> retained;
        retained.reserve(maximumRoutes);
        std::vector<SparseRouteChoice> protectedRetained;
        protectedRetained.reserve(maximumRoutes);
        std::uint64_t settledStates = 0U;
        while (!queue.empty() && settledStates < maximumSettledStates) {
            const auto [
                queuedCardinality,
                queuedSteps,
                queuedFuel,
                labelIndex] = queue.top();
            queue.pop();
            static_cast<void>(queuedCardinality);
            const SparseLabel current = labels.at(
                static_cast<std::size_t>(labelIndex));
            if (!current.active || current.usedSteps != queuedSteps ||
                current.usedFuel != queuedFuel) {
                continue;
            }
            ++settledStates;
            if (static_cast<std::int32_t>(std::popcount(current.mask)) >=
                    minimumSpots &&
                prefix.config.spotAtCell.at(
                    static_cast<std::size_t>(current.cell)) !=
                    udon::kInvalidSpot &&
                emittedMasks.insert(current.mask).second) {
                udon::BrandMask brands;
                std::int32_t servingPotential = 0;
                for (std::size_t spot = 0;
                     spot < prefix.config.spots.size();
                     ++spot) {
                    if ((current.mask &
                         (std::uint32_t{1} << static_cast<std::uint32_t>(spot))) ==
                        0U) {
                        continue;
                    }
                    brands |= udon::brand_bit(
                        prefix.config.spots.at(spot).brandIndex);
                    servingPotential +=
                        prefix.config.spots.at(spot).stock > 0 ? 1 : 0;
                }
                std::int32_t terminalBrandDistance = 0;
                for (std::int32_t brand = 0;
                     brand < prefix.config.brand_count();
                     ++brand) {
                    std::int32_t nearest =
                        std::numeric_limits<std::int32_t>::max();
                    for (const udon::Spot& spot : prefix.config.spots) {
                        if (spot.brandIndex == brand) {
                            nearest = std::min(
                                nearest,
                                prefix.config.map.hex_distance(
                                    current.cell,
                                    spot.position));
                        }
                    }
                    if (nearest != std::numeric_limits<std::int32_t>::max()) {
                        terminalBrandDistance += nearest;
                    }
                }
                const auto rank = std::tuple{
                    static_cast<std::int32_t>(
                        udon::brand_intersection_count(brands, preferredBrands)),
                    udon::brand_count(brands),
                    servingPotential,
                    static_cast<std::int32_t>(std::popcount(current.mask)),
                    -static_cast<std::int32_t>(current.usedSteps),
                    -static_cast<std::int32_t>(current.usedFuel),
                    -terminalBrandDistance,
                    -current.cell,
                    std::numeric_limits<std::uint32_t>::max() - current.mask};
                SparseRouteChoice choice{labelIndex, current.mask, rank};
                if (retained.size() < maximumRoutes) {
                    retained.push_back(choice);
                } else {
                    const auto worst = std::min_element(
                        retained.begin(),
                        retained.end(),
                        [](const SparseRouteChoice& left,
                           const SparseRouteChoice& right) {
                            return left.rank < right.rank;
                        });
                    if (worst->rank < rank) {
                        *worst = choice;
                    }
                }
            }
            if (static_cast<std::int32_t>(std::popcount(current.mask)) >=
                    minimumSpots &&
                current.cell == baseline.finalAgents.at(
                    static_cast<std::size_t>(agent)).position &&
                protectedEmittedMasks.insert(current.mask).second) {
                udon::BrandMask brands;
                std::int32_t servingPotential = 0;
                for (std::size_t spot = 0;
                     spot < prefix.config.spots.size();
                     ++spot) {
                    if ((current.mask &
                         (std::uint32_t{1} << static_cast<std::uint32_t>(spot))) ==
                        0U) {
                        continue;
                    }
                    brands |= udon::brand_bit(
                        prefix.config.spots.at(spot).brandIndex);
                    servingPotential +=
                        prefix.config.spots.at(spot).stock > 0 ? 1 : 0;
                }
                const auto rank = std::tuple{
                    static_cast<std::int32_t>(
                        udon::brand_intersection_count(brands, preferredBrands)),
                    udon::brand_count(brands),
                    servingPotential,
                    static_cast<std::int32_t>(std::popcount(current.mask)),
                    -static_cast<std::int32_t>(current.usedSteps),
                    -static_cast<std::int32_t>(current.usedFuel),
                    0,
                    -current.cell,
                    std::numeric_limits<std::uint32_t>::max() - current.mask};
                SparseRouteChoice choice{labelIndex, current.mask, rank};
                if (protectedRetained.size() < maximumRoutes) {
                    protectedRetained.push_back(choice);
                } else {
                    const auto worst = std::min_element(
                        protectedRetained.begin(),
                        protectedRetained.end(),
                        [](const SparseRouteChoice& left,
                           const SparseRouteChoice& right) {
                            return left.rank < right.rank;
                        });
                    if (worst->rank < rank) {
                        *worst = choice;
                    }
                }
            }

            const udon::MoveCost move = moveCosts.at(
                static_cast<std::size_t>(current.cell));
            const std::int32_t nextSteps =
                static_cast<std::int32_t>(current.usedSteps) + move.steps;
            const std::int32_t nextFuel =
                static_cast<std::int32_t>(current.usedFuel) + move.patrolFuel;
            if (nextSteps > daySteps || nextFuel > agentState.fuel) {
                continue;
            }
            for (std::int32_t direction = 0;
                 direction < udon::kDirectionCount;
                 ++direction) {
                const udon::CellId destination = prefix.config.map.neighbors
                    .at(static_cast<std::size_t>(current.cell))
                    .at(static_cast<std::size_t>(direction));
                if (destination == udon::kInvalidCell ||
                    prefix.config.map.terrain.at(
                        static_cast<std::size_t>(destination)) ==
                        udon::Terrain::Pond) {
                    continue;
                }
                relax(
                    current.mask | destinationSpotBits.at(
                        static_cast<std::size_t>(destination)),
                    destination,
                    static_cast<std::uint16_t>(nextSteps),
                    static_cast<std::uint16_t>(nextFuel),
                    labelIndex,
                    static_cast<std::uint8_t>(direction));
            }
        }

        std::sort(
            retained.begin(),
            retained.end(),
            [](const SparseRouteChoice& left, const SparseRouteChoice& right) {
                return left.rank > right.rank;
            });
        retained.insert(
            retained.end(),
            protectedRetained.begin(),
            protectedRetained.end());
        std::sort(
            retained.begin(),
            retained.end(),
            [](const SparseRouteChoice& left, const SparseRouteChoice& right) {
                return left.rank > right.rank;
            });
        retained.erase(
            std::unique(
                retained.begin(),
                retained.end(),
                [](const SparseRouteChoice& left,
                   const SparseRouteChoice& right) {
                    return left.label == right.label;
                }),
            retained.end());
        udon::OfficialScore agentBest = baselineScore;
        udon::OfficialScore protectedAgentBest = baselineScore;
        std::uint32_t agentBestMask = 0U;
        std::uint32_t protectedAgentBestMask = 0U;
        std::int32_t validRoutes = 0;
        for (const SparseRouteChoice& choice : retained) {
            const SparseLabel& terminal = labels.at(
                static_cast<std::size_t>(choice.label));
            std::vector<udon::PlanAction> reversed;
            std::uint32_t current = choice.label;
            while (current != rootLabel) {
                const SparseLabel& label = labels.at(
                    static_cast<std::size_t>(current));
                if (label.incoming ==
                    static_cast<std::uint8_t>(udon::kDirectionCount)) {
                    reversed.push_back(udon::PlanAction::wait(1));
                } else if (label.incoming <
                           static_cast<std::uint8_t>(udon::kDirectionCount)) {
                    reversed.push_back(udon::PlanAction::move(label.incoming));
                } else {
                    throw std::runtime_error(
                        "sparse frontier predecessor chain is broken");
                }
                current = label.parent;
            }
            std::reverse(reversed.begin(), reversed.end());
            if (terminal.usedSteps < daySteps) {
                reversed.push_back(udon::PlanAction::wait(
                    daySteps - terminal.usedSteps));
            }
            udon::DayPlan mutation = frozenWitness;
            mutation.actions.at(static_cast<std::size_t>(agent)) =
                std::move(reversed);
            const udon::SimulationResult simulation = simulator.simulate(
                prefix.targetState,
                mutation,
                false);
            const udon::SimulationResult validation = validator.validate(
                prefix.targetState,
                mutation,
                false);
            std::string mismatch;
            if (!simulation.valid ||
                !validator.agrees_with(simulation, validation, mismatch)) {
                continue;
            }
            ++validRoutes;
            const udon::OfficialScore score = udon::OfficialScore::after_day(
                prefix.ledger,
                simulation.score);
            const bool exactRoad =
                simulation.roadFootprint == baseline.roadFootprint;
            if (exactRoad) {
                ++exactRoadEquivalent;
                const udon::BrandMask baselineLifetime =
                    prefix.ledger.lifetimeBrands | baseline.score.brands;
                const udon::BrandMask candidateLifetime =
                    prefix.ledger.lifetimeBrands | simulation.score.brands;
                const bool componentwiseNonRegressing =
                    baselineLifetime.is_subset_of(candidateLifetime) &&
                    simulation.score.dailyDistinct >= baseline.score.dailyDistinct &&
                    simulation.score.servings >= baseline.score.servings;
                const bool componentwiseStrict =
                    baselineLifetime != candidateLifetime ||
                    simulation.score.dailyDistinct > baseline.score.dailyDistinct ||
                    simulation.score.servings > baseline.score.servings;
                if (componentwiseNonRegressing && componentwiseStrict) {
                    ++strictRoadEquivalent;
                    const udon::AgentState& baselineAgent =
                        baseline.finalAgents.at(static_cast<std::size_t>(agent));
                    const udon::AgentState& candidateAgent =
                        simulation.finalAgents.at(static_cast<std::size_t>(agent));
                    const bool changedTerminal =
                        baselineAgent.position != candidateAgent.position;
                    if (changedTerminal) {
                        ++strictRoadEquivalentChangedTerminal;
                    }
                    if (roadEquivalentGlobalBest < score ||
                        (roadEquivalentGlobalBest == score &&
                         roadEquivalentGlobalAgent != udon::kInvalidAgent &&
                         std::pair{agent, choice.mask} <
                             std::pair{roadEquivalentGlobalAgent,
                                       roadEquivalentGlobalMask})) {
                        roadEquivalentGlobalBest = score;
                        roadEquivalentGlobalAgent = agent;
                        roadEquivalentGlobalMask = choice.mask;
                        roadEquivalentTerminalDistance =
                            prefix.config.map.hex_distance(
                                baselineAgent.position,
                                candidateAgent.position);
                        roadEquivalentFuelDelta =
                            candidateAgent.fuel - baselineAgent.fuel;
                    }
                }
            }
            bool nonIncreasingRoad =
                simulation.roadFootprint.size() == baseline.roadFootprint.size();
            bool strictlyLowerRoad = false;
            std::int64_t roadReduction = 0;
            if (nonIncreasingRoad) {
                for (std::size_t road = 0;
                     road < baseline.roadFootprint.size();
                     ++road) {
                    const std::int32_t parentDwell =
                        baseline.roadFootprint.at(road);
                    const std::int32_t candidateDwell =
                        simulation.roadFootprint.at(road);
                    if (candidateDwell > parentDwell) {
                        nonIncreasingRoad = false;
                        break;
                    }
                    strictlyLowerRoad =
                        strictlyLowerRoad || candidateDwell < parentDwell;
                    roadReduction += parentDwell - candidateDwell;
                }
            }
            if (nonIncreasingRoad && strictlyLowerRoad) {
                ++trafficNonIncreasing;
                const udon::BrandMask baselineLifetime =
                    prefix.ledger.lifetimeBrands | baseline.score.brands;
                const udon::BrandMask candidateLifetime =
                    prefix.ledger.lifetimeBrands | simulation.score.brands;
                const bool componentwiseNonRegressing =
                    baselineLifetime.is_subset_of(candidateLifetime) &&
                    simulation.score.dailyDistinct >= baseline.score.dailyDistinct &&
                    simulation.score.servings >= baseline.score.servings;
                const bool componentwiseStrict =
                    baselineLifetime != candidateLifetime ||
                    simulation.score.dailyDistinct > baseline.score.dailyDistinct ||
                    simulation.score.servings > baseline.score.servings;
                if (componentwiseNonRegressing && componentwiseStrict) {
                    ++strictTrafficNonIncreasing;
                    const udon::AgentState& baselineAgent =
                        baseline.finalAgents.at(static_cast<std::size_t>(agent));
                    const udon::AgentState& candidateAgent =
                        simulation.finalAgents.at(static_cast<std::size_t>(agent));
                    const bool changedTerminal =
                        baselineAgent.position != candidateAgent.position;
                    if (changedTerminal) {
                        ++strictTrafficNonIncreasingChangedTerminal;
                    }
                    if (trafficNonIncreasingGlobalBest < score ||
                        (trafficNonIncreasingGlobalBest == score &&
                         trafficNonIncreasingGlobalAgent != udon::kInvalidAgent &&
                         std::pair{agent, choice.mask} <
                             std::pair{trafficNonIncreasingGlobalAgent,
                                       trafficNonIncreasingGlobalMask})) {
                        trafficNonIncreasingGlobalBest = score;
                        trafficNonIncreasingGlobalAgent = agent;
                        trafficNonIncreasingGlobalMask = choice.mask;
                        trafficNonIncreasingTerminalDistance =
                            prefix.config.map.hex_distance(
                                baselineAgent.position,
                                candidateAgent.position);
                        trafficNonIncreasingFuelDelta =
                            candidateAgent.fuel - baselineAgent.fuel;
                        trafficNonIncreasingRoadReduction = roadReduction;
                    }
                }
            }
            if (agentBest < score) {
                agentBest = score;
                agentBestMask = choice.mask;
            }
            const udon::BrandMask baselineLifetime =
                prefix.ledger.lifetimeBrands | baseline.score.brands;
            const udon::BrandMask candidateLifetime =
                prefix.ledger.lifetimeBrands | simulation.score.brands;
            const bool protectedImprovement =
                udon::protected_slack_transition_dominates(
                    baseline,
                    simulation) &&
                baselineLifetime.is_subset_of(candidateLifetime) &&
                simulation.score.dailyDistinct >= baseline.score.dailyDistinct &&
                simulation.score.servings >= baseline.score.servings &&
                (simulation.score.dailyDistinct > baseline.score.dailyDistinct ||
                 simulation.score.servings > baseline.score.servings);
            if (protectedImprovement && protectedAgentBest < score) {
                protectedAgentBest = score;
                protectedAgentBestMask = choice.mask;
            }
            if (globalBest < score) {
                globalBest = score;
                globalAgent = agent;
                globalMask = choice.mask;
                globalPlan = std::move(mutation);
            }
            if (protectedImprovement && protectedGlobalBest < score) {
                protectedGlobalBest = score;
                protectedGlobalAgent = agent;
                protectedGlobalMask = choice.mask;
            }
        }
        std::cout << "sparse_agent=" << agent
                  << " cap=" << maximumSettledStates
                  << " settled=" << settledStates
                  << " labels=" << labels.size()
                  << " emitted_masks=" << emittedMasks.size()
                  << " protected_masks=" << protectedEmittedMasks.size()
                  << " retained=" << retained.size()
                  << " valid=" << validRoutes
                  << " best=" << agentBest.lifetimeDistinct << '/'
                  << agentBest.totalDailyDistinct << '/'
                  << agentBest.totalServings
                  << " mask=0x" << std::hex << std::uppercase
                  << agentBestMask << std::dec << std::nouppercase
                  << " protected=" << protectedAgentBest.lifetimeDistinct << '/'
                  << protectedAgentBest.totalDailyDistinct << '/'
                  << protectedAgentBest.totalServings
                  << " protected_mask=0x" << std::hex << std::uppercase
                  << protectedAgentBestMask << std::dec << std::nouppercase
                  << '\n';
    }
    std::cout << "sparse_global_best=" << globalBest.lifetimeDistinct << '/'
              << globalBest.totalDailyDistinct << '/'
              << globalBest.totalServings
              << " baseline=" << baselineScore.lifetimeDistinct << '/'
              << baselineScore.totalDailyDistinct << '/'
              << baselineScore.totalServings
              << " agent=" << globalAgent
              << " mask=0x" << std::hex << std::uppercase << globalMask
              << std::dec << std::nouppercase << '\n';
    std::cout << "sparse_protected_best="
              << protectedGlobalBest.lifetimeDistinct << '/'
              << protectedGlobalBest.totalDailyDistinct << '/'
              << protectedGlobalBest.totalServings
              << " baseline=" << baselineScore.lifetimeDistinct << '/'
              << baselineScore.totalDailyDistinct << '/'
              << baselineScore.totalServings
              << " agent=" << protectedGlobalAgent
              << " mask=0x" << std::hex << std::uppercase
              << protectedGlobalMask << std::dec << std::nouppercase << '\n';
    std::cout << "road_equivalent_summary exact=" << exactRoadEquivalent
              << " strict=" << strictRoadEquivalent
              << " strict_changed_terminal="
              << strictRoadEquivalentChangedTerminal
              << " baseline=" << baselineScore.lifetimeDistinct << '/'
              << baselineScore.totalDailyDistinct << '/'
              << baselineScore.totalServings
              << " best=" << roadEquivalentGlobalBest.lifetimeDistinct << '/'
              << roadEquivalentGlobalBest.totalDailyDistinct << '/'
              << roadEquivalentGlobalBest.totalServings
              << " agent=" << roadEquivalentGlobalAgent
              << " mask=0x" << std::hex << std::uppercase
              << roadEquivalentGlobalMask << std::dec << std::nouppercase
              << " terminal_distance=" << roadEquivalentTerminalDistance
              << " fuel_delta=" << roadEquivalentFuelDelta << '\n';
    std::cout << "traffic_nonincreasing_summary candidates="
              << trafficNonIncreasing
              << " strict=" << strictTrafficNonIncreasing
              << " strict_changed_terminal="
              << strictTrafficNonIncreasingChangedTerminal
              << " baseline=" << baselineScore.lifetimeDistinct << '/'
              << baselineScore.totalDailyDistinct << '/'
              << baselineScore.totalServings
              << " best=" << trafficNonIncreasingGlobalBest.lifetimeDistinct
              << '/' << trafficNonIncreasingGlobalBest.totalDailyDistinct
              << '/' << trafficNonIncreasingGlobalBest.totalServings
              << " agent=" << trafficNonIncreasingGlobalAgent
              << " mask=0x" << std::hex << std::uppercase
              << trafficNonIncreasingGlobalMask << std::dec << std::nouppercase
              << " terminal_distance="
              << trafficNonIncreasingTerminalDistance
              << " fuel_delta=" << trafficNonIncreasingFuelDelta
              << " road_reduction=" << trafficNonIncreasingRoadReduction
              << '\n';
    if (baselineScore < globalBest) {
        std::cout << "sparse_best_plan="
                  << udon::serialize_day_plan(globalPlan).dump() << '\n';
    }
    if (prefix.targetState.dayNumber == prefix.config.day_count()) {
        udon::ProtectedSlackRefiner refiner(prefix.config);
        refiner.enableTerminalPairExchange = true;
        refiner.enableTerminalMarginalReservoir = enableTerminalMarginal;
        const udon::ProtectedSlackResult sidecar =
            refiner.refine_terminal_sparse(
                prefix.targetState,
                prefix.ledger,
                frozenWitness,
                baseline,
                std::chrono::steady_clock::now() +
                    std::chrono::milliseconds{5000});
        std::cout << "terminal_sidecar="
                  << sidecar.scoreAfterToday.lifetimeDistinct << '/'
                  << sidecar.scoreAfterToday.totalDailyDistinct << '/'
                  << sidecar.scoreAfterToday.totalServings
                  << " improved=" << sidecar.improved
                  << " agent=" << sidecar.witnessAgent
                  << " routes=" << sidecar.diagnostics.sparseRoutes
                  << " generated=" << sidecar.diagnostics.generatedPlans
                  << " valid=" << sidecar.diagnostics.validPlans
                  << " strict="
                  << sidecar.diagnostics.strictTerminalImprovements
                  << " rounds="
                  << sidecar.diagnostics.terminalSparseRounds
                  << " round1="
                  << sidecar.firstRoundScore.lifetimeDistinct << '/'
                  << sidecar.firstRoundScore.totalDailyDistinct << '/'
                  << sidecar.firstRoundScore.totalServings
                  << " canonical="
                  << sidecar.canonicalTerminalScore.lifetimeDistinct << '/'
                  << sidecar.canonicalTerminalScore.totalDailyDistinct << '/'
                  << sidecar.canonicalTerminalScore.totalServings
                  << " marginal_routes="
                  << sidecar.diagnostics.terminalMarginalRoutes
                  << " marginal_valid="
                  << sidecar.diagnostics.terminalMarginalValidPlans
                  << " marginal_acceptances="
                  << sidecar.diagnostics.terminalMarginalAcceptances
                  << " deadline=" << sidecar.diagnostics.deadlineReached
                  << '\n';
    }
    return globalPlan;
}

[[nodiscard]] std::vector<std::vector<std::int32_t>>
robust_terminal_geometries(
    const udon::MatchConfig& config,
    const udon::DayState& state,
    const udon::DayPlan& sourcePlan,
    std::int32_t& filteredPatrols) {
    std::unordered_set<udon::CellId> stationaryTankers;
    for (const udon::AgentState& agent : state.agents) {
        if (agent.kind == udon::AgentKind::Tanker) {
            stationaryTankers.insert(agent.position);
        }
    }

    std::vector<std::vector<std::int32_t>> geometries(
        static_cast<std::size_t>(config.agent_count()));
    filteredPatrols = 0;
    for (udon::AgentIndex agent = 0;
         agent < config.agent_count();
         ++agent) {
        const udon::AgentState& initial = state.agents.at(
            static_cast<std::size_t>(agent));
        if (initial.kind != udon::AgentKind::Patrol) {
            continue;
        }
        bool blocked = stationaryTankers.contains(initial.position);
        std::int32_t fuelUsed = 0;
        udon::CellId cell = initial.position;
        auto& geometry = geometries.at(static_cast<std::size_t>(agent));
        for (const udon::PlanAction& action : sourcePlan.actions.at(
                 static_cast<std::size_t>(agent))) {
            if (action.kind != udon::ActionKind::Move) {
                continue;
            }
            if (action.value < 0 || action.value >= udon::kDirectionCount) {
                throw std::runtime_error(
                    "terminal source plan has an invalid direction");
            }
            const udon::CellId destination = config.map.neighbors.at(
                static_cast<std::size_t>(cell)).at(
                    static_cast<std::size_t>(action.value));
            if (destination == udon::kInvalidCell ||
                config.map.terrain.at(static_cast<std::size_t>(destination)) ==
                    udon::Terrain::Pond) {
                throw std::runtime_error(
                    "terminal source plan has an invalid geometric move");
            }
            fuelUsed += config.move_cost(
                cell,
                udon::RoadStatus::Jammed).patrolFuel;
            geometry.push_back(action.value);
            cell = destination;
            blocked = blocked || stationaryTankers.contains(cell);
        }
        blocked = blocked || fuelUsed > initial.fuel;
        if (blocked) {
            geometry.clear();
            ++filteredPatrols;
        }
    }
    return geometries;
}

[[nodiscard]] udon::DayPlan compile_robust_terminal_plan(
    const udon::MatchConfig& config,
    const udon::DayState& state,
    const std::vector<std::vector<std::int32_t>>& geometries) {
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    udon::DayPlan plan;
    plan.actions.resize(static_cast<std::size_t>(config.agent_count()));
    for (udon::AgentIndex agent = 0;
         agent < config.agent_count();
         ++agent) {
        const udon::AgentState& initial = state.agents.at(
            static_cast<std::size_t>(agent));
        udon::AgentPlan& actions = plan.actions.at(
            static_cast<std::size_t>(agent));
        if (initial.kind == udon::AgentKind::Tanker ||
            geometries.at(static_cast<std::size_t>(agent)).empty()) {
            actions.push_back(udon::PlanAction::wait(daySteps));
            continue;
        }
        std::int32_t usedSteps = 0;
        udon::CellId cell = initial.position;
        for (const std::int32_t direction : geometries.at(
                 static_cast<std::size_t>(agent))) {
            const udon::MoveCost cost = config.move_cost(
                cell,
                state.roadStatuses.at(static_cast<std::size_t>(cell)));
            usedSteps += cost.steps;
            if (usedSteps > daySteps) {
                throw std::runtime_error(
                    "robust terminal geometry exceeds the day");
            }
            actions.push_back(udon::PlanAction::move(direction));
            cell = config.map.neighbors.at(static_cast<std::size_t>(cell)).at(
                static_cast<std::size_t>(direction));
        }
        if (usedSteps < daySteps) {
            actions.push_back(udon::PlanAction::wait(daySteps - usedSteps));
        }
    }
    return plan;
}

[[nodiscard]] std::vector<std::vector<std::int32_t>>
stationary_refuel_terminal_geometries(
    const udon::MatchConfig& config,
    const udon::DayState& state,
    const udon::DayPlan& sourcePlan,
    std::int32_t& truncatedPatrols,
    std::int32_t& stationaryRefuels) {
    std::unordered_set<udon::CellId> stationaryTankers;
    for (const udon::AgentState& agent : state.agents) {
        if (agent.kind == udon::AgentKind::Tanker) {
            stationaryTankers.insert(agent.position);
        }
    }

    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    std::vector<std::vector<std::int32_t>> geometries(
        static_cast<std::size_t>(config.agent_count()));
    truncatedPatrols = 0;
    stationaryRefuels = 0;
    for (udon::AgentIndex agent = 0;
         agent < config.agent_count();
         ++agent) {
        const udon::AgentState& initial = state.agents.at(
            static_cast<std::size_t>(agent));
        if (initial.kind != udon::AgentKind::Patrol) {
            continue;
        }
        std::int32_t usedSteps = 0;
        std::int32_t availableFuel = initial.fuel;
        udon::CellId cell = initial.position;
        bool truncated = false;
        auto& geometry = geometries.at(static_cast<std::size_t>(agent));
        for (const udon::PlanAction& action : sourcePlan.actions.at(
                 static_cast<std::size_t>(agent))) {
            if (action.kind != udon::ActionKind::Move) {
                continue;
            }
            if (action.value < 0 || action.value >= udon::kDirectionCount) {
                throw std::runtime_error(
                    "terminal source plan has an invalid direction");
            }
            const bool refuelBeforeMove = stationaryTankers.contains(cell);
            const udon::MoveCost cost = config.move_cost(
                cell,
                udon::RoadStatus::Jammed);
            const std::int32_t refuelSteps = refuelBeforeMove ? 1 : 0;
            const std::int32_t fuelBeforeMove = refuelBeforeMove
                ? config.fuelLimit
                : availableFuel;
            if (usedSteps + refuelSteps + cost.steps > daySteps ||
                fuelBeforeMove < cost.patrolFuel) {
                truncated = true;
                break;
            }
            if (refuelBeforeMove) {
                ++usedSteps;
                availableFuel = config.fuelLimit;
                ++stationaryRefuels;
            }
            const udon::CellId destination = config.map.neighbors.at(
                static_cast<std::size_t>(cell)).at(
                    static_cast<std::size_t>(action.value));
            if (destination == udon::kInvalidCell ||
                config.map.terrain.at(static_cast<std::size_t>(destination)) ==
                    udon::Terrain::Pond) {
                throw std::runtime_error(
                    "terminal source plan has an invalid geometric move");
            }
            usedSteps += cost.steps;
            availableFuel -= cost.patrolFuel;
            geometry.push_back(action.value);
            cell = destination;
        }
        if (truncated) {
            ++truncatedPatrols;
        }
    }
    return geometries;
}

[[nodiscard]] udon::DayPlan compile_stationary_refuel_terminal_plan(
    const udon::MatchConfig& config,
    const udon::DayState& state,
    const std::vector<std::vector<std::int32_t>>& geometries) {
    std::unordered_set<udon::CellId> stationaryTankers;
    for (const udon::AgentState& agent : state.agents) {
        if (agent.kind == udon::AgentKind::Tanker) {
            stationaryTankers.insert(agent.position);
        }
    }
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    udon::DayPlan plan;
    plan.actions.resize(static_cast<std::size_t>(config.agent_count()));
    for (udon::AgentIndex agent = 0;
         agent < config.agent_count();
         ++agent) {
        const udon::AgentState& initial = state.agents.at(
            static_cast<std::size_t>(agent));
        udon::AgentPlan& actions = plan.actions.at(
            static_cast<std::size_t>(agent));
        if (initial.kind == udon::AgentKind::Tanker ||
            geometries.at(static_cast<std::size_t>(agent)).empty()) {
            actions.push_back(udon::PlanAction::wait(daySteps));
            continue;
        }
        std::int32_t usedSteps = 0;
        udon::CellId cell = initial.position;
        for (const std::int32_t direction : geometries.at(
                 static_cast<std::size_t>(agent))) {
            if (stationaryTankers.contains(cell)) {
                actions.push_back(udon::PlanAction::wait(1));
                ++usedSteps;
            }
            const udon::MoveCost cost = config.move_cost(
                cell,
                state.roadStatuses.at(static_cast<std::size_t>(cell)));
            usedSteps += cost.steps;
            if (usedSteps > daySteps) {
                throw std::runtime_error(
                    "stationary-refuel geometry exceeds the day");
            }
            actions.push_back(udon::PlanAction::move(direction));
            cell = config.map.neighbors.at(static_cast<std::size_t>(cell)).at(
                static_cast<std::size_t>(direction));
        }
        if (usedSteps < daySteps) {
            actions.push_back(udon::PlanAction::wait(daySteps - usedSteps));
        }
    }
    return plan;
}

[[nodiscard]] bool equal_day_score(
    const udon::DayScore& left,
    const udon::DayScore& right) {
    return left.brands == right.brands &&
        left.dailyDistinct == right.dailyDistinct &&
        left.servings == right.servings;
}

void print_penultimate_robust_terminal_bound(
    const ReplayPrefix& prefix,
    const udon::DayPlan& frozenWitness,
    std::uint64_t maximumSettledStates,
    std::size_t maximumRoutes,
    bool stationaryRefuel) {
    if (prefix.targetState.dayNumber != prefix.config.day_count() - 1) {
        throw std::invalid_argument(
            "robust terminal bound requires the penultimate day");
    }

    const udon::ExactStepSimulator simulator(prefix.config);
    const udon::IndependentDayValidator validator(prefix.config);
    const udon::SimulationResult baseline = simulator.simulate(
        prefix.targetState,
        frozenWitness,
        false);
    const udon::DayPlan sparse = print_sparse_exchange(
        prefix,
        frozenWitness,
        maximumSettledStates,
        udon::kInvalidAgent,
        maximumRoutes);
    const udon::SimulationResult candidate = simulator.simulate(
        prefix.targetState,
        sparse,
        false);
    const udon::SimulationResult candidateValidation = validator.validate(
        prefix.targetState,
        sparse,
        false);
    std::string candidateMismatch;
    if (!baseline.valid || !candidate.valid ||
        !validator.agrees_with(
            candidate,
            candidateValidation,
            candidateMismatch)) {
        throw std::runtime_error(
            "penultimate sparse candidate failed exact agreement: " +
            candidateMismatch);
    }

    const udon::BrandMask baselineLifetime =
        prefix.ledger.lifetimeBrands | baseline.score.brands;
    const udon::BrandMask candidateLifetime =
        prefix.ledger.lifetimeBrands | candidate.score.brands;
    const bool currentNonRegressing =
        baselineLifetime.is_subset_of(candidateLifetime) &&
        candidate.score.dailyDistinct >= baseline.score.dailyDistinct &&
        candidate.score.servings >= baseline.score.servings;
    const bool currentStrict = currentNonRegressing &&
        (baselineLifetime != candidateLifetime ||
         candidate.score.dailyDistinct > baseline.score.dailyDistinct ||
         candidate.score.servings > baseline.score.servings);
    const udon::OfficialScore baselineCurrent = udon::OfficialScore::after_day(
        prefix.ledger,
        baseline.score);
    const udon::OfficialScore candidateCurrent = udon::OfficialScore::after_day(
        prefix.ledger,
        candidate.score);
    if (!currentStrict) {
        std::cout << "penultimate_robust_summary current_strict=0"
                  << " lower_valid=0 closes=0"
                  << " parent_current=" << baselineCurrent.lifetimeDistinct
                  << '/' << baselineCurrent.totalDailyDistinct << '/'
                  << baselineCurrent.totalServings
                  << " candidate_current="
                  << candidateCurrent.lifetimeDistinct << '/'
                  << candidateCurrent.totalDailyDistinct << '/'
                  << candidateCurrent.totalServings << '\n';
        return;
    }

    udon::MatchLedger candidateLedger = prefix.ledger;
    candidateLedger.apply(candidate.score);
    udon::DayState terminalState = prefix.targetState;
    terminalState.endsAt = 0;
    ++terminalState.dayNumber;
    terminalState.agents = candidate.finalAgents;
    terminalState.others.clear();
    terminalState.roadStatuses.assign(
        static_cast<std::size_t>(prefix.config.map.cell_count()),
        udon::RoadStatus::Jammed);

    udon::DeadlineCalibration calibration;
    calibration.version = "btc-http-local-budget-v8-idempotent-ack-resend";
    calibration.networkFloor = std::chrono::milliseconds{1600};
    calibration.networkPercent = 20;
    calibration.certificationPercent = 20;
    const std::int32_t solveBudgetMs = static_cast<std::int32_t>(
        udon::competition_compute_budget(
            std::chrono::milliseconds{5000}).count());
    calibration.normalThreshold = std::chrono::milliseconds{solveBudgetMs};
    calibration.version += "-logic-normal";
    udon::UdonShieldEngine engine(
        prefix.config,
        {},
        calibration,
        udon::RoutePoolSearch::SinglePass,
        7,
        false,
        7);
    engine.set_short_horizon_role_fallback(true);
    static_cast<void>(engine.select_roles_until(
        std::chrono::milliseconds{solveBudgetMs},
        3));
    const std::chrono::steady_clock::time_point solveStarted =
        std::chrono::steady_clock::now();
    const udon::DecisionResult terminalDecision = engine.solve_day(
        terminalState,
        candidateLedger,
        std::chrono::milliseconds{solveBudgetMs});
    const std::int64_t terminalSolveMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - solveStarted).count();

    std::int32_t filteredPatrols = 0;
    std::int32_t truncatedPatrols = 0;
    std::int32_t stationaryRefuels = 0;
    const std::vector<std::vector<std::int32_t>> geometries = stationaryRefuel
        ? stationary_refuel_terminal_geometries(
              prefix.config,
              terminalState,
              terminalDecision.candidate.plan,
              truncatedPatrols,
              stationaryRefuels)
        : robust_terminal_geometries(
              prefix.config,
              terminalState,
              terminalDecision.candidate.plan,
              filteredPatrols);
    std::array<udon::RoadStatus, 3> statuses{
        udon::RoadStatus::Jammed,
        udon::RoadStatus::Busy,
        udon::RoadStatus::Smooth,
    };
    std::array<udon::DayScore, 3> terminalScores;
    bool lowerValid = true;
    for (std::size_t index = 0; index < statuses.size(); ++index) {
        udon::DayState state = terminalState;
        state.roadStatuses.assign(
            static_cast<std::size_t>(prefix.config.map.cell_count()),
            statuses.at(index));
        const udon::DayPlan plan = stationaryRefuel
            ? compile_stationary_refuel_terminal_plan(
                  prefix.config,
                  state,
                  geometries)
            : compile_robust_terminal_plan(
                  prefix.config,
                  state,
                  geometries);
        const udon::SimulationResult result = simulator.simulate(
            state,
            plan,
            false);
        const udon::SimulationResult validation = validator.validate(
            state,
            plan,
            false);
        std::string mismatch;
        if (!result.valid ||
            !validator.agrees_with(result, validation, mismatch)) {
            lowerValid = false;
            break;
        }
        terminalScores.at(index) = result.score;
        if (index > 0U &&
            !equal_day_score(terminalScores.front(), terminalScores.at(index))) {
            lowerValid = false;
            break;
        }
    }

    udon::OfficialScore candidateLower = candidateCurrent;
    if (lowerValid) {
        candidateLower = udon::OfficialScore::after_day(
            candidateLedger,
            terminalScores.front());
    }
    udon::MatchLedger parentLedger = prefix.ledger;
    parentLedger.apply(baseline.score);
    const std::int32_t patrolCount = static_cast<std::int32_t>(
        std::count_if(
            baseline.finalAgents.begin(),
            baseline.finalAgents.end(),
            [](const udon::AgentState& agent) {
                return agent.kind == udon::AgentKind::Patrol;
            }));
    std::int32_t terminalServingUpper = 0;
    for (const udon::Spot& spot : prefix.config.spots) {
        terminalServingUpper += std::min(spot.stock, patrolCount);
    }
    const udon::OfficialScore parentUpper{
        prefix.config.brand_count(),
        parentLedger.totalDailyDistinct + prefix.config.brand_count(),
        parentLedger.totalServings + terminalServingUpper,
    };
    const bool closes = lowerValid && parentUpper < candidateLower;
    std::cout << "penultimate_robust_summary"
              << " current_strict=1"
              << " lower_valid=" << (lowerValid ? 1 : 0)
              << " closes=" << (closes ? 1 : 0)
              << " parent_current=" << baselineCurrent.lifetimeDistinct << '/'
              << baselineCurrent.totalDailyDistinct << '/'
              << baselineCurrent.totalServings
              << " candidate_current=" << candidateCurrent.lifetimeDistinct
              << '/' << candidateCurrent.totalDailyDistinct << '/'
              << candidateCurrent.totalServings
              << " lower=" << candidateLower.lifetimeDistinct << '/'
              << candidateLower.totalDailyDistinct << '/'
              << candidateLower.totalServings
              << " parent_upper=" << parentUpper.lifetimeDistinct << '/'
              << parentUpper.totalDailyDistinct << '/'
              << parentUpper.totalServings
              << " terminal_day=" << terminalScores.front().dailyDistinct
              << '/' << terminalScores.front().servings
              << " serving_upper=" << terminalServingUpper
              << " patrols=" << patrolCount
              << " filtered_patrols=" << filteredPatrols
              << " truncated_patrols=" << truncatedPatrols
              << " stationary_refuels=" << stationaryRefuels
              << " stationary_refuel_mode=" << (stationaryRefuel ? 1 : 0)
              << " terminal_solve_ms=" << terminalSolveMs
              << '\n';
}

void print_sparse_canonical_stage_membership(
    const ReplayPrefix& prefix,
    const udon::DayPlan& frozenWitness) {
    constexpr std::uint64_t kMaximumSparseStates = 50000U;
    constexpr std::size_t kMaximumSparseRoutes = 32U;

    const udon::ExactStepSimulator simulator(prefix.config);
    const udon::IndependentDayValidator validator(prefix.config);
    const udon::SimulationResult baseline = simulator.simulate(
        prefix.targetState,
        frozenWitness,
        false);
    const udon::DayPlan sparse = print_sparse_exchange(
        prefix,
        frozenWitness,
        kMaximumSparseStates,
        udon::kInvalidAgent,
        kMaximumSparseRoutes);
    const udon::SimulationResult sparseSimulation = simulator.simulate(
        prefix.targetState,
        sparse,
        false);
    const udon::SimulationResult sparseValidation = validator.validate(
        prefix.targetState,
        sparse,
        false);
    std::string sparseMismatch;
    if (!baseline.valid || !sparseSimulation.valid ||
        !validator.agrees_with(
            sparseSimulation,
            sparseValidation,
            sparseMismatch)) {
        throw std::runtime_error(
            "sparse canonical-stage witness failed exact agreement: " +
            sparseMismatch);
    }
    const udon::OfficialScore baselineScore = udon::OfficialScore::after_day(
        prefix.ledger,
        baseline.score);
    const udon::OfficialScore sparseScore = udon::OfficialScore::after_day(
        prefix.ledger,
        sparseSimulation.score);
    const bool strictGain =
        udon::compare_lexicographic(sparseScore, baselineScore) > 0;

    const auto same_agent_actions = [](
        const udon::AgentPlan& left,
        const udon::AgentPlan& right) {
        if (left.size() != right.size()) {
            return false;
        }
        for (std::size_t index = 0U; index < left.size(); ++index) {
            if (left.at(index).kind != right.at(index).kind ||
                left.at(index).value != right.at(index).value) {
                return false;
            }
        }
        return true;
    };

    udon::AgentIndex changedAgent = udon::kInvalidAgent;
    std::int32_t changedAgents = 0;
    for (udon::AgentIndex agent = 0;
         agent < prefix.config.agent_count();
         ++agent) {
        if (!same_agent_actions(
                frozenWitness.actions.at(static_cast<std::size_t>(agent)),
                sparse.actions.at(static_cast<std::size_t>(agent)))) {
            changedAgent = agent;
            ++changedAgents;
        }
    }
    if (!strictGain || changedAgents != 1) {
        std::cout << "stage_membership strict=" << (strictGain ? 1 : 0)
                  << " changed_agents=" << changedAgents
                  << " route_present=0 full_plan_present=0"
                  << " baseline=" << baselineScore.lifetimeDistinct << '/'
                  << baselineScore.totalDailyDistinct << '/'
                  << baselineScore.totalServings
                  << " sparse=" << sparseScore.lifetimeDistinct << '/'
                  << sparseScore.totalDailyDistinct << '/'
                  << sparseScore.totalServings << '\n';
        return;
    }

    const udon::ParetoRouter router(prefix.config);
    const udon::RouteColumnGenerator generator(prefix.config, router);
    udon::ColumnGenerationOptions generation;
    generation.maximumPathsPerTarget = 4;
    generation.maximumColumnsPerAgent = 16;
    generation.maximumTargetSpots = 12;
    generation.maximumEscorts = 16;
    generation.maximumSeedPlans = 2;
    generation.seedPlans.push_back(frozenWitness);
    generation.enableHarvestExtensions = true;
    generation.allowUncachedHarvestTargets = true;
    generation.enableHarvestOrienteering = true;
    generation.enableExactHarvestOrienteering = true;
    generation.enableFuelConstrainedExactHarvestOrienteering = false;
    generation.enableAnytimeFuelConstrainedHarvestOrienteering = false;
    generation.maximumHarvestExtensionSources = 4;
    generation.maximumHarvestExtensionDepth = 4;
    udon::ColumnGenerationDiagnostics generationDiagnostics;
    const udon::RoutePortfolio portfolio = generator.generate(
        prefix.targetState,
        prefix.ledger,
        generation,
        &generationDiagnostics);
    const std::vector<udon::RouteColumn>& changedColumns =
        portfolio.columnsByAgent.at(static_cast<std::size_t>(changedAgent));
    const bool routePresent = std::any_of(
        changedColumns.begin(),
        changedColumns.end(),
        [&sparse, changedAgent, &same_agent_actions](
            const udon::RouteColumn& column) {
            return same_agent_actions(
                column.actions,
                sparse.actions.at(static_cast<std::size_t>(changedAgent)));
        });

    const udon::RouteMaster master(prefix.config, simulator, validator);
    udon::MasterOptions masterOptions;
    masterOptions.maximumCombinations = 40000;
    masterOptions.maximumCandidates = 32;
    masterOptions.diversityCandidates = 8;
    masterOptions.preferBaselineHarvestSources = true;
    udon::MasterDiagnostics masterDiagnostics;
    const std::vector<udon::MasterCandidate> candidates = master.solve(
        prefix.targetState,
        prefix.ledger,
        portfolio,
        masterOptions,
        masterDiagnostics);
    const std::string sparseBytes = udon::canonical_plan_bytes(sparse);
    const bool fullPlanPresent = std::any_of(
        candidates.begin(),
        candidates.end(),
        [&sparseBytes](const udon::MasterCandidate& candidate) {
            return udon::canonical_plan_bytes(candidate.plan) == sparseBytes;
        });

    std::int32_t portfolioColumns = 0;
    for (const std::vector<udon::RouteColumn>& columns :
         portfolio.columnsByAgent) {
        portfolioColumns += static_cast<std::int32_t>(columns.size());
    }
    std::cout << "stage_membership strict=1 changed_agents=1 agent="
              << changedAgent
              << " route_present=" << (routePresent ? 1 : 0)
              << " full_plan_present=" << (fullPlanPresent ? 1 : 0)
              << " baseline=" << baselineScore.lifetimeDistinct << '/'
              << baselineScore.totalDailyDistinct << '/'
              << baselineScore.totalServings
              << " sparse=" << sparseScore.lifetimeDistinct << '/'
              << sparseScore.totalDailyDistinct << '/'
              << sparseScore.totalServings
              << " portfolio_columns=" << portfolioColumns
              << " agent_columns=" << changedColumns.size()
              << " exact_supported="
              << generationDiagnostics.exactOrienteeringSupportedAgents
              << " exact_bundles="
              << generationDiagnostics.exactOrienteeringBundles
              << " candidates=" << candidates.size()
              << " combinations=" << masterDiagnostics.combinationsVisited
              << " master_complete=" << masterDiagnostics.searchComplete
              << " master_deadline=" << masterDiagnostics.deadlineReached
              << '\n';
}

void print_fuel_dominating_agent_permutation(
    const ReplayPrefix& prefix,
    const udon::DayPlan& frozenWitness,
    std::uint64_t maximumSettledStates,
    std::size_t maximumRoutes,
    bool trafficDominance = false) {
    const udon::ExactStepSimulator simulator(prefix.config);
    const udon::IndependentDayValidator validator(prefix.config);
    const udon::SimulationResult baseline = simulator.simulate(
        prefix.targetState,
        frozenWitness,
        false);
    const udon::SimulationResult baselineValidation = validator.validate(
        prefix.targetState,
        frozenWitness,
        false);
    std::string baselineMismatch;
    if (!baseline.valid || !validator.agrees_with(
            baseline,
            baselineValidation,
            baselineMismatch)) {
        throw std::runtime_error(
            "fuel-permutation baseline failed exact agreement: " +
            baselineMismatch);
    }
    const std::string incumbentBytes = udon::canonical_plan_bytes(frozenWitness);
    const udon::OfficialScore baselineScore = udon::OfficialScore::after_day(
        prefix.ledger,
        baseline.score);
    udon::OfficialScore bestScore = baselineScore;
    udon::AgentIndex bestLeft = udon::kInvalidAgent;
    udon::AgentIndex bestRight = udon::kInvalidAgent;
    std::int32_t bestMinimumFuelDelta = 0;
    std::int32_t bestMaximumFuelDelta = 0;

    udon::BrandMask preferredBrands;
    for (std::int32_t brand = 0;
         brand < prefix.config.brand_count();
         ++brand) {
        if (!udon::has_brand(prefix.ledger.lifetimeBrands, brand)) {
            preferredBrands |= udon::brand_bit(brand);
        }
    }
    const bool missingLifetimeBrand = preferredBrands.any();
    if (!missingLifetimeBrand) {
        for (std::int32_t brand = 0;
             brand < prefix.config.brand_count();
             ++brand) {
            if (!udon::has_brand(baseline.score.brands, brand)) {
                preferredBrands |= udon::brand_bit(brand);
            }
        }
    }
    const std::int32_t minimumSpots = missingLifetimeBrand
        ? 1
        : std::min<std::int32_t>(
              std::max(1, prefix.config.brand_count() - 1),
              static_cast<std::int32_t>(prefix.config.spots.size()));

    std::vector<udon::AgentIndex> patrols;
    for (udon::AgentIndex agent = 0;
         agent < prefix.config.agent_count();
         ++agent) {
        if (prefix.targetState.agents.at(
                static_cast<std::size_t>(agent)).kind ==
            udon::AgentKind::Patrol) {
            patrols.push_back(agent);
        }
    }

    std::int64_t tasks = 0;
    std::int64_t routes = 0;
    std::int64_t crossPairs = 0;
    std::int64_t invalidPlans = 0;
    std::int64_t validatorMismatches = 0;
    std::int64_t validPlans = 0;
    std::int64_t roadEquivalent = 0;
    std::int64_t strictRoadDominating = 0;
    std::int64_t mappedStateDominating = 0;
    std::int64_t exactFuelCertified = 0;
    std::int64_t strictFuelCertified = 0;
    std::int64_t trafficCertified = 0;
    std::int64_t bestRoadReduction = 0;

    const auto append_routes = [](
        const udon::ExactOrienteeringReachability& reachability,
        std::vector<const udon::ExactOrienteeringRoute*>& output) {
        for (const udon::ExactOrienteeringRoute& route :
             reachability.maximalRoutes) {
            output.push_back(&route);
        }
        for (const udon::ExactOrienteeringRoute& route :
             reachability.supplementalRoutes) {
            output.push_back(&route);
        }
    };

    for (std::size_t leftOffset = 0;
         leftOffset + 1U < patrols.size();
         ++leftOffset) {
        for (std::size_t rightOffset = leftOffset + 1U;
             rightOffset < patrols.size();
             ++rightOffset) {
            const udon::AgentIndex left = patrols.at(leftOffset);
            const udon::AgentIndex right = patrols.at(rightOffset);
            const udon::CellId leftTerminal = baseline.finalAgents.at(
                static_cast<std::size_t>(left)).position;
            const udon::CellId rightTerminal = baseline.finalAgents.at(
                static_cast<std::size_t>(right)).position;
            const udon::ExactOrienteeringReachability leftReachability =
                udon::enumerate_sparse_anytime_resource_routes_to_terminal(
                    prefix.config,
                    prefix.targetState,
                    left,
                    rightTerminal,
                    minimumSpots,
                    maximumRoutes,
                    maximumSettledStates,
                    std::nullopt,
                    preferredBrands);
            const udon::ExactOrienteeringReachability rightReachability =
                udon::enumerate_sparse_anytime_resource_routes_to_terminal(
                    prefix.config,
                    prefix.targetState,
                    right,
                    leftTerminal,
                    minimumSpots,
                    maximumRoutes,
                    maximumSettledStates,
                    std::nullopt,
                    preferredBrands);
            tasks += 2;
            std::vector<const udon::ExactOrienteeringRoute*> leftRoutes;
            std::vector<const udon::ExactOrienteeringRoute*> rightRoutes;
            append_routes(leftReachability, leftRoutes);
            append_routes(rightReachability, rightRoutes);
            routes += static_cast<std::int64_t>(
                leftRoutes.size() + rightRoutes.size());

            for (const udon::ExactOrienteeringRoute* leftRoute : leftRoutes) {
                for (const udon::ExactOrienteeringRoute* rightRoute :
                     rightRoutes) {
                    ++crossPairs;
                    udon::DayPlan candidate = frozenWitness;
                    candidate.actions.at(static_cast<std::size_t>(left)) =
                        leftRoute->actions;
                    candidate.actions.at(static_cast<std::size_t>(right)) =
                        rightRoute->actions;
                    const udon::SimulationResult simulation =
                        simulator.simulate(prefix.targetState, candidate, false);
                    if (!simulation.valid) {
                        ++invalidPlans;
                        continue;
                    }
                    const udon::SimulationResult validation =
                        validator.validate(prefix.targetState, candidate, false);
                    std::string mismatch;
                    if (!validator.agrees_with(
                            simulation,
                            validation,
                            mismatch)) {
                        ++validatorMismatches;
                        continue;
                    }
                    ++validPlans;
                    bool roadRelation = true;
                    bool strictRoad = false;
                    std::int64_t roadReduction = 0;
                    if (trafficDominance) {
                        if (simulation.roadFootprint.size() !=
                            baseline.roadFootprint.size()) {
                            roadRelation = false;
                        } else {
                            for (std::size_t road = 0;
                                 road < baseline.roadFootprint.size();
                                 ++road) {
                                const std::int32_t expected =
                                    baseline.roadFootprint.at(road);
                                const std::int32_t actual =
                                    simulation.roadFootprint.at(road);
                                if (actual > expected) {
                                    roadRelation = false;
                                    break;
                                }
                                strictRoad = strictRoad || actual < expected;
                                roadReduction += expected - actual;
                            }
                        }
                        roadRelation = roadRelation && strictRoad;
                    } else {
                        roadRelation = simulation.roadFootprint ==
                            baseline.roadFootprint;
                    }
                    if (!roadRelation) {
                        continue;
                    }
                    ++roadEquivalent;
                    strictRoadDominating += strictRoad ? 1 : 0;

                    bool stateDominates = true;
                    bool strictFuel = false;
                    std::int32_t minimumFuelDelta =
                        std::numeric_limits<std::int32_t>::max();
                    std::int32_t maximumFuelDelta = 0;
                    for (std::size_t logical = 0;
                         logical < baseline.finalAgents.size();
                         ++logical) {
                        std::size_t physical = logical;
                        if (logical == static_cast<std::size_t>(left)) {
                            physical = static_cast<std::size_t>(right);
                        } else if (logical == static_cast<std::size_t>(right)) {
                            physical = static_cast<std::size_t>(left);
                        }
                        const udon::AgentState& expected =
                            baseline.finalAgents.at(logical);
                        const udon::AgentState& actual =
                            simulation.finalAgents.at(physical);
                        if (expected.kind != actual.kind ||
                            expected.position != actual.position ||
                            (expected.kind == udon::AgentKind::Patrol &&
                             actual.fuel < expected.fuel)) {
                            stateDominates = false;
                            break;
                        }
                        if (expected.kind == udon::AgentKind::Patrol) {
                            const std::int32_t delta =
                                actual.fuel - expected.fuel;
                            minimumFuelDelta = std::min(
                                minimumFuelDelta,
                                delta);
                            maximumFuelDelta = std::max(
                                maximumFuelDelta,
                                delta);
                            strictFuel = strictFuel || delta > 0;
                        }
                    }
                    if (!stateDominates) {
                        continue;
                    }
                    ++mappedStateDominating;

                    const udon::BrandMask baselineLifetime =
                        prefix.ledger.lifetimeBrands | baseline.score.brands;
                    const udon::BrandMask candidateLifetime =
                        prefix.ledger.lifetimeBrands | simulation.score.brands;
                    const bool scoreNonRegressing =
                        baselineLifetime.is_subset_of(candidateLifetime) &&
                        simulation.score.dailyDistinct >=
                            baseline.score.dailyDistinct &&
                        simulation.score.servings >= baseline.score.servings;
                    const bool scoreStrict =
                        baselineLifetime != candidateLifetime ||
                        simulation.score.dailyDistinct >
                            baseline.score.dailyDistinct ||
                        simulation.score.servings > baseline.score.servings;
                    if (!scoreNonRegressing || !scoreStrict) {
                        continue;
                    }
                    if (trafficDominance) {
                        ++trafficCertified;
                    } else {
                        if (!strictFuel) {
                            ++exactFuelCertified;
                            continue;
                        }
                        ++strictFuelCertified;
                    }
                    const udon::OfficialScore score =
                        udon::OfficialScore::after_day(
                            prefix.ledger,
                            simulation.score);
                    if (bestScore < score) {
                        bestScore = score;
                        bestLeft = left;
                        bestRight = right;
                        bestMinimumFuelDelta = minimumFuelDelta;
                        bestMaximumFuelDelta = maximumFuelDelta;
                        bestRoadReduction = roadReduction;
                    }
                }
            }
        }
    }

    std::cout << (trafficDominance
            ? "traffic_permutation_summary"
            : "fuel_permutation_summary")
              << " states=" << maximumSettledStates
              << " max_routes=" << maximumRoutes
              << " tasks=" << tasks
              << " routes=" << routes
              << " cross_pairs=" << crossPairs
              << " invalid=" << invalidPlans
              << " validator_mismatch=" << validatorMismatches
              << " valid=" << validPlans
              << " road_equal=" << roadEquivalent
              << " strict_road_dominating=" << strictRoadDominating
              << " mapped_dominating=" << mappedStateDominating
              << " exact_fuel_certified=" << exactFuelCertified
              << " strict_fuel_certified=" << strictFuelCertified
              << " traffic_certified=" << trafficCertified
              << " parent=" << baselineScore.lifetimeDistinct << '/'
              << baselineScore.totalDailyDistinct << '/'
              << baselineScore.totalServings
              << " best=" << bestScore.lifetimeDistinct << '/'
              << bestScore.totalDailyDistinct << '/'
              << bestScore.totalServings
              << " left=" << bestLeft
              << " right=" << bestRight
              << " min_fuel_delta=" << bestMinimumFuelDelta
              << " max_fuel_delta=" << bestMaximumFuelDelta
              << " road_reduction=" << bestRoadReduction
              << " incumbent_mutated="
              << (incumbentBytes != udon::canonical_plan_bytes(frozenWitness))
              << '\n';
}

struct OffroadSegmentBoundary {
    std::size_t actionIndex = 0;
    udon::CellId cell = udon::kInvalidCell;
    std::int32_t elapsedSteps = 0;
    std::int32_t rawPatrolFuel = 0;
    std::int32_t waitSteps = 0;
    std::int32_t roadTouches = 0;
};

[[nodiscard]] bool probe_path_avoids_roads(
    const udon::MatchConfig& config,
    udon::CellId source,
    const std::vector<std::int32_t>& directions,
    udon::CellId expectedTerminal) {
    udon::CellId cell = source;
    for (const std::int32_t direction : directions) {
        if (config.map.terrain.at(static_cast<std::size_t>(cell)) ==
            udon::Terrain::Road) {
            return false;
        }
        cell = config.map.neighbors.at(static_cast<std::size_t>(cell)).at(
            static_cast<std::size_t>(direction));
        if (cell == udon::kInvalidCell ||
            config.map.terrain.at(static_cast<std::size_t>(cell)) ==
                udon::Terrain::Pond) {
            return false;
        }
    }
    return cell == expectedTerminal &&
        config.map.terrain.at(static_cast<std::size_t>(cell)) !=
            udon::Terrain::Road;
}

[[nodiscard]] std::vector<OffroadSegmentBoundary> segment_boundaries(
    const udon::MatchConfig& config,
    const udon::DayState& state,
    udon::AgentIndex agent,
    const udon::AgentPlan& plan) {
    std::vector<OffroadSegmentBoundary> boundaries;
    boundaries.reserve(plan.size() + 1U);
    OffroadSegmentBoundary current;
    current.cell = state.agents.at(static_cast<std::size_t>(agent)).position;
    boundaries.push_back(current);
    for (std::size_t actionIndex = 0;
         actionIndex < plan.size();
         ++actionIndex) {
        const udon::PlanAction& action = plan.at(actionIndex);
        bool touchesRoad =
            config.map.terrain.at(static_cast<std::size_t>(current.cell)) ==
            udon::Terrain::Road;
        if (action.kind == udon::ActionKind::Wait) {
            if (action.value <= 0) {
                throw std::runtime_error(
                    "offroad segment probe found nonpositive incumbent WAIT");
            }
            current.elapsedSteps += action.value;
            current.waitSteps += action.value;
        } else {
            const udon::CellId destination = config.map.neighbors.at(
                static_cast<std::size_t>(current.cell)).at(
                static_cast<std::size_t>(action.value));
            if (destination == udon::kInvalidCell) {
                throw std::runtime_error(
                    "offroad segment probe found invalid incumbent MOVE");
            }
            const udon::MoveCost cost = config.move_cost(
                current.cell,
                state.roadStatuses.at(static_cast<std::size_t>(current.cell)));
            current.elapsedSteps += cost.steps;
            current.rawPatrolFuel += cost.patrolFuel;
            current.cell = destination;
            touchesRoad = touchesRoad ||
                config.map.terrain.at(static_cast<std::size_t>(current.cell)) ==
                    udon::Terrain::Road;
        }
        current.roadTouches += touchesRoad ? 1 : 0;
        current.actionIndex = actionIndex + 1U;
        boundaries.push_back(current);
    }
    return boundaries;
}

void print_offroad_segment_substitution(
    const ReplayPrefix& prefix,
    const udon::DayPlan& frozenWitness) {
    constexpr std::size_t kMaximumSegmentActions = 8U;
    constexpr std::int32_t kMaximumPaths = 4;
    constexpr std::int32_t kMaximumLabelsPerCell = 64;

    const udon::ExactStepSimulator simulator(prefix.config);
    const udon::IndependentDayValidator validator(prefix.config);
    const udon::SimulationResult baseline = simulator.simulate(
        prefix.targetState,
        frozenWitness,
        false);
    const udon::SimulationResult baselineValidation = validator.validate(
        prefix.targetState,
        frozenWitness,
        false);
    std::string baselineMismatch;
    if (!baseline.valid || !validator.agrees_with(
            baseline,
            baselineValidation,
            baselineMismatch)) {
        throw std::runtime_error(
            "offroad segment baseline failed exact agreement: " +
            baselineMismatch);
    }
    const std::string incumbentBytes =
        udon::canonical_plan_bytes(frozenWitness);
    const udon::OfficialScore baselineScore = udon::OfficialScore::after_day(
        prefix.ledger,
        baseline.score);
    udon::OfficialScore bestScore = baselineScore;
    udon::AgentIndex bestAgent = udon::kInvalidAgent;
    std::size_t bestBegin = 0U;
    std::size_t bestEnd = 0U;
    udon::SpotIndex bestSpot = udon::kInvalidSpot;
    std::int32_t bestDuration = 0;
    std::int32_t bestFuelDelta = 0;

    const udon::ParetoRouter router(prefix.config);
    std::unordered_set<std::string> planBytes;
    std::int64_t consideredSegments = 0;
    std::int64_t eligibleSegments = 0;
    std::int64_t routePairs = 0;
    std::int64_t generatedPlans = 0;
    std::int64_t invalidPlans = 0;
    std::int64_t validatorMismatches = 0;
    std::int64_t validPlans = 0;
    std::int64_t roadEquivalent = 0;
    std::int64_t stateDominating = 0;
    std::int64_t strictProtected = 0;

    const auto paths_between = [&router, &prefix](
        udon::CellId source,
        udon::CellId target,
        const udon::ParetoSearchOptions& options) {
        if (source == target) {
            return std::vector<udon::ParetoPath>{udon::ParetoPath{}};
        }
        return router.find_paths(
            source,
            target,
            prefix.targetState.roadStatuses,
            options);
    };

    for (udon::AgentIndex agent = 0;
         agent < prefix.config.agent_count();
         ++agent) {
        if (prefix.targetState.agents.at(
                static_cast<std::size_t>(agent)).kind !=
            udon::AgentKind::Patrol) {
            continue;
        }
        const udon::AgentPlan& incumbentAgentPlan = frozenWitness.actions.at(
            static_cast<std::size_t>(agent));
        const std::vector<OffroadSegmentBoundary> boundaries =
            segment_boundaries(
                prefix.config,
                prefix.targetState,
                agent,
                incumbentAgentPlan);
        for (std::size_t begin = 0U;
             begin + 1U < boundaries.size();
             ++begin) {
            const std::size_t maximumEnd = std::min(
                boundaries.size() - 1U,
                begin + kMaximumSegmentActions);
            for (std::size_t end = begin + 1U;
                 end <= maximumEnd;
                 ++end) {
                ++consideredSegments;
                const OffroadSegmentBoundary& first = boundaries.at(begin);
                const OffroadSegmentBoundary& last = boundaries.at(end);
                const std::int32_t duration =
                    last.elapsedSteps - first.elapsedSteps;
                const std::int32_t originalFuel =
                    last.rawPatrolFuel - first.rawPatrolFuel;
                const std::int32_t waitSteps =
                    last.waitSteps - first.waitSteps;
                const bool originalOffroad =
                    last.roadTouches == first.roadTouches;
                if (duration <= 0 || waitSteps <= 0 || !originalOffroad ||
                    prefix.config.map.terrain.at(
                        static_cast<std::size_t>(first.cell)) ==
                        udon::Terrain::Road ||
                    prefix.config.map.terrain.at(
                        static_cast<std::size_t>(last.cell)) ==
                        udon::Terrain::Road) {
                    continue;
                }
                ++eligibleSegments;
                udon::ParetoSearchOptions options;
                options.maximumTravelSteps = duration;
                options.maximumPatrolFuel = originalFuel;
                options.maximumLabelsPerCell = kMaximumLabelsPerCell;
                options.maximumPaths = kMaximumPaths;
                options.patrol = true;
                for (std::size_t spotIndex = 0U;
                     spotIndex < prefix.config.spots.size();
                     ++spotIndex) {
                    const udon::Spot& spot = prefix.config.spots.at(spotIndex);
                    const std::vector<udon::ParetoPath> outbound =
                        paths_between(first.cell, spot.position, options);
                    const std::vector<udon::ParetoPath> inbound =
                        paths_between(spot.position, last.cell, options);
                    for (const udon::ParetoPath& out : outbound) {
                        if (!probe_path_avoids_roads(
                                prefix.config,
                                first.cell,
                                out.directions,
                                spot.position)) {
                            continue;
                        }
                        for (const udon::ParetoPath& back : inbound) {
                            ++routePairs;
                            const std::int32_t usedSteps =
                                out.travelSteps + back.travelSteps;
                            const std::int32_t usedFuel =
                                out.patrolFuel + back.patrolFuel;
                            if (usedSteps > duration || usedFuel > originalFuel ||
                                !probe_path_avoids_roads(
                                    prefix.config,
                                    spot.position,
                                    back.directions,
                                    last.cell)) {
                                continue;
                            }

                            udon::DayPlan candidate = frozenWitness;
                            udon::AgentPlan replacement;
                            replacement.reserve(
                                out.directions.size() +
                                back.directions.size() + 1U);
                            for (const std::int32_t direction : out.directions) {
                                replacement.push_back(
                                    udon::PlanAction::move(direction));
                            }
                            for (const std::int32_t direction : back.directions) {
                                replacement.push_back(
                                    udon::PlanAction::move(direction));
                            }
                            if (usedSteps < duration) {
                                replacement.push_back(udon::PlanAction::wait(
                                    duration - usedSteps));
                            }
                            udon::AgentPlan& candidateAgent =
                                candidate.actions.at(
                                    static_cast<std::size_t>(agent));
                            candidateAgent.erase(
                                candidateAgent.begin() +
                                    static_cast<std::ptrdiff_t>(begin),
                                candidateAgent.begin() +
                                    static_cast<std::ptrdiff_t>(end));
                            candidateAgent.insert(
                                candidateAgent.begin() +
                                    static_cast<std::ptrdiff_t>(begin),
                                replacement.begin(),
                                replacement.end());
                            const std::string candidateBytes =
                                udon::canonical_plan_bytes(candidate);
                            if (!planBytes.insert(candidateBytes).second) {
                                continue;
                            }
                            ++generatedPlans;
                            const udon::SimulationResult simulation =
                                simulator.simulate(
                                    prefix.targetState,
                                    candidate,
                                    false);
                            if (!simulation.valid) {
                                ++invalidPlans;
                                continue;
                            }
                            const udon::SimulationResult validation =
                                validator.validate(
                                    prefix.targetState,
                                    candidate,
                                    false);
                            std::string mismatch;
                            if (!validator.agrees_with(
                                    simulation,
                                    validation,
                                    mismatch)) {
                                ++validatorMismatches;
                                continue;
                            }
                            ++validPlans;
                            if (simulation.roadFootprint !=
                                baseline.roadFootprint) {
                                continue;
                            }
                            ++roadEquivalent;
                            if (!udon::protected_slack_transition_dominates(
                                    baseline,
                                    simulation)) {
                                continue;
                            }
                            ++stateDominating;
                            const udon::BrandMask baselineLifetime =
                                prefix.ledger.lifetimeBrands |
                                baseline.score.brands;
                            const udon::BrandMask candidateLifetime =
                                prefix.ledger.lifetimeBrands |
                                simulation.score.brands;
                            const bool nonRegressing =
                                baselineLifetime.is_subset_of(candidateLifetime) &&
                                simulation.score.dailyDistinct >=
                                    baseline.score.dailyDistinct &&
                                simulation.score.servings >=
                                    baseline.score.servings;
                            const bool strict =
                                baselineLifetime != candidateLifetime ||
                                simulation.score.dailyDistinct >
                                    baseline.score.dailyDistinct ||
                                simulation.score.servings >
                                    baseline.score.servings;
                            if (!nonRegressing || !strict) {
                                continue;
                            }
                            ++strictProtected;
                            const udon::OfficialScore score =
                                udon::OfficialScore::after_day(
                                    prefix.ledger,
                                    simulation.score);
                            if (bestScore < score) {
                                bestScore = score;
                                bestAgent = agent;
                                bestBegin = begin;
                                bestEnd = end;
                                bestSpot = static_cast<udon::SpotIndex>(spotIndex);
                                bestDuration = duration;
                                bestFuelDelta =
                                    baseline.finalAgents.at(
                                        static_cast<std::size_t>(agent)).fuel -
                                    simulation.finalAgents.at(
                                        static_cast<std::size_t>(agent)).fuel;
                            }
                        }
                    }
                }
            }
        }
    }

    std::cout << "offroad_segment_summary"
              << " max_actions=" << kMaximumSegmentActions
              << " max_paths=" << kMaximumPaths
              << " labels=" << kMaximumLabelsPerCell
              << " considered=" << consideredSegments
              << " eligible=" << eligibleSegments
              << " route_pairs=" << routePairs
              << " generated=" << generatedPlans
              << " invalid=" << invalidPlans
              << " validator_mismatch=" << validatorMismatches
              << " valid=" << validPlans
              << " road_equal=" << roadEquivalent
              << " state_dominating=" << stateDominating
              << " strict_protected=" << strictProtected
              << " parent=" << baselineScore.lifetimeDistinct << '/'
              << baselineScore.totalDailyDistinct << '/'
              << baselineScore.totalServings
              << " best=" << bestScore.lifetimeDistinct << '/'
              << bestScore.totalDailyDistinct << '/'
              << bestScore.totalServings
              << " agent=" << bestAgent
              << " begin=" << bestBegin
              << " end=" << bestEnd
              << " spot=" << bestSpot
              << " duration=" << bestDuration
              << " fuel_delta=" << bestFuelDelta
              << " incumbent_mutated="
              << (incumbentBytes !=
                  udon::canonical_plan_bytes(frozenWitness))
              << '\n';
}

struct ClosedLoopSuffixResult {
    udon::OfficialScore score;
    std::vector<udon::OfficialScore> cumulativeByDay;
};

[[nodiscard]] ClosedLoopSuffixResult run_closed_loop_suffix(
    const std::vector<udon::JsonValue>& events,
    std::int32_t targetDay,
    const udon::DayPlan& forcedRoot,
    const std::string& label) {
    const auto setupEvent = std::find_if(
        events.begin(),
        events.end(),
        [](const udon::JsonValue& event) {
            return event.at("kind").string() == "setup";
        });
    if (setupEvent == events.end()) {
        throw std::runtime_error("replay has no setup event");
    }
    const udon::BtcAdapterOptions adapterOptions{5000};
    const udon::MatchConfig config =
        udon::parse_btc_setup(setupEvent->at("body"), adapterOptions);
    const ReplayPrefix prefix = reconstruct_prefix(events, targetDay);
    udon::DeadlineCalibration calibration;
    calibration.version = "btc-http-local-budget-v8-idempotent-ack-resend";
    calibration.networkFloor = std::chrono::milliseconds{1600};
    calibration.networkPercent = 20;
    calibration.certificationPercent = 20;
    const std::int32_t solveBudgetMs = static_cast<std::int32_t>(
        udon::competition_compute_budget(std::chrono::milliseconds{5000}).count());
    calibration.normalThreshold = std::chrono::milliseconds{solveBudgetMs};
    calibration.version += "-logic-normal";
    udon::UdonShieldEngine engine(
        config,
        {},
        calibration,
        udon::RoutePoolSearch::SinglePass,
        7,
        false,
        7);
    engine.set_short_horizon_role_fallback(true);
    static_cast<void>(engine.select_roles_until(
        std::chrono::milliseconds{solveBudgetMs},
        3));

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    udon::MatchLedger ledger = prefix.ledger;
    std::vector<udon::AgentState> counterfactualAgents;
    ClosedLoopSuffixResult result;
    std::int32_t expectedDay = targetDay;
    for (const udon::JsonValue& event : events) {
        if (event.at("kind").string() != "day_state") {
            continue;
        }
        const std::int64_t atUnixMs = event.at("atUnixMs").integer();
        udon::DayState state = udon::parse_btc_day_state(
            config,
            event.at("body"),
            std::chrono::system_clock::time_point{
                std::chrono::milliseconds{atUnixMs}},
            adapterOptions);
        if (state.dayNumber != expectedDay) {
            continue;
        }
        if (!counterfactualAgents.empty()) {
            state.agents = counterfactualAgents;
        }

        const bool root = state.dayNumber == targetDay;
        std::optional<udon::DecisionResult> decision;
        udon::DayPlan plan;
        const std::chrono::steady_clock::time_point started =
            std::chrono::steady_clock::now();
        if (root) {
            plan = forcedRoot;
        } else {
            decision.emplace(engine.solve_day(
                state,
                ledger,
                std::chrono::milliseconds{solveBudgetMs}));
            plan = decision->candidate.plan;
        }
        const std::chrono::milliseconds elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - started);
        const udon::SimulationResult simulation =
            simulator.simulate(state, plan, false);
        const udon::SimulationResult validation =
            validator.validate(state, plan, false);
        std::string mismatch;
        if (!simulation.valid ||
            !validator.agrees_with(simulation, validation, mismatch)) {
            throw std::runtime_error(
                label + " closed-loop plan failed exact agreement: " + mismatch);
        }
        ledger.apply(simulation.score);
        counterfactualAgents = simulation.finalAgents;
        if (root) {
            // The forced root has no production CandidateProfile. This exact
            // applied-transition boundary is deliberately used for both the
            // control and sparse arms so the suffix comparison differs only in
            // root plan/state, while subsequent days use the canonical engine.
            engine.record_applied_transition(state, simulation);
        } else {
            engine.record_submitted(*decision, elapsed);
        }
        const udon::OfficialScore cumulative{
            ledger.lifetime_distinct(),
            ledger.totalDailyDistinct,
            ledger.totalServings,
        };
        result.cumulativeByDay.push_back(cumulative);
        result.score = cumulative;
        std::cout << "closed_loop=" << label
                  << " day=" << state.dayNumber
                  << " score=" << cumulative.lifetimeDistinct << '/'
                  << cumulative.totalDailyDistinct << '/'
                  << cumulative.totalServings
                  << " daily=" << simulation.score.dailyDistinct << '/'
                  << simulation.score.servings
                  << " root=" << (root ? 1 : 0)
                  << " elapsed_ms=" << elapsed.count() << '\n';
        ++expectedDay;
        if (expectedDay > config.day_count()) {
            break;
        }
    }
    if (expectedDay <= config.day_count()) {
        throw std::runtime_error(label + " closed-loop replay ended early");
    }
    std::cout << "closed_loop_summary=" << label
              << " score=" << result.score.lifetimeDistinct << '/'
              << result.score.totalDailyDistinct << '/'
              << result.score.totalServings << '\n';
    return result;
}

void print_complete_resource_team_dp(
    const ReplayPrefix& prefix,
    const udon::DayPlan& frozenWitness) {
    const udon::ExactStepSimulator simulator(prefix.config);
    const udon::IndependentDayValidator validator(prefix.config);
    const udon::SimulationResult baseline = simulator.simulate(
        prefix.targetState,
        frozenWitness,
        false);
    const std::int32_t targetServings = baseline.score.servings + 1;
    const std::int32_t targetDailyDistinct = baseline.score.dailyDistinct;

    std::vector<udon::AgentIndex> patrols;
    std::vector<udon::ExactOrienteeringReachability> frontiers(
        static_cast<std::size_t>(prefix.config.agent_count()));
    for (udon::AgentIndex agent = 0; agent < prefix.config.agent_count(); ++agent) {
        if (prefix.targetState.agents.at(static_cast<std::size_t>(agent)).kind !=
            udon::AgentKind::Patrol) {
            continue;
        }
        patrols.push_back(agent);
        frontiers.at(static_cast<std::size_t>(agent)) =
            udon::enumerate_exact_resource_routes(
                prefix.config,
                prefix.targetState,
                agent,
                std::nullopt);
        const auto& frontier = frontiers.at(static_cast<std::size_t>(agent));
        std::cout << "team_frontier_agent=" << agent
                  << " supported=" << (frontier.supported ? 1 : 0)
                  << " complete=" << (frontier.complete ? 1 : 0)
                  << " settled=" << frontier.settledStates
                  << " routes=" << frontier.maximalRoutes.size() << '\n';
        if (!frontier.supported || !frontier.complete || frontier.maximalRoutes.empty()) {
            throw std::runtime_error("team DP requires complete nonempty patrol frontiers");
        }
    }
    std::sort(
        patrols.begin(),
        patrols.end(),
        [&frontiers](udon::AgentIndex left, udon::AgentIndex right) {
            return std::pair{
                       frontiers.at(static_cast<std::size_t>(left)).maximalRoutes.size(),
                       left} <
                std::pair{
                       frontiers.at(static_cast<std::size_t>(right)).maximalRoutes.size(),
                       right};
        });

    std::vector<std::vector<std::int16_t>> routeOrder(
        static_cast<std::size_t>(prefix.config.agent_count()));
    for (const udon::AgentIndex agent : patrols) {
        const auto& routes = frontiers.at(static_cast<std::size_t>(agent)).maximalRoutes;
        auto& order = routeOrder.at(static_cast<std::size_t>(agent));
        for (std::size_t index = 0; index < routes.size(); ++index) {
            order.push_back(static_cast<std::int16_t>(index));
        }
        std::sort(
            order.begin(),
            order.end(),
            [&prefix, &routes](std::int16_t left, std::int16_t right) {
                const auto rank = [&prefix, &routes](std::int16_t index) {
                    const std::uint32_t mask = routes.at(
                        static_cast<std::size_t>(index)).spotMask;
                    std::int32_t scarcity = 0;
                    for (std::size_t spot = 0; spot < prefix.config.spots.size(); ++spot) {
                        if ((mask & (std::uint32_t{1} << spot)) != 0U) {
                            scarcity += 10000 / std::max(1, prefix.config.spots.at(spot).stock);
                        }
                    }
                    return std::tuple{
                        static_cast<std::int32_t>(std::popcount(mask)),
                        -scarcity,
                        -routes.at(static_cast<std::size_t>(index)).usedSteps,
                        -static_cast<std::int32_t>(index)};
                };
                return rank(left) > rank(right);
            });
    }

    std::vector<std::array<std::uint8_t, 16>> suffixReach(patrols.size() + 1U);
    std::vector<udon::BrandMask> suffixBrands(patrols.size() + 1U);
    for (std::size_t depth = patrols.size(); depth-- > 0U;) {
        suffixReach.at(depth) = suffixReach.at(depth + 1U);
        suffixBrands.at(depth) = suffixBrands.at(depth + 1U);
        const udon::AgentIndex agent = patrols.at(depth);
        std::uint32_t reachableSpots = 0U;
        for (const udon::ExactOrienteeringRoute& route :
             frontiers.at(static_cast<std::size_t>(agent)).maximalRoutes) {
            reachableSpots |= route.spotMask;
        }
        for (std::size_t spot = 0; spot < prefix.config.spots.size(); ++spot) {
            if ((reachableSpots & (std::uint32_t{1} << spot)) != 0U) {
                ++suffixReach.at(depth).at(spot);
                suffixBrands.at(depth) |= udon::brand_bit(
                    prefix.config.spots.at(spot).brandIndex);
            }
        }
    }

    std::array<std::uint8_t, 16> counts{};
    std::array<std::int16_t, udon::kMaximumAgents> choices{};
    std::array<std::int16_t, udon::kMaximumAgents> solution{};
    choices.fill(-1);
    solution.fill(-1);
    std::vector<std::unordered_set<std::uint64_t>> memo(patrols.size() + 1U);
    std::uint64_t nodes = 0U;
    const auto count_key = [&prefix, &counts]() {
        std::uint64_t key = 0U;
        for (std::size_t spot = 0; spot < prefix.config.spots.size(); ++spot) {
            key |= static_cast<std::uint64_t>(counts.at(spot)) << (4U * spot);
        }
        return key;
    };
    const auto search = [&](auto&& self,
                            std::size_t depth,
                            std::int32_t servings,
                            udon::BrandMask brands) -> bool {
        ++nodes;
        std::int32_t optimisticServings = servings;
        for (std::size_t spot = 0; spot < prefix.config.spots.size(); ++spot) {
            const std::int32_t capacity = std::clamp(
                prefix.config.spots.at(spot).stock,
                0,
                static_cast<std::int32_t>(patrols.size()));
            optimisticServings += std::min<std::int32_t>(
                capacity - counts.at(spot),
                suffixReach.at(depth).at(spot));
        }
        if (optimisticServings < targetServings ||
            udon::brand_count(brands | suffixBrands.at(depth)) < targetDailyDistinct) {
            return false;
        }
        if (!memo.at(depth).insert(count_key()).second) {
            return false;
        }
        if (depth == patrols.size()) {
            if (servings >= targetServings &&
                udon::brand_count(brands) >= targetDailyDistinct) {
                solution = choices;
                return true;
            }
            return false;
        }
        const udon::AgentIndex agent = patrols.at(depth);
        const auto& routes = frontiers.at(static_cast<std::size_t>(agent)).maximalRoutes;
        for (const std::int16_t routeIndex : routeOrder.at(static_cast<std::size_t>(agent))) {
            const udon::ExactOrienteeringRoute& route = routes.at(
                static_cast<std::size_t>(routeIndex));
            std::uint32_t incremented = 0U;
            std::int32_t addedServings = 0;
            udon::BrandMask addedBrands;
            for (std::size_t spot = 0; spot < prefix.config.spots.size(); ++spot) {
                if ((route.spotMask & (std::uint32_t{1} << spot)) == 0U) {
                    continue;
                }
                addedBrands |= udon::brand_bit(prefix.config.spots.at(spot).brandIndex);
                const std::uint8_t capacity = static_cast<std::uint8_t>(std::clamp(
                    prefix.config.spots.at(spot).stock,
                    0,
                    static_cast<std::int32_t>(patrols.size())));
                if (counts.at(spot) < capacity) {
                    ++counts.at(spot);
                    incremented |= std::uint32_t{1} << spot;
                    ++addedServings;
                }
            }
            choices.at(static_cast<std::size_t>(agent)) = routeIndex;
            if (self(
                    self,
                    depth + 1U,
                    servings + addedServings,
                    brands | addedBrands)) {
                return true;
            }
            for (std::size_t spot = 0; spot < prefix.config.spots.size(); ++spot) {
                if ((incremented & (std::uint32_t{1} << spot)) != 0U) {
                    --counts.at(spot);
                }
            }
        }
        return false;
    };

    const bool found = search(search, 0U, 0, 0U);
    std::uint64_t memoStates = 0U;
    for (const auto& states : memo) {
        memoStates += states.size();
    }
    std::cout << "team_dp_found=" << (found ? 1 : 0)
              << " target_daily=" << targetDailyDistinct
              << " target_servings=" << targetServings
              << " nodes=" << nodes
              << " memo_states=" << memoStates << '\n';
    if (!found) {
        return;
    }

    udon::DayPlan plan = frozenWitness;
    for (const udon::AgentIndex agent : patrols) {
        const std::int16_t routeIndex = solution.at(static_cast<std::size_t>(agent));
        const udon::ExactOrienteeringRoute& route = frontiers
            .at(static_cast<std::size_t>(agent))
            .maximalRoutes.at(static_cast<std::size_t>(routeIndex));
        plan.actions.at(static_cast<std::size_t>(agent)) = route.actions;
        std::cout << "team_dp_agent=" << agent
                  << " route=" << routeIndex
                  << " mask=0x" << std::hex << std::uppercase << route.spotMask
                  << std::dec << std::nouppercase << '\n';
    }
    const udon::SimulationResult simulation = simulator.simulate(prefix.targetState, plan, false);
    const udon::SimulationResult validation = validator.validate(prefix.targetState, plan, false);
    std::string mismatch;
    if (!simulation.valid || !validator.agrees_with(simulation, validation, mismatch)) {
        throw std::runtime_error("team DP witness failed dual validation: " + mismatch);
    }
    const udon::OfficialScore score = udon::OfficialScore::after_day(prefix.ledger, simulation.score);
    std::cout << "team_dp_score=" << score.lifetimeDistinct << '/'
              << score.totalDailyDistinct << '/' << score.totalServings << '\n';
    std::cout << "team_dp_plan=" << udon::serialize_day_plan(plan).dump() << '\n';
}

void print_recorded_rendezvous_pairs(
    const ReplayPrefix& prefix,
    const udon::DayPlan& recordedPlan,
    std::int32_t maximumColumnsPerAgent) {
    const udon::ParetoRouter router(prefix.config);
    const udon::RouteColumnGenerator generator(prefix.config, router);
    udon::ColumnGenerationOptions generation;
    generation.maximumPathsPerTarget = 4;
    generation.maximumColumnsPerAgent = maximumColumnsPerAgent;
    generation.maximumTargetSpots = 12;
    generation.maximumEscorts = 16;
    generation.maximumSeedPlans = 2;
    generation.seedPlans.push_back(recordedPlan);
    generation.enableHarvestExtensions = true;
    generation.allowUncachedHarvestTargets = false;
    generation.enableHarvestOrienteering = true;
    generation.enableExactHarvestOrienteering = false;
    generation.maximumHarvestExtensionSources = 4;
    generation.maximumHarvestExtensionDepth = 4;
    udon::ColumnGenerationDiagnostics diagnostics;
    const udon::RoutePortfolio portfolio = generator.generate(
        prefix.targetState,
        prefix.ledger,
        generation,
        &diagnostics);
    const udon::ExactStepSimulator simulator(prefix.config);
    const udon::IndependentDayValidator validator(prefix.config);
    std::cout << "rendezvous_portfolio=";
    for (std::size_t agent = 0; agent < portfolio.columnsByAgent.size(); ++agent) {
        if (agent != 0U) {
            std::cout << '/';
        }
        std::cout << portfolio.columnsByAgent.at(agent).size();
    }
    std::cout << " coordination_queries=" << diagnostics.coordinationParetoQueries
              << " coordination_ms=" << diagnostics.coordinationMilliseconds
              << " deadline=" << diagnostics.deadlineReached << '\n';

    std::int32_t pairs = 0;
    std::int32_t validPairs = 0;
    udon::OfficialScore best = udon::OfficialScore::after_day(
        prefix.ledger,
        simulator.simulate(prefix.targetState, recordedPlan, true).score);
    for (udon::AgentIndex patrol = 0;
         patrol < prefix.config.agent_count();
         ++patrol) {
        if (prefix.targetState.agents.at(static_cast<std::size_t>(patrol)).kind !=
            udon::AgentKind::Patrol) {
            continue;
        }
        for (const udon::RouteColumn& patrolColumn :
             portfolio.columnsByAgent.at(static_cast<std::size_t>(patrol))) {
            if (patrolColumn.escortGroup < 0 ||
                patrolColumn.requiredRefuels.empty()) {
                continue;
            }
            for (udon::AgentIndex tanker = 0;
                 tanker < prefix.config.agent_count();
                 ++tanker) {
                if (prefix.targetState.agents.at(static_cast<std::size_t>(tanker)).kind !=
                    udon::AgentKind::Tanker) {
                    continue;
                }
                for (const udon::RouteColumn& tankerColumn :
                     portfolio.columnsByAgent.at(static_cast<std::size_t>(tanker))) {
                    if (tankerColumn.escortGroup != patrolColumn.escortGroup) {
                        continue;
                    }
                    ++pairs;
                    udon::DayPlan candidate = recordedPlan;
                    candidate.actions.at(static_cast<std::size_t>(patrol)) =
                        patrolColumn.actions;
                    candidate.actions.at(static_cast<std::size_t>(tanker)) =
                        tankerColumn.actions;
                    const udon::SimulationResult simulation =
                        simulator.simulate(prefix.targetState, candidate, true);
                    const udon::SimulationResult validation =
                        validator.validate(prefix.targetState, candidate, true);
                    std::string mismatch;
                    const bool valid = simulation.valid &&
                        validator.agrees_with(simulation, validation, mismatch);
                    if (!valid) {
                        std::cout << "rendezvous_pair group=" << patrolColumn.escortGroup
                                  << " patrol=" << patrol
                                  << " tanker=" << tanker
                                  << " valid=0 mismatch=" << mismatch << '\n';
                        continue;
                    }
                    ++validPairs;
                    const udon::OfficialScore score = udon::OfficialScore::after_day(
                        prefix.ledger,
                        simulation.score);
                    if (udon::compare_lexicographic(score, best) > 0) {
                        best = score;
                    }
                    std::cout << "rendezvous_pair group=" << patrolColumn.escortGroup
                              << " patrol=" << patrol
                              << " tanker=" << tanker
                              << " valid=1 score=" << score.lifetimeDistinct << '/'
                              << score.totalDailyDistinct << '/'
                              << score.totalServings
                              << " refuel=" << patrolColumn.requiredRefuels.front().cell
                              << '@' << patrolColumn.requiredRefuels.front().step
                              << " patrol_servings=" << patrolColumn.estimatedServings
                              << " patrol_terminal=" << patrolColumn.terminalCell << ':'
                              << patrolColumn.terminalFuel
                              << " tanker_terminal=" << tankerColumn.terminalCell
                              << " plan=" << udon::serialize_day_plan(candidate).dump()
                              << '\n';
                }
            }
        }
    }
    std::cout << "rendezvous_summary pairs=" << pairs
              << " valid=" << validPairs
              << " best=" << best.lifetimeDistinct << '/'
              << best.totalDailyDistinct << '/'
              << best.totalServings << '\n';
}

struct AgentComponents {
    explicit AgentComponents(std::int32_t count)
        : parent(static_cast<std::size_t>(count)), sizes(static_cast<std::size_t>(count), 1) {
        for (std::int32_t index = 0; index < count; ++index) {
            parent.at(static_cast<std::size_t>(index)) = index;
        }
    }

    [[nodiscard]] std::int32_t find(std::int32_t value) {
        std::int32_t& ancestor = parent.at(static_cast<std::size_t>(value));
        if (ancestor != value) {
            ancestor = find(ancestor);
        }
        return ancestor;
    }

    void unite(std::int32_t left, std::int32_t right) {
        left = find(left);
        right = find(right);
        if (left == right) {
            return;
        }
        if (sizes.at(static_cast<std::size_t>(left)) <
            sizes.at(static_cast<std::size_t>(right))) {
            std::swap(left, right);
        }
        parent.at(static_cast<std::size_t>(right)) = left;
        sizes.at(static_cast<std::size_t>(left)) +=
            sizes.at(static_cast<std::size_t>(right));
    }

    std::vector<std::int32_t> parent;
    std::vector<std::int32_t> sizes;
};

[[nodiscard]] bool integer_sets_intersect(
    const std::unordered_set<std::int64_t>& left,
    const std::unordered_set<std::int64_t>& right) {
    const auto& smaller = left.size() <= right.size() ? left : right;
    const auto& larger = left.size() <= right.size() ? right : left;
    return std::any_of(smaller.begin(), smaller.end(), [&larger](std::int64_t value) {
        return larger.contains(value);
    });
}

[[nodiscard]] std::int64_t refuel_key(const udon::RefuelEvent& event) {
    return (static_cast<std::int64_t>(event.cell) << 32U) |
        static_cast<std::uint32_t>(event.step);
}

[[nodiscard]] std::vector<std::vector<std::int32_t>> component_members(
    AgentComponents& components,
    std::int32_t count) {
    std::unordered_map<std::int32_t, std::vector<std::int32_t>> byRoot;
    for (std::int32_t agent = 0; agent < count; ++agent) {
        byRoot[components.find(agent)].push_back(agent);
    }
    std::vector<std::vector<std::int32_t>> result;
    result.reserve(byRoot.size());
    for (auto& [root, members] : byRoot) {
        static_cast<void>(root);
        result.push_back(std::move(members));
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        if (left.size() != right.size()) {
            return left.size() > right.size();
        }
        return left < right;
    });
    return result;
}

void print_master_factor_separability(
    const ReplayPrefix& prefix,
    const udon::DayPlan& recordedPlan) {
    const udon::FastViabilityAnalyzer viabilityAnalyzer(prefix.config);
    const udon::ViabilityBounds viability = viabilityAnalyzer.analyze(
        prefix.targetState,
        prefix.ledger);

    const udon::ParetoRouter router(prefix.config);
    const udon::RouteColumnGenerator generator(prefix.config, router);
    udon::ColumnGenerationOptions generation;
    generation.maximumPathsPerTarget = 4;
    generation.maximumColumnsPerAgent = 12;
    generation.maximumTargetSpots = 12;
    generation.maximumEscorts = 16;
    generation.maximumSeedPlans = 2;
    generation.seedPlans.push_back(recordedPlan);
    generation.enableHarvestExtensions = true;
    generation.allowUncachedHarvestTargets = false;
    generation.enableHarvestOrienteering = true;
    generation.enableExactHarvestOrienteering = false;
    generation.maximumHarvestExtensionSources = 4;
    generation.maximumHarvestExtensionDepth = 4;
    generation.mandatoryReservations = viability.reservations;
    udon::ColumnGenerationDiagnostics diagnostics;
    const udon::RoutePortfolio portfolio = generator.generate(
        prefix.targetState,
        prefix.ledger,
        generation,
        &diagnostics);

    const std::int32_t agentCount = prefix.config.agent_count();
    if (portfolio.columnsByAgent.size() != static_cast<std::size_t>(agentCount)) {
        throw std::runtime_error("factor probe portfolio has wrong agent count");
    }
    AgentComponents constraintComponents(agentCount);
    AgentComponents conservativeComponents(agentCount);
    std::vector<std::unordered_set<std::int64_t>> spots(static_cast<std::size_t>(agentCount));
    std::vector<std::unordered_set<std::int64_t>> required(static_cast<std::size_t>(agentCount));
    std::vector<std::unordered_set<std::int64_t>> provided(static_cast<std::size_t>(agentCount));
    std::vector<std::unordered_set<std::int64_t>> escorts(static_cast<std::size_t>(agentCount));
    std::vector<std::unordered_set<std::int64_t>> bundles(static_cast<std::size_t>(agentCount));
    std::vector<udon::BrandMask> brands(static_cast<std::size_t>(agentCount));
    std::vector<std::int32_t> columnCounts(static_cast<std::size_t>(agentCount));
    for (std::int32_t agent = 0; agent < agentCount; ++agent) {
        const auto& columns = portfolio.columnsByAgent.at(static_cast<std::size_t>(agent));
        columnCounts.at(static_cast<std::size_t>(agent)) =
            static_cast<std::int32_t>(columns.size());
        if (columns.empty()) {
            throw std::runtime_error("factor probe generated an empty agent portfolio");
        }
        for (const udon::RouteColumn& column : columns) {
            brands.at(static_cast<std::size_t>(agent)) |= column.estimatedBrands;
            for (const udon::ColumnVisitEvent& visit : column.firstVisits) {
                if (visit.claimedServing) {
                    spots.at(static_cast<std::size_t>(agent)).insert(visit.spot);
                    if (visit.brandIndex >= 0) {
                        brands.at(static_cast<std::size_t>(agent)) |=
                            udon::brand_bit(visit.brandIndex);
                    }
                }
            }
            for (const udon::RefuelEvent& event : column.requiredRefuels) {
                required.at(static_cast<std::size_t>(agent)).insert(refuel_key(event));
            }
            for (const udon::RefuelEvent& event : column.providedRefuels) {
                provided.at(static_cast<std::size_t>(agent)).insert(refuel_key(event));
            }
            if (column.escortGroup >= 0) {
                escorts.at(static_cast<std::size_t>(agent)).insert(column.escortGroup);
            }
            if (column.contingencyBundle >= 0) {
                bundles.at(static_cast<std::size_t>(agent)).insert(column.contingencyBundle);
            }
        }
    }

    std::vector<std::vector<bool>> reservationCapability(
        viability.reservations.size(),
        std::vector<bool>(static_cast<std::size_t>(agentCount), false));
    std::int32_t provenReservations = 0;
    for (std::size_t reservationIndex = 0;
         reservationIndex < viability.reservations.size();
         ++reservationIndex) {
        const udon::MandatoryReservation& reservation =
            viability.reservations.at(reservationIndex);
        if (!udon::is_proven_reservation(reservation)) {
            continue;
        }
        ++provenReservations;
        for (std::int32_t agent = 0; agent < agentCount; ++agent) {
            const bool bySpot = reservation.representativeSpot != udon::kInvalidSpot &&
                spots.at(static_cast<std::size_t>(agent)).contains(
                    reservation.representativeSpot);
            const bool byBrand = reservation.brandIndex >= 0 &&
                udon::has_brand(
                    brands.at(static_cast<std::size_t>(agent)),
                    reservation.brandIndex);
            reservationCapability.at(reservationIndex).at(
                static_cast<std::size_t>(agent)) = bySpot || byBrand;
        }
    }

    std::int32_t spotPairs = 0;
    std::int32_t refuelPairs = 0;
    std::int32_t escortPairs = 0;
    std::int32_t bundlePairs = 0;
    std::int32_t reservationPairs = 0;
    std::int32_t brandPairs = 0;
    for (std::int32_t left = 0; left < agentCount; ++left) {
        for (std::int32_t right = left + 1; right < agentCount; ++right) {
            const std::size_t leftOffset = static_cast<std::size_t>(left);
            const std::size_t rightOffset = static_cast<std::size_t>(right);
            const bool sharesSpot = integer_sets_intersect(
                spots.at(leftOffset), spots.at(rightOffset));
            const bool sharesRefuel = integer_sets_intersect(
                    required.at(leftOffset), provided.at(rightOffset)) ||
                integer_sets_intersect(required.at(rightOffset), provided.at(leftOffset));
            const bool sharesEscort = integer_sets_intersect(
                escorts.at(leftOffset), escorts.at(rightOffset));
            const bool sharesBundle = integer_sets_intersect(
                bundles.at(leftOffset), bundles.at(rightOffset));
            bool sharesReservation = false;
            for (std::size_t reservationIndex = 0;
                 reservationIndex < viability.reservations.size();
                 ++reservationIndex) {
                if (!udon::is_proven_reservation(
                        viability.reservations.at(reservationIndex))) {
                    continue;
                }
                sharesReservation = sharesReservation ||
                    (reservationCapability.at(reservationIndex).at(leftOffset) &&
                     reservationCapability.at(reservationIndex).at(rightOffset));
            }
            spotPairs += sharesSpot ? 1 : 0;
            refuelPairs += sharesRefuel ? 1 : 0;
            escortPairs += sharesEscort ? 1 : 0;
            bundlePairs += sharesBundle ? 1 : 0;
            reservationPairs += sharesReservation ? 1 : 0;
            const bool constrained = sharesSpot || sharesRefuel || sharesEscort ||
                sharesBundle || sharesReservation;
            if (constrained) {
                constraintComponents.unite(left, right);
                conservativeComponents.unite(left, right);
            }
            const bool sharesBrand = udon::brand_intersection_count(
                brands.at(leftOffset), brands.at(rightOffset)) > 0;
            brandPairs += sharesBrand ? 1 : 0;
            if (sharesBrand) {
                conservativeComponents.unite(left, right);
            }
        }
    }

    auto constraintMembers = component_members(constraintComponents, agentCount);
    auto conservativeMembers = component_members(conservativeComponents, agentCount);
    const auto component_text = [](const std::vector<std::vector<std::int32_t>>& components) {
        std::ostringstream text;
        for (std::size_t index = 0; index < components.size(); ++index) {
            if (index != 0U) {
                text << '|';
            }
            text << '[';
            for (std::size_t member = 0; member < components.at(index).size(); ++member) {
                if (member != 0U) {
                    text << ',';
                }
                text << components.at(index).at(member);
            }
            text << ']';
        }
        return text.str();
    };
    double fullProduct = 1.0;
    for (const std::int32_t count : columnCounts) {
        fullProduct *= static_cast<double>(count);
    }
    double independentWork = 0.0;
    for (const auto& component : conservativeMembers) {
        double product = 1.0;
        for (const std::int32_t agent : component) {
            product *= static_cast<double>(columnCounts.at(static_cast<std::size_t>(agent)));
        }
        independentWork += product;
    }
    const double reduction = independentWork > 0.0 ? fullProduct / independentWork : 0.0;

    std::cout << "factor_portfolio columns=";
    for (std::size_t agent = 0; agent < columnCounts.size(); ++agent) {
        if (agent != 0U) {
            std::cout << '/';
        }
        std::cout << columnCounts.at(agent);
    }
    std::cout << " generation_deadline=" << diagnostics.deadlineReached
              << " reservations=" << viability.reservations.size()
              << " proven_reservations=" << provenReservations << '\n';
    std::cout << "factor_edges spot=" << spotPairs
              << " refuel=" << refuelPairs
              << " escort=" << escortPairs
              << " bundle=" << bundlePairs
              << " reservation=" << reservationPairs
              << " brand=" << brandPairs << '\n';
    std::cout << "factor_components constraint=" << component_text(constraintMembers)
              << " conservative=" << component_text(conservativeMembers)
              << " conservative_largest=" << conservativeMembers.front().size()
              << " full_product=" << std::fixed << std::setprecision(0) << fullProduct
              << " independent_work=" << independentWork
              << " reduction=" << std::setprecision(6) << reduction << '\n';
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 4 && argc != 5 && argc != 6 && argc != 7) {
            throw std::invalid_argument(
                "usage: claim_probe REPLAY PLAN DAY [frontier-agent0|alns|one-exchange|complete-exchange|team-dp|sparse-exchange] or claim_probe REPLAY --recorded-sparse-exchange DAY or claim_probe REPLAY --recorded-sparse-stage DAY or claim_probe REPLAY --recorded-sparse-closed-loop DAY or claim_probe REPLAY --recorded-terminal-marginal DAY or claim_probe REPLAY --recorded-rendezvous-pairs DAY [MAX_COLUMNS] or claim_probe REPLAY --recorded-master-factor DAY or claim_probe REPLAY --recorded-sparse-budget DAY AGENT MAX_STATES [MAX_ROUTES] or claim_probe REPLAY --recorded-fuel-permutation DAY MAX_STATES [MAX_ROUTES] or claim_probe REPLAY --recorded-traffic-permutation DAY MAX_STATES [MAX_ROUTES] or claim_probe REPLAY --recorded-penultimate-robust-bound DAY MAX_STATES MAX_ROUTES or claim_probe REPLAY --recorded-penultimate-stationary-refuel-bound DAY MAX_STATES MAX_ROUTES or claim_probe REPLAY --recorded-offroad-segments DAY");
        }
        const std::int32_t targetDay = std::stoi(argv[3]);
        const std::vector<udon::JsonValue> events = read_replay(argv[1]);
        const ReplayPrefix prefix = reconstruct_prefix(events, targetDay);
        const bool recordedSparse =
            argc == 4 && std::string{argv[2]} == "--recorded-sparse-exchange";
        const bool recordedSparseStage =
            argc == 4 && std::string{argv[2]} == "--recorded-sparse-stage";
        const bool recordedSparseClosedLoop =
            argc == 4 && std::string{argv[2]} == "--recorded-sparse-closed-loop";
        const bool recordedTerminalMarginal =
            argc == 4 && std::string{argv[2]} == "--recorded-terminal-marginal";
        const bool recordedRendezvousPairs =
            (argc == 4 || argc == 5) &&
            std::string{argv[2]} == "--recorded-rendezvous-pairs";
        const bool recordedMasterFactor =
            argc == 4 && std::string{argv[2]} == "--recorded-master-factor";
        const bool recordedSparseBudget =
            (argc == 6 || argc == 7) &&
            std::string{argv[2]} == "--recorded-sparse-budget";
        const bool recordedFuelPermutation =
            (argc == 5 || argc == 6) &&
            std::string{argv[2]} == "--recorded-fuel-permutation";
        const bool recordedTrafficPermutation =
            (argc == 5 || argc == 6) &&
            std::string{argv[2]} == "--recorded-traffic-permutation";
        const bool recordedPenultimateRobustBound =
            argc == 6 &&
            std::string{argv[2]} == "--recorded-penultimate-robust-bound";
        const bool recordedPenultimateStationaryRefuelBound =
            argc == 6 && std::string{argv[2]} ==
                "--recorded-penultimate-stationary-refuel-bound";
        const bool recordedOffroadSegments =
            argc == 4 &&
            std::string{argv[2]} == "--recorded-offroad-segments";
        const udon::DayPlan plan = recordedSparse
            ? recorded_plan_for_day(events, prefix.config, targetDay)
            : (recordedSparseStage || recordedSparseClosedLoop ||
                recordedTerminalMarginal ||
                recordedRendezvousPairs ||
                recordedMasterFactor ||
                recordedSparseBudget || recordedFuelPermutation ||
               recordedTrafficPermutation ||
               recordedPenultimateRobustBound ||
               recordedPenultimateStationaryRefuelBound ||
               recordedOffroadSegments)
                ? recorded_plan_for_day(events, prefix.config, targetDay)
            : udon::parse_day_plan(
                  prefix.config,
                  udon::JsonValue::parse(read_file(argv[2])));
        const udon::ExactStepSimulator simulator(prefix.config);
        const udon::IndependentDayValidator validator(prefix.config);
        const udon::SimulationResult simulation = simulator.simulate(prefix.targetState, plan, true);
        const udon::SimulationResult validation = validator.validate(prefix.targetState, plan, true);
        std::string mismatch;
        if (!simulation.valid || !validator.agrees_with(simulation, validation, mismatch)) {
            throw std::runtime_error("frozen plan failed exact agreement: " + mismatch);
        }
        print_probe(prefix, plan, simulation);
        if (recordedMasterFactor) {
            print_master_factor_separability(prefix, plan);
        } else if (recordedSparseStage) {
            print_sparse_canonical_stage_membership(prefix, plan);
        } else if (recordedPenultimateRobustBound ||
                   recordedPenultimateStationaryRefuelBound) {
            const std::uint64_t maximumSettledStates = std::stoull(argv[4]);
            const std::size_t maximumRoutes = static_cast<std::size_t>(
                std::stoull(argv[5]));
            if (maximumSettledStates == 0U || maximumRoutes == 0U) {
                throw std::invalid_argument(
                    "MAX_STATES and MAX_ROUTES must be positive");
            }
            print_penultimate_robust_terminal_bound(
                prefix,
                plan,
                maximumSettledStates,
                maximumRoutes,
                recordedPenultimateStationaryRefuelBound);
        } else if (recordedOffroadSegments) {
            print_offroad_segment_substitution(prefix, plan);
        } else if (recordedFuelPermutation || recordedTrafficPermutation) {
            const std::uint64_t maximumSettledStates = std::stoull(argv[4]);
            const std::size_t maximumRoutes = argc == 6
                ? static_cast<std::size_t>(std::stoull(argv[5]))
                : 32U;
            if (maximumSettledStates == 0U || maximumRoutes == 0U) {
                throw std::invalid_argument(
                    "MAX_STATES and MAX_ROUTES must be positive");
            }
            print_fuel_dominating_agent_permutation(
                prefix,
                plan,
                maximumSettledStates,
                maximumRoutes,
                recordedTrafficPermutation);
        } else if (recordedRendezvousPairs) {
            const std::int32_t maximumColumnsPerAgent = argc == 5
                ? std::stoi(argv[4])
                : 64;
            if (maximumColumnsPerAgent <= 0) {
                throw std::invalid_argument("MAX_COLUMNS must be positive");
            }
            print_recorded_rendezvous_pairs(
                prefix,
                plan,
                maximumColumnsPerAgent);
        } else if (recordedTerminalMarginal) {
            static_cast<void>(print_sparse_exchange(
                prefix,
                plan,
                1250000U,
                1,
                32U,
                true));
        } else if (recordedSparseBudget) {
            const udon::AgentIndex selectedAgent = std::stoi(argv[4]);
            const std::uint64_t maximumSettledStates = std::stoull(argv[5]);
            const std::size_t maximumRoutes = argc == 7
                ? static_cast<std::size_t>(std::stoull(argv[6]))
                : 32U;
            static_cast<void>(print_sparse_exchange(
                prefix,
                plan,
                maximumSettledStates,
                selectedAgent,
                maximumRoutes));
        } else if (recordedSparseClosedLoop) {
            const udon::DayPlan sparse = print_sparse_exchange(prefix, plan);
            const ClosedLoopSuffixResult control = run_closed_loop_suffix(
                events,
                targetDay,
                plan,
                "recorded-root");
            const ClosedLoopSuffixResult forced = run_closed_loop_suffix(
                events,
                targetDay,
                sparse,
                "sparse-root");
            std::cout << "closed_loop_delta="
                      << forced.score.lifetimeDistinct - control.score.lifetimeDistinct
                      << '/'
                      << forced.score.totalDailyDistinct - control.score.totalDailyDistinct
                      << '/'
                      << forced.score.totalServings - control.score.totalServings
                      << '\n';
        } else if (recordedSparse) {
            static_cast<void>(print_sparse_exchange(prefix, plan));
        } else if (argc == 5) {
            const std::string mode{argv[4]};
            if (mode == "frontier-agent0") {
                print_frontier(prefix, 0);
            } else if (mode == "alns") {
                print_alns_diagnostics(prefix, plan);
            } else if (mode == "one-exchange") {
                print_exact_one_exchange(prefix, plan);
            } else if (mode == "complete-exchange") {
                print_complete_resource_exchange(prefix, plan);
            } else if (mode == "team-dp") {
                print_complete_resource_team_dp(prefix, plan);
            } else if (mode == "sparse-exchange") {
                static_cast<void>(print_sparse_exchange(prefix, plan));
            } else {
                throw std::invalid_argument("unknown probe mode");
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error=" << error.what() << '\n';
        return 1;
    }
}
