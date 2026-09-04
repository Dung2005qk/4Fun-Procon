#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "udon/decision.hpp"
#include "udon/json.hpp"
#include "udon/orienteering.hpp"
#include "udon/protocol.hpp"
#include "udon/runtime.hpp"
#include "udon/simulator.hpp"
#include "udon/slack_refiner.hpp"
#include "udon/validator.hpp"

namespace {

constexpr std::chrono::milliseconds kProductionBudget{5000};
constexpr std::chrono::milliseconds kBtcNetworkReserve{1600};
constexpr std::int32_t kHarvestMode = 7;
constexpr std::int32_t kFutureHarvestMode = 7;
constexpr std::size_t kTrackedRoadCapacity = 2U;
using TrafficFootprint = std::array<std::uint8_t, kTrackedRoadCapacity>;
using TrafficStatusKey = std::array<std::uint8_t, kTrackedRoadCapacity>;

struct ManifestRow {
    std::string experimentId;
    std::string family;
    std::string fuelProfile;
    std::int32_t horizon = 0;
    std::uint64_t firstSeed = 0;
    std::int32_t count = 0;
    std::int32_t activeAgents = 2;
    std::int32_t totalAgents = 3;
    std::int32_t spotCount = 0;
    std::int32_t players = 4;
    std::string roleMode = "all-patrol";
    std::string oracleScope;
};

struct Options {
    std::string manifest = "research/holdouts/CEILING-MULTI-PATROL-085.csv";
    std::string split = "development";
    std::int32_t maximumMatches = 0;
    std::uint64_t onlySeed = 0;
    bool details = false;
    bool headOnly = false;
    bool protectedHead = false;
    bool latestTerminal = false;
    std::int32_t protectedRefinementReserveMs = 1600;
    bool boundaryDominance = false;
    bool trafficHistoryQuotient = false;
    std::int32_t proofTailDays = 0;
    bool rootStream = false;
    bool traceMembership = false;
    std::int32_t rootOwnBegin = 0;
    std::int32_t rootOwnEnd = -1;
    std::string inspectPlan;
    std::string inspectParentPlan;
    std::string forcedPrefix;
    std::string attributePrefix;
};

[[nodiscard]] bool is_three_active_patrol_experiment(
    const std::string_view experimentId) {
    return experimentId == "CEILING-THREE-ACTIVE-PATROL-306" ||
        experimentId ==
            "CEILING-THREE-ACTIVE-PATROL-LOW-FUEL-PREVALENCE-312";
}

struct Fixture {
    udon::MatchConfig config;
    std::string family;
    std::string fuelProfile;
    std::uint64_t seed = 0;
    bool trafficAware = false;
    bool adversarialTraffic = false;
    std::vector<TrafficFootprint> opponentFootprints;
};

struct DayOutcome {
    udon::CellId position = udon::kInvalidCell;
    std::int32_t fuel = 0;
    std::uint32_t spotMask = 0;
    udon::AgentPlan actions;
    TrafficFootprint roadFootprint{};
};

using AdversarialKey = std::tuple<
    std::int32_t,
    udon::CellId,
    std::int32_t,
    udon::CellId,
    std::int32_t,
    udon::BrandMask,
    TrafficFootprint,
    TrafficFootprint,
    TrafficStatusKey>;

struct MinimaxNode {
    udon::OfficialScore score;
    DayOutcome bestOwn;
    DayOutcome worstOpponent;
};

struct MinimaxDiagnostics {
    std::size_t states = 0;
    std::size_t transitions = 0;
    std::size_t maximumOwnOutcomes = 0;
    std::size_t maximumOpponentOutcomes = 0;
    std::size_t maximumOwnOutcomesBeforeDominance = 0;
    std::size_t boundaryDominancePruned = 0;
};

using MinimaxMemo = std::map<AdversarialKey, MinimaxNode>;
using AdversarialDayCacheKey = std::tuple<
    std::int32_t,
    udon::CellId,
    std::int32_t,
    TrafficStatusKey>;

struct MinimaxSearch {
    MinimaxMemo memo;
    MinimaxDiagnostics diagnostics;
    std::map<AdversarialDayCacheKey, std::vector<DayOutcome>> ownCache;
    std::map<AdversarialDayCacheKey, std::vector<DayOutcome>> opponentCache;
    bool boundaryDominance = false;
    bool trafficHistoryQuotient = false;
};

[[nodiscard]] std::string score_text(const udon::OfficialScore& score);

using MatchKey = std::tuple<
    udon::CellId,
    std::int32_t,
    udon::CellId,
    std::int32_t,
    udon::BrandMask,
    TrafficFootprint,
    TrafficStatusKey>;

struct LayerEntry {
    MatchKey key{};
    std::int32_t totalDailyDistinct = 0;
    std::int32_t totalServings = 0;
    std::int32_t parentIndex = -1;
    udon::AgentPlan firstPlan;
    udon::AgentPlan secondPlan;
    bool swapForNextDay = false;
};

using ThreeMatchKey = std::tuple<
    udon::CellId,
    std::int32_t,
    udon::CellId,
    std::int32_t,
    udon::CellId,
    std::int32_t,
    udon::BrandMask>;

struct ThreeLayerEntry {
    ThreeMatchKey key{};
    std::int32_t totalDailyDistinct = 0;
    std::int32_t totalServings = 0;
    std::int32_t parentIndex = -1;
    std::array<udon::AgentPlan, 3> plans;
    std::array<std::size_t, 3> nextToCurrent{0U, 1U, 2U};
};

struct OracleResult {
    bool valid = false;
    udon::OfficialScore score;
    std::vector<udon::DayPlan> plans;
    std::vector<udon::OfficialScore> cumulative;
    std::vector<std::vector<udon::AgentState>> terminalAgents;
    std::size_t maximumFrontier = 0;
    std::size_t dailyEnumerations = 0;
};

struct HeadResult {
    bool valid = false;
    udon::OfficialScore score;
    std::vector<udon::DayPlan> plans;
    std::vector<udon::OfficialScore> cumulative;
    std::vector<std::vector<udon::AgentState>> terminalAgents;
    std::vector<udon::DecisionAudit> audits;
    struct CachePathAudit {
        std::int32_t cached = 0;
        std::int32_t eligible = 0;
        std::int32_t reused = 0;
        std::int32_t rejected = 0;
        std::int32_t retained = 0;
        bool selected = false;
        bool cachedValid = false;
        udon::OfficialScore cachedCurrent;
        udon::OfficialScore cachedUpper;
        udon::OfficialScore selectedCurrent;
        udon::OfficialScore selectedUpper;
        std::int32_t sourceMultiDayWitnesses = 0;
        std::int32_t sourceMaximumPlans = 0;
        udon::OfficialScore sourceFinal;
        bool suffixAttempted = false;
        bool suffixValid = false;
        bool suffixScoreMatches = false;
        udon::OfficialScore suffixReplayScore;
    };
    std::vector<CachePathAudit> cachePaths;
};

struct Summary {
    std::int32_t cases = 0;
    std::int32_t oracleWins = 0;
    std::int32_t ties = 0;
    std::int32_t headWins = 0;
    std::int32_t invalid = 0;
    std::int32_t tier1 = 0;
    std::int32_t tier2 = 0;
    std::int32_t tier3 = 0;
    std::int32_t maximumGain = 0;
    std::uint64_t resultHash = 1469598103934665603ULL;
};

[[nodiscard]] std::uint64_t mix64(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

void hash_value(std::uint64_t& hash, std::uint64_t value) {
    for (std::int32_t byte = 0; byte < 8; ++byte) {
        hash ^= (value >> static_cast<std::uint32_t>(byte * 8)) & 0xffU;
        hash *= 1099511628211ULL;
    }
}

[[nodiscard]] std::string evaluator_contract_hash() {
    constexpr std::string_view contract =
        "lexicographic-risk-comparator-v1|exact-step-simulator-v1|independent-validator-v1";
    std::uint64_t hash = 14695981039346656037ULL;
    for (const char byte : contract) {
        hash ^= static_cast<std::uint8_t>(byte);
        hash *= 1099511628211ULL;
    }
    return "fnv1a64:" + std::to_string(hash);
}

[[nodiscard]] std::vector<std::string> split_csv(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream input(line);
    std::string field;
    while (std::getline(input, field, ',')) {
        fields.push_back(field);
    }
    return fields;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
    Options options;
    for (std::int32_t index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--manifest" && index + 1 < argc) {
            options.manifest = argv[++index];
        } else if (argument == "--split" && index + 1 < argc) {
            options.split = argv[++index];
        } else if (argument == "--maximum-matches" && index + 1 < argc) {
            options.maximumMatches = std::stoi(argv[++index]);
        } else if (argument == "--only-seed" && index + 1 < argc) {
            options.onlySeed = std::stoull(argv[++index]);
        } else if (argument == "--details") {
            options.details = true;
        } else if (argument == "--head-only") {
            options.headOnly = true;
        } else if (argument == "--protected-head") {
            options.protectedHead = true;
        } else if (argument == "--latest-terminal") {
            options.latestTerminal = true;
        } else if (argument == "--protected-refinement-reserve-ms" &&
                   index + 1 < argc) {
            options.protectedRefinementReserveMs = std::stoi(argv[++index]);
        } else if (argument == "--boundary-dominance" && index + 1 < argc) {
            const std::string enabled = argv[++index];
            if (enabled != "0" && enabled != "1") {
                throw std::invalid_argument("boundary-dominance must be 0 or 1");
            }
            options.boundaryDominance = enabled == "1";
        } else if (argument == "--traffic-history-quotient" &&
                   index + 1 < argc) {
            const std::string enabled = argv[++index];
            if (enabled != "0" && enabled != "1") {
                throw std::invalid_argument(
                    "traffic-history-quotient must be 0 or 1");
            }
            options.trafficHistoryQuotient = enabled == "1";
        } else if (argument == "--proof-tail-days" && index + 1 < argc) {
            options.proofTailDays = std::stoi(argv[++index]);
        } else if (argument == "--root-stream") {
            options.rootStream = true;
        } else if (argument == "--trace-membership") {
            options.traceMembership = true;
        } else if (argument == "--root-own-begin" && index + 1 < argc) {
            options.rootOwnBegin = std::stoi(argv[++index]);
        } else if (argument == "--root-own-end" && index + 1 < argc) {
            options.rootOwnEnd = std::stoi(argv[++index]);
        } else if (argument == "--inspect-plan" && index + 1 < argc) {
            options.inspectPlan = argv[++index];
        } else if (argument == "--inspect-parent-plan" && index + 1 < argc) {
            options.inspectParentPlan = argv[++index];
        } else if (argument == "--forced-prefix" && index + 1 < argc) {
            options.forcedPrefix = argv[++index];
        } else if (argument == "--attribute-prefix" && index + 1 < argc) {
            options.attributePrefix = argv[++index];
        } else {
            throw std::invalid_argument("unknown or incomplete option: " + argument);
        }
    }
    if (options.split != "development" && options.split != "holdout" &&
        options.split != "consumed") {
        throw std::invalid_argument(
            "split must be consumed, development or holdout");
    }
    if (options.maximumMatches < 0) {
        throw std::invalid_argument("maximum matches cannot be negative");
    }
    if (options.proofTailDays < 0 || options.proofTailDays > 2) {
        throw std::invalid_argument("proof-tail-days must be 0, 1 or 2");
    }
    if (options.rootOwnBegin < 0 || options.rootOwnEnd < -1 ||
        (options.rootOwnEnd >= 0 &&
         options.rootOwnEnd < options.rootOwnBegin)) {
        throw std::invalid_argument(
            "root own range must be a nonnegative half-open interval");
    }
    if (!options.rootStream &&
        (options.rootOwnBegin != 0 || options.rootOwnEnd != -1)) {
        throw std::invalid_argument(
            "root own range requires root-stream mode");
    }
    if (!options.inspectPlan.empty() && options.onlySeed == 0U) {
        throw std::invalid_argument("inspect-plan requires only-seed");
    }
    if (options.traceMembership &&
        (!options.rootStream || options.onlySeed == 0U ||
         options.rootOwnEnd != options.rootOwnBegin + 1)) {
        throw std::invalid_argument(
            "trace-membership requires root-stream, only-seed and a singleton "
            "root-own range");
    }
    if (!options.inspectParentPlan.empty() && options.inspectPlan.empty()) {
        throw std::invalid_argument(
            "inspect-parent-plan requires inspect-plan");
    }
    if (options.protectedHead && options.onlySeed == 0U) {
        throw std::invalid_argument("protected-head requires only-seed");
    }
    if (options.latestTerminal && !options.protectedHead) {
        throw std::invalid_argument(
            "latest-terminal requires protected-head");
    }
    if (options.protectedRefinementReserveMs != 0 &&
        options.protectedRefinementReserveMs != 1100 &&
        options.protectedRefinementReserveMs != 1600) {
        throw std::invalid_argument(
            "protected-refinement-reserve-ms must be 0, 1100 or 1600");
    }
    if (options.protectedRefinementReserveMs != 1600 &&
        !options.protectedHead) {
        throw std::invalid_argument(
            "non-default protected refinement reserve requires protected-head");
    }
    return options;
}

[[nodiscard]] std::vector<ManifestRow> load_manifest(
    const std::string& path,
    const std::string& split) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open manifest: " + path);
    }
    std::string line;
    if (!std::getline(input, line)) {
        throw std::runtime_error("multi-patrol oracle manifest is empty");
    }
    const bool legacySchema =
        line == "experiment_id,split,family,fuel_profile,horizon,first_seed,count,active_agents,total_agents,spot_count,role_mode,oracle_scope";
    const bool threeActiveSchema =
        line == "experiment_id,split,family,fuel_profile,players,horizon,first_seed,count,active_agents,total_agents,spot_count,role_mode,oracle_scope";
    if (!legacySchema && !threeActiveSchema) {
        throw std::runtime_error("unexpected multi-patrol oracle manifest schema");
    }
    std::vector<ManifestRow> rows;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::vector<std::string> fields = split_csv(line);
        if (threeActiveSchema) {
            if (fields.size() != 13U ||
                !is_three_active_patrol_experiment(fields.at(0))) {
                throw std::runtime_error("invalid three-patrol manifest row: " + line);
            }
            if (fields.at(1) != split) {
                continue;
            }
            const std::int32_t players = std::stoi(fields.at(4));
            if (players < 8 || players > 10 || fields.at(5) != "4" ||
                fields.at(8) != "3" || fields.at(9) != "3" ||
                fields.at(10) != "5" || fields.at(11) != "all-patrol" ||
                fields.at(12) !=
                    "complete-three-active-patrol-roadless-full-match-dp") {
                throw std::runtime_error(
                    "three-patrol manifest row violates exact scope: " + line);
            }
            rows.push_back(ManifestRow{
                fields.at(0),
                fields.at(2),
                fields.at(3),
                std::stoi(fields.at(5)),
                std::stoull(fields.at(6)),
                std::stoi(fields.at(7)),
                std::stoi(fields.at(8)),
                std::stoi(fields.at(9)),
                std::stoi(fields.at(10)),
                players,
                fields.at(11),
                fields.at(12),
            });
            continue;
        }
        if (fields.size() != 12U ||
            (fields.at(0) != "CEILING-MULTI-PATROL-085" &&
             fields.at(0) != "CEILING-BRANCH-PATROL-089" &&
             fields.at(0) != "CEILING-CYCLE-PATROL-095" &&
             fields.at(0) != "CEILING-TRAFFIC-PATROL-097" &&
             fields.at(0) != "CEILING-TRAFFIC-INDEPENDENT-124" &&
             fields.at(0) != "SCORE-MASTER-ADDITIVE-100" &&
             fields.at(0) != "CEILING-LADDER-PATROL-101" &&
             fields.at(0) != "SCORE-W1-TERMINAL-FRONTIER-109" &&
             fields.at(0) != "ATTR-W0-CACHE-RETENTION-110" &&
             fields.at(0) != "SCORE-W0-SUFFIX-PRESERVE-113" &&
              fields.at(0) != "SCORE-W1-CLOSED-LOOP-114" &&
              fields.at(0) != "SCORE-TRAFFIC-F0-UPPER-126" &&
             fields.at(0) != "SCORE-TRAFFIC-UPPER-LANE-119" &&
               fields.at(0) != "CEILING-TRAFFIC-MINIMAX-172" &&
               fields.at(0) != "ORACLE-BOUNDARY-DOMINANCE-175" &&
               fields.at(0) != "ORACLE-ROOT-STREAM-185" &&
               fields.at(0) != "ATTR-ORACLE-CURRENT-188" &&
               fields.at(0) != "ATTR-ORACLE-LATEST-192" &&
               fields.at(0) != "ATTR-ORACLE-LATEST-RESERVE-199" &&
               fields.at(0) != "ATTR-ORACLE-ROOT-CAUSAL-200" &&
               fields.at(0) != "ATTR-ORACLE-ROOT-CAUSAL-202" &&
               fields.at(0) != "ATTR-SUFFIX-TRAJECTORY-MEMBERSHIP-209" &&
               fields.at(0) != "CEILING-BOTTLENECK-PATROL-135")) {
            throw std::runtime_error("invalid manifest row: " + line);
        }
        if (fields.at(1) != split) {
            continue;
        }
        const std::string expectedSpots = fields.at(0) == "CEILING-MULTI-PATROL-085"
            ? "5"
            : "6";
        const bool traceMembership =
            fields.at(0) == "ATTR-SUFFIX-TRAJECTORY-MEMBERSHIP-209";
        const bool rootStream =
            fields.at(0) == "ORACLE-ROOT-STREAM-185" || traceMembership;
        const bool currentRevalidation =
            fields.at(0) == "ATTR-ORACLE-CURRENT-188" ||
            fields.at(0) == "ATTR-ORACLE-LATEST-192" ||
            fields.at(0) == "ATTR-ORACLE-LATEST-RESERVE-199" ||
            fields.at(0) == "ATTR-ORACLE-ROOT-CAUSAL-200" ||
            fields.at(0) == "ATTR-ORACLE-ROOT-CAUSAL-202";
        const bool boundaryDominance =
            fields.at(0) == "ORACLE-BOUNDARY-DOMINANCE-175" || rootStream ||
            currentRevalidation;
        const bool adversarial =
            fields.at(0) == "CEILING-TRAFFIC-MINIMAX-172" ||
            boundaryDominance;
        const std::string expectedScope = adversarial
            ? (currentRevalidation
                   ? (fields.at(0) == "ATTR-ORACLE-ROOT-CAUSAL-200" ||
                              fields.at(0) == "ATTR-ORACLE-ROOT-CAUSAL-202"
                          ? "current-production-exact-root-causal-attribution"
                          : (fields.at(0) == "ATTR-ORACLE-LATEST-RESERVE-199"
                          ? "current-production-protected-reserve-revalidation"
                          : (fields.at(0) == "ATTR-ORACLE-LATEST-192"
                                 ? "current-production-terminal-coordinate-revalidation"
                                 : "current-production-protected-revalidation")))
               : (traceMembership
                   ? "winning-root-trajectory-membership"
                   : (rootStream
                   ? "complete-public-maxmin-root-slice-stream"
                   : (boundaryDominance
                   ? "complete-public-maxmin-boundary-dominance-full-match-dp"
                   : "complete-public-maxmin-one-active-per-team-full-match-dp"))))
            : (fields.at(0) == "CEILING-TRAFFIC-PATROL-097" ||
                fields.at(0) == "CEILING-TRAFFIC-INDEPENDENT-124" ||
                fields.at(0) == "SCORE-TRAFFIC-F0-UPPER-126" ||
                fields.at(0) == "SCORE-TRAFFIC-UPPER-LANE-119"
            ? "complete-two-active-patrol-own-traffic-full-match-dp"
            : "complete-two-active-patrol-full-match-dp");
        const std::string expectedActive = adversarial ? "1" : "2";
        if (fields.at(7) != expectedActive || fields.at(8) != "3" ||
            fields.at(9) != expectedSpots ||
            fields.at(10) != "all-patrol" ||
            fields.at(11) != expectedScope) {
            throw std::runtime_error("manifest row violates oracle scope: " + line);
        }
        rows.push_back(ManifestRow{
            fields.at(0),
            fields.at(2),
            fields.at(3),
            std::stoi(fields.at(4)),
            std::stoull(fields.at(5)),
            std::stoi(fields.at(6)),
            std::stoi(fields.at(7)),
            std::stoi(fields.at(8)),
            std::stoi(fields.at(9)),
            adversarial ? 2 : 4,
            fields.at(10),
            fields.at(11),
        });
    }
    if (rows.empty()) {
        throw std::runtime_error("manifest contains no rows for split " + split);
    }
    return rows;
}

[[nodiscard]] std::vector<std::int32_t> base_brands(const std::string& family) {
    if (family == "duplicate-brand" || family == "three-duplicate") {
        return {100, 100, 200, 300, 400};
    }
    if (family == "coverage-trap" || family == "three-coverage") {
        return {100, 100, 200, 200, 999};
    }
    if (family == "terminal-separation" || family == "three-terminal") {
        return {100, 200, 200, 300, 900};
    }
    if (family == "balanced" || family == "stock-contention" ||
        family == "fuel-allocation" || family == "three-balanced" ||
        family == "three-stock" || family == "three-fuel") {
        return {100, 200, 300, 400, 500};
    }
    if (family == "branched-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "rare-fork") {
        return {100, 100, 200, 200, 300, 999};
    }
    if (family == "terminal-fork") {
        return {100, 200, 300, 300, 400, 900};
    }
    if (family == "branched-balanced" || family == "stock-race" ||
        family == "fuel-split") {
        return {100, 200, 300, 400, 500, 600};
    }
    if (family == "cycle-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "rare-loop") {
        return {100, 100, 200, 200, 300, 999};
    }
    if (family == "terminal-loop") {
        return {100, 200, 300, 300, 400, 900};
    }
    if (family == "cycle-balanced" || family == "stock-crossing" ||
        family == "fuel-circuit") {
        return {100, 200, 300, 400, 500, 600};
    }
    if (family == "perimeter-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "rare-perimeter") {
        return {100, 100, 200, 200, 300, 999};
    }
    if (family == "terminal-perimeter") {
        return {100, 200, 300, 300, 400, 900};
    }
    if (family == "perimeter-balanced" || family == "stock-perimeter" ||
        family == "fuel-perimeter") {
        return {100, 200, 300, 400, 500, 600};
    }
    if (family == "ladder-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "rare-ladder") {
        return {100, 100, 200, 200, 300, 999};
    }
    if (family == "terminal-ladder") {
        return {100, 200, 300, 300, 400, 900};
    }
    if (family == "ladder-balanced" || family == "stock-ladder" ||
        family == "fuel-ladder") {
        return {100, 200, 300, 400, 500, 600};
    }
    if (family == "traffic-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "traffic-terminal") {
        return {100, 200, 300, 300, 400, 900};
    }
    if (family == "traffic-balanced" || family == "threshold-loop" ||
        family == "jam-loop" || family == "traffic-stock") {
        return {100, 200, 300, 400, 500, 600};
    }
    if (family == "adversarial-balanced") {
        return {100, 200, 300, 400, 500, 600};
    }
    if (family == "adversarial-threshold") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "adversarial-terminal") {
        return {100, 200, 300, 300, 400, 900};
    }
    if (family == "diamond-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "rare-diamond") {
        return {100, 100, 200, 200, 300, 999};
    }
    if (family == "terminal-diamond") {
        return {100, 200, 200, 300, 400, 900};
    }
    if (family == "diamond-balanced" || family == "stock-diamond" ||
        family == "fuel-diamond") {
        return {100, 200, 300, 400, 500, 600};
    }
    if (family == "bottleneck-duplicate") {
        return {100, 100, 200, 300, 400, 500};
    }
    if (family == "rare-bottleneck") {
        return {100, 100, 200, 200, 300, 999};
    }
    if (family == "terminal-bottleneck") {
        return {100, 200, 300, 300, 400, 900};
    }
    if (family == "bottleneck-balanced" ||
        family == "stock-bottleneck" ||
        family == "fuel-bottleneck") {
        return {100, 200, 300, 400, 500, 600};
    }
    throw std::invalid_argument("unknown family: " + family);
}

