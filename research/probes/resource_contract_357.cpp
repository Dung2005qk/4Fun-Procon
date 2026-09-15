#include "udon/protocol.hpp"
#include "udon/btc_protocol.hpp"
#include "udon/slack_refiner.hpp"
#include <iostream>
using J=udon::JsonValue;
void need(bool x,const char* s){if(!x)throw std::runtime_error(s);}
int main(){try{const std::string line((std::istreambuf_iterator<char>(std::cin)),std::istreambuf_iterator<char>());const auto q=J::parse(line);
    const auto c=udon::parse_match_config(q.at("config"));auto s=udon::parse_day_state(c,q.at("state"));
    const auto l=udon::parse_match_ledger(c,q.at("ledger"));const auto p=udon::parse_day_plan(c,q.at("plan64"));
    udon::ExactStepSimulator sim(c);udon::IndependentDayValidator val(c);udon::ProtectedSlackRefiner f(c);
    const auto make=[&](const udon::DayPlan& p){udon::ProtectedSlackResult b;b.plan=p;b.simulation=sim.simulate(s,p,false);b.scoreAfterToday=udon::OfficialScore::after_day(l,b.simulation.score);return b;};
    const auto b=make(p);need(b.simulation.valid,"base");const auto bytes=udon::canonical_plan_bytes(p);
    auto x=f.refine_resource_marginal(s,l,b,std::chrono::steady_clock::now());
    need(x.deadline && !x.entered && !x.takeover && udon::canonical_plan_bytes(x.output.plan)==bytes,"expired");
    auto dead=b;dead.diagnostics.deadlineReached=true;
    x=f.refine_resource_marginal(s,l,dead,std::chrono::steady_clock::now()+std::chrono::seconds(5));
    need(!x.entered && x.queries==0 && udon::canonical_plan_bytes(x.output.plan)==bytes,"complete-parent boundary");
    auto terminal=s;terminal.dayNumber=c.day_count();
    x=f.refine_resource_marginal(terminal,l,b,std::chrono::steady_clock::now()+std::chrono::seconds(5));
    need(!x.entered && x.queries==0 && udon::canonical_plan_bytes(x.output.plan)==bytes,"terminal inert");
    const auto wait=make(udon::make_wait_plan(c,s.dayNumber));
    x=f.refine_resource_marginal(s,l,wait,std::chrono::steady_clock::now()+std::chrono::seconds(5));
    need(!x.failure && !x.deadline && !x.takeover && udon::canonical_plan_bytes(x.output.plan)==udon::canonical_plan_bytes(wait.plan),"no-gain exact");
    x=f.refine_resource_marginal(s,l,b,std::chrono::steady_clock::now()+std::chrono::seconds(5));
    need(x.entered && x.takeover && !x.deadline && !x.failure && x.queries==8 && x.settled<=10000000 && x.routes<=256,"bounded witness");
    std::string why;const auto exact=sim.simulate(s,x.output.plan,false);
    need(val.agrees_with(exact,val.validate(s,x.output.plan,false),why) && udon::protected_slack_transition_dominates(b.simulation,exact),"exact transition");
    auto before=l,after=l;before.apply(b.simulation.score);after.apply(exact.score);
    need(udon::protected_slack_ledger_dominates(before,after) && b.scoreAfterToday<x.output.scoreAfterToday && exact.score.servings>=65,"strict ledger");
    need(udon::canonical_plan_bytes(b.plan)==bytes && s.agents.at(5).fuel==q.at("state").at("agents").array().at(5).at("fuel").integer(),"input unchanged");
    std::cout<<J(J::Object{{"passed",J(true)},{"contracts",J(std::int64_t{7})},{"servings",J(std::int64_t{exact.score.servings})},
        {"settled",J(x.settled)},{"queries",J(x.queries)},{"routes",J(x.routes)}}).dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
