// Exact canonical master consumer test: no mixed/partial bundle is authorized.
#include "udon/planner.hpp"
#include "udon/protocol.hpp"
#include <iostream>
int main() {
    std::string line;
    std::getline(std::cin,line);
    try {
        const auto input=udon::JsonValue::parse(line);
        const auto config=udon::parse_match_config(input.at("setup"));
        const auto state=udon::parse_day_state(config,input.at("state"));
        const auto ledger=udon::parse_match_ledger(config,input.at("ledger"));
        if(config.agent_count()!=3) throw std::runtime_error("three agents required");
        const udon::ExactStepSimulator simulator(config);
        const udon::IndependentDayValidator validator(config);
        const udon::RouteMaster master(config,simulator,validator);
        udon::JsonValue::Array rows;
        for(int a=-2;a<=2;++a) for(int b=-2;b<=2;++b) for(int c=-2;c<=2;++c) {
            const int modes[]={a,b,c};
            udon::RoutePortfolio portfolio;portfolio.columnsByAgent.resize(3);
            for(int agent=0;agent<3;++agent) {
                udon::RouteColumn column;
                column.agent=agent;column.columnId=agent;column.contingencyBundle=modes[agent];
                column.actions={udon::PlanAction::wait(config.steps_for_day(state.dayNumber))};
                portfolio.columnsByAgent[agent].push_back(column);
            }
            udon::MasterOptions options;options.maximumCombinations=100;options.maximumCandidates=4;
            udon::MasterDiagnostics diagnostics;
            const auto candidates=master.solve(state,ledger,portfolio,options,diagnostics);
            rows.emplace_back(udon::JsonValue::Object{
                {"modes",udon::JsonValue(udon::JsonValue::Array{udon::JsonValue(std::int64_t(a)),udon::JsonValue(std::int64_t(b)),udon::JsonValue(std::int64_t(c))})},
                {"admissible",udon::JsonValue(!candidates.empty())}});
        }
        std::cout<<udon::JsonValue(std::move(rows)).dump()<<std::endl;
    } catch(const std::exception& e) {std::cerr<<e.what()<<std::endl;return 1;}
}
