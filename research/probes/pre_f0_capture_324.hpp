// Research-only observation. Never included by canonical production sources.
#pragma once
#include "udon/planner.hpp"
#include "udon/protocol.hpp"
#include <map>
#include <stdexcept>

namespace udon::capture324 {
struct Capture {
    bool active = false;
    DayState root;
    MatchLedger ledger;
    std::map<std::string, RoutePortfolio> portfolios;
    std::map<std::string, std::vector<MasterCandidate>> pools;
};
inline thread_local Capture captured;
inline void begin(const DayState& state, const MatchLedger& ledger) {
    captured = Capture{};
    if (state.dayNumber == 2) {
        captured.active = true;
        captured.root = state;
        captured.ledger = ledger;
    }
}
inline void portfolio(const char* label, const RoutePortfolio& value) {
    if (captured.active && !captured.portfolios.emplace(label, value).second)
        throw std::logic_error("duplicate research portfolio capture");
}
inline void pool(const char* label, const std::vector<MasterCandidate>& value) {
    if (captured.active && !captured.pools.emplace(label, value).second)
        throw std::logic_error("duplicate research pool capture");
}
inline JsonValue number(std::int64_t v) { return JsonValue(v); }
inline JsonValue agents(const std::vector<AgentState>& values) {
    JsonValue::Array out;
    for (const auto& a : values) out.emplace_back(JsonValue::Object{
        {"kind", number(static_cast<int>(a.kind))}, {"pos", number(a.position)}, {"fuel", number(a.fuel)}});
    return JsonValue(std::move(out));
}
inline JsonValue claims(const std::vector<ClaimEvent>& values) {
    JsonValue::Array out;
    for (const auto& c : values) out.emplace_back(JsonValue::Object{
        {"agent", number(c.agent)}, {"spot", number(c.spot)}, {"step", number(c.step)}, {"served", JsonValue(c.served)}});
    return JsonValue(std::move(out));
}
inline JsonValue score(const OfficialScore& s) {
    return JsonValue(JsonValue::Array{number(s.lifetimeDistinct),number(s.totalDailyDistinct),number(s.totalServings)});
}
// Called only by replay serialization AFTER solve_day has returned.
inline JsonValue serialize(const MatchConfig& config, const DayState& state, const MatchLedger& ledger) {
    if (!captured.active || state.dayNumber != 2) return JsonValue(JsonValue::Object{{"active", JsonValue(false)}});
    if (captured.root.dayNumber != state.dayNumber || captured.root.endsAt != state.endsAt ||
        agents(captured.root.agents).dump() != agents(state.agents).dump() ||
        serialize_match_ledger(config, captured.ledger).dump() != serialize_match_ledger(config, ledger).dump())
        throw std::logic_error("research capture root drift");
    JsonValue::Object portfolios, pools;
    for (const auto& [label, value] : captured.portfolios) {
        JsonValue::Array byAgent;
        for (const auto& columns : value.columnsByAgent) {
            JsonValue::Array output;
            for (const auto& c : columns) {
                JsonValue::Array visits;
                for (const auto& v : c.firstVisits) visits.emplace_back(JsonValue::Object{
                    {"spot", number(v.spot)}, {"step", number(v.step)}, {"claimed", JsonValue(v.claimedServing)},
                    {"credited", JsonValue(v.creditedServing)}, {"brandIndex", number(v.brandIndex)}});
                DayPlan p; p.actions.push_back(c.actions);
                output.emplace_back(JsonValue::Object{{"agent", number(c.agent)}, {"columnId", number(c.columnId)},
                    {"plan", serialize_day_plan(p)}, {"terminalCell", number(c.terminalCell)},
                    {"terminalFuel", number(c.terminalFuel)}, {"firstVisits", JsonValue(std::move(visits))},
                    {"estimatedServings", number(c.estimatedServings)}, {"hasExactTimeline", JsonValue(c.hasExactTimeline)},
                    {"harvestExtension", JsonValue(c.harvestExtension)}, {"exactOrienteering", JsonValue(c.exactOrienteering)},
                    {"contingencyBundle", number(c.contingencyBundle)}, {"priority", number(c.priority)}});
            }
            byAgent.emplace_back(std::move(output));
        }
        portfolios.emplace(label, JsonValue(std::move(byAgent)));
    }
    for (const auto& [label, value] : captured.pools) {
        JsonValue::Array output;
        for (const auto& c : value) output.emplace_back(JsonValue::Object{
            {"stableId", JsonValue(c.stableId)}, {"plan", serialize_day_plan(c.plan)},
            {"score", score(c.scoreAfterToday)}, {"valid", JsonValue(c.simulation.valid)},
            {"agents", agents(c.simulation.finalAgents)}, {"claims", claims(c.simulation.claims)}});
        pools.emplace(label, JsonValue(std::move(output)));
    }
    return JsonValue(JsonValue::Object{{"active", JsonValue(true)}, {"day", number(state.dayNumber)},
        {"agents", agents(captured.root.agents)}, {"ledger", serialize_match_ledger(config, captured.ledger)},
        {"portfolios", JsonValue(std::move(portfolios))}, {"pools", JsonValue(std::move(pools))}});
}
}
