// Research-only observation of unchanged W1 components at a recorded day2 root.
#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <stdexcept>
#include "udon/decision.hpp"
#include "udon/protocol.hpp"
using J = udon::JsonValue;
J n(std::int64_t v) { return J(v); }
J score(const udon::OfficialScore& s) { return J(J::Array{n(s.lifetimeDistinct), n(s.totalDailyDistinct), n(s.totalServings)}); }
J agents(const std::vector<udon::AgentState>& values) {
    J::Array out;
    for (const auto& a : values) out.emplace_back(J::Object{{"kind", n(static_cast<int>(a.kind))}, {"pos", n(a.position)}, {"fuel", n(a.fuel)}});
    return J(std::move(out));
}
J run(const J& request) {
    const auto config = udon::parse_match_config(request.at("setup"));
    const auto state = udon::parse_day_state(config, request.at("state"));
    const auto ledger = udon::parse_match_ledger(config, request.at("ledger"));
    const auto plan = udon::parse_day_plan(config, request.at("plan"));
    if (state.dayNumber != 2 || config.day_count() != 4 || config.agent_count() != 3 ||
        !config.roadCells.empty() || !state.others.empty()) throw std::runtime_error("outside318 domain");
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::ParetoRouter router(config);
    const udon::RouteColumnGenerator generator(config, router);
    const udon::RouteMaster master(config, simulator, validator);
    const udon::FastViabilityAnalyzer viability(config);
    const auto recorded = master.evaluate_exact_plan(state, ledger, plan);
    if (!recorded) throw std::runtime_error("invalid recorded plan");
    auto expectedLedger = ledger;
    expectedLedger.apply(recorded->simulation.score);
    auto describe = [&](const udon::MasterCandidate& c) {
        auto l = ledger; l.apply(c.simulation.score);
        auto next = state; next.dayNumber++; next.agents = c.simulation.finalAgents;
        const auto upper = viability.analyze(next, l, std::chrono::steady_clock::now() + std::chrono::seconds(1)).upperBound;
        return J(J::Object{{"plan", udon::serialize_day_plan(c.plan)}, {"score", score(c.scoreAfterToday)},
            {"agents", agents(c.simulation.finalAgents)}, {"ledger", udon::serialize_match_ledger(config,l)}, {"next_upper", score(upper)}});
    };
    J::Array modes;
    for (const bool exact : {false, true}) {
        udon::ColumnGenerationOptions o;
        o.maximumPathsPerTarget=1; o.maximumColumnsPerAgent=3; o.maximumTargetSpots=6; o.maximumEscorts=4;
        o.enableHarvestExtensions=true; o.allowUncachedHarvestTargets=true;
        o.enableHarvestOrienteering = !exact && config.fuelLimit >= 3LL*config.steps_for_day(state.dayNumber);
        o.enableExactHarvestOrienteering = exact || config.fuelLimit >= 2LL*config.steps_for_day(state.dayNumber);
        o.enableFuelConstrainedExactHarvestOrienteering=exact;
        o.enableAnytimeFuelConstrainedHarvestOrienteering=false;
        o.maximumHarvestExtensionSources=4;
        o.maximumHarvestExtensionDepth=config.fuelLimit >= 3LL*config.steps_for_day(state.dayNumber) ? 4:3;
        o.deadline=std::chrono::steady_clock::now()+std::chrono::seconds(1);
        udon::ColumnGenerationDiagnostics gd;
        const auto portfolio=generator.generate(state,ledger,o,&gd);
        if(gd.deadlineReached) throw std::runtime_error("diagnostic generation deadline");
        J::Array columns;
        std::map<int,udon::DayPlan> bundles;
        std::map<int,std::vector<bool>> assigned;
        for(std::size_t a=0;a<portfolio.columnsByAgent.size();++a) {
            for(const auto& c:portfolio.columnsByAgent[a]) {
                udon::DayPlan one; one.actions={c.actions};
                columns.emplace_back(J::Object{{"agent",n(a)},{"id",n(c.columnId)},{"bundle",n(c.contingencyBundle)},
                    {"exact",J(c.exactOrienteering)},{"terminal",n(c.terminalCell)},{"fuel",n(c.terminalFuel)},
                    {"plan",udon::serialize_day_plan(one).array().at(0)}});
                if(!c.exactOrienteering || c.contingencyBundle<0) continue;
                auto& p=bundles[c.contingencyBundle]; auto& seen=assigned[c.contingencyBundle];
                if(p.actions.empty()){p.actions.resize(config.agent_count());seen.resize(config.agent_count());}
                if(seen[a]) throw std::runtime_error("duplicate bundle agent");
                p.actions[a]=c.actions;seen[a]=true;
            }
        }
        J::Array bundleRows;
        std::optional<udon::MasterCandidate> selected; udon::OfficialScore selectedUpper;
        for(const auto& [id,p]:bundles) {
            if(!std::all_of(assigned[id].begin(),assigned[id].end(),[](bool x){return x;})) continue;
            const auto c=master.evaluate_exact_plan(state,ledger,p);
            if(!c)throw std::runtime_error("invalid complete bundle");
            auto l=ledger;l.apply(c->simulation.score);auto next=state;next.dayNumber++;next.agents=c->simulation.finalAgents;
            const auto upper=viability.analyze(next,l,std::chrono::steady_clock::now()+std::chrono::seconds(1)).upperBound;
            bundleRows.emplace_back(J::Object{{"id",n(id)},{"candidate",describe(*c)}});
            if(!selected || selectedUpper<upper || (selectedUpper==upper && selected->scoreAfterToday<c->scoreAfterToday) ||
               (selectedUpper==upper && selected->scoreAfterToday==c->scoreAfterToday && c->stableId<selected->stableId)) {
                selected=*c;selectedUpper=upper;
            }
        }
        udon::MasterOptions mo;mo.maximumCombinations=100;mo.maximumCandidates=1;mo.maximumResolveRounds=1;
        mo.deadline=std::chrono::steady_clock::now()+std::chrono::seconds(1);
        udon::MasterDiagnostics md; const auto choices=master.solve(state,ledger,portfolio,mo,md);
        if(choices.empty() || md.deadlineReached) throw std::runtime_error("master diagnostic failed");
        modes.emplace_back(J::Object{{"exact_mode",J(exact)},{"columns",J(std::move(columns))},
            {"exact_bundles",J(std::move(bundleRows))},{"supported",n(gd.exactOrienteeringSupportedAgents)},
            {"complete",n(gd.exactOrienteeringCompleteAgents)},{"master_selected",describe(choices.front())},
            {"master_combinations",n(md.combinationsVisited)},{"master_complete",J(md.searchComplete)},
            {"bundle_selected",selected?describe(*selected):J()}});
    }
    udon::TrafficBelief belief(config);belief.observe(state);
    const auto scenarios=udon::ScenarioGenerator(config).freeze_manifest(state,belief);
    if(scenarios.scenarios.size()!=1 || scenarios.scenarios[0].scenarioClass!="deterministic-no-road")throw std::runtime_error("scenario mismatch");
    const udon::FutureWitnessRepairer repairer(config,generator,master,simulator,validator,7);
    auto certify=[&](const udon::MasterCandidate& c) {
        const auto upper=viability.analyze(state,ledger,std::chrono::steady_clock::now()+std::chrono::seconds(1)).upperBound;
        auto profile=repairer.provisional_profile(c,state,ledger,belief,scenarios,upper,24);
        repairer.repair_profile(profile,c,state,ledger,belief,scenarios,200,std::chrono::steady_clock::now()+std::chrono::seconds(1));
        const auto& w=profile.outcomes[0].witness;
        if(!w.certified || w.lowerBoundOnly || w.futurePlans.size()!=2)throw std::runtime_error("incomplete suffix");
        auto next=state;next.agents=c.simulation.finalAgents;auto l=ledger;l.apply(c.simulation.score);
        J::Array suffix;
        for(const auto& p:w.futurePlans){++next.dayNumber;const auto r=simulator.simulate(next,p,true);const auto v=validator.validate(next,p,true);std::string mismatch;
            if(!r.valid || !validator.agrees_with(r,v,mismatch))throw std::runtime_error("invalid suffix:"+mismatch);
            l.apply(r.score);next.agents=r.finalAgents;suffix.push_back(udon::serialize_day_plan(p));}
        if(w.score!=udon::OfficialScore{l.lifetime_distinct(),l.totalDailyDistinct,l.totalServings})throw std::runtime_error("suffix score mismatch");
        return J(J::Object{{"score",score(w.score)},{"future_plans",J(std::move(suffix))},{"dual_valid_days",n(2)}});
    };
    J::Array alternatives;
    for(int a=0;a<config.agent_count();++a){
        if(state.agents[a].kind!=udon::AgentKind::Patrol)throw std::runtime_error("non-patrol318");
        auto p=plan;p.actions[a]={udon::PlanAction::wait(config.steps_for_day(state.dayNumber))};
        const auto c=master.evaluate_exact_plan(state,ledger,p);if(!c)throw std::runtime_error("wait substitute invalid");
        auto l=ledger;l.apply(c->simulation.score);
        const bool neutral=c->scoreAfterToday==recorded->scoreAfterToday && l.lifetimeBrands==expectedLedger.lifetimeBrands;
        alternatives.emplace_back(J::Object{{"agent",n(a)},{"candidate",describe(*c)},{"neutral",J(neutral)},
            {"certificate",neutral?certify(*c):J()}});
    }
    return J(J::Object{{"ok",J(true)},{"recorded",describe(*recorded)}, {"modes",J(std::move(modes))},
        {"regenerated_control",certify(*recorded)},{"alternatives",J(std::move(alternatives))}});
}
int main(){std::string line;while(std::getline(std::cin,line)){try{std::cout<<run(J::parse(line)).dump()<<std::endl;}
catch(const std::exception& e){std::cout<<J(J::Object{{"ok",J(false)},{"error",J(e.what())}}).dump()<<std::endl;}}}
