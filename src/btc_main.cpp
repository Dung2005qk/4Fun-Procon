#include <algorithm>
#include <bit>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#endif

#include "udon/btc_protocol.hpp"
#include "udon/protocol.hpp"
#include "udon/runtime.hpp"
#include "udon/simulator.hpp"
#include "udon/slack_refiner.hpp"
#include "udon/validator.hpp"

namespace {

constexpr std::int64_t btcSubmissionFloorMs = 800;
constexpr std::int64_t btcProtectedRefinementFloorMs = 1100;
constexpr std::int32_t btcActionAckSliceMs = 750;

struct RuntimeOptions {
    std::string mode;
    std::string baseUrl = "https://procon.ptit.edu.vn";
    std::string matchId;
    std::string replayPath;
    std::string decisionDumpPath;
    std::int32_t responseBudgetMs = 5000;
    std::int32_t pollMs = 220;
    std::int32_t actionAckTimeoutMs = btcActionAckSliceMs;
    std::int32_t beamWidth = 8;
    std::int32_t dayNumber = 0;
    std::int32_t roleMask = -1;
    std::int32_t maximumReplayDays = std::numeric_limits<std::int32_t>::max();
    std::int32_t harvestExtensionMode = 7;
    std::int32_t futureHarvestExtensionMode = -1;
    std::int32_t logicBudgetMs = 0;
    std::int32_t postAckMs = 0;
    bool requireUndominatedCurrentFloor = false;
    bool shortHorizonRoleFallback = false;
};

[[nodiscard]] std::int32_t effective_response_budget_ms(
    const RuntimeOptions& options) {
    return static_cast<std::int32_t>(udon::competition_compute_budget(
        std::chrono::milliseconds{options.responseBudgetMs}).count());
}

[[nodiscard]] std::int32_t resolved_future_harvest_extension_mode(
    const RuntimeOptions& options) {
    if (options.futureHarvestExtensionMode >= 0) {
        return options.futureHarvestExtensionMode;
    }
    if (options.harvestExtensionMode > 6) {
        return 7;
    }
    return options.harvestExtensionMode > 5 ? 5 : -1;
}

struct HttpResponse {
    std::int32_t status = 0;
    std::string body;
    std::int32_t transportRetries = 0;
};

[[nodiscard]] std::int64_t unix_milliseconds() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

[[nodiscard]] udon::DeadlineCalibration btc_http_deadline_calibration() {
    udon::DeadlineCalibration calibration;
    calibration.version = "btc-http-local-budget-v8-idempotent-ack-resend";
    calibration.networkFloor = std::chrono::milliseconds{1600};
    calibration.networkPercent = 20;
    calibration.certificationPercent = 20;
    return calibration;
}

class ReplayWriter {
public:
    explicit ReplayWriter(const std::string& path) {
        if (path.empty()) {
            return;
        }
        const std::filesystem::path replayPath(path);
        if (replayPath.has_parent_path()) {
            std::filesystem::create_directories(replayPath.parent_path());
        }
        output_.open(replayPath, std::ios::binary | std::ios::app);
        if (!output_) {
            throw std::runtime_error("cannot open BTC replay file: " + path);
        }
    }

    void record(const std::string& kind, const udon::JsonValue& body, std::optional<std::int32_t> status = std::nullopt) {
        if (!output_) {
            return;
        }
        udon::JsonValue::Object event;
        event.emplace("atUnixMs", udon::JsonValue(unix_milliseconds()));
        event.emplace("kind", udon::JsonValue(kind));
        if (status.has_value()) {
            event.emplace("status", udon::JsonValue(static_cast<std::int64_t>(*status)));
        }
        event.emplace("body", body);
        output_ << udon::JsonValue(std::move(event)).dump() << '\n';
        output_.flush();
    }

private:
    std::ofstream output_;
};

[[nodiscard]] std::int32_t parse_positive_integer(const std::string& value, const std::string& name) {
    const long long parsed = std::stoll(value);
    if (parsed <= 0 || parsed > std::numeric_limits<std::int32_t>::max()) {
        throw std::invalid_argument(name + " must be a positive 32-bit integer");
    }
    return static_cast<std::int32_t>(parsed);
}

[[nodiscard]] RuntimeOptions parse_arguments(int argumentCount, char** arguments) {
    if (argumentCount < 2) {
        throw std::invalid_argument("missing BTC transport mode");
    }
    RuntimeOptions options;
    options.mode = arguments[1];
    for (int index = 2; index < argumentCount; ++index) {
        const std::string key = arguments[index];
        if (index + 1 >= argumentCount) {
            throw std::invalid_argument("missing value for argument: " + key);
        }
        const std::string value = arguments[++index];
        if (key == "--url") {
            options.baseUrl = value;
        } else if (key == "--match") {
            options.matchId = value;
        } else if (key == "--replay") {
            options.replayPath = value;
        } else if (key == "--decision-dump") {
            options.decisionDumpPath = value;
        } else if (key == "--response-ms") {
            options.responseBudgetMs = parse_positive_integer(value, key);
        } else if (key == "--logic-budget-ms") {
            options.logicBudgetMs = parse_positive_integer(value, key);
        } else if (key == "--post-ack-ms") {
            options.postAckMs = parse_positive_integer(value, key);
        } else if (key == "--current-floor") {
            if (value != "0" && value != "1") {
                throw std::invalid_argument("--current-floor must be 0 or 1");
            }
            options.requireUndominatedCurrentFloor = value == "1";
        } else if (key == "--short-role-fallback") {
            if (value != "0" && value != "1") {
                throw std::invalid_argument("--short-role-fallback must be 0 or 1");
            }
            options.shortHorizonRoleFallback = value == "1";
        } else if (key == "--poll-ms") {
            options.pollMs = parse_positive_integer(value, key);
        } else if (key == "--action-ack-ms") {
            options.actionAckTimeoutMs = parse_positive_integer(value, key);
        } else if (key == "--beam-width") {
            options.beamWidth = parse_positive_integer(value, key);
        } else if (key == "--day") {
            options.dayNumber = parse_positive_integer(value, key);
        } else if (key == "--max-days") {
            options.maximumReplayDays = parse_positive_integer(value, key);
        } else if (key == "--role-mask") {
            const long long parsed = std::stoll(value);
            if (parsed < 0 || parsed > 255) {
                throw std::invalid_argument("--role-mask must be in [0,255]");
            }
            options.roleMask = static_cast<std::int32_t>(parsed);
        } else if (key == "--harvest-extensions") {
            const long long parsed = std::stoll(value);
            if (parsed < 0 || parsed > 7) {
                throw std::invalid_argument("--harvest-extensions must be in [0,7]");
            }
            options.harvestExtensionMode = static_cast<std::int32_t>(parsed);
        } else if (key == "--future-harvest-extensions") {
            const long long parsed = std::stoll(value);
            if (parsed < 0 || parsed > 7) {
                throw std::invalid_argument("--future-harvest-extensions must be in [0,7]");
            }
            options.futureHarvestExtensionMode = static_cast<std::int32_t>(parsed);
        } else {
            throw std::invalid_argument("unsupported argument: " + key);
        }
    }
    if (options.pollMs < 200) {
        throw std::invalid_argument("--poll-ms must be at least 200 for the BTC rate limit");
    }
    if (options.actionAckTimeoutMs > 5000) {
        throw std::invalid_argument("--action-ack-ms must not exceed the 5000 ms hard cap");
    }
    if (options.mode == "http" && options.matchId.empty()) {
        throw std::invalid_argument("HTTP mode requires --match");
    }
    if (options.mode == "replay-solve" &&
        (options.replayPath.empty() || options.dayNumber <= 0)) {
        throw std::invalid_argument("replay-solve requires --replay and --day");
    }
    if (options.mode == "replay-roles" && options.replayPath.empty()) {
        throw std::invalid_argument("replay-roles requires --replay");
    }
    if (options.mode == "replay-counterfactual" &&
        (options.replayPath.empty() || options.roleMask < 0)) {
        throw std::invalid_argument(
            "replay-counterfactual requires --replay and --role-mask");
    }
    if (options.logicBudgetMs > 0 &&
        options.mode != "replay-counterfactual") {
        throw std::invalid_argument(
            "--logic-budget-ms is only valid for replay-counterfactual");
    }
    if (options.postAckMs > 0 &&
        options.mode != "replay-counterfactual") {
        throw std::invalid_argument(
            "--post-ack-ms is only valid for replay-counterfactual");
    }
    if (options.requireUndominatedCurrentFloor &&
        options.mode != "replay-counterfactual") {
        throw std::invalid_argument(
            "--current-floor is only valid for replay-counterfactual");
    }
    if (options.futureHarvestExtensionMode >= 0 &&
        options.mode != "replay-counterfactual" &&
        options.mode != "http" &&
        options.mode != "sandbox") {
        throw std::invalid_argument(
            "--future-harvest-extensions is only valid for replay-counterfactual, HTTP, or sandbox");
    }
    return options;
}

void print_usage() {
    std::cerr
        << "usage:\n"
        << "  udonshield_btc sandbox [--response-ms 5000] [--beam-width 8] "
           "[--harvest-extensions 0|1|2|3|4|5|6|7] "
           "[--future-harvest-extensions 0|1|2|3|4|5|6|7] [--replay replay.jsonl]\n"
        << "  udonshield_btc http --match MATCH_ID [--url https://procon.ptit.edu.vn] "
           "[--response-ms 5000] [--poll-ms 220] [--action-ack-ms 750] [--beam-width 8] "
           "[--harvest-extensions 0|1|2|3|4|5|6|7] "
           "[--future-harvest-extensions 0|1|2|3|4|5|6|7] [--replay replay.jsonl]\n"
        << "  udonshield_btc replay-check --replay replay.jsonl [--response-ms 5000]\n"
        << "  udonshield_btc replay-roles --replay replay.jsonl [--response-ms 5000] [--beam-width 8] "
           "[--short-role-fallback 0|1]\n"
        << "  udonshield_btc replay-counterfactual --replay replay.jsonl --role-mask MASK "
           "[--response-ms 5000] [--harvest-extensions 0|1|2|3|4|5|6|7] [--max-days N] "
           "[--future-harvest-extensions 0|1|2|3|4|5|6|7] [--logic-budget-ms N] [--current-floor 0|1] "
           "[--short-role-fallback 0|1] [--decision-dump decisions.jsonl]\n"
        << "  udonshield_btc replay-solve --replay replay.jsonl --day DAY [--response-ms 5000]\n"
        << "HTTP mode reads the bearer token only from HEXUDON_TOKEN.\n";
}

[[nodiscard]] udon::JsonValue parse_optional_json(const std::string& body) {
    return body.empty() ? udon::JsonValue(nullptr) : udon::JsonValue::parse(body);
}

[[nodiscard]] udon::JsonValue parse_tolerant_http_body(const std::string& body) {
    if (body.empty()) {
        return udon::JsonValue(nullptr);
    }
    try {
        return udon::JsonValue::parse(body);
    } catch (const std::exception&) {
        return udon::JsonValue(body);
    }
}

[[nodiscard]] udon::SimulationResult validate_fallback_plan(
    const udon::DayState& state,
    const udon::DayPlan& plan,
    const udon::ExactStepSimulator& simulator,
    const udon::IndependentDayValidator& validator,
    const std::string& context) {
    const udon::SimulationResult simulation = simulator.simulate(
        state,
        plan,
        false);
    const udon::SimulationResult validation = validator.validate(
        state,
        plan,
        false);
    std::string mismatch;
    if (!simulation.valid || !validator.agrees_with(simulation, validation, mismatch)) {
        throw std::runtime_error(context + ": " + mismatch);
    }
    return simulation;
}

[[nodiscard]] std::optional<udon::SimulationResult> try_validate_plan(
    const udon::DayState& state,
    const udon::DayPlan& plan,
    const udon::ExactStepSimulator& simulator,
    const udon::IndependentDayValidator& validator,
    std::string& failure) {
    try {
        return validate_fallback_plan(
            state,
            plan,
            simulator,
            validator,
            "plan revalidation failed");
    } catch (const std::exception& error) {
        failure = error.what();
        return std::nullopt;
    }
}

struct ReplayResumeState {
    struct AcceptedTransition {
        udon::DayState state;
        udon::SimulationResult simulation;
    };