[[nodiscard]] Fixture make_fixture(const ManifestRow& row, std::uint64_t seed) {
    constexpr std::int32_t side = 8;
    constexpr std::int32_t cells = side * side;
    const bool threeActive =
        is_three_active_patrol_experiment(row.experimentId);
    std::vector<std::int32_t> terrain(
        static_cast<std::size_t>(cells),
        static_cast<std::int32_t>(udon::Terrain::Pond));
    terrain.at(0) = static_cast<std::int32_t>(udon::Terrain::Plain);
    const bool branched =
        row.experimentId == "CEILING-BRANCH-PATROL-089" ||
        row.experimentId == "SCORE-W1-TERMINAL-FRONTIER-109" ||
        row.experimentId == "ATTR-W0-CACHE-RETENTION-110";
    const bool cyclic =
        row.experimentId == "CEILING-CYCLE-PATROL-095" ||
        row.experimentId == "SCORE-W0-SUFFIX-PRESERVE-113";
    const bool adversarialTraffic =
        row.experimentId == "CEILING-TRAFFIC-MINIMAX-172" ||
        row.experimentId == "ORACLE-BOUNDARY-DOMINANCE-175" ||
        row.experimentId == "ORACLE-ROOT-STREAM-185" ||
        row.experimentId == "ATTR-ORACLE-CURRENT-188" ||
        row.experimentId == "ATTR-ORACLE-LATEST-192" ||
        row.experimentId == "ATTR-ORACLE-LATEST-RESERVE-199" ||
        row.experimentId == "ATTR-ORACLE-ROOT-CAUSAL-200" ||
        row.experimentId == "ATTR-ORACLE-ROOT-CAUSAL-202" ||
        row.experimentId == "ATTR-SUFFIX-TRAJECTORY-MEMBERSHIP-209";
    if (adversarialTraffic) {
        terrain.at(7) = static_cast<std::int32_t>(udon::Terrain::Plain);
    }
    const bool trafficAware =
        row.experimentId == "CEILING-TRAFFIC-PATROL-097" ||
        row.experimentId == "CEILING-TRAFFIC-INDEPENDENT-124" ||
        row.experimentId == "SCORE-TRAFFIC-F0-UPPER-126" ||
        row.experimentId == "SCORE-TRAFFIC-UPPER-LANE-119" ||
        adversarialTraffic;
    const bool perimeter = row.experimentId == "SCORE-MASTER-ADDITIVE-100";
    const bool ladder = row.experimentId == "CEILING-LADDER-PATROL-101";
    const bool diamond = row.experimentId == "SCORE-W1-CLOSED-LOOP-114";
    const bool bottleneck =
        row.experimentId == "CEILING-BOTTLENECK-PATROL-135";
    std::vector<udon::CellId> spotCells;
    if (bottleneck) {
        for (const udon::CellId cell :
             {17, 18, 20, 21, 25, 26, 27, 28, 29, 36}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        terrain.at(27) = static_cast<std::int32_t>(udon::Terrain::Mountain);
        spotCells = {17, 18, 20, 21, 26, 29};
    } else if (diamond) {
        for (const udon::CellId cell :
             {17, 18, 19, 20, 25, 26, 27, 28, 34, 35, 36}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        terrain.at(27) = static_cast<std::int32_t>(udon::Terrain::Mountain);
        spotCells = {17, 20, 25, 28, 34, 36};
    } else if (trafficAware) {
        for (const udon::CellId cell : {17, 18, 19, 20, 25, 27, 28, 33, 34, 35, 36}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        terrain.at(18) = static_cast<std::int32_t>(udon::Terrain::Road);
        terrain.at(27) = static_cast<std::int32_t>(udon::Terrain::Mountain);
        terrain.at(35) = static_cast<std::int32_t>(udon::Terrain::Road);
        if (adversarialTraffic) {
            terrain.at(18) = static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        spotCells = {17, 19, 20, 33, 34, 36};
    } else if (ladder) {
        for (const udon::CellId cell : {18, 19, 20, 21, 26, 27, 28, 29}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        spotCells = {19, 20, 21, 26, 27, 28};
    } else if (perimeter) {
        for (const udon::CellId cell : {18, 19, 20, 21, 26, 29, 34, 35, 36, 37}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        terrain.at(26) = static_cast<std::int32_t>(udon::Terrain::Mountain);
        spotCells = {19, 20, 21, 29, 35, 36};
    } else if (cyclic) {
        for (const udon::CellId cell : {18, 19, 20, 26, 27, 28, 34, 35, 36}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        terrain.at(27) = static_cast<std::int32_t>(udon::Terrain::Mountain);
        spotCells = {19, 20, 26, 28, 34, 35};
    } else if (branched) {
        for (const udon::CellId cell : {10, 11, 16, 17, 18, 19, 20, 21, 26}) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        terrain.at(11) = static_cast<std::int32_t>(udon::Terrain::Mountain);
        spotCells = {10, 17, 18, 19, 20, 26};
    } else {
        for (udon::CellId cell = 16; cell <= (threeActive ? 23 : 22); ++cell) {
            terrain.at(static_cast<std::size_t>(cell)) =
                static_cast<std::int32_t>(udon::Terrain::Plain);
        }
        spotCells = {17, 18, 19, 20, 21};
    }
    std::sort(
        spotCells.begin(),
        spotCells.end(),
        [seed](udon::CellId left, udon::CellId right) {
            return mix64(seed ^ (static_cast<std::uint64_t>(left) << 17U)) <
                mix64(seed ^ (static_cast<std::uint64_t>(right) << 17U));
        });
    std::vector<std::int32_t> brands = base_brands(row.family);
    if (row.family == "terminal-separation" || row.family == "three-terminal" ||
        row.family == "terminal-fork" ||
        row.family == "terminal-loop" || row.family == "traffic-terminal" ||
        row.family == "terminal-perimeter" || row.family == "terminal-ladder" ||
        row.family == "terminal-diamond" ||
        row.family == "adversarial-terminal" ||
        row.family == "terminal-bottleneck") {
        const udon::CellId protectedTerminal = trafficAware
            ? 33
            : (bottleneck
                   ? 29
                   : (diamond
                          ? 36
                          : (ladder
                                 ? 28
                                 : (perimeter
                                        ? 36
                                        : (cyclic ? 35 : (branched ? 26 : 19))))));
        const auto center = std::find(
            spotCells.begin(),
            spotCells.end(),
            protectedTerminal);
        if (center != spotCells.end()) {
            std::iter_swap(center, spotCells.end() - 1);
        }
    }

    std::vector<std::int32_t> daySteps;
    for (std::int32_t day = 0; day < row.horizon; ++day) {
        daySteps.push_back(16 + static_cast<std::int32_t>(
            mix64(seed ^ (static_cast<std::uint64_t>(day) << 39U)) % 3U));
    }
    std::int32_t fuelLimit =
        (branched || cyclic || trafficAware || perimeter || diamond ||
         bottleneck)
        ? 24
        : 20;
    if (row.fuelProfile == "low") {
        fuelLimit =
            (branched || cyclic || trafficAware || perimeter || diamond ||
             bottleneck)
            ? 12
            : 10;
    } else if (row.fuelProfile == "high") {
        fuelLimit = 8 * row.horizon;
    } else if (row.fuelProfile != "default") {
        throw std::invalid_argument("unknown fuel profile: " + row.fuelProfile);
    }

    std::ostringstream document;
    document << "{\"startsAt\":1778227200,\"daySeconds\":[";
    for (std::int32_t day = 0; day < row.horizon; ++day) {
        if (day != 0) {
            document << ',';
        }
        document << 5;
    }
    document << "],\"daySteps\":[";
    for (std::size_t day = 0; day < daySteps.size(); ++day) {
        if (day != 0U) {
            document << ',';
        }
        document << daySteps.at(day);
    }
    document << "],\"map\":{\"height\":8,\"width\":8,\"cells\":[";
    for (std::int32_t mapRow = 0; mapRow < side; ++mapRow) {
        if (mapRow != 0) {
            document << ',';
        }
        document << '[';
        for (std::int32_t column = 0; column < side; ++column) {
            if (column != 0) {
                document << ',';
            }
            document << terrain.at(static_cast<std::size_t>(mapRow * side + column));
        }
        document << ']';
    }
    document << "]},\"spots\":[";
    for (std::size_t spot = 0; spot < spotCells.size(); ++spot) {
        if (spot != 0U) {
            document << ',';
        }
        std::int32_t stock = 2;
        if (row.family == "stock-contention" || row.family == "three-stock" ||
            row.family == "coverage-trap" || row.family == "three-coverage" ||
            row.family == "stock-race" || row.family == "rare-fork" ||
            row.family == "stock-crossing" || row.family == "rare-loop" ||
            row.family == "traffic-stock" || row.family == "stock-perimeter" ||
            row.family == "rare-perimeter" || row.family == "stock-ladder" ||
            row.family == "rare-ladder" || row.family == "stock-diamond" ||
            row.family == "rare-diamond" ||
            row.family == "stock-bottleneck" ||
            row.family == "rare-bottleneck") {
            stock = 1;
        } else if (row.family == "fuel-allocation" || row.family == "three-fuel" ||
            row.family == "fuel-split" ||
            row.family == "fuel-circuit" || row.family == "fuel-perimeter" ||
            row.family == "fuel-ladder" || row.family == "fuel-diamond" ||
            row.family == "fuel-bottleneck") {
            stock = 1 + static_cast<std::int32_t>((spot + seed) % 2U);
        }
        document << "{\"brand\":" << brands.at(spot)
                 << ",\"pos\":" << spotCells.at(spot)
                 << ",\"stocks\":" << stock << '}';
    }
    document << "],\"agents\":[" << (bottleneck ? 25 : (diamond ? 18 : (trafficAware ? 25 : (ladder ? 18 : (perimeter ? 18 : (cyclic ? 18 : 16)))))) << ','
             << (adversarialTraffic ? 0 : (bottleneck ? 36 : (diamond ? 35 : (trafficAware ? 28 : (ladder ? 29 : (perimeter ? 37 : (cyclic ? 36 : (branched ? 21 : 22))))))))
             << ',' << (threeActive ? 23 : (adversarialTraffic ? 7 : 0))
             << "],\"fuelLimits\":" << fuelLimit
             << ",\"players\":"
             << (threeActive ? row.players : (adversarialTraffic ? 2 : 4))
             << ",\"busyThreshold\":2,\"jammedThreshold\":4}";

    Fixture fixture;
    fixture.config = udon::parse_match_config(udon::JsonValue::parse(document.str()));
    fixture.family = row.family;
    fixture.fuelProfile = row.fuelProfile;
    fixture.seed = seed;
    fixture.trafficAware = trafficAware;
    fixture.adversarialTraffic = adversarialTraffic;
    fixture.opponentFootprints.assign(
        static_cast<std::size_t>(row.horizon),
        TrafficFootprint{});
    if (fixture.config.roadCells.size() > kTrackedRoadCapacity) {
        throw std::runtime_error("traffic oracle road capacity exceeded");
    }
    if (trafficAware && !adversarialTraffic) {
        for (std::int32_t day = 0; day < row.horizon; ++day) {
            for (std::size_t roadIndex = 0;
                 roadIndex < fixture.config.roadCells.size();
                 ++roadIndex) {
                const udon::CellId road = fixture.config.roadCells.at(roadIndex);
                const std::uint64_t draw = mix64(
                    seed ^ (static_cast<std::uint64_t>(day) << 41U) ^
                    (static_cast<std::uint64_t>(road) << 13U));
                std::int32_t stays = static_cast<std::int32_t>(draw % 4U);
                if (row.family == "threshold-loop") {
                    stays = fixture.config.players * fixture.config.busyThreshold - 3 +
                        static_cast<std::int32_t>(draw % 3U);
                } else if (row.family == "jam-loop") {
                    stays = fixture.config.players * fixture.config.jammedThreshold - 4 +
                        static_cast<std::int32_t>(draw % 4U);
                }
                fixture.opponentFootprints.at(static_cast<std::size_t>(day)).at(
                    roadIndex) = static_cast<std::uint8_t>(std::clamp(
                        stays,
                        0,
                        fixture.config.players * fixture.config.jammedThreshold));
            }
        }
    }
    if (!trafficAware && !fixture.config.roadCells.empty()) {
        throw std::runtime_error("multi-patrol exact domain unexpectedly contains roads");
    }
    if (threeActive &&
        (fixture.config.agent_count() != 3 || fixture.config.spots.size() != 5U ||
         fixture.config.players != row.players)) {
        throw std::runtime_error("three-patrol fixture violates frozen domain");
    }
    return fixture;
}

[[nodiscard]] udon::DayState day_state(
    const udon::MatchConfig& config,
    std::int32_t day,
    const std::vector<udon::AgentState>& agents,
    const std::vector<udon::RoadStatus>& roadStatuses) {
    udon::DayState state;
    state.endsAt = config.startsAt + static_cast<std::int64_t>(day) * 5;
    state.dayNumber = day;
    state.agents = agents;
    state.roadStatuses = roadStatuses;
    return state;
}

[[nodiscard]] udon::DayState day_state(
    const udon::MatchConfig& config,
    std::int32_t day,
    const std::vector<udon::AgentState>& agents) {
    return day_state(
        config,
        day,
        agents,
        std::vector<udon::RoadStatus>(
            static_cast<std::size_t>(config.map.cell_count()),
            udon::RoadStatus::Smooth));
}

[[nodiscard]] std::vector<udon::RoadStatus> exact_road_statuses(
    const Fixture& fixture,
    std::int32_t day,
    const TrafficFootprint& previousOwn,
    const TrafficFootprint& priorOwn) {
    std::vector<udon::RoadStatus> statuses(
        static_cast<std::size_t>(fixture.config.map.cell_count()),
        udon::RoadStatus::Smooth);
    if (!fixture.trafficAware) {
        return statuses;
    }
    for (std::size_t roadIndex = 0;
         roadIndex < fixture.config.roadCells.size();
         ++roadIndex) {
        const udon::CellId road = fixture.config.roadCells.at(roadIndex);
        const std::size_t cellOffset = static_cast<std::size_t>(road);
        std::int32_t stays = previousOwn.at(roadIndex) + priorOwn.at(roadIndex);
        for (std::int32_t history = 1; history <= 2; ++history) {
            const std::int32_t completedDay = day - history;
            if (completedDay > 0) {
                stays += fixture.opponentFootprints
                    .at(static_cast<std::size_t>(completedDay - 1))
                    .at(roadIndex);
            }
        }
        if (stays >= fixture.config.players * fixture.config.jammedThreshold) {
            statuses.at(cellOffset) = udon::RoadStatus::Jammed;
        } else if (stays >= fixture.config.players * fixture.config.busyThreshold) {
            statuses.at(cellOffset) = udon::RoadStatus::Busy;
        }
    }
    return statuses;
}

[[nodiscard]] std::size_t road_offset(
    const udon::MatchConfig& config,
    udon::CellId road) {
    const auto found = std::find(
        config.roadCells.begin(),
        config.roadCells.end(),
        road);
    if (found == config.roadCells.end()) {
        throw std::runtime_error("road footprint requested for a non-road cell");
    }
    return static_cast<std::size_t>(found - config.roadCells.begin());
}

[[nodiscard]] TrafficFootprint compact_road_footprint(
    const udon::MatchConfig& config,
    const std::vector<std::int32_t>& fullFootprint) {
    TrafficFootprint compact{};
    const std::int32_t saturation = config.players * config.jammedThreshold;
    for (std::size_t roadIndex = 0;
         roadIndex < config.roadCells.size();
         ++roadIndex) {
        compact.at(roadIndex) = static_cast<std::uint8_t>(std::clamp(
            fullFootprint.at(
                static_cast<std::size_t>(config.roadCells.at(roadIndex))),
            0,
            saturation));
    }
    return compact;
}

[[nodiscard]] TrafficStatusKey compact_road_statuses(
    const udon::MatchConfig& config,
    const std::vector<udon::RoadStatus>& statuses) {
    TrafficStatusKey compact{};
    for (std::size_t roadIndex = 0;
         roadIndex < config.roadCells.size();
         ++roadIndex) {
        compact.at(roadIndex) = static_cast<std::uint8_t>(
            statuses.at(static_cast<std::size_t>(
                config.roadCells.at(roadIndex))));
    }
    return compact;
}

[[nodiscard]] std::vector<udon::RoadStatus> expand_road_statuses(
    const udon::MatchConfig& config,
    const TrafficStatusKey& compact) {
    std::vector<udon::RoadStatus> statuses(
        static_cast<std::size_t>(config.map.cell_count()),
        udon::RoadStatus::Smooth);
    for (std::size_t roadIndex = 0;
         roadIndex < config.roadCells.size();
         ++roadIndex) {
        statuses.at(static_cast<std::size_t>(
            config.roadCells.at(roadIndex))) =
            static_cast<udon::RoadStatus>(compact.at(roadIndex));
    }
    return statuses;
}

void add_traffic_stays(
    const udon::MatchConfig& config,
    TrafficFootprint& footprint,
    std::size_t roadIndex,
    std::int32_t stays) {
    const std::int32_t saturation = config.players * config.jammedThreshold;
    footprint.at(roadIndex) = static_cast<std::uint8_t>(std::min(
        saturation,
        static_cast<std::int32_t>(footprint.at(roadIndex)) + stays));
}

[[nodiscard]] bool validates(
    const udon::MatchConfig& config,
    const udon::DayState& state,
    const udon::DayPlan& plan,
    udon::SimulationResult& simulation,
    std::string& mismatch) {
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    simulation = simulator.simulate(state, plan, false);
    const udon::SimulationResult independent = validator.validate(state, plan, false);
    return simulation.valid && independent.valid &&
        validator.agrees_with(simulation, independent, mismatch);
}

[[nodiscard]] std::vector<DayOutcome> enumerate_day(
    const udon::MatchConfig& config,
    std::int32_t day,
    udon::CellId start,
    std::int32_t availableFuel,
    const std::vector<udon::RoadStatus>& roadStatuses,
    bool pruneDominated = true) {
    struct Node {
        std::int32_t steps = 0;
        std::int32_t fuelUsed = 0;
        udon::CellId cell = udon::kInvalidCell;
        std::uint32_t spotMask = 0;
        TrafficFootprint roadFootprint{};
        std::int32_t parent = -1;
        udon::PlanAction incoming = udon::PlanAction::wait(1);
    };
    const std::int32_t limit = config.steps_for_day(day);
    std::vector<Node> nodes{
        Node{
            0,
            0,
            start,
            0U,
            TrafficFootprint{},
            -1,
            udon::PlanAction::wait(1)},
    };
    using NodeKey = std::tuple<
        std::int32_t,
        std::int32_t,
        udon::CellId,
        std::uint32_t,
        TrafficFootprint>;
    std::set<NodeKey> seen;
    seen.emplace(0, 0, start, 0U, nodes.front().roadFootprint);
    const auto push = [&nodes, &seen](const Node& node) {
        if (seen.emplace(
                node.steps,
                node.fuelUsed,
                node.cell,
                node.spotMask,
                node.roadFootprint).second) {
            nodes.push_back(node);
        }
    };
    for (std::size_t cursor = 0; cursor < nodes.size(); ++cursor) {
        const Node current = nodes.at(cursor);
        const udon::SpotIndex currentSpot =
            config.spotAtCell.at(static_cast<std::size_t>(current.cell));
        if (currentSpot != udon::kInvalidSpot && current.steps < limit &&
            (current.spotMask &
             (std::uint32_t{1} << static_cast<std::uint32_t>(currentSpot))) == 0U) {
            Node waited = current;
            ++waited.steps;
            if (config.map.terrain.at(static_cast<std::size_t>(waited.cell)) ==
                udon::Terrain::Road) {
                add_traffic_stays(
                    config,
                    waited.roadFootprint,
                    road_offset(config, waited.cell),
                    1);
            }
            waited.spotMask |=
                std::uint32_t{1} << static_cast<std::uint32_t>(currentSpot);
            waited.parent = static_cast<std::int32_t>(cursor);
            waited.incoming = udon::PlanAction::wait(1);
            push(waited);
        }
        const udon::MoveCost cost = config.move_cost(
            current.cell,
            roadStatuses.at(static_cast<std::size_t>(current.cell)));
        if (cost.steps <= 0 || current.steps + cost.steps > limit ||
            current.fuelUsed + cost.patrolFuel > availableFuel) {
            continue;
        }
        for (std::int32_t direction = 0; direction < udon::kDirectionCount; ++direction) {
            const udon::CellId destination = config.map.neighbors
                .at(static_cast<std::size_t>(current.cell))
                .at(static_cast<std::size_t>(direction));
            if (destination == udon::kInvalidCell ||
                config.map.terrain.at(static_cast<std::size_t>(destination)) ==
                    udon::Terrain::Pond) {
                continue;
            }
            Node moved = current;
            moved.steps += cost.steps;
            moved.fuelUsed += cost.patrolFuel;
            moved.cell = destination;
            if (config.map.terrain.at(static_cast<std::size_t>(current.cell)) ==
                udon::Terrain::Road) {
                add_traffic_stays(
                    config,
                    moved.roadFootprint,
                    road_offset(config, current.cell),
                    cost.steps - 1);
            }
            if (config.map.terrain.at(static_cast<std::size_t>(destination)) ==
                udon::Terrain::Road) {
                add_traffic_stays(
                    config,
                    moved.roadFootprint,
                    road_offset(config, destination),
                    1);
            }
            const udon::SpotIndex spot =
                config.spotAtCell.at(static_cast<std::size_t>(destination));
            if (spot != udon::kInvalidSpot) {
                moved.spotMask |=
                    std::uint32_t{1} << static_cast<std::uint32_t>(spot);
            }
            moved.parent = static_cast<std::int32_t>(cursor);
            moved.incoming = udon::PlanAction::move(direction);
            push(moved);
        }
    }

    using OutcomeKey = std::tuple<
        udon::CellId,
        std::int32_t,
        std::uint32_t,
        TrafficFootprint>;
    std::map<OutcomeKey, std::int32_t> compressed;
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const Node& node = nodes.at(index);
        TrafficFootprint finalFootprint = node.roadFootprint;
        if (config.map.terrain.at(static_cast<std::size_t>(node.cell)) ==
            udon::Terrain::Road) {
            add_traffic_stays(
                config,
                finalFootprint,
                road_offset(config, node.cell),
                limit - node.steps);
        }
        const OutcomeKey key{
            node.cell,
            availableFuel - node.fuelUsed,
            node.spotMask,
            std::move(finalFootprint)};
        const auto found = compressed.find(key);
        if (found == compressed.end() ||
            nodes.at(static_cast<std::size_t>(found->second)).steps > node.steps) {
            compressed[key] = static_cast<std::int32_t>(index);
        }
    }

    std::vector<DayOutcome> outcomes;
    for (const auto& [key, witness] : compressed) {
        const Node& terminal = nodes.at(static_cast<std::size_t>(witness));
        udon::AgentPlan reversed;
        std::int32_t current = witness;
        while (nodes.at(static_cast<std::size_t>(current)).parent >= 0) {
            reversed.push_back(nodes.at(static_cast<std::size_t>(current)).incoming);
            current = nodes.at(static_cast<std::size_t>(current)).parent;
        }
        std::reverse(reversed.begin(), reversed.end());
        if (terminal.steps < limit) {
            reversed.push_back(udon::PlanAction::wait(limit - terminal.steps));
        }
        outcomes.push_back(DayOutcome{
            std::get<0>(key),
            std::get<1>(key),
            std::get<2>(key),
            std::move(reversed),
            std::get<3>(key),
        });
    }

    if (!pruneDominated) {
        return outcomes;
    }
    std::vector<bool> dominated(outcomes.size(), false);
    for (std::size_t left = 0; left < outcomes.size(); ++left) {
        for (std::size_t right = 0; right < outcomes.size(); ++right) {
            if (left == right || outcomes.at(left).position != outcomes.at(right).position) {
                continue;
            }
            const DayOutcome& candidate = outcomes.at(left);
            const DayOutcome& other = outcomes.at(right);
            const bool spotSuperset =
                (other.spotMask | candidate.spotMask) == other.spotMask;
            const bool footprintNoWorse = std::equal(
                other.roadFootprint.begin(),
                other.roadFootprint.end(),
                candidate.roadFootprint.begin(),
                candidate.roadFootprint.end(),
                [](std::int32_t otherStays, std::int32_t candidateStays) {
                    return otherStays <= candidateStays;
                });
            if (spotSuperset && other.fuel >= candidate.fuel &&
                footprintNoWorse &&
                (other.spotMask != candidate.spotMask || other.fuel > candidate.fuel ||
                 other.roadFootprint != candidate.roadFootprint)) {
                dominated.at(left) = true;
                break;
            }
        }
    }
    std::vector<DayOutcome> frontier;
    for (std::size_t index = 0; index < outcomes.size(); ++index) {
        if (!dominated.at(index)) {
            frontier.push_back(std::move(outcomes.at(index)));
        }
    }
    return frontier;
}

[[nodiscard]] std::tuple<udon::CellId, std::int32_t, udon::CellId, std::int32_t, bool>
canonical_pair(const DayOutcome& first, const DayOutcome& second) {
    if (std::tie(second.position, second.fuel) < std::tie(first.position, first.fuel)) {
        return {second.position, second.fuel, first.position, first.fuel, true};
    }
    return {first.position, first.fuel, second.position, second.fuel, false};
}

struct CanonicalTriple {
    std::array<std::pair<udon::CellId, std::int32_t>, 3> states;
    std::array<std::size_t, 3> nextToCurrent{0U, 1U, 2U};
};

[[nodiscard]] CanonicalTriple canonical_triple(
    const std::array<const DayOutcome*, 3>& outcomes) {
    std::array<std::tuple<udon::CellId, std::int32_t, std::size_t>, 3> tagged;
    for (std::size_t index = 0; index < tagged.size(); ++index) {
        tagged.at(index) = {
            outcomes.at(index)->position,
            outcomes.at(index)->fuel,
            index,
        };
    }
    std::sort(tagged.begin(), tagged.end());
    CanonicalTriple canonical;
    for (std::size_t index = 0; index < tagged.size(); ++index) {
        canonical.states.at(index) = {
            std::get<0>(tagged.at(index)),
            std::get<1>(tagged.at(index)),
        };
        canonical.nextToCurrent.at(index) = std::get<2>(tagged.at(index));
    }
    return canonical;
}

[[nodiscard]] std::pair<udon::BrandMask, std::int32_t> joint_day_score(
    const udon::MatchConfig& config,
    std::uint32_t firstMask,
    std::uint32_t secondMask) {
    udon::BrandMask brands;
    std::int32_t servings = 0;
    for (std::size_t spotIndex = 0; spotIndex < config.spots.size(); ++spotIndex) {
        const std::uint32_t bit =
            std::uint32_t{1} << static_cast<std::uint32_t>(spotIndex);
        const std::int32_t claims =
            ((firstMask & bit) != 0U ? 1 : 0) +
            ((secondMask & bit) != 0U ? 1 : 0);
        if (claims == 0) {
            continue;
        }
        const udon::Spot& spot = config.spots.at(spotIndex);
        servings += std::min(claims, spot.stock);
        brands |= udon::brand_bit(spot.brandIndex);
    }
    return {brands, servings};
}

[[nodiscard]] std::pair<udon::BrandMask, std::int32_t> joint_day_score(
    const udon::MatchConfig& config,
    const std::array<std::uint32_t, 3>& masks) {
    udon::BrandMask brands;
    std::int32_t servings = 0;
    for (std::size_t spotIndex = 0; spotIndex < config.spots.size(); ++spotIndex) {
        const std::uint32_t bit =
            std::uint32_t{1} << static_cast<std::uint32_t>(spotIndex);
        const std::int32_t claims = static_cast<std::int32_t>(std::count_if(
            masks.begin(),
            masks.end(),
            [bit](std::uint32_t mask) { return (mask & bit) != 0U; }));
        if (claims == 0) {
            continue;
        }
        const udon::Spot& spot = config.spots.at(spotIndex);
        servings += std::min(claims, spot.stock);
        brands |= udon::brand_bit(spot.brandIndex);
    }
    return {brands, servings};
}

[[nodiscard]] TrafficStatusKey adversarial_next_statuses(
    const udon::MatchConfig& config,
    const TrafficFootprint& currentOwn,
    const TrafficFootprint& previousOwn,
    const TrafficFootprint& currentOpponent,
    const TrafficFootprint& previousOpponent) {
    TrafficStatusKey statuses{};
    for (std::size_t roadIndex = 0;
         roadIndex < config.roadCells.size();
         ++roadIndex) {
        const std::int32_t stays =
            static_cast<std::int32_t>(currentOwn.at(roadIndex)) +
            static_cast<std::int32_t>(previousOwn.at(roadIndex)) +
            static_cast<std::int32_t>(currentOpponent.at(roadIndex)) +
            static_cast<std::int32_t>(previousOpponent.at(roadIndex));
        statuses.at(roadIndex) = static_cast<std::uint8_t>(
            stays >= config.players * config.jammedThreshold
            ? udon::RoadStatus::Jammed
            : (stays >= config.players * config.busyThreshold
                   ? udon::RoadStatus::Busy
                   : udon::RoadStatus::Smooth));
    }
    return statuses;
}

[[nodiscard]] TrafficFootprint combined_traffic_history(
    const udon::MatchConfig& config,
    const TrafficFootprint& own,
    const TrafficFootprint& opponent) {
    TrafficFootprint combined{};
    for (std::size_t roadIndex = 0;
         roadIndex < config.roadCells.size();
         ++roadIndex) {
        add_traffic_stays(
            config,
            combined,
            roadIndex,
            static_cast<std::int32_t>(own.at(roadIndex)) +
                static_cast<std::int32_t>(opponent.at(roadIndex)));
    }
    return combined;
}

[[nodiscard]] std::vector<DayOutcome> opponent_day_outcomes(
    const udon::MatchConfig& config,
    std::int32_t day,
    udon::CellId start,
    std::int32_t fuel,
    const std::vector<udon::RoadStatus>& roadStatuses) {
    const std::vector<DayOutcome> raw = enumerate_day(
        config,
        day,
        start,
        fuel,
        roadStatuses,
        false);
    using OpponentKey = std::tuple<udon::CellId, TrafficFootprint>;
    std::map<OpponentKey, DayOutcome> unique;
    for (const DayOutcome& outcome : raw) {
        const OpponentKey key{outcome.position, outcome.roadFootprint};
        const auto found = unique.find(key);
        if (found == unique.end() || found->second.fuel < outcome.fuel) {
            unique[key] = outcome;
        }
    }
    std::vector<DayOutcome> outcomes;
    outcomes.reserve(unique.size());
    for (auto& [key, outcome] : unique) {
        static_cast<void>(key);
        outcomes.push_back(std::move(outcome));
    }
    return outcomes;
}

[[nodiscard]] std::vector<DayOutcome> minimax_own_day_outcomes(
    const udon::MatchConfig& config,
    std::int32_t day,
    udon::CellId start,
    std::int32_t fuel,
    const std::vector<udon::RoadStatus>& roadStatuses) {
    const std::vector<DayOutcome> raw = enumerate_day(
        config,
        day,
        start,
        fuel,
        roadStatuses,
        false);
    using OwnKey = std::tuple<
        udon::CellId,
        udon::BrandMask,
        std::int32_t,
        TrafficFootprint>;
    std::map<OwnKey, DayOutcome> unique;
    for (const DayOutcome& outcome : raw) {
        const auto [brands, servings] = joint_day_score(
            config,
            outcome.spotMask,
            0U);
        const OwnKey key{
            outcome.position,
            brands,
            servings,
            outcome.roadFootprint,
        };
        const auto found = unique.find(key);
        if (found == unique.end() || found->second.fuel < outcome.fuel) {
            unique[key] = outcome;
        }
    }
    std::vector<DayOutcome> outcomes;
    outcomes.reserve(unique.size());
    for (auto& [key, outcome] : unique) {
        static_cast<void>(key);
        outcomes.push_back(std::move(outcome));
    }
    return outcomes;
}

[[nodiscard]] std::vector<DayOutcome> prune_minimax_own_boundary_dominance(
    const udon::MatchConfig& config,
    udon::BrandMask lifetime,
    const std::vector<DayOutcome>& outcomes,
    MinimaxDiagnostics& diagnostics) {
    diagnostics.maximumOwnOutcomesBeforeDominance = std::max(
        diagnostics.maximumOwnOutcomesBeforeDominance,
        outcomes.size());
    std::vector<bool> dominated(outcomes.size(), false);
    for (std::size_t candidateIndex = 0;
         candidateIndex < outcomes.size();
         ++candidateIndex) {
        const DayOutcome& candidate = outcomes.at(candidateIndex);
        const auto [candidateDailyBrands, candidateServings] = joint_day_score(
            config,
            candidate.spotMask,
            0U);
        const udon::BrandMask candidateLifetime = lifetime | candidateDailyBrands;
        const std::int32_t candidateDailyDistinct =
            udon::brand_count(candidateDailyBrands);
        for (std::size_t challengerIndex = 0;
             challengerIndex < outcomes.size();
             ++challengerIndex) {
            if (candidateIndex == challengerIndex) {
                continue;
            }
            const DayOutcome& challenger = outcomes.at(challengerIndex);
            if (challenger.position != candidate.position ||
                challenger.roadFootprint != candidate.roadFootprint ||
                challenger.fuel < candidate.fuel) {
                continue;
            }
            const auto [challengerDailyBrands, challengerServings] = joint_day_score(
                config,
                challenger.spotMask,
                0U);
            const udon::BrandMask challengerLifetime = lifetime | challengerDailyBrands;
            const std::int32_t challengerDailyDistinct =
                udon::brand_count(challengerDailyBrands);
            const bool lifetimeSuperset =
                candidateLifetime.is_subset_of(challengerLifetime);
            const bool scoreNoWorse =
                challengerDailyDistinct >= candidateDailyDistinct &&
                challengerServings >= candidateServings;
            const bool strict = challenger.fuel > candidate.fuel ||
                challengerLifetime != candidateLifetime ||
                challengerDailyDistinct > candidateDailyDistinct ||
                challengerServings > candidateServings;
            if (lifetimeSuperset && scoreNoWorse && strict) {
                dominated.at(candidateIndex) = true;
                break;
            }
        }
    }
    std::vector<DayOutcome> frontier;
    frontier.reserve(outcomes.size());
    for (std::size_t index = 0; index < outcomes.size(); ++index) {
        if (dominated.at(index)) {
            ++diagnostics.boundaryDominancePruned;
            continue;
        }
        frontier.push_back(outcomes.at(index));
    }
    return frontier;
}

[[nodiscard]] MinimaxNode solve_public_minimax(
    const Fixture& fixture,
    const AdversarialKey& key,
    MinimaxSearch& search) {
    const auto cached = search.memo.find(key);
    if (cached != search.memo.end()) {
        return cached->second;
    }
    const auto& [
        day,
        ownPosition,
        ownFuel,
        opponentPosition,
        opponentFuel,
        lifetime,
        previousOwn,
        previousOpponent,
        roadStatusKey] = key;
    if (day > fixture.config.day_count()) {
        MinimaxNode terminal;
        terminal.score.lifetimeDistinct = udon::brand_count(lifetime);
        search.memo.emplace(key, terminal);
        ++search.diagnostics.states;
        return terminal;
    }
    const std::vector<udon::RoadStatus> roadStatuses = expand_road_statuses(
        fixture.config,
        roadStatusKey);
    const AdversarialDayCacheKey ownCacheKey{
        day,
        ownPosition,
        ownFuel,
        roadStatusKey,
    };
    auto ownFound = search.ownCache.find(ownCacheKey);
    if (ownFound == search.ownCache.end()) {
        ownFound = search.ownCache.emplace(
            ownCacheKey,
            minimax_own_day_outcomes(
                fixture.config,
                day,
                ownPosition,
                ownFuel,
                roadStatuses)).first;
    }
    const AdversarialDayCacheKey opponentCacheKey{
        day,
        opponentPosition,
        opponentFuel,
        roadStatusKey,
    };
    auto opponentFound = search.opponentCache.find(opponentCacheKey);
    if (opponentFound == search.opponentCache.end()) {
        opponentFound = search.opponentCache.emplace(
            opponentCacheKey,
            opponent_day_outcomes(
                fixture.config,
                day,
                opponentPosition,
                opponentFuel,
                roadStatuses)).first;
    }
    std::vector<DayOutcome> dominatedOwnOutcomes;
    if (search.boundaryDominance) {
        dominatedOwnOutcomes = prune_minimax_own_boundary_dominance(
            fixture.config,
            lifetime,
            ownFound->second,
            search.diagnostics);
    } else {
        search.diagnostics.maximumOwnOutcomesBeforeDominance = std::max(
            search.diagnostics.maximumOwnOutcomesBeforeDominance,
            ownFound->second.size());
    }
    const std::vector<DayOutcome>& ownOutcomes = search.boundaryDominance
        ? dominatedOwnOutcomes
        : ownFound->second;
    const std::vector<DayOutcome>& opponentOutcomes = opponentFound->second;
    search.diagnostics.maximumOwnOutcomes = std::max(
        search.diagnostics.maximumOwnOutcomes,
        ownOutcomes.size());
    search.diagnostics.maximumOpponentOutcomes = std::max(
        search.diagnostics.maximumOpponentOutcomes,
        opponentOutcomes.size());
    if (ownOutcomes.empty() || opponentOutcomes.empty()) {
        throw std::runtime_error("public minimax produced an empty legal action set");
    }
    MinimaxNode best;
    bool haveBest = false;
    for (const DayOutcome& own : ownOutcomes) {
        const auto [dailyBrands, dailyServings] = joint_day_score(
            fixture.config,
            own.spotMask,
            0U);
        MinimaxNode worst;
        bool haveWorst = false;
        for (const DayOutcome& opponent : opponentOutcomes) {
            const TrafficFootprint nextPreviousOwn =
                search.trafficHistoryQuotient
                ? combined_traffic_history(
                      fixture.config,
                      own.roadFootprint,
                      opponent.roadFootprint)
                : own.roadFootprint;
            const TrafficFootprint nextPreviousOpponent =
                search.trafficHistoryQuotient
                ? TrafficFootprint{}
                : opponent.roadFootprint;
            const AdversarialKey next{
                day + 1,
                own.position,
                own.fuel,
                opponent.position,
                opponent.fuel,
                lifetime | dailyBrands,
                nextPreviousOwn,
                nextPreviousOpponent,
                adversarial_next_statuses(
                    fixture.config,
                    own.roadFootprint,
                    previousOwn,
                    opponent.roadFootprint,
                    previousOpponent),
            };
            MinimaxNode continuation = solve_public_minimax(
                fixture,
                next,
                search);
            continuation.score.totalDailyDistinct += udon::brand_count(dailyBrands);
            continuation.score.totalServings += dailyServings;
            ++search.diagnostics.transitions;
            if (!haveWorst || continuation.score < worst.score) {
                worst = std::move(continuation);
                worst.worstOpponent = opponent;
                haveWorst = true;
            }
        }
        if (!haveBest || best.score < worst.score) {
            best = std::move(worst);
            best.bestOwn = own;
            haveBest = true;
        }
    }
    search.memo.emplace(key, best);
    ++search.diagnostics.states;
    return best;
}

struct RootStreamResult {
    MinimaxNode root;
    bool hasRoot = false;
    bool fullSlice = false;
    std::size_t ownCount = 0;
    std::size_t opponentCount = 0;
    std::size_t begin = 0;
    std::size_t end = 0;
};

[[nodiscard]] std::string agent_plan_text(const udon::AgentPlan& plan) {
    std::ostringstream output;
    for (std::size_t action = 0; action < plan.size(); ++action) {
        if (action != 0U) {
            output << '.';
        }
        output << plan.at(action).wire_value();
    }
    return output.str();
}

[[nodiscard]] RootStreamResult solve_public_minimax_root_stream(
    const Fixture& fixture,
    const AdversarialKey& key,
    MinimaxSearch& search,
    std::int32_t requestedBegin,
    std::int32_t requestedEnd) {
    const auto& [
        day,
        ownPosition,
        ownFuel,
        opponentPosition,
        opponentFuel,
        lifetime,
        previousOwn,
        previousOpponent,
        roadStatusKey] = key;
    if (day > fixture.config.day_count()) {
        throw std::invalid_argument(
            "root streaming requires a nonterminal public state");
    }
    const std::vector<udon::RoadStatus> roadStatuses = expand_road_statuses(
        fixture.config,
        roadStatusKey);
    const AdversarialDayCacheKey ownCacheKey{
        day,
        ownPosition,
        ownFuel,
        roadStatusKey,
    };
    auto ownFound = search.ownCache.find(ownCacheKey);
    if (ownFound == search.ownCache.end()) {
        ownFound = search.ownCache.emplace(
            ownCacheKey,
            minimax_own_day_outcomes(
                fixture.config,
                day,
                ownPosition,
                ownFuel,
                roadStatuses)).first;
    }
    const AdversarialDayCacheKey opponentCacheKey{
        day,
        opponentPosition,
        opponentFuel,
        roadStatusKey,
    };
    auto opponentFound = search.opponentCache.find(opponentCacheKey);
    if (opponentFound == search.opponentCache.end()) {
        opponentFound = search.opponentCache.emplace(
            opponentCacheKey,
            opponent_day_outcomes(
                fixture.config,
                day,
                opponentPosition,
                opponentFuel,
                roadStatuses)).first;
    }
    std::vector<DayOutcome> dominatedOwnOutcomes;
    if (search.boundaryDominance) {
        dominatedOwnOutcomes = prune_minimax_own_boundary_dominance(
            fixture.config,
            lifetime,
            ownFound->second,
            search.diagnostics);
    }
    const std::vector<DayOutcome>& ownOutcomes = search.boundaryDominance
        ? dominatedOwnOutcomes
        : ownFound->second;
    const std::vector<DayOutcome>& opponentOutcomes = opponentFound->second;
    if (ownOutcomes.empty() || opponentOutcomes.empty()) {
        throw std::runtime_error(
            "root streaming produced an empty legal action set");
    }
    search.diagnostics.maximumOwnOutcomes = std::max(
        search.diagnostics.maximumOwnOutcomes,
        ownOutcomes.size());
    search.diagnostics.maximumOpponentOutcomes = std::max(
        search.diagnostics.maximumOpponentOutcomes,
        opponentOutcomes.size());

    RootStreamResult result;
    result.ownCount = ownOutcomes.size();
    result.opponentCount = opponentOutcomes.size();
    result.begin = std::min(
        ownOutcomes.size(),
        static_cast<std::size_t>(requestedBegin));
    result.end = requestedEnd < 0
        ? ownOutcomes.size()
        : std::min(
              ownOutcomes.size(),
              static_cast<std::size_t>(requestedEnd));
    result.fullSlice = result.begin == 0U && result.end == ownOutcomes.size();
    std::cout << "root_stream_begin,day=" << day
              << ",own_count=" << result.ownCount
              << ",opponent_count=" << result.opponentCount
              << ",begin=" << result.begin
              << ",end=" << result.end
              << std::endl;

    for (std::size_t ownIndex = result.begin;
         ownIndex < result.end;
         ++ownIndex) {
        const DayOutcome& own = ownOutcomes.at(ownIndex);
        const auto [dailyBrands, dailyServings] = joint_day_score(
            fixture.config,
            own.spotMask,
            0U);
        MinimaxNode worst;
        bool haveWorst = false;
        for (const DayOutcome& opponent : opponentOutcomes) {
            const TrafficFootprint nextPreviousOwn =
                search.trafficHistoryQuotient
                ? combined_traffic_history(
                      fixture.config,
                      own.roadFootprint,
                      opponent.roadFootprint)
                : own.roadFootprint;
            const TrafficFootprint nextPreviousOpponent =
                search.trafficHistoryQuotient
                ? TrafficFootprint{}
                : opponent.roadFootprint;
            const AdversarialKey next{
                day + 1,
                own.position,
                own.fuel,
                opponent.position,
                opponent.fuel,
                lifetime | dailyBrands,
                nextPreviousOwn,
                nextPreviousOpponent,
                adversarial_next_statuses(
                    fixture.config,
                    own.roadFootprint,
                    previousOwn,
                    opponent.roadFootprint,
                    previousOpponent),
            };
            MinimaxNode continuation = solve_public_minimax(
                fixture,
                next,
                search);
            continuation.score.totalDailyDistinct += udon::brand_count(dailyBrands);
            continuation.score.totalServings += dailyServings;
            ++search.diagnostics.transitions;
            if (!haveWorst || continuation.score < worst.score) {
                worst = std::move(continuation);
                worst.worstOpponent = opponent;
                haveWorst = true;
            }
        }
        const bool improvesSlice =
            !result.hasRoot || result.root.score < worst.score;
        if (improvesSlice) {
            result.root = worst;
            result.root.bestOwn = own;
            result.hasRoot = true;
        }
        std::cout << "root_stream_action,day=" << day
                  << ",index=" << ownIndex
                  << ",robust=" << score_text(worst.score)
                  << ",slice_best=" << score_text(result.root.score)
                  << ",improves=" << (improvesSlice ? 1 : 0)
                  << ",terminal=" << own.position << '@' << own.fuel
                  << ",plan=" << agent_plan_text(own.actions)
                  << ",states=" << search.diagnostics.states
                  << ",transitions=" << search.diagnostics.transitions
                  << std::endl;
    }
    if (result.fullSlice && result.hasRoot) {
        search.memo.emplace(key, result.root);
        ++search.diagnostics.states;
    }
    std::cout << "root_stream_end,day=" << day
              << ",begin=" << result.begin
              << ",end=" << result.end
              << ",full=" << (result.fullSlice ? 1 : 0)
              << ",has_result=" << (result.hasRoot ? 1 : 0)
              << ",slice_best="
              << (result.hasRoot ? score_text(result.root.score) : "none")
              << ",states=" << search.diagnostics.states
              << ",transitions=" << search.diagnostics.transitions
              << std::endl;
    return result;
}

[[nodiscard]] bool accumulated_better(
    std::int32_t dailyDistinct,
    std::int32_t servings,
    const LayerEntry& incumbent) {
    return dailyDistinct > incumbent.totalDailyDistinct ||
        (dailyDistinct == incumbent.totalDailyDistinct &&
         servings > incumbent.totalServings);
}

[[nodiscard]] std::vector<LayerEntry> prune_layer(std::vector<LayerEntry> entries) {
    std::vector<bool> dominated(entries.size(), false);
    using PhysicalKey = std::tuple<
        udon::CellId,
        udon::CellId>;
    std::map<PhysicalKey, std::vector<std::size_t>> groups;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto& [p0, f0, p1, f1, lifetime, previousOwn, roadStatuses] =
            entries.at(index).key;
        static_cast<void>(f0);
        static_cast<void>(f1);
        static_cast<void>(lifetime);
        static_cast<void>(previousOwn);
        static_cast<void>(roadStatuses);
        groups[{p0, p1}].push_back(index);
    }
    for (const auto& [physical, indices] : groups) {
        static_cast<void>(physical);
        for (const std::size_t left : indices) {
            const udon::BrandMask leftLifetime = std::get<4>(entries.at(left).key);
            for (const std::size_t right : indices) {
            if (left == right) {
                continue;
            }
            const udon::BrandMask rightLifetime = std::get<4>(entries.at(right).key);
            const bool fuelNoWorse =
                std::get<1>(entries.at(right).key) >=
                    std::get<1>(entries.at(left).key) &&
                std::get<3>(entries.at(right).key) >=
                    std::get<3>(entries.at(left).key);
            const TrafficFootprint& leftPrevious =
                std::get<5>(entries.at(left).key);
            const TrafficFootprint& rightPrevious =
                std::get<5>(entries.at(right).key);
            const TrafficStatusKey& leftStatuses =
                std::get<6>(entries.at(left).key);
            const TrafficStatusKey& rightStatuses =
                std::get<6>(entries.at(right).key);
            const bool lifetimeSuperset = leftLifetime.is_subset_of(rightLifetime);
            const bool trafficNoWorse =
                std::equal(
                    rightPrevious.begin(),
                    rightPrevious.end(),
                    leftPrevious.begin(),
                    leftPrevious.end(),
                    std::less_equal<std::uint8_t>{}) &&
                std::equal(
                    rightStatuses.begin(),
                    rightStatuses.end(),
                    leftStatuses.begin(),
                    leftStatuses.end(),
                    std::less_equal<std::uint8_t>{});
            const bool scoreNoWorse =
                entries.at(right).totalDailyDistinct >= entries.at(left).totalDailyDistinct &&
                entries.at(right).totalServings >= entries.at(left).totalServings;
            const bool strict = rightLifetime != leftLifetime ||
                rightPrevious != leftPrevious ||
                rightStatuses != leftStatuses ||
                entries.at(right).totalDailyDistinct > entries.at(left).totalDailyDistinct ||
                entries.at(right).totalServings > entries.at(left).totalServings;
            const bool resourceStrict =
                std::get<1>(entries.at(right).key) >
                    std::get<1>(entries.at(left).key) ||
                std::get<3>(entries.at(right).key) >
                    std::get<3>(entries.at(left).key);
            if (fuelNoWorse && lifetimeSuperset && trafficNoWorse &&
                scoreNoWorse && (strict || resourceStrict)) {
                dominated.at(left) = true;
                break;
            }
        }
        }
    }
    std::vector<LayerEntry> frontier;
    frontier.reserve(entries.size());
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (!dominated.at(index)) {
            frontier.push_back(std::move(entries.at(index)));
        }
    }
    return frontier;
}

[[nodiscard]] OracleResult solve_oracle(const Fixture& fixture) {
    const auto initialFirst = std::pair{
        fixture.config.initialAgents.at(0),
        fixture.config.fuelLimit,
    };
    const auto initialSecond = std::pair{
        fixture.config.initialAgents.at(1),
        fixture.config.fuelLimit,
    };
    const bool initialSwap = initialSecond < initialFirst;
    const auto first = initialSwap ? initialSecond : initialFirst;
    const auto second = initialSwap ? initialFirst : initialSecond;

    std::vector<std::vector<LayerEntry>> layers;
    const TrafficFootprint emptyFootprint{};
    const TrafficStatusKey initialRoadStatuses = compact_road_statuses(
        fixture.config,
        exact_road_statuses(
            fixture,
            1,
            emptyFootprint,
            emptyFootprint));
    layers.push_back(std::vector<LayerEntry>{LayerEntry{
        MatchKey{
            first.first,
            first.second,
            second.first,
            second.second,
            0U,
            emptyFootprint,
            initialRoadStatuses},
    }});
    using DayCacheKey = std::tuple<
        std::int32_t,
        udon::CellId,
        std::int32_t,
        std::vector<udon::RoadStatus>>;
    std::map<DayCacheKey, std::vector<DayOutcome>> cache;
    OracleResult result;
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const std::vector<LayerEntry>& current = layers.back();
        std::vector<LayerEntry> next;
        std::map<MatchKey, std::size_t> retained;
        for (std::size_t parentIndex = 0; parentIndex < current.size(); ++parentIndex) {
            const LayerEntry& parent = current.at(parentIndex);
            const auto& [
                firstPosition,
                firstFuel,
                secondPosition,
                secondFuel,
                lifetime,
                previousOwn,
                roadStatusKey] = parent.key;
            const std::vector<udon::RoadStatus> roadStatuses =
                expand_road_statuses(
                    fixture.config,
                    roadStatusKey);
            const DayCacheKey firstCacheKey{
                day,
                firstPosition,
                firstFuel,
                roadStatuses};
            auto firstFound = cache.find(firstCacheKey);
            if (firstFound == cache.end()) {
                firstFound = cache.emplace(
                    firstCacheKey,
                    enumerate_day(
                        fixture.config,
                        day,
                        firstPosition,
                        firstFuel,
                        roadStatuses)).first;
                ++result.dailyEnumerations;
            }
            const DayCacheKey secondCacheKey{
                day,
                secondPosition,
                secondFuel,
                roadStatuses};
            auto secondFound = cache.find(secondCacheKey);
            if (secondFound == cache.end()) {
                secondFound = cache.emplace(
                    secondCacheKey,
                    enumerate_day(
                        fixture.config,
                        day,
                        secondPosition,
                        secondFuel,
                        roadStatuses)).first;
                ++result.dailyEnumerations;
            }
            for (const DayOutcome& firstOutcome : firstFound->second) {
                for (const DayOutcome& secondOutcome : secondFound->second) {
                    const auto [dailyBrands, dailyServings] = joint_day_score(
                        fixture.config,
                        firstOutcome.spotMask,
                        secondOutcome.spotMask);
                    const auto [nextP0, nextF0, nextP1, nextF1, swap] =
                        canonical_pair(firstOutcome, secondOutcome);
                    TrafficFootprint currentOwn{};
                    for (std::size_t roadIndex = 0;
                         roadIndex < fixture.config.roadCells.size();
                         ++roadIndex) {
                        add_traffic_stays(
                            fixture.config,
                            currentOwn,
                            roadIndex,
                            static_cast<std::int32_t>(
                                firstOutcome.roadFootprint.at(roadIndex)) +
                                static_cast<std::int32_t>(
                                    secondOutcome.roadFootprint.at(roadIndex)));
                    }
                    const MatchKey key{
                        nextP0,
                        nextF0,
                        nextP1,
                        nextF1,
                        lifetime | dailyBrands,
                        currentOwn,
                        compact_road_statuses(
                            fixture.config,
                            exact_road_statuses(
                                fixture,
                                day + 1,
                                currentOwn,
                                previousOwn)),
                    };
                    const std::int32_t nextDaily = parent.totalDailyDistinct +
                        udon::brand_count(dailyBrands);
                    const std::int32_t nextServings =
                        parent.totalServings + dailyServings;
                    const auto found = retained.find(key);
                    if (found != retained.end() &&
                        !accumulated_better(nextDaily, nextServings, next.at(found->second))) {
                        continue;
                    }
                    LayerEntry candidate;
                    candidate.key = key;
                    candidate.totalDailyDistinct = nextDaily;
                    candidate.totalServings = nextServings;
                    candidate.parentIndex = static_cast<std::int32_t>(parentIndex);
                    candidate.firstPlan = firstOutcome.actions;
                    candidate.secondPlan = secondOutcome.actions;
                    candidate.swapForNextDay = swap;
                    if (found == retained.end()) {
                        retained.emplace(key, next.size());
                        next.push_back(std::move(candidate));
                    } else {
                        next.at(found->second) = std::move(candidate);
                    }
                }
            }
        }
        next = prune_layer(std::move(next));
        if (next.empty()) {
            return result;
        }
        result.maximumFrontier = std::max(result.maximumFrontier, next.size());
        layers.push_back(std::move(next));
    }

    const std::vector<LayerEntry>& finalLayer = layers.back();
    std::size_t bestIndex = 0;
    udon::OfficialScore bestScore{
        udon::brand_count(std::get<4>(finalLayer.front().key)),
        finalLayer.front().totalDailyDistinct,
        finalLayer.front().totalServings,
    };
    for (std::size_t index = 1; index < finalLayer.size(); ++index) {
        const LayerEntry& candidate = finalLayer.at(index);
        const udon::OfficialScore score{
            udon::brand_count(std::get<4>(candidate.key)),
            candidate.totalDailyDistinct,
            candidate.totalServings,
        };
        if (bestScore < score) {
            bestScore = score;
            bestIndex = index;
        }
    }

    struct AbstractDay {
        udon::AgentPlan first;
        udon::AgentPlan second;
        bool swap = false;
        MatchKey terminal{};
    };
    std::vector<AbstractDay> abstractDays(
        static_cast<std::size_t>(fixture.config.day_count()));
    std::size_t currentIndex = bestIndex;
    for (std::int32_t day = fixture.config.day_count(); day >= 1; --day) {
        const LayerEntry& entry = layers.at(static_cast<std::size_t>(day)).at(currentIndex);
        abstractDays.at(static_cast<std::size_t>(day - 1)) = AbstractDay{
            entry.firstPlan,
            entry.secondPlan,
            entry.swapForNextDay,
            entry.key,
        };
        currentIndex = static_cast<std::size_t>(entry.parentIndex);
    }

    std::vector<udon::AgentState> agents;
    for (const udon::CellId start : fixture.config.initialAgents) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            start,
            fixture.config.fuelLimit,
        });
    }
    std::array<std::size_t, 2> abstractToPhysical = initialSwap
        ? std::array<std::size_t, 2>{1U, 0U}
        : std::array<std::size_t, 2>{0U, 1U};
    udon::MatchLedger ledger;
    TrafficFootprint previousOwn = emptyFootprint;
    TrafficStatusKey roadStatusKey = initialRoadStatuses;
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const AbstractDay& abstract = abstractDays.at(static_cast<std::size_t>(day - 1));
        udon::DayPlan plan;
        plan.actions.resize(static_cast<std::size_t>(fixture.config.agent_count()));
        plan.actions.at(abstractToPhysical.at(0)) = abstract.first;
        plan.actions.at(abstractToPhysical.at(1)) = abstract.second;
        plan.actions.at(2) = udon::AgentPlan{
            udon::PlanAction::wait(fixture.config.steps_for_day(day)),
        };
        const std::vector<udon::RoadStatus> roadStatuses =
            expand_road_statuses(
                fixture.config,
                roadStatusKey);
        const udon::DayState state = day_state(
            fixture.config,
            day,
            agents,
            roadStatuses);
        udon::SimulationResult simulation;
        std::string mismatch;
        if (!validates(fixture.config, state, plan, simulation, mismatch)) {
            throw std::runtime_error("oracle witness validation failed: " + mismatch);
        }
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
        if (abstract.swap) {
            std::swap(abstractToPhysical.at(0), abstractToPhysical.at(1));
        }
        const auto& [
            expectedP0,
            expectedF0,
            expectedP1,
            expectedF1,
            expectedLifetime,
            expectedPreviousOwn,
            expectedNextRoadStatuses] = abstract.terminal;
        const TrafficFootprint actualCurrentOwn = compact_road_footprint(
            fixture.config,
            simulation.roadFootprint);
        const TrafficStatusKey actualNextRoadStatuses = compact_road_statuses(
            fixture.config,
            exact_road_statuses(
                fixture,
                day + 1,
                actualCurrentOwn,
                previousOwn));
        if (agents.at(abstractToPhysical.at(0)).position != expectedP0 ||
            agents.at(abstractToPhysical.at(0)).fuel != expectedF0 ||
            agents.at(abstractToPhysical.at(1)).position != expectedP1 ||
            agents.at(abstractToPhysical.at(1)).fuel != expectedF1 ||
            ledger.lifetimeBrands != expectedLifetime ||
            actualCurrentOwn != expectedPreviousOwn ||
            actualNextRoadStatuses != expectedNextRoadStatuses) {
            throw std::runtime_error("oracle witness disagrees with canonical DP state");
        }
        previousOwn = actualCurrentOwn;
        roadStatusKey = actualNextRoadStatuses;
        result.plans.push_back(std::move(plan));
        result.cumulative.push_back(udon::OfficialScore{
            ledger.lifetime_distinct(),
            ledger.totalDailyDistinct,
            ledger.totalServings,
        });
        result.terminalAgents.push_back(agents);
    }
    result.score = udon::OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
    if (!(result.score == bestScore)) {
        throw std::runtime_error("oracle witness score disagrees with complete DP");
    }
    result.valid = true;
    return result;
}

