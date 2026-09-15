// Research only: repartition recorded spatial moves, never call a planner.
#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"
using J = udon::JsonValue;
J num(std::int64_t n) { return J(n); }
J agents_json(const std::vector<udon::AgentState>& agents) {
    J::Array out;
    for (const auto& a : agents) out.emplace_back(J::Object{{"kind",num(static_cast<int>(a.kind))},{"pos",num(a.position)},{"fuel",num(a.fuel)}});
    return J(std::move(out));
}
J plans_json(const std::vector<udon::DayPlan>& plans) {
    J::Array out; for (const auto& p : plans) out.push_back(udon::serialize_day_plan(p)); return J(std::move(out));
}
struct Replayed { udon::OfficialScore score; J document; };
J run(const J& request) {
    const auto config=udon::parse_match_config(request.at("setup"));
    const auto state=udon::parse_day_state(config,request.at("state"));
    const auto ledger=udon::parse_match_ledger(config,request.at("ledger"));
    if (config.agent_count()!=3 || config.day_count()-state.dayNumber+1!=3 ||
        !config.roadCells.empty() || !state.others.empty() ||
        std::any_of(state.agents.begin(),state.agents.end(),[](const auto& a){return a.kind!=udon::AgentKind::Patrol;}))
        throw std::runtime_error("outside320 roadless three-Patrol three-day scope");
    std::vector<udon::DayPlan> original;
    for (const auto& p : request.at("plans").array()) original.push_back(udon::parse_day_plan(config,p));
    if(original.size()!=3) throw std::runtime_error("wrong future plan count");
    const udon::ExactStepSimulator sim(config);
    const udon::IndependentDayValidator validator(config);
    std::int64_t dualDays=0;
    auto replay=[&](const std::vector<udon::DayPlan>& plans) {
        auto current=state; auto l=ledger; J::Array days;
        for(int d=0;d<3;++d){
            const auto s=sim.simulate(current,plans[d],true);
            const auto v=validator.validate(current,plans[d],true); std::string mismatch;
            if(!validator.agrees_with(s,v,mismatch)) throw std::runtime_error("dual mismatch:"+mismatch);
            if(!s.valid) throw std::runtime_error("invalid full witness:"+s.error->message);
            if(std::any_of(s.roadFootprint.begin(),s.roadFootprint.end(),[](int x){return x!=0;}))throw std::runtime_error("unexpected road footprint");
            ++dualDays; l.apply(s.score);current.agents=s.finalAgents;
            days.emplace_back(J::Object{{"day",num(current.dayNumber)},{"agents",agents_json(current.agents)},
                {"ledger",udon::serialize_match_ledger(config,l)},
                {"daily",J(J::Array{num(s.score.dailyDistinct),num(s.score.servings)})}});
            ++current.dayNumber;
        }
        const udon::OfficialScore score{l.lifetime_distinct(),l.totalDailyDistinct,l.totalServings};
        return Replayed{score,J(J::Object{{"score",J(J::Array{num(score.lifetimeDistinct),num(score.totalDailyDistinct),num(score.totalServings)})},
            {"agents",agents_json(current.agents)},{"ledger",udon::serialize_match_ledger(config,l)},
            {"days",J(std::move(days))},{"plans",plans_json(plans)}})};
    };
    const auto baseline=replay(original);
    auto best=baseline; J::Array agentRows; J::Array rows;
    std::int64_t attempted=0,infeasible=0,valid=0,duplicates=0;
    for(int agent=0;agent<3;++agent){
        udon::AgentPlan moves; std::vector<int> oldCuts{0};
        for(const auto& p:original){for(const auto& a:p.actions[agent])if(a.kind==udon::ActionKind::Move)moves.push_back(a);oldCuts.push_back(static_cast<int>(moves.size()));}
        const int count=static_cast<int>(moves.size());
        std::vector<int> costPrefix{0}; auto pos=state.agents[agent].position;
        for(const auto& action:moves){
            const auto cost=config.move_cost(pos,udon::RoadStatus::Smooth);
            costPrefix.push_back(costPrefix.back()+cost.steps);
            pos=config.map.neighbors.at(pos).at(action.value);
            if(pos==udon::kInvalidCell || config.map.terrain.at(pos)==udon::Terrain::Pond)throw std::runtime_error("invalid spatial sequence");
        }
        auto construct=[&](int first,int second)->std::optional<std::vector<udon::DayPlan>>{
            const int cuts[]={0,first,second,count};auto plans=original;
            for(int d=0;d<3;++d){
                const int budget=config.steps_for_day(state.dayNumber+d);
                const int duration=1+costPrefix[cuts[d+1]]-costPrefix[cuts[d]];
                if(duration>budget)return std::nullopt;
                auto& plan=plans[d].actions[agent];plan={udon::PlanAction::wait(1)};
                plan.insert(plan.end(),moves.begin()+cuts[d],moves.begin()+cuts[d+1]);
                if(duration<budget)plan.push_back(udon::PlanAction::wait(budget-duration));
            }
            return plans;
        };
        const auto normalized=construct(oldCuts[1],oldCuts[2]);
        // A prefix WAIT can make an originally exact-fill movement day infeasible.
        // Keep an explicit missing control rather than silently changing the schema.
        J control;
        if(normalized){auto c=replay(*normalized);
            if(c.document.at("agents").dump()!=baseline.document.at("agents").dump())throw std::runtime_error("normalized final-state mismatch");
            control=c.document;
        }
        agentRows.emplace_back(J::Object{{"agent",num(agent)},{"moves",num(count)},
            {"original_cuts",J(J::Array{num(oldCuts[1]),num(oldCuts[2])})},{"normalized",control}});
        std::set<std::string> seen;
        for(int first=0;first<=count;++first)for(int second=first;second<=count;++second){
            ++attempted;const auto plans=construct(first,second);
            if(!plans){++infeasible;continue;}
            if(!seen.insert(plans_json(*plans).dump()).second){++duplicates;continue;}
            const auto r=replay(*plans);++valid;
            if(r.document.at("agents").dump()!=baseline.document.at("agents").dump())throw std::runtime_error("retimed final-state mismatch");
            if(best.score<r.score)best=r;
            rows.emplace_back(J::Object{{"agent",num(agent)},{"cuts",J(J::Array{num(first),num(second)})},{"replay",r.document}});
        }
    }
    return J(J::Object{{"ok",J(true)},{"baseline",baseline.document},{"best",best.document},
        {"controls",J(std::move(agentRows))},{"rows",J(std::move(rows))},
        {"attempted",num(attempted)},{"infeasible_duration",num(infeasible)},{"valid",num(valid)},
        {"duplicates",num(duplicates)},{"dual_valid_days",num(dualDays)},{"final_state_mismatches",num(0)}});
}
int main(){std::string line;while(std::getline(std::cin,line)){try{std::cout<<run(J::parse(line)).dump()<<std::endl;}
catch(const std::exception& e){std::cout<<J(J::Object{{"ok",J(false)},{"error",J(e.what())}}).dump()<<std::endl;}}}
