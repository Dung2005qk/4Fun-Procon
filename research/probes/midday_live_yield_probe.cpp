// ATTR-MIDDAY-LIVE-YIELD-212 (read-only research probe).
//
// For every nonterminal day of the frozen live replays, reconstruct the
// exact decision-frame state/ledger/config and the SUBMITTED plan (actions
// frame) as the incumbent, then run the accepted 210 one-agent search
// structure offline with NO deadline at two candidate-cap tiers, counting
// why each candidate is rejected. The strict_protected_improvement
// certificate itself is never relaxed - the tiers only widen the candidate
// set the unchanged certificate judges.
//
// Usage: udonshield_midday_live_yield_probe <replay.jsonl> [...]

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "udon/btc_protocol.hpp"
#include "udon/json.hpp"
#include "udon/orienteering.hpp"
#include "udon/protocol.hpp"
#include "udon/simulator.hpp"
#include "udon/slack_refiner.hpp"
#include "udon/types.hpp"
#include "udon/validator.hpp"

namespace {

[[nodiscard]] std::uint64_t plan_hash(const udon::DayPlan& plan) {
    std::uint64_t hash = 1469598103934665603ULL;
    const auto mix = [&hash](std::uint64_t value) {
        hash ^= value;
        hash *= 1099511628211ULL;
    };
    mix(plan.actions.size());
    for (const udon::AgentPlan& actions : plan.actions) {
        mix(actions.size());
        for (const udon::PlanAction& action : actions) {
            mix(static_cast<std::uint32_t>(action.wire_value()));
        }
    }
    return hash;
}

struct TierCaps {
    const char* name;
    std::int32_t minimumSpots;  // -1 = production formula
    std::size_t maximumRoutes;
    std::uint64_t maximumSettledStates;
};

struct TierCounters {
    std::int64_t routes = 0;
    std::int64_t terminalMismatch = 0;
    std::int64_t duplicate = 0;
    std::int64_t generated = 0;
    std::int64_t invalid = 0;
    std::int64_t footprint = 0;
    std::int64_t fuel = 0;
    std::int64_t brand = 0;
    std::int64_t notStrict = 0;
    std::int64_t certified = 0;

    void add(const TierCounters& other) {
        routes += other.routes;
        terminalMismatch += other.terminalMismatch;
        duplicate += other.duplicate;
        generated += other.generated;
        invalid += other.invalid;
        footprint += other.footprint;
        fuel += other.fuel;
        brand += other.brand;
        notStrict += other.notStrict;
        certified += other.certified;
    }