[[nodiscard]] bool accumulated_better(
    std::int32_t dailyDistinct,
    std::int32_t servings,
    const ThreeLayerEntry& incumbent) {
    return dailyDistinct > incumbent.totalDailyDistinct ||
        (dailyDistinct == incumbent.totalDailyDistinct &&
         servings > incumbent.totalServings);
}

[[nodiscard]] std::vector<ThreeLayerEntry> prune_three_layer(
    std::vector<ThreeLayerEntry> entries) {
    std::vector<bool> dominated(entries.size(), false);
    using PhysicalKey =
        std::tuple<udon::CellId, udon::CellId, udon::CellId>;
    std::map<PhysicalKey, std::vector<std::size_t>> groups;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        groups[{
            std::get<0>(entries.at(index).key),
            std::get<2>(entries.at(index).key),
            std::get<4>(entries.at(index).key),
        }].push_back(index);
    }
    for (const auto& [physical, indices] : groups) {
        static_cast<void>(physical);
        for (const std::size_t left : indices) {
            const udon::BrandMask leftLifetime =
                std::get<6>(entries.at(left).key);
            for (const std::size_t right : indices) {
                if (left == right) {
                    continue;
                }
                const udon::BrandMask rightLifetime =
                    std::get<6>(entries.at(right).key);
                const bool fuelNoWorse =
                    std::get<1>(entries.at(right).key) >=
                        std::get<1>(entries.at(left).key) &&
                    std::get<3>(entries.at(right).key) >=
                        std::get<3>(entries.at(left).key) &&
                    std::get<5>(entries.at(right).key) >=
                        std::get<5>(entries.at(left).key);
                const bool lifetimeSuperset =
                    leftLifetime.is_subset_of(rightLifetime);
                const bool scoreNoWorse =
                    entries.at(right).totalDailyDistinct >=
                        entries.at(left).totalDailyDistinct &&
                    entries.at(right).totalServings >=
                        entries.at(left).totalServings;
                const bool strict = rightLifetime != leftLifetime ||
                    entries.at(right).totalDailyDistinct >
                        entries.at(left).totalDailyDistinct ||
                    entries.at(right).totalServings >
                        entries.at(left).totalServings ||
                    std::get<1>(entries.at(right).key) >
                        std::get<1>(entries.at(left).key) ||
                    std::get<3>(entries.at(right).key) >
                        std::get<3>(entries.at(left).key) ||
                    std::get<5>(entries.at(right).key) >
                        std::get<5>(entries.at(left).key);
                if (fuelNoWorse && lifetimeSuperset && scoreNoWorse && strict) {
                    dominated.at(left) = true;
                    break;
                }
            }
        }
    }
    std::vector<ThreeLayerEntry> frontier;
    frontier.reserve(entries.size());
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (!dominated.at(index)) {
            frontier.push_back(std::move(entries.at(index)));
        }
    }
    return frontier;
}

[[nodiscard]] OracleResult solve_three_oracle(const Fixture& fixture) {
    if (fixture.config.agent_count() != 3 || !fixture.config.roadCells.empty()) {
        throw std::runtime_error(
            "three-patrol oracle requires exactly three agents and no roads");
    }

    std::array<std::tuple<udon::CellId, std::int32_t, std::size_t>, 3> initial;
    for (std::size_t index = 0; index < initial.size(); ++index) {
        initial.at(index) = {
            fixture.config.initialAgents.at(index),
            fixture.config.fuelLimit,
            index,
        };
    }
    std::sort(initial.begin(), initial.end());

    std::vector<std::vector<ThreeLayerEntry>> layers;
    layers.push_back(std::vector<ThreeLayerEntry>{ThreeLayerEntry{
        ThreeMatchKey{
            std::get<0>(initial.at(0)),
            std::get<1>(initial.at(0)),
            std::get<0>(initial.at(1)),
            std::get<1>(initial.at(1)),
            std::get<0>(initial.at(2)),
            std::get<1>(initial.at(2)),
            udon::BrandMask{},
        },
    }});

    using DayCacheKey =
        std::tuple<std::int32_t, udon::CellId, std::int32_t>;
    std::map<DayCacheKey, std::vector<DayOutcome>> cache;
    const std::vector<udon::RoadStatus> smooth(
        static_cast<std::size_t>(fixture.config.map.cell_count()),
        udon::RoadStatus::Smooth);
    OracleResult result;

    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const std::vector<ThreeLayerEntry>& current = layers.back();
        std::vector<ThreeLayerEntry> next;
        std::map<ThreeMatchKey, std::size_t> retained;
        for (std::size_t parentIndex = 0; parentIndex < current.size();
             ++parentIndex) {
            const ThreeLayerEntry& parent = current.at(parentIndex);
            const std::array<udon::CellId, 3> positions{
                std::get<0>(parent.key),
                std::get<2>(parent.key),
                std::get<4>(parent.key),
            };
            const std::array<std::int32_t, 3> fuels{
                std::get<1>(parent.key),
                std::get<3>(parent.key),
                std::get<5>(parent.key),
            };
            std::array<const std::vector<DayOutcome>*, 3> outcomes{};
            for (std::size_t agent = 0; agent < outcomes.size(); ++agent) {
                const DayCacheKey cacheKey{day, positions.at(agent), fuels.at(agent)};
                auto found = cache.find(cacheKey);
                if (found == cache.end()) {
                    found = cache.emplace(
                        cacheKey,
                        enumerate_day(
                            fixture.config,
                            day,
                            positions.at(agent),
                            fuels.at(agent),
                            smooth)).first;
                    ++result.dailyEnumerations;
                }
                outcomes.at(agent) = &found->second;
            }

            for (const DayOutcome& first : *outcomes.at(0)) {
                for (const DayOutcome& second : *outcomes.at(1)) {
                    for (const DayOutcome& third : *outcomes.at(2)) {
                        const std::array<const DayOutcome*, 3> chosen{
                            &first,
                            &second,
                            &third,
                        };
                        const auto [dailyBrands, dailyServings] =
                            joint_day_score(
                                fixture.config,
                                std::array<std::uint32_t, 3>{
                                    first.spotMask,
                                    second.spotMask,
                                    third.spotMask,
                                });
                        const CanonicalTriple canonical =
                            canonical_triple(chosen);
                        const ThreeMatchKey key{
                            canonical.states.at(0).first,
                            canonical.states.at(0).second,
                            canonical.states.at(1).first,
                            canonical.states.at(1).second,
                            canonical.states.at(2).first,
                            canonical.states.at(2).second,
                            std::get<6>(parent.key) | dailyBrands,
                        };
                        const std::int32_t nextDaily =
                            parent.totalDailyDistinct +
                            udon::brand_count(dailyBrands);
                        const std::int32_t nextServings =
                            parent.totalServings + dailyServings;
                        const auto found = retained.find(key);
                        if (found != retained.end() &&
                            !accumulated_better(
                                nextDaily,
                                nextServings,
                                next.at(found->second))) {
                            continue;
                        }
                        ThreeLayerEntry candidate;
                        candidate.key = key;
                        candidate.totalDailyDistinct = nextDaily;
                        candidate.totalServings = nextServings;
                        candidate.parentIndex =
                            static_cast<std::int32_t>(parentIndex);
                        candidate.plans = {
                            first.actions,
                            second.actions,
                            third.actions,
                        };
                        candidate.nextToCurrent = canonical.nextToCurrent;
                        if (found == retained.end()) {
                            retained.emplace(key, next.size());
                            next.push_back(std::move(candidate));
                        } else {
                            next.at(found->second) = std::move(candidate);
                        }
                    }
                }
            }
        }
        next = prune_three_layer(std::move(next));
        if (next.empty()) {
            return result;
        }
        result.maximumFrontier = std::max(result.maximumFrontier, next.size());
        layers.push_back(std::move(next));
    }

    const std::vector<ThreeLayerEntry>& finalLayer = layers.back();
    std::size_t bestIndex = 0;
    udon::OfficialScore bestScore{
        udon::brand_count(std::get<6>(finalLayer.front().key)),
        finalLayer.front().totalDailyDistinct,
        finalLayer.front().totalServings,
    };
    for (std::size_t index = 1; index < finalLayer.size(); ++index) {
        const ThreeLayerEntry& candidate = finalLayer.at(index);
        const udon::OfficialScore score{
            udon::brand_count(std::get<6>(candidate.key)),
            candidate.totalDailyDistinct,
            candidate.totalServings,
        };
        if (bestScore < score) {
            bestScore = score;
            bestIndex = index;
        }
    }

    struct AbstractDay {
        std::array<udon::AgentPlan, 3> plans;
        std::array<std::size_t, 3> nextToCurrent{0U, 1U, 2U};
        ThreeMatchKey terminal{};
    };
    std::vector<AbstractDay> abstractDays(
        static_cast<std::size_t>(fixture.config.day_count()));
    std::size_t currentIndex = bestIndex;
    for (std::int32_t day = fixture.config.day_count(); day >= 1; --day) {
        const ThreeLayerEntry& entry =
            layers.at(static_cast<std::size_t>(day)).at(currentIndex);
        abstractDays.at(static_cast<std::size_t>(day - 1)) = AbstractDay{
            entry.plans,
            entry.nextToCurrent,
            entry.key,
        };
        currentIndex = static_cast<std::size_t>(entry.parentIndex);
    }

    std::vector<udon::AgentState> agents;
    for (const udon::CellId start : fixture.config.initialAgents) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            start,
            fixture.config.fuelLimit,
        });
    }
    std::array<std::size_t, 3> abstractToPhysical{
        std::get<2>(initial.at(0)),
        std::get<2>(initial.at(1)),
        std::get<2>(initial.at(2)),
    };
    udon::MatchLedger ledger;
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const AbstractDay& abstract =
            abstractDays.at(static_cast<std::size_t>(day - 1));
        udon::DayPlan plan;
        plan.actions.resize(3U);
        for (std::size_t slot = 0; slot < abstract.plans.size(); ++slot) {
            plan.actions.at(abstractToPhysical.at(slot)) =
                abstract.plans.at(slot);
        }
        const udon::DayState state = day_state(fixture.config, day, agents);
        udon::SimulationResult simulation;
        std::string mismatch;
        if (!validates(fixture.config, state, plan, simulation, mismatch)) {
            throw std::runtime_error(
                "three-patrol oracle witness validation failed: " + mismatch);
        }
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
        std::array<std::size_t, 3> nextMapping{};
        for (std::size_t slot = 0; slot < nextMapping.size(); ++slot) {
            nextMapping.at(slot) =
                abstractToPhysical.at(abstract.nextToCurrent.at(slot));
        }
        abstractToPhysical = nextMapping;

        const std::array<udon::CellId, 3> expectedPositions{
            std::get<0>(abstract.terminal),
            std::get<2>(abstract.terminal),
            std::get<4>(abstract.terminal),
        };
        const std::array<std::int32_t, 3> expectedFuels{
            std::get<1>(abstract.terminal),
            std::get<3>(abstract.terminal),
            std::get<5>(abstract.terminal),
        };
        for (std::size_t slot = 0; slot < expectedPositions.size(); ++slot) {
            const udon::AgentState& actual =
                agents.at(abstractToPhysical.at(slot));
            if (actual.position != expectedPositions.at(slot) ||
                actual.fuel != expectedFuels.at(slot)) {
                throw std::runtime_error(
                    "three-patrol witness disagrees with canonical terminal state");
            }
        }
        if (ledger.lifetimeBrands != std::get<6>(abstract.terminal)) {
            throw std::runtime_error(
                "three-patrol witness disagrees with canonical lifetime mask");
        }
        result.plans.push_back(std::move(plan));
        result.cumulative.push_back(udon::OfficialScore{
            ledger.lifetime_distinct(),
            ledger.totalDailyDistinct,
            ledger.totalServings,
        });
        result.terminalAgents.push_back(agents);
    }
    result.score = udon::OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
    if (!(result.score == bestScore)) {
        throw std::runtime_error(
            "three-patrol oracle witness score disagrees with complete DP");
    }
    result.valid = true;
    return result;
}

[[nodiscard]] HeadResult solve_head(const Fixture& fixture) {
    udon::UdonShieldEngine engine(
        fixture.config,
        {},
        {},
        udon::RoutePoolSearch::SinglePass,
        kHarvestMode,
        false,
        kFutureHarvestMode);
    const udon::ExactStepSimulator cacheSimulator(fixture.config);
    const udon::IndependentDayValidator cacheValidator(fixture.config);
    const udon::RouteMaster cacheMaster(
        fixture.config,
        cacheSimulator,
        cacheValidator);
    const udon::FastViabilityAnalyzer cacheViability(fixture.config);
    std::vector<udon::AgentState> agents;
    for (const udon::CellId start : fixture.config.initialAgents) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            start,
            fixture.config.fuelLimit,
        });
    }
    udon::MatchLedger ledger;
    TrafficFootprint previousOwn{};
    TrafficFootprint priorOwn{};
    HeadResult result;
    std::int32_t sourceMultiDayWitnesses = 0;
    std::int32_t sourceMaximumPlans = 0;
    udon::OfficialScore sourceFinal;
    std::optional<udon::FutureWitness> sourceWitness;
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const std::vector<udon::RoadStatus> roadStatuses = exact_road_statuses(
            fixture,
            day,
            previousOwn,
            priorOwn);
        const udon::DayState state = day_state(
            fixture.config,
            day,
            agents,
            roadStatuses);
        bool suffixAttempted = false;
        bool suffixValid = false;
        bool suffixScoreMatches = false;
        udon::OfficialScore suffixReplayScore;
        if (sourceWitness.has_value() &&
            sourceWitness->futurePlans.size() > 1U &&
            fixture.config.roadCells.empty()) {
            suffixAttempted = true;
            suffixValid = true;
            udon::DayState suffixState = state;
            udon::MatchLedger suffixLedger = ledger;
            for (std::size_t planOffset = 0;
                 planOffset < sourceWitness->futurePlans.size();
                 ++planOffset) {
                udon::SimulationResult suffixSimulation;
                std::string suffixMismatch;
                if (!validates(
                        fixture.config,
                        suffixState,
                        sourceWitness->futurePlans.at(planOffset),
                        suffixSimulation,
                        suffixMismatch)) {
                    suffixValid = false;
                    break;
                }
                suffixLedger.apply(suffixSimulation.score);
                if (planOffset + 1U < sourceWitness->futurePlans.size()) {
                    suffixState = day_state(
                        fixture.config,
                        suffixState.dayNumber + 1,
                        suffixSimulation.finalAgents);
                }
            }
            suffixReplayScore = udon::OfficialScore{
                suffixLedger.lifetime_distinct(),
                suffixLedger.totalDailyDistinct,
                suffixLedger.totalServings,
            };
            suffixScoreMatches = suffixValid &&
                suffixReplayScore == sourceWitness->score;
        }
        std::set<std::string> cachedPlanIds;
        bool cachedValid = false;
        udon::OfficialScore cachedCurrent;
        udon::OfficialScore cachedUpper;
        for (const udon::ResponseLedger::CachedContingency& contingency :
             engine.response_ledger().cachedContingencies) {
            if (contingency.dayNumber == day) {
                cachedPlanIds.insert(
                    udon::serialize_day_plan(contingency.plan).dump());
                const std::optional<udon::MasterCandidate> exact =
                    cacheMaster.evaluate_exact_plan(
                        state,
                        ledger,
                        contingency.plan);
                if (!exact.has_value()) {
                    continue;
                }
                udon::OfficialScore upper = exact->scoreAfterToday;
                if (day < fixture.config.day_count()) {
                    udon::MatchLedger futureLedger = ledger;
                    futureLedger.apply(exact->simulation.score);
                    upper = cacheViability.analyze(
                        day_state(
                            fixture.config,
                            day + 1,
                            exact->simulation.finalAgents),
                        futureLedger).upperBound;
                }
                if (!cachedValid || cachedUpper < upper ||
                    (cachedUpper == upper &&
                     cachedCurrent < exact->scoreAfterToday)) {
                    cachedValid = true;
                    cachedCurrent = exact->scoreAfterToday;
                    cachedUpper = upper;
                }
            }
        }
        const auto started = std::chrono::steady_clock::now();
        const udon::DecisionResult decision =
            engine.solve_day(state, ledger, kProductionBudget);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started);
        udon::SimulationResult simulation;
        std::string mismatch;
        if (!validates(
                fixture.config,
                state,
                decision.candidate.plan,
                simulation,
                mismatch)) {
            return result;
        }
        engine.record_submitted(decision, elapsed);
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
        result.plans.push_back(decision.candidate.plan);
        result.cumulative.push_back(udon::OfficialScore{
            ledger.lifetime_distinct(),
            ledger.totalDailyDistinct,
            ledger.totalServings,
        });
        result.terminalAgents.push_back(agents);
        result.audits.push_back(decision.audit);
        HeadResult::CachePathAudit cachePath;
        cachePath.cached = static_cast<std::int32_t>(cachedPlanIds.size());
        cachePath.eligible = decision.cacheRepair.eligibleContingencies;
        cachePath.reused = decision.cacheRepair.reusedContingencies;
        cachePath.rejected = decision.cacheRepair.rejectedContingencies;
        cachePath.retained = static_cast<std::int32_t>(std::count_if(
            decision.audit.candidates.begin(),
            decision.audit.candidates.end(),
            [&cachedPlanIds](const udon::CandidateAuditRecord& record) {
                return cachedPlanIds.contains(record.stableId);
            }));
        cachePath.selected = cachedPlanIds.contains(
            decision.candidate.stableId);
        cachePath.cachedValid = cachedValid;
        cachePath.cachedCurrent = cachedCurrent;
        cachePath.cachedUpper = cachedUpper;
        const auto selectedAudit = std::find_if(
            decision.audit.candidates.begin(),
            decision.audit.candidates.end(),
            [](const udon::CandidateAuditRecord& record) {
                return record.selected;
            });
        if (selectedAudit != decision.audit.candidates.end()) {
            cachePath.selectedCurrent = selectedAudit->scoreAfterToday;
            cachePath.selectedUpper = selectedAudit->validUpperBound;
        }
        cachePath.sourceMultiDayWitnesses = sourceMultiDayWitnesses;
        cachePath.sourceMaximumPlans = sourceMaximumPlans;
        cachePath.sourceFinal = sourceFinal;
        cachePath.suffixAttempted = suffixAttempted;
        cachePath.suffixValid = suffixValid;
        cachePath.suffixScoreMatches = suffixScoreMatches;
        cachePath.suffixReplayScore = suffixReplayScore;
        result.cachePaths.push_back(cachePath);
        sourceMultiDayWitnesses = 0;
        sourceMaximumPlans = 0;
        sourceFinal = {};
        sourceWitness.reset();
        for (const udon::ScenarioOutcome& outcome : decision.profile.outcomes) {
            if (!outcome.witness.certified ||
                outcome.witness.futurePlans.size() <= 1U) {
                continue;
            }
            ++sourceMultiDayWitnesses;
            sourceMaximumPlans = std::max(
                sourceMaximumPlans,
                static_cast<std::int32_t>(
                    outcome.witness.futurePlans.size()));
            sourceFinal = std::max(sourceFinal, outcome.score);
            if (!sourceWitness.has_value() ||
                sourceWitness->score < outcome.score) {
                sourceWitness = outcome.witness;
            }
        }
        priorOwn = previousOwn;
        previousOwn = compact_road_footprint(
            fixture.config,
            simulation.roadFootprint);
    }
    result.score = udon::OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
    result.valid = true;
    return result;
}

