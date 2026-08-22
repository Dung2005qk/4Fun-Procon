#include "udon/btc_protocol.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "udon/protocol.hpp"

namespace udon {

namespace {

[[nodiscard]] const JsonValue::Object& require_object(const JsonValue& value, std::string_view name) {
    if (!value.is_object()) {
        throw ProtocolError(std::string(name) + " must be an object");
    }
    return value.object();
}

[[nodiscard]] const JsonValue::Array& require_array(const JsonValue& value, std::string_view name) {
    if (!value.is_array()) {
        throw ProtocolError(std::string(name) + " must be an array");
    }
    return value.array();
}

[[nodiscard]] std::int64_t require_integer(const JsonValue& value, std::string_view name) {
    try {
        return value.integer();
    } catch (const std::exception&) {
        throw ProtocolError(std::string(name) + " must be an integer");
    }
}

[[nodiscard]] JsonValue repeated_integer_array(std::size_t count, std::int32_t value) {
    JsonValue::Array result;
    result.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        result.emplace_back(static_cast<std::int64_t>(value));
    }
    return JsonValue(std::move(result));
}

[[nodiscard]] std::int64_t deadline_seconds(
    const JsonValue& document,
    std::chrono::system_clock::time_point receivedAt,
    std::int32_t responseBudgetMs) {
    if (document.contains("endsAt")) {
        const std::int64_t raw = require_integer(document.at("endsAt"), "endsAt");
        if (raw < 0) {
            throw ProtocolError("endsAt cannot be negative");
        }
    }
    const std::int64_t receivedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        receivedAt.time_since_epoch()).count();
    return (receivedMs + std::max<std::int32_t>(1, responseBudgetMs)) / 1000;
}

[[nodiscard]] JsonValue normalize_agent(
    const MatchConfig& config,
    const JsonValue& document,
    bool ownAgent) {
    static_cast<void>(require_object(document, "BTC agent"));
    JsonValue::Object result;
    result.emplace("kind", document.at("kind"));
    result.emplace("pos", document.at("pos"));
    const std::int32_t kind = static_cast<std::int32_t>(require_integer(document.at("kind"), "agent kind"));
    if (document.contains("fuel") && !document.at("fuel").is_null()) {
        result.emplace("fuel", document.at("fuel"));
    } else if (!ownAgent || kind == static_cast<std::int32_t>(AgentKind::Tanker)) {
        result.emplace("fuel", JsonValue(static_cast<std::int64_t>(config.fuelLimit)));
    } else {
        throw ProtocolError("BTC omitted fuel for an own patrol agent");
    }
    return JsonValue(std::move(result));
}

}

MatchConfig parse_btc_setup(const JsonValue& document, BtcAdapterOptions options) {
    static_cast<void>(require_object(document, "BTC setup"));
    if (options.responseBudgetMs <= 0) {
        throw ProtocolError("BTC response budget must be positive");
    }
    const std::int32_t responseBudgetMs = static_cast<std::int32_t>(
        competition_compute_budget(
            std::chrono::milliseconds{options.responseBudgetMs}).count());
    const JsonValue::Array& daySteps = require_array(document.at("daySteps"), "BTC daySteps");
    JsonValue::Object normalized;
    normalized.emplace(
        "startsAt",
        document.contains("startsAt") ? document.at("startsAt") : JsonValue(std::int64_t{0}));
    normalized.emplace(
        "daySeconds",
        document.contains("daySeconds")
            ? document.at("daySeconds")
            : repeated_integer_array(
                  daySteps.size(),
                  std::max<std::int32_t>(1, responseBudgetMs / 1000)));
    normalized.emplace("daySteps", document.at("daySteps"));
    normalized.emplace("map", document.at("map"));
    normalized.emplace("agents", document.at("agents"));
    normalized.emplace("fuelLimits", document.at("fuelLimits"));
    normalized.emplace("players", document.at("players"));
    normalized.emplace("busyThreshold", document.at("busyThreshold"));
    normalized.emplace("jammedThreshold", document.at("jammedThreshold"));
    normalized.emplace("spots", document.at("spots"));
    return parse_match_config(JsonValue(std::move(normalized)));
}

