#include "observer355.hpp"
#include "udon/protocol.hpp"
#include "udon/slack_refiner.hpp"
#include <iostream>
using J=udon::JsonValue;
J n(std::int64_t x){return J(x);}
J score(const udon::OfficialScore& s){return J(J::Array{n(s.lifetimeDistinct),n(s.totalDailyDistinct),n(s.totalServings)});}
int main(){try{
    std::string line;std::getline(std::cin,line);const auto q=J::parse(line);
    const auto config=udon::parse_match_config(q.at("config"));
    const auto state=udon::parse_day_state(config,q.at("state"));
    const auto ledger=udon::parse_match_ledger(config,q.at("ledger"));
    const auto basePlan=udon::parse_day_plan(config,q.at("plan64"));
    const auto goal=udon::parse_day_plan(config,q.at("plan65"));
    udon::ExactStepSimulator sim(config);udon::IndependentDayValidator val(config);
    const auto base=sim.simulate(state,basePlan,false),goalSim=sim.simulate(state,goal,true);
    std::uint32_t goalMask=0;for(const auto& c:goalSim.claims)if(c.agent==5)goalMask|=1U<<c.spot;
    const auto pool=udon::enumerate_sparse_anytime_resource_routes_to_terminal(config,state,5,base.finalAgents.at(5).position,5,32,1250000,std::chrono::steady_clock::now()+std::chrono::seconds(30),{});
    const auto routeJson=[&](const udon::ExactOrienteeringRoute& r){udon::DayPlan p;p.actions={r.actions};return J(J::Object{
        {"mask",n(r.spotMask)},{"steps",n(r.usedSteps)},{"fuel",n(r.patrolFuel)},{"terminal",n(r.terminalCell)},
        {"distance",n(r.terminalBrandDistance)},{"on_spot",J(r.terminalOnSpot)},{"plan",udon::serialize_day_plan(p)}});};
    J::Array retained;for(const auto* v:{&pool.maximalRoutes,&pool.supplementalRoutes})for(const auto& r:*v)retained.push_back(routeJson(r));
    J::Array emitted;
    for(const auto& o:udon::observed355){auto p=basePlan;p.actions.at(5)=o.route.actions;const auto x=sim.simulate(state,p,false);std::string why;
        if(!val.agrees_with(x,val.validate(state,p,false),why))throw std::runtime_error("validator mismatch:"+why);
        auto before=ledger,after=ledger;before.apply(base.score);after.apply(x.score);
        auto r=routeJson(o.route);J::Array rank;for(auto v:o.rank)rank.push_back(n(v));r.object().emplace("rank",J(std::move(rank)));
        r.object().emplace("valid",J(x.valid));r.object().emplace("score",score(udon::OfficialScore::after_day(ledger,x.score)));
        r.object().emplace("safe",J(x.valid && udon::protected_slack_transition_dominates(base,x) && udon::protected_slack_ledger_dominates(before,after)));
        auto g=goal;g.actions.at(5)=o.route.actions;
        r.object().emplace("equals_witness",J(udon::canonical_plan_bytes(g)==udon::canonical_plan_bytes(goal)));
        emitted.push_back(std::move(r));}
    std::cout<<J(J::Object{{"ok",J(true)},{"input_echo",q},{"goal_mask",n(goalMask)},
        {"supported",J(pool.supported)},{"complete",J(pool.complete)},{"settled",n(pool.settledStates)},
        {"retained",J(std::move(retained))},{"emitted",J(std::move(emitted))},{"zero_validator_mismatch",J(true)}}).dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