[[nodiscard]] std::int32_t first_tier(
    const udon::OfficialScore& oracle,
    const udon::OfficialScore& head) {
    if (oracle.lifetimeDistinct != head.lifetimeDistinct) {
        return 1;
    }
    if (oracle.totalDailyDistinct != head.totalDailyDistinct) {
        return 2;
    }
    if (oracle.totalServings != head.totalServings) {
        return 3;
    }
    return 0;
}

[[nodiscard]] std::int32_t tier_gain(
    const udon::OfficialScore& oracle,
    const udon::OfficialScore& head,
    std::int32_t tier) {
    if (tier == 1) {
        return oracle.lifetimeDistinct - head.lifetimeDistinct;
    }
    if (tier == 2) {
        return oracle.totalDailyDistinct - head.totalDailyDistinct;
    }
    if (tier == 3) {
        return oracle.totalServings - head.totalServings;
    }
    return 0;
}

void record_summary(
    Summary& summary,
    const OracleResult& oracle,
    const HeadResult& head,
    std::uint64_t seed) {
    ++summary.cases;
    if (!oracle.valid || !head.valid) {
        ++summary.invalid;
        return;
    }
    if (head.score < oracle.score) {
        ++summary.oracleWins;
    } else if (oracle.score < head.score) {
        ++summary.headWins;
    } else {
        ++summary.ties;
    }
    const std::int32_t tier = first_tier(oracle.score, head.score);
    if (tier == 1) {
        ++summary.tier1;
    } else if (tier == 2) {
        ++summary.tier2;
    } else if (tier == 3) {
        ++summary.tier3;
    }
    summary.maximumGain = std::max(summary.maximumGain, tier_gain(oracle.score, head.score, tier));
    for (const std::int32_t value : {
             static_cast<std::int32_t>(seed & 0x7fffffffU),
             oracle.score.lifetimeDistinct,
             oracle.score.totalDailyDistinct,
             oracle.score.totalServings,
             head.score.lifetimeDistinct,
             head.score.totalDailyDistinct,
             head.score.totalServings,
         }) {
        hash_value(summary.resultHash, static_cast<std::uint64_t>(value));
    }
}

[[nodiscard]] std::string score_text(const udon::OfficialScore& score) {
    return std::to_string(score.lifetimeDistinct) + '/' +
        std::to_string(score.totalDailyDistinct) + '/' +
        std::to_string(score.totalServings);
}

[[nodiscard]] std::string agents_text(const std::vector<udon::AgentState>& agents) {
    std::ostringstream output;
    for (std::size_t index = 0; index < agents.size(); ++index) {
        if (index != 0U) {
            output << '|';
        }
        output << agents.at(index).position << '@' << agents.at(index).fuel;
    }
    return output.str();
}

using TerminalResourceSignature =
    std::vector<std::pair<udon::CellId, std::int32_t>>;

[[nodiscard]] TerminalResourceSignature terminal_resource_signature(
    const std::vector<udon::AgentState>& agents) {
    TerminalResourceSignature signature;
    signature.reserve(agents.size());
    for (const udon::AgentState& agent : agents) {
        signature.emplace_back(agent.position, agent.fuel);
    }
    std::sort(signature.begin(), signature.end());
    return signature;
}

[[nodiscard]] TerminalResourceSignature terminal_resource_signature(
    const udon::CandidateAuditRecord& record) {
    if (record.terminalCells.size() != record.terminalFuel.size()) {
        throw std::runtime_error(
            "candidate audit terminal cell/fuel widths disagree");
    }
    TerminalResourceSignature signature;
    signature.reserve(record.terminalCells.size());
    for (std::size_t index = 0; index < record.terminalCells.size(); ++index) {
        signature.emplace_back(
            record.terminalCells.at(index),
            record.terminalFuel.at(index));
    }
    std::sort(signature.begin(), signature.end());
    return signature;
}

[[nodiscard]] std::string terminal_resource_text(
    const udon::CandidateAuditRecord& record) {
    std::ostringstream output;
    for (std::size_t index = 0; index < record.terminalCells.size(); ++index) {
        if (index != 0U) {
            output << '|';
        }
        output << record.terminalCells.at(index) << '@'
               << record.terminalFuel.at(index);
    }
    return output.str();
}

[[nodiscard]] std::string plan_text(const udon::DayPlan& plan) {
    std::ostringstream output;
    for (std::size_t agent = 0; agent < plan.actions.size(); ++agent) {
        if (agent != 0U) {
            output << '|';
        }
        for (std::size_t action = 0; action < plan.actions.at(agent).size(); ++action) {
            if (action != 0U) {
                output << '.';
            }
            output << plan.actions.at(agent).at(action).wire_value();
        }
    }
    return output.str();
}

[[nodiscard]] udon::DayPlan parse_plan_text(
    const std::string& text,
    std::int32_t agentCount) {
    udon::DayPlan plan;
    std::stringstream agents{text};
    std::string agentText;
    while (std::getline(agents, agentText, '|')) {
        udon::AgentPlan actions;
        std::stringstream tokens{agentText};
        std::string token;
        while (std::getline(tokens, token, '.')) {
            if (token.empty()) {
                throw std::invalid_argument("inspect plan contains an empty action");
            }
            const std::int32_t wire = std::stoi(token);
            if (wire < 0) {
                actions.push_back(udon::PlanAction::wait(-wire));
            } else if (wire < udon::kDirectionCount) {
                actions.push_back(udon::PlanAction::move(wire));
            } else {
                throw std::invalid_argument("inspect plan contains an invalid wire action");
            }
        }
        if (actions.empty()) {
            throw std::invalid_argument("inspect plan contains an empty agent plan");
        }
        plan.actions.push_back(std::move(actions));
    }
    if (static_cast<std::int32_t>(plan.actions.size()) != agentCount) {
        throw std::invalid_argument("inspect plan agent count mismatch");
    }
    return plan;
}

[[nodiscard]] std::uint64_t plan_sequence_hash(
    const std::vector<udon::DayPlan>& plans) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const udon::DayPlan& plan : plans) {
        for (const udon::AgentPlan& actions : plan.actions) {
            for (const udon::PlanAction& action : actions) {
                hash_value(
                    hash,
                    static_cast<std::uint64_t>(action.wire_value()));
            }
            hash_value(hash, std::numeric_limits<std::uint64_t>::max());
        }
        hash_value(hash, std::numeric_limits<std::uint64_t>::max() - 1U);
    }
    return hash;
}

enum class CausalOpponentPolicy : std::uint8_t {
    MaximumDwell,
    MinimumDwell,
    StatusToggle,
};

struct AdversarialRun {
    bool valid = false;
    udon::OfficialScore score;
    udon::OfficialScore virtualScore;
    std::vector<udon::DayPlan> plans;
    std::vector<udon::DayPlan> opponentPlans;
    struct DayTrace {
        TrafficStatusKey startStatuses{};
        std::int32_t dailyDistinct = 0;
        std::int32_t dailyServings = 0;
        udon::CellId terminalPosition = udon::kInvalidCell;
        std::int32_t terminalFuel = 0;
        TrafficFootprint ownFootprint{};
        udon::CellId opponentTerminalPosition = udon::kInvalidCell;
        std::int32_t opponentTerminalFuel = 0;
        TrafficFootprint opponentFootprint{};
        udon::OfficialScore cumulative;
    };
    std::vector<DayTrace> traces;
    std::vector<udon::DecisionAudit> audits;
    std::int32_t deadlineLimitedDays = 0;
    std::int32_t protectedTakeovers = 0;
    std::int64_t protectedGeneratedPlans = 0;
    std::int64_t protectedValidPlans = 0;
    std::int64_t protectedLiftablePlans = 0;
    std::int32_t protectedDeadlineDays = 0;
    std::int64_t terminalSparseRoutes = 0;
    std::int64_t terminalValidPlans = 0;
    std::int64_t terminalStrictImprovements = 0;
    std::int64_t terminalRounds = 0;
    std::int32_t terminalDeadlineDays = 0;
};

[[nodiscard]] std::string_view policy_name(CausalOpponentPolicy policy) {
    switch (policy) {
    case CausalOpponentPolicy::MaximumDwell:
        return "maximum-dwell";
    case CausalOpponentPolicy::MinimumDwell:
        return "minimum-dwell";
    case CausalOpponentPolicy::StatusToggle:
        return "status-toggle";
    }
    return "unknown";
}

[[nodiscard]] std::vector<udon::AgentState> adversarial_agents(
    const Fixture& fixture,
    bool opponent,
    udon::CellId activePosition,
    std::int32_t activeFuel) {
    static_cast<void>(opponent);
    std::vector<udon::AgentState> agents;
    agents.reserve(static_cast<std::size_t>(fixture.config.agent_count()));
    agents.push_back(udon::AgentState{
        udon::AgentKind::Patrol,
        activePosition,
        activeFuel,
    });
    for (std::int32_t index = 1;
         index < fixture.config.agent_count();
         ++index) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            fixture.config.initialAgents.at(static_cast<std::size_t>(index)),
            fixture.config.fuelLimit,
        });
    }
    return agents;
}

[[nodiscard]] udon::DayPlan active_day_plan(
    const Fixture& fixture,
    std::int32_t day,
    const DayOutcome& outcome) {
    udon::DayPlan plan;
    plan.actions.resize(static_cast<std::size_t>(fixture.config.agent_count()));
    plan.actions.at(0) = outcome.actions;
    for (std::size_t index = 1; index < plan.actions.size(); ++index) {
        plan.actions.at(index).push_back(
            udon::PlanAction::wait(fixture.config.steps_for_day(day)));
    }
    return plan;
}

[[nodiscard]] std::vector<udon::DayPlan> parse_plan_sequence_text(
    const std::string& text,
    std::int32_t agentCount) {
    std::vector<udon::DayPlan> plans;
    std::stringstream days{text};
    std::string dayText;
    while (std::getline(days, dayText, ';')) {
        if (dayText.empty()) {
            throw std::invalid_argument("forced prefix contains an empty day plan");
        }
        plans.push_back(parse_plan_text(dayText, agentCount));
    }
    return plans;
}

[[nodiscard]] udon::DayState adversarial_day_state(
    const Fixture& fixture,
    const AdversarialKey& key) {
    const auto& [
        day,
        ownPosition,
        ownFuel,
        opponentPosition,
        opponentFuel,
        lifetime,
        previousOwn,
        previousOpponent,
        roadStatusKey] = key;
    static_cast<void>(lifetime);
    static_cast<void>(previousOwn);
    static_cast<void>(previousOpponent);
    udon::DayState state = day_state(
        fixture.config,
        day,
        adversarial_agents(
            fixture,
            false,
            ownPosition,
            ownFuel),
        expand_road_statuses(fixture.config, roadStatusKey));
    state.others.push_back(udon::OtherTeamState{
        1,
        adversarial_agents(
            fixture,
            true,
            opponentPosition,
            opponentFuel),
    });
    return state;
}

[[nodiscard]] std::int32_t footprint_mass(const TrafficFootprint& footprint) {
    return std::accumulate(
        footprint.begin(),
        footprint.end(),
        0);
}

[[nodiscard]] DayOutcome choose_causal_opponent(
    CausalOpponentPolicy policy,
    const TrafficStatusKey& statuses,
    const std::vector<DayOutcome>& outcomes) {
    if (outcomes.empty()) {
        throw std::runtime_error("causal opponent has no legal action");
    }
    bool maximize = policy == CausalOpponentPolicy::MaximumDwell;
    if (policy == CausalOpponentPolicy::StatusToggle) {
        maximize = statuses.at(0) == static_cast<std::uint8_t>(
            udon::RoadStatus::Smooth);
    }
    std::size_t selected = 0;
    for (std::size_t index = 1; index < outcomes.size(); ++index) {
        const std::int32_t candidateMass = footprint_mass(
            outcomes.at(index).roadFootprint);
        const std::int32_t selectedMass = footprint_mass(
            outcomes.at(selected).roadFootprint);
        const auto candidateTie = std::tie(
            outcomes.at(index).position,
            outcomes.at(index).fuel,
            outcomes.at(index).roadFootprint);
        const auto selectedTie = std::tie(
            outcomes.at(selected).position,
            outcomes.at(selected).fuel,
            outcomes.at(selected).roadFootprint);
        if ((maximize && candidateMass > selectedMass) ||
            (!maximize && candidateMass < selectedMass) ||
            (candidateMass == selectedMass && candidateTie < selectedTie)) {
            selected = index;
        }
    }
    return outcomes.at(selected);
}

[[nodiscard]] bool validate_adversarial_outcome(
    const Fixture& fixture,
    const udon::DayState& publicState,
    bool opponent,
    const DayOutcome& outcome,
    udon::SimulationResult& simulation) {
    udon::DayState state = publicState;
    if (opponent) {
        state.agents = publicState.others.front().agents;
        state.others.clear();
    }
    std::string mismatch;
    const bool valid = validates(
        fixture.config,
        state,
        active_day_plan(fixture, state.dayNumber, outcome),
        simulation,
        mismatch);
    if (!valid || simulation.finalAgents.at(0).position != outcome.position ||
        simulation.finalAgents.at(0).fuel != outcome.fuel ||
        compact_road_footprint(
            fixture.config,
            simulation.roadFootprint) != outcome.roadFootprint) {
        return false;
    }
    return true;
}

[[nodiscard]] AdversarialKey initial_adversarial_key(
    const Fixture& fixture,
    std::int32_t startDay = 1) {
    const TrafficFootprint empty{};
    const TrafficStatusKey smooth{};
    return AdversarialKey{
        startDay,
        fixture.config.initialAgents.at(0),
        fixture.config.fuelLimit,
        28,
        fixture.config.fuelLimit,
        0U,
        empty,
        empty,
        smooth,
    };
}

[[nodiscard]] AdversarialKey advance_adversarial_key(
    const Fixture& fixture,
    const AdversarialKey& key,
    const DayOutcome& own,
    const DayOutcome& opponent,
    udon::BrandMask dailyBrands,
    bool trafficHistoryQuotient = false) {
    const auto& [
        day,
        ownPosition,
        ownFuel,
        opponentPosition,
        opponentFuel,
        lifetime,
        previousOwn,
        previousOpponent,
        roadStatusKey] = key;
    static_cast<void>(ownPosition);
    static_cast<void>(ownFuel);
    static_cast<void>(opponentPosition);
    static_cast<void>(opponentFuel);
    static_cast<void>(roadStatusKey);
    const TrafficFootprint nextPreviousOwn = trafficHistoryQuotient
        ? combined_traffic_history(
              fixture.config,
              own.roadFootprint,
              opponent.roadFootprint)
        : own.roadFootprint;
    const TrafficFootprint nextPreviousOpponent = trafficHistoryQuotient
        ? TrafficFootprint{}
        : opponent.roadFootprint;
    return AdversarialKey{
        day + 1,
        own.position,
        own.fuel,
        opponent.position,
        opponent.fuel,
        lifetime | dailyBrands,
        nextPreviousOwn,
        nextPreviousOpponent,
        adversarial_next_statuses(
            fixture.config,
            own.roadFootprint,
            previousOwn,
            opponent.roadFootprint,
            previousOpponent),
    };
}

[[nodiscard]] AdversarialRun run_minimax_policy(
    const Fixture& fixture,
    const MinimaxMemo& memo,
    CausalOpponentPolicy policy,
    bool trafficHistoryQuotient) {
    AdversarialRun result;
    AdversarialKey key = initial_adversarial_key(fixture);
    udon::MatchLedger ledger;
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const auto node = memo.find(key);
        if (node == memo.end()) {
            return result;
        }
        const udon::DayState state = adversarial_day_state(fixture, key);
        const TrafficStatusKey statuses = std::get<8>(key);
        const std::vector<DayOutcome> opponentOutcomes = opponent_day_outcomes(
            fixture.config,
            day,
            std::get<3>(key),
            std::get<4>(key),
            state.roadStatuses);
        const DayOutcome opponent = choose_causal_opponent(
            policy,
            statuses,
            opponentOutcomes);
        udon::SimulationResult ownSimulation;
        udon::SimulationResult opponentSimulation;
        if (!validate_adversarial_outcome(
                fixture,
                state,
                false,
                node->second.bestOwn,
                ownSimulation) ||
            !validate_adversarial_outcome(
                fixture,
                state,
                true,
                opponent,
                opponentSimulation)) {
            return result;
        }
        ledger.apply(ownSimulation.score);
        result.plans.push_back(active_day_plan(
            fixture,
            day,
            node->second.bestOwn));
        result.opponentPlans.push_back(active_day_plan(
            fixture,
            day,
            opponent));
        result.traces.push_back(AdversarialRun::DayTrace{
            statuses,
            udon::brand_count(ownSimulation.score.brands),
            ownSimulation.score.servings,
            ownSimulation.finalAgents.at(0).position,
            ownSimulation.finalAgents.at(0).fuel,
            compact_road_footprint(fixture.config, ownSimulation.roadFootprint),
            opponentSimulation.finalAgents.at(0).position,
            opponentSimulation.finalAgents.at(0).fuel,
            compact_road_footprint(
                fixture.config,
                opponentSimulation.roadFootprint),
            udon::OfficialScore{
                ledger.lifetime_distinct(),
                ledger.totalDailyDistinct,
                ledger.totalServings,
            },
        });
        key = advance_adversarial_key(
            fixture,
            key,
            node->second.bestOwn,
            opponent,
            ownSimulation.score.brands,
            trafficHistoryQuotient);
    }
    result.score = udon::OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
    result.valid = true;
    return result;
}

[[nodiscard]] AdversarialRun run_head_against_policy(
    const Fixture& fixture,
    CausalOpponentPolicy policy,
    const std::vector<udon::DayPlan>* forcedPrefix = nullptr) {
    AdversarialRun result;
    udon::MatchSession session(
        fixture.config,
        {},
        {},
        kHarvestMode,
        kFutureHarvestMode);
    udon::MatchLedger ledger;
    AdversarialKey key = initial_adversarial_key(fixture);
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const udon::DayState state = adversarial_day_state(fixture, key);
        udon::DayPlan ownPlan;
        bool sessionSelected = false;
        bool deadlineLimited = false;
        if (forcedPrefix != nullptr &&
            static_cast<std::size_t>(day) <= forcedPrefix->size()) {
            ownPlan = forcedPrefix->at(static_cast<std::size_t>(day - 1));
        } else {
            const udon::SessionDecision decision = session.on_authoritative_state_for(
                state,
                ledger,
                kProductionBudget);
            if (!decision.maySubmit) {
                return result;
            }
            ownPlan = decision.decision.candidate.plan;
            result.audits.push_back(decision.decision.audit);
            sessionSelected = true;
            deadlineLimited =
                decision.decision.viability.deadlineReached ||
                decision.decision.diagnostics.deadlineReached ||
                decision.decision.audit.independentDeadlineReached ||
                decision.decision.cacheRepair.deadlineReached;
        }
        udon::SimulationResult ownSimulation;
        std::string mismatch;
        if (!validates(
                fixture.config,
                state,
                ownPlan,
                ownSimulation,
                mismatch)) {
            return result;
        }
        const std::vector<DayOutcome> opponentOutcomes = opponent_day_outcomes(
            fixture.config,
            day,
            std::get<3>(key),
            std::get<4>(key),
            state.roadStatuses);
        const DayOutcome opponent = choose_causal_opponent(
            policy,
            std::get<8>(key),
            opponentOutcomes);
        udon::SimulationResult opponentSimulation;
        if (!validate_adversarial_outcome(
                fixture,
                state,
                true,
                opponent,
                opponentSimulation)) {
            return result;
        }
        if (sessionSelected) {
            static_cast<void>(session.acknowledge_submitted(
                std::chrono::milliseconds{0},
                std::chrono::milliseconds{0}));
        }
        ledger.apply(ownSimulation.score);
        result.plans.push_back(ownPlan);
        result.opponentPlans.push_back(active_day_plan(
            fixture,
            day,
            opponent));
        result.traces.push_back(AdversarialRun::DayTrace{
            std::get<8>(key),
            udon::brand_count(ownSimulation.score.brands),
            ownSimulation.score.servings,
            ownSimulation.finalAgents.at(0).position,
            ownSimulation.finalAgents.at(0).fuel,
            compact_road_footprint(fixture.config, ownSimulation.roadFootprint),
            opponentSimulation.finalAgents.at(0).position,
            opponentSimulation.finalAgents.at(0).fuel,
            compact_road_footprint(
                fixture.config,
                opponentSimulation.roadFootprint),
            udon::OfficialScore{
                ledger.lifetime_distinct(),
                ledger.totalDailyDistinct,
                ledger.totalServings,
            },
        });
        result.deadlineLimitedDays += deadlineLimited ? 1 : 0;
        const TrafficFootprint ownFootprint = compact_road_footprint(
            fixture.config,
            ownSimulation.roadFootprint);
        const DayOutcome own{
            ownSimulation.finalAgents.at(0).position,
            ownSimulation.finalAgents.at(0).fuel,
            0U,
            ownPlan.actions.at(0),
            ownFootprint,
        };
        key = advance_adversarial_key(
            fixture,
            key,
            own,
            opponent,
            ownSimulation.score.brands);
    }
    result.score = udon::OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
    result.valid = true;
    return result;
}

[[nodiscard]] AdversarialRun run_current_protected_against_policy(
    const Fixture& fixture,
    CausalOpponentPolicy policy,
    bool latestTerminal,
    std::chrono::milliseconds protectedRefinementReserve) {
    AdversarialRun result;
    udon::DeadlineCalibration calibration;
    calibration.version = "btc-http-local-budget-v8-idempotent-ack-resend";
    calibration.networkFloor = kBtcNetworkReserve;
    calibration.networkPercent = 20;
    calibration.certificationPercent = 20;
    udon::MatchSession session(
        fixture.config,
        {},
        calibration,
        kHarvestMode,
        kFutureHarvestMode);
    const udon::ExactStepSimulator simulator(fixture.config);
    const udon::IndependentDayValidator validator(fixture.config);
    const udon::ProtectedSlackRefiner slackRefiner(fixture.config);
    udon::MatchLedger ledger;
    udon::MatchLedger virtualLedger;
    std::vector<udon::AgentState> virtualAgents;
    bool protectedDivergenceActive = false;
    AdversarialKey key = initial_adversarial_key(fixture);
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const udon::DayState state = adversarial_day_state(fixture, key);
        if (!protectedDivergenceActive || virtualAgents.empty() ||
            !udon::protected_slack_agents_dominate(
                virtualAgents,
                state.agents) ||
            !udon::protected_slack_ledger_dominates(
                virtualLedger,
                ledger)) {
            virtualAgents = state.agents;
            virtualLedger = ledger;
            protectedDivergenceActive = false;
        }
        udon::DayState planningState = state;
        planningState.agents = virtualAgents;
        const auto dayStarted = std::chrono::steady_clock::now();
        const udon::SessionDecision decision = session.on_authoritative_state_for(
            planningState,
            virtualLedger,
            kProductionBudget);
        if (!decision.maySubmit) {
            return result;
        }
        result.audits.push_back(decision.decision.audit);
        const bool deadlineLimited =
            decision.decision.viability.deadlineReached ||
            decision.decision.diagnostics.deadlineReached ||
            decision.decision.audit.independentDeadlineReached ||
            decision.decision.cacheRepair.deadlineReached;

        const udon::DayPlan parentPlan = decision.decision.candidate.plan;
        udon::SimulationResult virtualSimulation;
        udon::SimulationResult submittedSimulation;
        std::string mismatch;
        if (!validates(
                fixture.config,
                planningState,
                parentPlan,
                virtualSimulation,
                mismatch) ||
            !validates(
                fixture.config,
                state,
                parentPlan,
                submittedSimulation,
                mismatch)) {
            return result;
        }

        udon::DayPlan submittedPlan = parentPlan;
        bool submittedProtectedImprovement = false;
        const auto protectedDeadline =
            dayStarted + kProductionBudget - protectedRefinementReserve;
        if (std::chrono::steady_clock::now() < protectedDeadline &&
            (day < fixture.config.day_count() || latestTerminal)) {
            udon::ProtectedSlackResult refinement =
                day == fixture.config.day_count()
                ? slackRefiner.refine_terminal_sparse(
                      state,
                      ledger,
                      parentPlan,
                      submittedSimulation,
                      protectedDeadline)
                : slackRefiner.refine_wait_detours(
                      state,
                      ledger,
                      parentPlan,
                      submittedSimulation,
                      protectedDeadline);
            result.protectedGeneratedPlans +=
                refinement.diagnostics.generatedPlans;
            result.protectedValidPlans += refinement.diagnostics.validPlans;
            result.protectedLiftablePlans +=
                refinement.diagnostics.liftablePlans;
            result.protectedDeadlineDays +=
                refinement.diagnostics.deadlineReached ? 1 : 0;
            if (day == fixture.config.day_count()) {
                result.terminalSparseRoutes +=
                    refinement.diagnostics.sparseRoutes;
                result.terminalValidPlans +=
                    refinement.diagnostics.validPlans;
                result.terminalStrictImprovements +=
                    refinement.diagnostics.strictTerminalImprovements;
                result.terminalRounds +=
                    refinement.diagnostics.terminalSparseRounds;
                result.terminalDeadlineDays +=
                    refinement.diagnostics.deadlineReached ? 1 : 0;
            }
            if (refinement.improved) {
                submittedPlan = std::move(refinement.plan);
                submittedSimulation = std::move(refinement.simulation);
                submittedProtectedImprovement = true;
                ++result.protectedTakeovers;
            }
        }

        udon::MatchLedger prospectiveVirtualLedger = virtualLedger;
        prospectiveVirtualLedger.apply(virtualSimulation.score);
        udon::MatchLedger prospectiveActualLedger = ledger;
        prospectiveActualLedger.apply(submittedSimulation.score);
        const bool finalDay = day == fixture.config.day_count();
        const bool admissible = finalDay && latestTerminal
            ? !(udon::OfficialScore{
                    prospectiveActualLedger.lifetime_distinct(),
                    prospectiveActualLedger.totalDailyDistinct,
                    prospectiveActualLedger.totalServings} <
                udon::OfficialScore{
                    prospectiveVirtualLedger.lifetime_distinct(),
                    prospectiveVirtualLedger.totalDailyDistinct,
                    prospectiveVirtualLedger.totalServings})
            : udon::protected_slack_transition_dominates(
                  virtualSimulation,
                  submittedSimulation) &&
                udon::protected_slack_ledger_dominates(
                    prospectiveVirtualLedger,
                    prospectiveActualLedger);
        if (!admissible) {
            return result;
        }

        const std::vector<DayOutcome> opponentOutcomes = opponent_day_outcomes(
            fixture.config,
            day,
            std::get<3>(key),
            std::get<4>(key),
            state.roadStatuses);
        const DayOutcome opponent = choose_causal_opponent(
            policy,
            std::get<8>(key),
            opponentOutcomes);
        udon::SimulationResult opponentSimulation;
        if (!validate_adversarial_outcome(
                fixture,
                state,
                true,
                opponent,
                opponentSimulation)) {
            return result;
        }

        static_cast<void>(session.acknowledge_submitted(
            std::chrono::milliseconds{0},
            std::chrono::milliseconds{0}));
        virtualLedger = prospectiveVirtualLedger;
        virtualAgents = virtualSimulation.finalAgents;
        ledger = prospectiveActualLedger;
        protectedDivergenceActive =
            protectedDivergenceActive || submittedProtectedImprovement;
        if (!udon::protected_slack_ledger_dominates(
                virtualLedger,
                ledger)) {
            return result;
        }

        result.plans.push_back(submittedPlan);
        result.opponentPlans.push_back(active_day_plan(
            fixture,
            day,
            opponent));
        result.traces.push_back(AdversarialRun::DayTrace{
            std::get<8>(key),
            udon::brand_count(submittedSimulation.score.brands),
            submittedSimulation.score.servings,
            submittedSimulation.finalAgents.at(0).position,
            submittedSimulation.finalAgents.at(0).fuel,
            compact_road_footprint(
                fixture.config,
                submittedSimulation.roadFootprint),
            opponentSimulation.finalAgents.at(0).position,
            opponentSimulation.finalAgents.at(0).fuel,
            compact_road_footprint(
                fixture.config,
                opponentSimulation.roadFootprint),
            udon::OfficialScore{
                ledger.lifetime_distinct(),
                ledger.totalDailyDistinct,
                ledger.totalServings,
            },
        });
        result.deadlineLimitedDays += deadlineLimited ? 1 : 0;
        const DayOutcome own{
            submittedSimulation.finalAgents.at(0).position,
            submittedSimulation.finalAgents.at(0).fuel,
            0U,
            submittedPlan.actions.at(0),
            compact_road_footprint(
                fixture.config,
                submittedSimulation.roadFootprint),
        };
        key = advance_adversarial_key(
            fixture,
            key,
            own,
            opponent,
            submittedSimulation.score.brands);
    }
    result.score = udon::OfficialScore{
        ledger.lifetime_distinct(),
        ledger.totalDailyDistinct,
        ledger.totalServings,
    };
    result.virtualScore = udon::OfficialScore{
        virtualLedger.lifetime_distinct(),
        virtualLedger.totalDailyDistinct,
        virtualLedger.totalServings,
    };
    result.valid = true;
    return result;
}

struct AdversarialPrefixContext {
    bool valid = false;
    AdversarialKey key;
    udon::MatchLedger ledger;
};

[[nodiscard]] AdversarialPrefixContext replay_adversarial_prefix(
    const Fixture& fixture,
    CausalOpponentPolicy policy,
    const std::vector<udon::DayPlan>& prefix) {
    AdversarialPrefixContext result;
    result.key = initial_adversarial_key(fixture);
    for (std::size_t index = 0; index < prefix.size(); ++index) {
        const std::int32_t day = static_cast<std::int32_t>(index + 1U);
        const udon::DayState state = adversarial_day_state(fixture, result.key);
        udon::SimulationResult ownSimulation;
        std::string mismatch;
        if (!validates(
                fixture.config,
                state,
                prefix.at(index),
                ownSimulation,
                mismatch)) {
            return result;
        }
        const std::vector<DayOutcome> opponentOutcomes = opponent_day_outcomes(
            fixture.config,
            day,
            std::get<3>(result.key),
            std::get<4>(result.key),
            state.roadStatuses);
        const DayOutcome opponent = choose_causal_opponent(
            policy,
            std::get<8>(result.key),
            opponentOutcomes);
        udon::SimulationResult opponentSimulation;
        if (!validate_adversarial_outcome(
                fixture,
                state,
                true,
                opponent,
                opponentSimulation)) {
            return result;
        }
        result.ledger.apply(ownSimulation.score);
        const DayOutcome own{
            ownSimulation.finalAgents.at(0).position,
            ownSimulation.finalAgents.at(0).fuel,
            0U,
            prefix.at(index).actions.at(0),
            compact_road_footprint(
                fixture.config,
                ownSimulation.roadFootprint),
        };
        result.key = advance_adversarial_key(
            fixture,
            result.key,
            own,
            opponent,
            ownSimulation.score.brands);
    }
    result.valid = true;
    return result;
}