DayState parse_btc_day_state(
    const MatchConfig& config,
    const JsonValue& document,
    std::chrono::system_clock::time_point receivedAt,
    BtcAdapterOptions options) {
    static_cast<void>(require_object(document, "BTC day state"));
    if (options.responseBudgetMs <= 0) {
        throw ProtocolError("BTC response budget must be positive");
    }
    const std::int32_t responseBudgetMs = static_cast<std::int32_t>(
        competition_compute_budget(
            std::chrono::milliseconds{options.responseBudgetMs}).count());
    const std::int64_t wireDay = require_integer(document.at("day"), "BTC day");
    if (wireDay < 0 || wireDay >= config.day_count()) {
        throw ProtocolError("BTC day is outside the configured zero-based range");
    }

    JsonValue::Object normalized;
    normalized.emplace("endsAt", JsonValue(deadline_seconds(document, receivedAt, responseBudgetMs)));
    normalized.emplace("day", JsonValue(wireDay + 1));

    const JsonValue::Array& ownAgents = require_array(document.at("agents"), "BTC own agents");
    JsonValue::Array normalizedOwn;
    normalizedOwn.reserve(ownAgents.size());
    for (const JsonValue& agent : ownAgents) {
        normalizedOwn.push_back(normalize_agent(config, agent, true));
    }
    normalized.emplace("agents", JsonValue(std::move(normalizedOwn)));

    JsonValue::Array normalizedOthers;
    if (document.contains("others")) {
        const JsonValue::Array& others = require_array(document.at("others"), "BTC other teams");
        normalizedOthers.reserve(others.size());
        for (std::size_t teamIndex = 0; teamIndex < others.size(); ++teamIndex) {
            const JsonValue& teamDocument = others.at(teamIndex);
            static_cast<void>(require_object(teamDocument, "BTC other team"));
            JsonValue::Object team;
            if (teamDocument.contains("id")) {
                team.emplace("id", teamDocument.at("id"));
            } else if (teamDocument.contains("teamId")) {
                team.emplace("id", teamDocument.at("teamId"));
            } else {
                team.emplace("id", JsonValue(static_cast<std::int64_t>(teamIndex)));
            }
            const JsonValue::Array& agents = require_array(teamDocument.at("agents"), "BTC other agents");
            JsonValue::Array normalizedAgents;
            normalizedAgents.reserve(agents.size());
            for (const JsonValue& agent : agents) {
                normalizedAgents.push_back(normalize_agent(config, agent, false));
            }
            team.emplace("agents", JsonValue(std::move(normalizedAgents)));
            normalizedOthers.emplace_back(std::move(team));
        }
    }
    normalized.emplace("others", JsonValue(std::move(normalizedOthers)));

    std::vector<RoadStatus> statuses(
        static_cast<std::size_t>(config.map.cell_count()),
        RoadStatus::Smooth);
    std::vector<bool> supplied(static_cast<std::size_t>(config.map.cell_count()), false);
    if (document.contains("traffics")) {
        const JsonValue::Array& traffics = require_array(document.at("traffics"), "BTC traffics");
        for (const JsonValue& traffic : traffics) {
            static_cast<void>(require_object(traffic, "BTC traffic"));
            const std::int64_t position = require_integer(traffic.at("pos"), "traffic pos");
            const std::int64_t status = require_integer(traffic.at("status"), "traffic status");
            if (position < 0 || position >= config.map.cell_count() ||
                config.map.terrain.at(static_cast<std::size_t>(position)) != Terrain::Road) {
                throw ProtocolError("BTC traffic points to a non-road cell");
            }
            if (status < 0 || status > 2) {
                throw ProtocolError("BTC traffic status is outside 0..2");
            }
            if (supplied.at(static_cast<std::size_t>(position))) {
                throw ProtocolError("BTC traffic position is duplicated");
            }
            supplied.at(static_cast<std::size_t>(position)) = true;
            statuses.at(static_cast<std::size_t>(position)) = static_cast<RoadStatus>(status);
        }
    }
    JsonValue::Array normalizedTraffics;
    normalizedTraffics.reserve(config.roadCells.size());
    for (const CellId road : config.roadCells) {
        JsonValue::Object traffic;
        traffic.emplace("pos", JsonValue(static_cast<std::int64_t>(road)));
        traffic.emplace(
            "status",
            JsonValue(static_cast<std::int64_t>(statuses.at(static_cast<std::size_t>(road)))));
        normalizedTraffics.emplace_back(std::move(traffic));
    }
    normalized.emplace("traffics", JsonValue(std::move(normalizedTraffics)));
    return parse_day_state(config, JsonValue(std::move(normalized)));
}

