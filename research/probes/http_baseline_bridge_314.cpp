// Research transport only. Reuse the exact frozen fixture generator without
// editing it or invoking its solver/oracle main. No planning occurs here.
#define main frozen_oracle_entry_not_called_314
#include "multi_patrol_oracle.cpp"
#undef main

namespace {
using J314 = udon::JsonValue;
J314 number314(std::int64_t value) { return J314(value); }
J314 array314(const std::vector<std::int32_t>& values) {
    J314::Array out;
    for (const auto value : values) out.push_back(number314(value));
    return J314(std::move(out));
}
J314 setup314(const udon::MatchConfig& c) {
    J314::Array cells;
    for (int row = 0; row < c.map.height; ++row) {
        J314::Array line;
        for (int col = 0; col < c.map.width; ++col)
            line.push_back(number314(static_cast<int>(c.map.terrain.at(row * c.map.width + col))));
        cells.emplace_back(std::move(line));
    }
    J314::Array spots;
    for (const auto& spot : c.spots)
        spots.emplace_back(J314::Object{{"brand", number314(spot.brandValue)},
            {"pos", number314(spot.position)}, {"stocks", number314(spot.stock)}});
    return J314(J314::Object{{"startsAt", number314(c.startsAt)},
        {"daySeconds", array314(c.daySeconds)}, {"daySteps", array314(c.daySteps)},
        {"map", J314(J314::Object{{"width", number314(c.map.width)},
            {"height", number314(c.map.height)}, {"cells", J314(std::move(cells))}})},
        {"spots", J314(std::move(spots))}, {"agents", array314(c.initialAgents)},
        {"fuelLimits", number314(c.fuelLimit)}, {"players", number314(c.players)},
        {"busyThreshold", number314(c.busyThreshold)}, {"jammedThreshold", number314(c.jammedThreshold)}});
}
J314 respond314(const J314& request) {
    if (request.at("op").string() == "fixture") {
        ManifestRow row;
        row.experimentId = "CEILING-THREE-ACTIVE-PATROL-LOW-FUEL-PREVALENCE-312";
        row.family = request.at("family").string();
        row.fuelProfile = "low";
        row.horizon = 4;
        row.activeAgents = 3;
        row.totalAgents = 3;
        row.spotCount = 5;
        row.players = static_cast<int>(request.at("players").integer());
        const auto fixture = make_fixture(row, static_cast<std::uint64_t>(request.at("seed").integer()));
        const auto document = setup314(fixture.config);
        const auto roundtrip = udon::parse_match_config(document);
        if (setup314(roundtrip).dump() != document.dump() ||
            fixture.config.map.neighbors != roundtrip.map.neighbors)
            throw std::runtime_error("fixture roundtrip mismatch");
        return J314(J314::Object{{"ok", J314(true)}, {"setup", document}});
    }
    if (request.at("op").string() != "step") throw std::runtime_error("unknown bridge operation");
    const auto config = udon::parse_match_config(request.at("setup"));
    const auto state = udon::parse_day_state(config, request.at("state"));
    const auto plan = udon::parse_day_plan(config, request.at("plan"));
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const auto simulation = simulator.simulate(state, plan, true);
    const auto validation = validator.validate(state, plan, true);
    std::string mismatch;
    const bool agrees = validator.agrees_with(simulation, validation, mismatch);
    if (!agrees) throw std::runtime_error("dual-validator disagreement: " + mismatch);
    if (!simulation.valid)
        return J314(J314::Object{{"ok", J314(false)}, {"agrees", J314(true)},
            {"error", J314(simulation.error->message)}});
    auto ledger = udon::parse_match_ledger(config, request.at("ledger"));
    ledger.apply(simulation.score);
    J314::Array agents;
    for (const auto& a : simulation.finalAgents)
        agents.emplace_back(J314::Object{{"kind", number314(static_cast<int>(a.kind))},
            {"pos", number314(a.position)}, {"fuel", number314(a.fuel)}});
    return J314(J314::Object{{"ok", J314(true)}, {"agrees", J314(true)},
        {"agents", J314(std::move(agents))},
        {"ledger", udon::serialize_match_ledger(config, ledger)},
        {"score", array314({ledger.lifetime_distinct(), ledger.totalDailyDistinct, ledger.totalServings})}});
}
}
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        try { std::cout << respond314(J314::parse(line)).dump() << std::endl; }
        catch (const std::exception& error) {
            std::cout << J314(J314::Object{{"ok", J314(false)}, {"error", J314(error.what())}}).dump() << std::endl;
        }
    }
}
