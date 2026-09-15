// Offline claims only; unchanged canonical simulator and independent validator.
#include "pre_f0_capture_324.hpp"
#include <iostream>
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            const auto request = udon::JsonValue::parse(line);
            if (request.at("op").string() != "step") throw std::runtime_error("offline step operation required");
            const auto config = udon::parse_match_config(request.at("setup"));
            const auto state = udon::parse_day_state(config, request.at("state"));
            const auto plan = udon::parse_day_plan(config, request.at("plan"));
            const auto simulation = udon::ExactStepSimulator(config).simulate(state, plan, true);
            const udon::IndependentDayValidator validator(config);
            const auto validation = validator.validate(state, plan, true);
            std::string mismatch;
            if (!simulation.valid || !validator.agrees_with(simulation, validation, mismatch))
                throw std::runtime_error("claims validation mismatch: " + mismatch);
            auto ledger = udon::parse_match_ledger(config, request.at("ledger"));
            ledger.apply(simulation.score);
            auto object = udon::JsonValue::Object{{"ok", udon::JsonValue(true)}, {"agrees", udon::JsonValue(true)},
                {"agents", udon::capture324::agents(simulation.finalAgents)},
                {"ledger", udon::serialize_match_ledger(config, ledger)},
                {"score", udon::capture324::score(udon::OfficialScore{
                    ledger.lifetime_distinct(), ledger.totalDailyDistinct, ledger.totalServings})}};
            object.emplace("claims", udon::capture324::claims(simulation.claims));
            std::cout << udon::JsonValue(std::move(object)).dump() << std::endl;
        } catch (const std::exception& e) {
            std::cout << udon::JsonValue(udon::JsonValue::Object{
                {"ok", udon::JsonValue(false)}, {"error", udon::JsonValue(e.what())}}).dump() << std::endl;
        }
    }
}
