#include <cstdint>
#include <cstdlib>
#include <fstream>
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

void write_candidate(
    std::ostream& output,
    std::int32_t seed,
    std::size_t rank,
    const udon::MasterCandidate& candidate) {
    output << "candidate,seed=" << seed
              << ",rank=" << rank
              << ",score=" << candidate.scoreAfterToday.lifetimeDistinct
              << '/' << candidate.scoreAfterToday.totalDailyDistinct
              << '/' << candidate.scoreAfterToday.totalServings
              << ",slack=" << candidate.terminalSlack.worstRemainingBrandSteps
              << '/' << candidate.terminalSlack.totalRemainingBrandSteps
              << '/' << candidate.terminalSlack.patrolFuelReserve
              << '/' << candidate.terminalSlack.overnightSpotCount
              << ",traffic=" << candidate.trafficSafety.thresholdCrossings
              << '/' << candidate.trafficSafety.thresholdBandRoads
              << '/' << candidate.trafficSafety.totalRoadStays
              << ",agents=";
    for (const udon::AgentState& agent : candidate.simulation.finalAgents) {
        output << static_cast<std::int32_t>(agent.kind)
               << ':' << agent.position << ':' << agent.fuel << ';';
    }
    output << ",footprint=";
    for (const std::int32_t count : candidate.simulation.roadFootprint) {
        output << count << ';';
    }
    output << ",stable_id_size=" << candidate.stableId.size()
           << ",stable_id=" << candidate.stableId << '\n';
}

}  // namespace

int main(int argumentCount, char** arguments) {
    try {
        std::int32_t seedCount = 1000;
        std::string outputPath;
        for (int argument = 1; argument < argumentCount; ++argument) {
            const std::string value = arguments[argument];
            if (value == "--seeds" && argument + 1 < argumentCount) {
                seedCount = std::stoi(arguments[++argument]);
            } else if (value == "--output" && argument + 1 < argumentCount) {
                outputPath = arguments[++argument];
            } else {
                throw std::invalid_argument(
                    "usage: udonshield_population_oracle [--seeds N] [--output PATH]");
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

        std::ofstream outputFile;
        if (!outputPath.empty()) {
            outputFile.open(outputPath, std::ios::binary | std::ios::trunc);
            if (!outputFile) {
                throw std::runtime_error(
                    "unable to open population oracle output: " + outputPath);
            }
        }
        std::ostream& output = outputPath.empty()
            ? static_cast<std::ostream&>(std::cout)
            : static_cast<std::ostream&>(outputFile);

        std::int64_t totalCandidates = 0;
        std::int64_t totalCombinations = 0;
        output << "schema=udon-shield-population-oracle-v1,seeds="
               << seedCount << '\n';
        for (std::int32_t seed = 0; seed < seedCount; ++seed) {
            std::mt19937_64 random(
                static_cast<std::uint64_t>(seed) ^ 0x94d049bb133111ebULL);
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
                {udon::AgentKind::Patrol,
                 config.initialAgents.at(0),
                 config.fuelLimit},
                {seed % 3 == 0 ? udon::AgentKind::Patrol
                               : udon::AgentKind::Tanker,
                 config.initialAgents.at(1),
                 config.fuelLimit},
                {udon::AgentKind::Patrol,
                 config.initialAgents.at(2),
                 config.fuelLimit},
            };
            udon::MatchLedger ledger;
            for (std::int32_t brand = 0;
                 brand < config.brand_count();
                 ++brand) {
                if ((random() & 3U) == 0U) {
                    ledger.lifetimeBrands |= udon::brand_bit(brand);
                }
            }
            ledger.totalDailyDistinct =
                static_cast<std::int32_t>(random() % 16U);
            ledger.totalServings =
                static_cast<std::int32_t>(random() % 32U);

            udon::ColumnGenerationOptions generation;
            generation.maximumPathsPerTarget = 3;
            generation.maximumColumnsPerAgent = 8;
            generation.maximumTargetSpots = 8;
            generation.maximumEscorts = 4;
            const udon::RoutePortfolio portfolio = generator.generate(
                state,
                ledger,
                generation);

            udon::MasterOptions options;
            options.maximumCombinations = 4096;
            options.maximumCandidates = 16;
            options.diversityCandidates = 4;
            options.maximumResolveRounds = 2;
            options.enableLexicographicBranchAndBound = false;
            udon::MasterDiagnostics diagnostics;
            const std::vector<udon::MasterCandidate> candidates = master.solve(
                state,
                ledger,
                portfolio,
                options,
                diagnostics);
            if (!diagnostics.searchComplete || candidates.empty()) {
                throw std::runtime_error(
                    "population oracle incomplete at seed " +
                    std::to_string(seed));
            }
            totalCandidates += static_cast<std::int64_t>(candidates.size());
            totalCombinations += diagnostics.combinationsVisited;
            output << "population,seed=" << seed
                   << ",count=" << candidates.size()
                   << ",combinations=" << diagnostics.combinationsVisited
                   << '\n';
            for (std::size_t rank = 0; rank < candidates.size(); ++rank) {
                write_candidate(output, seed, rank, candidates.at(rank));
            }
        }
        output << "complete,seeds=" << seedCount
               << ",candidates=" << totalCandidates
               << ",combinations=" << totalCombinations << '\n';
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
