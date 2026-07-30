#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "udon/btc_protocol.hpp"
#include "udon/decision.hpp"
#include "udon/graph.hpp"
#include "udon/json.hpp"
#include "udon/orienteering.hpp"
#include "udon/planner.hpp"
#include "udon/simulator.hpp"
#include "udon/types.hpp"

namespace {

constexpr std::uint16_t kUnreachable = std::numeric_limits<std::uint16_t>::max();

struct ReplayState {
    udon::MatchConfig config;
    udon::DayState state;
};

struct AgentReachability {
    udon::AgentIndex agent = udon::kInvalidAgent;
    std::vector<std::uint16_t> maximalMasks;
    std::vector<udon::AgentPlan> witnesses;
    std::int32_t maximumSpots = 0;
    std::uint64_t settledStates = 0;
};

struct LexScore {
    std::int32_t brands = 0;
    std::int32_t servings = 0;

    [[nodiscard]] friend bool operator<(const LexScore& left, const LexScore& right) {
        return std::pair{left.brands, left.servings} <
            std::pair{right.brands, right.servings};
    }

    [[nodiscard]] friend bool operator==(const LexScore& left, const LexScore& right) = default;
};

struct TeamSolution {
    LexScore score;
    std::vector<std::uint16_t> masks;
    std::uint64_t nodes = 0;
    std::uint64_t boundPrunes = 0;
    std::uint64_t memoPrunes = 0;
};

[[nodiscard]] ReplayState load_replay(const std::string& path, std::int32_t requestedDay) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open replay: " + path);
    }

    std::optional<udon::MatchConfig> config;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const udon::JsonValue envelope = udon::JsonValue::parse(line);
        if (!envelope.is_object() || !envelope.contains("kind") ||
            !envelope.at("kind").is_string() || !envelope.contains("body")) {
            continue;
        }
        const std::string& kind = envelope.at("kind").string();
        if (kind == "setup") {
            config = udon::parse_btc_setup(envelope.at("body"));
            continue;
        }
        if (kind != "day_state" || !config.has_value()) {
            continue;
        }
        const udon::JsonValue& body = envelope.at("body");
        if (!body.is_object() || !body.contains("day") ||
            body.at("day").integer() + 1 != requestedDay) {
            continue;
        }
        const std::int64_t receivedMilliseconds = envelope.contains("atUnixMs")
            ? envelope.at("atUnixMs").integer()
            : 0;
        const auto receivedAt = std::chrono::system_clock::time_point{
            std::chrono::milliseconds(receivedMilliseconds)};
        return ReplayState{
            *config,
            udon::parse_btc_day_state(*config, body, receivedAt),
        };
    }
    throw std::runtime_error("requested setup/day_state pair was not found in replay");
}

[[nodiscard]] udon::MatchLedger load_replay_ledger(
    const std::string& path,
    std::int32_t requestedDay) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open replay: " + path);
    }
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const udon::JsonValue envelope = udon::JsonValue::parse(line);
        if (!envelope.is_object() || !envelope.contains("kind") ||
            envelope.at("kind").string() != "decision" ||
            !envelope.contains("body")) {
            continue;
        }
        const udon::JsonValue& body = envelope.at("body");
        if (!body.contains("state") || !body.contains("ledger") ||
            body.at("state").at("day").integer() != requestedDay) {
            continue;
        }
        const udon::JsonValue& serialized = body.at("ledger");
        udon::MatchLedger ledger;
        for (const udon::JsonValue& brand : serialized.at("brands").array()) {
            ledger.lifetimeBrands |= udon::brand_bit(
                static_cast<std::int32_t>(brand.integer()));
        }
        ledger.totalDailyDistinct = static_cast<std::int32_t>(
            serialized.at("totalDailyDistinct").integer());
        ledger.totalServings = static_cast<std::int32_t>(
            serialized.at("totalServings").integer());
        return ledger;
    }
    throw std::runtime_error("requested decision ledger was not found in replay");
}

[[nodiscard]] std::vector<std::uint16_t> inclusion_maximal_masks(
    const std::vector<bool>& reachable) {
    std::vector<std::uint16_t> maximal;
    const std::uint32_t maskCount = static_cast<std::uint32_t>(reachable.size());
    const std::uint32_t fullMask = maskCount - 1U;
    for (std::uint32_t mask = 0; mask < maskCount; ++mask) {
        if (!reachable.at(mask)) {
            continue;
        }
        bool dominated = false;
        const std::uint32_t complement = fullMask ^ mask;
        for (std::uint32_t added = complement; added != 0U; added = (added - 1U) & complement) {
            if (reachable.at(mask | added)) {
                dominated = true;
                break;
            }
        }
        if (!dominated) {
            maximal.push_back(static_cast<std::uint16_t>(mask));
        }
    }
    std::sort(
        maximal.begin(),
        maximal.end(),
        [](std::uint16_t left, std::uint16_t right) {
            return std::pair{std::popcount(left), left} >
                std::pair{std::popcount(right), right};
        });
    return maximal;
}

