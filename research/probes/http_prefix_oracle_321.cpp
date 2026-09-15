// Consumed-input attribution. Include frozen primitives, not its CLI/solver path.
#define main frozen_oracle_main_not_called_321
#include "multi_patrol_oracle.cpp"
#undef main

namespace prefix321 {
using J=udon::JsonValue;
J n(std::int64_t value){return J(value);}
J score_json(const udon::OfficialScore& s){return J(J::Array{n(s.lifetimeDistinct),n(s.totalDailyDistinct),n(s.totalServings)});}
J agents_json(const std::vector<udon::AgentState>& values){J::Array out;for(const auto& a:values)out.emplace_back(J::Object{{"kind",n(static_cast<int>(a.kind))},{"pos",n(a.position)},{"fuel",n(a.fuel)}});return J(std::move(out));}
using Key=std::tuple<int,ThreeMatchKey>;
struct Node {udon::OfficialScore value;std::array<udon::AgentPlan,3> plans;std::array<std::size_t,3> nextToCurrent{};ThreeMatchKey next{};};
class Solver {
    const udon::MatchConfig& c;
    std::map<Key,Node> memo;
    std::map<std::tuple<int,int,int>,std::vector<DayOutcome>> dayCache;
    std::vector<udon::RoadStatus> smooth;
    std::int64_t transitions=0;
public:
    explicit Solver(const udon::MatchConfig& config):c(config),smooth(c.map.cell_count(),udon::RoadStatus::Smooth){}
    const Node& solve(int day,const ThreeMatchKey& key){
        const Key k{day,key};if(const auto found=memo.find(k);found!=memo.end())return found->second;
        if(day>c.day_count())return memo.emplace(k,Node{udon::OfficialScore{udon::brand_count(std::get<6>(key)),0,0}}).first->second;
        const int pos[]={std::get<0>(key),std::get<2>(key),std::get<4>(key)};
        const int fuel[]={std::get<1>(key),std::get<3>(key),std::get<5>(key)};
        std::array<const std::vector<DayOutcome>*,3> outcomes;
        for(int a=0;a<3;++a){const auto ck=std::make_tuple(day,pos[a],fuel[a]);auto f=dayCache.find(ck);
            if(f==dayCache.end())f=dayCache.emplace(ck,enumerate_day(c,day,pos[a],fuel[a],smooth)).first;
            outcomes[a]=&f->second;
        }
        std::optional<Node> best;
        for(const auto& a:*outcomes[0])for(const auto& b:*outcomes[1])for(const auto& d:*outcomes[2]){
            ++transitions;
            const auto [brands,servings]=joint_day_score(c,std::array<std::uint32_t,3>{a.spotMask,b.spotMask,d.spotMask});
            const auto canonical=canonical_triple(std::array<const DayOutcome*,3>{&a,&b,&d});
            ThreeMatchKey next{canonical.states[0].first,canonical.states[0].second,
                canonical.states[1].first,canonical.states[1].second,canonical.states[2].first,canonical.states[2].second,
                std::get<6>(key)|brands};
            auto value=solve(day+1,next).value;value.totalDailyDistinct+=udon::brand_count(brands);value.totalServings+=servings;
            if(!best || best->value<value)best=Node{value,{a.actions,b.actions,d.actions},canonical.nextToCurrent,next};
        }
        if(!best)throw std::runtime_error("empty complete state");
        return memo.emplace(k,std::move(*best)).first->second;
    }
    J run(const udon::DayState& root,const udon::MatchLedger& prefix){
        std::array<std::tuple<int,int,std::size_t>,3> initial;
        for(std::size_t a=0;a<3;++a)initial[a]={root.agents[a].position,root.agents[a].fuel,a};
        std::sort(initial.begin(),initial.end());
        ThreeMatchKey key{std::get<0>(initial[0]),std::get<1>(initial[0]),std::get<0>(initial[1]),std::get<1>(initial[1]),
            std::get<0>(initial[2]),std::get<1>(initial[2]),prefix.lifetimeBrands};
        auto expected=solve(root.dayNumber,key).value;
        expected.totalDailyDistinct+=prefix.totalDailyDistinct;expected.totalServings+=prefix.totalServings;
        auto state=root;auto ledger=prefix;
        std::array<std::size_t,3> mapping{std::get<2>(initial[0]),std::get<2>(initial[1]),std::get<2>(initial[2])};
        J::Array days;
        for(int day=root.dayNumber;day<=c.day_count();++day){
            state.dayNumber=day;const auto& node=memo.at(Key{day,key});udon::DayPlan plan;plan.actions.resize(3);
            for(std::size_t a=0;a<3;++a)plan.actions[mapping[a]]=node.plans[a];
            udon::SimulationResult simulation;std::string mismatch;
            if(!validates(c,state,plan,simulation,mismatch))throw std::runtime_error("conditional witness invalid:"+mismatch);
            state.agents=simulation.finalAgents;ledger.apply(simulation.score);
            std::array<std::size_t,3> nextMapping;
            for(std::size_t a=0;a<3;++a)nextMapping[a]=mapping[node.nextToCurrent[a]];
            mapping=nextMapping;key=node.next;
            const int pos[]={std::get<0>(key),std::get<2>(key),std::get<4>(key)};
            const int fuel[]={std::get<1>(key),std::get<3>(key),std::get<5>(key)};
            for(int a=0;a<3;++a)if(state.agents[mapping[a]].position!=pos[a] || state.agents[mapping[a]].fuel!=fuel[a])throw std::runtime_error("physical mapping mismatch");
            if(ledger.lifetimeBrands!=std::get<6>(key))throw std::runtime_error("brand ledger mismatch");
            days.emplace_back(J::Object{{"day",n(day)},{"plan",udon::serialize_day_plan(plan)},
                {"agents",agents_json(state.agents)},{"ledger",udon::serialize_match_ledger(c,ledger)},
                {"score",score_json({ledger.lifetime_distinct(),ledger.totalDailyDistinct,ledger.totalServings})}});
        }
        const udon::OfficialScore actual{ledger.lifetime_distinct(),ledger.totalDailyDistinct,ledger.totalServings};
        if(actual!=expected)throw std::runtime_error("conditional value reconstruction mismatch");
        return J(J::Object{{"ok",J(true)},{"complete",J(true)},{"score",score_json(actual)},
            {"days",J(std::move(days))},{"memo_states",n(memo.size())},{"day_enumerations",n(dayCache.size())},
            {"joint_transitions",n(transitions)},{"failure",J()}});
    }
};
J request(const J& q){
    const auto c=udon::parse_match_config(q.at("setup"));const auto state=udon::parse_day_state(c,q.at("state"));
    const auto ledger=udon::parse_match_ledger(c,q.at("ledger"));
    if(c.agent_count()!=3 || !c.roadCells.empty() || !state.others.empty() || c.spots.size()>6 ||
        std::count_if(c.map.terrain.begin(),c.map.terrain.end(),[](auto t){return t!=udon::Terrain::Pond;})>10 ||
        std::any_of(state.agents.begin(),state.agents.end(),[](const auto& a){return a.kind!=udon::AgentKind::Patrol;}))
        throw std::runtime_error("outside321 small roadless three-Patrol scope");
    // Validate authoritative root even when the optimal plan would avoid its bad fields.
    udon::DayPlan wait;wait.actions.resize(3,{udon::PlanAction::wait(c.steps_for_day(state.dayNumber))});
    udon::SimulationResult s;std::string mismatch;
    if(!validates(c,state,wait,s,mismatch))throw std::runtime_error("invalid root:"+mismatch);
    if(q.contains("op") && q.at("op").string()=="reference"){
        if(state.dayNumber!=1 || ledger.lifetime_distinct()!=0 || ledger.totalDailyDistinct!=0 || ledger.totalServings!=0)throw std::runtime_error("reference root not initial");
        for(int a=0;a<3;++a)if(state.agents[a].position!=c.initialAgents[a] || state.agents[a].fuel!=c.fuelLimit)throw std::runtime_error("reference initial agents");
        Fixture fixture;fixture.config=c;const auto r=solve_three_oracle(fixture);
        if(!r.valid)throw std::runtime_error("reference failed");
        return J(J::Object{{"ok",J(true)},{"score",score_json(r.score)}});
    }
    return Solver(c).run(state,ledger);
}
}
int main(){std::string line;while(std::getline(std::cin,line)){try{std::cout<<prefix321::request(udon::JsonValue::parse(line)).dump()<<std::endl;}
catch(const std::exception& e){std::cout<<udon::JsonValue(udon::JsonValue::Object{{"ok",udon::JsonValue(false)},{"error",udon::JsonValue(e.what())}}).dump()<<std::endl;}}}
