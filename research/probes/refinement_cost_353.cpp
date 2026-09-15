// Research-only direct phase call. Link unchanged frozen352 libraries.
#include "udon/protocol.hpp"
#include "udon/slack_refiner.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
using J=udon::JsonValue;
using Clock=std::chrono::steady_clock;
J n(std::int64_t x){return J(x);}
J score(const udon::OfficialScore& s){return J(J::Array{n(s.lifetimeDistinct),n(s.totalDailyDistinct),n(s.totalServings)});}
double ms(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::milli>(b-a).count();}
J run(const J& q){
    const auto config=udon::parse_match_config(q.at("config"));
    const auto state=udon::parse_day_state(config,q.at("state"));
    const auto ledger=udon::parse_match_ledger(config,q.at("ledger"));
    const auto plan=udon::parse_day_plan(config,q.at("plan"));
    udon::ExactStepSimulator simulator(config);
    udon::IndependentDayValidator validator(config);
    const auto base=simulator.simulate(state,plan,false);
    std::string mismatch;
    if(!base.valid || !validator.agrees_with(base,validator.validate(state,plan,false),mismatch))
        throw std::runtime_error("invalid base: "+mismatch);
    if(state.dayNumber!=1 || config.day_count()!=4)throw std::runtime_error("wrong snapshot");
    udon::ProtectedSlackRefiner refiner(config);
    // Exact run_http checkpoint flags; terminal-only flags do not act on day1.
    refiner.enableTerminalPairExchange=true;
    refiner.enableMiddayChainAdoption=true;
    refiner.enableMiddayTargetTerminalFollowup=true;
    const auto started=Clock::now();
    const auto deadline=started+std::chrono::milliseconds(q.at("budget_ms").integer());
    const auto wait=refiner.refine_wait_detours(state,ledger,plan,base,deadline);
    const auto waitEnd=Clock::now();
    const auto midday=refiner.refine_midday_chains(state,ledger,wait.plan,wait.simulation,deadline);
    const auto end=Clock::now();
    const auto exact=simulator.simulate(state,midday.plan,false);
    auto before=ledger;before.apply(base.score);
    auto after=ledger;after.apply(exact.score);
    if(!exact.valid || !validator.agrees_with(exact,validator.validate(state,midday.plan,false),mismatch)
       || !validator.agrees_with(midday.simulation,exact,mismatch)
       || !udon::protected_slack_transition_dominates(base,exact)
       || !udon::protected_slack_ledger_dominates(before,after)
       || !(udon::OfficialScore::after_day(ledger,exact.score)==midday.scoreAfterToday))
        throw std::runtime_error("certificate failure: "+mismatch);
    const auto& d=midday.diagnostics;
    if(d.middayFailure || d.sparseFailure)throw std::runtime_error("phase failure");
    return J(J::Object{{"ok",J(true)},{"budget_ms",q.at("budget_ms")},
        {"input_echo",q},{"base_score",score(udon::OfficialScore::after_day(ledger,base.score))},
        {"wait_score",score(wait.scoreAfterToday)},{"score",score(midday.scoreAfterToday)},
        {"plan",udon::serialize_day_plan(midday.plan)},
        {"ledger",udon::serialize_match_ledger(config,after)},
        {"first_round",score(midday.firstRoundScore)},
        {"wait_ms",J(ms(started,waitEnd))},{"midday_ms",J(ms(waitEnd,end))},
        {"total_ms",J(ms(started,end))},{"deadline",J(d.deadlineReached)},
        {"wait_deadline",J(wait.diagnostics.deadlineReached)},
        {"routes",n(d.middayRoutes)},{"plans",n(d.middayGeneratedPlans)},
        {"valid",n(d.middayValidPlans)},{"acceptances",n(d.middayChainAcceptances)},
        {"target_routes",n(d.middayTargetRoutes)},{"target_plans",n(d.middayTargetGeneratedPlans)},
        {"target_acceptances",n(d.middayTargetAcceptances)},
        {"target_followup",J(d.middayTargetFollowup)},{"zero_safety_failure",J(true)}});
}
int main(){
    try{std::string line;std::getline(std::cin,line);std::cout<<run(J::parse(line)).dump()<<'\n';}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