[[nodiscard]] bool same_agent_plan(
    const udon::AgentPlan& left,
    const udon::AgentPlan& right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t action = 0U; action < left.size(); ++action) {
        if (left.at(action).wire_value() != right.at(action).wire_value()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::string portfolio_plan_mask(
    const udon::RoutePortfolio& portfolio,
    const udon::DayPlan& plan) {
    std::string mask;
    for (std::size_t agent = 0U; agent < plan.actions.size(); ++agent) {
        const bool present = agent < portfolio.columnsByAgent.size() &&
            std::any_of(
                portfolio.columnsByAgent.at(agent).begin(),
                portfolio.columnsByAgent.at(agent).end(),
                [&plan, agent](const udon::RouteColumn& column) {
                    return same_agent_plan(column.actions, plan.actions.at(agent));
                });
        mask += present ? '1' : '0';
    }
    return mask;
}

[[nodiscard]] std::string matching_column_flags(
    const udon::RoutePortfolio& portfolio,
    const udon::DayPlan& plan,
    std::size_t agent) {
    if (agent >= portfolio.columnsByAgent.size() ||
        agent >= plan.actions.size()) {
        return "missing";
    }
    std::ostringstream output;
    bool found = false;
    for (const udon::RouteColumn& column : portfolio.columnsByAgent.at(agent)) {
        if (!same_agent_plan(column.actions, plan.actions.at(agent))) {
            continue;
        }
        if (found) {
            output << '|';
        }
        found = true;
        output << 'h' << (column.harvestExtension ? 1 : 0)
               << 'e' << (column.exactOrienteering ? 1 : 0)
               << 's' << column.harvestExtensionSourceRank
               << 'b' << column.contingencyBundle
               << 'p' << column.priority
               << 't' << column.terminalCell
               << 'f' << column.terminalFuel;
    }
    return found ? output.str() : "missing";
}

[[nodiscard]] bool contains_candidate(
    const std::vector<udon::MasterCandidate>& candidates,
    const udon::MasterCandidate& exact) {
    return std::any_of(
        candidates.begin(),
        candidates.end(),
        [&exact](const udon::MasterCandidate& candidate) {
            return candidate.stableId == exact.stableId;
        });
}

[[nodiscard]] bool contains_outcome(
    const std::vector<udon::MasterCandidate>& candidates,
    const udon::MasterCandidate& exact) {
    return std::any_of(
        candidates.begin(),
        candidates.end(),
        [&exact](const udon::MasterCandidate& candidate) {
            if (!(candidate.scoreAfterToday == exact.scoreAfterToday) ||
                candidate.simulation.finalAgents.size() !=
                    exact.simulation.finalAgents.size()) {
                return false;
            }
            for (std::size_t agent = 0U;
                 agent < candidate.simulation.finalAgents.size();
                 ++agent) {
                const udon::AgentState& left =
                    candidate.simulation.finalAgents.at(agent);
                const udon::AgentState& right =
                    exact.simulation.finalAgents.at(agent);
                if (left.kind != right.kind || left.position != right.position ||
                    left.fuel != right.fuel) {
                    return false;
                }
            }
            return true;
        });
}

void inspect_exact_bundle_capability(
    const Fixture& fixture,
    const udon::DayPlan& plan) {
    std::vector<udon::AgentState> agents;
    for (const udon::CellId start : fixture.config.initialAgents) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            start,
            fixture.config.fuelLimit,
        });
    }
    const TrafficFootprint emptyFootprint{};
    const udon::DayState state = day_state(
        fixture.config,
        1,
        agents,
        exact_road_statuses(
            fixture,
            1,
            emptyFootprint,
            emptyFootprint));
    const udon::ExactStepSimulator simulator(fixture.config);
    const udon::IndependentDayValidator validator(fixture.config);
    const udon::ParetoRouter router(fixture.config);
    const udon::RouteColumnGenerator generator(fixture.config, router);
    const udon::RouteMaster master(fixture.config, simulator, validator);
    const udon::GreedyPlanner greedy(fixture.config, generator, master);
    const udon::FastViabilityAnalyzer viabilityAnalyzer(fixture.config);
    const udon::MatchLedger ledger;
    const std::optional<udon::MasterCandidate> exact =
        master.evaluate_exact_plan(state, ledger, plan);
    if (!exact.has_value()) {
        throw std::runtime_error("inspect plan is not dual-valid on day 1");
    }

    udon::ColumnGenerationOptions seedGeneration;
    seedGeneration.maximumPathsPerTarget = 1;
    seedGeneration.maximumColumnsPerAgent = 2;
    seedGeneration.maximumTargetSpots = 4;
    seedGeneration.maximumEscorts = 2;
    udon::MasterOptions seedMaster;
    seedMaster.maximumCombinations = 512;
    seedMaster.maximumCandidates = 1;
    udon::MasterDiagnostics seedDiagnostics;
    const udon::MasterCandidate incumbent = greedy.build_incumbent(
        state,
        ledger,
        seedGeneration,
        seedMaster,
        seedDiagnostics);
    const udon::ViabilityBounds viability = viabilityAnalyzer.analyze(
        state,
        ledger);

    udon::ColumnGenerationOptions generation;
    generation.maximumPathsPerTarget = 4;
    generation.maximumColumnsPerAgent = 16;
    generation.maximumTargetSpots = 12;
    generation.maximumEscorts = 16;
    generation.maximumSeedPlans = 2;
    generation.enableHarvestExtensions = true;
    generation.allowUncachedHarvestTargets = true;
    generation.enableHarvestOrienteering = false;
    generation.enableExactHarvestOrienteering = true;
    generation.enableFuelConstrainedExactHarvestOrienteering = true;
    generation.enableAnytimeFuelConstrainedHarvestOrienteering = false;
    generation.maximumHarvestExtensionSources = 4;
    generation.maximumHarvestExtensionDepth = 3;
    generation.mandatoryReservations = viability.reservations;
    generation.seedPlans.push_back(incumbent.plan);
    udon::ColumnGenerationDiagnostics generationDiagnostics;
    const udon::RoutePortfolio portfolio = generator.generate(
        state,
        ledger,
        generation,
        &generationDiagnostics);

    udon::MasterOptions masterOptions;
    masterOptions.maximumCombinations = 40000;
    masterOptions.maximumCandidates = 32;
    masterOptions.diversityCandidates = 8;
    masterOptions.preferBaselineHarvestSources = true;
    masterOptions.mandatoryReservations = viability.reservations;
    udon::MasterDiagnostics masterDiagnostics;
    const std::vector<udon::MasterCandidate> candidates = master.solve(
        state,
        ledger,
        portfolio,
        masterOptions,
        masterDiagnostics);

    std::ostringstream widths;
    for (std::size_t agent = 0; agent < portfolio.columnsByAgent.size(); ++agent) {
        if (agent != 0U) {
            widths << '|';
        }
        widths << portfolio.columnsByAgent.at(agent).size();
    }
    std::cout << "exact_bundle_inspect,seed=" << fixture.seed
              << ",day=1"
              << ",score=" << score_text(exact->scoreAfterToday)
              << ",agents=" << agents_text(exact->simulation.finalAgents)
              << ",route_mask=" << portfolio_plan_mask(portfolio, plan)
              << ",columns=" << widths.str()
              << ",supported="
              << generationDiagnostics.exactOrienteeringSupportedAgents
              << ",complete="
              << generationDiagnostics.exactOrienteeringCompleteAgents
              << ",states="
              << generationDiagnostics.exactOrienteeringSettledStates
              << ",variants="
              << generationDiagnostics.exactOrienteeringTerminalVariants
              << ",bundles="
              << generationDiagnostics.exactOrienteeringBundles
              << ",deadline=" << generationDiagnostics.deadlineReached
              << ",master_exact=" << contains_candidate(candidates, *exact)
              << ",master_outcome=" << contains_outcome(candidates, *exact)
              << ",master_candidates=" << candidates.size()
              << ",master_nodes=" << masterDiagnostics.combinationsVisited
              << ",master_exact_bundles="
              << masterDiagnostics.exactBundlesEvaluated
              << '\n';
    for (std::size_t agent = 0; agent < plan.actions.size(); ++agent) {
        const std::vector<udon::RouteColumn>& columns =
            portfolio.columnsByAgent.at(agent);
        std::int32_t matches = 0;
        for (std::size_t columnIndex = 0;
             columnIndex < columns.size();
             ++columnIndex) {
            const udon::RouteColumn& column = columns.at(columnIndex);
            if (!same_agent_plan(column.actions, plan.actions.at(agent))) {
                continue;
            }
            ++matches;
            std::cout << "exact_bundle_route,seed=" << fixture.seed
                      << ",day=1"
                      << ",agent=" << agent
                      << ",rank=" << (columnIndex + 1U)
                      << ",priority=" << column.priority
                      << ",terminal=" << column.terminalCell
                      << ",fuel=" << column.terminalFuel
                      << ",brands=" << column.estimatedBrands
                      << ",servings=" << column.estimatedServings
                      << ",bundle=" << column.contingencyBundle
                      << ",exact=" << column.exactOrienteering
                      << ",footprint=";
            for (std::size_t road = 0;
                 road < fixture.config.roadCells.size();
                 ++road) {
                if (road != 0U) {
                    std::cout << '|';
                }
                const udon::CellId cell = fixture.config.roadCells.at(road);
                std::cout << cell << ':' << column.fullFootprint.at(cell);
            }
            std::cout << '\n';
        }
        if (matches == 0) {
            std::cout << "exact_bundle_route,seed=" << fixture.seed
                      << ",day=1,agent=" << agent
                      << ",matches=0\n";
        }
    }

    udon::ColumnGenerationOptions genericGeneration = generation;
    genericGeneration.maximumColumnsPerAgent = 64;
    genericGeneration.enableExactHarvestOrienteering = false;
    genericGeneration.enableFuelConstrainedExactHarvestOrienteering = false;
    const udon::RoutePortfolio genericWide = generator.generate(
        state,
        ledger,
        genericGeneration);
    const auto traffic_projection = [&fixture](
                                        const udon::RoutePortfolio& sourcePortfolio) {
        udon::RoutePortfolio output;
        output.columnsByAgent.resize(sourcePortfolio.columnsByAgent.size());
        for (std::size_t agent = 0;
             agent < sourcePortfolio.columnsByAgent.size();
             ++agent) {
            const std::vector<udon::RouteColumn>& source =
                sourcePortfolio.columnsByAgent.at(agent);
            std::vector<udon::RouteColumn>& projected =
                output.columnsByAgent.at(agent);
            const auto append = [&projected](const udon::RouteColumn& column) {
                if (std::none_of(
                        projected.begin(),
                        projected.end(),
                        [&column](const udon::RouteColumn& existing) {
                            return same_agent_plan(
                                existing.actions,
                                column.actions);
                        })) {
                    projected.push_back(column);
                }
            };
            const auto safeWait = std::find_if(
                source.begin(),
                source.end(),
                [](const udon::RouteColumn& column) {
                    return column.escortGroup < 0 &&
                        column.contingencyBundle < 0 &&
                        column.requiredRefuels.empty() &&
                        column.actions.size() == 1U &&
                        column.actions.front().kind == udon::ActionKind::Wait;
                });
            if (safeWait != source.end()) {
                append(*safeWait);
            }
            const auto rank = [&fixture](const udon::RouteColumn& column) {
                std::int32_t totalRoadStays = 0;
                std::int32_t maximumRoadStays = 0;
                for (const udon::CellId road : fixture.config.roadCells) {
                    const std::int32_t stays = column.fullFootprint.at(road);
                    totalRoadStays += stays;
                    maximumRoadStays = std::max(maximumRoadStays, stays);
                }
                return std::tuple{
                    totalRoadStays,
                    maximumRoadStays,
                    -column.terminalFuel,
                    -static_cast<std::int32_t>(
                        udon::brand_count(column.estimatedBrands)),
                    -column.estimatedServings,
                    column.columnId};
            };
            for (const udon::Spot& terminal : fixture.config.spots) {
                const udon::RouteColumn* best = nullptr;
                for (const udon::RouteColumn& column : source) {
                    if (column.terminalCell != terminal.position ||
                        column.estimatedServings <= 0 ||
                        column.estimatedServings > 3 ||
                        column.escortGroup >= 0 ||
                        column.contingencyBundle >= 0 ||
                        !column.requiredRefuels.empty() ||
                        !column.hasExactTimeline) {
                        continue;
                    }
                    if (best == nullptr || rank(column) < rank(*best)) {
                        best = &column;
                    }
                }
                if (best != nullptr) {
                    append(*best);
                }
            }
        }
        return output;
    };
    udon::ColumnGenerationOptions productionProjection = genericGeneration;
    productionProjection.maximumColumnsPerAgent = 16;
    const udon::RoutePortfolio productionProjected = generator.generate(
        state,
        ledger,
        productionProjection);
    const udon::RoutePortfolio productionSidecar =
        traffic_projection(productionProjected);
    udon::MasterDiagnostics productionSidecarDiagnostics;
    const std::vector<udon::MasterCandidate> productionSidecarCandidates =
        master.solve(
            state,
            ledger,
            productionSidecar,
            masterOptions,
            productionSidecarDiagnostics);
    std::ostringstream productionSidecarWidths;
    for (std::size_t agent = 0;
         agent < productionSidecar.columnsByAgent.size();
         ++agent) {
        if (agent != 0U) {
            productionSidecarWidths << '|';
        }
        productionSidecarWidths <<
            productionSidecar.columnsByAgent.at(agent).size();
    }
    std::cout << "production_sidecar,seed=" << fixture.seed
              << ",day=1"
              << ",route_mask="
              << portfolio_plan_mask(productionSidecar, plan)
              << ",columns=" << productionSidecarWidths.str()
              << ",exact="
              << contains_candidate(productionSidecarCandidates, *exact)
              << ",outcome="
              << contains_outcome(productionSidecarCandidates, *exact)
              << ",candidates=" << productionSidecarCandidates.size()
              << ",nodes="
              << productionSidecarDiagnostics.combinationsVisited
              << '\n';
    for (const std::int32_t structuralWidth :
         std::array<std::int32_t, 5>{16, 24, 32, 48, 64}) {
        udon::RoutePortfolio source;
        if (structuralWidth == 16) {
            source = productionProjected;
        } else if (structuralWidth == 64) {
            source = genericWide;
        } else {
            udon::ColumnGenerationOptions sweepGeneration = genericGeneration;
            sweepGeneration.maximumColumnsPerAgent = structuralWidth;
            source = generator.generate(state, ledger, sweepGeneration);
        }
        const udon::RoutePortfolio projected = traffic_projection(source);
        std::ostringstream widths;
        for (std::size_t agent = 0;
             agent < projected.columnsByAgent.size();
             ++agent) {
            if (agent != 0U) {
                widths << '|';
            }
            widths << projected.columnsByAgent.at(agent).size();
        }
        std::cout << "traffic_construction_sweep,seed=" << fixture.seed
                  << ",day=1,width=" << structuralWidth
                  << ",route_mask=" << portfolio_plan_mask(projected, plan)
                  << ",columns=" << widths.str()
                  << '\n';
    }
    const auto claimed_spots = [](const udon::RouteColumn& column) {
        std::uint32_t mask = 0U;
        for (const udon::ColumnVisitEvent& visit : column.firstVisits) {
            if (visit.claimedServing) {
                mask |= std::uint32_t{1} <<
                    static_cast<std::uint32_t>(visit.spot);
            }
        }
        return mask;
    };
    for (std::size_t agent = 0;
         agent < productionSidecar.columnsByAgent.size();
         ++agent) {
        for (const udon::RouteColumn& column :
             productionSidecar.columnsByAgent.at(agent)) {
            std::ostringstream actions;
            for (std::size_t action = 0;
                 action < column.actions.size();
                 ++action) {
                if (action != 0U) {
                    actions << '.';
                }
                actions << column.actions.at(action).wire_value();
            }
            std::int32_t totalRoadStays = 0;
            std::int32_t maximumRoadStays = 0;
            for (const udon::CellId road : fixture.config.roadCells) {
                const std::int32_t stays = column.fullFootprint.at(road);
                totalRoadStays += stays;
                maximumRoadStays = std::max(maximumRoadStays, stays);
            }
            std::cout << "production_sidecar_route,seed=" << fixture.seed
                      << ",day=1,agent=" << agent
                      << ",terminal=" << column.terminalCell
                      << ",fuel=" << column.terminalFuel
                      << ",spots=" << claimed_spots(column)
                      << ",brands=" << column.estimatedBrands
                      << ",servings=" << column.estimatedServings
                      << ",total_road_stays=" << totalRoadStays
                      << ",max_road_stays=" << maximumRoadStays
                      << ",column_id=" << column.columnId
                      << ",actions=" << actions.str()
                      << '\n';
        }
    }
    const auto resource_dominates = [
                                        &fixture,
                                        &claimed_spots](
                                        const udon::RouteColumn& challenger,
                                        const udon::RouteColumn& candidate) {
        if (challenger.terminalCell != candidate.terminalCell ||
            challenger.escortGroup >= 0 ||
            challenger.contingencyBundle >= 0 ||
            candidate.escortGroup >= 0 ||
            candidate.contingencyBundle >= 0) {
            return false;
        }
        const std::uint32_t challengerSpots = claimed_spots(challenger);
        const std::uint32_t candidateSpots = claimed_spots(candidate);
        if ((challengerSpots | candidateSpots) != challengerSpots ||
            challenger.terminalFuel < candidate.terminalFuel) {
            return false;
        }
        bool strict = challengerSpots != candidateSpots ||
            challenger.terminalFuel > candidate.terminalFuel;
        for (const udon::CellId road : fixture.config.roadCells) {
            const std::int32_t challengerStays =
                challenger.fullFootprint.at(road);
            const std::int32_t candidateStays =
                candidate.fullFootprint.at(road);
            if (challengerStays > candidateStays) {
                return false;
            }
            strict = strict || challengerStays < candidateStays;
        }
        return strict;
    };
    const auto traffic_rank = [&fixture](
                                      const udon::RouteColumn& candidate,
                                      std::size_t index) {
        std::int32_t totalStays = 0;
        std::int32_t maximumStays = 0;
        for (const udon::CellId road : fixture.config.roadCells) {
            const std::int32_t stays = candidate.fullFootprint.at(road);
            totalStays += stays;
            maximumStays = std::max(maximumStays, stays);
        }
        return std::tuple{
            totalStays,
            maximumStays,
            -candidate.terminalFuel,
            -static_cast<std::int32_t>(
                udon::brand_count(candidate.estimatedBrands)),
            -candidate.estimatedServings,
            index};
    };
    for (std::size_t agent = 0; agent < plan.actions.size(); ++agent) {
        const std::vector<udon::RouteColumn>& columns =
            genericWide.columnsByAgent.at(agent);
        std::int32_t projected = 0;
        std::int32_t maximumTerminalFrontier = 0;
        for (const udon::Spot& terminal : fixture.config.spots) {
            std::int32_t terminalFrontier = 0;
            for (std::size_t candidateIndex = 0;
                 candidateIndex < columns.size();
                 ++candidateIndex) {
                const udon::RouteColumn& candidate = columns.at(candidateIndex);
                if (candidate.terminalCell != terminal.position ||
                    claimed_spots(candidate) == 0U ||
                    candidate.escortGroup >= 0 ||
                    candidate.contingencyBundle >= 0) {
                    continue;
                }
                const bool dominated = std::any_of(
                    columns.begin(),
                    columns.end(),
                    [&candidate, &resource_dominates](
                        const udon::RouteColumn& challenger) {
                        return resource_dominates(challenger, candidate);
                    });
                if (!dominated) {
                    ++terminalFrontier;
                }
            }
            projected += terminalFrontier;
            maximumTerminalFrontier = std::max(
                maximumTerminalFrontier,
                terminalFrontier);
        }
        std::int32_t sameTerminalRank = 0;
        std::int32_t sameTerminalCount = 0;
        std::int32_t matches = 0;
        for (std::size_t columnIndex = 0;
             columnIndex < columns.size();
             ++columnIndex) {
            const udon::RouteColumn& column = columns.at(columnIndex);
            if (!same_agent_plan(column.actions, plan.actions.at(agent))) {
                continue;
            }
            ++matches;
            sameTerminalRank = 0;
            sameTerminalCount = 0;
            for (std::size_t otherIndex = 0;
                 otherIndex < columns.size();
                 ++otherIndex) {
                if (columns.at(otherIndex).terminalCell == column.terminalCell) {
                    ++sameTerminalCount;
                    if (otherIndex <= columnIndex) {
                        ++sameTerminalRank;
                    }
                }
            }
            const bool dominated = std::any_of(
                columns.begin(),
                columns.end(),
                [&column, &resource_dominates](
                    const udon::RouteColumn& challenger) {
                    return resource_dominates(challenger, column);
                });
            std::size_t trafficSelector = columns.size();
            std::int32_t trafficSelectorRank = 1;
            const auto oracleTrafficRank = traffic_rank(column, columnIndex);
            for (std::size_t otherIndex = 0;
                 otherIndex < columns.size();
                 ++otherIndex) {
                const udon::RouteColumn& other = columns.at(otherIndex);
                if (other.terminalCell != column.terminalCell ||
                    claimed_spots(other) == 0U ||
                    other.escortGroup >= 0 ||
                    other.contingencyBundle >= 0) {
                    continue;
                }
                const auto otherRank = traffic_rank(other, otherIndex);
                if (trafficSelector == columns.size() ||
                    otherRank < traffic_rank(
                        columns.at(trafficSelector),
                        trafficSelector)) {
                    trafficSelector = otherIndex;
                }
                if (otherRank < oracleTrafficRank) {
                    ++trafficSelectorRank;
                }
            }
            std::cout << "terminal_projection_route,seed=" << fixture.seed
                      << ",day=1"
                      << ",agent=" << agent
                      << ",global_rank=" << (columnIndex + 1U)
                      << ",same_terminal_rank=" << sameTerminalRank
                      << ",same_terminal_count=" << sameTerminalCount
                      << ",terminal=" << column.terminalCell
                      << ",fuel=" << column.terminalFuel
                      << ",spots=" << claimed_spots(column)
                      << ",brands=" << column.estimatedBrands
                      << ",servings=" << column.estimatedServings
                      << ",dominated=" << dominated
                      << ",traffic_selector_rank=" << trafficSelectorRank
                      << ",traffic_selected="
                      << (trafficSelector == columnIndex)
                      << ",column_id=" << column.columnId
                      << ",harvest_extension=" << column.harvestExtension
                      << ",exact_orienteering=" << column.exactOrienteering
                      << ",priority=" << column.priority
                      << ",escort=" << column.escortGroup
                      << ",bundle=" << column.contingencyBundle
                      << ",footprint=";
            for (std::size_t road = 0;
                 road < fixture.config.roadCells.size();
                 ++road) {
                if (road != 0U) {
                    std::cout << '|';
                }
                const udon::CellId cell = fixture.config.roadCells.at(road);
                std::cout << cell << ':' << column.fullFootprint.at(cell);
            }
            std::cout << '\n';
        }
        std::cout << "terminal_projection_summary,seed=" << fixture.seed
                  << ",day=1"
                  << ",agent=" << agent
                  << ",columns=" << columns.size()
                  << ",matches=" << matches
                  << ",projected=" << projected
                  << ",max_terminal_frontier=" << maximumTerminalFrontier
                  << '\n';
    }

    udon::RoutePortfolio projectedPortfolio;
    projectedPortfolio.columnsByAgent.resize(
        genericWide.columnsByAgent.size());
    std::int32_t nextProjectedId = 0;
    for (std::size_t agent = 0;
         agent < genericWide.columnsByAgent.size();
         ++agent) {
        const std::vector<udon::RouteColumn>& source =
            genericWide.columnsByAgent.at(agent);
        std::vector<udon::RouteColumn>& projected =
            projectedPortfolio.columnsByAgent.at(agent);
        const auto append = [&projected, &nextProjectedId](
                                const udon::RouteColumn& sourceColumn) {
            if (std::any_of(
                    projected.begin(),
                    projected.end(),
                    [&sourceColumn](const udon::RouteColumn& existing) {
                        return same_agent_plan(
                            existing.actions,
                            sourceColumn.actions);
                    })) {
                return;
            }
            udon::RouteColumn copy = sourceColumn;
            copy.columnId = nextProjectedId++;
            projected.push_back(std::move(copy));
        };
        const auto safeWait = std::find_if(
            source.begin(),
            source.end(),
            [](const udon::RouteColumn& column) {
                return column.escortGroup < 0 &&
                    column.contingencyBundle < 0 &&
                    column.actions.size() == 1U &&
                    column.actions.front().kind == udon::ActionKind::Wait;
            });
        if (safeWait != source.end()) {
            append(*safeWait);
        }
        for (const udon::Spot& terminal : fixture.config.spots) {
            std::size_t selected = source.size();
            for (std::size_t index = 0; index < source.size(); ++index) {
                const udon::RouteColumn& column = source.at(index);
                if (column.terminalCell != terminal.position ||
                    claimed_spots(column) == 0U ||
                    column.escortGroup >= 0 ||
                    column.contingencyBundle >= 0) {
                    continue;
                }
                if (selected == source.size() ||
                    traffic_rank(column, index) <
                        traffic_rank(source.at(selected), selected)) {
                    selected = index;
                }
            }
            if (selected != source.size()) {
                append(source.at(selected));
            }
        }
    }
    udon::MasterDiagnostics projectedDiagnostics;
    const std::vector<udon::MasterCandidate> projectedCandidates =
        master.solve(
            state,
            ledger,
            projectedPortfolio,
            masterOptions,
            projectedDiagnostics);
    const auto candidate_upper = [
                                     &fixture,
                                     &ledger,
                                     &state,
                                     &viabilityAnalyzer](
                                     const udon::MasterCandidate& candidate) {
        if (state.dayNumber >= fixture.config.day_count()) {
            return candidate.scoreAfterToday;
        }
        udon::MatchLedger futureLedger = ledger;
        futureLedger.apply(candidate.simulation.score);
        udon::DayState futureState;
        futureState.dayNumber = state.dayNumber + 1;
        futureState.agents = candidate.simulation.finalAgents;
        futureState.roadStatuses = state.roadStatuses;
        return viabilityAnalyzer.analyze(
            futureState,
            futureLedger).upperBound;
    };
    const udon::OfficialScore exactUpper = candidate_upper(*exact);
    udon::OfficialScore bestUpper;
    const udon::MasterCandidate* bestUpperCandidate = nullptr;
    std::int32_t exactUpperRank = 1;
    for (const udon::MasterCandidate& candidate : projectedCandidates) {
        const udon::OfficialScore upper = candidate_upper(candidate);
        if (exactUpper < upper) {
            ++exactUpperRank;
        }
        if (bestUpperCandidate == nullptr || bestUpper < upper ||
            (bestUpper == upper &&
             bestUpperCandidate->scoreAfterToday <
                 candidate.scoreAfterToday)) {
            bestUpper = upper;
            bestUpperCandidate = &candidate;
        }
    }
    std::ostringstream projectedWidths;
    for (std::size_t agent = 0;
         agent < projectedPortfolio.columnsByAgent.size();
         ++agent) {
        if (agent != 0U) {
            projectedWidths << '|';
        }
        projectedWidths <<
            projectedPortfolio.columnsByAgent.at(agent).size();
    }
    std::cout << "traffic_upper_lane,seed=" << fixture.seed
              << ",day=1"
              << ",route_mask="
              << portfolio_plan_mask(projectedPortfolio, plan)
              << ",columns=" << projectedWidths.str()
              << ",master_candidates=" << projectedCandidates.size()
              << ",master_nodes=" << projectedDiagnostics.combinationsVisited
              << ",exact=" << contains_candidate(projectedCandidates, *exact)
              << ",outcome=" << contains_outcome(projectedCandidates, *exact)
              << ",exact_upper=" << score_text(exactUpper)
              << ",exact_upper_rank=" << exactUpperRank
              << ",best_upper="
              << (bestUpperCandidate == nullptr
                      ? std::string{"none"}
                      : score_text(bestUpper))
              << ",best_current="
              << (bestUpperCandidate == nullptr
                      ? std::string{"none"}
                      : score_text(
                            bestUpperCandidate->scoreAfterToday))
              << ",best_agents="
               << (bestUpperCandidate == nullptr
                       ? std::string{"none"}
                       : agents_text(
                             bestUpperCandidate->simulation.finalAgents))
               << '\n';

    udon::RoutePortfolio endpointPortfolio;
    endpointPortfolio.columnsByAgent.resize(
        genericWide.columnsByAgent.size());
    std::int32_t nextEndpointId = 0;
    for (std::size_t agent = 0;
         agent < genericWide.columnsByAgent.size();
         ++agent) {
        const std::vector<udon::RouteColumn>& source =
            genericWide.columnsByAgent.at(agent);
        std::vector<udon::RouteColumn>& retained =
            endpointPortfolio.columnsByAgent.at(agent);
        const auto append = [&retained, &nextEndpointId](
                                const udon::RouteColumn& sourceColumn) {
            if (std::any_of(
                    retained.begin(),
                    retained.end(),
                    [&sourceColumn](const udon::RouteColumn& existing) {
                        return same_agent_plan(
                            existing.actions,
                            sourceColumn.actions);
                    })) {
                return;
            }
            udon::RouteColumn copy = sourceColumn;
            copy.columnId = nextEndpointId++;
            retained.push_back(std::move(copy));
        };
        for (const udon::RouteColumn& column : source) {
            if (column.contingencyBundle >= 0) {
                append(column);
            }
        }
        const auto safeWait = std::find_if(
            source.begin(),
            source.end(),
            [](const udon::RouteColumn& column) {
                return column.escortGroup < 0 &&
                    column.contingencyBundle < 0 &&
                    column.requiredRefuels.empty() &&
                    column.actions.size() == 1U &&
                    column.actions.front().kind == udon::ActionKind::Wait;
            });
        if (safeWait != source.end()) {
            append(*safeWait);
        }
        const auto current_rank = [&ledger](
                                      const udon::RouteColumn& column,
                                      std::size_t index) {
            return std::tuple{
                udon::brand_difference_count(
                    column.estimatedBrands,
                    ledger.lifetimeBrands),
                udon::brand_count(column.estimatedBrands),
                column.estimatedServings,
                column.terminalFuel,
                column.priority,
                -static_cast<std::int32_t>(index)};
        };
        const auto fuel_rank = [&current_rank](
                                   const udon::RouteColumn& column,
                                   std::size_t index) {
            const auto current = current_rank(column, index);
            return std::tuple{
                column.terminalFuel,
                std::get<0>(current),
                std::get<1>(current),
                std::get<2>(current),
                std::get<4>(current),
                std::get<5>(current)};
        };
        for (const udon::Spot& terminal : fixture.config.spots) {
            std::size_t bestCurrent = source.size();
            std::size_t bestFuel = source.size();
            for (std::size_t index = 0; index < source.size(); ++index) {
                const udon::RouteColumn& column = source.at(index);
                if (column.terminalCell != terminal.position ||
                    claimed_spots(column) == 0U ||
                    column.escortGroup >= 0 ||
                    column.contingencyBundle >= 0 ||
                    !column.requiredRefuels.empty() ||
                    !column.hasExactTimeline) {
                    continue;
                }
                if (bestCurrent == source.size() ||
                    current_rank(source.at(bestCurrent), bestCurrent) <
                        current_rank(column, index)) {
                    bestCurrent = index;
                }
                if (bestFuel == source.size() ||
                    fuel_rank(source.at(bestFuel), bestFuel) <
                        fuel_rank(column, index)) {
                    bestFuel = index;
                }
            }
            if (bestCurrent != source.size()) {
                append(source.at(bestCurrent));
            }
            if (bestFuel != source.size()) {
                append(source.at(bestFuel));
            }
        }
    }
    udon::MasterDiagnostics endpointDiagnostics;
    const std::vector<udon::MasterCandidate> endpointCandidates = master.solve(
        state,
        ledger,
        endpointPortfolio,
        masterOptions,
        endpointDiagnostics);
    std::ostringstream endpointWidths;
    for (std::size_t agent = 0;
         agent < endpointPortfolio.columnsByAgent.size();
         ++agent) {
        if (agent != 0U) {
            endpointWidths << '|';
        }
        endpointWidths << endpointPortfolio.columnsByAgent.at(agent).size();
    }
    for (std::size_t agent = 0; agent < plan.actions.size(); ++agent) {
        const udon::AgentState& exactTerminal =
            exact->simulation.finalAgents.at(agent);
        std::uint32_t exactClaimed = 0U;
        for (const udon::RouteColumn& column :
             genericWide.columnsByAgent.at(agent)) {
            if (same_agent_plan(column.actions, plan.actions.at(agent))) {
                exactClaimed = claimed_spots(column);
                break;
            }
        }
        std::int32_t stateClassMatches = 0;
        for (const udon::RouteColumn& column :
             endpointPortfolio.columnsByAgent.at(agent)) {
            stateClassMatches +=
                column.terminalCell == exactTerminal.position &&
                    column.terminalFuel == exactTerminal.fuel &&
                    claimed_spots(column) == exactClaimed
                ? 1
                : 0;
        }
        std::cout << "terminal_endpoint_class,seed=" << fixture.seed
                  << ",day=1,agent=" << agent
                  << ",terminal=" << exactTerminal.position
                  << ",fuel=" << exactTerminal.fuel
                  << ",spots=" << exactClaimed
                  << ",matches=" << stateClassMatches
                  << '\n';
    }
    std::cout << "terminal_endpoint_union,seed=" << fixture.seed
              << ",day=1,route_mask="
              << portfolio_plan_mask(endpointPortfolio, plan)
              << ",columns=" << endpointWidths.str()
              << ",master_candidates=" << endpointCandidates.size()
              << ",master_nodes=" << endpointDiagnostics.combinationsVisited
              << ",exact=" << contains_candidate(endpointCandidates, *exact)
              << ",outcome=" << contains_outcome(endpointCandidates, *exact)
              << '\n';

    udon::RoutePortfolio additivePortfolio = productionProjected;
    std::int32_t nextAdditiveId = 0;
    for (const std::vector<udon::RouteColumn>& columns :
         additivePortfolio.columnsByAgent) {
        for (const udon::RouteColumn& column : columns) {
            nextAdditiveId = std::max(nextAdditiveId, column.columnId + 1);
        }
    }
    for (std::size_t agent = 0;
         agent < endpointPortfolio.columnsByAgent.size();
         ++agent) {
        std::vector<udon::RouteColumn>& retained =
            additivePortfolio.columnsByAgent.at(agent);
        for (const udon::RouteColumn& endpoint :
             endpointPortfolio.columnsByAgent.at(agent)) {
            if (std::any_of(
                    retained.begin(),
                    retained.end(),
                    [&endpoint](const udon::RouteColumn& existing) {
                        return same_agent_plan(
                            existing.actions,
                            endpoint.actions);
                    })) {
                continue;
            }
            udon::RouteColumn copy = endpoint;
            copy.columnId = nextAdditiveId++;
            retained.push_back(std::move(copy));
        }
    }
    udon::MasterDiagnostics additiveDiagnostics;
    const std::vector<udon::MasterCandidate> additiveCandidates = master.solve(
        state,
        ledger,
        additivePortfolio,
        masterOptions,
        additiveDiagnostics);
    std::ostringstream additiveWidths;
    for (std::size_t agent = 0;
         agent < additivePortfolio.columnsByAgent.size();
         ++agent) {
        if (agent != 0U) {
            additiveWidths << '|';
        }
        additiveWidths << additivePortfolio.columnsByAgent.at(agent).size();
    }
    std::cout << "terminal_endpoint_additive,seed=" << fixture.seed
              << ",day=1,route_mask="
              << portfolio_plan_mask(additivePortfolio, plan)
              << ",columns=" << additiveWidths.str()
              << ",master_candidates=" << additiveCandidates.size()
              << ",master_nodes=" << additiveDiagnostics.combinationsVisited
              << ",exact=" << contains_candidate(additiveCandidates, *exact)
              << ",outcome=" << contains_outcome(additiveCandidates, *exact)
              << '\n';
}