    std::optional<udon::JsonValue> assignment;
    bool assignmentAccepted = false;
    udon::MatchLedger ledger;
    udon::MatchLedger checkpointLedger;
    udon::MatchLedger virtualLedger;
    std::optional<std::vector<udon::AgentState>> checkpointAgents;
    std::optional<std::vector<udon::AgentState>> virtualAgents;
    bool checkpointDivergenceActive = false;
    bool protectedDivergenceActive = false;
    std::int32_t lastAcceptedWireDay = -1;
    std::vector<AcceptedTransition> acceptedTransitions;
    std::optional<std::int32_t> artifactsAfterDay;
    std::vector<udon::ResponseLedger::CachedContingency> cachedContingencies;
    std::vector<udon::ResponseLedger::StrongProofRecord> strongProofs;
};

[[nodiscard]] std::int32_t checkpoint_integer(
    const udon::JsonValue& value,
    const std::string& field) {
    if (!value.is_number()) {
        throw std::runtime_error("BTC session checkpoint field " + field + " is not an integer");
    }
    const std::int64_t integer = value.integer();
    if (integer < std::numeric_limits<std::int32_t>::min() ||
        integer > std::numeric_limits<std::int32_t>::max()) {
        throw std::runtime_error("BTC session checkpoint field " + field + " is out of range");
    }
    return static_cast<std::int32_t>(integer);
}

[[nodiscard]] udon::JsonValue serialize_checkpoint_score(
    const udon::OfficialScore& score) {
    udon::JsonValue::Object object;
    object.emplace("lifetimeDistinct", udon::JsonValue(static_cast<std::int64_t>(score.lifetimeDistinct)));
    object.emplace("totalDailyDistinct", udon::JsonValue(static_cast<std::int64_t>(score.totalDailyDistinct)));
    object.emplace("totalServings", udon::JsonValue(static_cast<std::int64_t>(score.totalServings)));
    return udon::JsonValue(std::move(object));
}

[[nodiscard]] udon::OfficialScore parse_checkpoint_score(
    const udon::JsonValue& document) {
    if (!document.is_object()) {
        throw std::runtime_error("BTC session checkpoint score is not an object");
    }
    return udon::OfficialScore{
        checkpoint_integer(document.at("lifetimeDistinct"), "lifetimeDistinct"),
        checkpoint_integer(document.at("totalDailyDistinct"), "totalDailyDistinct"),
        checkpoint_integer(document.at("totalServings"), "totalServings"),
    };
}

[[nodiscard]] udon::JsonValue serialize_checkpoint_witness(
    const udon::FutureWitness& witness) {
    udon::JsonValue::Array plans;
    plans.reserve(witness.futurePlans.size());
    for (const udon::DayPlan& plan : witness.futurePlans) {
        plans.push_back(udon::serialize_day_plan(plan));
    }
    udon::JsonValue::Object object;
    object.emplace("futurePlans", udon::JsonValue(std::move(plans)));
    object.emplace("score", serialize_checkpoint_score(witness.score));
    object.emplace("certified", udon::JsonValue(witness.certified));
    object.emplace("lowerBoundOnly", udon::JsonValue(witness.lowerBoundOnly));
    return udon::JsonValue(std::move(object));
}

[[nodiscard]] udon::FutureWitness parse_checkpoint_witness(
    const udon::MatchConfig& config,
    const udon::JsonValue& document) {
    if (!document.is_object() || !document.at("futurePlans").is_array() ||
        !document.at("certified").is_bool() ||
        !document.at("lowerBoundOnly").is_bool()) {
        throw std::runtime_error("BTC session checkpoint witness has an invalid shape");
    }
    udon::FutureWitness witness;
    for (const udon::JsonValue& plan : document.at("futurePlans").array()) {
        witness.futurePlans.push_back(udon::parse_day_plan(config, plan));
    }
    witness.score = parse_checkpoint_score(document.at("score"));
    witness.certified = document.at("certified").boolean();
    witness.lowerBoundOnly = document.at("lowerBoundOnly").boolean();
    return witness;
}

[[nodiscard]] udon::JsonValue serialize_session_checkpoint(
    std::int32_t acceptedDay,
    const udon::ResponseLedger& ledger) {
    udon::JsonValue::Array contingencies;
    contingencies.reserve(ledger.cachedContingencies.size());
    for (const udon::ResponseLedger::CachedContingency& contingency :
         ledger.cachedContingencies) {
        udon::JsonValue::Object object;
        object.emplace("day", udon::JsonValue(static_cast<std::int64_t>(contingency.dayNumber)));
        object.emplace("scenarioId", udon::JsonValue(static_cast<std::int64_t>(contingency.scenarioId)));
        object.emplace("scenarioClass", udon::JsonValue(contingency.scenarioClass));
        object.emplace("plan", udon::serialize_day_plan(contingency.plan));
        object.emplace(
            "certifiedSuffix",
            contingency.certifiedSuffix.has_value()
                ? serialize_checkpoint_witness(*contingency.certifiedSuffix)
                : udon::JsonValue(nullptr));
        contingencies.emplace_back(std::move(object));
    }
    udon::JsonValue::Array proofs;
    proofs.reserve(ledger.strongProofs.size());
    for (const udon::ResponseLedger::StrongProofRecord& proof : ledger.strongProofs) {
        udon::JsonValue::Object object;
        object.emplace("day", udon::JsonValue(static_cast<std::int64_t>(proof.dayNumber)));
        object.emplace("scenarioId", udon::JsonValue(static_cast<std::int64_t>(proof.scenarioId)));
        object.emplace("scenarioClass", udon::JsonValue(proof.scenarioClass));
        object.emplace("scope", udon::JsonValue(proof.scope));
        object.emplace("bestScore", serialize_checkpoint_score(proof.bestScore));
        object.emplace("upperBound", serialize_checkpoint_score(proof.upperBound));
        object.emplace("combinationsVisited", udon::JsonValue(static_cast<std::int64_t>(proof.combinationsVisited)));
        object.emplace("branchesPruned", udon::JsonValue(static_cast<std::int64_t>(proof.branchesPruned)));
        object.emplace("complete", udon::JsonValue(proof.complete));
        object.emplace("infeasible", udon::JsonValue(proof.infeasible));
        proofs.emplace_back(std::move(object));
    }
    udon::JsonValue::Object checkpoint;
    checkpoint.emplace("acceptedDay", udon::JsonValue(static_cast<std::int64_t>(acceptedDay)));
    checkpoint.emplace("cachedContingencies", udon::JsonValue(std::move(contingencies)));
    checkpoint.emplace("strongProofs", udon::JsonValue(std::move(proofs)));
    return udon::JsonValue(std::move(checkpoint));
}

void parse_session_checkpoint(
    const udon::MatchConfig& config,
    const udon::JsonValue& document,
    ReplayResumeState& resume) {
    if (!document.is_object() ||
        !document.at("cachedContingencies").is_array() ||
        !document.at("strongProofs").is_array()) {
        throw std::runtime_error("BTC session checkpoint has an invalid shape");
    }
    const std::int32_t acceptedDay = checkpoint_integer(
        document.at("acceptedDay"),
        "acceptedDay");
    if (acceptedDay != resume.lastAcceptedWireDay + 1) {
        throw std::runtime_error("BTC session checkpoint is not attached to the latest accepted day");
    }
    std::vector<udon::ResponseLedger::CachedContingency> contingencies;
    for (const udon::JsonValue& value : document.at("cachedContingencies").array()) {
        if (!value.is_object() || !value.at("scenarioClass").is_string()) {
            throw std::runtime_error("BTC cached contingency has an invalid shape");
        }
        udon::ResponseLedger::CachedContingency contingency;
        contingency.dayNumber = checkpoint_integer(value.at("day"), "contingency.day");
        contingency.scenarioId = checkpoint_integer(value.at("scenarioId"), "contingency.scenarioId");
        contingency.scenarioClass = value.at("scenarioClass").string();
        contingency.plan = udon::parse_day_plan(config, value.at("plan"));
        if (!value.at("certifiedSuffix").is_null()) {
            contingency.certifiedSuffix = parse_checkpoint_witness(
                config,
                value.at("certifiedSuffix"));
        }
        contingencies.push_back(std::move(contingency));
    }
    std::vector<udon::ResponseLedger::StrongProofRecord> proofs;
    for (const udon::JsonValue& value : document.at("strongProofs").array()) {
        if (!value.is_object() || !value.at("scenarioClass").is_string() ||
            !value.at("scope").is_string() || !value.at("complete").is_bool() ||
            !value.at("infeasible").is_bool()) {
            throw std::runtime_error("BTC strong proof checkpoint has an invalid shape");
        }
        udon::ResponseLedger::StrongProofRecord proof;
        proof.dayNumber = checkpoint_integer(value.at("day"), "proof.day");
        proof.scenarioId = checkpoint_integer(value.at("scenarioId"), "proof.scenarioId");
        proof.scenarioClass = value.at("scenarioClass").string();
        proof.scope = value.at("scope").string();
        proof.bestScore = parse_checkpoint_score(value.at("bestScore"));
        proof.upperBound = parse_checkpoint_score(value.at("upperBound"));
        proof.combinationsVisited = checkpoint_integer(value.at("combinationsVisited"), "proof.combinationsVisited");
        proof.branchesPruned = checkpoint_integer(value.at("branchesPruned"), "proof.branchesPruned");
        proof.complete = value.at("complete").boolean();
        proof.infeasible = value.at("infeasible").boolean();
        proofs.push_back(std::move(proof));
    }
    resume.artifactsAfterDay = acceptedDay;
    resume.cachedContingencies = std::move(contingencies);
    resume.strongProofs = std::move(proofs);
}

[[nodiscard]] std::vector<udon::ResponseLedger::CachedContingency>
reconstruct_initial_contingencies(
    const udon::MatchConfig& config,
    const udon::JsonValue& replayDecision) {
    const udon::JsonValue& decision = replayDecision.at("decision");
    const std::int32_t decisionDay = checkpoint_integer(
        decision.at("dayNumber"),
        "decision.dayNumber");
    const udon::JsonValue::Array& outcomes =
        decision.at("profile").at("outcomes").array();
    const udon::JsonValue::Array& scenarios =
        decision.at("manifest").at("scenarios").array();
    std::set<std::string> planIds;
    std::vector<udon::ResponseLedger::CachedContingency> contingencies;
    for (std::size_t index = 0; index < outcomes.size(); ++index) {
        const udon::JsonValue& outcome = outcomes.at(index);
        if (!outcome.at("certified").is_bool() ||
            !outcome.at("certified").boolean() ||
            !outcome.at("futurePlans").is_array() ||
            outcome.at("futurePlans").array().empty()) {
            continue;
        }
        udon::DayPlan plan = udon::parse_day_plan(
            config,
            outcome.at("futurePlans").array().front());
        const std::string planId = udon::serialize_day_plan(plan).dump();
        if (!planIds.insert(planId).second) {
            continue;
        }
        udon::ResponseLedger::CachedContingency contingency;
        contingency.dayNumber = decisionDay + 1;
        contingency.plan = std::move(plan);
        if (index < scenarios.size()) {
            contingency.scenarioId = checkpoint_integer(
                scenarios.at(index).at("id"),
                "scenario.id");
            contingency.scenarioClass = scenarios.at(index).at("class").string();
        } else {
            contingency.scenarioId = static_cast<std::int32_t>(index);
            contingency.scenarioClass = "certified-cache";
        }
        if (config.roadCells.empty()) {
            udon::FutureWitness suffix;
            for (const udon::JsonValue& futurePlan : outcome.at("futurePlans").array()) {
                suffix.futurePlans.push_back(udon::parse_day_plan(config, futurePlan));
            }
            suffix.score = parse_checkpoint_score(outcome.at("witnessScore"));
            suffix.certified = true;
            suffix.lowerBoundOnly = outcome.at("lowerBoundOnly").boolean();
            contingency.certifiedSuffix = std::move(suffix);
        }
        contingencies.push_back(std::move(contingency));
    }
    return contingencies;
}

[[nodiscard]] bool exact_agent_states_equal(
    const std::vector<udon::AgentState>& left,
    const std::vector<udon::AgentState>& right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (left.at(index).kind != right.at(index).kind ||
            left.at(index).position != right.at(index).position ||
            left.at(index).fuel != right.at(index).fuel) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool exact_ledgers_equal(
    const udon::MatchLedger& left,
    const udon::MatchLedger& right) {
    return left.lifetimeBrands == right.lifetimeBrands &&
        left.totalDailyDistinct == right.totalDailyDistinct &&
        left.totalServings == right.totalServings;
}

[[nodiscard]] ReplayResumeState load_replay_resume(
    const std::string& path,
    const udon::MatchConfig& config,
    const udon::BtcAdapterOptions& adapterOptions) {
    ReplayResumeState resume;
    if (path.empty() || !std::filesystem::exists(path)) {
        return resume;
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot read BTC replay for resume: " + path);
    }
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    std::optional<udon::DayState> currentState;
    std::optional<udon::DayState> currentCheckpointState;
    std::optional<udon::DayState> currentVirtualState;
    std::optional<udon::SimulationResult> pendingSimulation;
    std::optional<udon::SimulationResult> pendingCheckpointSimulation;
    std::optional<udon::SimulationResult> pendingVirtualSimulation;
    std::optional<std::vector<udon::ResponseLedger::CachedContingency>>
        pendingInitialContingencies;
    std::int32_t pendingWireDay = -1;
    const auto invalidate_response_artifacts = [&resume]() {
        resume.artifactsAfterDay.reset();
        resume.cachedContingencies.clear();
        resume.strongProofs.clear();
    };
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const udon::JsonValue event = udon::JsonValue::parse(line);
        const std::string& kind = event.at("kind").string();
        if (kind == "assignment") {
            resume.assignment = event.at("body");
            continue;
        }
        if (kind == "assignment_result") {
            resume.assignmentAccepted = resume.assignment.has_value() &&
                udon::btc_action_result_accepted(event.at("body"));
            continue;
        }
        if (kind == "session_checkpoint") {
            parse_session_checkpoint(config, event.at("body"), resume);
            continue;
        }
        if (kind == "day_state") {
            const std::int64_t atUnixMs = event.at("atUnixMs").integer();
            currentState = udon::parse_btc_day_state(
                config,
                event.at("body"),
                std::chrono::system_clock::time_point{std::chrono::milliseconds{atUnixMs}},
                adapterOptions);
            currentCheckpointState = currentState;
            if (resume.checkpointDivergenceActive &&
                resume.checkpointAgents.has_value() &&
                udon::protected_slack_agents_dominate(
                    *resume.checkpointAgents,
                    currentState->agents) &&
                udon::protected_slack_ledger_dominates(
                    resume.checkpointLedger,
                    resume.ledger)) {
                currentCheckpointState->agents = *resume.checkpointAgents;
            } else {
                resume.checkpointLedger = resume.ledger;
                resume.checkpointAgents = currentState->agents;
                resume.checkpointDivergenceActive = false;
            }
            currentVirtualState = currentCheckpointState;
            if (resume.protectedDivergenceActive &&
                resume.virtualAgents.has_value() &&
                udon::protected_slack_agents_dominate(
                    *resume.virtualAgents,
                    currentCheckpointState->agents) &&
                udon::protected_slack_ledger_dominates(
                    resume.virtualLedger,
                    resume.checkpointLedger)) {
                currentVirtualState->agents = *resume.virtualAgents;
            } else {
                resume.virtualLedger = resume.checkpointLedger;
                resume.virtualAgents = currentCheckpointState->agents;
                resume.protectedDivergenceActive = false;
            }
            pendingWireDay = currentState->dayNumber - 1;
            pendingSimulation.reset();
            pendingCheckpointSimulation.reset();
            pendingVirtualSimulation.reset();
            pendingInitialContingencies.reset();
            continue;
        }
        if (kind == "decision") {
            if (!currentVirtualState.has_value()) {
                throw std::runtime_error(
                    "BTC replay resume found a decision without a day state");
            }
            const udon::DayPlan plan = udon::parse_day_plan(
                config,
                event.at("body")
                    .at("decision")
                    .at("candidate")
                    .at("plan"));
            const udon::SimulationResult simulation =
                simulator.simulate(*currentVirtualState, plan, false);
            const udon::SimulationResult validation =
                validator.validate(*currentVirtualState, plan, false);
            std::string mismatch;
            if (!simulation.valid ||
                !validator.agrees_with(simulation, validation, mismatch)) {
                throw std::runtime_error(
                    "BTC replay resume rejected a virtual-parent plan: " +
                    mismatch);
            }
            pendingVirtualSimulation = simulation;
            pendingInitialContingencies = reconstruct_initial_contingencies(
                config,
                event.at("body"));
            continue;
        }
        if (kind == "checkpoint_actions") {
            if (!currentCheckpointState.has_value()) {
                throw std::runtime_error(
                    "BTC replay resume found checkpoint actions without a day state");
            }
            const udon::DayPlan plan = udon::parse_day_plan(
                config,
                event.at("body"));
            const udon::SimulationResult simulation =
                simulator.simulate(*currentCheckpointState, plan, false);
            const udon::SimulationResult validation =
                validator.validate(*currentCheckpointState, plan, false);
            std::string mismatch;
            if (!simulation.valid ||
                !validator.agrees_with(simulation, validation, mismatch)) {
                throw std::runtime_error(
                    "BTC replay resume rejected a checkpoint plan: " +
                    mismatch);
            }
            pendingCheckpointSimulation = simulation;
            continue;
        }
        if (kind == "actions" || kind == "actions_fallback" ||
            kind == "actions_recovery_wait" || kind == "actions_server_wait") {
            if (!currentState.has_value()) {
                throw std::runtime_error("BTC replay resume found actions without a day state");
            }
            const udon::DayPlan plan = udon::parse_day_plan(config, event.at("body"));
            const udon::SimulationResult simulation = simulator.simulate(*currentState, plan, false);
            const udon::SimulationResult validation = validator.validate(*currentState, plan, false);
            std::string mismatch;
            if (!simulation.valid || !validator.agrees_with(simulation, validation, mismatch)) {
                throw std::runtime_error("BTC replay resume rejected a recorded plan: " + mismatch);
            }
            pendingSimulation = simulation;
            if (kind != "actions") {
                pendingCheckpointSimulation.reset();
                pendingVirtualSimulation.reset();
                pendingInitialContingencies.reset();
            }
            if (kind == "actions_server_wait" &&
                pendingWireDay > resume.lastAcceptedWireDay) {
                resume.ledger.apply(pendingSimulation->score);
                resume.checkpointLedger = resume.ledger;
                resume.checkpointAgents = pendingSimulation->finalAgents;
                resume.checkpointDivergenceActive = false;
                resume.virtualLedger = resume.ledger;
                resume.virtualAgents = pendingSimulation->finalAgents;
                resume.protectedDivergenceActive = false;
                resume.lastAcceptedWireDay = pendingWireDay;
                invalidate_response_artifacts();
                resume.acceptedTransitions.push_back(
                    ReplayResumeState::AcceptedTransition{
                        *currentState,
                        *pendingSimulation});
                pendingSimulation.reset();
                pendingCheckpointSimulation.reset();
            }
            continue;
        }
        if (kind != "action_result" && kind != "action_result_recovery") {
            continue;
        }
        const bool accepted = udon::btc_action_result_accepted(event.at("body"));
        const std::optional<std::int32_t> acceptedDay =
            udon::btc_action_result_day(event.at("body"));
        if (!pendingSimulation.has_value() ||
            pendingWireDay <= resume.lastAcceptedWireDay ||
            !accepted) {
            continue;
        }
        if (accepted && acceptedDay.has_value() &&
            *acceptedDay != pendingWireDay + 1) {
            throw std::runtime_error(
                "BTC replay resume found an accepted action for the wrong day: expected " +
                std::to_string(pendingWireDay + 1) + ", got " +
                std::to_string(*acceptedDay));
        }
        const udon::SimulationResult actualSimulation = *pendingSimulation;
        resume.ledger.apply(actualSimulation.score);
        const udon::SimulationResult* checkpointSimulation =
            pendingCheckpointSimulation.has_value()
            ? &*pendingCheckpointSimulation
            : &actualSimulation;
        udon::MatchLedger candidateCheckpointLedger =
            resume.checkpointLedger;
        candidateCheckpointLedger.apply(checkpointSimulation->score);
        if (udon::protected_slack_transition_dominates(
                *checkpointSimulation,
                actualSimulation) &&
            udon::protected_slack_ledger_dominates(
                candidateCheckpointLedger,
                resume.ledger)) {
            resume.checkpointLedger = candidateCheckpointLedger;
            resume.checkpointAgents = checkpointSimulation->finalAgents;
            resume.checkpointDivergenceActive =
                resume.checkpointDivergenceActive ||
                !exact_ledgers_equal(
                    resume.checkpointLedger,
                    resume.ledger) ||
                !exact_agent_states_equal(
                    *resume.checkpointAgents,
                    actualSimulation.finalAgents);
        } else {
            resume.checkpointLedger = resume.ledger;
            resume.checkpointAgents = actualSimulation.finalAgents;
            resume.checkpointDivergenceActive = false;
            checkpointSimulation = &actualSimulation;
        }
        if (pendingVirtualSimulation.has_value()) {
            udon::MatchLedger candidateVirtualLedger = resume.virtualLedger;
            candidateVirtualLedger.apply(pendingVirtualSimulation->score);
            if (udon::protected_slack_transition_dominates(
                    *pendingVirtualSimulation,
                    *checkpointSimulation) &&
                udon::protected_slack_ledger_dominates(
                    candidateVirtualLedger,
                    resume.checkpointLedger)) {
                resume.virtualLedger = candidateVirtualLedger;
                resume.virtualAgents =
                    pendingVirtualSimulation->finalAgents;
                resume.protectedDivergenceActive =
                    resume.protectedDivergenceActive ||
                    !exact_ledgers_equal(
                        resume.virtualLedger,
                        resume.checkpointLedger) ||
                    !exact_agent_states_equal(
                        *resume.virtualAgents,
                        checkpointSimulation->finalAgents);
            } else {
                resume.virtualLedger = resume.checkpointLedger;
                resume.virtualAgents = checkpointSimulation->finalAgents;
                resume.protectedDivergenceActive = false;
            }
        } else {
            resume.virtualLedger = resume.checkpointLedger;
            resume.virtualAgents = checkpointSimulation->finalAgents;
            resume.protectedDivergenceActive = false;
        }
        resume.lastAcceptedWireDay = pendingWireDay;
        invalidate_response_artifacts();
        if (pendingInitialContingencies.has_value()) {
            resume.artifactsAfterDay = pendingWireDay + 1;
            resume.cachedContingencies = *pendingInitialContingencies;
        }
        resume.acceptedTransitions.push_back(
            ReplayResumeState::AcceptedTransition{
                *currentState,
                actualSimulation});
        pendingSimulation.reset();
        pendingCheckpointSimulation.reset();
        pendingVirtualSimulation.reset();
        pendingInitialContingencies.reset();
    }
    return resume;
}

void emit_wire(const udon::JsonValue& value) {
    std::cout << value.dump() << '\n';
    std::cout.flush();
}

void run_replay_check(const RuntimeOptions& options) {
    if (options.replayPath.empty()) {
        throw std::invalid_argument("replay-check requires --replay");
    }
    std::ifstream input(options.replayPath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open BTC replay: " + options.replayPath);
    }
    std::vector<udon::JsonValue> events;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            events.push_back(udon::JsonValue::parse(line));
        }
    }
    const auto setupEvent = std::find_if(events.begin(), events.end(), [](const udon::JsonValue& event) {
        return event.at("kind").string() == "setup";
    });
    if (setupEvent == events.end()) {
        throw std::runtime_error("BTC replay has no setup event");
    }
    const udon::BtcAdapterOptions adapterOptions{effective_response_budget_ms(options)};
    const udon::MatchConfig config = udon::parse_btc_setup(setupEvent->at("body"), adapterOptions);
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    udon::MatchLedger ledger;
    std::optional<udon::DayState> currentState;
    std::optional<udon::SimulationResult> previousSimulation;
    std::optional<udon::SimulationResult> pendingSimulation;
    std::int32_t simulatedDays = 0;
    std::int32_t reconciledTransitions = 0;

    for (const udon::JsonValue& event : events) {
        const std::string& kind = event.at("kind").string();
        if (kind == "day_state") {
            if (pendingSimulation.has_value()) {
                std::cout << "unacknowledged_action_before_next_state=1\n";
                pendingSimulation.reset();
            }
            const std::int64_t atUnixMs = event.at("atUnixMs").integer();
            const udon::DayState nextState = udon::parse_btc_day_state(
                config,
                event.at("body"),
                std::chrono::system_clock::time_point{std::chrono::milliseconds{atUnixMs}},
                adapterOptions);
            if (previousSimulation.has_value()) {
                bool reconciled = previousSimulation->finalAgents.size() == nextState.agents.size();
                std::vector<std::string> transitionMismatches;
                if (reconciled) {
                    for (std::size_t agentIndex = 0; agentIndex < nextState.agents.size(); ++agentIndex) {
                        const udon::AgentState& expected = previousSimulation->finalAgents.at(agentIndex);
                        const udon::AgentState& actual = nextState.agents.at(agentIndex);
                        if (expected.kind != actual.kind || expected.position != actual.position || expected.fuel != actual.fuel) {
                            reconciled = false;
                            transitionMismatches.push_back(
                                "a" + std::to_string(agentIndex) +
                                " expected=" + std::to_string(expected.position) + "/" + std::to_string(expected.fuel) +
                                " actual=" + std::to_string(actual.position) + "/" + std::to_string(actual.fuel));
                        }
                    }
                }
                std::cout << "transition_to_day=" << nextState.dayNumber
                          << " reconciled=" << (reconciled ? 1 : 0) << '\n';
                for (const std::string& transitionMismatch : transitionMismatches) {
                    std::cout << "  " << transitionMismatch << '\n';
                }
                if (reconciled) {
                    ++reconciledTransitions;
                }
                previousSimulation.reset();
            }
            currentState = nextState;
            continue;
        }
        if (kind == "action_result" || kind == "action_result_recovery") {
            if (!pendingSimulation.has_value() || !currentState.has_value()) {
                continue;
            }
            const bool accepted = udon::btc_action_result_accepted(event.at("body"));
            const std::optional<std::int32_t> acceptedDay =
                udon::btc_action_result_day(event.at("body"));
            const bool currentDay = !acceptedDay.has_value() ||
                *acceptedDay == currentState->dayNumber;
            if (accepted && currentDay) {
                ledger.apply(pendingSimulation->score);
                previousSimulation = *pendingSimulation;
                ++simulatedDays;
            } else {
                std::cout << "action_result_accepted=" << (accepted ? 1 : 0)
                          << " expected_day=" << currentState->dayNumber;
                if (acceptedDay.has_value()) {
                    std::cout << " accepted_day=" << *acceptedDay;
                }
                const std::string reason = udon::btc_action_result_reason(event.at("body"));
                if (!reason.empty()) {
                    std::cout << " reason=" << reason;
                }
                std::cout << '\n';
                previousSimulation.reset();
            }
            pendingSimulation.reset();
            continue;
        }
        if (kind != "actions" && kind != "actions_fallback" &&
            kind != "actions_recovery_wait" && kind != "actions_server_wait") {
            continue;
        }
        if (!currentState.has_value()) {
            throw std::runtime_error("BTC replay contains actions before day state");
        }
        const udon::DayPlan plan = udon::parse_day_plan(config, event.at("body"));
        const udon::SimulationResult simulation = simulator.simulate(*currentState, plan, true);
        const udon::SimulationResult validation = validator.validate(*currentState, plan, true);
        std::string mismatch;
        const bool agrees = validator.agrees_with(simulation, validation, mismatch);
        std::int32_t moves = 0;
        std::int32_t waits = 0;
        std::vector<std::int32_t> servedByAgent(static_cast<std::size_t>(config.agent_count()), 0);
        std::vector<std::int32_t> servedBySpot(config.spots.size(), 0);
        for (const udon::ClaimEvent& claim : simulation.claims) {
            if (claim.served) {
                ++servedByAgent.at(static_cast<std::size_t>(claim.agent));
                ++servedBySpot.at(static_cast<std::size_t>(claim.spot));
            }
        }
        for (const udon::AgentPlan& agentPlan : plan.actions) {
            for (const udon::PlanAction& action : agentPlan) {
                if (action.kind == udon::ActionKind::Move) {
                    ++moves;
                } else {
                    waits += action.value;
                }
            }
        }
        std::cout << "day=" << currentState->dayNumber
                  << " valid=" << (simulation.valid ? 1 : 0)
                  << " validator_agrees=" << (agrees ? 1 : 0)
                  << " daily_distinct=" << simulation.score.dailyDistinct
                  << " servings=" << simulation.score.servings
                  << " moves=" << moves
                  << " wait_steps=" << waits;
        if (!simulation.valid && simulation.error.has_value()) {
            std::cout << " error=" << simulation.error->message;
        } else if (!agrees) {
            std::cout << " mismatch=" << mismatch;
        }
        std::cout << '\n';
        std::cout << "  served_by_agent=";
        for (std::size_t agentIndex = 0; agentIndex < servedByAgent.size(); ++agentIndex) {
            std::cout << (agentIndex == 0 ? "" : ",") << servedByAgent.at(agentIndex);
        }
        std::cout << " served_by_spot=";
        bool firstServedSpot = true;
        for (std::size_t spotIndex = 0; spotIndex < servedBySpot.size(); ++spotIndex) {
            if (servedBySpot.at(spotIndex) == 0) {
                continue;
            }
            std::cout << (firstServedSpot ? "" : ",")
                      << config.spots.at(spotIndex).position << ':' << servedBySpot.at(spotIndex);
            firstServedSpot = false;
        }
        std::cout << '\n';
        std::cout << "  starts=";
        for (std::size_t agentIndex = 0; agentIndex < currentState->agents.size(); ++agentIndex) {
            const udon::AgentState& agent = currentState->agents.at(agentIndex);
            const udon::PlanAction& firstAction = plan.actions.at(agentIndex).front();
            std::cout << (agentIndex == 0 ? "" : ",")
                      << agentIndex << '@' << agent.position << '/' << agent.fuel
                      << ':' << (firstAction.kind == udon::ActionKind::Move ? 'M' : 'W')
                      << firstAction.value;
        }
        std::cout << '\n';
        std::cout << "  claims=";
        bool firstClaim = true;
        for (const udon::ClaimEvent& claim : simulation.claims) {
            const udon::Spot& spot = config.spots.at(static_cast<std::size_t>(claim.spot));
            std::cout << (firstClaim ? "" : ",")
                      << "a" << claim.agent
                      << "@p" << spot.position
                      << "/b" << spot.brandIndex
                      << "/t" << claim.step
                      << '/' << (claim.served ? 'Y' : 'N');
            firstClaim = false;
        }
        std::cout << '\n';
        std::cout << "  refuels=";
        bool firstRefuel = true;
        for (std::int32_t step = 1; step < simulation.trace.stepCount; ++step) {
            for (udon::AgentIndex agentIndex = 0; agentIndex < config.agent_count(); ++agentIndex) {
                if (currentState->agents.at(static_cast<std::size_t>(agentIndex)).kind != udon::AgentKind::Patrol ||
                    simulation.trace.fuel_at(step, agentIndex) <=
                        simulation.trace.fuel_at(step - 1, agentIndex)) {
                    continue;
                }
                std::cout << (firstRefuel ? "" : ",")
                          << "a" << agentIndex
                          << "@p" << simulation.trace.position_at(step, agentIndex)
                          << "/t" << step;
                firstRefuel = false;
            }
        }
        std::cout << '\n';
        if (!simulation.valid || !agrees) {
            throw std::runtime_error("BTC replay failed exact local validation");
        }
        if (kind == "actions_server_wait") {
            ledger.apply(simulation.score);
            previousSimulation = simulation;
            ++simulatedDays;
            pendingSimulation.reset();
            std::cout << "server_wait_applied=1\n";
        } else {
            pendingSimulation = simulation;
        }
    }
    std::cout << "summary days=" << simulatedDays
              << " reconciled_transitions=" << reconciledTransitions
              << " lifetime_distinct=" << ledger.lifetime_distinct()
              << " daily_distinct=" << ledger.totalDailyDistinct
              << " servings=" << ledger.totalServings << '\n';
    const ReplayResumeState resume = load_replay_resume(
        options.replayPath,
        config,
        adapterOptions);
    std::cout << "resume accepted_days=" << resume.acceptedTransitions.size()
              << " last_wire_day=" << resume.lastAcceptedWireDay
              << " cached_contingencies=" << resume.cachedContingencies.size()
              << " strong_proofs=" << resume.strongProofs.size() << '\n';
}

