// Isolated capability probe, unchanged library. Never submits plans.
#include "udon/protocol.hpp"
#include "udon/slack_refiner.hpp"
#include "udon/orienteering.hpp"
#include <algorithm>
#include <atomic>
#include <iostream>
#include <thread>
using J=udon::JsonValue;
using Clock=std::chrono::steady_clock;
J n(std::int64_t v){return J(v);}
J score(const udon::OfficialScore& s){return J(J::Array{n(s.lifetimeDistinct),n(s.totalDailyDistinct),n(s.totalServings)});}
bool same_state(const udon::DayState& a,const udon::DayState& b){
    const auto agents=[](const auto& x,const auto& y){return x.size()==y.size() && std::equal(x.begin(),x.end(),y.begin(),[](const auto& p,const auto& q){return p.kind==q.kind && p.position==q.position && p.fuel==q.fuel;});};
    if(a.endsAt!=b.endsAt || a.dayNumber!=b.dayNumber || a.roadStatuses!=b.roadStatuses || !agents(a.agents,b.agents) || a.others.size()!=b.others.size())return false;
    for(std::size_t i=0;i<a.others.size();++i)if(a.others[i].teamId!=b.others[i].teamId || !agents(a.others[i].agents,b.others[i].agents))return false;
    return true;
}

J run(const J& q,std::chrono::milliseconds budget){
    const auto started=Clock::now(),deadline=started+budget;
    const auto config=udon::parse_match_config(q.at("config"));
    const auto state=udon::parse_day_state(config,q.at("state"));
    const auto ledger=udon::parse_match_ledger(config,q.at("ledger"));
    const auto plan=udon::parse_day_plan(config,q.at("plan64"));
    udon::ExactStepSimulator simulator(config);udon::IndependentDayValidator validator(config);
    const auto base=simulator.simulate(state,plan,true);std::string why;
    if(!base.valid || !validator.agrees_with(base,validator.validate(state,plan,true),why))throw std::runtime_error("base mismatch:"+why);
    auto bestPlan=plan;auto bestSimulation=base;auto bestScore=udon::OfficialScore::after_day(ledger,base.score);
    auto baseLedger=ledger;baseLedger.apply(base.score);
    const auto baseScore=bestScore;
    J::Array queries,candidates;
    bool rollback=false;std::int64_t bestAgent=-1;
    const auto finish=[&](){return J(J::Object{{"input_echo",q},{"budget_ms",n(budget.count())},
        {"base_score",score(baseScore)},{"score",score(rollback?baseScore:bestScore)},
        {"plan",udon::serialize_day_plan(rollback?plan:bestPlan)},{"queries",J(queries)},
        {"candidates",J(candidates)},{"rollback",J(rollback)},{"takeover",J(!rollback && baseScore<bestScore)},
        {"agent",n(rollback?-1:bestAgent)},{"ms",J(std::chrono::duration<double,std::milli>(Clock::now()-started).count())},
        {"input_unmodified",J(same_state(state,udon::parse_day_state(config,q.at("state"))))},
        {"zero_validator_mismatch",J(true)}});};
    if(Clock::now()+std::chrono::milliseconds(80)>=deadline){rollback=true;return finish();}
    std::vector<udon::AgentIndex> tasks;
    std::vector<std::uint32_t> masks(config.agent_count());
    std::vector<std::int32_t> claims(config.spots.size());
    for(const auto& c:base.claims)masks.at(c.agent)|=std::uint32_t{1}<<c.spot;
    for(udon::AgentIndex a=0;a<config.agent_count();++a){
        if(state.agents.at(a).kind!=udon::AgentKind::Patrol)continue;
        tasks.push_back(a);for(std::size_t s=0;s<claims.size();++s)if(masks[a]&(std::uint32_t{1}<<s))++claims[s];
    }
    std::vector<udon::ExactOrienteeringReachability> pools(config.agent_count());
    std::vector<std::int32_t> bounds(config.agent_count());
    std::atomic<std::size_t> next{0},completed{0};std::atomic<bool> failed{false};
    const auto searchDeadline=deadline-std::chrono::milliseconds(80);
    {
        std::vector<std::jthread> workers;
        for(std::size_t w=0;w<std::min<std::size_t>(4,tasks.size());++w)workers.emplace_back([&](){try{
            while(Clock::now()<searchDeadline){const auto t=next.fetch_add(1);if(t>=tasks.size())return;const auto a=tasks[t];
                const int bound=state.agents[a].fuel-base.finalAgents[a].fuel;bounds[a]=bound;
                if(bound>=0){auto queryState=state;queryState.agents[a].fuel=bound;
                    udon::TerminalMarginalRouteContext ctx;ctx.lifetimeBrands=ledger.lifetimeBrands;ctx.incumbentClaimsWithoutAgent=claims;
                    for(std::size_t s=0;s<claims.size();++s)if(masks[a]&(std::uint32_t{1}<<s))--ctx.incumbentClaimsWithoutAgent[s];
                    pools[a]=udon::enumerate_sparse_anytime_resource_routes_to_terminal(config,queryState,a,base.finalAgents[a].position,
                        std::min<std::int32_t>(std::max(1,config.brand_count()-1),config.spots.size()),32,1250000,searchDeadline,{},&ctx);
                }
                ++completed;
            }
        }catch(...){failed=true;}});
    }
    if(failed)throw std::runtime_error("query worker failed");
    for(const auto a:tasks){const auto& p=pools[a];queries.emplace_back(J::Object{{"agent",n(a)},{"fuel_bound",n(bounds[a])},
        {"supported",J(p.supported)},{"complete",J(p.complete)},{"settled",n(p.settledStates)},
        {"canonical_routes",n(p.maximalRoutes.size()+p.supplementalRoutes.size())},{"marginal_routes",n(p.terminalMarginalRoutes.size())}});}
    if(completed!=tasks.size() || Clock::now()>=searchDeadline){rollback=true;return finish();}
    for(const auto a:tasks)for(const auto& r:pools[a].terminalMarginalRoutes){
        if(Clock::now()>=deadline){rollback=true;return finish();}
        if(r.patrolFuel>bounds[a])throw std::runtime_error("query expenditure violation");
        auto p=plan;p.actions[a]=r.actions;const auto s=simulator.simulate(state,p,false);
        if(!validator.agrees_with(s,validator.validate(state,p,false),why))throw std::runtime_error("candidate mismatch:"+why);
        auto l=ledger;l.apply(s.score);const auto cs=udon::OfficialScore::after_day(ledger,s.score);
        const bool safe=s.valid && udon::protected_slack_transition_dominates(base,s) && udon::protected_slack_ledger_dominates(baseLedger,l);
        candidates.emplace_back(J::Object{{"agent",n(a)},{"mask",n(r.spotMask)},{"fuel",n(r.patrolFuel)},
            {"valid",J(s.valid)},{"safe",J(safe)},{"score",score(cs)}});
        if(safe && bestScore<cs){bestPlan=std::move(p);bestSimulation=s;bestScore=cs;bestAgent=a;}
    }
    if(Clock::now()>=deadline)rollback=true;
    return finish();
}
int main(){try{std::string line;std::getline(std::cin,line);const auto q=J::parse(line);
    // Registered order: expired control, then exactly one bounded suffix.
    const auto expired=run(q,std::chrono::milliseconds(0));
    const auto bounded=run(q,std::chrono::milliseconds(5000));
    if(expired.at("plan").dump()!=q.at("plan64").dump() || !expired.at("queries").array().empty())throw std::runtime_error("expired mutation");
    std::cout<<J(J::Object{{"expired",expired},{"bounded",bounded}}).dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
