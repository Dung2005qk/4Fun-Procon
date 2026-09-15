#include "udon/decision.hpp"
#include "udon/protocol.hpp"
#include <iostream>
using J=udon::JsonValue;
J num(std::int64_t n){return J(n);}
J score(const udon::OfficialScore& s){return J(J::Array{num(s.lifetimeDistinct),num(s.totalDailyDistinct),num(s.totalServings)});}
J ask(const J& q){
    const auto config=udon::parse_match_config(q.at("setup"));
    const auto state=udon::parse_day_state(config,q.at("state"));
    const auto ledger=udon::parse_match_ledger(config,q.at("ledger"));
    const auto end=q.contains("expired") && q.at("expired").boolean()
        ? std::chrono::steady_clock::now()-std::chrono::seconds{1}
        : std::chrono::steady_clock::now()+std::chrono::seconds{5};
    const auto v=udon::FastViabilityAnalyzer(config).analyze(state,ledger,end);
    J::Array tiers;for(const auto& t:v.conditionalTiers)tiers.emplace_back(J::Object{
        {"daily",num(t.upperDailyDistinct)},{"servings",num(t.upperServings)}});
    return J(J::Object{{"ok",J(true)},{"upper",score(v.upperBound)},
        {"pessimistic",score(v.pessimisticUpperBound)},{"tiers",J(std::move(tiers))},
        {"expired",J(v.deadlineReached)}});
}
int main(){std::string line;while(std::getline(std::cin,line)){try{std::cout<<ask(J::parse(line)).dump()<<std::endl;}
catch(const std::exception& e){std::cout<<J(J::Object{{"ok",J(false)},{"error",J(e.what())}}).dump()<<std::endl;}}}