[[nodiscard]] std::string best_candidate_score(
    const std::vector<udon::MasterCandidate>& candidates) {
    if (candidates.empty()) {
        return "none";
    }
    const udon::MasterCandidate* best = &candidates.front();
    for (const udon::MasterCandidate& candidate : candidates) {
        if (best->scoreAfterToday < candidate.scoreAfterToday) {
            best = &candidate;
        }
    }
    return score_text(best->scoreAfterToday);
}

[[nodiscard]] std::string slack_text(const udon::TerminalSlack& slack) {
    return std::to_string(slack.worstRemainingBrandSteps) + '/' +
        std::to_string(slack.totalRemainingBrandSteps) + '/' +
        std::to_string(slack.patrolFuelReserve) + '/' +
        std::to_string(slack.overnightSpotCount);
}

[[nodiscard]] bool profile_certified(const udon::CandidateProfile& profile) {
    return !profile.outcomes.empty() && std::all_of(
        profile.outcomes.begin(),
        profile.outcomes.end(),
        [](const udon::ScenarioOutcome& outcome) {
            return outcome.witness.certified;
        });
}

[[nodiscard]] std::string profile_support_text(
    const udon::CandidateProfile& profile) {
    std::ostringstream output;
    for (std::size_t index = 0; index < profile.outcomes.size(); ++index) {
        if (index != 0U) {
            output << '|';
        }
        output << score_text(profile.outcomes.at(index).score) << '@'
               << score_text(profile.scenarioValidUpperBounds.at(index)) << ':'
               << (profile.outcomes.at(index).witness.certified ? 1 : 0);
    }
    return output.str();
}

[[nodiscard]] std::string dominance_failure_text(
    const udon::CandidateProfile& challenger,
    const udon::CandidateProfile& incumbent) {
    if (!profile_certified(challenger) || !incumbent.hasValidUpperBound ||
        challenger.outcomes.size() != incumbent.outcomes.size() ||
        challenger.scenarioWeights != incumbent.scenarioWeights ||
        incumbent.scenarioValidUpperBounds.size() != incumbent.outcomes.size()) {
        return "profile-precondition";
    }
    std::vector<udon::OfficialScore> support;
    for (const udon::ScenarioOutcome& outcome : challenger.outcomes) {
        support.push_back(outcome.score);
    }
    support.insert(
        support.end(),
        incumbent.scenarioValidUpperBounds.begin(),
        incumbent.scenarioValidUpperBounds.end());
    std::sort(support.begin(), support.end());
    support.erase(std::unique(support.begin(), support.end()), support.end());
    bool strict = false;
    for (const udon::OfficialScore& threshold : support) {
        std::uint64_t challengerWeight = 0;
        std::uint64_t incumbentPossibleWeight = 0;
        for (std::size_t scenario = 0; scenario < challenger.outcomes.size(); ++scenario) {
            if (!(challenger.outcomes.at(scenario).score < threshold)) {
                challengerWeight += challenger.scenarioWeights.at(scenario);
            }
            if (!(incumbent.scenarioValidUpperBounds.at(scenario) < threshold)) {
                incumbentPossibleWeight += incumbent.scenarioWeights.at(scenario);
            }
        }
        if (challengerWeight < incumbentPossibleWeight) {
            return score_text(threshold) + ':' +
                std::to_string(challengerWeight) + '<' +
                std::to_string(incumbentPossibleWeight);
        }
        strict = strict || challengerWeight > incumbentPossibleWeight;
    }
    return strict ? "none" : "no-strict-threshold";
}

[[nodiscard]] const udon::MasterCandidate* best_same_prefix_slack(
    const std::vector<udon::MasterCandidate>& candidates,
    const udon::MasterCandidate& exact) {
    const udon::MasterCandidate* best = nullptr;
    for (const udon::MasterCandidate& candidate : candidates) {
        if (candidate.scoreAfterToday.lifetimeDistinct !=
                exact.scoreAfterToday.lifetimeDistinct ||
            candidate.scoreAfterToday.totalDailyDistinct !=
                exact.scoreAfterToday.totalDailyDistinct) {
            continue;
        }
        if (best == nullptr ||
            udon::compare_terminal_slack(
                candidate.terminalSlack,
                best->terminalSlack) > 0) {
            best = &candidate;
        }
    }
    return best;
}

void attribute_oracle_day(
    const Fixture& fixture,
    std::int32_t day,
    const udon::DayState& state,
    const udon::MatchLedger& ledger,
    const udon::DayPlan& oraclePlan,
    const udon::DayPlan& parentPlan) {
    const udon::ExactStepSimulator simulator(fixture.config);
    const udon::IndependentDayValidator validator(fixture.config);
    const udon::ParetoRouter router(fixture.config);
    const udon::RouteColumnGenerator generator(fixture.config, router);
    const udon::RouteMaster master(fixture.config, simulator, validator);
    const udon::GreedyPlanner greedy(fixture.config, generator, master);
    const udon::FastViabilityAnalyzer viabilityAnalyzer(fixture.config);

    const std::optional<udon::MasterCandidate> exact = master.evaluate_exact_plan(
        state,
        ledger,
        oraclePlan);
    if (!exact.has_value()) {
        std::cout << "attribute,seed=" << fixture.seed
                  << ",day=" << day
                  << ",status=oracle-plan-not-dual-valid\n";
        return;
    }
    const std::optional<udon::MasterCandidate> parent =
        master.evaluate_exact_plan(state, ledger, parentPlan);

    udon::ColumnGenerationOptions seedGeneration;
    seedGeneration.maximumPathsPerTarget = 1;
    seedGeneration.maximumColumnsPerAgent = 2;
    seedGeneration.maximumTargetSpots = 4;
    seedGeneration.maximumEscorts = 2;
    udon::MasterOptions seedMaster;
    seedMaster.maximumCombinations = 512;
    seedMaster.maximumCandidates = 1;
    udon::MasterDiagnostics seedDiagnostics;
    const udon::MasterCandidate incumbent = greedy.build_incumbent(
        state,
        ledger,
        seedGeneration,
        seedMaster,
        seedDiagnostics);

    const udon::ViabilityBounds viability = viabilityAnalyzer.analyze(state, ledger);
    udon::ColumnGenerationOptions generation;
    generation.maximumPathsPerTarget = 4;
    generation.maximumColumnsPerAgent = 16;
    generation.maximumTargetSpots = 12;
    generation.maximumEscorts = 16;
    generation.maximumSeedPlans = 2;
    generation.enableHarvestExtensions = true;
    generation.allowUncachedHarvestTargets = true;
    const bool exactFuelOrienteering =
        static_cast<std::int64_t>(fixture.config.fuelLimit) >=
        2LL * fixture.config.steps_for_day(day);
    const bool hasFuelConstrainedPatrol = std::any_of(
        state.agents.begin(),
        state.agents.end(),
        [&fixture, day](const udon::AgentState& agent) {
            return agent.kind == udon::AgentKind::Patrol &&
                static_cast<std::int64_t>(agent.fuel) <
                    2LL * fixture.config.steps_for_day(day);
        });
    generation.enableExactHarvestOrienteering =
        (exactFuelOrienteering || day == fixture.config.day_count());
    generation.enableFuelConstrainedExactHarvestOrienteering =
        hasFuelConstrainedPatrol && day == fixture.config.day_count();
    generation.enableAnytimeFuelConstrainedHarvestOrienteering =
        generation.enableFuelConstrainedExactHarvestOrienteering;
    generation.maximumHarvestExtensionSources = 4;
    generation.maximumHarvestExtensionDepth = 3;
    generation.mandatoryReservations = viability.reservations;
    generation.seedPlans.push_back(incumbent.plan);

    udon::ColumnGenerationOptions legacyGeneration = generation;
    legacyGeneration.maximumColumnsPerAgent = 12;
    legacyGeneration.allowUncachedHarvestTargets = false;
    udon::RoutePortfolio merged = generator.generate(
        state,
        ledger,
        legacyGeneration);
    const udon::RoutePortfolio legacy = merged;
    const udon::RoutePortfolio expanded = generator.generate(
        state,
        ledger,
        generation);

    std::int32_t nextColumnId = 0;
    for (const std::vector<udon::RouteColumn>& columns : merged.columnsByAgent) {
        for (const udon::RouteColumn& column : columns) {
            nextColumnId = std::max(nextColumnId, column.columnId + 1);
        }
    }
    for (std::size_t agent = 0U; agent < merged.columnsByAgent.size(); ++agent) {
        std::vector<udon::RouteColumn>& retained = merged.columnsByAgent.at(agent);
        for (const udon::RouteColumn& source : expanded.columnsByAgent.at(agent)) {
            if (!source.harvestExtension ||
                std::any_of(
                    retained.begin(),
                    retained.end(),
                    [&source](const udon::RouteColumn& existing) {
                        return same_agent_plan(existing.actions, source.actions);
                    })) {
                continue;
            }
            udon::RouteColumn added = source;
            added.columnId = nextColumnId++;
            retained.push_back(std::move(added));
        }
    }

    udon::ColumnGenerationOptions wideGeneration = generation;
    wideGeneration.maximumColumnsPerAgent = 32;
    const udon::RoutePortfolio wide32 = generator.generate(
        state,
        ledger,
        wideGeneration);
    wideGeneration.maximumColumnsPerAgent = 64;
    const udon::RoutePortfolio wide64 = generator.generate(
        state,
        ledger,
        wideGeneration);

    udon::MasterOptions masterOptions;
    masterOptions.maximumCombinations = 40000;
    masterOptions.maximumCandidates = 32;
    masterOptions.diversityCandidates = 8;
    masterOptions.preferBaselineHarvestSources = true;
    masterOptions.mandatoryReservations = viability.reservations;
    udon::MasterDiagnostics mergedDiagnostics;
    const std::vector<udon::MasterCandidate> mergedCandidates = master.solve(
        state,
        ledger,
        merged,
        masterOptions,
        mergedDiagnostics);

    {
        udon::ColumnGenerationOptions completeCurrent = generation;
        completeCurrent.enableExactHarvestOrienteering = true;
        completeCurrent.enableFuelConstrainedExactHarvestOrienteering = true;
        completeCurrent.enableAnytimeFuelConstrainedHarvestOrienteering = false;
        udon::ColumnGenerationDiagnostics completeCurrentDiagnostics;
        const udon::RoutePortfolio completeCurrentPortfolio = generator.generate(
            state,
            ledger,
            completeCurrent,
            &completeCurrentDiagnostics);
        udon::MasterDiagnostics completeCurrentMasterDiagnostics;
        const std::vector<udon::MasterCandidate> completeCurrentCandidates =
            master.solve(
                state,
                ledger,
                completeCurrentPortfolio,
                masterOptions,
                completeCurrentMasterDiagnostics);
        std::ostringstream completeCurrentWidths;
        for (std::size_t agent = 0;
             agent < completeCurrentPortfolio.columnsByAgent.size();
             ++agent) {
            if (agent != 0U) {
                completeCurrentWidths << '|';
            }
            completeCurrentWidths <<
                completeCurrentPortfolio.columnsByAgent.at(agent).size();
        }
        std::cout << "current_exact_fuel_attribute,seed=" << fixture.seed
                  << ",day=" << day
                  << ",oracle_mask="
                  << portfolio_plan_mask(completeCurrentPortfolio, oraclePlan)
                  << ",columns=" << completeCurrentWidths.str()
                  << ",supported="
                  << completeCurrentDiagnostics.exactOrienteeringSupportedAgents
                  << ",complete="
                  << completeCurrentDiagnostics.exactOrienteeringCompleteAgents
                  << ",states="
                  << completeCurrentDiagnostics.exactOrienteeringSettledStates
                  << ",variants="
                  << completeCurrentDiagnostics.exactOrienteeringTerminalVariants
                  << ",bundles="
                  << completeCurrentDiagnostics.exactOrienteeringBundles
                  << ",nodes="
                  << completeCurrentMasterDiagnostics.combinationsVisited
                  << ",master_exact="
                  << contains_candidate(completeCurrentCandidates, *exact)
                  << ",master_outcome="
                  << contains_outcome(completeCurrentCandidates, *exact)
                  << '\n';

        udon::ColumnGenerationOptions frontierCurrent = completeCurrent;
        frontierCurrent.maximumCoordinatedExactBundles = 4;
        udon::ColumnGenerationDiagnostics frontierCurrentDiagnostics;
        const udon::RoutePortfolio frontierCurrentPortfolio = generator.generate(
            state,
            ledger,
            frontierCurrent,
            &frontierCurrentDiagnostics);
        udon::MasterDiagnostics frontierCurrentMasterDiagnostics;
        const std::vector<udon::MasterCandidate> frontierCurrentCandidates =
            master.solve(
                state,
                ledger,
                frontierCurrentPortfolio,
                masterOptions,
                frontierCurrentMasterDiagnostics);
        std::cout << "exact_bundle_frontier_attribute,seed=" << fixture.seed
                  << ",day=" << day
                  << ",off_frontier_candidates="
                  << completeCurrentDiagnostics.exactOrienteeringFrontierCandidates
                  << ",off_frontier_bundles="
                  << completeCurrentDiagnostics.exactOrienteeringFrontierBundles
                  << ",on_frontier_candidates="
                  << frontierCurrentDiagnostics.exactOrienteeringFrontierCandidates
                  << ",on_frontier_bundles="
                  << frontierCurrentDiagnostics.exactOrienteeringFrontierBundles
                  << ",off_bundles="
                  << completeCurrentDiagnostics.exactOrienteeringBundles
                  << ",on_bundles="
                  << frontierCurrentDiagnostics.exactOrienteeringBundles
                  << ",oracle_mask="
                  << portfolio_plan_mask(frontierCurrentPortfolio, oraclePlan)
                  << ",master_exact="
                  << contains_candidate(frontierCurrentCandidates, *exact)
                  << ",master_outcome="
                  << contains_outcome(frontierCurrentCandidates, *exact)
                  << ",best=" << best_candidate_score(frontierCurrentCandidates)
                  << ",nodes="
                  << frontierCurrentMasterDiagnostics.combinationsVisited
                  << '\n';
    }

    udon::RoutePortfolio forcedBundle = merged;
    bool completeForcedBundle = true;
    constexpr std::int32_t forcedBundleId = 1000000000;
    for (std::size_t agent = 0U; agent < exact->plan.actions.size(); ++agent) {
        const auto found = std::find_if(
            forcedBundle.columnsByAgent.at(agent).begin(),
            forcedBundle.columnsByAgent.at(agent).end(),
            [&exact, agent](const udon::RouteColumn& column) {
                return same_agent_plan(
                    column.actions,
                    exact->plan.actions.at(agent));
            });
        if (found == forcedBundle.columnsByAgent.at(agent).end()) {
            completeForcedBundle = false;
            break;
        }
        found->contingencyBundle = forcedBundleId;
    }
    std::vector<udon::MasterCandidate> forcedCandidates;
    if (completeForcedBundle) {
        udon::MasterDiagnostics forcedDiagnostics;
        forcedCandidates = master.solve(
            state,
            ledger,
            forcedBundle,
            masterOptions,
            forcedDiagnostics);
    }

    udon::MasterOptions wideMasterOptions = masterOptions;
    wideMasterOptions.maximumCandidates = 256;
    wideMasterOptions.diversityCandidates = 64;
    wideMasterOptions.enableLexicographicBranchAndBound = false;
    udon::MasterDiagnostics wideMasterDiagnostics;
    const std::vector<udon::MasterCandidate> wideMasterCandidates = master.solve(
        state,
        ledger,
        merged,
        wideMasterOptions,
        wideMasterDiagnostics);

    const std::int32_t augmentationCap = static_cast<std::int32_t>(
        std::max({
            merged.columnsByAgent.at(0).size(),
            merged.columnsByAgent.at(1).size(),
            merged.columnsByAgent.at(2).size(),
        }) + 4U);
    const udon::RoutePoolAugmentation augmented = generator.augment_with_candidate_routes(
        state,
        merged,
        std::vector<udon::MasterCandidate>{*exact},
        augmentationCap);
    udon::MasterDiagnostics augmentedDiagnostics;
    const std::vector<udon::MasterCandidate> augmentedCandidates = master.solve(
        state,
        ledger,
        augmented.portfolio,
        masterOptions,
        augmentedDiagnostics);
    udon::MasterDiagnostics wideAugmentedDiagnostics;
    const std::vector<udon::MasterCandidate> wideAugmentedCandidates = master.solve(
        state,
        ledger,
        augmented.portfolio,
        wideMasterOptions,
        wideAugmentedDiagnostics);
    const auto candidateUpper = [
                                    &fixture,
                                    day,
                                    &ledger,
                                    &state,
                                    &viabilityAnalyzer](
                                    const udon::MasterCandidate& candidate) {
        if (day == fixture.config.day_count()) {
            return candidate.scoreAfterToday;
        }
        udon::MatchLedger futureLedger = ledger;
        futureLedger.apply(candidate.simulation.score);
        udon::DayState futureState;
        futureState.dayNumber = day + 1;
        futureState.agents = candidate.simulation.finalAgents;
        futureState.roadStatuses = state.roadStatuses;
        return viabilityAnalyzer.analyze(futureState, futureLedger).upperBound;
    };
    const udon::OfficialScore exactUpper = candidateUpper(*exact);
    if (day == 1) {
        std::vector<udon::MasterCandidate> f0Source;
        f0Source.reserve(mergedCandidates.size() + 1U);
        f0Source.push_back(incumbent);
        std::set<std::string> sourceIds{incumbent.stableId};
        for (const udon::MasterCandidate& candidate : mergedCandidates) {
            if (sourceIds.insert(candidate.stableId).second) {
                f0Source.push_back(candidate);
            }
        }
        const auto better = [](const udon::MasterCandidate& left,
                               const udon::MasterCandidate& right) {
            const std::int32_t scoreOrder = udon::compare_lexicographic(
                left.scoreAfterToday,
                right.scoreAfterToday);
            if (scoreOrder != 0) {
                return scoreOrder > 0;
            }
            const std::int32_t slackOrder = udon::compare_terminal_slack(
                left.terminalSlack,
                right.terminalSlack);
            if (slackOrder != 0) {
                return slackOrder > 0;
            }
            return left.stableId < right.stableId;
        };
        std::sort(f0Source.begin(), f0Source.end(), better);
        const auto plan_distance = [](
                                       const udon::MasterCandidate& left,
                                       const udon::MasterCandidate& right) {
            const std::size_t agentCount = std::max(
                left.plan.actions.size(),
                right.plan.actions.size());
            std::int32_t distance = 0;
            for (std::size_t agent = 0; agent < agentCount; ++agent) {
                if (agent >= left.plan.actions.size() ||
                    agent >= right.plan.actions.size()) {
                    const udon::AgentPlan& existing =
                        agent < left.plan.actions.size()
                        ? left.plan.actions.at(agent)
                        : right.plan.actions.at(agent);
                    distance += std::max(
                        1,
                        static_cast<std::int32_t>(existing.size()));
                    continue;
                }
                const udon::AgentPlan& leftActions =
                    left.plan.actions.at(agent);
                const udon::AgentPlan& rightActions =
                    right.plan.actions.at(agent);
                const std::size_t actionCount = std::max(
                    leftActions.size(),
                    rightActions.size());
                for (std::size_t action = 0;
                     action < actionCount;
                     ++action) {
                    if (action >= leftActions.size() ||
                        action >= rightActions.size() ||
                        leftActions.at(action).wire_value() !=
                            rightActions.at(action).wire_value()) {
                        ++distance;
                    }
                }
            }
            return distance;
        };
        constexpr std::int32_t f0Limit = 16;
        constexpr std::int32_t diversitySlots = f0Limit / 4;
        constexpr std::int32_t qualityTarget =
            f0Limit - diversitySlots;
        std::set<std::string> qualityIds{incumbent.stableId};
        for (const udon::MasterCandidate& candidate : f0Source) {
            if (static_cast<std::int32_t>(qualityIds.size()) >=
                qualityTarget) {
                break;
            }
            qualityIds.insert(candidate.stableId);
        }
        std::map<std::string, udon::OfficialScore> uppers;
        for (const udon::MasterCandidate& candidate : f0Source) {
            uppers.emplace(candidate.stableId, candidateUpper(candidate));
        }
        const auto select_f0 = [
                                   &f0Source,
                                   &qualityIds,
                                   &uppers,
                                   &plan_distance,
                                   &better](bool upperAware) {
            std::set<std::string> selected = qualityIds;
            while (static_cast<std::int32_t>(selected.size()) < f0Limit) {
                const udon::MasterCandidate* best = nullptr;
                udon::OfficialScore bestUpper;
                std::int32_t bestMinimumDistance = -1;
                for (const udon::MasterCandidate& candidate : f0Source) {
                    if (selected.contains(candidate.stableId)) {
                        continue;
                    }
                    std::int32_t minimumDistance =
                        std::numeric_limits<std::int32_t>::max();
                    for (const udon::MasterCandidate& retained : f0Source) {
                        if (selected.contains(retained.stableId)) {
                            minimumDistance = std::min(
                                minimumDistance,
                                plan_distance(candidate, retained));
                        }
                    }
                    const std::int32_t upperOrder =
                        !upperAware || best == nullptr
                        ? 0
                        : udon::compare_lexicographic(
                            uppers.at(candidate.stableId),
                            bestUpper);
                    if ((upperAware && best == nullptr) ||
                        upperOrder > 0 ||
                        (upperOrder == 0 &&
                         minimumDistance > bestMinimumDistance) ||
                        (upperOrder == 0 &&
                         minimumDistance == bestMinimumDistance &&
                         (best == nullptr || better(candidate, *best)))) {
                        best = &candidate;
                        bestMinimumDistance = minimumDistance;
                        if (upperAware) {
                            bestUpper = uppers.at(candidate.stableId);
                        }
                    }
                }
                if (best == nullptr) {
                    break;
                }
                selected.insert(best->stableId);
            }
            return selected;
        };
        const std::set<std::string> currentF0 = select_f0(false);
        const std::set<std::string> upperF0 = select_f0(true);
        const std::int32_t qualityEvictions = static_cast<std::int32_t>(
            std::count_if(
                qualityIds.begin(),
                qualityIds.end(),
                [&upperF0](const std::string& id) {
                    return !upperF0.contains(id);
                }));
        std::int32_t changedDiversity = 0;
        for (const std::string& id : currentF0) {
            if (!qualityIds.contains(id) && !upperF0.contains(id)) {
                ++changedDiversity;
            }
        }
        std::cout << "f0_upper_attribute,seed=" << fixture.seed
                  << ",day=1"
                  << ",source_candidates=" << f0Source.size()
                  << ",quality=" << qualityIds.size()
                  << ",current_size=" << currentF0.size()
                  << ",upper_size=" << upperF0.size()
                  << ",oracle_quality="
                  << qualityIds.contains(exact->stableId)
                  << ",oracle_current="
                  << currentF0.contains(exact->stableId)
                  << ",oracle_upper="
                  << upperF0.contains(exact->stableId)
                  << ",quality_evictions=" << qualityEvictions
                  << ",changed_diversity=" << changedDiversity
                  << ",oracle_upper_score=" << score_text(exactUpper)
                  << '\n';
    }
    if (parent.has_value()) {
        udon::TrafficBelief belief(fixture.config);
        belief.observe(state);
        const udon::ScenarioGenerator scenarios(fixture.config);
        udon::ScenarioManifest manifest = scenarios.freeze_manifest(state, belief);
        const udon::RiskPolicy policy;
        manifest.generatorSeed = 0;
        manifest.evaluatorHash = evaluator_contract_hash();
        manifest.riskPolicyVersion = policy.version;
        manifest.confidenceBasisPoints = policy.confidenceBasisPoints;
        manifest.safetySlack = policy.safetySlack;
        manifest.resolutionBasisPoints = policy.resolutionBasisPoints;
        manifest.quantileBasisPoints = policy.quantileBasisPoints;
        const udon::FutureWitnessRepairer repairer(
            fixture.config,
            generator,
            master,
            simulator,
            validator,
            kFutureHarvestMode);
        const auto build_profile = [&](const udon::MasterCandidate& candidate) {
            const std::chrono::steady_clock::time_point deadline =
                std::chrono::steady_clock::now() + kProductionBudget;
            udon::CandidateProfile profile = repairer.provisional_profile(
                candidate,
                state,
                ledger,
                belief,
                manifest,
                candidateUpper(candidate),
                24,
                deadline);
            repairer.repair_profile(
                profile,
                candidate,
                state,
                ledger,
                belief,
                manifest,
                200,
                deadline);
            std::vector<udon::CandidateEvaluation> evaluations;
            evaluations.push_back(udon::CandidateEvaluation{candidate, std::move(profile)});
            udon::LexicographicRiskComparator comparator(policy);
            comparator.finalize_profiles(evaluations, manifest);
            return std::move(evaluations.front().profile);
        };
        const udon::CandidateProfile exactProfile = build_profile(*exact);
        const udon::CandidateProfile parentProfile = build_profile(*parent);
        const udon::LexicographicRiskComparator comparator(policy);
        std::cout << "profile_attribute,seed=" << fixture.seed
                  << ",day=" << day
                  << ",exact_current=" << score_text(exact->scoreAfterToday)
                  << ",parent_current=" << score_text(parent->scoreAfterToday)
                  << ",exact_lower=" << score_text(exactProfile.certifiedLowerBound)
                  << ",parent_lower=" << score_text(parentProfile.certifiedLowerBound)
                  << ",exact_upper=" << score_text(exactProfile.validUpperBound)
                  << ",parent_upper=" << score_text(parentProfile.validUpperBound)
                  << ",exact_certified=" << profile_certified(exactProfile)
                  << ",parent_certified=" << profile_certified(parentProfile)
                  << ",exact_dominates_parent="
                  << comparator.certified_dominates(exactProfile, parentProfile)
                  << ",parent_dominates_exact="
                  << comparator.certified_dominates(parentProfile, exactProfile)
                  << ",exact_failure="
                  << dominance_failure_text(exactProfile, parentProfile)
                  << ",parent_failure="
                  << dominance_failure_text(parentProfile, exactProfile)
                  << ",exact_support=" << profile_support_text(exactProfile)
                  << ",parent_support=" << profile_support_text(parentProfile)
                  << '\n';
    }
    std::int32_t upperRank = 1;
    for (const udon::MasterCandidate& candidate : wideAugmentedCandidates) {
        if (exactUpper < candidateUpper(candidate)) {
            ++upperRank;
        }
    }
    std::int32_t firstMasterCap = -1;
    for (const std::int32_t cap : {32, 48, 64, 96, 128, 192, 256}) {
        udon::MasterOptions capOptions = masterOptions;
        capOptions.maximumCandidates = cap;
        capOptions.diversityCandidates = std::max(1, cap / 4);
        udon::MasterDiagnostics capDiagnostics;
        const std::vector<udon::MasterCandidate> capCandidates = master.solve(
            state,
            ledger,
            augmented.portfolio,
            capOptions,
            capDiagnostics);
        if (contains_outcome(capCandidates, *exact)) {
            firstMasterCap = cap;
            break;
        }
    }
    std::string diversityRetention;
    for (const std::int32_t slots : {0, 8, 16, 24, 31}) {
        udon::MasterOptions diversityOptions = masterOptions;
        diversityOptions.diversityCandidates = slots;
        udon::MasterDiagnostics diversityDiagnostics;
        const std::vector<udon::MasterCandidate> diversityCandidates = master.solve(
            state,
            ledger,
            augmented.portfolio,
            diversityOptions,
            diversityDiagnostics);
        if (!diversityRetention.empty()) {
            diversityRetention += '/';
        }
        diversityRetention += std::to_string(slots) + ':' +
            (contains_outcome(diversityCandidates, *exact) ? '1' : '0');
    }
    std::string unboundedRetention;
    for (const std::int32_t slots : {0, 8, 16, 24, 31}) {
        udon::MasterOptions explorationOptions = masterOptions;
        explorationOptions.diversityCandidates = slots;
        explorationOptions.enableLexicographicBranchAndBound = false;
        udon::MasterDiagnostics explorationDiagnostics;
        const std::vector<udon::MasterCandidate> explorationCandidates = master.solve(
            state,
            ledger,
            augmented.portfolio,
            explorationOptions,
            explorationDiagnostics);
        if (!unboundedRetention.empty()) {
            unboundedRetention += '/';
        }
        unboundedRetention += std::to_string(slots) + ':' +
            (contains_outcome(explorationCandidates, *exact) ? '1' : '0');
    }
    const udon::MasterCandidate* prefixSlack = best_same_prefix_slack(
        wideAugmentedCandidates,
        *exact);

    std::cout << "attribute,seed=" << fixture.seed
              << ",day=" << day
              << ",exact=" << score_text(exact->scoreAfterToday)
              << ",legacy12_mask=" << portfolio_plan_mask(legacy, exact->plan)
              << ",expanded16_mask=" << portfolio_plan_mask(expanded, exact->plan)
              << ",expanded_active_flags="
              << matching_column_flags(expanded, exact->plan, 0U)
              << ",merged_mask=" << portfolio_plan_mask(merged, exact->plan)
              << ",wide32_mask=" << portfolio_plan_mask(wide32, exact->plan)
              << ",wide64_mask=" << portfolio_plan_mask(wide64, exact->plan)
              << ",merged_master_exact=" << contains_candidate(mergedCandidates, *exact)
              << ",merged_master_outcome=" << contains_outcome(mergedCandidates, *exact)
              << ",merged_best=" << best_candidate_score(mergedCandidates)
              << ",forced_bundle=" << completeForcedBundle
              << ",forced_exact=" << contains_candidate(forcedCandidates, *exact)
              << ",wide_master_exact=" << contains_candidate(wideMasterCandidates, *exact)
              << ",wide_master_outcome=" << contains_outcome(wideMasterCandidates, *exact)
              << ",wide_master_count=" << wideMasterCandidates.size()
              << ",exact_slack=" << slack_text(exact->terminalSlack)
              << ",prefix_slack=" << (prefixSlack == nullptr
                    ? std::string{"none"}
                    : slack_text(prefixSlack->terminalSlack))
              << ",prefix_slack_exact=" << (prefixSlack != nullptr &&
                    prefixSlack->stableId == exact->stableId)
              << ",augmented_retained=" << augmented.retainedNovelRoutes
              << ",augmented_mask=" << portfolio_plan_mask(augmented.portfolio, exact->plan)
              << ",augmented_master_exact=" << contains_candidate(augmentedCandidates, *exact)
              << ",augmented_master_outcome=" << contains_outcome(augmentedCandidates, *exact)
              << ",augmented_best=" << best_candidate_score(augmentedCandidates)
              << ",wide_augmented_exact=" << contains_candidate(wideAugmentedCandidates, *exact)
              << ",wide_augmented_outcome=" << contains_outcome(wideAugmentedCandidates, *exact)
              << ",exact_upper=" << score_text(exactUpper)
              << ",upper_rank=" << upperRank
              << ",first_master_cap=" << firstMasterCap
              << ",diversity=" << diversityRetention
              << ",no_bnb=" << unboundedRetention
              << ",merged_nodes=" << mergedDiagnostics.combinationsVisited
              << ",augmented_nodes=" << augmentedDiagnostics.combinationsVisited
              << '\n';
}

void attribute_oracle_path(
    const Fixture& fixture,
    const OracleResult& oracle,
    const HeadResult& head) {
    std::vector<udon::AgentState> agents;
    for (const udon::CellId start : fixture.config.initialAgents) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            start,
            fixture.config.fuelLimit,
        });
    }
    udon::MatchLedger ledger;
    TrafficFootprint previousOwn{};
    TrafficFootprint priorOwn{};
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const std::vector<udon::RoadStatus> roadStatuses = exact_road_statuses(
            fixture,
            day,
            previousOwn,
            priorOwn);
        const udon::DayState state = day_state(
            fixture.config,
            day,
            agents,
            roadStatuses);
        const udon::DayPlan& plan = oracle.plans.at(
            static_cast<std::size_t>(day - 1));
        attribute_oracle_day(
            fixture,
            day,
            state,
            ledger,
            plan,
            head.plans.at(static_cast<std::size_t>(day - 1)));
        udon::SimulationResult simulation;
        std::string mismatch;
        if (!validates(fixture.config, state, plan, simulation, mismatch)) {
            throw std::runtime_error(
                "oracle attribution replay failed: " + mismatch);
        }
        agents = simulation.finalAgents;
        ledger.apply(simulation.score);
        priorOwn = previousOwn;
        previousOwn = compact_road_footprint(
            fixture.config,
            simulation.roadFootprint);
    }
}