void run_replay_solve(const RuntimeOptions& options) {
    std::ifstream input(options.replayPath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open BTC replay: " + options.replayPath);
    }
    std::vector<udon::JsonValue> events;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            events.push_back(udon::JsonValue::parse(line));
        }
    }
    const auto setupEvent = std::find_if(events.begin(), events.end(), [](const udon::JsonValue& event) {
        return event.at("kind").string() == "setup";
    });
    if (setupEvent == events.end()) {
        throw std::runtime_error("BTC replay has no setup event");
    }
    const udon::BtcAdapterOptions adapterOptions{effective_response_budget_ms(options)};
    const udon::MatchConfig config = udon::parse_btc_setup(setupEvent->at("body"), adapterOptions);
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    udon::MatchLedger ledger;
    std::optional<udon::DayState> currentState;
    std::optional<udon::DayState> targetState;
    std::optional<udon::SimulationResult> pendingSimulation;
    std::int32_t lastAcceptedWireDay = -1;
    std::int32_t pendingWireDay = -1;
    for (const udon::JsonValue& event : events) {
        const std::string& kind = event.at("kind").string();
        if (kind == "day_state") {
            const std::int64_t atUnixMs = event.at("atUnixMs").integer();
            currentState = udon::parse_btc_day_state(
                config,
                event.at("body"),
                std::chrono::system_clock::time_point{std::chrono::milliseconds{atUnixMs}},
                adapterOptions);
            if (currentState->dayNumber == options.dayNumber) {
                targetState = currentState;
                break;
            }
            pendingWireDay = currentState->dayNumber - 1;
            pendingSimulation.reset();
            continue;
        }
        if (kind == "actions" || kind == "actions_fallback" || kind == "actions_recovery_wait") {
            if (!currentState.has_value()) {
                throw std::runtime_error("BTC replay solve found actions without a day state");
            }
            const udon::DayPlan plan = udon::parse_day_plan(config, event.at("body"));
            const udon::SimulationResult simulation = simulator.simulate(*currentState, plan, false);
            const udon::SimulationResult validation = validator.validate(*currentState, plan, false);
            std::string mismatch;
            if (!simulation.valid || !validator.agrees_with(simulation, validation, mismatch)) {
                throw std::runtime_error("BTC replay solve rejected a recorded plan: " + mismatch);
            }
            pendingSimulation = simulation;
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
    if (!targetState.has_value()) {
        throw std::runtime_error("requested day is absent from BTC replay");
    }
    udon::UdonShieldEngine engine(
        config,
        {},
        btc_http_deadline_calibration(),
        udon::RoutePoolSearch::SinglePass,
        options.harvestExtensionMode);
    const auto started = std::chrono::steady_clock::now();
    const udon::DecisionResult decision = engine.solve_day(
        *targetState,
        ledger,
        std::chrono::milliseconds{effective_response_budget_ms(options)});
    const std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started);
    const udon::SimulationResult simulation = simulator.simulate(*targetState, decision.candidate.plan, false);
    const udon::SimulationResult validation = validator.validate(*targetState, decision.candidate.plan, false);
    std::string mismatch;
    if (!simulation.valid || !validator.agrees_with(simulation, validation, mismatch)) {
        throw std::runtime_error("offline BTC decision failed exact validation: " + mismatch);
    }
    std::int32_t bestPoolDailyDistinct = 0;
    std::int32_t certifiedCandidates = 0;
    for (const udon::CandidateAuditRecord& candidate : decision.audit.candidates) {
        bestPoolDailyDistinct = std::max(
            bestPoolDailyDistinct,
            candidate.scoreAfterToday.totalDailyDistinct - ledger.totalDailyDistinct);
        certifiedCandidates += candidate.certified ? 1 : 0;
    }
    const char* deadlineClass = "emergency";
    switch (decision.deadline.deadlineClass) {
    case udon::DeadlineClass::Emergency:
        break;
    case udon::DeadlineClass::Short:
        deadlineClass = "short";
        break;
    case udon::DeadlineClass::Normal:
        deadlineClass = "normal";
        break;
    case udon::DeadlineClass::Long:
        deadlineClass = "long";
        break;
    }
    const auto compactIntegers = [](const std::vector<std::int32_t>& values) {
        std::string result;
        for (const std::int32_t value : values) {
            if (!result.empty()) {
                result.push_back(',');
            }
            result += std::to_string(value);
        }
        return result;
    };
    std::cout << "day=" << options.dayNumber
              << " budget_ms=" << effective_response_budget_ms(options)
              << " elapsed_ms=" << elapsed.count()
              << " class=" << deadlineClass
              << " daily_distinct=" << simulation.score.dailyDistinct
              << " servings=" << simulation.score.servings
              << " lifetime=" << decision.candidate.scoreAfterToday.lifetimeDistinct
              << " daily_total=" << decision.candidate.scoreAfterToday.totalDailyDistinct
              << " serving_total=" << decision.candidate.scoreAfterToday.totalServings
              << " independent_ms=" << decision.timing.independentGenerators.count()
              << " search_ms=" << decision.timing.search.count()
              << " candidate_ms=" << decision.timing.candidatePreparation.count()
              << " certification_ms=" << decision.timing.certification.count()
              << " exact_ms="
              << decision.audit.columnGeneration.exactOrienteeringMilliseconds
              << " exact_enumeration_ms="
              << decision.audit.columnGeneration
                     .exactOrienteeringEnumerationMilliseconds
              << " exact_finalization_ms="
              << decision.audit.columnGeneration
                     .exactOrienteeringFinalizationMilliseconds
              << " exact_deadline_start_ms="
              << decision.audit.columnGeneration
                     .exactOrienteeringDeadlineRemainingAtStartMilliseconds
              << " exact_deadline_overrun_ms="
              << decision.audit.columnGeneration
                     .exactOrienteeringDeadlineOverrunMilliseconds
              << " exact_supported="
              << decision.audit.columnGeneration.exactOrienteeringSupportedAgents
              << " exact_complete="
              << decision.audit.columnGeneration.exactOrienteeringCompleteAgents
              << " exact_terminal_variants="
              << decision.audit.columnGeneration.exactOrienteeringTerminalVariants
              << " exact_bundles="
              << decision.audit.columnGeneration.exactOrienteeringBundles
              << " exact_seed_servings="
              << decision.audit.columnGeneration.exactOrienteeringSeedServings
              << " exact_local_servings="
              << decision.audit.columnGeneration.exactOrienteeringLocalServings
              << " exact_feasibility_nodes="
              << decision.audit.columnGeneration
                     .exactOrienteeringFeasibilityNodes
              << " column_deadline="
              << (decision.audit.columnGeneration.deadlineReached ? 1 : 0)
              << " pool=" << decision.audit.candidates.size()
              << " pool_best_daily=" << bestPoolDailyDistinct
              << " certified=" << certifiedCandidates
              << " columns=" << compactIntegers(decision.audit.portfolioColumnsByAgent)
              << " brand_frontier=" << compactIntegers(decision.audit.portfolioBrandCountsByAgent)
              << " combinations=" << decision.diagnostics.combinationsVisited
              << " beam_combinations=" << decision.diagnostics.beamCombinationsVisited
              << " dfs_combinations=" << decision.diagnostics.depthFirstCombinationsVisited
              << " branch_orders=" << decision.diagnostics.branchOrderingCalls
              << " bound_checks=" << decision.diagnostics.upperBoundChecks
              << " bound_prunes=" << decision.diagnostics.upperBoundPrunes
              << " bundle_bound_checks="
              << decision.diagnostics.bundleUpperBoundChecks
              << " bundle_bound_prunes="
              << decision.diagnostics.bundleUpperBoundPrunes
              << " bundle_brand_states="
              << decision.diagnostics.bundleBrandFrontierStates
              << " bundle_brand_fallbacks="
              << decision.diagnostics.bundleBrandFrontierFallbacks
              << " master_upper="
              << decision.diagnostics.optimisticUpperBound.lifetimeDistinct
              << '/'
              << decision.diagnostics.optimisticUpperBound.totalDailyDistinct
              << '/'
              << decision.diagnostics.optimisticUpperBound.totalServings
              << " guidance_upper="
              << decision.diagnostics.searchGuidanceUpperBound.lifetimeDistinct
              << '/'
              << decision.diagnostics.searchGuidanceUpperBound.totalDailyDistinct
              << '/'
              << decision.diagnostics.searchGuidanceUpperBound.totalServings
              << " bundle_prunes=" << decision.diagnostics.bundlePrunes
              << " partial_checks=" << decision.diagnostics.partialSynchronizationChecks
              << " partial_prunes=" << decision.diagnostics.partialSynchronizationPrunes
              << " master_prepare_us=" << decision.diagnostics.roundPreparationMicroseconds
              << " master_beam_build_us=" << decision.diagnostics.beamConstructionMicroseconds
              << " master_beam_eval_us=" << decision.diagnostics.beamEvaluationMicroseconds
              << " master_dfs_us=" << decision.diagnostics.depthFirstSearchMicroseconds
              << " master_population_us=" << decision.diagnostics.populationMaintenanceMicroseconds
              << " alns_iterations=" << decision.alns.iterations
              << " alns_improvements=" << decision.alns.improvements
              << " recombination_improvements=" << decision.alns.recombinationImprovements
              << " master_deadline=" << (decision.diagnostics.deadlineReached ? 1 : 0)
              << " emergency=" << (decision.emergency ? 1 : 0)
              << " selection=" << decision.audit.selectionReason
              << '\n';
    std::cout << udon::serialize_day_plan(decision.candidate.plan).dump() << '\n';
}