    void print(const char* label) const {
        std::cout << label
                  << ",routes=" << routes
                  << ",terminal_mismatch=" << terminalMismatch
                  << ",duplicate=" << duplicate
                  << ",generated=" << generated
                  << ",invalid=" << invalid
                  << ",reject_footprint=" << footprint
                  << ",reject_fuel=" << fuel
                  << ",reject_brand=" << brand
                  << ",reject_not_strict=" << notStrict
                  << ",certified=" << certified << '\n';
    }
};

void probe_day(
    const udon::MatchConfig& config,
    const udon::DayState& state,
    const udon::MatchLedger& ledger,
    const udon::DayPlan& incumbent,
    const std::string& replayName,
    TierCounters* totals) {
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::SimulationResult incumbentSimulation =
        simulator.simulate(state, incumbent, false);
    const udon::SimulationResult independentIncumbent =
        validator.validate(state, incumbent, false);
    std::string incumbentMismatch;
    if (!incumbentSimulation.valid ||
        !validator.agrees_with(
            incumbentSimulation,
            independentIncumbent,
            incumbentMismatch)) {
        std::cout << "day_skip,replay=" << replayName
                  << ",day=" << state.dayNumber
                  << ",reason=incumbent-invalid\n";
        return;
    }

    std::uint64_t preferredBrands = 0U;
    for (std::int32_t brand = 0; brand < config.brand_count(); ++brand) {
        if (!udon::has_brand(ledger.lifetimeBrands, brand)) {
            preferredBrands |= udon::brand_bit(brand);
        }
    }
    if (preferredBrands == 0U) {
        for (std::int32_t brand = 0; brand < config.brand_count(); ++brand) {
            if (!udon::has_brand(incumbentSimulation.score.brands, brand)) {
                preferredBrands |= udon::brand_bit(brand);
            }
        }
    }
    const std::int32_t productionMinimumSpots = std::min<std::int32_t>(
        std::max(1, config.brand_count() - 1),
        static_cast<std::int32_t>(config.spots.size()));

    const TierCaps tiers[2] = {
        {"production", -1, 32U, 1250000ULL},
        {"raised", 1, 128U, 8000000ULL},
    };
    const std::uint64_t incumbentLifetime =
        ledger.lifetimeBrands | incumbentSimulation.score.brands;

    for (std::size_t tier = 0; tier < 2; ++tier) {
        const TierCaps& caps = tiers[tier];
        TierCounters counters;
        std::set<std::uint64_t> planHashes;
        planHashes.insert(plan_hash(incumbent));
        const std::int32_t minimumSpots =
            caps.minimumSpots < 0 ? productionMinimumSpots : caps.minimumSpots;
        for (udon::AgentIndex agent = 0;
             agent < config.agent_count();
             ++agent) {
            if (state.agents.at(static_cast<std::size_t>(agent)).kind !=
                udon::AgentKind::Patrol) {
                continue;
            }
            const udon::ExactOrienteeringReachability reach =
                udon::enumerate_sparse_anytime_resource_routes(
                    config,
                    state,
                    agent,
                    minimumSpots,
                    caps.maximumRoutes,
                    caps.maximumSettledStates,
                    std::nullopt,
                    preferredBrands);
            const udon::CellId terminal =
                incumbentSimulation.finalAgents.at(
                    static_cast<std::size_t>(agent)).position;
            const std::vector<udon::ExactOrienteeringRoute>* pools[2] = {
                &reach.maximalRoutes, &reach.supplementalRoutes};
            for (const auto* pool : pools) {
                for (const udon::ExactOrienteeringRoute& route : *pool) {
                    ++counters.routes;
                    if (route.terminalCell != terminal) {
                        ++counters.terminalMismatch;
                        continue;
                    }
                    udon::DayPlan candidate = incumbent;
                    candidate.actions.at(static_cast<std::size_t>(agent)) =
                        route.actions;
                    if (!planHashes.insert(plan_hash(candidate)).second) {
                        ++counters.duplicate;
                        continue;
                    }
                    ++counters.generated;
                    const udon::SimulationResult detailed =
                        simulator.simulate(state, candidate, false);
                    const udon::SimulationResult independent =
                        validator.validate(state, candidate, false);
                    std::string mismatch;
                    if (!detailed.valid ||
                        !validator.agrees_with(
                            detailed,
                            independent,
                            mismatch)) {
                        ++counters.invalid;
                        continue;
                    }
                    if (detailed.roadFootprint !=
                        incumbentSimulation.roadFootprint) {
                        ++counters.footprint;
                        continue;
                    }
                    if (!udon::protected_slack_agents_dominate(
                            incumbentSimulation.finalAgents,
                            detailed.finalAgents)) {
                        ++counters.fuel;
                        continue;
                    }
                    const std::uint64_t challengerLifetime =
                        ledger.lifetimeBrands | detailed.score.brands;
                    if ((incumbentLifetime & ~challengerLifetime) != 0U) {
                        ++counters.brand;
                        continue;
                    }
                    const bool geq =
                        detailed.score.dailyDistinct >=
                            incumbentSimulation.score.dailyDistinct &&
                        detailed.score.servings >=
                            incumbentSimulation.score.servings;
                    const bool strict =
                        detailed.score.dailyDistinct >
                            incumbentSimulation.score.dailyDistinct ||
                        detailed.score.servings >
                            incumbentSimulation.score.servings;
                    if (!geq || !strict) {
                        ++counters.notStrict;
                        continue;
                    }
                    ++counters.certified;
                    std::cout << "certified,replay=" << replayName
                              << ",tier=" << caps.name
                              << ",day=" << state.dayNumber
                              << ",agent=" << agent
                              << ",daily_delta="
                              << detailed.score.dailyDistinct -
                                     incumbentSimulation.score.dailyDistinct
                              << ",servings_delta="
                              << detailed.score.servings -
                                     incumbentSimulation.score.servings
                              << ",spots=" << route.spotMask
                              << ",used_steps=" << route.usedSteps << '\n';
                }
            }
        }
        std::cout << "day_tier,replay=" << replayName
                  << ",day=" << state.dayNumber
                  << ",tier=" << caps.name;
        counters.print("");
        totals[tier].add(counters);
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: udonshield_midday_live_yield_probe <replay.jsonl> [...]\n";
        return 2;
    }
    TierCounters totals[2];
    for (int argument = 1; argument < argc; ++argument) {
        const std::string path = argv[argument];
        std::ifstream input(path);
        if (!input) {
            std::cerr << "cannot open " << path << '\n';
            return 2;
        }
        std::optional<udon::MatchConfig> config;
        std::optional<udon::DayState> state;
        std::optional<udon::MatchLedger> ledger;
        std::string line;
        std::int64_t lineNumber = 0;
        while (std::getline(input, line)) {
            ++lineNumber;
            if (line.empty()) {
                continue;
            }
            try {
            const udon::JsonValue event = udon::JsonValue::parse(line);
            if (!event.is_object() || !event.object().contains("kind") ||
                !event.object().contains("body")) {
                continue;
            }
            const std::string kind = event.at("kind").string();
            const udon::JsonValue& body = event.at("body");
            if (kind == "setup") {
                config = udon::parse_btc_setup(
                    body,
                    udon::BtcAdapterOptions{5000});
            } else if (kind == "day_state") {
                if (!config.has_value()) {
                    continue;
                }
                state = udon::parse_btc_day_state(
                    *config,
                    body,
                    std::chrono::system_clock::time_point{},
                    udon::BtcAdapterOptions{5000});
            } else if (kind == "decision") {
                if (!config.has_value()) {
                    continue;
                }
                const udon::JsonValue& ledgerEcho = body.at("ledger");
                udon::MatchLedger parsed;
                for (const udon::JsonValue& brand :
                     ledgerEcho.at("brands").array()) {
                    parsed.lifetimeBrands |= udon::brand_bit(
                        static_cast<std::int32_t>(brand.integer()));
                }
                parsed.totalDailyDistinct = static_cast<std::int32_t>(
                    ledgerEcho.at("totalDailyDistinct").integer());
                parsed.totalServings = static_cast<std::int32_t>(
                    ledgerEcho.at("totalServings").integer());
                ledger = parsed;
            } else if (kind == "actions") {
                if (!config.has_value() || !state.has_value() ||
                    !ledger.has_value()) {
                    continue;
                }
                if (state->dayNumber >= config->day_count()) {
                    continue;  // nonterminal days only
                }
                const udon::DayPlan incumbent =
                    udon::parse_day_plan(*config, body);
                probe_day(*config, *state, *ledger, incumbent, path, totals);
            }
            } catch (const std::exception& error) {
                std::cerr << "line_error,replay=" << path
                          << ",line=" << lineNumber
                          << ",what=" << error.what() << '\n';
                return 3;
            }
        }
    }
    totals[0].print("total,tier=production");
    totals[1].print("total,tier=raised");
    return 0;
}