void attribute_w1_continuation(
    const Fixture& fixture,
    const OracleResult& oracle) {
    const udon::ExactStepSimulator simulator(fixture.config);
    const udon::IndependentDayValidator validator(fixture.config);
    const udon::ParetoRouter router(fixture.config);
    const udon::RouteColumnGenerator generator(fixture.config, router);
    const udon::RouteMaster master(fixture.config, simulator, validator);
    const udon::FastViabilityAnalyzer viability(fixture.config);
    std::vector<udon::AgentState> agents;
    for (const udon::CellId start : fixture.config.initialAgents) {
        agents.push_back(udon::AgentState{
            udon::AgentKind::Patrol,
            start,
            fixture.config.fuelLimit,
        });
    }
    udon::MatchLedger ledger;
    const udon::DayState firstState = day_state(fixture.config, 1, agents);
    const std::optional<udon::MasterCandidate> first = master.evaluate_exact_plan(
        firstState,
        ledger,
        oracle.plans.front());
    if (!first.has_value()) {
        throw std::runtime_error("W1 attribution exact day1 plan is invalid");
    }
    ledger.apply(first->simulation.score);
    agents = first->simulation.finalAgents;
    constexpr std::int32_t perDayRepairCap = 100;
    for (std::int32_t day = 2; day <= fixture.config.day_count(); ++day) {
        const udon::DayState state = day_state(fixture.config, day, agents);
        const udon::DayPlan& oraclePlan = oracle.plans.at(
            static_cast<std::size_t>(day - 1));
        const std::optional<udon::MasterCandidate> exact =
            master.evaluate_exact_plan(state, ledger, oraclePlan);
        if (!exact.has_value()) {
            std::cout << "w1_attribute,seed=" << fixture.seed
                      << ",day=" << day
                      << ",status=oracle-plan-invalid-after-divergence\n";
            break;
        }
        udon::ColumnGenerationOptions generation;
        generation.maximumPathsPerTarget = 1;
        generation.maximumColumnsPerAgent = day <= 3 ? 3 : 2;
        generation.maximumTargetSpots = day <= 3 ? 6 : 4;
        generation.maximumEscorts = day <= 3 ? 4 : 2;
        generation.maximumSeedPlans = day <= 3 ? 0 : 1;
        generation.enableHarvestExtensions = kFutureHarvestMode > 0;
        generation.allowUncachedHarvestTargets = kFutureHarvestMode > 1;
        generation.enableHarvestOrienteering =
            kFutureHarvestMode > 5 &&
            static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                3LL * fixture.config.steps_for_day(day);
        generation.enableExactHarvestOrienteering =
            kFutureHarvestMode > 5 &&
            static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                2LL * fixture.config.steps_for_day(day) &&
            (kFutureHarvestMode > 6 || day == fixture.config.day_count());
        generation.maximumHarvestExtensionSources =
            kFutureHarvestMode > 2 ? 4 : 1;
        generation.maximumHarvestExtensionDepth =
            kFutureHarvestMode > 4 &&
                static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                    3LL * fixture.config.steps_for_day(day)
            ? 4
            : (kFutureHarvestMode > 3 ? 3 : 2);
        udon::MasterOptions masterOptions;
        masterOptions.maximumCombinations = day <= 3
            ? perDayRepairCap
            : std::max(16, perDayRepairCap / 4);
        masterOptions.maximumCandidates = 1;
        masterOptions.maximumResolveRounds = 1;
        if (day > 3) {
            const udon::ViabilityBounds bounds = viability.analyze(state, ledger);
            generation.mandatoryReservations = bounds.reservations;
            masterOptions.mandatoryReservations = bounds.reservations;
        }
        const udon::RoutePortfolio portfolio = generator.generate(
            state,
            ledger,
            generation);
        if (day == 2) {
            for (udon::AgentIndex agent = 0; agent < 2; ++agent) {
                std::uint32_t oracleSpotMask = 0U;
                for (const udon::ClaimEvent& claim : exact->simulation.claims) {
                    if (claim.agent == agent) {
                        oracleSpotMask |= std::uint32_t{1} <<
                            static_cast<std::uint32_t>(claim.spot);
                    }
                }
                const udon::ExactOrienteeringReachability reachability =
                    udon::enumerate_exact_resource_routes(
                        fixture.config,
                        state,
                        agent);
                const auto route_mask_present = [oracleSpotMask](
                    const std::vector<udon::ExactOrienteeringRoute>& routes) {
                    return std::any_of(
                        routes.begin(),
                        routes.end(),
                        [oracleSpotMask](const udon::ExactOrienteeringRoute& route) {
                            return route.spotMask == oracleSpotMask;
                        });
                };
                std::int32_t strictSupersets = 0;
                std::int32_t sameTerminalFuelSupersets = 0;
                for (const udon::ExactOrienteeringRoute& route :
                     reachability.maximalRoutes) {
                    if (route.spotMask != oracleSpotMask &&
                        (route.spotMask & oracleSpotMask) == oracleSpotMask) {
                        ++strictSupersets;
                        const udon::AgentState& oracleTerminal =
                            exact->simulation.finalAgents.at(
                                static_cast<std::size_t>(agent));
                        if (route.terminalCell == oracleTerminal.position &&
                            route.patrolFuel == oracleTerminal.fuel) {
                            ++sameTerminalFuelSupersets;
                        }
                    }
                }
                std::cout << "w1_mask_attribute,seed=" << fixture.seed
                          << ",day=" << day
                          << ",agent=" << agent
                          << ",oracle_mask=" << oracleSpotMask
                          << ",oracle_terminal="
                          << exact->simulation.finalAgents.at(
                                 static_cast<std::size_t>(agent)).position
                          << ",oracle_fuel="
                          << exact->simulation.finalAgents.at(
                                 static_cast<std::size_t>(agent)).fuel
                          << ",maximal_present="
                          << route_mask_present(reachability.maximalRoutes)
                          << ",terminal_present="
                          << route_mask_present(reachability.terminalVariants)
                          << ",strict_supersets=" << strictSupersets
                          << ",same_terminal_fuel_supersets="
                          << sameTerminalFuelSupersets
                          << ",complete=" << reachability.complete
                          << '\n';
            }
            const std::vector<DayOutcome> firstOutcomes = enumerate_day(
                fixture.config,
                day,
                state.agents.at(0).position,
                state.agents.at(0).fuel,
                state.roadStatuses);
            const std::vector<DayOutcome> secondOutcomes = enumerate_day(
                fixture.config,
                day,
                state.agents.at(1).position,
                state.agents.at(1).fuel,
                state.roadStatuses);
            std::uint32_t oracleFirstMask = 0U;
            std::uint32_t oracleSecondMask = 0U;
            for (const udon::ClaimEvent& claim : exact->simulation.claims) {
                if (claim.agent == 0) {
                    oracleFirstMask |= std::uint32_t{1} <<
                        static_cast<std::uint32_t>(claim.spot);
                } else if (claim.agent == 1) {
                    oracleSecondMask |= std::uint32_t{1} <<
                        static_cast<std::uint32_t>(claim.spot);
                }
            }
            const auto terminal_frontier_text = [&fixture](
                                                        const std::vector<DayOutcome>& outcomes) {
                std::map<udon::CellId, std::int32_t> counts;
                for (const DayOutcome& outcome : outcomes) {
                    if (outcome.spotMask == 0U ||
                        fixture.config.spotAtCell.at(
                            static_cast<std::size_t>(outcome.position)) ==
                            udon::kInvalidSpot) {
                        continue;
                    }
                    ++counts[outcome.position];
                }
                std::ostringstream text;
                bool first = true;
                for (const auto& [terminal, count] : counts) {
                    if (!first) {
                        text << '|';
                    }
                    first = false;
                    text << terminal << ':' << count;
                }
                return text.str();
            };
            const auto maximum_terminal_frontier = [&fixture](
                                                           const std::vector<DayOutcome>& outcomes) {
                std::map<udon::CellId, std::int32_t> counts;
                for (const DayOutcome& outcome : outcomes) {
                    if (outcome.spotMask != 0U &&
                        fixture.config.spotAtCell.at(
                            static_cast<std::size_t>(outcome.position)) !=
                            udon::kInvalidSpot) {
                        ++counts[outcome.position];
                    }
                }
                std::int32_t maximum = 0;
                for (const auto& [terminal, count] : counts) {
                    static_cast<void>(terminal);
                    maximum = std::max(maximum, count);
                }
                return maximum;
            };
            std::vector<udon::MasterCandidate> exactFrontierCandidates;
            std::set<std::string> exactFrontierIds;
            for (const DayOutcome& firstOutcome : firstOutcomes) {
                for (const DayOutcome& secondOutcome : secondOutcomes) {
                    udon::DayPlan plan;
                    plan.actions.resize(
                        static_cast<std::size_t>(fixture.config.agent_count()));
                    plan.actions.at(0) = firstOutcome.actions;
                    plan.actions.at(1) = secondOutcome.actions;
                    for (std::size_t agent = 2U;
                         agent < plan.actions.size();
                         ++agent) {
                        plan.actions.at(agent) = udon::AgentPlan{
                            udon::PlanAction::wait(
                                fixture.config.steps_for_day(day)),
                        };
                    }
                    std::optional<udon::MasterCandidate> candidate =
                        master.evaluate_exact_plan(state, ledger, plan);
                    if (candidate.has_value() &&
                        exactFrontierIds.insert(candidate->stableId).second) {
                        exactFrontierCandidates.push_back(std::move(*candidate));
                    }
                }
            }
            const std::int32_t fullFrontierColumnCap =
                static_cast<std::int32_t>(std::max(
                    firstOutcomes.size(),
                    secondOutcomes.size())) + 8;
            const udon::RoutePoolAugmentation frontierAugmentation =
                generator.augment_with_candidate_routes(
                    state,
                    portfolio,
                    exactFrontierCandidates,
                    fullFrontierColumnCap);
            udon::MasterDiagnostics frontierW1Diagnostics;
            const std::vector<udon::MasterCandidate> frontierW1Candidates =
                master.solve(
                    state,
                    ledger,
                    frontierAugmentation.portfolio,
                    masterOptions,
                    frontierW1Diagnostics);
            udon::MasterOptions frontierNormalOptions;
            frontierNormalOptions.maximumCombinations = 40000;
            frontierNormalOptions.maximumCandidates = 32;
            frontierNormalOptions.diversityCandidates = 8;
            udon::MasterDiagnostics frontierNormalDiagnostics;
            const std::vector<udon::MasterCandidate> frontierNormalCandidates =
                master.solve(
                    state,
                    ledger,
                    frontierAugmentation.portfolio,
                    frontierNormalOptions,
                    frontierNormalDiagnostics);
            std::int32_t frontierFirstCap = -1;
            for (const std::int32_t cap : {32, 48, 64, 96, 128, 192, 256}) {
                udon::MasterOptions capOptions = frontierNormalOptions;
                capOptions.maximumCandidates = cap;
                capOptions.diversityCandidates = std::max(1, cap / 4);
                udon::MasterDiagnostics capDiagnostics;
                const std::vector<udon::MasterCandidate> capCandidates =
                    master.solve(
                        state,
                        ledger,
                        frontierAugmentation.portfolio,
                        capOptions,
                        capDiagnostics);
                if (contains_outcome(capCandidates, *exact)) {
                    frontierFirstCap = cap;
                    break;
                }
            }
            std::ostringstream frontierWidths;
            for (std::size_t agent = 0;
                 agent < frontierAugmentation.portfolio.columnsByAgent.size();
                 ++agent) {
                if (agent != 0U) {
                    frontierWidths << '|';
                }
                frontierWidths <<
                    frontierAugmentation.portfolio.columnsByAgent.at(agent).size();
            }
            std::cout << "w1_resource_frontier,seed=" << fixture.seed
                      << ",day=" << day
                      << ",first_terminal_counts="
                      << terminal_frontier_text(firstOutcomes)
                      << ",second_terminal_counts="
                      << terminal_frontier_text(secondOutcomes)
                      << ",first_max_terminal="
                      << maximum_terminal_frontier(firstOutcomes)
                      << ",second_max_terminal="
                      << maximum_terminal_frontier(secondOutcomes)
                      << ",joint_candidates="
                      << exactFrontierCandidates.size()
                      << ",portfolio_widths=" << frontierWidths.str()
                      << ",novel_routes="
                      << frontierAugmentation.retainedNovelRoutes
                      << ",w1_nodes="
                      << frontierW1Diagnostics.combinationsVisited
                      << ",w1_exact="
                      << contains_candidate(frontierW1Candidates, *exact)
                      << ",w1_outcome="
                      << contains_outcome(frontierW1Candidates, *exact)
                      << ",normal_nodes="
                      << frontierNormalDiagnostics.combinationsVisited
                      << ",normal_exact="
                      << contains_candidate(frontierNormalCandidates, *exact)
                      << ",normal_outcome="
                      << contains_outcome(frontierNormalCandidates, *exact)
                      << ",first_cap=" << frontierFirstCap
                      << '\n';
            const DayOutcome* exactSecondOutcome = nullptr;
            for (const DayOutcome& outcome : secondOutcomes) {
                if (outcome.position == exact->simulation.finalAgents.at(1).position &&
                    outcome.fuel == exact->simulation.finalAgents.at(1).fuel &&
                    outcome.spotMask == oracleSecondMask) {
                    exactSecondOutcome = &outcome;
                    break;
                }
            }
            const auto conditional_rank =
                [&](const DayOutcome& outcome, bool fuelFirst) {
                    const auto [brands, servings] = joint_day_score(
                        fixture.config,
                        outcome.spotMask,
                        oracleSecondMask);
                    const udon::OfficialScore current{
                        udon::brand_count(ledger.lifetimeBrands | brands),
                        ledger.totalDailyDistinct +
                            udon::brand_count(brands),
                        ledger.totalServings + servings,
                    };
                    return fuelFirst
                        ? std::tuple{
                              outcome.fuel,
                              current.lifetimeDistinct,
                              current.totalDailyDistinct,
                              current.totalServings,
                              -static_cast<std::int32_t>(outcome.spotMask)}
                        : std::tuple{
                              current.lifetimeDistinct,
                              current.totalDailyDistinct,
                              current.totalServings,
                              outcome.fuel,
                              -static_cast<std::int32_t>(outcome.spotMask)};
                };
            const udon::CellId oracleFirstTerminal =
                exact->simulation.finalAgents.at(0).position;
            const std::int32_t oracleFirstFuel =
                exact->simulation.finalAgents.at(0).fuel;
            std::int32_t sameTerminalOutcomes = 0;
            std::int32_t oracleOutcomeMatches = 0;
            std::int32_t fuelRank = 1;
            std::int32_t fuelTies = 0;
            std::int32_t currentRank = 1;
            std::int32_t currentTies = 0;
            std::int32_t upperRank = 1;
            std::int32_t upperTies = 0;
            udon::OfficialScore oracleConditionalUpper;
            bool oracleConditionalUpperSet = false;
            const DayOutcome oracleFirstOutcome{
                oracleFirstTerminal,
                oracleFirstFuel,
                oracleFirstMask,
                {},
                {},
            };
            const auto oracleFuelRank = conditional_rank(
                oracleFirstOutcome,
                true);
            const auto oracleCurrentRank = conditional_rank(
                oracleFirstOutcome,
                false);
            if (exactSecondOutcome != nullptr) {
                udon::MatchLedger futureLedger = ledger;
                futureLedger.apply(exact->simulation.score);
                const udon::DayState futureState = day_state(
                    fixture.config,
                    day + 1,
                    exact->simulation.finalAgents);
                oracleConditionalUpper = viability.analyze(
                    futureState,
                    futureLedger).upperBound;
                oracleConditionalUpperSet = true;
            }
            for (const DayOutcome& outcome : firstOutcomes) {
                if (outcome.position != oracleFirstTerminal ||
                    outcome.spotMask == 0U) {
                    continue;
                }
                ++sameTerminalOutcomes;
                oracleOutcomeMatches +=
                    outcome.fuel == oracleFirstFuel &&
                    outcome.spotMask == oracleFirstMask
                    ? 1
                    : 0;
                const auto candidateFuelRank = conditional_rank(outcome, true);
                const auto candidateCurrentRank = conditional_rank(outcome, false);
                fuelRank += oracleFuelRank < candidateFuelRank ? 1 : 0;
                fuelTies += oracleFuelRank == candidateFuelRank ? 1 : 0;
                currentRank += oracleCurrentRank < candidateCurrentRank ? 1 : 0;
                currentTies += oracleCurrentRank == candidateCurrentRank ? 1 : 0;
                if (exactSecondOutcome == nullptr) {
                    continue;
                }
                const auto [brands, servings] = joint_day_score(
                    fixture.config,
                    outcome.spotMask,
                    exactSecondOutcome->spotMask);
                udon::MatchLedger futureLedger = ledger;
                futureLedger.apply(udon::DayScore{
                    brands,
                    udon::brand_count(brands),
                    servings,
                });
                std::vector<udon::AgentState> futureAgents = state.agents;
                futureAgents.at(0).position = outcome.position;
                futureAgents.at(0).fuel = outcome.fuel;
                futureAgents.at(1).position = exactSecondOutcome->position;
                futureAgents.at(1).fuel = exactSecondOutcome->fuel;
                const udon::OfficialScore upper = viability.analyze(
                    day_state(fixture.config, day + 1, futureAgents),
                    futureLedger).upperBound;
                upperRank += oracleConditionalUpper < upper ? 1 : 0;
                upperTies += oracleConditionalUpper == upper ? 1 : 0;
            }
            std::cout << "w1_terminal_rank,seed=" << fixture.seed
                      << ",day=" << day
                      << ",agent=0"
                      << ",terminal=" << oracleFirstTerminal
                      << ",oracle_mask=" << oracleFirstMask
                      << ",oracle_fuel=" << oracleFirstFuel
                      << ",same_terminal_outcomes=" << sameTerminalOutcomes
                      << ",oracle_matches=" << oracleOutcomeMatches
                      << ",fuel_rank=" << fuelRank
                      << ",fuel_ties=" << fuelTies
                      << ",current_rank=" << currentRank
                      << ",current_ties=" << currentTies
                      << ",upper_rank="
                      << (oracleConditionalUpperSet ? upperRank : -1)
                      << ",upper_ties="
                      << (oracleConditionalUpperSet ? upperTies : -1)
                      << ",upper="
                      << (oracleConditionalUpperSet
                              ? score_text(oracleConditionalUpper)
                              : "unavailable")
                      << '\n';
            using JointState = std::tuple<
                udon::CellId,
                std::int32_t,
                udon::CellId,
                std::int32_t,
                udon::BrandMask,
                std::int32_t>;
            std::set<JointState> jointStates;
            udon::MatchLedger exactFutureLedger = ledger;
            exactFutureLedger.apply(exact->simulation.score);
            const udon::DayState exactFutureState = day_state(
                fixture.config,
                day + 1,
                exact->simulation.finalAgents);
            const udon::OfficialScore oracleUpper = viability.analyze(
                exactFutureState,
                exactFutureLedger).upperBound;
            udon::OfficialScore maximumUpper;
            std::int32_t betterThanOracle = 0;
            std::int32_t tiedWithOracle = 0;
            bool oracleClassFound = false;
            for (const DayOutcome& firstOutcome : firstOutcomes) {
                for (const DayOutcome& secondOutcome : secondOutcomes) {
                    const auto [brands, servings] = joint_day_score(
                        fixture.config,
                        firstOutcome.spotMask,
                        secondOutcome.spotMask);
                    const JointState key{
                        firstOutcome.position,
                        firstOutcome.fuel,
                        secondOutcome.position,
                        secondOutcome.fuel,
                        brands,
                        servings,
                    };
                    if (!jointStates.insert(key).second) {
                        continue;
                    }
                    udon::MatchLedger futureLedger = ledger;
                    futureLedger.apply(udon::DayScore{
                        brands,
                        udon::brand_count(brands),
                        servings,
                    });
                    std::vector<udon::AgentState> futureAgents = state.agents;
                    futureAgents.at(0).position = firstOutcome.position;
                    futureAgents.at(0).fuel = firstOutcome.fuel;
                    futureAgents.at(1).position = secondOutcome.position;
                    futureAgents.at(1).fuel = secondOutcome.fuel;
                    const udon::DayState futureState = day_state(
                        fixture.config,
                        day + 1,
                        futureAgents);
                    const udon::OfficialScore upper = viability.analyze(
                        futureState,
                        futureLedger).upperBound;
                    if (maximumUpper < upper) {
                        maximumUpper = upper;
                    }
                    betterThanOracle += oracleUpper < upper ? 1 : 0;
                    tiedWithOracle += upper == oracleUpper ? 1 : 0;
                    oracleClassFound = oracleClassFound ||
                        (firstOutcome.position ==
                                exact->simulation.finalAgents.at(0).position &&
                         firstOutcome.fuel ==
                                exact->simulation.finalAgents.at(0).fuel &&
                         secondOutcome.position ==
                                exact->simulation.finalAgents.at(1).position &&
                         secondOutcome.fuel ==
                                exact->simulation.finalAgents.at(1).fuel &&
                         brands == exact->simulation.score.brands &&
                         servings == exact->simulation.score.servings);
                }
            }
            std::cout << "w1_upper_frontier,seed=" << fixture.seed
                      << ",day=" << day
                      << ",first_outcomes=" << firstOutcomes.size()
                      << ",second_outcomes=" << secondOutcomes.size()
                      << ",joint_states=" << jointStates.size()
                      << ",oracle_upper=" << score_text(oracleUpper)
                      << ",maximum_upper=" << score_text(maximumUpper)
                      << ",oracle_rank=" << (betterThanOracle + 1)
                      << ",oracle_ties=" << tiedWithOracle
                      << ",oracle_class_found=" << oracleClassFound
                      << '\n';
            udon::ColumnGenerationOptions anytime = generation;
            anytime.enableExactHarvestOrienteering = true;
            anytime.enableFuelConstrainedExactHarvestOrienteering = true;
            anytime.enableAnytimeFuelConstrainedHarvestOrienteering = true;
            udon::ColumnGenerationDiagnostics anytimeDiagnostics;
            const udon::RoutePortfolio anytimePortfolio = generator.generate(
                state,
                ledger,
                anytime,
                &anytimeDiagnostics);
            std::ostringstream anytimeWidths;
            for (std::size_t agent = 0;
                 agent < anytimePortfolio.columnsByAgent.size();
                 ++agent) {
                if (agent != 0U) {
                    anytimeWidths << '|';
                }
                anytimeWidths << anytimePortfolio.columnsByAgent.at(agent).size();
            }
            std::cout << "w1_anytime_attribute,seed=" << fixture.seed
                      << ",day=" << day
                      << ",oracle_mask="
                      << portfolio_plan_mask(anytimePortfolio, oraclePlan)
                      << ",columns=" << anytimeWidths.str()
                      << ",supported="
                      << anytimeDiagnostics.exactOrienteeringSupportedAgents
                      << ",complete="
                      << anytimeDiagnostics.exactOrienteeringCompleteAgents
                      << ",states="
                      << anytimeDiagnostics.exactOrienteeringSettledStates
                      << ",variants="
                      << anytimeDiagnostics.exactOrienteeringTerminalVariants
                      << ",bundles="
                      << anytimeDiagnostics.exactOrienteeringBundles
                      << ",deadline=" << anytimeDiagnostics.deadlineReached
                      << '\n';
            udon::ColumnGenerationOptions exactFuel = generation;
            exactFuel.enableExactHarvestOrienteering = true;
            exactFuel.enableFuelConstrainedExactHarvestOrienteering = true;
            exactFuel.enableAnytimeFuelConstrainedHarvestOrienteering = false;
            udon::ColumnGenerationDiagnostics exactFuelDiagnostics;
            const udon::RoutePortfolio exactFuelPortfolio = generator.generate(
                state,
                ledger,
                exactFuel,
                &exactFuelDiagnostics);
            std::ostringstream exactFuelWidths;
            for (std::size_t agent = 0;
                 agent < exactFuelPortfolio.columnsByAgent.size();
                 ++agent) {
                if (agent != 0U) {
                    exactFuelWidths << '|';
                }
                exactFuelWidths << exactFuelPortfolio.columnsByAgent.at(agent).size();
            }
            std::cout << "w1_exact_fuel_attribute,seed=" << fixture.seed
                      << ",day=" << day
                      << ",oracle_mask="
                      << portfolio_plan_mask(exactFuelPortfolio, oraclePlan)
                      << ",columns=" << exactFuelWidths.str()
                      << ",supported="
                      << exactFuelDiagnostics.exactOrienteeringSupportedAgents
                      << ",complete="
                      << exactFuelDiagnostics.exactOrienteeringCompleteAgents
                      << ",states="
                      << exactFuelDiagnostics.exactOrienteeringSettledStates
                      << ",variants="
                      << exactFuelDiagnostics.exactOrienteeringTerminalVariants
                      << ",bundles="
                      << exactFuelDiagnostics.exactOrienteeringBundles
                      << ",deadline=" << exactFuelDiagnostics.deadlineReached
                      << '\n';
        }
        udon::MasterDiagnostics diagnostics;
        const std::vector<udon::MasterCandidate> candidates = master.solve(
            state,
            ledger,
            portfolio,
            masterOptions,
            diagnostics);
        std::ostringstream widths;
        for (std::size_t agent = 0; agent < portfolio.columnsByAgent.size(); ++agent) {
            if (agent != 0U) {
                widths << '|';
            }
            widths << portfolio.columnsByAgent.at(agent).size();
        }
        const bool sameOutcome = contains_outcome(candidates, *exact);
        std::cout << "w1_attribute,seed=" << fixture.seed
                  << ",day=" << day
                  << ",oracle_mask=" << portfolio_plan_mask(portfolio, oraclePlan)
                  << ",columns=" << widths.str()
                  << ",nodes=" << diagnostics.combinationsVisited
                  << ",exact=" << score_text(exact->scoreAfterToday)
                  << ",chosen=" << best_candidate_score(candidates)
                  << ",exact_plan_retained=" << contains_candidate(candidates, *exact)
                  << ",exact_outcome_retained=" << sameOutcome
                  << '\n';
        if (!sameOutcome || candidates.empty()) {
            break;
        }
        ledger.apply(candidates.front().simulation.score);
        agents = candidates.front().simulation.finalAgents;
    }
}

} // namespace