void run_replay_roles(const RuntimeOptions& options) {
    std::ifstream input(options.replayPath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open BTC replay: " + options.replayPath);
    }
    std::optional<udon::JsonValue> setupDocument;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const udon::JsonValue event = udon::JsonValue::parse(line);
        if (event.at("kind").string() == "setup") {
            setupDocument = event.at("body");
            break;
        }
    }
    if (!setupDocument.has_value()) {
        throw std::runtime_error("BTC replay has no setup event");
    }
    const udon::BtcAdapterOptions adapterOptions{effective_response_budget_ms(options)};
    const udon::MatchConfig config = udon::parse_btc_setup(*setupDocument, adapterOptions);
    udon::MatchSession session(
        config,
        {},
        {},
        options.harvestExtensionMode,
        resolved_future_harvest_extension_mode(options));
    session.set_short_horizon_role_fallback(options.shortHorizonRoleFallback);
    const std::chrono::steady_clock::time_point started =
        std::chrono::steady_clock::now();
    const udon::RoleSelectionDiagnostics roleDiagnostics =
        session.select_roles_until_with_diagnostics(
        std::chrono::milliseconds{effective_response_budget_ms(options)},
        options.beamWidth);
    const std::vector<udon::RoleAssignment>& assignments =
        roleDiagnostics.assignments;
    const std::chrono::milliseconds elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started);
    for (std::size_t index = 0; index < assignments.size(); ++index) {
        const udon::RoleAssignment& assignment = assignments.at(index);
        std::string roles;
        roles.reserve(assignment.roles.size());
        for (const udon::AgentKind role : assignment.roles) {
            roles.push_back(role == udon::AgentKind::Patrol ? 'P' : 'T');
        }
        std::string trace;
        for (const std::int32_t daily :
             roleDiagnostics.rolloutDailyDistinct.at(index)) {
            if (!trace.empty()) {
                trace.push_back(',');
            }
            trace += std::to_string(daily);
        }
        std::cout << "rank=" << index + 1U
                  << " roles=" << roles
                  << " patrols=" << assignment.patrolCount
                  << " sustainable=" << assignment.sustainableCoverage
                  << " rollout_valid=" << (assignment.rolloutValid ? 1 : 0)
                  << " rollout_complete=" << (assignment.rolloutComplete ? 1 : 0)
                  << " rollout=" << assignment.rolloutScore.lifetimeDistinct
                  << '/' << assignment.rolloutScore.totalDailyDistinct
                  << '/' << assignment.rolloutScore.totalServings
                  << " upper=" << assignment.cheapUpperBound.lifetimeDistinct
                  << '/' << assignment.cheapUpperBound.totalDailyDistinct
                  << '/' << assignment.cheapUpperBound.totalServings
                  << " trace=" << trace
                  << '\n';
    }
    std::cout << "elapsed_ms=" << elapsed.count() << '\n';
}

void run_replay_counterfactual(const RuntimeOptions& options) {
    std::ifstream input(options.replayPath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open BTC replay: " + options.replayPath);
    }
    std::vector<udon::JsonValue> events;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            events.push_back(udon::JsonValue::parse(line));
        }
    }
    const auto setupEvent = std::find_if(
        events.begin(),
        events.end(),
        [](const udon::JsonValue& event) {
            return event.at("kind").string() == "setup";
        });
    if (setupEvent == events.end()) {
        throw std::runtime_error("BTC replay has no setup event");
    }
    const udon::BtcAdapterOptions adapterOptions{effective_response_budget_ms(options)};
    const udon::MatchConfig config =
        udon::parse_btc_setup(setupEvent->at("body"), adapterOptions);
    if (options.roleMask >= (std::int32_t{1} << config.agent_count())) {
        throw std::invalid_argument("--role-mask has bits outside the configured agents");
    }
    if (std::popcount(static_cast<std::uint32_t>(options.roleMask)) >=
        config.agent_count()) {
        throw std::invalid_argument("--role-mask must leave at least one patrol");
    }
    udon::ExactStepSimulator simulator(config);
    udon::IndependentDayValidator validator(config);
    udon::DeadlineCalibration deadlineCalibration =
        btc_http_deadline_calibration();
    const std::int32_t solveBudgetMs = static_cast<std::int32_t>(
        udon::competition_compute_budget(std::chrono::milliseconds{
            options.logicBudgetMs > 0
                ? options.logicBudgetMs
                : options.responseBudgetMs}).count());
    if (options.logicBudgetMs > 0) {
        deadlineCalibration.normalThreshold =
            std::chrono::milliseconds{solveBudgetMs};
        deadlineCalibration.version += "-logic-normal";
    }
    udon::UdonShieldEngine engine(
        config,
        {},
        deadlineCalibration,
        udon::RoutePoolSearch::SinglePass,
        options.harvestExtensionMode,
        options.requireUndominatedCurrentFloor,
        resolved_future_harvest_extension_mode(options));
    engine.set_short_horizon_role_fallback(options.shortHorizonRoleFallback);
    static_cast<void>(engine.select_roles_until(
        std::chrono::milliseconds{solveBudgetMs},
        options.beamWidth));
    udon::MatchLedger ledger;
    std::vector<udon::AgentState> counterfactualAgents;
    std::ofstream decisionDump;
    if (!options.decisionDumpPath.empty()) {
        const std::filesystem::path dumpPath(options.decisionDumpPath);
        if (dumpPath.has_parent_path()) {
            std::filesystem::create_directories(dumpPath.parent_path());
        }
        decisionDump.open(dumpPath, std::ios::binary | std::ios::trunc);
        if (!decisionDump) {
            throw std::runtime_error(
                "cannot open counterfactual decision dump: " +
                options.decisionDumpPath);
        }
    }
    std::int32_t expectedDay = 1;
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
        if (counterfactualAgents.empty()) {
            for (udon::AgentIndex agentIndex = 0;
                 agentIndex < config.agent_count();
                 ++agentIndex) {
                const bool tanker =
                    (options.roleMask &
                     (std::int32_t{1} << agentIndex)) != 0;
                state.agents.at(static_cast<std::size_t>(agentIndex)).kind =
                    tanker ? udon::AgentKind::Tanker : udon::AgentKind::Patrol;
            }
        } else {
            state.agents = counterfactualAgents;
        }
        if (options.postAckMs > 0) {
            for (const udon::ResponseLedger::CachedContingency& contingency :
                 engine.response_ledger().cachedContingencies) {
                if (contingency.dayNumber != state.dayNumber) {
                    continue;
                }
                const udon::SimulationResult preview =
                    simulator.simulate(state, contingency.plan, false);
                std::cout << "cached_contingency day=" << state.dayNumber
                          << " scenario=" << contingency.scenarioId
                          << " valid=" << (preview.valid ? 1 : 0)
                          << " daily=" << preview.score.dailyDistinct
                          << " servings=" << preview.score.servings
                          << '\n';
            }
        }
        const std::chrono::steady_clock::time_point started =
            std::chrono::steady_clock::now();
        const udon::DecisionResult decision = engine.solve_day(
            state,
            ledger,
            std::chrono::milliseconds{solveBudgetMs});
        const std::chrono::milliseconds elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - started);
        const udon::SimulationResult simulation =
            simulator.simulate(state, decision.candidate.plan, false);
        const udon::SimulationResult validation =
            validator.validate(state, decision.candidate.plan, false);
        std::string mismatch;
        if (!simulation.valid ||
            !validator.agrees_with(simulation, validation, mismatch)) {
            throw std::runtime_error(
                "counterfactual plan failed exact validation: " + mismatch);
        }
        if (decisionDump) {
            decisionDump << udon::serialize_decision_replay(
                config,
                state,
                ledger,
                decision).dump() << '\n';
        }
        const udon::MatchLedger ledgerBeforeApply = ledger;
        ledger.apply(simulation.score);
        counterfactualAgents = simulation.finalAgents;
        engine.record_submitted(decision, elapsed);
        std::int32_t postAckContingencies = 0;
        if (options.postAckMs > 0) {
            postAckContingencies = engine.precompute_next_day_contingencies(
                state,
                ledgerBeforeApply,
                decision,
                std::chrono::steady_clock::now() +
                    std::chrono::milliseconds{options.postAckMs});
            // record_submitted above already records the accepted transition.
            // Keep its post-ACK response artifacts alive for the next replay
            // day, matching MatchSession::acknowledge_submitted in HTTP mode.
        }
        const auto compact_score = [](const udon::OfficialScore& score) {
            return std::to_string(score.lifetimeDistinct) + '/' +
                std::to_string(score.totalDailyDistinct) + '/' +
                std::to_string(score.totalServings);
        };
        std::string terminals;
        for (udon::AgentIndex agentIndex = 0;
             agentIndex < config.agent_count();
             ++agentIndex) {
            if (!terminals.empty()) {
                terminals.push_back(',');
            }
            const udon::AgentState& finalAgent =
                simulation.finalAgents.at(static_cast<std::size_t>(agentIndex));
            terminals += 'a' + std::to_string(agentIndex) + '@' +
                std::to_string(finalAgent.position) + '/' +
                std::to_string(finalAgent.fuel);
        }
        std::cout << "day=" << state.dayNumber
                  << " score=" << ledger.lifetime_distinct()
                  << '/' << ledger.totalDailyDistinct
                  << '/' << ledger.totalServings
                  << " daily=" << simulation.score.dailyDistinct
                  << '/' << simulation.score.servings
                  << " profile_lb=" << compact_score(decision.profile.provisionalLowerBound)
                  << " profile_cert=" << compact_score(decision.profile.certifiedLowerBound)
                  << " profile_q50=" << compact_score(decision.profile.quantiles.at(2))
                  << " candidates=" << decision.audit.candidates.size()
                  << " candidate_ms=" << decision.timing.candidatePreparation.count()
                  << " certification_ms=" << decision.timing.certification.count()
                  << " terminals=" << terminals
                  << " post_ack_contingencies=" << postAckContingencies
                  << " elapsed_ms=" << elapsed.count()
                  << '\n';
        ++expectedDay;
        if (expectedDay > options.maximumReplayDays) {
            break;
        }
    }
    std::cout << "summary role_mask=" << options.roleMask
              << " days=" << expectedDay - 1
              << " score=" << ledger.lifetime_distinct()
              << '/' << ledger.totalDailyDistinct
              << '/' << ledger.totalServings
              << '\n';
}

