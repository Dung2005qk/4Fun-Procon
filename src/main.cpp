#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "udon/decision.hpp"
#include "udon/protocol.hpp"
#include "udon/runtime.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"

namespace {

[[nodiscard]] std::string read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open file: " + path);
    }
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

[[nodiscard]] std::chrono::milliseconds parse_milliseconds(const char* argument) {
    const long long value = std::stoll(argument);
    if (value < 0) {
        throw std::invalid_argument("available milliseconds cannot be negative");
    }
    return std::chrono::milliseconds{value};
}

void print_usage() {
    std::cerr << "usage:\n"
              << "  udonshield_cli roles <config.json> [beam-width]\n"
              << "  udonshield_cli plan <config.json> <state.json> <available-ms>\n"
              << "  udonshield_cli plan <config.json> <state.json> <ledger.json> <available-ms>\n"
              << "  udonshield_cli validate <config.json> <state.json> <plan.json>\n"
              << "  udonshield_cli advance-ledger <config.json> <ledger.json> <state.json> <plan.json>\n"
              << "  udonshield_cli session <config.json>\n";
}

void run_session(const udon::MatchConfig& config) {
    udon::MatchSession session(config);
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }
        const udon::JsonValue event = udon::JsonValue::parse(line);
        const std::string& type = event.at("type").string();
        udon::JsonValue::Object response;
        if (type == "state") {
            const udon::DayState state = udon::parse_day_state(config, event.at("state"));
            udon::MatchLedger ledger;
            if (event.contains("ledger")) {
                ledger = udon::parse_match_ledger(config, event.at("ledger"));
            } else if (state.dayNumber != 1) {
                throw std::invalid_argument("session state from day 2 onward requires a match ledger");
            }
            std::chrono::system_clock::time_point receivedAt = std::chrono::system_clock::now();
            if (event.contains("receivedAtUnixMs")) {
                receivedAt = std::chrono::system_clock::time_point{
                    std::chrono::milliseconds{event.at("receivedAtUnixMs").integer()}};
            }
            const udon::SessionDecision decision = session.on_authoritative_state(state, ledger, receivedAt);
            response.emplace("type", udon::JsonValue("decision"));
            response.emplace("maySubmit", udon::JsonValue(decision.maySubmit));
            response.emplace("plan", udon::serialize_day_plan(decision.decision.candidate.plan));
            response.emplace("replay", decision.replay);
        } else if (type == "ack") {
            const std::chrono::milliseconds postAckBudget = event.contains("availableMs")
                ? std::chrono::milliseconds{event.at("availableMs").integer()}
                : std::chrono::milliseconds{0};
            const udon::PostAckWork postAck = session.acknowledge_submitted(
                std::chrono::milliseconds{event.at("responseMs").integer()},
                postAckBudget);
            response.emplace("type", udon::JsonValue("acknowledged"));
            response.emplace("totalResponseMs", udon::JsonValue(
                static_cast<std::int64_t>(session.response_ledger().totalResponse.count())));
            response.emplace(
                "cachedContingencies",
                udon::JsonValue(static_cast<std::int64_t>(postAck.cachedContingencies)));
            response.emplace(
                "completedProofs",
                udon::JsonValue(static_cast<std::int64_t>(postAck.completedProofs)));
        } else if (type == "precompute") {
            const std::int32_t cached = session.precompute_until(
                std::chrono::milliseconds{event.at("availableMs").integer()});
            response.emplace("type", udon::JsonValue("precomputed"));
            response.emplace("cachedContingencies", udon::JsonValue(static_cast<std::int64_t>(cached)));
        } else if (type == "proof") {
            const std::int32_t completed = session.prove_until(
                std::chrono::milliseconds{event.at("availableMs").integer()});
            udon::JsonValue::Array records;
            for (const udon::ResponseLedger::StrongProofRecord& proof : session.response_ledger().strongProofs) {
                udon::JsonValue::Object record;
                record.emplace("day", udon::JsonValue(static_cast<std::int64_t>(proof.dayNumber)));
                record.emplace("scenarioId", udon::JsonValue(static_cast<std::int64_t>(proof.scenarioId)));
                record.emplace("scenarioClass", udon::JsonValue(proof.scenarioClass));
                record.emplace("scope", udon::JsonValue(proof.scope));
                record.emplace("complete", udon::JsonValue(proof.complete));
                record.emplace("infeasible", udon::JsonValue(proof.infeasible));
                record.emplace(
                    "combinationsVisited",
                    udon::JsonValue(static_cast<std::int64_t>(proof.combinationsVisited)));
                record.emplace(
                    "branchesPruned",
                    udon::JsonValue(static_cast<std::int64_t>(proof.branchesPruned)));
                udon::JsonValue::Object bestScore;
                bestScore.emplace(
                    "lifetimeDistinct",
                    udon::JsonValue(static_cast<std::int64_t>(proof.bestScore.lifetimeDistinct)));
                bestScore.emplace(
                    "totalDailyDistinct",
                    udon::JsonValue(static_cast<std::int64_t>(proof.bestScore.totalDailyDistinct)));
                bestScore.emplace(
                    "totalServings",
                    udon::JsonValue(static_cast<std::int64_t>(proof.bestScore.totalServings)));
                udon::JsonValue::Object upperBound;
                upperBound.emplace(
                    "lifetimeDistinct",
                    udon::JsonValue(static_cast<std::int64_t>(proof.upperBound.lifetimeDistinct)));
                upperBound.emplace(
                    "totalDailyDistinct",
                    udon::JsonValue(static_cast<std::int64_t>(proof.upperBound.totalDailyDistinct)));
                upperBound.emplace(
                    "totalServings",
                    udon::JsonValue(static_cast<std::int64_t>(proof.upperBound.totalServings)));
                record.emplace("bestScore", udon::JsonValue(std::move(bestScore)));
                record.emplace("upperBound", udon::JsonValue(std::move(upperBound)));
                records.emplace_back(std::move(record));
            }
            response.emplace("type", udon::JsonValue("proof"));
            response.emplace("completed", udon::JsonValue(static_cast<std::int64_t>(completed)));
            response.emplace("records", udon::JsonValue(std::move(records)));
        } else if (type == "roles") {
            const std::int32_t beamWidth = event.contains("beamWidth")
                ? static_cast<std::int32_t>(event.at("beamWidth").integer())
                : 3;
            const std::chrono::milliseconds available{event.at("availableMs").integer()};
            const std::vector<udon::RoleAssignment> assignments = session.select_roles_until(available, beamWidth);
            if (assignments.empty()) {
                throw std::runtime_error("role deadline expired before a viable assignment was available");
            }
            response.emplace("type", udon::JsonValue("roles"));
            response.emplace("roles", udon::serialize_role_selection(assignments.front().roles));
        } else {
            throw std::invalid_argument("session event type must be state, ack, precompute, proof, or roles");
        }
        std::cout << udon::JsonValue(std::move(response)).dump() << '\n';
        std::cout.flush();
    }
}

} 