// ATTR-SUFFIX-TRAJECTORY-MEMBERSHIP-209: walk the memoized argmax trajectory
// of a single winning day-1 root under one causal opponent policy and, for
// every day, measure whether the oracle's own day plan is expressible by the
// production generator from the coupled day state (both teams' traffic):
// (a) spot-mask membership in exact-orienteering maximal/terminal routes,
// (b) witness-caps portfolio + W1 master contains-outcome,
// (c) production-caps portfolio + production master contains-outcome and the
//     first candidate cap in {32..256} that retains the oracle outcome.
void attribute_trajectory_membership(
    const Fixture& fixture,
    const MinimaxMemo& memo,
    CausalOpponentPolicy policy,
    bool trafficHistoryQuotient,
    std::int32_t rootIndex) {
    const udon::ExactStepSimulator simulator(fixture.config);
    const udon::IndependentDayValidator validator(fixture.config);
    const udon::ParetoRouter router(fixture.config);
    const udon::RouteColumnGenerator generator(fixture.config, router);
    const udon::RouteMaster master(fixture.config, simulator, validator);
    const udon::FastViabilityAnalyzer viability(fixture.config);
    AdversarialKey key = initial_adversarial_key(fixture);
    udon::MatchLedger ledger;
    const auto outcome_matches = [](
        const udon::MasterCandidate& candidate,
        const udon::MasterCandidate& exact) {
        if (!(candidate.scoreAfterToday == exact.scoreAfterToday) ||
            candidate.simulation.finalAgents.size() !=
                exact.simulation.finalAgents.size()) {
            return false;
        }
        for (std::size_t agent = 0U;
             agent < candidate.simulation.finalAgents.size();
             ++agent) {
            const udon::AgentState& left =
                candidate.simulation.finalAgents.at(agent);
            const udon::AgentState& right =
                exact.simulation.finalAgents.at(agent);
            if (left.kind != right.kind || left.position != right.position ||
                left.fuel != right.fuel) {
                return false;
            }
        }
        return true;
    };
    for (std::int32_t day = 1; day <= fixture.config.day_count(); ++day) {
        const auto node = memo.find(key);
        if (node == memo.end()) {
            std::cout << "trace_membership,seed=" << fixture.seed
                      << ",root=" << rootIndex
                      << ",policy=" << policy_name(policy)
                      << ",day=" << day << ",status=memo-miss\n";
            return;
        }
        const udon::DayState state = adversarial_day_state(fixture, key);
        const TrafficStatusKey statuses = std::get<8>(key);
        const std::vector<DayOutcome> opponentOutcomes = opponent_day_outcomes(
            fixture.config,
            day,
            std::get<3>(key),
            std::get<4>(key),
            state.roadStatuses);
        const DayOutcome opponent = choose_causal_opponent(
            policy,
            statuses,
            opponentOutcomes);
        udon::SimulationResult ownSimulation;
        udon::SimulationResult opponentSimulation;
        if (!validate_adversarial_outcome(
                fixture,
                state,
                false,
                node->second.bestOwn,
                ownSimulation) ||
            !validate_adversarial_outcome(
                fixture,
                state,
                true,
                opponent,
                opponentSimulation)) {
            std::cout << "trace_membership,seed=" << fixture.seed
                      << ",root=" << rootIndex
                      << ",policy=" << policy_name(policy)
                      << ",day=" << day << ",status=trajectory-invalid\n";
            return;
        }
        const udon::DayPlan ownPlan =
            active_day_plan(fixture, day, node->second.bestOwn);
        const std::optional<udon::MasterCandidate> exact =
            master.evaluate_exact_plan(state, ledger, ownPlan);
        const std::uint32_t oracleMask = node->second.bestOwn.spotMask;

        // (a) exact-orienteering reachability from the coupled state.
        const udon::ExactOrienteeringReachability reachability =
            udon::enumerate_exact_resource_routes(fixture.config, state, 0);
        const auto mask_present = [oracleMask](
            const std::vector<udon::ExactOrienteeringRoute>& routes) {
            return std::any_of(
                routes.begin(),
                routes.end(),
                [oracleMask](const udon::ExactOrienteeringRoute& route) {
                    return route.spotMask == oracleMask;
                });
        };
        std::int32_t strictSupersets = 0;
        std::int32_t sameTerminalFuelSupersets = 0;
        const udon::AgentState& oracleTerminal =
            ownSimulation.finalAgents.at(0);
        for (const udon::ExactOrienteeringRoute& route :
             reachability.maximalRoutes) {
            if (route.spotMask != oracleMask &&
                (route.spotMask & oracleMask) == oracleMask) {
                ++strictSupersets;
                if (route.terminalCell == oracleTerminal.position &&
                    route.patrolFuel == oracleTerminal.fuel) {
                    ++sameTerminalFuelSupersets;
                }
            }
        }

        // (b) witness-caps portfolio + W1 master (mirrors the accepted W1
        // witness configuration in attribute_w1_continuation).
        udon::ColumnGenerationOptions witnessGeneration;
        witnessGeneration.maximumPathsPerTarget = 1;
        witnessGeneration.maximumColumnsPerAgent = day <= 3 ? 3 : 2;
        witnessGeneration.maximumTargetSpots = day <= 3 ? 6 : 4;
        witnessGeneration.maximumEscorts = day <= 3 ? 4 : 2;
        witnessGeneration.maximumSeedPlans = day <= 3 ? 0 : 1;
        witnessGeneration.enableHarvestExtensions = kFutureHarvestMode > 0;
        witnessGeneration.allowUncachedHarvestTargets = kFutureHarvestMode > 1;
        witnessGeneration.enableHarvestOrienteering =
            kFutureHarvestMode > 5 &&
            static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                3LL * fixture.config.steps_for_day(day);
        witnessGeneration.enableExactHarvestOrienteering =
            kFutureHarvestMode > 5 &&
            static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                2LL * fixture.config.steps_for_day(day) &&
            (kFutureHarvestMode > 6 || day == fixture.config.day_count());
        witnessGeneration.maximumHarvestExtensionSources =
            kFutureHarvestMode > 2 ? 4 : 1;
        witnessGeneration.maximumHarvestExtensionDepth =
            kFutureHarvestMode > 4 &&
                static_cast<std::int64_t>(fixture.config.fuelLimit) >=
                    3LL * fixture.config.steps_for_day(day)
            ? 4
            : (kFutureHarvestMode > 3 ? 3 : 2);
        udon::MasterOptions witnessMasterOptions;
        witnessMasterOptions.maximumCombinations = day <= 3 ? 100 : 25;
        witnessMasterOptions.maximumCandidates = 1;
        witnessMasterOptions.maximumResolveRounds = 1;
        if (day > 3) {
            const udon::ViabilityBounds bounds = viability.analyze(state, ledger);
            witnessGeneration.mandatoryReservations = bounds.reservations;
            witnessMasterOptions.mandatoryReservations = bounds.reservations;
        }
        const udon::RoutePortfolio witnessPortfolio = generator.generate(
            state,
            ledger,
            witnessGeneration);
        udon::MasterDiagnostics witnessDiagnostics;
        const std::vector<udon::MasterCandidate> witnessCandidates =
            master.solve(
                state,
                ledger,
                witnessPortfolio,
                witnessMasterOptions,
                witnessDiagnostics);

        // (c) production-caps portfolio (long deadline class of the live day
        // pipeline, decision.cpp) + production master, no deadline so the
        // membership answer is structural, never timed.
        udon::ColumnGenerationOptions productionGeneration = witnessGeneration;
        productionGeneration.maximumPathsPerTarget = 4;
        productionGeneration.maximumColumnsPerAgent = 16;
        productionGeneration.maximumTargetSpots = 12;
        productionGeneration.maximumEscorts = 16;
        productionGeneration.maximumSeedPlans = 2;
        const udon::RoutePortfolio productionPortfolio = generator.generate(
            state,
            ledger,
            productionGeneration);
        udon::MasterOptions productionMasterOptions;
        productionMasterOptions.maximumCombinations = 40000;
        productionMasterOptions.maximumCandidates = 32;
        productionMasterOptions.diversityCandidates = 8;
        udon::MasterDiagnostics productionDiagnostics;
        const std::vector<udon::MasterCandidate> productionCandidates =
            master.solve(
                state,
                ledger,
                productionPortfolio,
                productionMasterOptions,
                productionDiagnostics);
        std::int32_t firstCap = -1;
        if (exact.has_value()) {
            for (const std::int32_t cap : {32, 48, 64, 96, 128, 192, 256}) {
                udon::MasterOptions capOptions = productionMasterOptions;
                capOptions.maximumCandidates = cap;
                capOptions.diversityCandidates = std::max(1, cap / 4);
                udon::MasterDiagnostics capDiagnostics;
                const std::vector<udon::MasterCandidate> capCandidates =
                    master.solve(
                        state,
                        ledger,
                        productionPortfolio,
                        capOptions,
                        capDiagnostics);
                if (contains_outcome(capCandidates, *exact)) {
                    firstCap = cap;
                    break;
                }
            }
        }

        std::cout << "trace_membership,seed=" << fixture.seed
                  << ",root=" << rootIndex
                  << ",policy=" << policy_name(policy)
                  << ",day=" << day
                  << ",plan=" << plan_text(ownPlan)
                  << ",oracle_mask=" << oracleMask
                  << ",day_brands="
                  << udon::brand_count(ownSimulation.score.brands)
                  << ",day_servings=" << ownSimulation.score.servings
                  << ",exact_valid=" << (exact.has_value() ? 1 : 0)
                  << ",mask_maximal=" << mask_present(reachability.maximalRoutes)
                  << ",mask_terminal="
                  << mask_present(reachability.terminalVariants)
                  << ",strict_supersets=" << strictSupersets
                  << ",same_terminal_fuel_supersets=" << sameTerminalFuelSupersets
                  << ",reachability_complete=" << reachability.complete
                  << ",w1_outcome="
                  << (exact.has_value() &&
                      contains_outcome(witnessCandidates, *exact))
                  << ",w1_first="
                  << (exact.has_value() && !witnessCandidates.empty() &&
                      outcome_matches(witnessCandidates.front(), *exact))
                  << ",prod_outcome="
                  << (exact.has_value() &&
                      contains_outcome(productionCandidates, *exact))
                  << ",prod_first="
                  << (exact.has_value() && !productionCandidates.empty() &&
                      outcome_matches(productionCandidates.front(), *exact))
                  << ",first_cap=" << firstCap
                  << ",witness_widths=" << witnessPortfolio.columnsByAgent.at(0).size()
                  << ",production_widths="
                  << productionPortfolio.columnsByAgent.at(0).size();
        ledger.apply(ownSimulation.score);
        std::cout << ",cum="
                  << score_text(udon::OfficialScore{
                         ledger.lifetime_distinct(),
                         ledger.totalDailyDistinct,
                         ledger.totalServings,
                     })
                  << '\n';
        key = advance_adversarial_key(
            fixture,
            key,
            node->second.bestOwn,
            opponent,
            ownSimulation.score.brands,
            trafficHistoryQuotient);
    }
    std::cout << "trace_membership_summary,seed=" << fixture.seed
              << ",root=" << rootIndex
              << ",policy=" << policy_name(policy)
              << ",final="
              << score_text(udon::OfficialScore{
                     ledger.lifetime_distinct(),
                     ledger.totalDailyDistinct,
                     ledger.totalServings,
                 })
              << '\n';
}

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        const std::vector<ManifestRow> rows = load_manifest(options.manifest, options.split);
        Summary summary;
        std::map<std::pair<std::string, std::string>, Summary> strata;
        std::int32_t headOnlyCases = 0;
        std::int32_t headOnlyInvalid = 0;
        for (const ManifestRow& row : rows) {
            for (std::int32_t offset = 0; offset < row.count; ++offset) {
                const std::uint64_t seed = row.firstSeed + static_cast<std::uint64_t>(offset);
                if (options.onlySeed != 0U && seed != options.onlySeed) {
                    continue;
                }
                if (options.maximumMatches > 0 && summary.cases >= options.maximumMatches) {
                    break;
                }
                const Fixture fixture = make_fixture(row, seed);
                if (fixture.adversarialTraffic) {
                    if (options.protectedHead) {
                        for (const CausalOpponentPolicy policy : {
                                 CausalOpponentPolicy::MaximumDwell,
                                 CausalOpponentPolicy::MinimumDwell,
                                 CausalOpponentPolicy::StatusToggle,
                             }) {
                            const AdversarialRun current =
                                run_current_protected_against_policy(
                                    fixture,
                                    policy,
                                    options.latestTerminal,
                                    std::chrono::milliseconds{
                                        options.protectedRefinementReserveMs});
                            std::cout << "protected_head,seed=" << seed
                                      << ",policy=" << policy_name(policy)
                                      << ",protected_reserve_ms="
                                      << options.protectedRefinementReserveMs
                                      << ",score=" << score_text(current.score)
                                      << ",virtual="
                                      << score_text(current.virtualScore)
                                      << ",valid=" << current.valid
                                      << ",takeovers="
                                      << current.protectedTakeovers
                                      << ",generated="
                                      << current.protectedGeneratedPlans
                                      << ",valid_plans="
                                      << current.protectedValidPlans
                                      << ",liftable="
                                      << current.protectedLiftablePlans
                                      << ",protected_deadline_days="
                                      << current.protectedDeadlineDays
                                      << ",solver_deadline_days="
                                      << current.deadlineLimitedDays
                                      << ",terminal_sparse_routes="
                                      << current.terminalSparseRoutes
                                      << ",terminal_valid_plans="
                                      << current.terminalValidPlans
                                      << ",terminal_improvements="
                                      << current.terminalStrictImprovements
                                      << ",terminal_rounds="
                                      << current.terminalRounds
                                      << ",terminal_deadline_days="
                                      << current.terminalDeadlineDays
                                      << ",plan_hash=" << std::hex
                                      << plan_sequence_hash(current.plans)
                                      << std::dec << '\n';
                            for (std::size_t traceDay = 0;
                                 traceDay < current.traces.size();
                                 ++traceDay) {
                                const AdversarialRun::DayTrace& trace =
                                    current.traces.at(traceDay);
                                std::cout
                                    << "protected_head_day,seed=" << seed
                                    << ",policy=" << policy_name(policy)
                                    << ",day=" << (traceDay + 1U)
                                    << ",daily=" << trace.dailyDistinct << '/'
                                    << trace.dailyServings
                                    << ",cumulative="
                                    << score_text(trace.cumulative)
                                    << ",pos_fuel="
                                    << trace.terminalPosition << '/'
                                    << trace.terminalFuel
                                    << ",plan="
                                    << plan_text(current.plans.at(traceDay))
                                    << '\n';
                            }
                        }
                        return 0;
                    }
                    if (options.headOnly) {
                        const AdversarialRun head = run_head_against_policy(
                            fixture,
                            CausalOpponentPolicy::MaximumDwell);
                        std::cout << "adversarial_head,seed=" << seed
                                  << ",score=" << score_text(head.score)
                                  << ",valid=" << head.valid
                                  << ",plan_hash=" << std::hex
                                  << plan_sequence_hash(head.plans)
                                  << std::dec << '\n';
                        for (std::size_t dayIndex = 0;
                             dayIndex < head.audits.size();
                             ++dayIndex) {
                            const udon::DecisionAudit& audit =
                                head.audits.at(dayIndex);
                            std::cout << "adversarial_head_day,seed=" << seed
                                      << ",day=" << (dayIndex + 1U)
                                      << ",plan="
                                      << plan_text(head.plans.at(dayIndex))
                                      << ",selection_reason="
                                      << audit.selectionReason
                                      << ",exact_supported="
                                      << audit.columnGeneration
                                             .exactOrienteeringSupportedAgents
                                      << ",exact_complete="
                                      << audit.columnGeneration
                                             .exactOrienteeringCompleteAgents
                                      << ",exact_states="
                                      << audit.columnGeneration
                                             .exactOrienteeringSettledStates
                                      << ",exact_variants="
                                      << audit.columnGeneration
                                             .exactOrienteeringTerminalVariants
                                      << ",exact_bundles="
                                      << audit.columnGeneration
                                             .exactOrienteeringBundles
                                      << ",exact_deadline="
                                      << audit.columnGeneration.deadlineReached
                                      << ",portfolio_widths=";
                            for (std::size_t agent = 0;
                                 agent < audit.portfolioColumnsByAgent.size();
                                 ++agent) {
                                if (agent != 0U) {
                                    std::cout << '|';
                                }
                                std::cout <<
                                    audit.portfolioColumnsByAgent.at(agent);
                            }
                            std::cout << '\n';
                            for (std::size_t candidateIndex = 0;
                                 candidateIndex < audit.candidates.size();
                                 ++candidateIndex) {
                                const udon::CandidateAuditRecord& record =
                                    audit.candidates.at(candidateIndex);
                                std::cout << "adversarial_head_candidate,seed="
                                          << seed
                                          << ",day=" << (dayIndex + 1U)
                                          << ",index=" << candidateIndex
                                          << ",current="
                                          << score_text(record.scoreAfterToday)
                                          << ",lower="
                                          << score_text(
                                                 record.finalCertifiedLowerBound)
                                          << ",upper="
                                          << score_text(record.validUpperBound)
                                          << ",q50="
                                          << score_text(record.finalQuantile50)
                                          << ",certified=" << record.certified
                                          << ",selected=" << record.selected
                                          << ",role=" << record.w1Role
                                          << ",disposition="
                                          << record.disposition << '\n';
                            }
                        }
                        return 0;
                    }
                    if (!options.inspectPlan.empty()) {
                        const udon::DayPlan oraclePlan = parse_plan_text(
                            options.inspectPlan,
                            fixture.config.agent_count());
                        const udon::DayPlan parentPlan = parse_plan_text(
                            options.inspectParentPlan,
                            fixture.config.agent_count());
                        if (!options.attributePrefix.empty()) {
                            const std::vector<udon::DayPlan> attributePrefix =
                                parse_plan_sequence_text(
                                    options.attributePrefix,
                                    fixture.config.agent_count());
                            for (const CausalOpponentPolicy policy : {
                                     CausalOpponentPolicy::MaximumDwell,
                                     CausalOpponentPolicy::MinimumDwell,
                                     CausalOpponentPolicy::StatusToggle,
                                 }) {
                                const AdversarialPrefixContext context =
                                    replay_adversarial_prefix(
                                        fixture,
                                        policy,
                                        attributePrefix);
                                std::cout << "attribute_context,seed=" << seed
                                          << ",policy=" << policy_name(policy)
                                          << ",day="
                                          << (attributePrefix.size() + 1U)
                                          << ",valid=" << context.valid << '\n';
                                if (!context.valid) {
                                    continue;
                                }
                                attribute_oracle_day(
                                    fixture,
                                    static_cast<std::int32_t>(
                                        attributePrefix.size() + 1U),
                                    adversarial_day_state(fixture, context.key),
                                    context.ledger,
                                    oraclePlan,
                                    parentPlan);
                            }
                            return 0;
                        }
                        const AdversarialKey key = initial_adversarial_key(fixture);
                        attribute_oracle_day(
                            fixture,
                            1,
                            adversarial_day_state(fixture, key),
                            udon::MatchLedger{},
                            oraclePlan,
                            parentPlan);
                        const std::vector<udon::DayPlan> forcedPrefix =
                            options.forcedPrefix.empty()
                            ? std::vector<udon::DayPlan>{oraclePlan}
                            : parse_plan_sequence_text(
                                  options.forcedPrefix,
                                  fixture.config.agent_count());
                        for (const CausalOpponentPolicy policy : {
                                 CausalOpponentPolicy::MaximumDwell,
                                 CausalOpponentPolicy::MinimumDwell,
                                 CausalOpponentPolicy::StatusToggle,
                             }) {
                            const AdversarialRun forced = run_head_against_policy(
                                fixture,
                                policy,
                                &forcedPrefix);
                            std::cout << "forced_first,seed=" << seed
                                      << ",policy=" << policy_name(policy)
                                      << ",prefix_days=" << forcedPrefix.size()
                                      << ",score=" << score_text(forced.score)
                                      << ",valid=" << forced.valid
                                      << ",deadline_days="
                                      << forced.deadlineLimitedDays
                                      << ",plan_hash=" << std::hex
                                      << plan_sequence_hash(forced.plans)
                                      << std::dec;
                            for (std::size_t traceDay = 0;
                                 traceDay < forced.traces.size();
                                 ++traceDay) {
                                std::cout << ",d" << (traceDay + 1U) << '='
                                          << score_text(
                                                 forced.traces.at(traceDay).cumulative)
                                          << ",p" << (traceDay + 1U) << '='
                                          << plan_text(forced.plans.at(traceDay));
                            }
                            std::cout << '\n';
                        }
                        return 0;
                    }
                    MinimaxSearch minimaxSearch;
                    minimaxSearch.boundaryDominance = options.boundaryDominance;
                    minimaxSearch.trafficHistoryQuotient =
                        options.trafficHistoryQuotient;
                    const std::int32_t startDay = options.proofTailDays > 0
                        ? fixture.config.day_count() - options.proofTailDays + 1
                        : 1;
                    const AdversarialKey rootKey =
                        initial_adversarial_key(fixture, startDay);
                    MinimaxNode root;
                    if (options.rootStream) {
                        const RootStreamResult streamed =
                            solve_public_minimax_root_stream(
                                fixture,
                                rootKey,
                                minimaxSearch,
                                options.rootOwnBegin,
                                options.rootOwnEnd);
                        if (!streamed.fullSlice || !streamed.hasRoot) {
                            std::cout << "minimax_slice,seed=" << seed
                                      << ",family=" << row.family
                                      << ",fuel=" << row.fuelProfile
                                      << ",begin=" << streamed.begin
                                      << ",end=" << streamed.end
                                      << ",own_count=" << streamed.ownCount
                                      << ",opponent_count="
                                      << streamed.opponentCount
                                      << ",slice_best="
                                      << (streamed.hasRoot
                                              ? score_text(streamed.root.score)
                                              : "none")
                                      << std::endl;
                            if (streamed.hasRoot) {
                                minimaxSearch.memo.emplace(
                                    rootKey,
                                    streamed.root);
                                for (const CausalOpponentPolicy policy : {
                                         CausalOpponentPolicy::MaximumDwell,
                                         CausalOpponentPolicy::MinimumDwell,
                                         CausalOpponentPolicy::StatusToggle,
                                     }) {
                                    const AdversarialRun oracle =
                                        run_minimax_policy(
                                            fixture,
                                            minimaxSearch.memo,
                                            policy,
                                            options.trafficHistoryQuotient);
                                    const AdversarialRun head =
                                        run_head_against_policy(
                                            fixture,
                                            policy);
                                    std::cout
                                        << "root_stream_case,seed=" << seed
                                        << ",begin=" << streamed.begin
                                        << ",end=" << streamed.end
                                        << ",policy=" << policy_name(policy)
                                        << ",slice_robust="
                                        << score_text(streamed.root.score)
                                        << ",oracle="
                                        << score_text(oracle.score)
                                        << ",head=" << score_text(head.score)
                                        << ",oracle_valid=" << oracle.valid
                                        << ",head_valid=" << head.valid
                                        << ",oracle_hash=" << std::hex
                                        << plan_sequence_hash(oracle.plans)
                                        << ",head_hash="
                                        << plan_sequence_hash(head.plans)
                                        << std::dec << std::endl;
                                    if (options.traceMembership) {
                                        attribute_trajectory_membership(
                                            fixture,
                                            minimaxSearch.memo,
                                            policy,
                                            options.trafficHistoryQuotient,
                                            options.rootOwnBegin);
                                    }
                                }
                            }
                            continue;
                        }
                        root = streamed.root;
                    } else {
                        root = solve_public_minimax(
                            fixture,
                            rootKey,
                            minimaxSearch);
                    }
                    std::cout << "minimax,seed=" << seed
                              << ",family=" << row.family
                              << ",fuel=" << row.fuelProfile
                              << ",robust=" << score_text(root.score)
                              << ",states=" << minimaxSearch.diagnostics.states
                              << ",transitions=" << minimaxSearch.diagnostics.transitions
                              << ",boundary=" << (options.boundaryDominance ? 1 : 0)
                              << ",traffic_history_quotient="
                              << (options.trafficHistoryQuotient ? 1 : 0)
                              << ",proof_tail=" << options.proofTailDays
                              << ",raw_max="
                              << minimaxSearch.diagnostics.maximumOwnOutcomesBeforeDominance
                              << ",max_own=" << minimaxSearch.diagnostics.maximumOwnOutcomes
                              << ",max_opponent=" << minimaxSearch.diagnostics.maximumOpponentOutcomes
                              << ",pruned="
                              << minimaxSearch.diagnostics.boundaryDominancePruned
                              << '\n';
                    if (options.proofTailDays > 0) {
                        continue;
                    }
                    for (const CausalOpponentPolicy policy : {
                             CausalOpponentPolicy::MaximumDwell,
                             CausalOpponentPolicy::MinimumDwell,
                             CausalOpponentPolicy::StatusToggle,
                         }) {
                        const AdversarialRun oracle = run_minimax_policy(
                            fixture,
                            minimaxSearch.memo,
                            policy,
                            options.trafficHistoryQuotient);
                        const AdversarialRun head = run_head_against_policy(
                            fixture,
                            policy);
                        OracleResult oracleSummary;
                        oracleSummary.valid = oracle.valid;
                        oracleSummary.score = oracle.score;
                        oracleSummary.plans = oracle.plans;
                        HeadResult headSummary;
                        headSummary.valid = head.valid;
                        headSummary.score = head.score;
                        headSummary.plans = head.plans;
                        record_summary(
                            summary,
                            oracleSummary,
                            headSummary,
                            seed ^ static_cast<std::uint64_t>(policy));
                        record_summary(
                            strata[{row.family, row.fuelProfile}],
                            oracleSummary,
                            headSummary,
                            seed ^ static_cast<std::uint64_t>(policy));
                        const std::int32_t tier = first_tier(
                            oracle.score,
                            head.score);
                        std::cout << "adversarial_case,seed=" << seed
                                  << ",family=" << row.family
                                  << ",fuel=" << row.fuelProfile
                                  << ",policy=" << policy_name(policy)
                                  << ",robust=" << score_text(root.score)
                                  << ",oracle=" << score_text(oracle.score)
                                  << ",head=" << score_text(head.score)
                                  << ",tier=" << tier
                                  << ",gain=" << tier_gain(
                                         oracle.score,
                                         head.score,
                                         tier)
                                  << ",oracle_valid=" << oracle.valid
                                  << ",head_valid=" << head.valid
                                  << ",head_deadline_days="
                                  << head.deadlineLimitedDays
                                  << ",oracle_hash=" << std::hex
                                  << plan_sequence_hash(oracle.plans)
                                  << ",head_hash="
                                  << plan_sequence_hash(head.plans)
                                  << std::dec << '\n';
                        if (options.details) {
                            const std::size_t traceDays = std::min(
                                oracle.traces.size(),
                                head.traces.size());
                            for (std::size_t traceDay = 0;
                                 traceDay < traceDays;
                                 ++traceDay) {
                                const AdversarialRun::DayTrace& oracleTrace =
                                    oracle.traces.at(traceDay);
                                const AdversarialRun::DayTrace& headTrace =
                                    head.traces.at(traceDay);
                                std::cout << "adversarial_trace,seed=" << seed
                                          << ",policy=" << policy_name(policy)
                                          << ",day=" << (traceDay + 1U)
                                          << ",oracle_daily="
                                          << oracleTrace.dailyDistinct << '/'
                                          << oracleTrace.dailyServings
                                          << ",head_daily="
                                          << headTrace.dailyDistinct << '/'
                                          << headTrace.dailyServings
                                          << ",oracle_cumulative="
                                          << score_text(oracleTrace.cumulative)
                                          << ",head_cumulative="
                                          << score_text(headTrace.cumulative)
                                          << ",oracle_pos_fuel="
                                          << oracleTrace.terminalPosition << '/'
                                          << oracleTrace.terminalFuel
                                          << ",head_pos_fuel="
                                          << headTrace.terminalPosition << '/'
                                          << headTrace.terminalFuel
                                          << ",oracle_status="
                                          << static_cast<std::int32_t>(
                                                 oracleTrace.startStatuses.at(0))
                                          << '.'
                                          << static_cast<std::int32_t>(
                                                 oracleTrace.startStatuses.at(1))
                                          << ",head_status="
                                          << static_cast<std::int32_t>(
                                                 headTrace.startStatuses.at(0))
                                          << '.'
                                          << static_cast<std::int32_t>(
                                                 headTrace.startStatuses.at(1))
                                          << ",oracle_footprint="
                                          << static_cast<std::int32_t>(
                                                 oracleTrace.ownFootprint.at(0))
                                          << '.'
                                          << static_cast<std::int32_t>(
                                                 oracleTrace.ownFootprint.at(1))
                                          << ",head_footprint="
                                          << static_cast<std::int32_t>(
                                                 headTrace.ownFootprint.at(0))
                                          << '.'
                                          << static_cast<std::int32_t>(
                                                 headTrace.ownFootprint.at(1))
                                          << ",oracle_plan="
                                          << plan_text(oracle.plans.at(traceDay))
                                          << ",head_plan="
                                          << plan_text(head.plans.at(traceDay))
                                          << ",oracle_opponent_plan="
                                          << plan_text(
                                                 oracle.opponentPlans.at(traceDay))
                                          << ",head_opponent_plan="
                                          << plan_text(
                                                 head.opponentPlans.at(traceDay))
                                          << '\n';
                            }
                        }
                    }
                    continue;
                }
                if (!options.inspectPlan.empty()) {
                    if (!options.inspectParentPlan.empty()) {
                        std::vector<udon::AgentState> agents;
                        for (const udon::CellId start :
                             fixture.config.initialAgents) {
                            agents.push_back(udon::AgentState{
                                udon::AgentKind::Patrol,
                                start,
                                fixture.config.fuelLimit,
                            });
                        }
                        const TrafficFootprint emptyFootprint{};
                        const udon::DayState state = day_state(
                            fixture.config,
                            1,
                            agents,
                            exact_road_statuses(
                                fixture,
                                1,
                                emptyFootprint,
                                emptyFootprint));
                        attribute_oracle_day(
                            fixture,
                            1,
                            state,
                            udon::MatchLedger{},
                            parse_plan_text(
                                options.inspectPlan,
                                fixture.config.agent_count()),
                            parse_plan_text(
                                options.inspectParentPlan,
                                fixture.config.agent_count()));
                        return 0;
                    }
                    inspect_exact_bundle_capability(
                        fixture,
                        parse_plan_text(
                            options.inspectPlan,
                            fixture.config.agent_count()));
                    return 0;
                }
                if (options.headOnly) {
                    const HeadResult head = solve_head(fixture);
                    const std::int32_t strictTakeoverDays =
                        static_cast<std::int32_t>(std::count_if(
                            head.audits.begin(),
                            head.audits.end(),
                            [](const udon::DecisionAudit& audit) {
                                return audit.selectionReason ==
                                    "certified-supplemental-strict-dominance";
                            }));
                    ++headOnlyCases;
                    headOnlyInvalid += head.valid ? 0 : 1;
                    std::cout << "head,seed=" << seed
                              << ",family=" << row.family
                              << ",fuel=" << row.fuelProfile
                              << ",score=" << score_text(head.score)
                              << ",valid=" << head.valid
                              << ",plan_hash=" << std::hex
                              << plan_sequence_hash(head.plans) << std::dec
                              << ",strict_takeover_days=" << strictTakeoverDays
                              << '\n';
                    if (options.details) {
                        for (std::size_t day = 0;
                             day < head.audits.size();
                             ++day) {
                            const udon::DecisionAudit& audit =
                                head.audits.at(day);
                            const auto selected = std::find_if(
                                audit.candidates.begin(),
                                audit.candidates.end(),
                                [](const udon::CandidateAuditRecord& record) {
                                    return record.selected;
                                });
                            std::cout << "head_audit,seed=" << seed
                                      << ",day=" << (day + 1U)
                                      << ",reason=" << audit.selectionReason
                                      << ",candidate_count="
                                      << audit.candidates.size();
                            if (selected != audit.candidates.end()) {
                                std::cout << ",current="
                                          << score_text(
                                                 selected->scoreAfterToday)
                                          << ",lower="
                                          << score_text(
                                                 selected
                                                     ->finalCertifiedLowerBound)
                                          << ",upper="
                                          << score_text(
                                                 selected->validUpperBound)
                                          << ",w1=" << selected->w1Role
                                          << ",disposition="
                                          << selected->disposition;
                            }
                            std::cout << '\n';
                            if (day < head.cachePaths.size()) {
                                const HeadResult::CachePathAudit& cache =
                                    head.cachePaths.at(day);
                                std::cout << "cache_path,seed=" << seed
                                          << ",day=" << (day + 1U)
                                          << ",cached=" << cache.cached
                                          << ",eligible=" << cache.eligible
                                          << ",reused=" << cache.reused
                                          << ",rejected=" << cache.rejected
                                          << ",retained=" << cache.retained
                                          << ",selected=" << cache.selected
                                          << ",cached_current="
                                          << score_text(cache.cachedCurrent)
                                          << ",cached_upper="
                                          << score_text(cache.cachedUpper)
                                          << ",selected_current="
                                          << score_text(cache.selectedCurrent)
                                          << ",selected_upper="
                                          << score_text(cache.selectedUpper)
                                          << ",source_multi="
                                          << cache.sourceMultiDayWitnesses
                                          << ",source_plans="
                                          << cache.sourceMaximumPlans
                                          << ",source_final="
                                          << score_text(cache.sourceFinal)
                                          << ",suffix_attempted="
                                          << cache.suffixAttempted
                                          << ",suffix_valid="
                                          << cache.suffixValid
                                          << ",suffix_match="
                                          << cache.suffixScoreMatches
                                          << ",suffix_replay="
                                          << score_text(
                                                 cache.suffixReplayScore)
                                          << '\n';
                            }
                        }
                    }
                    continue;
                }
                const OracleResult oracle =
                    is_three_active_patrol_experiment(row.experimentId)
                    ? solve_three_oracle(fixture)
                    : solve_oracle(fixture);
                const HeadResult head = solve_head(fixture);
                record_summary(summary, oracle, head, seed);
                record_summary(strata[{row.family, row.fuelProfile}], oracle, head, seed);
                const std::int32_t tier = first_tier(oracle.score, head.score);
                std::cout << "case,seed=" << seed
                          << ",family=" << row.family
                          << ",fuel=" << row.fuelProfile
                          << ",players=" << fixture.config.players
                          << ",agents=" << fixture.config.agent_count()
                          << ",horizon=" << fixture.config.day_count()
                          << ",oracle=" << score_text(oracle.score)
                          << ",head=" << score_text(head.score)
                          << ",tier=" << tier
                          << ",gain=" << tier_gain(oracle.score, head.score, tier)
                          << ",oracle_valid=" << oracle.valid
                          << ",head_valid=" << head.valid
                          << ",max_frontier=" << oracle.maximumFrontier
                          << ",day_enumerations=" << oracle.dailyEnumerations
                          << '\n';
                if (options.details) {
                    for (std::size_t day = 0; day < oracle.plans.size(); ++day) {
                        const std::string oracleId =
                            udon::serialize_day_plan(oracle.plans.at(day)).dump();
                        const std::string headId =
                            udon::serialize_day_plan(head.plans.at(day)).dump();
                        const udon::DecisionAudit& audit = head.audits.at(day);
                        const TerminalResourceSignature oracleTerminal =
                            terminal_resource_signature(
                                oracle.terminalAgents.at(day));
                        std::int32_t oracleAuditMatches = 0;
                        std::int32_t oracleStateMatches = 0;
                        bool oracleStateCertified = false;
                        bool oracleStateSelected = false;
                        const udon::CandidateAuditRecord* exactMatch = nullptr;
                        const udon::CandidateAuditRecord* stateMatch = nullptr;
                        const udon::CandidateAuditRecord* selected = nullptr;
                        for (const udon::CandidateAuditRecord& candidate :
                             audit.candidates) {
                            if (candidate.stableId == oracleId) {
                                ++oracleAuditMatches;
                                if (exactMatch == nullptr) {
                                    exactMatch = &candidate;
                                }
                            }
                            if (candidate.scoreAfterToday ==
                                    oracle.cumulative.at(day) &&
                                terminal_resource_signature(candidate) ==
                                    oracleTerminal) {
                                ++oracleStateMatches;
                                oracleStateCertified =
                                    oracleStateCertified || candidate.certified;
                                oracleStateSelected =
                                    oracleStateSelected || candidate.selected;
                                if (stateMatch == nullptr ||
                                    candidate.selected ||
                                    (candidate.certified &&
                                     !stateMatch->certified)) {
                                    stateMatch = &candidate;
                                }
                            }
                            if (candidate.selected) {
                                selected = &candidate;
                            }
                        }
                        std::cout << "trace,seed=" << seed
                                  << ",day=" << (day + 1U)
                                  << ",oracle_score=" << score_text(oracle.cumulative.at(day))
                                  << ",head_score=" << score_text(head.cumulative.at(day))
                                  << ",oracle_agents=" << agents_text(oracle.terminalAgents.at(day))
                                  << ",head_agents=" << agents_text(head.terminalAgents.at(day))
                                  << ",oracle_plan=" << plan_text(oracle.plans.at(day))
                                  << ",head_plan=" << plan_text(head.plans.at(day))
                                  << ",oracle_audit_matches=" << oracleAuditMatches
                                  << ",oracle_state_matches=" << oracleStateMatches
                                  << ",audit_candidates=" << audit.candidates.size()
                                  << ",selection_reason=" << audit.selectionReason
                                  << ",exact_disposition="
                                  << (exactMatch == nullptr
                                          ? "absent"
                                          : exactMatch->disposition)
                                  << ",exact_certified="
                                  << (exactMatch != nullptr && exactMatch->certified)
                                  << ",exact_selected="
                                  << (exactMatch != nullptr && exactMatch->selected)
                                  << ",state_disposition="
                                  << (stateMatch == nullptr
                                          ? "absent"
                                          : stateMatch->disposition)
                                  << ",state_certified="
                                  << oracleStateCertified
                                  << ",state_selected="
                                  << oracleStateSelected
                                  << ",state_lower="
                                  << (stateMatch == nullptr
                                          ? "0/0/0"
                                          : score_text(
                                                stateMatch->finalCertifiedLowerBound))
                                  << ",state_upper="
                                  << (stateMatch == nullptr
                                          ? "0/0/0"
                                          : score_text(stateMatch->validUpperBound))
                                  << ",selected_score="
                                  << (selected == nullptr
                                          ? "0/0/0"
                                          : score_text(selected->scoreAfterToday))
                                  << ",selected_lower="
                                  << (selected == nullptr
                                          ? "0/0/0"
                                          : score_text(
                                                selected->finalCertifiedLowerBound))
                                  << ",selected_upper="
                                  << (selected == nullptr
                                          ? "0/0/0"
                                          : score_text(selected->validUpperBound))
                                  << ",selected_terminal="
                                  << (selected == nullptr
                                          ? "none"
                                          : terminal_resource_text(*selected))
                                  << ",oracle_id=" << oracleId
                                  << ",head_id=" << headId
                                  << '\n';
                    }
                    if (!is_three_active_patrol_experiment(
                            row.experimentId)) {
                        attribute_oracle_path(fixture, oracle, head);
                        attribute_w1_continuation(fixture, oracle);
                    }
                }
            }
            if (options.maximumMatches > 0 && summary.cases >= options.maximumMatches) {
                break;
            }
        }
        if (options.headOnly) {
            std::cout << "head_summary,cases=" << headOnlyCases
                      << ",invalid=" << headOnlyInvalid
                      << '\n';
            return headOnlyInvalid == 0 ? 0 : 2;
        }
        for (const auto& [key, stratum] : strata) {
            std::cout << "stratum,family=" << key.first
                      << ",fuel=" << key.second
                      << ",cases=" << stratum.cases
                      << ",oracle_wins=" << stratum.oracleWins
                      << ",ties=" << stratum.ties
                      << ",head_wins=" << stratum.headWins
                      << ",invalid=" << stratum.invalid
                      << '\n';
        }
        std::cout << "summary,cases=" << summary.cases
                  << ",oracle_wins=" << summary.oracleWins
                  << ",ties=" << summary.ties
                  << ",head_wins=" << summary.headWins
                  << ",invalid=" << summary.invalid
                  << ",tier1=" << summary.tier1
                  << ",tier2=" << summary.tier2
                  << ",tier3=" << summary.tier3
                  << ",max_gain=" << summary.maximumGain
                  << ",hash=" << std::hex << summary.resultHash << std::dec
                  << '\n';
        return summary.invalid == 0 && summary.headWins == 0 ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "multi-patrol oracle failed: " << error.what() << '\n';
        return 1;
    }
}