void run_sandbox(const RuntimeOptions& options) {
    ReplayWriter replay(options.replayPath);
    const udon::BtcAdapterOptions adapterOptions{effective_response_budget_ms(options)};
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw std::runtime_error("sandbox ended before setup");
    }
    const udon::JsonValue setupDocument = udon::JsonValue::parse(line);
    if (udon::classify_btc_frame(setupDocument) != udon::BtcFrameKind::Setup) {
        throw std::runtime_error("first sandbox frame is not BTC setup");
    }
    replay.record("setup", setupDocument);
    const udon::MatchConfig config = udon::parse_btc_setup(setupDocument, adapterOptions);
    udon::MatchSession session(
        config,
        {},
        {},
        options.harvestExtensionMode,
        resolved_future_harvest_extension_mode(options));
    // Short (<=5-day) matches keep the single-tanker fallback whenever the
    // incomplete-rollout comparison would seat a tanker-heavy composition.
    session.set_short_horizon_role_fallback(true);
    const std::vector<udon::RoleAssignment> assignments = session.select_roles_until(
        std::chrono::milliseconds{effective_response_budget_ms(options)},
        options.beamWidth);
    if (assignments.empty()) {
        throw std::runtime_error("no role assignment survived BTC viability scan");
    }
    const udon::JsonValue roles = udon::serialize_role_selection(assignments.front().roles);
    replay.record("assignment", roles);
    emit_wire(roles);

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    udon::MatchLedger ledger;
    std::optional<udon::DayState> pendingState;
    std::optional<udon::SimulationResult> pendingSimulation;
    std::optional<std::int32_t> pendingDay;
    std::chrono::milliseconds pendingResponse{};
    bool pendingDecision = false;
    bool fallbackPending = false;

    const auto acknowledge_pending = [&]() {
        if (!pendingSimulation.has_value() || !pendingState.has_value()) {
            return;
        }
        if (pendingDecision) {
            static_cast<void>(session.acknowledge_submitted(pendingResponse));
        } else {
            session.record_applied_transition(*pendingState, *pendingSimulation);
        }
        ledger.apply(pendingSimulation->score);
        pendingState.reset();
        pendingSimulation.reset();
        pendingDay.reset();
        pendingDecision = false;
    };

    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }
        const udon::JsonValue frame = udon::JsonValue::parse(line);
        const udon::BtcFrameKind kind = udon::classify_btc_frame(frame);
        if (kind == udon::BtcFrameKind::DayState) {
            acknowledge_pending();
            fallbackPending = false;
            const auto receivedAt = std::chrono::system_clock::now();
            const auto responseStarted = std::chrono::steady_clock::now();
            replay.record("day_state", frame);
            const udon::DayState state = udon::parse_btc_day_state(
                config,
                frame,
                receivedAt,
                adapterOptions);
            const udon::SessionDecision decision = session.on_authoritative_state(state, ledger, receivedAt);
            replay.record("decision", decision.replay);
            const udon::DayPlan plan = decision.maySubmit
                ? decision.decision.candidate.plan
                : udon::make_wait_plan(config, state.dayNumber);
            const udon::SimulationResult simulation = validate_fallback_plan(
                state,
                plan,
                simulator,
                validator,
                "BTC sandbox plan failed local validation");
            const udon::JsonValue wirePlan = udon::serialize_day_plan(plan);
            replay.record(decision.maySubmit ? "actions" : "actions_fallback", wirePlan);
            emit_wire(wirePlan);
            pendingResponse = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - responseStarted);
            pendingState = state;
            pendingSimulation = simulation;
            pendingDay = state.dayNumber;
            pendingDecision = decision.maySubmit;
            continue;
        }
        if (kind == udon::BtcFrameKind::ActionResult) {
            replay.record("action_result", frame);
            if (udon::btc_action_result_accepted(frame)) {
                const std::optional<std::int32_t> acceptedDay =
                    udon::btc_action_result_day(frame);
                if (acceptedDay.has_value() && pendingDay.has_value() &&
                    *acceptedDay != *pendingDay) {
                    throw std::runtime_error(
                        "BTC sandbox accepted an action for a stale day");
                }
                acknowledge_pending();
                fallbackPending = false;
                continue;
            }
            if (fallbackPending) {
                throw std::runtime_error("BTC rejected the exact WAIT fallback: " + udon::btc_action_result_reason(frame));
            }
            if (!pendingDay.has_value()) {
                throw std::runtime_error("BTC rejected an action without a pending day");
            }
            if (session.has_pending_submission()) {
                session.reject_pending_submission();
            }
            if (!pendingState.has_value()) {
                throw std::runtime_error("BTC rejected an action without a pending state");
            }
            const udon::DayPlan waitPlan = udon::make_wait_plan(config, *pendingDay);
            pendingSimulation = validate_fallback_plan(
                *pendingState,
                waitPlan,
                simulator,
                validator,
                "BTC sandbox recovery WAIT failed local validation");
            pendingDecision = false;
            const udon::JsonValue wireWaitPlan = udon::serialize_day_plan(waitPlan);
            replay.record("actions_recovery_wait", wireWaitPlan);
            emit_wire(wireWaitPlan);
            fallbackPending = true;
            continue;
        }
        if (kind == udon::BtcFrameKind::MatchResult) {
            acknowledge_pending();
            replay.record("result", frame);
            std::cerr << "BTC result: " << frame.dump() << '\n';
            return;
        }
        throw std::runtime_error("sandbox supplied an unsupported BTC frame");
    }
    if (session.has_pending_submission()) {
        acknowledge_pending();
    }
}

#ifdef _WIN32

[[nodiscard]] std::wstring utf8_to_wide(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0);
    if (required <= 0) {
        throw std::runtime_error("invalid UTF-8 in HTTP argument");
    }
    std::wstring result(static_cast<std::size_t>(required), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), required) != required) {
        throw std::runtime_error("failed to convert HTTP argument to UTF-16");
    }
    return result;
}

class WinHttpHandle {
public:
    explicit WinHttpHandle(HINTERNET handle = nullptr) : handle_(handle) {}
    ~WinHttpHandle() {
        if (handle_ != nullptr) {
            WinHttpCloseHandle(handle_);
        }
    }
    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
    WinHttpHandle(WinHttpHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
    WinHttpHandle& operator=(WinHttpHandle&& other) noexcept {
        if (this != &other) {
            if (handle_ != nullptr) {
                WinHttpCloseHandle(handle_);
            }
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }
    [[nodiscard]] HINTERNET get() const { return handle_; }
private:
    HINTERNET handle_ = nullptr;
};

enum class WinHttpRequestStage {
    Send,
    Receive,
    QueryBody,
    ReadBody,
};

class WinHttpRequestError final : public std::runtime_error {
public:
    WinHttpRequestError(WinHttpRequestStage stage, DWORD code)
        : std::runtime_error(
              "BTC HTTP request failed during " + stage_name(stage) +
              " with WinHTTP error " + std::to_string(code)),
          code_(code) {}

    [[nodiscard]] bool retryable_resend() const noexcept {
        return code_ == ERROR_WINHTTP_TIMEOUT ||
            code_ == ERROR_WINHTTP_RESEND_REQUEST;
    }

private:
    [[nodiscard]] static std::string stage_name(WinHttpRequestStage stage) {
        switch (stage) {
        case WinHttpRequestStage::Send:
            return "send";
        case WinHttpRequestStage::Receive:
            return "receive";
        case WinHttpRequestStage::QueryBody:
            return "response-body query";
        case WinHttpRequestStage::ReadBody:
            return "response-body read";
        }
        return "unknown stage";
    }

    DWORD code_;
};

class WinHttpClient {
public:
    WinHttpClient(const std::string& baseUrl, const std::string& bearerToken) {
        const std::wstring wideUrl = utf8_to_wide(baseUrl);
        URL_COMPONENTS components{};
        components.dwStructSize = sizeof(components);
        components.dwSchemeLength = static_cast<DWORD>(-1);
        components.dwHostNameLength = static_cast<DWORD>(-1);
        components.dwUrlPathLength = static_cast<DWORD>(-1);
        if (!WinHttpCrackUrl(wideUrl.c_str(), static_cast<DWORD>(wideUrl.size()), 0, &components)) {
            throw std::runtime_error("invalid BTC base URL");
        }
        secure_ = components.nScheme == INTERNET_SCHEME_HTTPS;
        host_.assign(components.lpszHostName, components.dwHostNameLength);
        const bool localHost = host_ == L"localhost" || host_ == L"127.0.0.1" || host_ == L"::1";
        if (!secure_ && !localHost) {
            throw std::runtime_error("refusing to transmit BTC token over non-HTTPS transport");
        }
        port_ = components.nPort;
        basePath_.assign(components.lpszUrlPath, components.dwUrlPathLength);
        while (!basePath_.empty() && basePath_.back() == L'/') {
            basePath_.pop_back();
        }
        token_ = utf8_to_wide(bearerToken);
        session_ = WinHttpHandle(WinHttpOpen(
            L"UDON-SHIELD/1.0",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0));
        if (session_.get() == nullptr) {
            throw std::runtime_error("WinHttpOpen failed");
        }
        if (!WinHttpSetTimeouts(session_.get(), 5000, 5000, 5000, 5000)) {
            throw std::runtime_error("WinHttpSetTimeouts failed");
        }
        connection_ = WinHttpHandle(WinHttpConnect(session_.get(), host_.c_str(), port_, 0));
        if (connection_.get() == nullptr) {
            throw std::runtime_error("WinHttpConnect failed");
        }
    }

    [[nodiscard]] HttpResponse request(
        const std::string& method,
        const std::string& path,
        const std::optional<std::string>& body = std::nullopt,
        std::int32_t ioTimeoutMs = 0) const {
        const std::wstring wideMethod = utf8_to_wide(method);
        const std::wstring widePath = basePath_ + utf8_to_wide(path);
        const DWORD flags = secure_ ? WINHTTP_FLAG_SECURE : 0;
        WinHttpHandle request(WinHttpOpenRequest(
            connection_.get(),
            wideMethod.c_str(),
            widePath.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            flags));
        if (request.get() == nullptr) {
            throw std::runtime_error("WinHttpOpenRequest failed");
        }
        DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        if (!WinHttpSetOption(request.get(), WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy))) {
            throw std::runtime_error("WinHttpSetOption redirect policy failed");
        }
        if (ioTimeoutMs > 0 &&
            !WinHttpSetTimeouts(
                request.get(),
                5000,
                5000,
                ioTimeoutMs,
                ioTimeoutMs)) {
            throw std::runtime_error("WinHttpSetTimeouts request override failed");
        }
        const std::wstring headers = L"Authorization: Bearer " + token_ +
            L"\r\nContent-Type: application/json\r\nAccept: application/json\r\n";
        const std::string emptyBody;
        const std::string& payload = body.has_value() ? *body : emptyBody;
        if (!WinHttpSendRequest(
                request.get(),
                headers.c_str(),
                static_cast<DWORD>(headers.size()),
                payload.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(payload.data()),
                static_cast<DWORD>(payload.size()),
                static_cast<DWORD>(payload.size()),
                0)) {
            throw WinHttpRequestError(WinHttpRequestStage::Send, GetLastError());
        }
        if (!WinHttpReceiveResponse(request.get(), nullptr)) {
            throw WinHttpRequestError(WinHttpRequestStage::Receive, GetLastError());
        }
        DWORD status = 0;
        DWORD statusSize = sizeof(status);
        if (!WinHttpQueryHeaders(
                request.get(),
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &status,
                &statusSize,
                WINHTTP_NO_HEADER_INDEX)) {
            throw std::runtime_error("cannot read BTC HTTP status");
        }
        std::string responseBody;
        while (true) {
            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(request.get(), &available)) {
                throw WinHttpRequestError(WinHttpRequestStage::QueryBody, GetLastError());
            }
            if (available == 0) {
                break;
            }
            const std::size_t offset = responseBody.size();
            responseBody.resize(offset + available);
            DWORD read = 0;
            if (!WinHttpReadData(request.get(), responseBody.data() + offset, available, &read)) {
                throw WinHttpRequestError(WinHttpRequestStage::ReadBody, GetLastError());
            }
            responseBody.resize(offset + read);
        }
        return HttpResponse{static_cast<std::int32_t>(status), std::move(responseBody)};
    }

private:
    WinHttpHandle session_;
    WinHttpHandle connection_;
    std::wstring host_;
    std::wstring basePath_;
    std::wstring token_;
    INTERNET_PORT port_ = 0;
    bool secure_ = false;
};

[[nodiscard]] bool transient_http_status(std::int32_t status) {
    return status == 425 || status == 429 || status >= 500;
}

