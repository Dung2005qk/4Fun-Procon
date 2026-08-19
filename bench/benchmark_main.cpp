#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "udon/decision.hpp"
#include "udon/protocol.hpp"

namespace {

[[nodiscard]] int terrain_at(int row, int column) {
    if ((row + column) % 7 == 0) {
        return 1;
    }
    if ((row * 3 + column) % 19 == 0) {
        return 2;
    }
    return 0;
}

[[nodiscard]] std::string fixture_config() {
    constexpr int side = 32;
    std::vector<int> spots;
    for (int cell = 96; cell < side * side && spots.size() < 24U; ++cell) {
        if (terrain_at(cell / side, cell % side) == 0 && cell % 5 != 0) {
            spots.push_back(cell);
        }
    }
    if (spots.size() != 24U) {
        throw std::runtime_error("benchmark fixture cannot place all spots");
    }
    std::ostringstream output;
    output << "{\"startsAt\":1778227200,\"daySeconds\":[5,5,5,5,5,5],\"daySteps\":[128,128,128,128,128,128],";
    output << "\"map\":{\"height\":32,\"width\":32,\"cells\":[";
    for (int row = 0; row < side; ++row) {
        if (row != 0) {
            output << ',';
        }
        output << '[';
        for (int column = 0; column < side; ++column) {
            if (column != 0) {
                output << ',';
            }
            output << terrain_at(row, column);
        }
        output << ']';
    }
    output << "]},\"spots\":[";
    for (std::size_t index = 0; index < spots.size(); ++index) {
        if (index != 0U) {
            output << ',';
        }
        output << "{\"brand\":" << index << ",\"pos\":" << spots.at(index) << ",\"stocks\":4}";
    }
    output << "],\"agents\":[33,34,35,36,37,39,40,41],\"fuelLimits\":40,\"players\":3,";
    output << "\"busyThreshold\":4,\"jammedThreshold\":8}";
    return output.str();
}

[[nodiscard]] std::string fixture_state(const udon::MatchConfig& config) {
    std::ostringstream output;
    output << "{\"endsAt\":1778227205,\"day\":1,\"agents\":[";
    for (int agent = 0; agent < config.agent_count(); ++agent) {
        if (agent != 0) {
            output << ',';
        }
        output << "{\"kind\":" << (agent == config.agent_count() - 1 ? 1 : 0)
               << ",\"pos\":" << config.initialAgents.at(static_cast<std::size_t>(agent))
               << ",\"fuel\":" << config.fuelLimit << '}';
    }
    output << "],\"others\":[],\"traffics\":[";
    bool first = true;
    for (const udon::CellId road : config.roadCells) {
        if (!first) {
            output << ',';
        }
        first = false;
        output << "{\"pos\":" << road << ",\"status\":0}";
    }
    output << "]}";
    return output.str();
}

template <typename Function>
[[nodiscard]] std::chrono::milliseconds measure(Function&& function) {
    const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    function();
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started);
}

} 

int main() {
    try {
        const udon::MatchConfig config = udon::parse_match_config(udon::JsonValue::parse(fixture_config()));
        const udon::DayState state = udon::parse_day_state(config, udon::JsonValue::parse(fixture_state(config)));
        std::vector<udon::RoleAssignment> roles;
        const std::chrono::milliseconds roleTime = measure([&] {
            roles = udon::RoleAssignmentEnumerator(config).shortlist(3);
        });
        udon::UdonShieldEngine roleEngine(config);
        std::vector<udon::RoleAssignment> roleRollouts;
        const std::chrono::milliseconds roleRolloutTime = measure([&] {
            roleRollouts = roleEngine.select_roles_exhaustive_oracle(3);
        });

        const udon::ParetoRouter router(config);
        const udon::RouteColumnGenerator generator(config, router);
        const udon::ExactStepSimulator simulator(config);
        const udon::IndependentDayValidator validator(config);
        const udon::RouteMaster master(config, simulator, validator);
        udon::RoutePortfolio portfolio;
        const std::chrono::milliseconds generationTime = measure([&] {
            udon::ColumnGenerationOptions options;
            options.maximumPathsPerTarget = 3;
            options.maximumColumnsPerAgent = 8;
            options.maximumTargetSpots = 12;
            options.maximumEscorts = 16;
            portfolio = generator.generate(state, udon::MatchLedger{}, options);
        });
        udon::MasterDiagnostics diagnostics;
        std::vector<udon::MasterCandidate> candidates;
        const std::chrono::milliseconds masterTime = measure([&] {
            udon::MasterOptions options;
            options.maximumCombinations = 40000;
            options.maximumCandidates = 32;
            candidates = master.solve(state, udon::MatchLedger{}, portfolio, options, diagnostics);
        });

        udon::UdonShieldEngine engine(config);
        udon::DecisionResult decision;
        const std::chrono::milliseconds decisionTime = measure([&] {
            decision = engine.solve_day(state, udon::MatchLedger{}, std::chrono::milliseconds{5000});
        });
        const udon::ExactStepSimulator finalSimulator(config);
        const udon::SimulationResult finalResult = finalSimulator.simulate(state, decision.candidate.plan, false);
        if (!finalResult.valid) {
            throw std::runtime_error("benchmark decision became invalid");
        }
        std::cout << "role_scan_ms=" << roleTime.count() << " beam=" << roles.size() << '\n';
        std::cout << "role_rollout_ms=" << roleRolloutTime.count() << " beam=" << roleRollouts.size() << '\n';
        std::cout << "column_generation_ms=" << generationTime.count() << '\n';
        std::cout << "master_ms=" << masterTime.count() << " combinations=" << diagnostics.combinationsVisited
                  << " valid=" << diagnostics.simulatorValidCombinations
                  << " branches_pruned=" << diagnostics.branchesPruned
                  << " complete=" << diagnostics.searchComplete << '\n';
        std::cout << "decision_ms=" << decisionTime.count() << " emergency=" << decision.emergency
                  << " servings=" << finalResult.score.servings
                  << " score_today=" << decision.candidate.scoreAfterToday.lifetimeDistinct << ','
                  << decision.candidate.scoreAfterToday.totalDailyDistinct << ','
                  << decision.candidate.scoreAfterToday.totalServings
                  << " certified_lb=" << decision.profile.certifiedLowerBound.lifetimeDistinct << ','
                  << decision.profile.certifiedLowerBound.totalDailyDistinct << ','
                  << decision.profile.certifiedLowerBound.totalServings
                  << " w0_cache=" << decision.cacheRepair.reusedContingencies << '/' <<
                         decision.cacheRepair.eligibleContingencies
                  << '\n';
        std::cout << "decision_phases_ms=incumbent:" << decision.timing.incumbent.count()
                  << " fast:" << decision.timing.fastPath.count()
                  << " search:" << decision.timing.search.count()
                  << " prepare:" << decision.timing.candidatePreparation.count()
                  << " certify:" << decision.timing.certification.count()
                  << " total:" << decision.timing.total.count() << '\n';
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
