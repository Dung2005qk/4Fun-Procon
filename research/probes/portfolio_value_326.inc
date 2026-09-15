// Complete first-day portfolio restriction with the unchanged321 suffix DP.
struct Column326 { DayOutcome outcome; int mode; };
using Portfolio326=std::array<std::vector<Column326>,3>;
bool modes326(int a,int b,int d){return (a<0&&b<0&&d<0)||(a>=0&&a==b&&a==d);}
Column326 column326(const udon::MatchConfig& c,const udon::DayState& root,int agent,const J& input){
    udon::DayPlan plan;plan.actions.resize(3,{udon::PlanAction::wait(c.steps_for_day(root.dayNumber))});
    auto wire=udon::serialize_day_plan(plan).array();wire[agent]=input.at("plan").array().at(0);
    plan=udon::parse_day_plan(c,J(std::move(wire)));
    udon::SimulationResult sim;std::string mismatch;
    if(!validates(c,root,plan,sim,mismatch))throw std::runtime_error("isolated column invalid:"+mismatch);
    DayOutcome out;out.actions=plan.actions[agent];out.position=sim.finalAgents[agent].position;out.fuel=sim.finalAgents[agent].fuel;
    for(const auto& claim:sim.claims)if(claim.agent==agent)out.spotMask|=std::uint32_t{1}<<claim.spot;
    std::uint32_t expected=0;for(const auto& v:input.at("firstVisits").array())if(v.at("claimed").boolean())expected|=std::uint32_t{1}<<v.at("spot").integer();
    if(out.position!=input.at("terminalCell").integer()||out.fuel!=input.at("terminalFuel").integer()||out.spotMask!=expected)
        throw std::runtime_error("raw column feature mismatch");
    return {std::move(out),int(input.at("contingencyBundle").integer())};
}
J portfolio326(const udon::MatchConfig& c,const udon::DayState& root,const udon::MatchLedger& prefix,const J& q){
    Solver future(c);J::Object answers;std::int64_t validatedColumns=0,certificateCount=0;
    for(const auto& cert:q.at("certificates").array()){
        const int agent=int(cert.at("agent").integer());
        const auto old=column326(c,root,agent,cert.at("original"));
        const auto rep=column326(c,root,agent,cert.at("representative"));
        if(cert.at("unchanged").boolean()){
            if(cert.at("original").dump()!=cert.at("representative").dump())throw std::runtime_error("unchanged certificate mismatch");
        }else if(old.mode>=0||rep.mode>=0||old.outcome.position!=rep.outcome.position||old.outcome.fuel>rep.outcome.fuel||
            (old.outcome.spotMask&rep.outcome.spotMask)!=old.outcome.spotMask)
            throw std::runtime_error("resource certificate failure");
        ++certificateCount;
    }
    for(const auto& [label,input]:q.at("portfolios").object()){
        if(input.array().size()!=3)throw std::runtime_error("portfolio agents");
        Portfolio326 columns;std::int64_t before=0;
        for(int agent=0;agent<3;++agent){
            std::set<std::tuple<int,int,int,std::uint32_t>> seen;
            for(const auto& value:input.array()[agent].array()){
                auto col=column326(c,root,agent,value);++validatedColumns;++before;
                const auto key=std::make_tuple(col.mode<0?-1:col.mode,col.outcome.position,col.outcome.fuel,col.outcome.spotMask);
                if(seen.insert(key).second)columns[agent].push_back(std::move(col));
            }
        }
        struct RootOption {udon::OfficialScore today;std::array<udon::AgentPlan,3> plans;};
        std::map<ThreeMatchKey,RootOption> roots;std::int64_t admissible=0,rejected=0;
        for(const auto& a:columns[0])for(const auto& b:columns[1])for(const auto& d:columns[2]){
            if(!modes326(a.mode,b.mode,d.mode)){++rejected;continue;}++admissible;
            const auto [brands,servings]=joint_day_score(c,std::array<std::uint32_t,3>{a.outcome.spotMask,b.outcome.spotMask,d.outcome.spotMask});
            const auto canon=canonical_triple(std::array<const DayOutcome*,3>{&a.outcome,&b.outcome,&d.outcome});
            ThreeMatchKey key{canon.states[0].first,canon.states[0].second,canon.states[1].first,canon.states[1].second,
                canon.states[2].first,canon.states[2].second,prefix.lifetimeBrands|brands};
            RootOption option{{udon::brand_count(prefix.lifetimeBrands|brands),udon::brand_count(brands),servings},
                {a.outcome.actions,b.outcome.actions,d.outcome.actions}};
            const auto found=roots.find(key);
            if(found==roots.end())roots.emplace(key,std::move(option));
            else if(found->second.today<option.today)found->second=std::move(option);
        }
        std::optional<udon::OfficialScore> best;const RootOption* selected=nullptr;
        for(const auto& [key,option]:roots){
            auto value=future.solve(root.dayNumber+1,key).value;
            value.totalDailyDistinct+=prefix.totalDailyDistinct+option.today.totalDailyDistinct;
            value.totalServings+=prefix.totalServings+option.today.totalServings;
            if(!best||*best<value){best=value;selected=&option;}
        }
        if(!selected)throw std::runtime_error("empty admissible portfolio");
        udon::DayPlan plan;plan.actions.assign(selected->plans.begin(),selected->plans.end());
        udon::SimulationResult sim;std::string mismatch;
        if(!validates(c,root,plan,sim,mismatch))throw std::runtime_error("joint portfolio witness invalid:"+mismatch);
        auto ledger=prefix;ledger.apply(sim.score);auto next=root;next.dayNumber++;next.agents=sim.finalAgents;
        auto suffix=future.run(next,ledger);
        if(suffix.at("score").dump()!=score_json(*best).dump())throw std::runtime_error("portfolio value reconstruction");
        J::Array days{J(J::Object{{"day",n(root.dayNumber)},{"plan",udon::serialize_day_plan(plan)},
            {"agents",agents_json(sim.finalAgents)},{"ledger",udon::serialize_match_ledger(c,ledger)},
            {"score",score_json({ledger.lifetime_distinct(),ledger.totalDailyDistinct,ledger.totalServings})}})};
        for(const auto& day:suffix.at("days").array())days.push_back(day);
        answers.emplace(label,J(J::Object{{"score",score_json(*best)},{"days",J(std::move(days))},
            {"columns_before",n(before)},{"unique_joint_roots",n(roots.size())},{"admissible_combinations",n(admissible)},
            {"incompatible_combinations",n(rejected)},{"memo_states",suffix.at("memo_states")},
            {"joint_transitions",suffix.at("joint_transitions")},{"complete",J(true)}}));
    }
    return J(J::Object{{"ok",J(true)},{"complete",J(true)},{"portfolios",J(std::move(answers))},
        {"dual_validated_columns",n(validatedColumns)},{"certificates_checked",n(certificateCount)}});
}
