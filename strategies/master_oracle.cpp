#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

#include "udon/graph.hpp"
#include "udon/planner.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"

namespace {

[[nodiscard]] std::string oracle_config() {
    return R"({
        "startsAt":1778227200,
        "daySeconds":[5,5,5,5],
        "daySteps":[16,16,16,16],
        "map":{"height":8,"width":8,"cells":[
            [0,2,0,0,0,2,0,0],
            [0,0,0,1,0,0,0,0],
            [0,2,0,0,1,0,2,0],
            [0,0,0,2,0,0,0,0],
            [0,1,0,0,0,2,0,0],
            [0,0,2,0,0,0,1,0],
            [0,0,0,0,2,0,0,0],
            [0,2,0,0,0,0,0,0]
        ]},
        "spots":[
            {"brand":10,"pos":16,"stocks":2},
            {"brand":11,"pos":19,"stocks":1},
            {"brand":12,"pos":23,"stocks":2},
            {"brand":13,"pos":34,"stocks":1},
            {"brand":14,"pos":38,"stocks":2},
            {"brand":15,"pos":45,"stocks":1},
            {"brand":10,"pos":54,"stocks":2},
            {"brand":11,"pos":61,"stocks":2}
        ],
        "agents":[8,9,10],
        "fuelLimits":18,
        "players":3,
        "busyThreshold":3,
        "jammedThreshold":6
    })";
}

[[nodiscard]] bool same_best(
    const udon::MasterCandidate& left,
    const udon::MasterCandidate& right) {
    return left.scoreAfterToday == right.scoreAfterToday &&
        left.terminalSlack == right.terminalSlack &&
        left.trafficSafety == right.trafficSafety &&
        left.stableId == right.stableId;
}

}

int main(int argumentCount, char** arguments) {
    try {
        std::int32_t seedCount = 1000;
        for (int argument = 1; argument < argumentCount; ++argument) {
            const std::string value = arguments[argument];
            if (value == "--seeds" && argument + 1 < argumentCount) {
                seedCount = std::stoi(arguments[++argument]);
            } else {
                throw std::invalid_argument("usage: udonshield_master_oracle [--seeds N]");
            }
        }
        if (seedCount <= 0) {
            throw std::invalid_argument("oracle seed count must be positive");
        }

        const udon::MatchConfig config = udon::parse_match_config(
            udon::JsonValue::parse(oracle_config()));
        const udon::ParetoRouter router(config);
        const udon::RouteColumnGenerator generator(config, router);
        const udon::ExactStepSimulator simulator(config);
        const udon::IndependentDayValidator validator(config);
        const udon::RouteMaster master(config, simulator, validator);

        std::int64_t exhaustiveCombinations = 0;
        std::int64_t boundedCombinations = 0;
        std::int64_t branchesPruned = 0;
        for (std::int32_t seed = 0; seed < seedCount; ++seed) {
            std::mt19937_64 random(static_cast<std::uint64_t>(seed) ^ 0xd1b54a32d192ed03ULL);
            udon::DayState state;
            state.dayNumber = 1 + seed % config.day_count();
            state.roadStatuses.assign(
                static_cast<std::size_t>(config.map.cell_count()),
                udon::RoadStatus::Smooth);
            for (const udon::CellId road : config.roadCells) {
                state.roadStatuses.at(static_cast<std::size_t>(road)) =
                    static_cast<udon::RoadStatus>(random() % 3U);
            }
            state.agents = {
                {udon::AgentKind::Patrol, config.initialAgents.at(0), config.fuelLimit},
                {seed % 3 == 0 ? udon::AgentKind::Patrol : udon::AgentKind::Tanker,
                 config.initialAgents.at(1), config.fuelLimit},
                {udon::AgentKind::Patrol, config.initialAgents.at(2), config.fuelLimit},
            };
            udon::MatchLedger ledger;
            for (std::int32_t brand = 0; brand < config.brand_count(); ++brand) {
                if ((random() & 3U) == 0U) {
                    ledger.lifetimeBrands |= udon::brand_bit(brand);
                }
            }
            ledger.totalDailyDistinct = static_cast<std::int32_t>(random() % 16U);
            ledger.totalServings = static_cast<std::int32_t>(random() % 32U);

            udon::ColumnGenerationOptions generation;
            generation.maximumPathsPerTarget = 2;
            generation.maximumColumnsPerAgent = 4;
            generation.maximumTargetSpots = 8;
            generation.maximumEscorts = 4;
            const udon::RoutePortfolio portfolio = generator.generate(state, ledger, generation);

            udon::MasterOptions exhaustiveOptions;
            exhaustiveOptions.maximumCombinations = 100000;
            exhaustiveOptions.maximumCandidates = 100000;
            exhaustiveOptions.maximumResolveRounds = 1;
            exhaustiveOptions.enableLexicographicBranchAndBound = false;
            udon::MasterDiagnostics exhaustiveDiagnostics;
            const std::vector<udon::MasterCandidate> exhaustive = master.solve(
                state,
                ledger,
                portfolio,
                exhaustiveOptions,
                exhaustiveDiagnostics);

            udon::MasterOptions boundedOptions = exhaustiveOptions;
            boundedOptions.maximumCandidates = 1;
            boundedOptions.enableLexicographicBranchAndBound = true;
            udon::MasterDiagnostics boundedDiagnostics;
            const std::vector<udon::MasterCandidate> bounded = master.solve(
                state,
                ledger,
                portfolio,
                boundedOptions,
                boundedDiagnostics);

            if (!exhaustiveDiagnostics.searchComplete || !boundedDiagnostics.searchComplete ||
                exhaustive.empty() != bounded.empty() ||
                (!exhaustive.empty() && !same_best(exhaustive.front(), bounded.front()))) {
                throw std::runtime_error("branch-and-bound oracle mismatch at seed " + std::to_string(seed));
            }
            exhaustiveCombinations += exhaustiveDiagnostics.combinationsVisited;
            boundedCombinations += boundedDiagnostics.combinationsVisited;
            branchesPruned += boundedDiagnostics.branchesPruned;
        }

        const double reduction = exhaustiveCombinations == 0
            ? 0.0
            : 1.0 - static_cast<double>(boundedCombinations) /
                static_cast<double>(exhaustiveCombinations);
        std::cout << "schema=udon-shield-master-oracle-v1"
                  << ",seeds=" << seedCount
                  << ",exact_matches=" << seedCount
                  << ",exhaustive_combinations=" << exhaustiveCombinations
                  << ",bounded_combinations=" << boundedCombinations
                  << ",branches_pruned=" << branchesPruned
                  << ",combination_reduction=" << reduction
                  << '\n';
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