std::int64_t btc_authoritative_action_deadline_ms(
    const MatchConfig& config,
    const JsonValue& document,
    std::chrono::system_clock::time_point receivedAt,
    BtcAdapterOptions options) {
    static_cast<void>(require_object(document, "BTC day state"));
    if (options.responseBudgetMs <= 0) {
        throw ProtocolError("BTC response budget must be positive");
    }
    const std::int64_t wireDay = require_integer(document.at("day"), "BTC day");
    if (wireDay < 0 || wireDay >= config.day_count()) {
        throw ProtocolError("BTC day is outside the configured zero-based range");
    }
    const std::int64_t receivedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            receivedAt.time_since_epoch()).count();
    const std::int64_t fallbackDeadlineMs = receivedMs +
        competition_compute_budget(
            std::chrono::milliseconds{options.responseBudgetMs}).count();
    if (!document.contains("endsAt")) {
        return fallbackDeadlineMs;
    }
    const std::int64_t rawSeconds =
        require_integer(document.at("endsAt"), "endsAt");
    if (rawSeconds < 0) {
        throw ProtocolError("endsAt is outside the supported range");
    }
    constexpr std::int64_t kEpochMillisecondsThreshold = 100000000000LL;
    const std::int64_t rawDeadlineMs =
        rawSeconds >= kEpochMillisecondsThreshold
        ? rawSeconds
        : (rawSeconds <= std::numeric_limits<std::int64_t>::max() / 1000
               ? rawSeconds * 1000
               : throw ProtocolError("endsAt is outside the supported range"));
    const std::int64_t configuredWindowMs =
        static_cast<std::int64_t>(config.daySeconds.at(
            static_cast<std::size_t>(wireDay))) * 1000;
    return std::min(
        rawDeadlineMs,
        receivedMs + configuredWindowMs);
}

BtcFrameKind classify_btc_frame(const JsonValue& document) {
    if (!document.is_object()) {
        return BtcFrameKind::Unknown;
    }
    if (document.contains("map") && document.contains("daySteps") && document.contains("spots")) {
        return BtcFrameKind::Setup;
    }
    if (document.contains("day") && document.contains("agents") && document.contains("traffics")) {
        return BtcFrameKind::DayState;
    }
    if (document.contains("standings") || document.contains("ranking") || document.contains("winner")) {
        return BtcFrameKind::MatchResult;
    }
    if (document.contains("reason") || document.contains("valid") || document.contains("accepted") ||
        document.contains("success") || document.contains("ok")) {
        return BtcFrameKind::ActionResult;
    }
    return BtcFrameKind::Unknown;
}

bool btc_action_result_accepted(const JsonValue& document) {
    if (!document.is_object()) {
        return false;
    }
    if (document.contains("day") && !document.at("day").is_null()) {
        if (!document.at("day").is_number()) {
            return false;
        }
        try {
            const std::int64_t day = document.at("day").integer();
            if (day < std::numeric_limits<std::int32_t>::min() ||
                day > std::numeric_limits<std::int32_t>::max()) {
                return false;
            }
        } catch (const JsonError&) {
            return false;
        }
    }
    bool hasStatus = false;
    bool accepted = true;
    for (const std::string key : {"valid", "accepted", "success", "ok"}) {
        if (!document.contains(key)) {
            continue;
        }
        if (!document.at(key).is_bool()) {
            return false;
        }
        hasStatus = true;
        accepted = accepted && document.at(key).boolean();
    }
    bool hasReason = false;
    if (document.contains("reason") && !document.at("reason").is_null()) {
        if (!document.at("reason").is_string()) {
            return false;
        }
        hasReason = true;
        if (!document.at("reason").string().empty()) {
            return false;
        }
    }
    return hasStatus ? accepted : hasReason;
}

std::optional<std::int32_t> btc_action_result_day(const JsonValue& document) {
    if (!document.is_object()) {
        return std::nullopt;
    }
    static_cast<void>(require_object(document, "BTC action result"));
    if (!document.contains("day") || document.at("day").is_null()) {
        return std::nullopt;
    }
    if (!document.at("day").is_number()) {
        return std::nullopt;
    }
    std::int64_t day = 0;
    try {
        day = document.at("day").integer();
    } catch (const JsonError&) {
        return std::nullopt;
    }
    if (day < std::numeric_limits<std::int32_t>::min() ||
        day > std::numeric_limits<std::int32_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::int32_t>(day);
}

std::string btc_action_result_reason(const JsonValue& document) {
    if (!document.is_object() || !document.contains("reason") || document.at("reason").is_null()) {
        return {};
    }
    if (!document.at("reason").is_string()) {
        return "unstructured rejection";
    }
    return document.at("reason").string();
}

DayPlan make_wait_plan(const MatchConfig& config, std::int32_t dayNumber) {
    if (dayNumber < 1 || dayNumber > config.day_count()) {
        throw ProtocolError("cannot build WAIT plan for an invalid day");
    }
    DayPlan plan;
    plan.actions.resize(static_cast<std::size_t>(config.agent_count()));
    for (AgentPlan& actions : plan.actions) {
        actions.push_back(PlanAction::wait(config.steps_for_day(dayNumber)));
    }
    return plan;
}

}
