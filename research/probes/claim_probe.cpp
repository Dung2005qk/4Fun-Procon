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
#include "udon/json.hpp"
#include "udon/orienteering.hpp"
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
    std::cout << "day_score=" << std::popcount(simulation.score.brands) << '/'
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

void print_sparse_exchange(
    const ReplayPrefix& prefix,
    const udon::DayPlan& frozenWitness) {
    constexpr std::uint64_t kMaximumSettledStates = 1250000U;
    constexpr std::size_t kMaximumRoutes = 32U;
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
    udon::AgentIndex globalAgent = udon::kInvalidAgent;
    udon::AgentIndex protectedGlobalAgent = udon::kInvalidAgent;
    std::uint32_t globalMask = 0U;
    std::uint32_t protectedGlobalMask = 0U;
    udon::DayPlan globalPlan = frozenWitness;

    std::uint64_t preferredBrands = 0U;
    for (std::int32_t brand = 0; brand < prefix.config.brand_count(); ++brand) {
        if (!udon::has_brand(prefix.ledger.lifetimeBrands, brand)) {
            preferredBrands |= udon::brand_bit(brand);
        }
    }
    const std::int32_t minimumSpots = preferredBrands != 0U
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
        retained.reserve(kMaximumRoutes);
        std::vector<SparseRouteChoice> protectedRetained;
        protectedRetained.reserve(kMaximumRoutes);
        std::uint64_t settledStates = 0U;
        while (!queue.empty() && settledStates < kMaximumSettledStates) {
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
                std::uint64_t brands = 0U;
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
                        std::popcount(brands & preferredBrands)),
                    static_cast<std::int32_t>(std::popcount(brands)),
                    servingPotential,
                    static_cast<std::int32_t>(std::popcount(current.mask)),
                    -static_cast<std::int32_t>(current.usedSteps),
                    -static_cast<std::int32_t>(current.usedFuel),
                    -terminalBrandDistance,
                    -current.cell,
                    std::numeric_limits<std::uint32_t>::max() - current.mask};
                SparseRouteChoice choice{labelIndex, current.mask, rank};
                if (retained.size() < kMaximumRoutes) {
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
                std::uint64_t brands = 0U;
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
                        std::popcount(brands & preferredBrands)),
                    static_cast<std::int32_t>(std::popcount(brands)),
                    servingPotential,
                    static_cast<std::int32_t>(std::popcount(current.mask)),
                    -static_cast<std::int32_t>(current.usedSteps),
                    -static_cast<std::int32_t>(current.usedFuel),
                    0,
                    -current.cell,
                    std::numeric_limits<std::uint32_t>::max() - current.mask};
                SparseRouteChoice choice{labelIndex, current.mask, rank};
                if (protectedRetained.size() < kMaximumRoutes) {
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
            if (agentBest < score) {
                agentBest = score;
                agentBestMask = choice.mask;
            }
            const std::uint64_t baselineLifetime =
                prefix.ledger.lifetimeBrands | baseline.score.brands;
            const std::uint64_t candidateLifetime =
                prefix.ledger.lifetimeBrands | simulation.score.brands;
            const bool protectedImprovement =
                udon::protected_slack_transition_dominates(
                    baseline,
                    simulation) &&
                (baselineLifetime & ~candidateLifetime) == 0U &&
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
    if (baselineScore < globalBest) {
        std::cout << "sparse_best_plan="
                  << udon::serialize_day_plan(globalPlan).dump() << '\n';
    }
    if (prefix.targetState.dayNumber == prefix.config.day_count()) {
        const udon::ProtectedSlackRefiner refiner(prefix.config);
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
                  << " deadline=" << sidecar.diagnostics.deadlineReached
                  << '\n';
    }
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
    std::vector<std::uint64_t> suffixBrands(patrols.size() + 1U, 0U);
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
                            std::uint64_t brands) -> bool {
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
            static_cast<std::int32_t>(std::popcount(brands | suffixBrands.at(depth))) <
                targetDailyDistinct) {
            return false;
        }
        if (!memo.at(depth).insert(count_key()).second) {
            return false;
        }
        if (depth == patrols.size()) {
            if (servings >= targetServings &&
                static_cast<std::int32_t>(std::popcount(brands)) >= targetDailyDistinct) {
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
            std::uint64_t addedBrands = 0U;
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

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 4 && argc != 5) {
            throw std::invalid_argument(
                "usage: claim_probe REPLAY PLAN DAY [frontier-agent0|alns|one-exchange|complete-exchange|team-dp|sparse-exchange] or claim_probe REPLAY --recorded-sparse-exchange DAY");
        }
        const std::int32_t targetDay = std::stoi(argv[3]);
        const std::vector<udon::JsonValue> events = read_replay(argv[1]);
        const ReplayPrefix prefix = reconstruct_prefix(events, targetDay);
        const bool recordedSparse =
            argc == 4 && std::string{argv[2]} == "--recorded-sparse-exchange";
        const udon::DayPlan plan = recordedSparse
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
        if (recordedSparse) {
            print_sparse_exchange(prefix, plan);
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
                print_sparse_exchange(prefix, plan);
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
