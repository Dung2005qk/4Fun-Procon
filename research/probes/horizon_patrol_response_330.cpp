// Isolated capability probe; the existing full-team oracle main is never called.
#define main frozen_oracle_main_not_called_330
#include "multi_patrol_oracle.cpp"
#undef main

namespace response330 {
using J=udon::JsonValue;
J n(std::int64_t x){return J(x);}
J score(const udon::OfficialScore& s){return J(J::Array{n(s.lifetimeDistinct),n(s.totalDailyDistinct),n(s.totalServings)});}
J agents(const std::vector<udon::AgentState>& a){J::Array out;for(const auto& v:a)out.emplace_back(J::Object{
    {"kind",n(static_cast<int>(v.kind))},{"pos",n(v.position)},{"fuel",n(v.fuel)}});return J(std::move(out));}
using Key=std::tuple<int,int,int,udon::BrandMask>;
struct Node {udon::OfficialScore value;udon::AgentPlan actions;int pos=0,fuel=0;udon::BrandMask brands;};

class Response {
    const udon::MatchConfig& c;
    const udon::DayState& root;
    const udon::MatchLedger& ledger;
    const std::vector<udon::DayPlan>& plans;
    const std::vector<std::array<std::uint32_t,3>>& masks;
    const std::vector<std::vector<udon::AgentState>>& fixedStates;
    int agent;
    std::map<Key,Node> memo;
    std::map<std::tuple<int,int,int>,std::vector<DayOutcome>> dayCache;
    std::vector<udon::RoadStatus> smooth;
    std::int64_t transitions=0;
public:
    Response(const udon::MatchConfig& config,const udon::DayState& start,const udon::MatchLedger& prefix,
             const std::vector<udon::DayPlan>& original,const std::vector<std::array<std::uint32_t,3>>& claims,
             const std::vector<std::vector<udon::AgentState>>& fixed,int chosen)
        :c(config),root(start),ledger(prefix),plans(original),masks(claims),fixedStates(fixed),agent(chosen),
         smooth(c.map.cell_count(),udon::RoadStatus::Smooth){}
    const Node& solve(int day,int pos,int fuel,udon::BrandMask brands){
        Key key{day,pos,fuel,brands};if(const auto f=memo.find(key);f!=memo.end())return f->second;
        if(day>c.day_count())return memo.emplace(key,Node{{udon::brand_count(brands),0,0},{},pos,fuel,brands}).first->second;
        const auto dk=std::make_tuple(day,pos,fuel);auto cache=dayCache.find(dk);
        if(cache==dayCache.end())cache=dayCache.emplace(dk,enumerate_day(c,day,pos,fuel,smooth)).first;
        std::optional<Node> best;
        for(const auto& option:cache->second){
            ++transitions;auto claims=masks.at(day-root.dayNumber);claims[agent]=option.spotMask;
            const auto [daily,servings]=joint_day_score(c,claims);
            const auto nextBrands=brands|daily;auto value=solve(day+1,option.position,option.fuel,nextBrands).value;
            value.totalDailyDistinct+=udon::brand_count(daily);value.totalServings+=servings;
            if(!best||best->value<value)best=Node{value,option.actions,option.position,option.fuel,nextBrands};
        }
        if(!best)throw std::runtime_error("empty one-Patrol domain");
        return memo.emplace(key,std::move(*best)).first->second;
    }
    J run(){
        auto expected=solve(root.dayNumber,root.agents[agent].position,root.agents[agent].fuel,ledger.lifetimeBrands).value;
        expected.totalDailyDistinct+=ledger.totalDailyDistinct;expected.totalServings+=ledger.totalServings;
        auto s=root;auto l=ledger;J::Array days;
        for(int day=root.dayNumber;day<=c.day_count();++day){
            s.dayNumber=day;const auto node=solve(day,s.agents[agent].position,s.agents[agent].fuel,l.lifetimeBrands);
            auto plan=plans.at(day-root.dayNumber);plan.actions[agent]=node.actions;
            udon::SimulationResult sim;std::string mismatch;
            if(!validates(c,s,plan,sim,mismatch))throw std::runtime_error("dual invalid response:"+mismatch);
            for(int a=0;a<3;++a)if(a!=agent && (sim.finalAgents[a].position!=fixedStates.at(day-root.dayNumber)[a].position||
                sim.finalAgents[a].fuel!=fixedStates.at(day-root.dayNumber)[a].fuel))throw std::runtime_error("fixed other state drift");
            if(sim.finalAgents[agent].position!=node.pos||sim.finalAgents[agent].fuel!=node.fuel)throw std::runtime_error("response state drift");
            l.apply(sim.score);if(l.lifetimeBrands!=node.brands)throw std::runtime_error("response brand ledger drift");
            s.agents=sim.finalAgents;
            days.emplace_back(J::Object{{"day",n(day)},{"plan",udon::serialize_day_plan(plan)},
                {"agents",agents(s.agents)},{"ledger",udon::serialize_match_ledger(c,l)},
                {"score",score({l.lifetime_distinct(),l.totalDailyDistinct,l.totalServings})}});
        }
        if(udon::OfficialScore{l.lifetime_distinct(),l.totalDailyDistinct,l.totalServings}!=expected)
            throw std::runtime_error("response score reconstruction drift");
        return J(J::Object{{"agent",n(agent)},{"score",score(expected)},{"days",J(std::move(days))},
            {"memo_states",n(memo.size())},{"day_enumerations",n(dayCache.size())},{"transitions",n(transitions)},
            {"complete",J(true)}});
    }
};
J request(const J& q){
    auto c=udon::parse_match_config(q.at("setup"));auto root=udon::parse_day_state(c,q.at("state"));
    auto ledger=udon::parse_match_ledger(c,q.at("ledger"));
    if(c.agent_count()!=3||!c.roadCells.empty()||!root.others.empty()||c.spots.size()>6||
       std::count_if(c.map.terrain.begin(),c.map.terrain.end(),[](auto t){return t!=udon::Terrain::Pond;})>10||
       std::any_of(root.agents.begin(),root.agents.end(),[](const auto& a){return a.kind!=udon::AgentKind::Patrol;}))
        throw std::runtime_error("outside330 attribution domain");
    if(q.at("plans").array().size()!=static_cast<std::size_t>(c.day_count()-root.dayNumber+1))
        throw std::runtime_error("incomplete fixed witness");
    std::vector<udon::DayPlan> plans;std::vector<std::array<std::uint32_t,3>> masks;
    std::vector<std::vector<udon::AgentState>> fixed;auto s=root;auto l=ledger;
    for(const auto& wire:q.at("plans").array()){
        auto plan=udon::parse_day_plan(c,wire);udon::SimulationResult sim;std::string mismatch;
        if(!validates(c,s,plan,sim,mismatch))throw std::runtime_error("invalid original witness:"+mismatch);
        std::array<std::uint32_t,3> claims{};for(const auto& claim:sim.claims)claims.at(claim.agent)|=std::uint32_t{1}<<claim.spot;
        const auto [brands,servings]=joint_day_score(c,claims);
        if(brands!=sim.score.brands||servings!=sim.score.servings)throw std::runtime_error("fixed claim score mismatch");
        plans.push_back(plan);masks.push_back(claims);fixed.push_back(sim.finalAgents);s.agents=sim.finalAgents;
        ++s.dayNumber;l.apply(sim.score);
    }
    const udon::OfficialScore baseline{l.lifetime_distinct(),l.totalDailyDistinct,l.totalServings};J::Array responses;
    for(int a=0;a<3;++a){auto r=Response(c,root,ledger,plans,masks,fixed,a).run();
        const auto& v=r.at("score").array();udon::OfficialScore value{int(v[0].integer()),int(v[1].integer()),int(v[2].integer())};
        if(value<baseline)throw std::runtime_error("best response lost original witness");responses.push_back(std::move(r));}
    return J(J::Object{{"ok",J(true)},{"complete",J(true)},{"baseline",score(baseline)},{"responses",J(std::move(responses))}});
}
}
int main(){std::string line;while(std::getline(std::cin,line)){try{std::cout<<response330::request(udon::JsonValue::parse(line)).dump()<<std::endl;}
catch(const std::exception& e){std::cout<<udon::JsonValue(udon::JsonValue::Object{{"ok",udon::JsonValue(false)},{"error",udon::JsonValue(e.what())}}).dump()<<std::endl;}}}