int main(int argumentCount, char** arguments) {
    try {
        if (argumentCount < 3) {
            print_usage();
            return 2;
        }
        const std::string command = arguments[1];
        const udon::MatchConfig config = udon::parse_match_config(udon::JsonValue::parse(read_file(arguments[2])));
        if (command == "session") {
            if (argumentCount != 3) {
                print_usage();
                return 2;
            }
            run_session(config);
            return 0;
        }
        if (command == "roles") {
            const std::int32_t beamWidth =
                argumentCount >= 4 ? std::stoi(arguments[3]) : 3;
            udon::UdonShieldEngine engine(config);
            const std::vector<udon::RoleAssignment> assignments =
                engine.select_roles_until(
                    udon::kCompetitionComputeHardCap,
                    beamWidth);
            if (assignments.empty()) {
                throw std::runtime_error("no role assignment passed the cheap viability scan");
            }
            std::cout << udon::serialize_role_selection(assignments.front().roles).dump() << '\n';
            return 0;
        }
        if (command == "plan") {
            if (argumentCount != 5 && argumentCount != 6) {
                print_usage();
                return 2;
            }
            const udon::DayState state = udon::parse_day_state(config, udon::JsonValue::parse(read_file(arguments[3])));
            udon::MatchLedger ledger;
            std::chrono::milliseconds available;
            if (argumentCount == 5) {
                if (state.dayNumber != 1) {
                    throw std::runtime_error("a ledger file is required from day 2 onward");
                }
                available = parse_milliseconds(arguments[4]);
            } else {
                ledger = udon::parse_match_ledger(config, udon::JsonValue::parse(read_file(arguments[4])));
                available = parse_milliseconds(arguments[5]);
            }
            udon::UdonShieldEngine engine(config);
            const udon::DecisionResult decision = engine.solve_day(state, ledger, available);
            std::cout << udon::serialize_day_plan(decision.candidate.plan).dump() << '\n';
            return 0;
        }
        if (command == "validate") {
            if (argumentCount != 5) {
                print_usage();
                return 2;
            }
            const udon::DayState state = udon::parse_day_state(config, udon::JsonValue::parse(read_file(arguments[3])));
            const udon::DayPlan plan = udon::parse_day_plan(config, udon::JsonValue::parse(read_file(arguments[4])));
            const udon::ExactStepSimulator simulator(config);
            const udon::IndependentDayValidator validator(config);
            const udon::SimulationResult simulation = simulator.simulate(state, plan, true);
            const udon::SimulationResult validation = validator.validate(state, plan, true);
            std::string mismatch;
            if (!validator.agrees_with(simulation, validation, mismatch)) {
                throw std::runtime_error("simulator disagreement: " + mismatch);
            }
            if (!simulation.valid) {
                std::cerr << simulation.error->message << '\n';
                return 1;
            }
            std::cout << "valid\n";
            return 0;
        }
        if (command == "advance-ledger") {
            if (argumentCount != 6) {
                print_usage();
                return 2;
            }
            udon::MatchLedger ledger = udon::parse_match_ledger(config, udon::JsonValue::parse(read_file(arguments[3])));
            const udon::DayState state = udon::parse_day_state(config, udon::JsonValue::parse(read_file(arguments[4])));
            const udon::DayPlan plan = udon::parse_day_plan(config, udon::JsonValue::parse(read_file(arguments[5])));
            const udon::ExactStepSimulator simulator(config);
            const udon::IndependentDayValidator validator(config);
            const udon::SimulationResult simulation = simulator.simulate(state, plan, false);
            const udon::SimulationResult validation = validator.validate(state, plan, false);
            std::string mismatch;
            if (!simulation.valid || !validator.agrees_with(simulation, validation, mismatch)) {
                throw std::runtime_error("cannot advance ledger from an invalid plan: " + mismatch);
            }
            ledger.apply(simulation.score);
            std::cout << udon::serialize_match_ledger(config, ledger).dump() << '\n';
            return 0;
        }
        print_usage();
        return 2;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