[[nodiscard]] AgentReachability exact_agent_reachability(
    const udon::MatchConfig& config,
    const udon::DayState& state,
    udon::AgentIndex agentIndex) {
    const udon::AgentState& agent = state.agents.at(static_cast<std::size_t>(agentIndex));
    if (agent.kind != udon::AgentKind::Patrol) {
        throw std::invalid_argument("orienteering reachability requires a patrol");
    }
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    if (agent.fuel < 2 * daySteps) {
        throw std::runtime_error(
            "time-only exact oracle requires patrol fuel >= 2 * daySteps");
    }
    if (config.spots.size() > 16U) {
        throw std::runtime_error("exact oracle currently supports at most 16 spots");
    }

    const std::uint32_t maskCount = std::uint32_t{1} <<
        static_cast<std::uint32_t>(config.spots.size());
    const std::uint32_t cellCount = static_cast<std::uint32_t>(config.map.cell_count());
    const std::size_t stateCount = static_cast<std::size_t>(maskCount) * cellCount;
    std::vector<std::uint16_t> distance(stateCount, kUnreachable);
    std::vector<std::uint32_t> parent(
        stateCount,
        std::numeric_limits<std::uint32_t>::max());
    std::vector<std::uint8_t> incoming(stateCount, std::numeric_limits<std::uint8_t>::max());
    using QueueEntry = std::pair<std::uint16_t, std::uint32_t>;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> queue;
    const auto state_id = [cellCount](std::uint32_t mask, udon::CellId cell) {
        return mask * cellCount + static_cast<std::uint32_t>(cell);
    };
    const auto relax = [&distance, &parent, &incoming, &queue](
                           std::uint32_t id,
                           std::uint16_t candidate,
                           std::uint32_t predecessor,
                           std::uint8_t action) {
        if (candidate >= distance.at(id)) {
            return;
        }
        distance.at(id) = candidate;
        parent.at(id) = predecessor;
        incoming.at(id) = action;
        queue.emplace(candidate, id);
    };

    const std::uint32_t root = state_id(0U, agent.position);
    relax(
        root,
        0U,
        std::numeric_limits<std::uint32_t>::max(),
        std::numeric_limits<std::uint8_t>::max());
    const udon::SpotIndex startSpot =
        config.spotAtCell.at(static_cast<std::size_t>(agent.position));
    if (startSpot != udon::kInvalidSpot && daySteps >= 1) {
        relax(
            state_id(
                std::uint32_t{1} << static_cast<std::uint32_t>(startSpot),
                agent.position),
            1U,
            root,
            static_cast<std::uint8_t>(udon::kDirectionCount));
    }

    std::vector<bool> reachableMasks(maskCount, false);
    std::vector<std::uint32_t> witnessState(
        maskCount,
        std::numeric_limits<std::uint32_t>::max());
    std::uint64_t settledStates = 0;
    while (!queue.empty()) {
        const auto [usedSteps, id] = queue.top();
        queue.pop();
        if (distance.at(id) != usedSteps) {
            continue;
        }
        ++settledStates;
        const std::uint32_t mask = id / cellCount;
        const udon::CellId cell = static_cast<udon::CellId>(id % cellCount);
        reachableMasks.at(mask) = true;
        if (witnessState.at(mask) == std::numeric_limits<std::uint32_t>::max()) {
            witnessState.at(mask) = id;
        }
        for (std::int32_t direction = 0; direction < udon::kDirectionCount; ++direction) {
            const udon::CellId destination = config.map.neighbors
                .at(static_cast<std::size_t>(cell))
                .at(static_cast<std::size_t>(direction));
            if (destination == udon::kInvalidCell ||
                config.map.terrain.at(static_cast<std::size_t>(destination)) ==
                    udon::Terrain::Pond) {
                continue;
            }
            const udon::MoveCost move = config.move_cost(
                cell,
                state.roadStatuses.at(static_cast<std::size_t>(cell)));
            const std::int32_t candidateSteps = static_cast<std::int32_t>(usedSteps) + move.steps;
            if (candidateSteps > daySteps) {
                continue;
            }
            std::uint32_t candidateMask = mask;
            const udon::SpotIndex spot =
                config.spotAtCell.at(static_cast<std::size_t>(destination));
            if (spot != udon::kInvalidSpot) {
                candidateMask |= std::uint32_t{1} << static_cast<std::uint32_t>(spot);
            }
            relax(
                state_id(candidateMask, destination),
                static_cast<std::uint16_t>(candidateSteps),
                id,
                static_cast<std::uint8_t>(direction));
        }
    }

    AgentReachability result;
    result.agent = agentIndex;
    result.maximalMasks = inclusion_maximal_masks(reachableMasks);
    result.witnesses.reserve(result.maximalMasks.size());
    result.settledStates = settledStates;
    for (const std::uint16_t mask : result.maximalMasks) {
        result.maximumSpots = std::max(result.maximumSpots, std::popcount(mask));
        std::vector<udon::PlanAction> reversed;
        std::uint32_t current = witnessState.at(mask);
        const std::uint16_t usedSteps = distance.at(current);
        while (current != root) {
            const std::uint8_t action = incoming.at(current);
            if (action == static_cast<std::uint8_t>(udon::kDirectionCount)) {
                reversed.push_back(udon::PlanAction::wait(1));
            } else if (action < static_cast<std::uint8_t>(udon::kDirectionCount)) {
                reversed.push_back(udon::PlanAction::move(action));
            } else {
                throw std::runtime_error("orienteering witness has a broken predecessor chain");
            }
            current = parent.at(current);
        }
        std::reverse(reversed.begin(), reversed.end());
        if (usedSteps < daySteps) {
            reversed.push_back(udon::PlanAction::wait(daySteps - usedSteps));
        }
        result.witnesses.push_back(std::move(reversed));
    }
    return result;
}

