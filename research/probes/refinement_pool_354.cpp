// Fixed development attribution only; unchanged frozen library, no live plans.
#include "udon/protocol.hpp"
#include "udon/slack_refiner.hpp"
#include "udon/orienteering.hpp"
#include <chrono>
#include <iostream>
using J=udon::JsonValue;
J n(std::int64_t x){return J(x);}
J score(const udon::OfficialScore& s){return J(J::Array{n(s.lifetimeDistinct),n(s.totalDailyDistinct),n(s.totalServings)});}
J agents(const udon::SimulationResult& r){J::Array a;for(const auto& x:r.finalAgents)a.emplace_back(J::Object{{"pos",n(x.position)},{"fuel",n(x.fuel)},{"kind",n(static_cast<int>(x.kind))}});return J(std::move(a));}
int main(){try{
    std::string line;std::getline(std::cin,line);const J q=J::parse(line);
    const auto config=udon::parse_match_config(q.at("config"));
    const auto state=udon::parse_day_state(config,q.at("state"));
    const auto ledger=udon::parse_match_ledger(config,q.at("ledger"));
    const auto original=udon::parse_day_plan(config,q.at("plan"));
    const auto a=udon::parse_day_plan(config,q.at("plan64"));
    const auto b=udon::parse_day_plan(config,q.at("plan65"));
    udon::ExactStepSimulator sim(config);udon::IndependentDayValidator val(config);
    const auto checked=[&](const udon::DayPlan& p){auto x=sim.simulate(state,p,false);std::string why;
        if(!val.agrees_with(x,val.validate(state,p,false),why))throw std::runtime_error("dual mismatch:"+why);return x;};
    const auto base=checked(original),aSim=checked(a),bSim=checked(b);
    if(config.agent_count()!=8 || !base.valid || !aSim.valid || !bSim.valid ||
       base.score.servings!=55 || aSim.score.servings!=64 || bSim.score.servings!=65)throw std::runtime_error("snapshot/endpoints mismatch");
    const auto dominates=[&](const udon::SimulationResult& x,const udon::SimulationResult& y){auto lx=ledger,ly=ledger;lx.apply(x.score);ly.apply(y.score);
        return y.valid && udon::protected_slack_transition_dominates(x,y) && udon::protected_slack_ledger_dominates(lx,ly);};
    const auto audit=[&](const udon::DayPlan& p){auto x=checked(p);return J(J::Object{{"valid",J(x.valid)},
        {"score",score(udon::OfficialScore::after_day(ledger,x.score))},{"dominates55",J(dominates(base,x))},
        {"dominates64",J(dominates(aSim,x))},{"agents",agents(x)}});};
    J::Array mixes;
    for(unsigned mask=0;mask<256;++mask){auto p=a;for(unsigned i=0;i<8;++i)if(mask&(1U<<i))p.actions[i]=b.actions[i];
        auto r=audit(p);r.object().emplace("mask",n(mask));mixes.push_back(std::move(r));}
    J::Array pools;
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
    for(udon::AgentIndex i=0;i<8;++i){
        const auto pool=udon::enumerate_sparse_anytime_resource_routes_to_terminal(config,state,i,base.finalAgents[i].position,5,32,1250000,deadline,{});
        J::Array routes;
        for(const auto* v:{&pool.maximalRoutes,&pool.supplementalRoutes})for(const auto& route:*v){auto p=a;p.actions[i]=route.actions;
            auto r=audit(p);r.object().emplace("route",udon::serialize_day_plan(p).array().at(i));
            auto pb=b;pb.actions[i]=route.actions;
            r.object().emplace("equals64component",J(udon::canonical_plan_bytes(p)==udon::canonical_plan_bytes(a)));
            r.object().emplace("equals65component",J(udon::canonical_plan_bytes(pb)==udon::canonical_plan_bytes(b)));
            r.object().emplace("mask",n(route.spotMask));routes.push_back(std::move(r));}
        pools.emplace_back(J::Object{{"agent",n(i)},{"supported",J(pool.supported)},
            {"complete",J(pool.complete)},{"settled",n(pool.settledStates)},{"routes",J(std::move(routes))}});
    }
    J::Object report{{"ok",J(true)},{"input_echo",q},{"mixtures",J(std::move(mixes))},
        {"pools",J(std::move(pools))},{"base_agents",agents(base)},{"a_agents",agents(aSim)},{"b_agents",agents(bSim)},
        {"deadline",J(std::chrono::steady_clock::now()>=deadline)},{"zero_validator_mismatch",J(true)}};
    std::cout<<J(std::move(report)).dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
