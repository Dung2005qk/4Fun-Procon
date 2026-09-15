// Isolated two-coordinate capability. The historical full-team oracle is not called.
#define main unused_historical_entry_336
#include "multi_patrol_oracle.cpp"
#undef main

namespace pair336 {
using J = udon::JsonValue;
J num(std::int64_t v) { return J(v); }
J score(const udon::OfficialScore& s) { return J(J::Array{num(s.lifetimeDistinct),num(s.totalDailyDistinct),num(s.totalServings)}); }
J agents(const std::vector<udon::AgentState>& values) {
    J::Array a; for(const auto& v:values) a.emplace_back(J::Object{{"kind",num(static_cast<int>(v.kind))},{"pos",num(v.position)},{"fuel",num(v.fuel)}});
    return J(std::move(a));
}
struct Exhausted : std::runtime_error { using std::runtime_error::runtime_error; };
using Key = std::tuple<int,int,int,int,int,udon::BrandMask>;
struct Node { udon::OfficialScore value; std::array<udon::AgentPlan,2> actions; std::array<int,2> pos{},fuel{}; udon::BrandMask brands; };

class PairResponse {
    const udon::MatchConfig& config;
    const udon::DayState& root;
    const udon::MatchLedger& prefix;
    const std::vector<udon::DayPlan>& original;
    const std::vector<std::array<std::uint32_t,3>>& masks;
    const std::vector<std::vector<udon::AgentState>>& fixed;
    std::array<int,2> pair;
    std::size_t memoLimit;
    std::int64_t transitionLimit,transitions=0;
    std::map<Key,Node> memo;
    std::map<std::tuple<int,int,int>,std::vector<DayOutcome>> dayCache;
    std::vector<udon::RoadStatus> smooth;
    const std::vector<DayOutcome>& outcomes(int day,int pos,int fuel) {
        auto key=std::make_tuple(day,pos,fuel); auto f=dayCache.find(key);
        if(f==dayCache.end()) f=dayCache.emplace(key,enumerate_day(config,day,pos,fuel,smooth)).first;
        return f->second;
    }
    const Node& solve(int day,int p0,int f0,int p1,int f1,udon::BrandMask brands) {
        Key key{day,p0,f0,p1,f1,brands}; if(auto f=memo.find(key); f!=memo.end()) return f->second;
        if(memo.size()>=memoLimit) throw Exhausted("memo safety ceiling");
        if(day>config.day_count()) return memo.emplace(key,Node{{udon::brand_count(brands),0,0},{},{p0,p1},{f0,f1},brands}).first->second;
        const auto& left=outcomes(day,p0,f0); const auto& right=outcomes(day,p1,f1); std::optional<Node> best;
        for(const auto& a:left) for(const auto& b:right) {
            if(transitions>=transitionLimit) throw Exhausted("transition safety ceiling"); ++transitions;
            auto claims=masks.at(day-root.dayNumber); claims[pair[0]]=a.spotMask; claims[pair[1]]=b.spotMask;
            const auto [daily,servings]=joint_day_score(config,claims); const auto nextBrands=brands|daily;
            auto value=solve(day+1,a.position,a.fuel,b.position,b.fuel,nextBrands).value;
            value.totalDailyDistinct+=udon::brand_count(daily); value.totalServings+=servings;
            if(!best || best->value<value) best=Node{value,{a.actions,b.actions},{a.position,b.position},{a.fuel,b.fuel},nextBrands};
        }
        if(!best) throw std::runtime_error("empty pair domain");
        if(memo.size()>=memoLimit) throw Exhausted("memo safety ceiling on unwind");
        return memo.emplace(key,std::move(*best)).first->second;
    }
public:
    PairResponse(const udon::MatchConfig& c,const udon::DayState& s,const udon::MatchLedger& l,
                 const std::vector<udon::DayPlan>& p,const std::vector<std::array<std::uint32_t,3>>& m,
                 const std::vector<std::vector<udon::AgentState>>& f,std::array<int,2> a,
                 std::size_t ml,std::int64_t tl): config(c),root(s),prefix(l),original(p),masks(m),fixed(f),pair(a),
                 memoLimit(ml),transitionLimit(tl),smooth(c.map.cell_count(),udon::RoadStatus::Smooth) {}
    J run() {
        auto value=solve(root.dayNumber,root.agents[pair[0]].position,root.agents[pair[0]].fuel,
                         root.agents[pair[1]].position,root.agents[pair[1]].fuel,prefix.lifetimeBrands).value;
        value.totalDailyDistinct+=prefix.totalDailyDistinct; value.totalServings+=prefix.totalServings;
        auto s=root;auto l=prefix;J::Array days;
        for(int d=root.dayNumber;d<=config.day_count();++d) {
            s.dayNumber=d; const auto node=solve(d,s.agents[pair[0]].position,s.agents[pair[0]].fuel,
                s.agents[pair[1]].position,s.agents[pair[1]].fuel,l.lifetimeBrands);
            auto plan=original.at(d-root.dayNumber);
            for(int j=0;j<2;++j) plan.actions[pair[j]]=node.actions[j];
            udon::SimulationResult sim;std::string mismatch;
            if(!validates(config,s,plan,sim,mismatch)) throw std::runtime_error("dual pair validation:"+mismatch);
            for(int a=0;a<3;++a) {
                int j=a==pair[0]?0:(a==pair[1]?1:-1);
                const int pos=j<0?fixed.at(d-root.dayNumber)[a].position:node.pos[j];
                const int fuel=j<0?fixed.at(d-root.dayNumber)[a].fuel:node.fuel[j];
                if(sim.finalAgents[a].position!=pos||sim.finalAgents[a].fuel!=fuel) throw std::runtime_error("pair/fixed physical drift");
            }
            l.apply(sim.score); if(l.lifetimeBrands!=node.brands) throw std::runtime_error("pair brand ledger drift");
            s.agents=sim.finalAgents;
            days.emplace_back(J::Object{{"day",num(d)},{"plan",udon::serialize_day_plan(plan)},
                {"agents",agents(s.agents)},{"ledger",udon::serialize_match_ledger(config,l)},
                {"score",score({l.lifetime_distinct(),l.totalDailyDistinct,l.totalServings})}});
        }
        if(udon::OfficialScore{l.lifetime_distinct(),l.totalDailyDistinct,l.totalServings}!=value) throw std::runtime_error("pair score mismatch");
        return J(J::Object{{"pair",J(J::Array{num(pair[0]),num(pair[1])})},{"complete",J(true)},
            {"score",score(value)},{"days",J(std::move(days))},{"memo_states",num(memo.size())},
            {"day_enumerations",num(dayCache.size())},{"transitions",num(transitions)}});
    }
};

J request(const J& q) {
    const auto c=udon::parse_match_config(q.at("setup"));const auto root=udon::parse_day_state(c,q.at("state"));
    const auto ledger=udon::parse_match_ledger(c,q.at("ledger"));
    if(c.agent_count()!=3||!c.roadCells.empty()||!root.others.empty()||c.spots.size()>6||
        std::count_if(c.map.terrain.begin(),c.map.terrain.end(),[](auto t){return t!=udon::Terrain::Pond;})>10||
        std::any_of(root.agents.begin(),root.agents.end(),[](const auto& a){return a.kind!=udon::AgentKind::Patrol;}))
        throw std::runtime_error("outside336 research domain");
    if(q.at("plans").array().size()!=static_cast<std::size_t>(c.day_count()-root.dayNumber+1)) throw std::runtime_error("incomplete original W1");
    std::int64_t ml=250000,tl=30000000;
    if(q.contains("memo_limit")) ml=q.at("memo_limit").integer();
    if(q.contains("transition_limit")) tl=q.at("transition_limit").integer();
    if(ml<0||ml>250000||tl<0||tl>30000000) throw std::runtime_error("invalid or enlarged safety ceiling");
    auto s=root;auto l=ledger;std::vector<udon::DayPlan> plans;std::vector<std::array<std::uint32_t,3>> masks;
    std::vector<std::vector<udon::AgentState>> fixed;
    for(const auto& wire:q.at("plans").array()) {
        auto plan=udon::parse_day_plan(c,wire);udon::SimulationResult sim;std::string mismatch;
        if(!validates(c,s,plan,sim,mismatch)) throw std::runtime_error("original W1 invalid:"+mismatch);
        std::array<std::uint32_t,3> claims{}; for(const auto& claim:sim.claims) claims.at(claim.agent)|=std::uint32_t{1}<<claim.spot;
        const auto [brands,servings]=joint_day_score(c,claims);
        if(brands!=sim.score.brands||servings!=sim.score.servings) throw std::runtime_error("original joint claims mismatch");
        plans.push_back(plan);masks.push_back(claims);fixed.push_back(sim.finalAgents);s.agents=sim.finalAgents;++s.dayNumber;l.apply(sim.score);
    }
    const udon::OfficialScore baseline{l.lifetime_distinct(),l.totalDailyDistinct,l.totalServings};J::Array responses;
    for(const std::array<int,2> pair:{std::array<int,2>{0,1},std::array<int,2>{0,2},std::array<int,2>{1,2}}) {
        auto r=PairResponse(c,root,ledger,plans,masks,fixed,pair,static_cast<std::size_t>(ml),tl).run();const auto& v=r.at("score").array();
        if(udon::OfficialScore{int(v[0].integer()),int(v[1].integer()),int(v[2].integer())}<baseline) throw std::runtime_error("original feasible incumbent lost");
        responses.push_back(std::move(r));
    }
    return J(J::Object{{"ok",J(true)},{"complete",J(true)},{"baseline",score(baseline)},{"responses",J(std::move(responses))}});
}
}
int main() {
    std::string line;while(std::getline(std::cin,line)) {
        try {std::cout<<pair336::request(udon::JsonValue::parse(line)).dump()<<std::endl;}
        catch(const pair336::Exhausted& e) {std::cout<<udon::JsonValue(udon::JsonValue::Object{{"ok",udon::JsonValue(true)},{"complete",udon::JsonValue(false)},{"reason",udon::JsonValue(e.what())}}).dump()<<std::endl;}
        catch(const std::exception& e) {std::cout<<udon::JsonValue(udon::JsonValue::Object{{"ok",udon::JsonValue(false)},{"error",udon::JsonValue(e.what())}}).dump()<<std::endl;}
    }
}
