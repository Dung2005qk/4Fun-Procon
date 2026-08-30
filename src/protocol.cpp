#include "udon/protocol.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <sstream>
#include <string_view>

namespace udon {

namespace {

constexpr std::string_view kProtocolVersion = "hexudon-v1";

const JsonValue::Object& require_object(const JsonValue& value, const std::string& field);

void reject_unknown_keys(
    const JsonValue& value,
    const std::string& field,
    std::initializer_list<std::string_view> allowedKeys) {
    const JsonValue::Object& object = require_object(value, field);
    for (const auto& [key, ignored] : object) {
        static_cast<void>(ignored);
        if (std::find(allowedKeys.begin(), allowedKeys.end(), key) == allowedKeys.end()) {
            throw ProtocolError(field + " contains unsupported field: " + key);
        }
    }
}

void validate_protocol_version(const JsonValue& document, const std::string& field) {
    if (!document.contains("protocolVersion")) {
        return;
    }
    const JsonValue& version = document.at("protocolVersion");
    if (!version.is_string() || version.string() != kProtocolVersion) {
        throw ProtocolError(field + " uses an unsupported protocolVersion");
    }
}

[[nodiscard]] std::int32_t integer_in_range(
    const JsonValue& value,
    const std::string& field,
    std::int32_t minimum,
    std::int32_t maximum) {
    const std::int64_t parsed = value.integer();
    if (parsed < minimum || parsed > maximum) {
        throw ProtocolError(field + " is outside its allowed range");
    }
    return static_cast<std::int32_t>(parsed);
}

[[nodiscard]] std::int64_t integer_at_least(
    const JsonValue& value,
    const std::string& field,
    std::int64_t minimum) {
    const std::int64_t parsed = value.integer();
    if (parsed < minimum) {
        throw ProtocolError(field + " is smaller than allowed");
    }
    return parsed;
}

const JsonValue::Array& require_array(const JsonValue& value, const std::string& field) {
    if (!value.is_array()) {
        throw ProtocolError(field + " must be an array");
    }
    return value.array();
}

const JsonValue::Object& require_object(const JsonValue& value, const std::string& field) {
    if (!value.is_object()) {
        throw ProtocolError(field + " must be an object");
    }
    return value.object();
}

[[nodiscard]] Terrain parse_terrain(const JsonValue& value) {
    const std::int32_t raw = integer_in_range(value, "terrain", 0, 3);
    return static_cast<Terrain>(raw);
}

[[nodiscard]] RoadStatus parse_road_status(const JsonValue& value) {
    const std::int32_t raw = integer_in_range(value, "road status", 0, 2);
    return static_cast<RoadStatus>(raw);
}

[[nodiscard]] AgentKind parse_agent_kind(const JsonValue& value, const std::string& field) {
    const std::int32_t raw = integer_in_range(value, field, 0, 1);
    return static_cast<AgentKind>(raw);
}

[[nodiscard]] CellId parse_cell(
    const JsonValue& value,
    const GridMap& map,
    const std::string& field,
    bool rejectPond) {
    const CellId cell = integer_in_range(value, field, 0, map.cell_count() - 1);
    if (rejectPond && map.terrain.at(static_cast<std::size_t>(cell)) == Terrain::Pond) {
        throw ProtocolError(field + " points to a pond");
    }
    return cell;
}

void build_even_row_neighbors(GridMap& map) {
    map.neighbors.assign(static_cast<std::size_t>(map.cell_count()), {});
    for (std::int32_t rowIndex = 0; rowIndex < map.height; ++rowIndex) {
        const bool evenRow = (rowIndex % 2) == 0;
        const std::array<std::pair<std::int32_t, std::int32_t>, kDirectionCount> offsets = evenRow
            ? std::array<std::pair<std::int32_t, std::int32_t>, kDirectionCount>{
                  {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {0, -1}}}
            : std::array<std::pair<std::int32_t, std::int32_t>, kDirectionCount>{
                  {{-1, -1}, {-1, 0}, {0, 1}, {1, 0}, {1, -1}, {0, -1}}};
        for (std::int32_t columnIndex = 0; columnIndex < map.width; ++columnIndex) {
            const CellId source = rowIndex * map.width + columnIndex;
            std::array<CellId, kDirectionCount>& neighbors = map.neighbors.at(static_cast<std::size_t>(source));
            neighbors.fill(kInvalidCell);
            for (std::int32_t direction = 0; direction < kDirectionCount; ++direction) {
                const std::int32_t targetRow = rowIndex + offsets.at(static_cast<std::size_t>(direction)).first;
                const std::int32_t targetColumn = columnIndex + offsets.at(static_cast<std::size_t>(direction)).second;
                if (targetRow >= 0 && targetRow < map.height && targetColumn >= 0 && targetColumn < map.width) {
                    neighbors.at(static_cast<std::size_t>(direction)) = targetRow * map.width + targetColumn;
                }
            }
        }
    }
}

void verify_config_invariants(MatchConfig& config) {
    if (config.daySeconds.size() != config.daySteps.size() || config.daySteps.size() < 4U ||
        config.daySteps.size() > 10U) {
        throw ProtocolError("daySeconds and daySteps must contain the same 4 to 10 days");
    }
    for (std::size_t dayIndex = 0; dayIndex < config.daySteps.size(); ++dayIndex) {
        if (config.daySeconds.at(dayIndex) <= 0) {
            throw ProtocolError("daySeconds must be positive");
        }
        if (config.daySteps.at(dayIndex) <= 0) {
            throw ProtocolError("daySteps must be positive");
        }
    }
    if (config.agent_count() < 3 || config.agent_count() > kMaximumAgents) {
        throw ProtocolError("agent count must be between 3 and 8");
    }
    if (config.spots.empty()) {
        throw ProtocolError("a match must contain at least one spot");
    }
    if (config.fuelLimit <= 0 || config.players <= 0 || config.busyThreshold <= 0 ||
        config.jammedThreshold <= config.busyThreshold) {
        throw ProtocolError("invalid match-level numeric configuration");
    }
    if (config.brand_count() > kMaximumBrands) {
        throw ProtocolError("brand count exceeds the official map capacity");
    }
}

[[nodiscard]] AgentState parse_agent_state(const MatchConfig& config, const JsonValue& document) {
    reject_unknown_keys(document, "agent", {"kind", "pos", "fuel"});
    AgentState agent;
    agent.kind = parse_agent_kind(document.at("kind"), "agent kind");
    agent.position = parse_cell(document.at("pos"), config.map, "agent position", true);
    agent.fuel = integer_in_range(document.at("fuel"), "agent fuel", 0, config.fuelLimit);
    return agent;
}

} 

MatchConfig parse_match_config(const JsonValue& document) {
    reject_unknown_keys(
        document,
        "match config",
        {
            "protocolVersion",
            "startsAt",
            "daySeconds",
            "daySteps",
            "map",
            "agents",
            "fuelLimits",
            "players",
            "busyThreshold",
            "jammedThreshold",
            "spots",
        });
    validate_protocol_version(document, "match config");
    MatchConfig config;
    config.startsAt = integer_at_least(document.at("startsAt"), "startsAt", 0);

    const JsonValue::Array& daySeconds = require_array(document.at("daySeconds"), "daySeconds");
    const JsonValue::Array& daySteps = require_array(document.at("daySteps"), "daySteps");
    config.daySeconds.reserve(daySeconds.size());
    config.daySteps.reserve(daySteps.size());
    for (std::size_t dayIndex = 0; dayIndex < daySeconds.size(); ++dayIndex) {
        config.daySeconds.push_back(integer_in_range(
            daySeconds.at(dayIndex),
            "daySeconds entry",
            1,
            std::numeric_limits<std::int32_t>::max()));
    }
    for (std::size_t dayIndex = 0; dayIndex < daySteps.size(); ++dayIndex) {
        config.daySteps.push_back(integer_in_range(
            daySteps.at(dayIndex),
            "daySteps entry",
            1,
            std::numeric_limits<std::int32_t>::max()));
    }

    const JsonValue& mapDocument = document.at("map");
    reject_unknown_keys(mapDocument, "map", {"height", "width", "cells"});
    config.map.height = integer_in_range(mapDocument.at("height"), "map height", 8, kMaximumMapSide);
    config.map.width = integer_in_range(mapDocument.at("width"), "map width", 8, kMaximumMapSide);
    const JsonValue::Array& rows = require_array(mapDocument.at("cells"), "map cells");
    if (rows.size() != static_cast<std::size_t>(config.map.height)) {
        throw ProtocolError("map cells height does not match map height");
    }
    config.map.terrain.reserve(static_cast<std::size_t>(config.map.cell_count()));
    for (std::int32_t rowIndex = 0; rowIndex < config.map.height; ++rowIndex) {
        const JsonValue::Array& columns = require_array(rows.at(static_cast<std::size_t>(rowIndex)), "map row");
        if (columns.size() != static_cast<std::size_t>(config.map.width)) {
            throw ProtocolError("map row width does not match map width");
        }
        for (std::int32_t columnIndex = 0; columnIndex < config.map.width; ++columnIndex) {
            config.map.terrain.push_back(parse_terrain(columns.at(static_cast<std::size_t>(columnIndex))));
        }
    }
    build_even_row_neighbors(config.map);

    const JsonValue::Array& initialAgents = require_array(document.at("agents"), "agents");
    config.initialAgents.reserve(initialAgents.size());
    std::set<CellId> uniqueStarts;
    for (std::size_t agentIndex = 0; agentIndex < initialAgents.size(); ++agentIndex) {
        const CellId position = parse_cell(initialAgents.at(agentIndex), config.map, "initial agent position", true);
        if (config.map.terrain.at(static_cast<std::size_t>(position)) != Terrain::Plain) {
            throw ProtocolError("initial agent position must be on a plain cell");
        }
        if (!uniqueStarts.insert(position).second) {
            throw ProtocolError("initial agent positions must be distinct");
        }
        config.initialAgents.push_back(position);
    }

    config.fuelLimit = integer_in_range(document.at("fuelLimits"), "fuelLimits", 1, std::numeric_limits<std::int32_t>::max());
    config.players = integer_in_range(document.at("players"), "players", 1, std::numeric_limits<std::int32_t>::max());
    config.busyThreshold = integer_in_range(
        document.at("busyThreshold"),
        "busyThreshold",
        1,
        std::numeric_limits<std::int32_t>::max());
    config.jammedThreshold = integer_in_range(
        document.at("jammedThreshold"),
        "jammedThreshold",
        2,
        std::numeric_limits<std::int32_t>::max());

    config.spotAtCell.assign(static_cast<std::size_t>(config.map.cell_count()), kInvalidSpot);
    const JsonValue::Array& spots = require_array(document.at("spots"), "spots");
    config.spots.reserve(spots.size());
    for (std::size_t spotOffset = 0; spotOffset < spots.size(); ++spotOffset) {
        const JsonValue& spotDocument = spots.at(spotOffset);
        reject_unknown_keys(spotDocument, "spot", {"brand", "pos", "stocks"});
        Spot spot;
        spot.brandValue = integer_in_range(
            spotDocument.at("brand"),
            "spot brand",
            std::numeric_limits<std::int32_t>::min(),
            std::numeric_limits<std::int32_t>::max());
        spot.position = parse_cell(spotDocument.at("pos"), config.map, "spot position", true);
        spot.stock = integer_in_range(
            spotDocument.at("stocks"),
            "spot stocks",
            1,
            config.agent_count());
        if (config.map.terrain.at(static_cast<std::size_t>(spot.position)) != Terrain::Plain) {
            throw ProtocolError("spot position must be on a plain cell");
        }
        if (config.spotAtCell.at(static_cast<std::size_t>(spot.position)) != kInvalidSpot) {
            throw ProtocolError("only one spot may occupy a cell");
        }
        config.spotAtCell.at(static_cast<std::size_t>(spot.position)) = static_cast<SpotIndex>(config.spots.size());
        config.spots.push_back(spot);
        config.brandToIndex.emplace(spot.brandValue, 0);
    }
    for (const auto& [brandValue, ignored] : config.brandToIndex) {
        static_cast<void>(ignored);
        config.brandValues.push_back(brandValue);
    }
    for (std::size_t brandIndex = 0; brandIndex < config.brandValues.size(); ++brandIndex) {
        config.brandToIndex.at(config.brandValues.at(brandIndex)) = static_cast<std::int32_t>(brandIndex);
    }
    for (Spot& spot : config.spots) {
        spot.brandIndex = config.brandToIndex.at(spot.brandValue);
    }

    for (CellId cell = 0; cell < config.map.cell_count(); ++cell) {
        if (config.map.terrain.at(static_cast<std::size_t>(cell)) == Terrain::Road) {
            config.roadCells.push_back(cell);
        }
    }
    for (const CellId position : config.initialAgents) {
        if (config.spotAtCell.at(static_cast<std::size_t>(position)) != kInvalidSpot) {
            throw ProtocolError("initial agent positions cannot contain a spot");
        }
    }
    verify_config_invariants(config);
    return config;
}

DayState parse_day_state(const MatchConfig& config, const JsonValue& document) {
    reject_unknown_keys(
        document,
        "day state",
        {"protocolVersion", "endsAt", "day", "agents", "others", "traffics"});
    validate_protocol_version(document, "day state");
    DayState state;
    state.endsAt = integer_at_least(document.at("endsAt"), "endsAt", 0);
    state.dayNumber = integer_in_range(document.at("day"), "day", 1, config.day_count());

    const JsonValue::Array& agents = require_array(document.at("agents"), "day agents");
    if (agents.size() != static_cast<std::size_t>(config.agent_count())) {
        throw ProtocolError("day state agent count differs from match config");
    }
    state.agents.reserve(agents.size());
    for (const JsonValue& agentDocument : agents) {
        state.agents.push_back(parse_agent_state(config, agentDocument));
    }

    const JsonValue::Array& others = require_array(document.at("others"), "others");
    std::set<std::int32_t> otherIds;
    state.others.reserve(others.size());
    for (const JsonValue& otherDocument : others) {
        reject_unknown_keys(otherDocument, "other team", {"id", "agents"});
        OtherTeamState team;
        team.teamId = integer_in_range(
            otherDocument.at("id"),
            "other team id",
            0,
            std::numeric_limits<std::int32_t>::max());
        if (!otherIds.insert(team.teamId).second) {
            throw ProtocolError("other team ids must be unique");
        }
        const JsonValue::Array& otherAgents = require_array(otherDocument.at("agents"), "other team agents");
        if (otherAgents.size() != static_cast<std::size_t>(config.agent_count())) {
            throw ProtocolError("other team agent count differs from match config");
        }
        team.agents.reserve(otherAgents.size());
        for (const JsonValue& agentDocument : otherAgents) {
            team.agents.push_back(parse_agent_state(config, agentDocument));
        }
        state.others.push_back(std::move(team));
    }

    state.roadStatuses.assign(static_cast<std::size_t>(config.map.cell_count()), RoadStatus::Smooth);
    std::vector<bool> supplied(static_cast<std::size_t>(config.map.cell_count()), false);
    const JsonValue::Array& traffics = require_array(document.at("traffics"), "traffics");
    for (const JsonValue& trafficDocument : traffics) {
        reject_unknown_keys(trafficDocument, "traffic entry", {"pos", "status"});
        const CellId position = parse_cell(trafficDocument.at("pos"), config.map, "traffic position", false);
        if (config.map.terrain.at(static_cast<std::size_t>(position)) != Terrain::Road) {
            throw ProtocolError("traffic position must be a road cell");
        }
        if (supplied.at(static_cast<std::size_t>(position))) {
            throw ProtocolError("traffic position is duplicated");
        }
        supplied.at(static_cast<std::size_t>(position)) = true;
        state.roadStatuses.at(static_cast<std::size_t>(position)) = parse_road_status(trafficDocument.at("status"));
    }
    for (const CellId roadCell : config.roadCells) {
        if (!supplied.at(static_cast<std::size_t>(roadCell))) {
            throw ProtocolError("day state omits a road traffic status");
        }
    }
    return state;
}

std::vector<AgentKind> parse_role_selection(const MatchConfig& config, const JsonValue& document) {
    const JsonValue::Array& values = require_array(document, "role selection");
    if (values.size() != static_cast<std::size_t>(config.agent_count())) {
        throw ProtocolError("role selection length differs from agent count");
    }
    std::vector<AgentKind> result;
    result.reserve(values.size());
    for (const JsonValue& value : values) {
        result.push_back(parse_agent_kind(value, "role selection value"));
    }
    return result;
}

DayPlan parse_day_plan(const MatchConfig& config, const JsonValue& document) {
    const JsonValue::Array& agents = require_array(document, "day plan");
    if (agents.size() != static_cast<std::size_t>(config.agent_count())) {
        throw ProtocolError("day plan length differs from agent count");
    }
    DayPlan result;
    result.actions.reserve(agents.size());
    for (const JsonValue& agentActionsDocument : agents) {
        const JsonValue::Array& rawActions = require_array(agentActionsDocument, "agent action list");
        if (rawActions.empty()) {
            throw ProtocolError("agent action list cannot be empty");
        }
        AgentPlan actions;
        actions.reserve(rawActions.size());
        for (const JsonValue& rawActionDocument : rawActions) {
            const std::int64_t rawAction = rawActionDocument.integer();
            if (rawAction >= 0) {
                if (rawAction >= kDirectionCount) {
                    throw ProtocolError("move direction is outside 0..5");
                }
                actions.push_back(PlanAction::move(static_cast<std::int32_t>(rawAction)));
                continue;
            }
            if (rawAction == std::numeric_limits<std::int64_t>::min() || -rawAction > std::numeric_limits<std::int32_t>::max()) {
                throw ProtocolError("wait duration is too large");
            }
            actions.push_back(PlanAction::wait(static_cast<std::int32_t>(-rawAction)));
        }
        result.actions.push_back(std::move(actions));
    }
    return result;
}

MatchLedger parse_match_ledger(const MatchConfig& config, const JsonValue& document) {
    reject_unknown_keys(
        document,
        "match ledger",
        {"protocolVersion", "brands", "totalDailyDistinct", "totalServings"});
    validate_protocol_version(document, "match ledger");
    const JsonValue::Array& brands = require_array(document.at("brands"), "ledger brands");
    MatchLedger ledger;
    std::set<std::int32_t> seenBrands;
    for (const JsonValue& value : brands) {
        const std::int32_t brandValue = integer_in_range(
            value,
            "ledger brand",
            std::numeric_limits<std::int32_t>::min(),
            std::numeric_limits<std::int32_t>::max());
        const auto iterator = config.brandToIndex.find(brandValue);
        if (iterator == config.brandToIndex.end()) {
            throw ProtocolError("ledger contains a brand absent from the match configuration");
        }
        if (!seenBrands.insert(brandValue).second) {
            throw ProtocolError("ledger lists a brand more than once");
        }
        ledger.lifetimeBrands |= brand_bit(iterator->second);
    }
    ledger.totalDailyDistinct = integer_in_range(
        document.at("totalDailyDistinct"),
        "ledger totalDailyDistinct",
        0,
        std::numeric_limits<std::int32_t>::max());
    ledger.totalServings = integer_in_range(
        document.at("totalServings"),
        "ledger totalServings",
        0,
        std::numeric_limits<std::int32_t>::max());
    return ledger;
}

JsonValue serialize_role_selection(const std::vector<AgentKind>& roles) {
    JsonValue::Array values;
    values.reserve(roles.size());
    for (const AgentKind role : roles) {
        values.emplace_back(static_cast<std::int64_t>(role));
    }
    return JsonValue(std::move(values));
}

JsonValue serialize_day_plan(const DayPlan& plan) {
    JsonValue::Array agents;
    agents.reserve(plan.actions.size());
    for (const AgentPlan& agentActions : plan.actions) {
        JsonValue::Array rawActions;
        rawActions.reserve(agentActions.size());
        for (const PlanAction& action : agentActions) {
            rawActions.emplace_back(static_cast<std::int64_t>(action.wire_value()));
        }
        agents.emplace_back(std::move(rawActions));
    }
    return JsonValue(std::move(agents));
}

JsonValue serialize_match_ledger(const MatchConfig& config, const MatchLedger& ledger) {
    JsonValue::Array brands;
    for (const auto& [brandValue, brandIndex] : config.brandToIndex) {
        if (has_brand(ledger.lifetimeBrands, brandIndex)) {
            brands.emplace_back(static_cast<std::int64_t>(brandValue));
        }
    }
    JsonValue::Object object;
    object.emplace("brands", JsonValue(std::move(brands)));
    object.emplace("totalDailyDistinct", JsonValue(static_cast<std::int64_t>(ledger.totalDailyDistinct)));
    object.emplace("totalServings", JsonValue(static_cast<std::int64_t>(ledger.totalServings)));
    return JsonValue(std::move(object));
}

std::string canonical_plan_bytes(const DayPlan& plan) {
    return serialize_day_plan(plan).dump();
}

}
