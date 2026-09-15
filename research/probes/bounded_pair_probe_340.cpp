#include "udon/orienteering.hpp"
#include "udon/protocol.hpp"
#ifndef LEGACY_ONLY_338
#include "udon/horizon_pricing.hpp"
#endif
#include <iostream>

namespace udon { std::uint64_t pair_bound_cut_count(); }
using J = udon::JsonValue;
J integer(std::int64_t n) { return J(n); }
J score(const udon::OfficialScore& s) { return J(J::Array{integer(s.lifetimeDistinct), integer(s.totalDailyDistinct), integer(s.totalServings)}); }
J route(const udon::ExactOrienteeringRoute& r) {
    udon::DayPlan p; p.actions.push_back(r.actions);
    return J(J::Object{{"mask", integer(r.spotMask)}, {"plan", udon::serialize_day_plan(p)},
        {"steps", integer(r.usedSteps)}, {"fuel", integer(r.patrolFuel)}, {"cell", integer(r.terminalCell)}});
}
J routes(const std::vector<udon::ExactOrienteeringRoute>& rs) { J::Array a; for(const auto& r:rs)a.push_back(route(r));return J(std::move(a)); }
J ask(const J& q) {
    const auto c = udon::parse_match_config(q.at("setup"));
    const auto state = udon::parse_day_state(c,q.at("state"));
    const auto ledger = udon::parse_match_ledger(c,q.at("ledger"));
    const auto op = q.at("op").string();
    const auto end = std::chrono::steady_clock::now()+std::chrono::seconds{10};
    if(op=="legacy") {
        J::Array out;
        for(int a=0;a<c.agent_count();++a)for(int mode=0;mode<5;++mode) {
            udon::TerminalMarginalRouteContext marginal;
            marginal.lifetimeBrands=ledger.lifetimeBrands;
            marginal.incumbentClaimsWithoutAgent.assign(c.spots.size(),1);
            udon::ExactOrienteeringReachability r;
            if(mode==0)r=udon::enumerate_sparse_anytime_resource_routes(c,state,a,1,32,10000,end);
            if(mode==1)r=udon::enumerate_sparse_anytime_resource_routes_to_terminal(c,state,a,state.agents[a].position,1,32,10000,end);
            if(mode==2)r=udon::enumerate_sparse_anytime_resource_routes(c,state,a,1,32,10000,end,udon::BrandMask{1},&marginal);
            if(mode==3)r=udon::enumerate_exact_resource_routes(c,state,a,end);
            if(mode==4)r=udon::enumerate_exact_high_fuel_routes(c,state,a,end);
            out.emplace_back(J::Object{{"agent",integer(a)},{"mode",integer(mode)},
                {"supported",J(r.supported)},{"complete",J(r.complete)},{"settled",integer(r.settledStates)},
                {"maximal",routes(r.maximalRoutes)},{"supplemental",routes(r.supplementalRoutes)},
                {"marginal",routes(r.terminalMarginalRoutes)},{"variants",routes(r.terminalVariants)},
                {"fuelRoutes",routes(r.servedSpotFuelRoutes)}});
        }
        return J(J::Object{{"ok",J(true)},{"legacy",J(std::move(out))}});
    }
#ifndef LEGACY_ONLY_338
    if(op=="frontier") {
        const auto f=udon::enumerate_horizon_resource_frontier(c,state,0,
            q.at("settled").integer(),q.at("created").integer(),q.at("actions").integer(),
            q.at("expired").boolean()?std::chrono::steady_clock::now():end);
        return J(J::Object{{"ok",J(true)},{"supported",J(f.supported)},{"complete",J(f.complete)},
            {"settled",integer(f.settledStates)},{"created",integer(f.createdLabels)},
            {"storedActions",integer(f.storedPlanActions)},{"routes",routes(f.routes)}});
    }
    udon::ExactStepSimulator sim(c);udon::IndependentDayValidator val(c);
    udon::FutureWitness original;original.certified=true;auto s=state;auto l=ledger;
    for(const auto& plan:q.at("plans").array()) {
        original.futurePlans.push_back(udon::parse_day_plan(c,plan));
        const auto r=sim.simulate(s,original.futurePlans.back(),true);
        const auto v=val.validate(s,original.futurePlans.back(),true);std::string mismatch;
        if(!r.valid||!val.agrees_with(r,v,mismatch))throw std::runtime_error("invalid control");
        s.agents=r.finalAgents;++s.dayNumber;l.apply(r.score);
    }
    original.score={l.lifetime_distinct(),l.totalDailyDistinct,l.totalServings};
    if(q.at("corrupt").boolean())++original.score.totalServings;
    const auto copy=original;
    udon::HorizonPricingDiagnostics d;
    udon::HorizonPricingLimits limits;
    if (q.object().contains("limit")) {
        const auto name = q.at("limit").string();
        const auto n = static_cast<std::uint64_t>(q.at("value").integer());
        if (name=="settled") limits.settled=n;
        else if(name=="created") limits.created=n;
        else if(name=="actions") limits.actions=n;
        else if(name=="memo") limits.memo=n;
        else if(name=="transitions") limits.transitions=n;
        else if(name=="queries") limits.queries=n;
        else throw std::runtime_error("unknown limit");
    }
    const auto result=udon::price_horizon_witness(c,state,ledger,original,sim,val,
        q.at("expired").boolean()?std::chrono::steady_clock::now():end,d,limits);
    J::Array plans;for(const auto& p:(result?result->futurePlans:original.futurePlans))plans.push_back(udon::serialize_day_plan(p));
    bool same=copy.score==original.score&&copy.futurePlans.size()==original.futurePlans.size();
    for(std::size_t i=0;i<copy.futurePlans.size();++i)same=same&&udon::serialize_day_plan(copy.futurePlans[i]).dump()==udon::serialize_day_plan(original.futurePlans[i]).dump();
    return J(J::Object{{"ok",J(true)},{"baseline",score(copy.score)},{"score",score(result?result->score:original.score)},
        {"plans",J(std::move(plans))},{"originalUnchanged",J(same)},
        {"supported",integer(d.supported)},{"completed",integer(d.completed)},{"improvements",integer(d.improvements)},
        {"exhausted",integer(d.exhausted)},{"failures",integer(d.failures)},
        {"boundCuts",integer(udon::pair_bound_cut_count())},{"settled",integer(d.settledLabels)},{"created",integer(d.createdLabels)},{"actions",integer(d.storedActions)},
        {"memo",integer(d.memoStates)},{"transitions",integer(d.transitions)},{"queries",integer(d.dayQueries)}});
#else
    throw std::runtime_error("legacy-only probe");
#endif
}
int main(){std::string line;while(std::getline(std::cin,line)){try{std::cout<<ask(J::parse(line)).dump()<<std::endl;}
catch(const std::exception& e){std::cout<<J(J::Object{{"ok",J(false)},{"error",J(e.what())}}).dump()<<std::endl;}}}
