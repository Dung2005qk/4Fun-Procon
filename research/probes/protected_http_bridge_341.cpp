// Research transport validation only. No planner, role override or oracle.
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"
#include <iostream>
#include <string>
using J = udon::JsonValue;
J integers(const std::vector<std::int32_t>& values) {
    J::Array out;
    for (auto v : values) out.emplace_back(static_cast<std::int64_t>(v));
    return J(std::move(out));
}
J respond(const J& q) {
    const auto config = udon::parse_match_config(q.at("setup"));
    const auto op = q.at("op").string();
    if (op == "config") return J(J::Object{{"ok",J(true)}});
    if (op == "roles") {
        static_cast<void>(udon::parse_role_selection(config,q.at("roles")));
        return J(J::Object{{"ok",J(true)}});
    }
    if (op != "step") throw std::runtime_error("unknown protected bridge operation");
    const auto state = udon::parse_day_state(config,q.at("state"));
    const auto plan = udon::parse_day_plan(config,q.at("plan"));
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const auto a = simulator.simulate(state,plan,true);
    const auto b = validator.validate(state,plan,true);
    std::string mismatch;
    if (!validator.agrees_with(a,b,mismatch)) throw std::runtime_error("dual mismatch:"+mismatch);
    if (!a.valid) return J(J::Object{{"ok",J(false)},{"agrees",J(true)}});
    auto ledger = udon::parse_match_ledger(config,q.at("ledger"));
    ledger.apply(a.score);
    J::Array agents;
    for (const auto& x : a.finalAgents)
        agents.emplace_back(J::Object{{"kind",J(static_cast<std::int64_t>(x.kind))},
            {"pos",J(static_cast<std::int64_t>(x.position))},{"fuel",J(static_cast<std::int64_t>(x.fuel))}});
    return J(J::Object{{"ok",J(true)},{"agrees",J(true)},{"agents",J(std::move(agents))},
        {"ledger",udon::serialize_match_ledger(config,ledger)},
        {"score",integers({ledger.lifetime_distinct(),ledger.totalDailyDistinct,ledger.totalServings})},
        {"road_footprint",integers(a.roadFootprint)}});
}
int main() {
    std::string line;
    while (std::getline(std::cin,line)) {
        try { std::cout << respond(J::parse(line)).dump() << std::endl; }
        catch (const std::exception& e) {
            std::cout << J(J::Object{{"ok",J(false)},{"error",J(e.what())}}).dump() << std::endl;
        }
    }
}