class ExactTeamMaster {
public:
    ExactTeamMaster(
        const udon::MatchConfig& config,
        std::vector<AgentReachability> reachability)
        : config_(config), reachability_(std::move(reachability)) {
        std::sort(
            reachability_.begin(),
            reachability_.end(),
            [](const AgentReachability& left, const AgentReachability& right) {
                return std::pair{left.maximalMasks.size(), left.agent} <
                    std::pair{right.maximalMasks.size(), right.agent};
            });
        const std::size_t count = reachability_.size();
        suffixSpots_.assign(count + 1U, 0U);
        suffixReachCount_.assign(count + 1U, {});
        memo_.resize(count + 1U);
        for (std::size_t depth = count; depth-- > 0U;) {
            suffixSpots_.at(depth) = suffixSpots_.at(depth + 1U);
            suffixReachCount_.at(depth) = suffixReachCount_.at(depth + 1U);
            std::uint16_t agentSpots = 0U;
            for (const std::uint16_t mask : reachability_.at(depth).maximalMasks) {
                agentSpots = static_cast<std::uint16_t>(agentSpots | mask);
            }
            suffixSpots_.at(depth) = static_cast<std::uint16_t>(
                suffixSpots_.at(depth) | agentSpots);
            for (std::size_t spot = 0; spot < config_.spots.size(); ++spot) {
                if ((agentSpots & (std::uint16_t{1} << spot)) != 0U) {
                    ++suffixReachCount_.at(depth).at(spot);
                }
            }
        }
        selected_.assign(count, 0U);
        best_.masks.assign(count, 0U);
    }

    [[nodiscard]] TeamSolution solve(
        bool stopOnFirstImprovement = false,
        std::int32_t stopAtServings = -1) {
        std::array<std::uint8_t, 16> counts{};
        seed_greedy(counts);
        initialScore_ = best_.score;
        stopOnFirstImprovement_ = stopOnFirstImprovement;
        stopAtServings_ = stopAtServings;
        counts.fill(0U);
        search(0U, counts, LexScore{});

        std::vector<std::uint16_t> masksByOriginalAgent(
            static_cast<std::size_t>(config_.agent_count()),
            0U);
        for (std::size_t depth = 0; depth < reachability_.size(); ++depth) {
            masksByOriginalAgent.at(
                static_cast<std::size_t>(reachability_.at(depth).agent)) =
                best_.masks.at(depth);
        }
        best_.masks = std::move(masksByOriginalAgent);
        return best_;
    }

private:
    [[nodiscard]] std::uint64_t brand_mask(std::uint16_t spots) const {
        std::uint64_t brands = 0U;
        for (std::size_t spot = 0; spot < config_.spots.size(); ++spot) {
            if ((spots & (std::uint16_t{1} << spot)) != 0U &&
                config_.spots.at(spot).stock > 0) {
                brands |= udon::brand_bit(config_.spots.at(spot).brandIndex);
            }
        }
        return brands;
    }

    [[nodiscard]] std::uint64_t brand_mask_from_counts(
        const std::array<std::uint8_t, 16>& counts) const {
        std::uint64_t brands = 0U;
        for (std::size_t spot = 0; spot < config_.spots.size(); ++spot) {
            if (counts.at(spot) > 0U && config_.spots.at(spot).stock > 0) {
                brands |= udon::brand_bit(config_.spots.at(spot).brandIndex);
            }
        }
        return brands;
    }

    [[nodiscard]] std::uint64_t count_key(
        const std::array<std::uint8_t, 16>& counts) const {
        std::uint64_t key = 0U;
        for (std::size_t spot = 0; spot < config_.spots.size(); ++spot) {
            key |= static_cast<std::uint64_t>(counts.at(spot)) << (4U * spot);
        }
        return key;
    }

    [[nodiscard]] LexScore apply_mask(
        std::uint16_t mask,
        std::array<std::uint8_t, 16>& counts,
        LexScore score) const {
        for (std::size_t spot = 0; spot < config_.spots.size(); ++spot) {
            if ((mask & (std::uint16_t{1} << spot)) == 0U) {
                continue;
            }
            const std::uint8_t capacity = static_cast<std::uint8_t>(std::min(
                config_.spots.at(spot).stock,
                static_cast<std::int32_t>(reachability_.size())));
            if (counts.at(spot) < capacity) {
                ++counts.at(spot);
                ++score.servings;
            }
        }
        score.brands = static_cast<std::int32_t>(
            std::popcount(brand_mask_from_counts(counts)));
        return score;
    }