[[nodiscard]] HttpResponse post_until_deadline(
    const WinHttpClient& client,
    const std::string& path,
    const std::string& body,
    std::int32_t pollMs,
    std::int32_t actionAckTimeoutMs,
    std::int64_t deadlineUnixMs) {
    std::int32_t transportRetries = 0;
    while (true) {
        const std::int64_t remainingMs = deadlineUnixMs - unix_milliseconds();
        if (remainingMs <= 0) {
            throw std::runtime_error("BTC action ACK deadline expired before an HTTP attempt");
        }
        const std::int64_t retryReserveMs = remainingMs > pollMs
            ? pollMs
            : 0;
        const std::int32_t requestTimeoutMs = static_cast<std::int32_t>(
            std::max<std::int64_t>(
                1,
                std::min<std::int64_t>(
                    actionAckTimeoutMs,
                    remainingMs - retryReserveMs)));
        HttpResponse response;
        try {
            response = client.request("POST", path, body, requestTimeoutMs);
        } catch (const WinHttpRequestError& error) {
            if (!error.retryable_resend() ||
                unix_milliseconds() + pollMs >= deadlineUnixMs) {
                throw;
            }
            ++transportRetries;
            std::this_thread::sleep_for(std::chrono::milliseconds{pollMs});
            continue;
        }
        response.transportRetries = transportRetries;
        if (!transient_http_status(response.status) || unix_milliseconds() + pollMs >= deadlineUnixMs) {
            return response;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{pollMs});
    }
}

[[nodiscard]] HttpResponse wait_for_get(
    const WinHttpClient& client,
    const std::string& path,
    std::int32_t pollMs) {
    while (true) {
        const HttpResponse response = client.request("GET", path);
        if (response.status == 200) {
            return response;
        }
        if (!transient_http_status(response.status)) {
            throw std::runtime_error("BTC GET " + path + " returned HTTP " + std::to_string(response.status));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{pollMs});
    }
}

[[nodiscard]] bool valid_match_id(const std::string& matchId) {
    return !matchId.empty() && std::all_of(matchId.begin(), matchId.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '-' || character == '_';
    });
}

