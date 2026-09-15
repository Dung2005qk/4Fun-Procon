// Research-only caller of the unchanged production F0/W1 implementation.
// Inputs are recorded HTTP audit plans; no oracle plan or suffix enters here.
#include <chrono>
#include <iostream>
#include <stdexcept>
#include "udon/decision.hpp"
#include "udon/protocol.hpp"

using J = udon::JsonValue;
J score_json(const udon::OfficialScore& score) {
    return J(J::Array{J(static_cast<std::int64_t>(score.lifetimeDistinct)),
        J(static_cast<std::int64_t>(score.totalDailyDistinct)),
        J(static_cast<std::int64_t>(score.totalServings))});
}
J run(const J& request) {
    const auto config = udon::parse_match_config(request.at("setup"));
    const auto state = udon::parse_day_state(config, request.at("state"));
    const auto ledger = udon::parse_match_ledger(config, request.at("ledger"));
    const auto plan = udon::parse_day_plan(config, request.at("plan"));
    if (state.dayNumber != 1 || config.day_count() != 4 || !config.roadCells.empty() ||
        config.agent_count() != 3 || !state.others.empty())
        throw std::runtime_error("outside frozen316 attribution domain");
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::ParetoRouter router(config);
    const udon::RouteColumnGenerator generator(config, router);
    const udon::RouteMaster master(config, simulator, validator);
    const auto candidate = master.evaluate_exact_plan(state, ledger, plan);
    if (!candidate.has_value()) throw std::runtime_error("invalid recorded candidate");
    udon::TrafficBelief belief(config);
    belief.observe(state);
    const auto manifest = udon::ScenarioGenerator(config).freeze_manifest(state, belief);
    if (manifest.scenarios.size() != 1 || manifest.scenarios.front().scenarioClass != "deterministic-no-road" ||
        manifest.scenarios.front().weight != 10000 || manifest.scenarios.front().pessimisticFallback)
        throw std::runtime_error("scenario identity mismatch");
    const auto& bound = request.at("upper").array();
    const udon::OfficialScore upper{static_cast<int>(bound.at(0).integer()),
        static_cast<int>(bound.at(1).integer()), static_cast<int>(bound.at(2).integer())};
    const udon::FutureWitnessRepairer repairer(config, generator, master, simulator, validator, 7);
    auto profile = repairer.provisional_profile(*candidate, state, ledger, belief, manifest, upper, 24);
    const auto f0 = profile.outcomes.at(0).score;
    const auto started = std::chrono::steady_clock::now();
    repairer.repair_profile(profile, *candidate, state, ledger, belief, manifest, 200,
                           started + std::chrono::milliseconds(1000));
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - started).count();
    const auto& witness = profile.outcomes.at(0).witness;
    if (!witness.certified || witness.futurePlans.size() != 3)
        throw std::runtime_error("missing complete certified suffix");
    auto nextLedger = ledger;
    nextLedger.apply(candidate->simulation.score);
    auto nextState = state;
    nextState.agents = candidate->simulation.finalAgents;
    J::Array plans;
    for (const auto& suffixPlan : witness.futurePlans) {
        ++nextState.dayNumber;
        const auto result = simulator.simulate(nextState, suffixPlan, true);
        const auto independent = validator.validate(nextState, suffixPlan, true);
        std::string mismatch;
        if (!result.valid || !validator.agrees_with(result, independent, mismatch))
            throw std::runtime_error("suffix failed dual validation: " + mismatch);
        nextLedger.apply(result.score);
        nextState.agents = result.finalAgents;
        plans.push_back(udon::serialize_day_plan(suffixPlan));
    }
    const udon::OfficialScore replayed{nextLedger.lifetime_distinct(), nextLedger.totalDailyDistinct,
                                       nextLedger.totalServings};
    if ((!witness.lowerBoundOnly && !(replayed == witness.score)) || replayed < witness.score)
        throw std::runtime_error("certified suffix score mismatch");
    J::Array currentAgents;
    for (const auto& agent : candidate->simulation.finalAgents)
        currentAgents.emplace_back(J::Object{{"kind", J(static_cast<std::int64_t>(agent.kind))},
            {"pos", J(static_cast<std::int64_t>(agent.position))},
            {"fuel", J(static_cast<std::int64_t>(agent.fuel))}});
    auto currentLedger = ledger;
    currentLedger.apply(candidate->simulation.score);
    return J(J::Object{{"ok", J(true)}, {"f0_score", score_json(f0)},
        {"current_agents", J(std::move(currentAgents))},
        {"current_ledger", udon::serialize_match_ledger(config, currentLedger)},
        {"current_score", score_json(candidate->scoreAfterToday)},
        {"w1_score", score_json(witness.score)}, {"replayed_score", score_json(replayed)},
        {"lower_bound_only", J(witness.lowerBoundOnly)}, {"certified", J(witness.certified)},
        {"future_plans", J(std::move(plans))}, {"dual_valid_days", J(std::int64_t(3))},
        {"w1_elapsed_us_diagnostic_only", J(static_cast<std::int64_t>(elapsed))}});
}
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        try { std::cout << run(J::parse(line)).dump() << std::endl; }
        catch (const std::exception& e) {
            std::cout << J(J::Object{{"ok", J(false)}, {"error", J(e.what())}}).dump() << std::endl;
        }
    }
}