    [[nodiscard]] LexScore optimistic_score(
        std::size_t depth,
        const std::array<std::uint8_t, 16>& counts,
        LexScore score) const {
        const std::uint64_t possibleBrands = brand_mask_from_counts(counts) |
            brand_mask(suffixSpots_.at(depth));
        score.brands = static_cast<std::int32_t>(std::popcount(possibleBrands));
        for (std::size_t spot = 0; spot < config_.spots.size(); ++spot) {
            const std::int32_t capacity = std::min(
                config_.spots.at(spot).stock,
                static_cast<std::int32_t>(reachability_.size()));
            score.servings += std::min<std::int32_t>(
                capacity - counts.at(spot),
                suffixReachCount_.at(depth).at(spot));
        }
        return score;
    }

    void seed_greedy(std::array<std::uint8_t, 16>& counts) {
        LexScore score;
        for (std::size_t depth = 0; depth < reachability_.size(); ++depth) {
            LexScore bestCandidate;
            std::uint16_t bestMask = 0U;
            bool found = false;
            for (const std::uint16_t mask : reachability_.at(depth).maximalMasks) {
                auto candidateCounts = counts;
                const LexScore candidate = apply_mask(mask, candidateCounts, score);
                if (!found || bestCandidate < candidate) {
                    found = true;
                    bestCandidate = candidate;
                    bestMask = mask;
                }
            }
            if (!found) {
                continue;
            }
            score = apply_mask(bestMask, counts, score);
            selected_.at(depth) = bestMask;
        }
        best_.score = score;
        best_.masks = selected_;
    }

    void search(
        std::size_t depth,
        const std::array<std::uint8_t, 16>& counts,
        LexScore score) {
        if (stop_) {
            return;
        }
        ++best_.nodes;
        if (optimistic_score(depth, counts, score) < best_.score ||
            optimistic_score(depth, counts, score) == best_.score) {
            ++best_.boundPrunes;
            return;
        }
        if (!memo_.at(depth).insert(count_key(counts)).second) {
            ++best_.memoPrunes;
            return;
        }
        if (depth == reachability_.size()) {
            if (best_.score < score) {
                best_.score = score;
                best_.masks = selected_;
                if (stopOnFirstImprovement_ && initialScore_ < score) {
                    stop_ = true;
                }
                if (stopAtServings_ >= 0 && score.servings >= stopAtServings_) {
                    stop_ = true;
                }
            }
            return;
        }

        struct Candidate {
            std::uint16_t mask = 0U;
            LexScore score;
        };
        std::vector<Candidate> candidates;
        candidates.reserve(reachability_.at(depth).maximalMasks.size());
        for (const std::uint16_t mask : reachability_.at(depth).maximalMasks) {
            auto candidateCounts = counts;
            candidates.push_back(Candidate{
                mask,
                apply_mask(mask, candidateCounts, score),
            });
        }
        std::sort(
            candidates.begin(),
            candidates.end(),
            [](const Candidate& left, const Candidate& right) {
                return std::pair{left.score.brands, left.score.servings} >
                    std::pair{right.score.brands, right.score.servings};
            });
        for (const Candidate& candidate : candidates) {
            auto candidateCounts = counts;
            const LexScore candidateScore = apply_mask(candidate.mask, candidateCounts, score);
            selected_.at(depth) = candidate.mask;
            search(depth + 1U, candidateCounts, candidateScore);
        }
    }

    const udon::MatchConfig& config_;
    std::vector<AgentReachability> reachability_;
    std::vector<std::uint16_t> suffixSpots_;
    std::vector<std::array<std::uint8_t, 16>> suffixReachCount_;
    std::vector<std::unordered_set<std::uint64_t>> memo_;
    std::vector<std::uint16_t> selected_;
    TeamSolution best_;
    LexScore initialScore_;
    bool stopOnFirstImprovement_ = false;
    std::int32_t stopAtServings_ = -1;
    bool stop_ = false;
};

[[nodiscard]] std::string mask_string(std::uint16_t mask, std::size_t spotCount) {
    std::string result;
    for (std::size_t spot = 0; spot < spotCount; ++spot) {
        if ((mask & (std::uint16_t{1} << spot)) == 0U) {
            continue;
        }
        if (!result.empty()) {
            result.push_back(':');
        }
        result += std::to_string(spot);
    }
    return result.empty() ? "-" : result;
}

[[nodiscard]] const udon::AgentPlan& witness_for(
    const std::vector<AgentReachability>& reachability,
    udon::AgentIndex agent,
    std::uint16_t mask) {
    const auto agentResult = std::find_if(
        reachability.begin(),
        reachability.end(),
        [agent](const AgentReachability& candidate) {
            return candidate.agent == agent;
        });
    if (agentResult == reachability.end()) {
        throw std::runtime_error("selected patrol is missing exact reachability");
    }
    const auto maskResult = std::find(
        agentResult->maximalMasks.begin(),
        agentResult->maximalMasks.end(),
        mask);
    if (maskResult == agentResult->maximalMasks.end()) {
        throw std::runtime_error("selected mask is missing an exact witness");
    }
    return agentResult->witnesses.at(static_cast<std::size_t>(
        std::distance(agentResult->maximalMasks.begin(), maskResult)));
}

}