void run_http(const RuntimeOptions& options) {
    if (!valid_match_id(options.matchId)) {
        throw std::invalid_argument("match ID contains unsupported characters");
    }
    char* tokenBuffer = nullptr;
    std::size_t tokenLength = 0;
    if (_dupenv_s(&tokenBuffer, &tokenLength, "HEXUDON_TOKEN") != 0 ||
        tokenBuffer == nullptr || tokenLength <= 1) {
        std::free(tokenBuffer);
        throw std::runtime_error("HEXUDON_TOKEN is not set");
    }
    const std::string token(tokenBuffer);
    std::free(tokenBuffer);
    const WinHttpClient client(options.baseUrl, token);
    const std::string root = "/api/v1/matches/" + options.matchId;
    ReplayWriter replay(options.replayPath);
    const udon::BtcAdapterOptions adapterOptions{effective_response_budget_ms(options)};

    const HttpResponse setupResponse = wait_for_get(client, root + "/setup", options.pollMs);
    const udon::JsonValue setupDocument = parse_optional_json(setupResponse.body);
    replay.record("setup", setupDocument, setupResponse.status);
    const udon::MatchConfig config = udon::parse_btc_setup(setupDocument, adapterOptions);
    const ReplayResumeState resume = load_replay_resume(options.replayPath, config, adapterOptions);
    const udon::DeadlineCalibration deadlineCalibration = btc_http_deadline_calibration();
    udon::MatchSession session(
        config,
        {},
        deadlineCalibration,
        options.harvestExtensionMode,
        resolved_future_harvest_extension_mode(options));
    // Short (<=5-day) matches keep the single-tanker fallback whenever the
    // incomplete-rollout comparison would seat a tanker-heavy composition.
    session.set_short_horizon_role_fallback(true);
    for (const ReplayResumeState::AcceptedTransition& transition :
         resume.acceptedTransitions) {
        session.record_applied_transition(
            transition.state,
            transition.simulation);
    }
    if (resume.artifactsAfterDay.has_value() &&
        *resume.artifactsAfterDay == resume.lastAcceptedWireDay + 1) {
        session.restore_response_artifacts(
            resume.cachedContingencies,
            resume.strongProofs);
    }
    if (!resume.assignmentAccepted || !resume.assignment.has_value()) {
        const std::chrono::milliseconds roleSelectionBudget{
            effective_response_budget_ms(options)};
        udon::JsonValue roles;
        if (resume.assignment.has_value()) {
            roles = *resume.assignment;
        } else {
            const std::vector<udon::RoleAssignment> assignments = session.select_roles_until(
                roleSelectionBudget,
                options.beamWidth);
            if (assignments.empty()) {
                throw std::runtime_error("no role assignment survived BTC viability scan");
            }
            roles = udon::serialize_role_selection(assignments.front().roles);
            replay.record("assignment", roles);
        }
        HttpResponse assignmentResponse;
        do {
            assignmentResponse = client.request("POST", root + "/assignment", roles.dump());
            if (transient_http_status(assignmentResponse.status)) {
                std::this_thread::sleep_for(std::chrono::milliseconds{options.pollMs});
            }
        } while (transient_http_status(assignmentResponse.status));
        const udon::JsonValue assignmentResult = parse_tolerant_http_body(assignmentResponse.body);
        replay.record("assignment_result", assignmentResult, assignmentResponse.status);
        if (assignmentResponse.status < 200 || assignmentResponse.status >= 300) {
            throw std::runtime_error("BTC assignment returned HTTP " + std::to_string(assignmentResponse.status));
        }
        if (!udon::btc_action_result_accepted(assignmentResult)) {
            throw std::runtime_error("BTC rejected role assignment: " + udon::btc_action_result_reason(assignmentResult));
        }
    }
    static_cast<void>(wait_for_get(client, root + "/start", options.pollMs));

    udon::MatchLedger ledger = resume.ledger;
    udon::MatchLedger checkpointLedger = resume.checkpointLedger;
    udon::MatchLedger virtualLedger = resume.virtualLedger;
    std::optional<std::vector<udon::AgentState>> checkpointAgents =
        resume.checkpointAgents;
    std::optional<std::vector<udon::AgentState>> virtualAgents =
        resume.virtualAgents;
    bool checkpointDivergenceActive = resume.checkpointDivergenceActive;
    bool protectedDivergenceActive = resume.protectedDivergenceActive;
    std::int32_t lastAcceptedWireDay = resume.lastAcceptedWireDay;
    std::string lastSessionCheckpoint;
    const auto record_session_checkpoint = [&](std::int32_t acceptedDay) {
        const udon::JsonValue checkpoint = serialize_session_checkpoint(
            acceptedDay,
            session.response_ledger());
        const std::string checkpointBytes = checkpoint.dump();
        if (checkpointBytes == lastSessionCheckpoint) {
            return;
        }
        replay.record("session_checkpoint", checkpoint);
        lastSessionCheckpoint = checkpointBytes;
    };
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    udon::ProtectedSlackRefiner slackRefiner(config);
    // Accepted SCORE-TERMINAL-PAIR-EXCHANGE-207: after the 191 one-agent
    // terminal ascent reaches its fixed point, spend only the remaining
    // protected terminal budget on strict-improvement pair exchanges.
    slackRefiner.enableTerminalPairExchange = true;
    // Accepted SCORE-MIDDAY-CHAIN-ADOPTION-210: mid-day one-agent deep-chain
    // substitution under the unchanged strict_protected_improvement
    // certificate, chained after the wait-detour fixed point inside the same
    // protected refinement window.
    slackRefiner.enableMiddayChainAdoption = true;
    // Accepted SCORE-MIDDAY-TARGET-FOLLOWUP-215: after the complete 210
    // global-pool fixed point, spend only remaining protected time on sparse
    // routes conditioned on each patrol's already protected terminal. Every
    // takeover still passes the unchanged strict future-domain certificate.
    slackRefiner.enableMiddayTargetTerminalFollowup = true;
    bool idlePostAckWorkPending = false;
    bool idleContingencyPrecompute = true;
    while (true) {
        const HttpResponse stateResponse = client.request("GET", root + "/state");
        if (stateResponse.status == 200) {
            idlePostAckWorkPending = false;
            idleContingencyPrecompute = true;
            const auto receivedAt = std::chrono::system_clock::now();
            const auto responseStarted = std::chrono::steady_clock::now();
            const udon::JsonValue stateDocument = parse_optional_json(stateResponse.body);
            const std::int32_t wireDay = static_cast<std::int32_t>(stateDocument.at("day").integer());
            if (wireDay == lastAcceptedWireDay) {
                std::this_thread::sleep_for(std::chrono::milliseconds{options.pollMs});
                continue;
            }
            replay.record("day_state", stateDocument, stateResponse.status);
            const udon::DayState state = udon::parse_btc_day_state(
                config,
                stateDocument,
                receivedAt,
                adapterOptions);
            if (!checkpointDivergenceActive ||
                !checkpointAgents.has_value() ||
                !udon::protected_slack_agents_dominate(
                    *checkpointAgents,
                    state.agents) ||
                !udon::protected_slack_ledger_dominates(
                    checkpointLedger,
                    ledger)) {
                checkpointAgents = state.agents;
                checkpointLedger = ledger;
                checkpointDivergenceActive = false;
            }
            udon::DayState checkpointState = state;
            checkpointState.agents = *checkpointAgents;
            if (!protectedDivergenceActive ||
                !virtualAgents.has_value() ||
                !udon::protected_slack_agents_dominate(
                    *virtualAgents,
                    checkpointState.agents) ||
                !udon::protected_slack_ledger_dominates(
                    virtualLedger,
                    checkpointLedger)) {
                virtualAgents = checkpointState.agents;
                virtualLedger = checkpointLedger;
                protectedDivergenceActive = false;
            }
            udon::DayState planningState = checkpointState;
            planningState.agents = *virtualAgents;
            const std::int64_t receivedAtUnixMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    receivedAt.time_since_epoch()).count();
            const std::int64_t configuredDeadlineMs =
                receivedAtUnixMs + effective_response_budget_ms(options);
            const std::int64_t actionDeadlineMs =
                udon::btc_authoritative_action_deadline_ms(
                    config,
                    stateDocument,
                    receivedAt,
                    adapterOptions);
            const bool publicContinuationAuthorized =
                actionDeadlineMs > configuredDeadlineMs;
            const bool checkpointClosedLoopActive =
                checkpointDivergenceActive || publicContinuationAuthorized;
            const std::int64_t solveDeadlineMs = std::min(
                configuredDeadlineMs,
                actionDeadlineMs);
            const udon::SessionDecision decision = session.on_authoritative_state_for(
                planningState,
                virtualLedger,
                std::chrono::milliseconds{
                    std::max<std::int64_t>(
                        0,
                        solveDeadlineMs - receivedAtUnixMs)});
            replay.record("decision", decision.replay);
            const std::int64_t minimumSubmissionWindowMs = std::min<std::int64_t>(
                btcSubmissionFloorMs,
                std::max<std::int64_t>(
                    200,
                    deadlineCalibration.networkFloor.count() - 100));
            if (unix_milliseconds() + minimumSubmissionWindowMs >= actionDeadlineMs) {
                if (session.has_pending_submission()) {
                    session.reject_pending_submission();
                }
                udon::JsonValue::Object skipped;
                skipped.emplace("day", udon::JsonValue(static_cast<std::int64_t>(state.dayNumber)));
                skipped.emplace("reason", udon::JsonValue("insufficient-authoritative-day-window"));
                replay.record("actions_deadline_skip", udon::JsonValue(std::move(skipped)));
                const udon::DayPlan waitPlan = udon::make_wait_plan(config, state.dayNumber);
                const udon::SimulationResult waitSimulation = validate_fallback_plan(
                    state,
                    waitPlan,
                    simulator,
                    validator,
                    "BTC deadline WAIT failed local validation");
                replay.record(
                    "actions_server_wait",
                    udon::serialize_day_plan(waitPlan));
                session.record_applied_transition(state, waitSimulation);
                ledger.apply(waitSimulation.score);
                checkpointLedger = ledger;
                checkpointAgents = waitSimulation.finalAgents;
                checkpointDivergenceActive = false;
                virtualLedger = ledger;
                virtualAgents = waitSimulation.finalAgents;
                protectedDivergenceActive = false;
                lastAcceptedWireDay = wireDay;
                continue;
            }
            udon::DayPlan submittedPlan;
            udon::SimulationResult submittedSimulation;
            udon::DayPlan checkpointPlan;
            udon::SimulationResult checkpointSimulation;
            bool submittedPublicImprovement = false;
            bool submitDecision = decision.maySubmit;
            if (submitDecision) {
                submittedPlan = decision.decision.candidate.plan;
                std::string authoritativeFailure;
                const std::optional<udon::SimulationResult> authoritativeSimulation =
                    try_validate_plan(
                        checkpointState,
                        submittedPlan,
                        simulator,
                        validator,
                        authoritativeFailure);
                if (authoritativeSimulation.has_value()) {
                    submittedSimulation = *authoritativeSimulation;
                } else {
                    if (session.has_pending_submission()) {
                        session.reject_pending_submission();
                    }
                    submitDecision = false;
                    checkpointLedger = ledger;
                    checkpointAgents = state.agents;
                    checkpointDivergenceActive = false;
                    virtualLedger = ledger;
                    virtualAgents = state.agents;
                    protectedDivergenceActive = false;
                    udon::JsonValue::Object dropped;
                    dropped.emplace(
                        "day",
                        udon::JsonValue(static_cast<std::int64_t>(state.dayNumber)));
                    dropped.emplace("reason", udon::JsonValue(authoritativeFailure));
                    replay.record(
                        "virtual_parent_drop",
                        udon::JsonValue(std::move(dropped)));
                }
            }
            if (submitDecision) {
                udon::ProtectedSlackResult refinement;
                refinement.plan = submittedPlan;
                refinement.simulation = submittedSimulation;
                refinement.scoreAfterToday =
                    udon::OfficialScore::after_day(
                        checkpointLedger,
                        submittedSimulation.score);
                const std::int64_t refinementReserveMs =
                    std::max<std::int64_t>(
                        minimumSubmissionWindowMs,
                        btcProtectedRefinementFloorMs);
                const std::int64_t refinementDeadlineMs = std::min(
                    configuredDeadlineMs,
                    actionDeadlineMs - refinementReserveMs);
                const std::int64_t refinementRemainingMs =
                    refinementDeadlineMs - unix_milliseconds();
                if (refinementRemainingMs > 0) {
                    const auto refinementDeadline =
                        std::chrono::steady_clock::now() +
                        std::chrono::milliseconds{refinementRemainingMs};
                    if (state.dayNumber == config.day_count()) {
                        refinement = slackRefiner.refine_terminal_sparse(
                            checkpointState,
                            checkpointLedger,
                            submittedPlan,
                            submittedSimulation,
                            refinementDeadline);
                    } else {
                        refinement = slackRefiner.refine_wait_detours(
                            checkpointState,
                            checkpointLedger,
                            submittedPlan,
                            submittedSimulation,
                            refinementDeadline);
                        const udon::ProtectedSlackResult midday =
                            slackRefiner.refine_midday_chains(
                                checkpointState,
                                checkpointLedger,
                                refinement.plan,
                                refinement.simulation,
                                refinementDeadline);
                        refinement.diagnostics.middayRoutes =
                            midday.diagnostics.middayRoutes;
                        refinement.diagnostics.middayGeneratedPlans =
                            midday.diagnostics.middayGeneratedPlans;
                        refinement.diagnostics.middayValidPlans =
                            midday.diagnostics.middayValidPlans;
                        refinement.diagnostics.middayChainAcceptances =
                            midday.diagnostics.middayChainAcceptances;
                        refinement.diagnostics.middayRounds =
                            midday.diagnostics.middayRounds;
                        refinement.diagnostics.middayTargetRoutes =
                            midday.diagnostics.middayTargetRoutes;
                        refinement.diagnostics.middayTargetGeneratedPlans =
                            midday.diagnostics.middayTargetGeneratedPlans;
                        refinement.diagnostics.middayTargetValidPlans =
                            midday.diagnostics.middayTargetValidPlans;
                        refinement.diagnostics.middayTargetAcceptances =
                            midday.diagnostics.middayTargetAcceptances;
                        refinement.diagnostics.middayTargetRounds =
                            midday.diagnostics.middayTargetRounds;
                        refinement.diagnostics.middayChain =
                            midday.diagnostics.middayChain;
                        refinement.diagnostics.middayTargetFollowup =
                            midday.diagnostics.middayTargetFollowup;
                        refinement.diagnostics.middayFailure =
                            midday.diagnostics.middayFailure;
                        refinement.diagnostics.deadlineReached =
                            refinement.diagnostics.deadlineReached ||
                            midday.diagnostics.deadlineReached;
                        if (midday.improved) {
                            refinement.plan = midday.plan;
                            refinement.simulation = midday.simulation;
                            refinement.scoreAfterToday = midday.scoreAfterToday;
                            refinement.firstRoundScore = midday.firstRoundScore;
                            refinement.improved = true;
                            refinement.witnessAgent = midday.witnessAgent;
                            refinement.witnessParentFuel =
                                midday.witnessParentFuel;
                            refinement.witnessCandidateFuel =
                                midday.witnessCandidateFuel;
                        }
                    }
                }
                if (refinement.improved) {
                    submittedPlan = refinement.plan;
                    submittedSimulation = refinement.simulation;
                }
                udon::MatchLedger prospectiveVirtualLedger = virtualLedger;
                prospectiveVirtualLedger.apply(
                    decision.decision.candidate.simulation.score);
                udon::MatchLedger prospectiveCheckpointLedger = checkpointLedger;
                prospectiveCheckpointLedger.apply(submittedSimulation.score);
                const auto submission_admissible = [&]() {
                    const bool terminalDay =
                        state.dayNumber == config.day_count();
                    return (terminalDay ||
                            udon::protected_slack_transition_dominates(
                                decision.decision.candidate.simulation,
                                submittedSimulation)) &&
                        udon::protected_slack_ledger_relation_for_day(
                            prospectiveVirtualLedger,
                            prospectiveCheckpointLedger,
                            terminalDay);
                };
                if (!submission_admissible()) {
                    submittedPlan = decision.decision.candidate.plan;
                    submittedSimulation = validate_fallback_plan(
                        checkpointState,
                        submittedPlan,
                        simulator,
                        validator,
                        "BTC protected-slack parent revalidation failed");
                    prospectiveCheckpointLedger = checkpointLedger;
                    prospectiveCheckpointLedger.apply(submittedSimulation.score);
                    if (!submission_admissible()) {
                        throw std::runtime_error(
                            "BTC virtual-parent dominance invariant failed before submission");
                    }
                }
                checkpointPlan = submittedPlan;
                checkpointSimulation = submittedSimulation;
                udon::ProtectedSlackResult publicContinuation;
                publicContinuation.plan = submittedPlan;
                publicContinuation.simulation = submittedSimulation;
                publicContinuation.scoreAfterToday =
                    udon::OfficialScore::after_day(
                        ledger,
                        submittedSimulation.score);
                const std::int64_t publicContinuationDeadlineMs =
                    actionDeadlineMs - refinementReserveMs;
                std::int64_t publicContinuationRemainingMs = 0;
                if (checkpointClosedLoopActive) {
                    const udon::SimulationResult replayedCheckpoint =
                        validate_fallback_plan(
                            state,
                            checkpointPlan,
                            simulator,
                            validator,
                            "BTC checkpoint replay failed on authoritative richer state");
                    udon::MatchLedger prospectiveRicherLedger = ledger;
                    prospectiveRicherLedger.apply(replayedCheckpoint.score);
                    const bool terminalDay =
                        state.dayNumber == config.day_count();
                    if ((!terminalDay &&
                         !udon::protected_slack_transition_dominates(
                             checkpointSimulation,
                             replayedCheckpoint)) ||
                        !udon::protected_slack_ledger_relation_for_day(
                            prospectiveCheckpointLedger,
                            prospectiveRicherLedger,
                            terminalDay)) {
                        throw std::runtime_error(
                            "BTC checkpoint replay failed richer-state dominance");
                    }
                    replay.record(
                        "checkpoint_actions",
                        udon::serialize_day_plan(checkpointPlan));
                    submittedPlan = checkpointPlan;
                    submittedSimulation = replayedCheckpoint;
                    publicContinuation.plan = submittedPlan;
                    publicContinuation.simulation = submittedSimulation;
                    publicContinuation.scoreAfterToday =
                        udon::OfficialScore::after_day(
                            ledger,
                            submittedSimulation.score);
                    publicContinuationRemainingMs =
                        publicContinuationDeadlineMs - unix_milliseconds();
                    if (publicContinuationAuthorized &&
                        publicContinuationRemainingMs > 0) {
                        const auto publicDeadline =
                            std::chrono::steady_clock::now() +
                            std::chrono::milliseconds{
                                publicContinuationRemainingMs};
                        udon::ProtectedSlackRefiner publicSlackRefiner =
                            slackRefiner;
                        publicSlackRefiner.enableTerminalMarginalReservoir =
                            true;
                        if (state.dayNumber == config.day_count()) {
                            publicContinuation =
                                publicSlackRefiner.refine_terminal_sparse(
                                    state,
                                    ledger,
                                    submittedPlan,
                                    submittedSimulation,
                                    publicDeadline);
                        } else {
                            const udon::ProtectedSlackResult publicWait =
                                publicSlackRefiner.refine_wait_detours(
                                    state,
                                    ledger,
                                    submittedPlan,
                                    submittedSimulation,
                                    publicDeadline);
                            publicContinuation = publicWait;
                            const udon::ProtectedSlackResult publicMidday =
                                publicSlackRefiner.refine_midday_chains(
                                    state,
                                    ledger,
                                    publicWait.plan,
                                    publicWait.simulation,
                                    publicDeadline);
                            publicContinuation.diagnostics.middayRoutes =
                                publicMidday.diagnostics.middayRoutes;
                            publicContinuation.diagnostics.middayGeneratedPlans =
                                publicMidday.diagnostics.middayGeneratedPlans;
                            publicContinuation.diagnostics.middayValidPlans =
                                publicMidday.diagnostics.middayValidPlans;
                            publicContinuation.diagnostics.middayChainAcceptances =
                                publicMidday.diagnostics.middayChainAcceptances;
                            publicContinuation.diagnostics.middayRounds =
                                publicMidday.diagnostics.middayRounds;
                            publicContinuation.diagnostics.middayTargetRoutes =
                                publicMidday.diagnostics.middayTargetRoutes;
                            publicContinuation.diagnostics.middayTargetGeneratedPlans =
                                publicMidday.diagnostics.middayTargetGeneratedPlans;
                            publicContinuation.diagnostics.middayTargetValidPlans =
                                publicMidday.diagnostics.middayTargetValidPlans;
                            publicContinuation.diagnostics.middayTargetAcceptances =
                                publicMidday.diagnostics.middayTargetAcceptances;
                            publicContinuation.diagnostics.middayTargetRounds =
                                publicMidday.diagnostics.middayTargetRounds;
                            publicContinuation.diagnostics.middayChain =
                                publicMidday.diagnostics.middayChain;
                            publicContinuation.diagnostics.middayTargetFollowup =
                                publicMidday.diagnostics.middayTargetFollowup;
                            publicContinuation.diagnostics.middayFailure =
                                publicMidday.diagnostics.middayFailure;
                            publicContinuation.diagnostics.deadlineReached =
                                publicWait.diagnostics.deadlineReached ||
                                publicMidday.diagnostics.deadlineReached;
                            if (publicMidday.improved) {
                                publicContinuation.plan = publicMidday.plan;
                                publicContinuation.simulation =
                                    publicMidday.simulation;
                                publicContinuation.scoreAfterToday =
                                    publicMidday.scoreAfterToday;
                                publicContinuation.firstRoundScore =
                                    publicMidday.firstRoundScore;
                                publicContinuation.improved = true;
                                publicContinuation.witnessAgent =
                                    publicMidday.witnessAgent;
                                publicContinuation.witnessParentFuel =
                                    publicMidday.witnessParentFuel;
                                publicContinuation.witnessCandidateFuel =
                                    publicMidday.witnessCandidateFuel;
                            }
                        }
                    }
                    if (publicContinuation.improved) {
                        const udon::OfficialScore richerCheckpointScore =
                            udon::OfficialScore::after_day(
                                ledger,
                                replayedCheckpoint.score);
                        const bool publicContinuationCertified =
                            state.dayNumber == config.day_count()
                            ? richerCheckpointScore <
                                  publicContinuation.scoreAfterToday
                            : udon::protected_slack_transition_dominates(
                                  replayedCheckpoint,
                                  publicContinuation.simulation) &&
                                  richerCheckpointScore <
                                      publicContinuation.scoreAfterToday;
                        if (!publicContinuationCertified) {
                            throw std::runtime_error(
                                "BTC public continuation failed strict richer-state certificate");
                        }
                        submittedPlan = publicContinuation.plan;
                        submittedSimulation = publicContinuation.simulation;
                        submittedPublicImprovement = true;
                    }
                    prospectiveRicherLedger = ledger;
                    prospectiveRicherLedger.apply(submittedSimulation.score);
                    const bool completeCheckpointDominates =
                        (terminalDay ||
                         udon::protected_slack_transition_dominates(
                             checkpointSimulation,
                             submittedSimulation)) &&
                        udon::protected_slack_ledger_relation_for_day(
                            prospectiveCheckpointLedger,
                            prospectiveRicherLedger,
                            terminalDay);
                    if (!completeCheckpointDominates) {
                        throw std::runtime_error(
                            "BTC public continuation failed complete checkpoint dominance");
                    }
                }
                udon::JsonValue::Object telemetry;
                telemetry.emplace(
                    "virtualParentActive",
                    udon::JsonValue(protectedDivergenceActive));
                telemetry.emplace(
                    "day",
                    udon::JsonValue(
                        static_cast<std::int64_t>(state.dayNumber)));
                telemetry.emplace(
                    "outerWindowMs",
                    udon::JsonValue(std::max<std::int64_t>(
                        0,
                        actionDeadlineMs - receivedAtUnixMs)));
                telemetry.emplace(
                    "computeWindowMs",
                    udon::JsonValue(std::max<std::int64_t>(
                        0,
                        solveDeadlineMs - receivedAtUnixMs)));
                telemetry.emplace(
                    "refinementReserveMs",
                    udon::JsonValue(refinementReserveMs));
                telemetry.emplace(
                    "refinementDeadlineMs",
                    udon::JsonValue(refinementDeadlineMs));
                telemetry.emplace(
                    "refinementBudgetMs",
                    udon::JsonValue(std::max<std::int64_t>(
                        0,
                        refinementRemainingMs)));
                telemetry.emplace(
                    "waitAnchors",
                    udon::JsonValue(refinement.diagnostics.waitAnchors));
                telemetry.emplace(
                    "eligibleWaitAnchors",
                    udon::JsonValue(
                        refinement.diagnostics.eligibleWaitAnchors));
                telemetry.emplace(
                    "routePairs",
                    udon::JsonValue(refinement.diagnostics.routePairs));
                telemetry.emplace(
                    "generatedPlans",
                    udon::JsonValue(
                        refinement.diagnostics.generatedPlans));
                telemetry.emplace(
                    "validPlans",
                    udon::JsonValue(refinement.diagnostics.validPlans));
                telemetry.emplace(
                    "liftablePlans",
                    udon::JsonValue(
                        refinement.diagnostics.liftablePlans));
                telemetry.emplace(
                    "sparseRoutes",
                    udon::JsonValue(
                        refinement.diagnostics.sparseRoutes));
                telemetry.emplace(
                    "strictTerminalImprovements",
                    udon::JsonValue(
                        refinement.diagnostics.strictTerminalImprovements));
                telemetry.emplace(
                    "terminalSparseRounds",
                    udon::JsonValue(
                        refinement.diagnostics.terminalSparseRounds));
                telemetry.emplace(
                    "middayRoutes",
                    udon::JsonValue(refinement.diagnostics.middayRoutes));
                telemetry.emplace(
                    "middayGeneratedPlans",
                    udon::JsonValue(
                        refinement.diagnostics.middayGeneratedPlans));
                telemetry.emplace(
                    "middayValidPlans",
                    udon::JsonValue(
                        refinement.diagnostics.middayValidPlans));
                telemetry.emplace(
                    "middayChainAcceptances",
                    udon::JsonValue(
                        refinement.diagnostics.middayChainAcceptances));
                telemetry.emplace(
                    "middayRounds",
                    udon::JsonValue(refinement.diagnostics.middayRounds));
                telemetry.emplace(
                    "middayTargetRoutes",
                    udon::JsonValue(
                        refinement.diagnostics.middayTargetRoutes));
                telemetry.emplace(
                    "middayTargetGeneratedPlans",
                    udon::JsonValue(
                        refinement.diagnostics.middayTargetGeneratedPlans));
                telemetry.emplace(
                    "middayTargetValidPlans",
                    udon::JsonValue(
                        refinement.diagnostics.middayTargetValidPlans));
                telemetry.emplace(
                    "middayTargetAcceptances",
                    udon::JsonValue(
                        refinement.diagnostics.middayTargetAcceptances));
                telemetry.emplace(
                    "middayTargetRounds",
                    udon::JsonValue(
                        refinement.diagnostics.middayTargetRounds));
                telemetry.emplace(
                    "middayChain",
                    udon::JsonValue(refinement.diagnostics.middayChain));
                telemetry.emplace(
                    "middayTargetFollowup",
                    udon::JsonValue(
                        refinement.diagnostics.middayTargetFollowup));
                telemetry.emplace(
                    "middayFailure",
                    udon::JsonValue(refinement.diagnostics.middayFailure));
                telemetry.emplace(
                    "firstRoundDailyDistinct",
                    udon::JsonValue(static_cast<std::int64_t>(
                        refinement.firstRoundScore.totalDailyDistinct)));
                telemetry.emplace(
                    "firstRoundServings",
                    udon::JsonValue(static_cast<std::int64_t>(
                        refinement.firstRoundScore.totalServings)));
                telemetry.emplace(
                    "terminalSparse",
                    udon::JsonValue(
                        refinement.diagnostics.terminalSparse));
                telemetry.emplace(
                    "sparseFailure",
                    udon::JsonValue(
                        refinement.diagnostics.sparseFailure));
                telemetry.emplace(
                    "deadlineReached",
                    udon::JsonValue(
                        refinement.diagnostics.deadlineReached));
                telemetry.emplace(
                    "improved",
                    udon::JsonValue(refinement.improved));
                telemetry.emplace(
                    "parentDailyDistinct",
                    udon::JsonValue(static_cast<std::int64_t>(
                        decision.decision.candidate.simulation.score.dailyDistinct)));
                telemetry.emplace(
                    "parentServings",
                    udon::JsonValue(static_cast<std::int64_t>(
                        decision.decision.candidate.simulation.score.servings)));
                telemetry.emplace(
                    "refinedDailyDistinct",
                    udon::JsonValue(static_cast<std::int64_t>(
                        submittedSimulation.score.dailyDistinct)));
                telemetry.emplace(
                    "refinedServings",
                    udon::JsonValue(static_cast<std::int64_t>(
                        submittedSimulation.score.servings)));
                telemetry.emplace(
                    "witnessAgent",
                    udon::JsonValue(static_cast<std::int64_t>(
                        refinement.witnessAgent)));
                telemetry.emplace(
                    "witnessAnchor",
                    udon::JsonValue(static_cast<std::int64_t>(
                        refinement.witnessAnchor)));
                telemetry.emplace(
                    "witnessSpot",
                    udon::JsonValue(static_cast<std::int64_t>(
                        refinement.witnessSpot)));
                telemetry.emplace(
                    "witnessDuration",
                    udon::JsonValue(static_cast<std::int64_t>(
                        refinement.witnessDuration)));
                telemetry.emplace(
                    "witnessTravelSteps",
                    udon::JsonValue(static_cast<std::int64_t>(
                        refinement.witnessTravelSteps)));
                telemetry.emplace(
                    "witnessParentFuel",
                    udon::JsonValue(static_cast<std::int64_t>(
                        refinement.witnessParentFuel)));
                telemetry.emplace(
                    "witnessCandidateFuel",
                    udon::JsonValue(static_cast<std::int64_t>(
                        refinement.witnessCandidateFuel)));
                if (checkpointClosedLoopActive) {
                    telemetry.emplace(
                        "checkpointClosedLoop",
                        udon::JsonValue(true));
                    telemetry.emplace(
                        "checkpointDivergenceActive",
                        udon::JsonValue(checkpointDivergenceActive));
                    telemetry.emplace(
                        "publicContinuationAuthorized",
                        udon::JsonValue(publicContinuationAuthorized));
                    telemetry.emplace(
                        "publicContinuationDeadlineMs",
                        udon::JsonValue(publicContinuationDeadlineMs));
                    telemetry.emplace(
                        "publicContinuationBudgetMs",
                        udon::JsonValue(std::max<std::int64_t>(
                            0,
                            publicContinuationRemainingMs)));
                    telemetry.emplace(
                        "publicContinuationImproved",
                        udon::JsonValue(submittedPublicImprovement));
                    telemetry.emplace(
                        "publicContinuationDeadlineReached",
                        udon::JsonValue(
                            publicContinuation.diagnostics.deadlineReached));
                    telemetry.emplace(
                        "publicContinuationFailure",
                        udon::JsonValue(
                            publicContinuation.diagnostics.sparseFailure ||
                            publicContinuation.diagnostics.middayFailure ||
                            publicContinuation.diagnostics.terminalMarginalFailure));
                    telemetry.emplace(
                        "publicContinuationSparseRoutes",
                        udon::JsonValue(
                            publicContinuation.diagnostics.sparseRoutes));
                    telemetry.emplace(
                        "publicContinuationMiddayRoutes",
                        udon::JsonValue(
                            publicContinuation.diagnostics.middayRoutes));
                    telemetry.emplace(
                        "publicContinuationGeneratedPlans",
                        udon::JsonValue(
                            publicContinuation.diagnostics.generatedPlans +
                            publicContinuation.diagnostics.middayGeneratedPlans +
                            publicContinuation.diagnostics
                                .terminalMarginalGeneratedPlans));
                    telemetry.emplace(
                        "publicContinuationValidPlans",
                        udon::JsonValue(
                            publicContinuation.diagnostics.validPlans +
                            publicContinuation.diagnostics.middayValidPlans +
                            publicContinuation.diagnostics
                                .terminalMarginalValidPlans));
                    telemetry.emplace(
                        "publicContinuationAcceptances",
                        udon::JsonValue(
                            publicContinuation.diagnostics.strictTerminalImprovements +
                            publicContinuation.diagnostics.middayChainAcceptances +
                            publicContinuation.diagnostics
                                .terminalMarginalAcceptances));
                    telemetry.emplace(
                        "publicContinuationTerminalMarginalRoutes",
                        udon::JsonValue(
                            publicContinuation.diagnostics.terminalMarginalRoutes));
                    telemetry.emplace(
                        "publicContinuationTerminalMarginalGeneratedPlans",
                        udon::JsonValue(
                            publicContinuation.diagnostics
                                .terminalMarginalGeneratedPlans));
                    telemetry.emplace(
                        "publicContinuationTerminalMarginalValidPlans",
                        udon::JsonValue(
                            publicContinuation.diagnostics
                                .terminalMarginalValidPlans));
                    telemetry.emplace(
                        "publicContinuationTerminalMarginalAcceptances",
                        udon::JsonValue(
                            publicContinuation.diagnostics
                                .terminalMarginalAcceptances));
                    telemetry.emplace(
                        "publicContinuationTerminalMarginalRounds",
                        udon::JsonValue(
                            publicContinuation.diagnostics.terminalMarginalRounds));
                    telemetry.emplace(
                        "publicContinuationTerminalMarginalDeadlineRollback",
                        udon::JsonValue(
                            publicContinuation.diagnostics.terminalMarginalDeadline));
                    telemetry.emplace(
                        "publicContinuationTerminalMarginalFailure",
                        udon::JsonValue(
                            publicContinuation.diagnostics.terminalMarginalFailure));
                    telemetry.emplace(
                        "checkpointDailyDistinct",
                        udon::JsonValue(static_cast<std::int64_t>(
                            checkpointSimulation.score.dailyDistinct)));
                    telemetry.emplace(
                        "checkpointServings",
                        udon::JsonValue(static_cast<std::int64_t>(
                            checkpointSimulation.score.servings)));
                }
                replay.record(
                    "protected_slack",
                    udon::JsonValue(std::move(telemetry)));
            } else {
                submittedPlan =
                    udon::make_wait_plan(config, state.dayNumber);
                submittedSimulation = validate_fallback_plan(
                    state,
                    submittedPlan,
                    simulator,
                    validator,
                    "BTC exact WAIT fallback failed local validation");
            }
            udon::JsonValue wirePlan = udon::serialize_day_plan(submittedPlan);
            replay.record(submitDecision ? "actions" : "actions_fallback", wirePlan);
            const std::string wireBody = wirePlan.dump();

            HttpResponse actionResponse = post_until_deadline(
                client,
                root + "/actions",
                wireBody,
                options.pollMs,
                options.actionAckTimeoutMs,
                actionDeadlineMs);
            if (actionResponse.transportRetries > 0) {
                udon::JsonValue::Object retryTelemetry;
                retryTelemetry.emplace(
                    "day",
                    udon::JsonValue(static_cast<std::int64_t>(state.dayNumber)));
                retryTelemetry.emplace(
                    "transportRetries",
                    udon::JsonValue(static_cast<std::int64_t>(actionResponse.transportRetries)));
                replay.record(
                    "action_transport_retry",
                    udon::JsonValue(std::move(retryTelemetry)));
            }
            udon::JsonValue actionResult = parse_tolerant_http_body(actionResponse.body);
            replay.record("action_result", actionResult, actionResponse.status);
            bool accepted = actionResponse.status >= 200 && actionResponse.status < 300 &&
                udon::btc_action_result_accepted(actionResult);
            const std::optional<std::int32_t> acceptedDay = udon::btc_action_result_day(actionResult);
            if (!accepted && acceptedDay.has_value() &&
                *acceptedDay != wireDay + 1) {
                if (session.has_pending_submission()) {
                    session.reject_pending_submission();
                }
                const udon::DayPlan waitPlan = udon::make_wait_plan(config, state.dayNumber);
                const udon::SimulationResult waitSimulation = validate_fallback_plan(
                    state,
                    waitPlan,
                    simulator,
                    validator,
                    "BTC expired WAIT failed local validation");
                replay.record(
                    "actions_server_wait",
                    udon::serialize_day_plan(waitPlan));
                session.record_applied_transition(state, waitSimulation);
                ledger.apply(waitSimulation.score);
                checkpointLedger = ledger;
                checkpointAgents = waitSimulation.finalAgents;
                checkpointDivergenceActive = false;
                virtualLedger = ledger;
                virtualAgents = waitSimulation.finalAgents;
                protectedDivergenceActive = false;
                lastAcceptedWireDay = wireDay;
                continue;
            }
            if (accepted && acceptedDay.has_value() && *acceptedDay != wireDay + 1) {
                if (session.has_pending_submission()) {
                    session.reject_pending_submission();
                }
                throw std::runtime_error(
                    "BTC accepted an action for a stale day: expected " +
                    std::to_string(wireDay + 1) + ", got " + std::to_string(*acceptedDay));
            }
            bool appliedDecision = accepted && submitDecision;
            std::optional<udon::SimulationResult> appliedFallback;
            if (accepted && !submitDecision) {
                appliedFallback = submittedSimulation;
            }
            if (!accepted) {
                if (session.has_pending_submission()) {
                    session.reject_pending_submission();
                }
                submittedPlan = udon::make_wait_plan(config, state.dayNumber);
                wirePlan = udon::serialize_day_plan(submittedPlan);
                replay.record("actions_recovery_wait", wirePlan);
                actionResponse = post_until_deadline(
                    client,
                    root + "/actions",
                    wirePlan.dump(),
                    options.pollMs,
                    options.actionAckTimeoutMs,
                    actionDeadlineMs);
                if (actionResponse.transportRetries > 0) {
                    udon::JsonValue::Object retryTelemetry;
                    retryTelemetry.emplace(
                        "day",
                        udon::JsonValue(static_cast<std::int64_t>(state.dayNumber)));
                    retryTelemetry.emplace(
                        "transportRetries",
                        udon::JsonValue(static_cast<std::int64_t>(actionResponse.transportRetries)));
                    replay.record(
                        "action_recovery_transport_retry",
                        udon::JsonValue(std::move(retryTelemetry)));
                }
                actionResult = parse_tolerant_http_body(actionResponse.body);
                replay.record("action_result_recovery", actionResult, actionResponse.status);
                accepted = actionResponse.status >= 200 && actionResponse.status < 300 &&
                    udon::btc_action_result_accepted(actionResult);
                const std::optional<std::int32_t> recoveryDay = udon::btc_action_result_day(actionResult);
                if (accepted && recoveryDay.has_value() && *recoveryDay != wireDay + 1) {
                    throw std::runtime_error(
                        "BTC accepted a recovery WAIT for a stale day: expected " +
                        std::to_string(wireDay + 1) + ", got " + std::to_string(*recoveryDay));
                }
                appliedDecision = false;
                if (!accepted) {
                    throw std::runtime_error(
                        "BTC rejected both certified plan and exact WAIT fallback: " +
                        udon::btc_action_result_reason(actionResult));
                }
                const udon::SimulationResult waitSimulation = validate_fallback_plan(
                    state,
                    submittedPlan,
                    simulator,
                    validator,
                    "BTC recovery WAIT failed local validation");
                appliedFallback = waitSimulation;
            }
            const std::chrono::milliseconds responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - responseStarted);
            if (appliedDecision) {
                static_cast<void>(session.acknowledge_submitted(responseTime));
                record_session_checkpoint(state.dayNumber);
                virtualLedger.apply(
                    decision.decision.candidate.simulation.score);
                virtualAgents =
                    decision.decision.candidate.simulation.finalAgents;
                checkpointLedger.apply(checkpointSimulation.score);
                checkpointAgents = checkpointSimulation.finalAgents;
                ledger.apply(submittedSimulation.score);
                protectedDivergenceActive =
                    !exact_ledgers_equal(virtualLedger, checkpointLedger) ||
                    !exact_agent_states_equal(
                        *virtualAgents,
                        *checkpointAgents);
                checkpointDivergenceActive =
                    !exact_ledgers_equal(checkpointLedger, ledger) ||
                    !exact_agent_states_equal(
                        *checkpointAgents,
                        submittedSimulation.finalAgents);
                const bool terminalDay =
                    state.dayNumber == config.day_count();
                const bool acknowledgedCheckpointLedgerValid =
                    udon::protected_slack_ledger_relation_for_day(
                        virtualLedger,
                        checkpointLedger,
                        terminalDay);
                const bool acknowledgedSubmittedLedgerValid =
                    udon::protected_slack_ledger_relation_for_day(
                        checkpointLedger,
                        ledger,
                        terminalDay);
                if (!acknowledgedCheckpointLedgerValid ||
                    !acknowledgedSubmittedLedgerValid) {
                    throw std::runtime_error(
                        "BTC checkpoint closed-loop ledger invariant failed after acknowledgement");
                }
                idlePostAckWorkPending = state.dayNumber < config.day_count();
            } else if (appliedFallback.has_value()) {
                session.record_applied_transition(state, *appliedFallback);
                ledger.apply(appliedFallback->score);
                checkpointLedger = ledger;
                checkpointAgents = appliedFallback->finalAgents;
                checkpointDivergenceActive = false;
                virtualLedger = ledger;
                virtualAgents = appliedFallback->finalAgents;
                protectedDivergenceActive = false;
            }
            lastAcceptedWireDay = wireDay;
            continue;
        }

        const HttpResponse resultResponse = client.request("GET", root + "/result");
        if (resultResponse.status == 200) {
            const udon::JsonValue result = parse_optional_json(resultResponse.body);
            replay.record("result", result, resultResponse.status);
            std::cout << result.dump() << '\n';
            return;
        }
        if (!transient_http_status(stateResponse.status) &&
            stateResponse.status != 404 && stateResponse.status != 409) {
            throw std::runtime_error(
                "BTC state returned HTTP " + std::to_string(stateResponse.status));
        }
        const auto idleStarted = std::chrono::steady_clock::now();
        if (idlePostAckWorkPending) {
            const std::chrono::milliseconds idleSlice{
                std::max<std::int32_t>(1, std::min<std::int32_t>(100, options.pollMs / 2))};
            if (idleContingencyPrecompute) {
                const std::int32_t added = session.precompute_until(idleSlice);
                idleContingencyPrecompute = added > 0;
            } else {
                static_cast<void>(session.prove_until(idleSlice));
            }
            record_session_checkpoint(lastAcceptedWireDay + 1);
        }
        const std::chrono::milliseconds idleElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - idleStarted);
        const std::chrono::milliseconds pollDelay{
            std::max<std::int64_t>(0, static_cast<std::int64_t>(options.pollMs) - idleElapsed.count())};
        std::this_thread::sleep_for(pollDelay);
    }
}

#else

void run_http(const RuntimeOptions&) {
    throw std::runtime_error("HTTP mode currently requires Windows WinHTTP; use sandbox mode on this platform");
}

#endif

}

int main(int argumentCount, char** arguments) {
    try {
        const RuntimeOptions options = parse_arguments(argumentCount, arguments);
        if (options.mode == "sandbox") {
            run_sandbox(options);
            return EXIT_SUCCESS;
        }
        if (options.mode == "http") {
            run_http(options);
            return EXIT_SUCCESS;
        }
        if (options.mode == "replay-check") {
            run_replay_check(options);
            return EXIT_SUCCESS;
        }
        if (options.mode == "replay-roles") {
            run_replay_roles(options);
            return EXIT_SUCCESS;
        }
        if (options.mode == "replay-counterfactual") {
            run_replay_counterfactual(options);
            return EXIT_SUCCESS;
        }
        if (options.mode == "replay-solve") {
            run_replay_solve(options);
            return EXIT_SUCCESS;
        }
        print_usage();
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        print_usage();
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