int main(int argumentCount, char** arguments) {
    try {
        std::string replayPath;
        std::int32_t day = 1;
        bool reachOnly = false;
        bool productionReachOnly = false;
        bool productionCheck = false;
        bool engineCheck = false;
        bool replayLedger = false;
        std::int32_t budgetMilliseconds = 5000;
        bool improveOnly = false;
        std::int32_t stopServings = -1;
        std::int32_t feasibleServings = -1;
        for (int argument = 1; argument < argumentCount; ++argument) {
            const std::string value = arguments[argument];
            if (value == "--replay" && argument + 1 < argumentCount) {
                replayPath = arguments[++argument];
            } else if (value == "--day" && argument + 1 < argumentCount) {
                day = std::stoi(arguments[++argument]);
            } else if (value == "--reach-only") {
                reachOnly = true;
            } else if (value == "--production-reach-only") {
                productionReachOnly = true;
            } else if (value == "--production-check") {
                productionCheck = true;
            } else if (value == "--engine-check") {
                engineCheck = true;
            } else if (value == "--replay-ledger") {
                replayLedger = true;
            } else if (value == "--budget-ms" && argument + 1 < argumentCount) {
                budgetMilliseconds = std::stoi(arguments[++argument]);
            } else if (value == "--improve-only") {
                improveOnly = true;
            } else if (value == "--stop-servings" && argument + 1 < argumentCount) {
                stopServings = std::stoi(arguments[++argument]);
            } else if (value == "--feasible-servings" && argument + 1 < argumentCount) {
                feasibleServings = std::stoi(arguments[++argument]);
            } else {
                throw std::invalid_argument(
                    "usage: udonshield_orienteering_oracle --replay PATH [--day N] [--reach-only] [--production-reach-only] [--production-check] [--engine-check] [--budget-ms N] [--improve-only] [--stop-servings N] [--feasible-servings N]");
            }
        }
        if (replayPath.empty() || day <= 0) {
            throw std::invalid_argument(
                "usage: udonshield_orienteering_oracle --replay PATH [--day N] [--reach-only] [--production-reach-only] [--production-check] [--engine-check] [--budget-ms N] [--improve-only] [--stop-servings N] [--feasible-servings N]");
        }

        const ReplayState replay = load_replay(replayPath, day);
        if (engineCheck) {
            udon::DeadlineCalibration calibration;
            calibration.version = "btc-http-fair-w1-v3-guarded";
            calibration.networkFloor = std::chrono::milliseconds{1100};
            calibration.networkPercent = 20;
            calibration.certificationPercent = 20;
            udon::UdonShieldEngine engine(
                replay.config,
                {},
                calibration,
                udon::RoutePoolSearch::SinglePass,
                7,
                true,
                7);
            const udon::DecisionResult decision = engine.solve_day(
                replay.state,
                load_replay_ledger(replayPath, day),
                std::chrono::milliseconds{budgetMilliseconds});
            std::cout << "schema=udon-shield-engine-orienteering-check-v1"
                      << ",day=" << day
                      << ",score=" << decision.candidate.scoreAfterToday.lifetimeDistinct
                      << '/' << decision.candidate.scoreAfterToday.totalDailyDistinct
                      << '/' << decision.candidate.scoreAfterToday.totalServings
                      << ",exact_generated="
                      << decision.audit.columnGeneration.exactOrienteeringBundles
                      << ",exact_discovered="
                      << decision.diagnostics.exactBundlesDiscovered
                      << ",exact_evaluated="
                      << decision.diagnostics.exactBundlesEvaluated
                      << ",exact_accepted="
                      << decision.diagnostics.exactBundlesAccepted
                      << ",exact_score="
                      << decision.diagnostics.bestExactBundleScore.lifetimeDistinct
                      << '/' << decision.diagnostics.bestExactBundleScore.totalDailyDistinct
                      << '/' << decision.diagnostics.bestExactBundleScore.totalServings
                      << ",elapsed_ms=" << decision.timing.total.count()
                      << '\n';
            return EXIT_SUCCESS;
        }
        if (productionReachOnly) {
            const auto reachabilityStarted = std::chrono::steady_clock::now();
            std::int32_t patrols = 0;
            for (udon::AgentIndex agent = 0;
                 agent < static_cast<udon::AgentIndex>(replay.state.agents.size());
                 ++agent) {
                if (replay.state.agents.at(static_cast<std::size_t>(agent)).kind !=
                    udon::AgentKind::Patrol) {
                    continue;
                }
                const udon::ExactOrienteeringReachability exact =
                    udon::enumerate_exact_high_fuel_routes(
                        replay.config,
                        replay.state,
                        agent);
                const auto agentFinished = std::chrono::steady_clock::now();
                std::cout << "agent=" << agent
                          << ",complete=" << (exact.complete ? 1 : 0)
                          << ",maximal_masks=" << exact.maximalRoutes.size()
                          << ",terminal_variants=" << exact.terminalVariants.size()
                          << ",settled_states=" << exact.settledStates
                          << ",cumulative_ms="
                          << std::chrono::duration_cast<std::chrono::milliseconds>(
                                 agentFinished - reachabilityStarted).count()
                          << '\n';
                ++patrols;
            }
            const std::int64_t milliseconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - reachabilityStarted).count();
            std::cout << "schema=udon-shield-production-orienteering-reachability-v1"
                      << ",day=" << day
                      << ",spots=" << replay.config.spots.size()
                      << ",patrols=" << patrols
                      << ",milliseconds=" << milliseconds
                      << '\n';
            return EXIT_SUCCESS;
        }
        if (productionCheck) {
            const udon::ParetoRouter router(replay.config);
            const udon::RouteColumnGenerator generator(replay.config, router);
            udon::ColumnGenerationOptions options;
            options.maximumPathsPerTarget = 4;
            options.maximumColumnsPerAgent = 16;
            options.maximumTargetSpots = 12;
            options.maximumEscorts = 16;
            options.enableHarvestExtensions = true;
            options.allowUncachedHarvestTargets = true;
            options.enableHarvestOrienteering = true;
            options.enableExactHarvestOrienteering = true;
            options.maximumHarvestExtensionSources = 4;
            options.maximumHarvestExtensionDepth = 4;
            const udon::MatchLedger checkLedger = replayLedger
                ? load_replay_ledger(replayPath, day)
                : udon::MatchLedger{};
            const udon::RoutePortfolio portfolio = generator.generate(
                replay.state,
                checkLedger,
                options);
            const udon::ExactStepSimulator simulator(replay.config);
            const udon::IndependentDayValidator validator(replay.config);
            const udon::RouteMaster master(replay.config, simulator, validator);
            std::map<std::int32_t, std::vector<const udon::RouteColumn*>> bundles;
            for (udon::AgentIndex agent = 0;
                 agent < static_cast<udon::AgentIndex>(replay.state.agents.size());
                 ++agent) {
                for (const udon::RouteColumn& column :
                     portfolio.columnsByAgent.at(static_cast<std::size_t>(agent))) {
                    if (!column.exactOrienteering) {
                        continue;
                    }
                    std::vector<const udon::RouteColumn*>& bundle =
                        bundles[column.contingencyBundle];
                    if (bundle.empty()) {
                        bundle.resize(replay.state.agents.size(), nullptr);
                    }
                    bundle.at(static_cast<std::size_t>(agent)) = &column;
                }
            }
            for (const auto& [bundleId, columns] : bundles) {
                if (std::any_of(
                        columns.begin(),
                        columns.end(),
                        [](const udon::RouteColumn* column) { return column == nullptr; })) {
                    throw std::runtime_error("production exact bundle is incomplete");
                }
                udon::DayPlan plan;
                plan.actions.resize(replay.state.agents.size());
                std::int32_t terminalFuel = 0;
                for (udon::AgentIndex agent = 0;
                     agent < static_cast<udon::AgentIndex>(replay.state.agents.size());
                     ++agent) {
                    const udon::RouteColumn& column =
                        *columns.at(static_cast<std::size_t>(agent));
                    plan.actions.at(static_cast<std::size_t>(agent)) = column.actions;
                    terminalFuel += column.terminalFuel;
                }
                const udon::SimulationResult simulation = simulator.simulate(
                    replay.state,
                    plan,
                    false);
                const udon::SimulationResult tracedSimulation = simulator.simulate(
                    replay.state,
                    plan,
                    true);
                if (!simulation.valid) {
                    throw std::runtime_error("production exact bundle is invalid in simulator");
                }
                const std::optional<udon::MasterCandidate> evaluated =
                    master.evaluate_exact_plan(
                        replay.state,
                        checkLedger,
                        plan);
                if (!evaluated.has_value()) {
                    throw std::runtime_error("production exact bundle was rejected by RouteMaster");
                }
                std::cout << "schema=udon-shield-production-orienteering-check-v2"
                          << ",bundle=" << bundleId
                          << ",brands=" << simulation.score.dailyDistinct
                          << ",servings=" << simulation.score.servings
                          << ",traced=" << tracedSimulation.score.dailyDistinct
                          << '/' << tracedSimulation.score.servings
                          << ",evaluated_day="
                          << evaluated->simulation.score.dailyDistinct
                          << '/' << evaluated->simulation.score.servings
                          << ",direct_score="
                          << udon::OfficialScore::after_day(
                                 checkLedger,
                                 tracedSimulation.score).lifetimeDistinct
                          << '/' << udon::OfficialScore::after_day(
                                 checkLedger,
                                 tracedSimulation.score).totalDailyDistinct
                          << '/' << udon::OfficialScore::after_day(
                                 checkLedger,
                                 tracedSimulation.score).totalServings
                          << ",master_score="
                          << evaluated->scoreAfterToday.lifetimeDistinct
                          << '/' << evaluated->scoreAfterToday.totalDailyDistinct
                          << '/' << evaluated->scoreAfterToday.totalServings
                          << ",terminal_fuel=" << terminalFuel
                          << '\n';
            }
            udon::MasterOptions masterOptions;
            masterOptions.maximumCombinations = 1;
            masterOptions.maximumCandidates = 16;
            udon::MasterDiagnostics masterDiagnostics;
            static_cast<void>(master.solve(
                replay.state,
                checkLedger,
                portfolio,
                masterOptions,
                masterDiagnostics));
            std::cout << "schema=udon-shield-production-orienteering-master-check-v1"
                      << ",exact_discovered="
                      << masterDiagnostics.exactBundlesDiscovered
                      << ",exact_accepted="
                      << masterDiagnostics.exactBundlesAccepted
                      << ",exact_score="
                      << masterDiagnostics.bestExactBundleScore.lifetimeDistinct
                      << '/' << masterDiagnostics.bestExactBundleScore.totalDailyDistinct
                      << '/' << masterDiagnostics.bestExactBundleScore.totalServings
                      << '\n';
            return EXIT_SUCCESS;
        }
        std::vector<AgentReachability> reachability;
        const auto reachabilityStarted = std::chrono::steady_clock::now();
        for (udon::AgentIndex agent = 0;
             agent < static_cast<udon::AgentIndex>(replay.state.agents.size());
             ++agent) {
            if (replay.state.agents.at(static_cast<std::size_t>(agent)).kind !=
                udon::AgentKind::Patrol) {
                continue;
            }
            AgentReachability exact = exact_agent_reachability(
                replay.config,
                replay.state,
                agent);
            const auto agentFinished = std::chrono::steady_clock::now();
            std::cout << "agent=" << agent
                      << ",max_spots=" << exact.maximumSpots
                      << ",maximal_masks=" << exact.maximalMasks.size()
                      << ",settled_states=" << exact.settledStates
                      << ",cumulative_ms="
                      << std::chrono::duration_cast<std::chrono::milliseconds>(
                             agentFinished - reachabilityStarted).count()
                      << '\n';
            reachability.push_back(std::move(exact));
        }
        const std::int64_t reachabilityMilliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - reachabilityStarted).count();
        if (reachOnly) {
            std::cout << "schema=udon-shield-orienteering-reachability-v1"
                      << ",day=" << day
                      << ",spots=" << replay.config.spots.size()
                      << ",patrols=" << reachability.size()
                      << ",milliseconds=" << reachabilityMilliseconds
                      << '\n';
            return EXIT_SUCCESS;
        }
        if (feasibleServings >= 0) {
            for (AgentReachability& agent : reachability) {
                std::sort(
                    agent.maximalMasks.begin(),
                    agent.maximalMasks.end(),
                    [&replay](std::uint16_t left, std::uint16_t right) {
                        const auto rank = [&replay](std::uint16_t mask) {
                            std::int32_t scarcity = 0;
                            for (std::size_t spot = 0;
                                 spot < replay.config.spots.size();
                                 ++spot) {
                                if ((mask & (std::uint16_t{1} << spot)) != 0U) {
                                    scarcity += 10000 / std::max(
                                        1,
                                        replay.config.spots.at(spot).stock);
                                }
                            }
                            return std::pair{std::popcount(mask), -scarcity};
                        };
                        return rank(left) > rank(right);
                    });
            }
            std::vector<std::size_t> ordering(reachability.size());
            for (std::size_t index = 0; index < ordering.size(); ++index) {
                ordering.at(index) = index;
            }
            std::sort(
                ordering.begin(),
                ordering.end(),
                [&reachability](std::size_t left, std::size_t right) {
                    return reachability.at(left).maximalMasks.size() <
                        reachability.at(right).maximalMasks.size();
                });
            std::vector<std::int32_t> suffixMaximum(ordering.size() + 1U, 0);
            std::vector<std::uint64_t> suffixBrands(ordering.size() + 1U, 0U);
            const auto brands_for = [&replay](std::uint16_t mask) {
                std::uint64_t brands = 0U;
                for (std::size_t spot = 0; spot < replay.config.spots.size(); ++spot) {
                    if ((mask & (std::uint16_t{1} << spot)) != 0U &&
                        replay.config.spots.at(spot).stock > 0) {
                        brands |= udon::brand_bit(replay.config.spots.at(spot).brandIndex);
                    }
                }
                return brands;
            };
            for (std::size_t depth = ordering.size(); depth-- > 0U;) {
                suffixMaximum.at(depth) = suffixMaximum.at(depth + 1U);
                suffixBrands.at(depth) = suffixBrands.at(depth + 1U);
                const AgentReachability& agent = reachability.at(ordering.at(depth));
                for (const std::uint16_t mask : agent.maximalMasks) {
                    suffixMaximum.at(depth) = std::max(
                        suffixMaximum.at(depth),
                        suffixMaximum.at(depth + 1U) + std::popcount(mask));
                    suffixBrands.at(depth) |= brands_for(mask);
                }
            }
            std::array<std::uint8_t, 16> counts{};
            std::vector<std::uint16_t> choices(ordering.size(), 0U);
            std::uint64_t nodes = 0;
            const auto started = std::chrono::steady_clock::now();
            const auto search = [&](auto&& self,
                                    std::size_t depth,
                                    std::int32_t servings,
                                    std::uint64_t brands) -> bool {
                ++nodes;
                if (servings + suffixMaximum.at(depth) < feasibleServings ||
                    static_cast<std::int32_t>(std::popcount(
                        brands | suffixBrands.at(depth))) < replay.config.brand_count()) {
                    return false;
                }
                if (depth == ordering.size()) {
                    return servings >= feasibleServings &&
                        static_cast<std::int32_t>(std::popcount(brands)) ==
                            replay.config.brand_count();
                }
                const AgentReachability& agent = reachability.at(ordering.at(depth));
                for (const std::uint16_t mask : agent.maximalMasks) {
                    bool feasible = true;
                    for (std::size_t spot = 0; spot < replay.config.spots.size(); ++spot) {
                        if ((mask & (std::uint16_t{1} << spot)) != 0U &&
                            counts.at(spot) >= replay.config.spots.at(spot).stock) {
                            feasible = false;
                            break;
                        }
                    }
                    if (!feasible) {
                        continue;
                    }
                    for (std::size_t spot = 0; spot < replay.config.spots.size(); ++spot) {
                        if ((mask & (std::uint16_t{1} << spot)) != 0U) {
                            ++counts.at(spot);
                        }
                    }
                    choices.at(depth) = mask;
                    if (self(
                            self,
                            depth + 1U,
                            servings + std::popcount(mask),
                            brands | brands_for(mask))) {
                        return true;
                    }
                    for (std::size_t spot = 0; spot < replay.config.spots.size(); ++spot) {
                        if ((mask & (std::uint16_t{1} << spot)) != 0U) {
                            --counts.at(spot);
                        }
                    }
                }
                return false;
            };
            const bool found = search(search, 0U, 0, 0U);
            const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - started).count();
            std::cout << "schema=udon-shield-capacity-feasibility-v1"
                      << ",target=" << feasibleServings
                      << ",found=" << (found ? 1 : 0)
                      << ",nodes=" << nodes
                      << ",milliseconds=" << milliseconds
                      << '\n';
            if (found) {
                for (std::size_t depth = 0; depth < ordering.size(); ++depth) {
                    std::cout << "feasible_agent="
                              << reachability.at(ordering.at(depth)).agent
                              << ",spots=" << mask_string(
                                  choices.at(depth),
                                  replay.config.spots.size())
                              << '\n';
                }
            }
            return found ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        const auto masterStarted = std::chrono::steady_clock::now();
        ExactTeamMaster master(replay.config, reachability);
        const TeamSolution solution = master.solve(improveOnly, stopServings);
        const std::int64_t masterMilliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - masterStarted).count();
        udon::DayPlan witnessPlan;
        witnessPlan.actions.resize(replay.state.agents.size());
        for (udon::AgentIndex agent = 0;
             agent < static_cast<udon::AgentIndex>(replay.state.agents.size());
             ++agent) {
            if (replay.state.agents.at(static_cast<std::size_t>(agent)).kind ==
                udon::AgentKind::Patrol) {
                witnessPlan.actions.at(static_cast<std::size_t>(agent)) = witness_for(
                    reachability,
                    agent,
                    solution.masks.at(static_cast<std::size_t>(agent)));
            } else {
                witnessPlan.actions.at(static_cast<std::size_t>(agent)).push_back(
                    udon::PlanAction::wait(replay.config.steps_for_day(replay.state.dayNumber)));
            }
        }
        const udon::ExactStepSimulator simulator(replay.config);
        const udon::SimulationResult verification = simulator.simulate(
            replay.state,
            witnessPlan,
            false);
        if (!verification.valid ||
            verification.score.dailyDistinct != solution.score.brands ||
            verification.score.servings != solution.score.servings) {
            throw std::runtime_error("exact master witness disagrees with official simulator");
        }
        std::cout << "schema=udon-shield-orienteering-oracle-v1"
                  << ",day=" << day
                  << ",spots=" << replay.config.spots.size()
                  << ",patrols=" << reachability.size()
                  << ",exact_brands=" << solution.score.brands
                  << ",exact_servings=" << solution.score.servings
                  << ",master_nodes=" << solution.nodes
                  << ",bound_prunes=" << solution.boundPrunes
                  << ",memo_prunes=" << solution.memoPrunes
                  << ",reachability_ms=" << reachabilityMilliseconds
                  << ",master_ms=" << masterMilliseconds
                  << ",simulator_verified=1"
                  << '\n';
        for (std::size_t agent = 0; agent < solution.masks.size(); ++agent) {
            if (replay.state.agents.at(agent).kind != udon::AgentKind::Patrol) {
                continue;
            }
            std::cout << "selected_agent=" << agent
                      << ",spots=" << mask_string(
                          solution.masks.at(agent),
                          replay.config.spots.size())
                      << '\n';
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
