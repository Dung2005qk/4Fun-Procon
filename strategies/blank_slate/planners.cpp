#include "planners.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "udon/protocol.hpp"

namespace udon::blank_slate {

namespace {

struct RendezvousWindow {
    CellId cell = kInvalidCell;
    std::int32_t startsAt = 0;
    std::int32_t endsAt = 0;
    bool requiresRefuelStep = false;
};

struct HubPlan {
    AgentIndex tanker = kInvalidAgent;
    AgentPlan actions;
    std::vector<RendezvousWindow> windows;
    std::int32_t centrality = 0;
};

struct BuiltRoute {
    AgentPlan actions;
    std::vector<SpotIndex> reached;
    CellId terminalCell = kInvalidCell;
    std::int32_t terminalFuel = 0;
    std::int32_t elapsed = 0;
};

struct RouteOption {
    AgentPlan actions;
    std::vector<SpotIndex> reached;
    BrandMask brands;
    std::int64_t localValue = 0;
};

struct Evaluation {
    DayPlan plan;
    SimulationResult simulation;
    OfficialScore score;
    std::int64_t terminalFuel = 0;
    std::int32_t futureDistance = std::numeric_limits<std::int32_t>::max();
    std::string stableId;
    bool valid = false;
};

struct CombinationNode {
    std::vector<std::int32_t> routeIndices;
    std::int64_t estimate = 0;
};

struct CombinationOrder {
    [[nodiscard]] bool operator()(
        const CombinationNode& left,
        const CombinationNode& right) const {
        if (left.estimate != right.estimate) {
            return left.estimate < right.estimate;
        }
        return left.routeIndices > right.routeIndices;
    }
};

struct MacroAction {
    AgentIndex agent = kInvalidAgent;
    std::int32_t value = -1;
    std::int64_t priority = 0;
};

struct MacroState {
    std::int32_t hubIndex = -1;
    std::vector<std::vector<SpotIndex>> sequences;
    std::vector<std::int32_t> assignedBySpot;
    std::int32_t depth = 0;
};

struct MctsNode {
    MacroState state;
    std::int32_t parent = -1;
    std::vector<std::int32_t> children;
    std::vector<MacroAction> untried;
    std::int32_t visits = 0;
    std::array<long double, 3> totalScore{};
};

void merge_diagnostics(
    Diagnostics& target,
    const Diagnostics& addition) {
    target.hubsConsidered += addition.hubsConsidered;
    target.routesGenerated += addition.routesGenerated;
    target.plansEvaluated += addition.plansEvaluated;
    target.incumbentImprovements += addition.incumbentImprovements;
    target.resourceConflicts += addition.resourceConflicts;
    target.searchNodes += addition.searchNodes;
    target.rollouts += addition.rollouts;
    target.deadlineReached = target.deadlineReached || addition.deadlineReached;
}

[[nodiscard]] AgentPlan wait_actions(
    const MatchConfig& config,
    const DayState& state) {
    return AgentPlan{PlanAction::wait(config.steps_for_day(state.dayNumber))};
}

[[nodiscard]] DayPlan wait_plan(
    const MatchConfig& config,
    const DayState& state) {
    DayPlan plan;
    plan.actions.reserve(static_cast<std::size_t>(config.agent_count()));
    for (AgentIndex agentIndex = 0; agentIndex < config.agent_count(); ++agentIndex) {
        plan.actions.push_back(wait_actions(config, state));
    }
    return plan;
}

[[nodiscard]] std::int32_t grid_distance(
    const MatchConfig& config,
    CellId left,
    CellId right) {
    const std::int32_t leftRow = config.map.row_of(left);
    const std::int32_t leftColumn = config.map.column_of(left);
    const std::int32_t rightRow = config.map.row_of(right);
    const std::int32_t rightColumn = config.map.column_of(right);
    return std::abs(leftRow - rightRow) + std::abs(leftColumn - rightColumn);
}

[[nodiscard]] std::int64_t mix_seed(
    const MatchConfig& config,
    const DayState& state,
    Method method) {
    std::uint64_t value = static_cast<std::uint64_t>(state.dayNumber) * 0x9E3779B97F4A7C15ULL;
    value ^= static_cast<std::uint64_t>(config.map.cell_count()) << 32U;
    value ^= static_cast<std::uint64_t>(config.spots.size()) * 0xBF58476D1CE4E5B9ULL;
    value ^= static_cast<std::uint64_t>(method) * 0x94D049BB133111EBULL;
    for (const AgentState& agent : state.agents) {
        value ^= static_cast<std::uint64_t>(agent.position + 1) * 0xD6E8FEB86659FD93ULL;
        value = (value << 13U) | (value >> 51U);
    }
    return static_cast<std::int64_t>(value);
}

[[nodiscard]] bool better_evaluation(
    const Evaluation& left,
    const Evaluation& right) {
    if (!left.valid) {
        return false;
    }
    if (!right.valid) {
        return true;
    }
    const std::int32_t scoreOrder = compare_lexicographic(left.score, right.score);
    if (scoreOrder != 0) {
        return scoreOrder > 0;
    }
    if (left.terminalFuel != right.terminalFuel) {
        return left.terminalFuel > right.terminalFuel;
    }
    if (left.futureDistance != right.futureDistance) {
        return left.futureDistance < right.futureDistance;
    }
    return left.stableId < right.stableId;
}

[[nodiscard]] std::int32_t future_distance(
    const MatchConfig& config,
    const MatchLedger& ledger,
    const SimulationResult& simulation) {
    const BrandMask collected = ledger.lifetimeBrands | simulation.score.brands;
    std::int32_t total = 0;
    for (std::int32_t brand = 0; brand < config.brand_count(); ++brand) {
        if (has_brand(collected, brand)) {
            continue;
        }
        std::int32_t best = 1000000;
        for (const Spot& spot : config.spots) {
            if (spot.brandIndex != brand) {
                continue;
            }
            for (const AgentState& agent : simulation.finalAgents) {
                if (agent.kind != AgentKind::Patrol) {
                    continue;
                }
                best = std::min(best, grid_distance(config, agent.position, spot.position));
            }
        }
        total += best;
    }
    return total;
}

[[nodiscard]] Evaluation evaluate_plan(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    const DayPlan& plan,
    const ExactStepSimulator& simulator,
    const IndependentDayValidator& validator,
    Diagnostics& diagnostics,
    bool validateIncumbent,
    const Evaluation& incumbent) {
    ++diagnostics.plansEvaluated;
    Evaluation evaluation;
    evaluation.plan = plan;
    evaluation.simulation = simulator.simulate(state, plan, false);
    if (!evaluation.simulation.valid) {
        return evaluation;
    }
    evaluation.score = OfficialScore::after_day(ledger, evaluation.simulation.score);
    evaluation.stableId = canonical_plan_bytes(plan);
    for (const AgentState& agent : evaluation.simulation.finalAgents) {
        if (agent.kind == AgentKind::Patrol) {
            evaluation.terminalFuel += agent.fuel;
        }
    }
    evaluation.futureDistance = future_distance(config, ledger, evaluation.simulation);
    evaluation.valid = true;
    if (validateIncumbent && better_evaluation(evaluation, incumbent)) {
        const SimulationResult independent = validator.validate(state, plan, false);
        std::string mismatch;
        if (!validator.agrees_with(evaluation.simulation, independent, mismatch)) {
            throw std::runtime_error("blank-slate incumbent disagrees with validator: " + mismatch);
        }
    }
    return evaluation;
}

[[nodiscard]] std::optional<ParetoPath> shortest_path(
    ParetoRouter& router,
    const DayState& state,
    CellId source,
    CellId target,
    std::int32_t maximumSteps,
    std::int32_t maximumFuel,
    bool patrol,
    std::chrono::steady_clock::time_point deadline) {
    if (source == target) {
        return ParetoPath{};
    }
    if (maximumSteps <= 0 || (patrol && maximumFuel < 0)) {
        return std::nullopt;
    }
    ParetoSearchOptions options;
    options.maximumTravelSteps = maximumSteps;
    options.maximumPatrolFuel = patrol
        ? maximumFuel
        : std::numeric_limits<std::int32_t>::max() / 4;
    options.maximumLabelsPerCell = 8;
    options.maximumPaths = 1;
    options.patrol = patrol;
    options.deadline = deadline;
    std::vector<ParetoPath> paths = router.find_paths(
        source,
        target,
        state.roadStatuses,
        options);
    if (paths.empty()) {
        return std::nullopt;
    }
    return paths.front();
}

void append_path(
    BuiltRoute& route,
    const ParetoPath& path,
    bool patrol) {
    for (const std::int32_t direction : path.directions) {
        route.actions.push_back(PlanAction::move(direction));
    }
    route.elapsed += path.travelSteps;
    if (patrol) {
        route.terminalFuel -= path.patrolFuel;
    }
}

[[nodiscard]] bool rendezvous_refuel(
    const MatchConfig& config,
    const DayState& state,
    ParetoRouter& router,
    const HubPlan& hub,
    BuiltRoute& route,
    std::chrono::steady_clock::time_point deadline) {
    if (hub.windows.empty()) {
        return false;
    }
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    struct Choice {
        ParetoPath path;
        RendezvousWindow window;
        std::int32_t completion = 0;
    };
    std::optional<Choice> best;
    for (const RendezvousWindow& window : hub.windows) {
        const std::optional<ParetoPath> path = shortest_path(
            router,
            state,
            route.terminalCell,
            window.cell,
            daySteps - route.elapsed,
            route.terminalFuel,
            true,
            deadline);
        if (!path.has_value()) {
            continue;
        }
        std::int32_t completion = std::max(
            route.elapsed + path->travelSteps,
            window.startsAt);
        if (completion == 0) {
            completion = 1;
        }
        if (completion > window.endsAt || completion > daySteps) {
            continue;
        }
        if (!best.has_value() ||
            std::tie(completion, path->patrolFuel, window.cell) <
                std::tie(
                    best->completion,
                    best->path.patrolFuel,
                    best->window.cell)) {
            best = Choice{*path, window, completion};
        }
    }
    if (!best.has_value()) {
        return false;
    }
    append_path(route, best->path, true);
    route.terminalCell = best->window.cell;
    const std::int32_t refuelStep =
        best->completion + (best->window.requiresRefuelStep ? 1 : 0);
    if (refuelStep > best->window.endsAt || refuelStep > daySteps) {
        return false;
    }
    const std::int32_t wait = refuelStep - route.elapsed;
    if (wait > 0) {
        route.actions.push_back(PlanAction::wait(wait));
        route.elapsed += wait;
    }
    route.terminalFuel = config.fuelLimit;
    return true;
}

[[nodiscard]] BuiltRoute build_patrol_route(
    const MatchConfig& config,
    const DayState& state,
    ParetoRouter& router,
    AgentIndex agentIndex,
    const std::vector<SpotIndex>& targets,
    const HubPlan& hub,
    std::chrono::steady_clock::time_point deadline) {
    const AgentState& agent = state.agents.at(static_cast<std::size_t>(agentIndex));
    BuiltRoute route;
    route.terminalCell = agent.position;
    route.terminalFuel = agent.fuel;
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    std::set<SpotIndex> visited;
    for (const SpotIndex targetIndex : targets) {
        if (targetIndex < 0 ||
            targetIndex >= static_cast<SpotIndex>(config.spots.size()) ||
            !visited.insert(targetIndex).second) {
            continue;
        }
        const CellId target = config.spots.at(static_cast<std::size_t>(targetIndex)).position;
        std::optional<ParetoPath> direct = shortest_path(
            router,
            state,
            route.terminalCell,
            target,
            daySteps - route.elapsed,
            route.terminalFuel,
            true,
            deadline);
        bool refuelFirst = !direct.has_value();
        if (direct.has_value() && !hub.windows.empty()) {
            std::int32_t minimumFuelToWindow =
                std::numeric_limits<std::int32_t>::max();
            for (const RendezvousWindow& window : hub.windows) {
                const std::int32_t targetArrival =
                    route.elapsed + direct->travelSteps;
                const std::optional<ParetoPath> targetToWindow = shortest_path(
                    router,
                    state,
                    target,
                    window.cell,
                    daySteps - targetArrival,
                    std::numeric_limits<std::int32_t>::max() / 4,
                    true,
                    deadline);
                if (!targetToWindow.has_value() ||
                    targetArrival + targetToWindow->travelSteps > window.endsAt) {
                    continue;
                }
                minimumFuelToWindow = std::min(
                    minimumFuelToWindow,
                    targetToWindow->patrolFuel);
            }
            refuelFirst =
                minimumFuelToWindow != std::numeric_limits<std::int32_t>::max() &&
                direct->patrolFuel + minimumFuelToWindow > route.terminalFuel;
        }
        if (refuelFirst) {
            if (!rendezvous_refuel(
                    config,
                    state,
                    router,
                    hub,
                    route,
                    deadline)) {
                break;
            }
            direct = shortest_path(
                router,
                state,
                route.terminalCell,
                target,
                daySteps - route.elapsed,
                route.terminalFuel,
                true,
                deadline);
        }
        if (!direct.has_value()) {
            break;
        }
        append_path(route, *direct, true);
        route.terminalCell = target;
        if (direct->travelSteps == 0 && route.elapsed < daySteps) {
            route.actions.push_back(PlanAction::wait(1));
            ++route.elapsed;
        }
        route.reached.push_back(targetIndex);
    }
    if (route.elapsed < daySteps) {
        route.actions.push_back(PlanAction::wait(daySteps - route.elapsed));
        route.elapsed = daySteps;
    }
    if (route.actions.empty()) {
        route.actions = wait_actions(config, state);
        route.elapsed = daySteps;
    }
    return route;
}

[[nodiscard]] std::vector<HubPlan> hub_plans(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    ParetoRouter& router,
    std::int32_t limit,
    std::chrono::steady_clock::time_point deadline,
    bool mobileOnly = false) {
    AgentIndex tanker = kInvalidAgent;
    for (AgentIndex agentIndex = 0; agentIndex < config.agent_count(); ++agentIndex) {
        if (state.agents.at(static_cast<std::size_t>(agentIndex)).kind == AgentKind::Tanker) {
            tanker = agentIndex;
            break;
        }
    }
    if (tanker == kInvalidAgent) {
        if (mobileOnly) {
            return {};
        }
        return {HubPlan{}};
    }
    std::vector<CellId> cells;
    cells.push_back(state.agents.at(static_cast<std::size_t>(tanker)).position);
    for (const Spot& spot : config.spots) {
        cells.push_back(spot.position);
    }
    for (const AgentState& agent : state.agents) {
        cells.push_back(agent.position);
    }
    std::sort(cells.begin(), cells.end());
    cells.erase(std::unique(cells.begin(), cells.end()), cells.end());
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    const CellId tankerStart =
        state.agents.at(static_cast<std::size_t>(tanker)).position;
    struct HubCandidate {
        CellId cell = kInvalidCell;
        ParetoPath fromStart;
        std::int32_t centrality = 0;
    };
    std::vector<HubCandidate> candidates;
    for (const CellId cell : cells) {
        const std::optional<ParetoPath> path = shortest_path(
            router,
            state,
            tankerStart,
            cell,
            daySteps,
            0,
            false,
            deadline);
        if (!path.has_value()) {
            continue;
        }
        HubCandidate candidate;
        candidate.cell = cell;
        candidate.fromStart = *path;
        for (const Spot& spot : config.spots) {
            const std::int32_t weight = has_brand(ledger.lifetimeBrands, spot.brandIndex) ? 1 : 4;
            candidate.centrality += weight * grid_distance(config, cell, spot.position);
        }
        candidate.centrality += path->travelSteps * 8;
        candidates.push_back(std::move(candidate));
    }
    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const HubCandidate& left, const HubCandidate& right) {
            if (left.centrality != right.centrality) {
                return left.centrality < right.centrality;
            }
            if (left.fromStart.travelSteps != right.fromStart.travelSteps) {
                return left.fromStart.travelSteps < right.fromStart.travelSteps;
            }
            return left.cell < right.cell;
        });
    std::vector<HubPlan> plans;
    plans.reserve(candidates.size() +
        static_cast<std::size_t>(std::max(0, limit)));
    if (!mobileOnly) {
        for (const HubCandidate& candidate : candidates) {
            HubPlan plan;
            plan.tanker = tanker;
            plan.centrality = candidate.centrality;
            for (const std::int32_t direction : candidate.fromStart.directions) {
                plan.actions.push_back(PlanAction::move(direction));
            }
            if (candidate.fromStart.travelSteps < daySteps) {
                plan.actions.push_back(
                    PlanAction::wait(daySteps - candidate.fromStart.travelSteps));
            }
            plan.windows.push_back(RendezvousWindow{
                candidate.cell,
                candidate.fromStart.travelSteps,
                daySteps,
                false,
            });
            plans.push_back(std::move(plan));
        }
        if (static_cast<std::int32_t>(plans.size()) > limit) {
            plans.resize(static_cast<std::size_t>(limit));
        }
        if (plans.empty()) {
            return {HubPlan{}};
        }
        return plans;
    }
    struct MobileHubCandidate {
        std::size_t first = 0U;
        std::size_t second = 0U;
        ParetoPath transition;
        std::int32_t centrality = 0;
    };
    std::vector<MobileHubCandidate> mobileCandidates;
    for (std::size_t first = 0; first < candidates.size(); ++first) {
        for (std::size_t second = 0; second < candidates.size(); ++second) {
            if (first == second ||
                candidates.at(first).cell == candidates.at(second).cell ||
                std::chrono::steady_clock::now() >= deadline) {
                continue;
            }
            const std::int32_t firstArrival =
                candidates.at(first).fromStart.travelSteps;
            const std::optional<ParetoPath> transition = shortest_path(
                router,
                state,
                candidates.at(first).cell,
                candidates.at(second).cell,
                daySteps - firstArrival - 1,
                0,
                false,
                deadline);
            if (!transition.has_value()) {
                continue;
            }
            const std::int32_t secondArrival =
                firstArrival + 1 + transition->travelSteps;
            if (secondArrival >= daySteps) {
                continue;
            }
            mobileCandidates.push_back(MobileHubCandidate{
                first,
                second,
                *transition,
                candidates.at(first).centrality +
                    candidates.at(second).centrality +
                    transition->travelSteps * 8,
            });
        }
    }
    std::sort(
        mobileCandidates.begin(),
        mobileCandidates.end(),
        [&candidates](
            const MobileHubCandidate& left,
            const MobileHubCandidate& right) {
            if (left.centrality != right.centrality) {
                return left.centrality < right.centrality;
            }
            return std::tie(
                candidates.at(left.first).cell,
                candidates.at(left.second).cell) <
                std::tie(
                    candidates.at(right.first).cell,
                    candidates.at(right.second).cell);
        });
    const std::size_t mobileLimit = std::min<std::size_t>(
        mobileCandidates.size(),
        static_cast<std::size_t>(std::max(0, limit)));
    for (std::size_t mobile = 0; mobile < mobileLimit; ++mobile) {
        const MobileHubCandidate& candidate = mobileCandidates.at(mobile);
        const HubCandidate& first = candidates.at(candidate.first);
        const HubCandidate& second = candidates.at(candidate.second);
        HubPlan plan;
        plan.tanker = tanker;
        plan.centrality = candidate.centrality;
        for (const std::int32_t direction : first.fromStart.directions) {
            plan.actions.push_back(PlanAction::move(direction));
        }
        plan.actions.push_back(PlanAction::wait(1));
        for (const std::int32_t direction : candidate.transition.directions) {
            plan.actions.push_back(PlanAction::move(direction));
        }
        const std::int32_t firstArrival = first.fromStart.travelSteps;
        const std::int32_t secondArrival =
            firstArrival + 1 + candidate.transition.travelSteps;
        if (secondArrival < daySteps) {
            plan.actions.push_back(PlanAction::wait(daySteps - secondArrival));
        }
        plan.windows.push_back(RendezvousWindow{
            first.cell,
            firstArrival,
            secondArrival - 1,
            true,
        });
        plan.windows.push_back(RendezvousWindow{
            second.cell,
            secondArrival,
            daySteps,
            true,
        });
        plans.push_back(std::move(plan));
    }
    const auto window_key = [](const HubPlan& plan, std::size_t index) {
        if (index >= plan.windows.size()) {
            return std::tuple{
                kInvalidCell,
                std::numeric_limits<std::int32_t>::max(),
                std::numeric_limits<std::int32_t>::max(),
            };
        }
        const RendezvousWindow& window = plan.windows.at(index);
        return std::tuple{window.cell, window.startsAt, window.endsAt};
    };
    std::sort(
        plans.begin(),
        plans.end(),
        [&window_key](const HubPlan& left, const HubPlan& right) {
            if (left.centrality != right.centrality) {
                return left.centrality < right.centrality;
            }
            if (left.windows.size() != right.windows.size()) {
                return left.windows.size() < right.windows.size();
            }
            if (window_key(left, 0) != window_key(right, 0)) {
                return window_key(left, 0) < window_key(right, 0);
            }
            return window_key(left, 1) < window_key(right, 1);
        });
    if (plans.empty()) {
        return {};
    }
    return plans;
}

[[nodiscard]] DayPlan base_plan(
    const MatchConfig& config,
    const DayState& state,
    const HubPlan& hub) {
    DayPlan plan;
    plan.actions.reserve(static_cast<std::size_t>(config.agent_count()));
    for (AgentIndex agentIndex = 0; agentIndex < config.agent_count(); ++agentIndex) {
        if (agentIndex == hub.tanker && !hub.windows.empty()) {
            plan.actions.push_back(hub.actions);
        } else {
            plan.actions.push_back(wait_actions(config, state));
        }
    }
    return plan;
}

[[nodiscard]] std::vector<AgentIndex> patrol_agents(
    const MatchConfig& config,
    const DayState& state) {
    std::vector<AgentIndex> patrols;
    for (AgentIndex agentIndex = 0; agentIndex < config.agent_count(); ++agentIndex) {
        if (state.agents.at(static_cast<std::size_t>(agentIndex)).kind == AgentKind::Patrol) {
            patrols.push_back(agentIndex);
        }
    }
    return patrols;
}

[[nodiscard]] std::int64_t spot_priority(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    AgentIndex agent,
    SpotIndex spotIndex,
    bool backward) {
    const Spot& spot = config.spots.at(static_cast<std::size_t>(spotIndex));
    const std::int32_t multiplicity = static_cast<std::int32_t>(std::count_if(
        config.spots.begin(),
        config.spots.end(),
        [&spot](const Spot& candidate) {
            return candidate.brandIndex == spot.brandIndex;
        }));
    const std::int32_t distance = grid_distance(
        config,
        state.agents.at(static_cast<std::size_t>(agent)).position,
        spot.position);
    std::int64_t value = has_brand(ledger.lifetimeBrands, spot.brandIndex)
        ? 1000000
        : 1000000000;
    value += static_cast<std::int64_t>(1000000 / std::max(1, multiplicity));
    value += static_cast<std::int64_t>(spot.stock) * 10000;
    value += backward
        ? static_cast<std::int64_t>(distance) * 500
        : -static_cast<std::int64_t>(distance) * 500;
    return value;
}

[[nodiscard]] std::vector<SpotIndex> ranked_spots(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    AgentIndex agent,
    bool backward,
    std::int32_t limit) {
    std::vector<SpotIndex> spots;
    spots.reserve(config.spots.size());
    for (SpotIndex spot = 0; spot < static_cast<SpotIndex>(config.spots.size()); ++spot) {
        spots.push_back(spot);
    }
    std::sort(
        spots.begin(),
        spots.end(),
        [&](SpotIndex left, SpotIndex right) {
            const std::int64_t leftPriority =
                spot_priority(config, state, ledger, agent, left, backward);
            const std::int64_t rightPriority =
                spot_priority(config, state, ledger, agent, right, backward);
            if (leftPriority != rightPriority) {
                return leftPriority > rightPriority;
            }
            return left < right;
        });
    if (static_cast<std::int32_t>(spots.size()) > limit) {
        spots.resize(static_cast<std::size_t>(limit));
    }
    return spots;
}

[[nodiscard]] std::string actions_key(const AgentPlan& actions) {
    std::string key;
    key.reserve(actions.size() * 4U);
    for (const PlanAction& action : actions) {
        key += std::to_string(action.wire_value());
        key.push_back(',');
    }
    return key;
}

[[nodiscard]] std::vector<RouteOption> event_routes(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    ParetoRouter& router,
    AgentIndex agent,
    const HubPlan& hub,
    Diagnostics& diagnostics,
    std::chrono::steady_clock::time_point deadline) {
    struct SequenceCandidate {
        std::vector<SpotIndex> sequence;
        BuiltRoute built;
        std::int64_t value = 0;
    };
    const std::vector<SpotIndex> targets = ranked_spots(
        config,
        state,
        ledger,
        agent,
        false,
        14);
    std::vector<SequenceCandidate> beam;
    beam.push_back(SequenceCandidate{
        {},
        build_patrol_route(config, state, router, agent, {}, hub, deadline),
        0,
    });
    std::vector<RouteOption> options;
    options.push_back(RouteOption{beam.front().built.actions, {}, 0, 0});
    const std::int32_t maximumDepth = std::min(5, static_cast<std::int32_t>(targets.size()));
    for (std::int32_t depth = 0; depth < maximumDepth; ++depth) {
        std::vector<SequenceCandidate> next;
        for (const SequenceCandidate& candidate : beam) {
            for (const SpotIndex target : targets) {
                if (std::find(
                        candidate.sequence.begin(),
                        candidate.sequence.end(),
                        target) != candidate.sequence.end()) {
                    continue;
                }
                SequenceCandidate expanded;
                expanded.sequence = candidate.sequence;
                expanded.sequence.push_back(target);
                expanded.built = build_patrol_route(
                    config,
                    state,
                    router,
                    agent,
                    expanded.sequence,
                    hub,
                    deadline);
                if (expanded.built.reached.size() != expanded.sequence.size()) {
                    continue;
                }
                BrandMask brands;
                for (const SpotIndex reached : expanded.built.reached) {
                    brands |= brand_bit(
                        config.spots.at(static_cast<std::size_t>(reached)).brandIndex);
                }
                const std::int32_t newBrands =
                    brand_difference_count(brands, ledger.lifetimeBrands);
                expanded.value =
                    static_cast<std::int64_t>(newBrands) * 1000000000LL +
                    static_cast<std::int64_t>(brand_count(brands)) * 1000000LL +
                    static_cast<std::int64_t>(expanded.built.reached.size()) * 10000LL +
                    expanded.built.terminalFuel * 10LL -
                    expanded.built.elapsed;
                next.push_back(expanded);
                options.push_back(RouteOption{
                    expanded.built.actions,
                    expanded.built.reached,
                    brands,
                    expanded.value,
                });
            }
        }
        std::sort(
            next.begin(),
            next.end(),
            [](const SequenceCandidate& left, const SequenceCandidate& right) {
                if (left.value != right.value) {
                    return left.value > right.value;
                }
                return left.sequence < right.sequence;
            });
        if (next.size() > 18U) {
            next.resize(18U);
        }
        beam = std::move(next);
        if (beam.empty()) {
            break;
        }
    }
    std::sort(
        options.begin(),
        options.end(),
        [](const RouteOption& left, const RouteOption& right) {
            if (left.localValue != right.localValue) {
                return left.localValue > right.localValue;
            }
            return actions_key(left.actions) < actions_key(right.actions);
        });
    std::set<std::string> seen;
    std::vector<RouteOption> unique;
    for (RouteOption& option : options) {
        if (seen.insert(actions_key(option.actions)).second) {
            unique.push_back(std::move(option));
        }
        if (unique.size() >= 16U) {
            break;
        }
    }
    diagnostics.routesGenerated += static_cast<std::int32_t>(unique.size());
    return unique;
}

[[nodiscard]] std::string combination_key(
    std::int32_t hubIndex,
    const std::vector<std::int32_t>& indices) {
    std::ostringstream stream;
    stream << hubIndex << ':';
    for (const std::int32_t index : indices) {
        stream << index << ',';
    }
    return stream.str();
}

[[nodiscard]] DayPlan solve_event_conflict(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    ParetoRouter& router,
    const ExactStepSimulator& simulator,
    const IndependentDayValidator& validator,
    std::chrono::steady_clock::time_point deadline,
    Diagnostics& diagnostics) {
    Evaluation incumbent = evaluate_plan(
        config,
        state,
        ledger,
        wait_plan(config, state),
        simulator,
        validator,
        diagnostics,
        true,
        Evaluation{});
    const std::vector<AgentIndex> patrols = patrol_agents(config, state);
    const std::vector<HubPlan> hubs = hub_plans(config, state, ledger, router, 5, deadline);
    diagnostics.hubsConsidered = static_cast<std::int32_t>(hubs.size());
    const auto evaluate_hubs = [&](const std::vector<HubPlan>& phaseHubs) {
    for (std::size_t hubIndex = 0;
         hubIndex < phaseHubs.size() && std::chrono::steady_clock::now() < deadline;
         ++hubIndex) {
        const HubPlan& hub = phaseHubs.at(hubIndex);
        std::vector<std::vector<RouteOption>> routes;
        routes.reserve(patrols.size());
        for (const AgentIndex patrol : patrols) {
            routes.push_back(event_routes(
                config,
                state,
                ledger,
                router,
                patrol,
                hub,
                diagnostics,
                deadline));
        }
        if (routes.empty() || std::any_of(
                routes.begin(),
                routes.end(),
                [](const std::vector<RouteOption>& options) { return options.empty(); })) {
            continue;
        }
        CombinationNode root;
        root.routeIndices.assign(routes.size(), 0);
        for (std::size_t patrolOffset = 0; patrolOffset < routes.size(); ++patrolOffset) {
            root.estimate += routes.at(patrolOffset).front().localValue;
        }
        std::priority_queue<
            CombinationNode,
            std::vector<CombinationNode>,
            CombinationOrder> queue;
        queue.push(root);
        std::set<std::string> seen;
        seen.insert(combination_key(static_cast<std::int32_t>(hubIndex), root.routeIndices));
        std::int32_t hubEvaluations = 0;
        while (!queue.empty() &&
               hubEvaluations < 512 &&
               std::chrono::steady_clock::now() < deadline) {
            CombinationNode current = queue.top();
            queue.pop();
            ++diagnostics.searchNodes;
            ++hubEvaluations;
            DayPlan plan = base_plan(config, state, hub);
            for (std::size_t patrolOffset = 0; patrolOffset < patrols.size(); ++patrolOffset) {
                plan.actions.at(static_cast<std::size_t>(patrols.at(patrolOffset))) =
                    routes.at(patrolOffset)
                        .at(static_cast<std::size_t>(
                            current.routeIndices.at(patrolOffset)))
                        .actions;
            }
            Evaluation evaluation = evaluate_plan(
                config,
                state,
                ledger,
                plan,
                simulator,
                validator,
                diagnostics,
                true,
                incumbent);
            std::set<AgentIndex> conflictAgents;
            if (evaluation.valid) {
                for (const ClaimEvent& claim : evaluation.simulation.claims) {
                    if (!claim.served) {
                        conflictAgents.insert(claim.agent);
                        ++diagnostics.resourceConflicts;
                    }
                }
                if (better_evaluation(evaluation, incumbent)) {
                    incumbent = std::move(evaluation);
                    ++diagnostics.incumbentImprovements;
                }
            }
            std::vector<std::size_t> expansionOrder;
            for (std::size_t patrolOffset = 0; patrolOffset < patrols.size(); ++patrolOffset) {
                if (conflictAgents.contains(patrols.at(patrolOffset))) {
                    expansionOrder.push_back(patrolOffset);
                }
            }
            for (std::size_t patrolOffset = 0; patrolOffset < patrols.size(); ++patrolOffset) {
                if (std::find(
                        expansionOrder.begin(),
                        expansionOrder.end(),
                        patrolOffset) == expansionOrder.end()) {
                    expansionOrder.push_back(patrolOffset);
                }
            }
            for (const std::size_t patrolOffset : expansionOrder) {
                const std::int32_t currentIndex =
                    current.routeIndices.at(patrolOffset);
                if (currentIndex + 1 >= static_cast<std::int32_t>(
                        routes.at(patrolOffset).size())) {
                    continue;
                }
                CombinationNode next = current;
                next.routeIndices.at(patrolOffset) = currentIndex + 1;
                next.estimate -= routes.at(patrolOffset)
                    .at(static_cast<std::size_t>(currentIndex))
                    .localValue;
                next.estimate += routes.at(patrolOffset)
                    .at(static_cast<std::size_t>(currentIndex + 1))
                    .localValue;
                const std::string key = combination_key(
                    static_cast<std::int32_t>(hubIndex),
                    next.routeIndices);
                if (seen.insert(key).second) {
                    queue.push(std::move(next));
                }
            }
        }
    }
    };
    evaluate_hubs(hubs);
    if (state.dayNumber == config.day_count() &&
        std::chrono::steady_clock::now() < deadline) {
        const std::vector<HubPlan> mobileHubs =
            hub_plans(config, state, ledger, router, 5, deadline, true);
        diagnostics.hubsConsidered +=
            static_cast<std::int32_t>(mobileHubs.size());
        evaluate_hubs(mobileHubs);
    }
    diagnostics.deadlineReached = std::chrono::steady_clock::now() >= deadline;
    return incumbent.plan;
}

[[nodiscard]] std::vector<SpotIndex> backward_sequence(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    AgentIndex agent,
    std::vector<SpotIndex> assigned,
    std::uint64_t salt) {
    if (assigned.empty()) {
        return {};
    }
    const auto priority = [&](SpotIndex spot) {
        const std::uint64_t noise =
            (static_cast<std::uint64_t>(spot + 1) * 0x9E3779B97F4A7C15ULL + salt) >> 52U;
        return spot_priority(config, state, ledger, agent, spot, true) +
            static_cast<std::int64_t>(noise);
    };
    const auto endpoint = std::max_element(
        assigned.begin(),
        assigned.end(),
        [&](SpotIndex left, SpotIndex right) {
            return priority(left) < priority(right);
        });
    std::vector<SpotIndex> sequence{*endpoint};
    assigned.erase(endpoint);
    while (!assigned.empty()) {
        const CellId firstCell =
            config.spots.at(static_cast<std::size_t>(sequence.front())).position;
        const CellId startCell =
            state.agents.at(static_cast<std::size_t>(agent)).position;
        const auto predecessor = std::min_element(
            assigned.begin(),
            assigned.end(),
            [&](SpotIndex left, SpotIndex right) {
                const CellId leftCell =
                    config.spots.at(static_cast<std::size_t>(left)).position;
                const CellId rightCell =
                    config.spots.at(static_cast<std::size_t>(right)).position;
                const std::int64_t leftCost =
                    static_cast<std::int64_t>(grid_distance(config, startCell, leftCell)) * 1000 +
                    grid_distance(config, leftCell, firstCell) * 100 -
                    priority(left) / 100000;
                const std::int64_t rightCost =
                    static_cast<std::int64_t>(grid_distance(config, startCell, rightCell)) * 1000 +
                    grid_distance(config, rightCell, firstCell) * 100 -
                    priority(right) / 100000;
                if (leftCost != rightCost) {
                    return leftCost < rightCost;
                }
                return left < right;
            });
        sequence.insert(sequence.begin(), *predecessor);
        assigned.erase(predecessor);
    }
    return sequence;
}

[[nodiscard]] DayPlan solve_backward_deadline(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    ParetoRouter& router,
    const ExactStepSimulator& simulator,
    const IndependentDayValidator& validator,
    std::chrono::steady_clock::time_point deadline,
    Diagnostics& diagnostics) {
    Evaluation incumbent = evaluate_plan(
        config,
        state,
        ledger,
        wait_plan(config, state),
        simulator,
        validator,
        diagnostics,
        true,
        Evaluation{});
    const std::vector<AgentIndex> patrols = patrol_agents(config, state);
    const std::vector<HubPlan> hubs = hub_plans(config, state, ledger, router, 6, deadline);
    diagnostics.hubsConsidered = static_cast<std::int32_t>(hubs.size());
    std::vector<SpotIndex> globalTargets;
    for (SpotIndex spot = 0; spot < static_cast<SpotIndex>(config.spots.size()); ++spot) {
        globalTargets.push_back(spot);
    }
    std::sort(
        globalTargets.begin(),
        globalTargets.end(),
        [&](SpotIndex left, SpotIndex right) {
            std::int64_t leftBest = std::numeric_limits<std::int64_t>::min();
            std::int64_t rightBest = std::numeric_limits<std::int64_t>::min();
            for (const AgentIndex patrol : patrols) {
                leftBest = std::max(
                    leftBest,
                    spot_priority(config, state, ledger, patrol, left, true));
                rightBest = std::max(
                    rightBest,
                    spot_priority(config, state, ledger, patrol, right, true));
            }
            if (leftBest != rightBest) {
                return leftBest > rightBest;
            }
            return left < right;
        });
    const std::int32_t targetLimit = std::min(
        static_cast<std::int32_t>(globalTargets.size()),
        std::max(8, static_cast<std::int32_t>(patrols.size()) * 5));
    globalTargets.resize(static_cast<std::size_t>(targetLimit));
    std::uint64_t salt = static_cast<std::uint64_t>(
        mix_seed(config, state, Method::BackwardDeadline));
    for (std::int32_t variant = 0;
         variant < 96 && std::chrono::steady_clock::now() < deadline;
         ++variant) {
        const HubPlan& hub = hubs.at(static_cast<std::size_t>(variant) % hubs.size());
        std::vector<std::vector<SpotIndex>> assigned(patrols.size());
        std::vector<CellId> frontCell;
        frontCell.reserve(patrols.size());
        for (const AgentIndex patrol : patrols) {
            frontCell.push_back(state.agents.at(static_cast<std::size_t>(patrol)).position);
        }
        const std::int32_t rotation = globalTargets.empty()
            ? 0
            : variant % static_cast<std::int32_t>(globalTargets.size());
        for (std::int32_t offset = 0; offset < targetLimit; ++offset) {
            const SpotIndex target = globalTargets.at(static_cast<std::size_t>(
                (rotation + offset) % targetLimit));
            std::size_t bestPatrol = 0;
            std::int64_t bestCost = std::numeric_limits<std::int64_t>::max();
            for (std::size_t patrolOffset = 0; patrolOffset < patrols.size(); ++patrolOffset) {
                const CellId targetCell =
                    config.spots.at(static_cast<std::size_t>(target)).position;
                const std::int64_t cost =
                    static_cast<std::int64_t>(
                        grid_distance(config, frontCell.at(patrolOffset), targetCell)) * 1000 +
                    static_cast<std::int64_t>(assigned.at(patrolOffset).size()) * 500 -
                    spot_priority(
                        config,
                        state,
                        ledger,
                        patrols.at(patrolOffset),
                        target,
                        true) / 100000;
                if (cost < bestCost) {
                    bestCost = cost;
                    bestPatrol = patrolOffset;
                }
            }
            assigned.at(bestPatrol).push_back(target);
            frontCell.at(bestPatrol) =
                config.spots.at(static_cast<std::size_t>(target)).position;
        }
        DayPlan plan = base_plan(config, state, hub);
        for (std::size_t patrolOffset = 0; patrolOffset < patrols.size(); ++patrolOffset) {
            std::vector<SpotIndex> sequence = backward_sequence(
                config,
                state,
                ledger,
                patrols.at(patrolOffset),
                assigned.at(patrolOffset),
                salt + static_cast<std::uint64_t>(variant * 131 + patrolOffset));
            BuiltRoute route = build_patrol_route(
                config,
                state,
                router,
                patrols.at(patrolOffset),
                sequence,
                hub,
                deadline);
            diagnostics.routesGenerated += 1;
            plan.actions.at(static_cast<std::size_t>(patrols.at(patrolOffset))) =
                std::move(route.actions);
        }
        Evaluation evaluation = evaluate_plan(
            config,
            state,
            ledger,
            plan,
            simulator,
            validator,
            diagnostics,
            true,
            incumbent);
        ++diagnostics.searchNodes;
        if (better_evaluation(evaluation, incumbent)) {
            incumbent = std::move(evaluation);
            ++diagnostics.incumbentImprovements;
        }
        salt ^= salt << 7U;
        salt ^= salt >> 9U;
    }
    diagnostics.deadlineReached = std::chrono::steady_clock::now() >= deadline;
    return incumbent.plan;
}

[[nodiscard]] std::vector<MacroAction> macro_actions(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    const std::vector<AgentIndex>& patrols,
    const std::vector<HubPlan>& hubs,
    const MacroState& macroState,
    std::int32_t limit) {
    std::vector<MacroAction> actions;
    if (macroState.hubIndex < 0) {
        for (std::int32_t hubIndex = 0;
             hubIndex < static_cast<std::int32_t>(hubs.size());
             ++hubIndex) {
            actions.push_back(MacroAction{
                kInvalidAgent,
                hubIndex,
                -hubs.at(static_cast<std::size_t>(hubIndex)).centrality,
            });
        }
        return actions;
    }
    for (std::size_t patrolOffset = 0; patrolOffset < patrols.size(); ++patrolOffset) {
        const AgentIndex patrol = patrols.at(patrolOffset);
        const CellId origin = macroState.sequences.at(patrolOffset).empty()
            ? state.agents.at(static_cast<std::size_t>(patrol)).position
            : config.spots
                .at(static_cast<std::size_t>(
                    macroState.sequences.at(patrolOffset).back()))
                .position;
        for (SpotIndex spot = 0;
             spot < static_cast<SpotIndex>(config.spots.size());
             ++spot) {
            if (macroState.assignedBySpot.at(static_cast<std::size_t>(spot)) >=
                    config.spots.at(static_cast<std::size_t>(spot)).stock ||
                std::find(
                    macroState.sequences.at(patrolOffset).begin(),
                    macroState.sequences.at(patrolOffset).end(),
                    spot) != macroState.sequences.at(patrolOffset).end()) {
                continue;
            }
            const Spot& target = config.spots.at(static_cast<std::size_t>(spot));
            std::int64_t priority =
                has_brand(ledger.lifetimeBrands, target.brandIndex)
                ? 1000000
                : 1000000000;
            priority += static_cast<std::int64_t>(target.stock) * 10000;
            priority -= static_cast<std::int64_t>(
                grid_distance(config, origin, target.position)) * 1000;
            actions.push_back(MacroAction{
                static_cast<AgentIndex>(patrolOffset),
                spot,
                priority,
            });
        }
    }
    std::sort(
        actions.begin(),
        actions.end(),
        [](const MacroAction& left, const MacroAction& right) {
            if (left.priority != right.priority) {
                return left.priority > right.priority;
            }
            return std::tie(left.agent, left.value) <
                std::tie(right.agent, right.value);
        });
    if (static_cast<std::int32_t>(actions.size()) > limit) {
        actions.resize(static_cast<std::size_t>(limit));
    }
    return actions;
}

[[nodiscard]] MacroState apply_macro_action(
    const MacroState& source,
    const MacroAction& action) {
    MacroState result = source;
    if (action.agent == kInvalidAgent) {
        result.hubIndex = action.value;
        return result;
    }
    result.sequences.at(static_cast<std::size_t>(action.agent))
        .push_back(action.value);
    ++result.assignedBySpot.at(static_cast<std::size_t>(action.value));
    ++result.depth;
    return result;
}

[[nodiscard]] DayPlan macro_plan(
    const MatchConfig& config,
    const DayState& state,
    ParetoRouter& router,
    const std::vector<AgentIndex>& patrols,
    const std::vector<HubPlan>& hubs,
    const MacroState& macroState,
    Diagnostics& diagnostics,
    std::chrono::steady_clock::time_point deadline) {
    const HubPlan& hub = hubs.at(static_cast<std::size_t>(
        std::max(0, macroState.hubIndex)));
    DayPlan plan = base_plan(config, state, hub);
    for (std::size_t patrolOffset = 0; patrolOffset < patrols.size(); ++patrolOffset) {
        BuiltRoute route = build_patrol_route(
            config,
            state,
            router,
            patrols.at(patrolOffset),
            macroState.sequences.at(patrolOffset),
            hub,
            deadline);
        ++diagnostics.routesGenerated;
        plan.actions.at(static_cast<std::size_t>(patrols.at(patrolOffset))) =
            std::move(route.actions);
    }
    return plan;
}

[[nodiscard]] DayPlan solve_macro_mcts(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    ParetoRouter& router,
    const ExactStepSimulator& simulator,
    const IndependentDayValidator& validator,
    std::chrono::steady_clock::time_point deadline,
    Diagnostics& diagnostics) {
    Evaluation incumbent = evaluate_plan(
        config,
        state,
        ledger,
        wait_plan(config, state),
        simulator,
        validator,
        diagnostics,
        true,
        Evaluation{});
    const std::vector<AgentIndex> patrols = patrol_agents(config, state);
    const std::vector<HubPlan> hubs = hub_plans(config, state, ledger, router, 5, deadline);
    diagnostics.hubsConsidered = static_cast<std::int32_t>(hubs.size());
    MacroState rootState;
    rootState.sequences.resize(patrols.size());
    rootState.assignedBySpot.assign(config.spots.size(), 0);
    std::vector<MctsNode> nodes;
    MctsNode root;
    root.state = rootState;
    root.untried = macro_actions(
        config,
        state,
        ledger,
        patrols,
        hubs,
        root.state,
        24);
    nodes.push_back(std::move(root));
    std::mt19937_64 random(static_cast<std::uint64_t>(
        mix_seed(config, state, Method::MacroMcts)));
    std::unordered_map<std::string, Evaluation> evaluationCache;
    const std::int32_t maximumDepth = std::min(
        10,
        std::max(4, static_cast<std::int32_t>(config.spots.size())));
    while (diagnostics.rollouts < 1600 &&
           std::chrono::steady_clock::now() < deadline) {
        std::int32_t nodeIndex = 0;
        while (nodes.at(static_cast<std::size_t>(nodeIndex)).untried.empty() &&
               !nodes.at(static_cast<std::size_t>(nodeIndex)).children.empty() &&
               nodes.at(static_cast<std::size_t>(nodeIndex)).state.depth < maximumDepth) {
            const MctsNode& current =
                nodes.at(static_cast<std::size_t>(nodeIndex));
            std::int32_t bestChild = current.children.front();
            std::array<long double, 3> bestScore{
                -std::numeric_limits<long double>::infinity(),
                -std::numeric_limits<long double>::infinity(),
                -std::numeric_limits<long double>::infinity(),
            };
            for (const std::int32_t childIndex : current.children) {
                const MctsNode& child =
                    nodes.at(static_cast<std::size_t>(childIndex));
                const long double exploration = child.visits == 0
                    ? std::numeric_limits<long double>::infinity()
                    : 1.4142135623730951L * std::sqrt(
                        std::log(static_cast<long double>(
                            std::max(1, current.visits))) /
                        child.visits);
                std::array<long double, 3> score{};
                for (std::size_t tier = 0; tier < score.size(); ++tier) {
                    score.at(tier) = child.visits == 0
                        ? exploration
                        : child.totalScore.at(tier) / child.visits + exploration;
                }
                if (score > bestScore) {
                    bestScore = score;
                    bestChild = childIndex;
                }
            }
            nodeIndex = bestChild;
        }
        MctsNode& selected = nodes.at(static_cast<std::size_t>(nodeIndex));
        if (!selected.untried.empty() &&
            selected.state.depth < maximumDepth) {
            const std::size_t actionOffset = static_cast<std::size_t>(
                random() % selected.untried.size());
            const MacroAction action = selected.untried.at(actionOffset);
            selected.untried.erase(
                selected.untried.begin() +
                static_cast<std::ptrdiff_t>(actionOffset));
            MctsNode child;
            child.parent = nodeIndex;
            child.state = apply_macro_action(selected.state, action);
            child.untried = macro_actions(
                config,
                state,
                ledger,
                patrols,
                hubs,
                child.state,
                24);
            nodes.push_back(std::move(child));
            const std::int32_t childIndex =
                static_cast<std::int32_t>(nodes.size() - 1U);
            nodes.at(static_cast<std::size_t>(nodeIndex))
                .children.push_back(childIndex);
            nodeIndex = childIndex;
        }
        MacroState rolloutState =
            nodes.at(static_cast<std::size_t>(nodeIndex)).state;
        while (rolloutState.depth < maximumDepth) {
            std::vector<MacroAction> actions = macro_actions(
                config,
                state,
                ledger,
                patrols,
                hubs,
                rolloutState,
                12);
            if (actions.empty()) {
                break;
            }
            const std::size_t selectionWindow = std::min<std::size_t>(4U, actions.size());
            const std::size_t actionOffset = static_cast<std::size_t>(
                random() % selectionWindow);
            rolloutState = apply_macro_action(
                rolloutState,
                actions.at(actionOffset));
        }
        if (rolloutState.hubIndex < 0) {
            rolloutState.hubIndex = 0;
        }
        DayPlan plan = macro_plan(
            config,
            state,
            router,
            patrols,
            hubs,
            rolloutState,
            diagnostics,
            deadline);
        const std::string planId = canonical_plan_bytes(plan);
        Evaluation evaluation;
        const auto cached = evaluationCache.find(planId);
        if (cached != evaluationCache.end()) {
            evaluation = cached->second;
        } else {
            evaluation = evaluate_plan(
                config,
                state,
                ledger,
                plan,
                simulator,
                validator,
                diagnostics,
                true,
                incumbent);
            evaluationCache.emplace(planId, evaluation);
        }
        if (better_evaluation(evaluation, incumbent)) {
            incumbent = evaluation;
            ++diagnostics.incumbentImprovements;
        }
        const std::array<long double, 3> score = evaluation.valid
            ? std::array<long double, 3>{
                  static_cast<long double>(evaluation.score.lifetimeDistinct),
                  static_cast<long double>(evaluation.score.totalDailyDistinct),
                  static_cast<long double>(evaluation.score.totalServings),
              }
            : std::array<long double, 3>{-1.0L, -1.0L, -1.0L};
        while (nodeIndex >= 0) {
            MctsNode& node = nodes.at(static_cast<std::size_t>(nodeIndex));
            ++node.visits;
            for (std::size_t tier = 0; tier < score.size(); ++tier) {
                node.totalScore.at(tier) += score.at(tier);
            }
            nodeIndex = node.parent;
        }
        ++diagnostics.rollouts;
        diagnostics.searchNodes = static_cast<std::int32_t>(nodes.size());
    }
    diagnostics.deadlineReached = std::chrono::steady_clock::now() >= deadline;
    return incumbent.plan;
}

}

Planner::Planner(const MatchConfig& config, Method method)
    : config_(config),
      method_(method),
      router_(config_),
      simulator_(config_),
      validator_(config_) {}

std::vector<AgentKind> Planner::select_roles() const {
    std::vector<AgentKind> roles(
        static_cast<std::size_t>(config_.agent_count()),
        AgentKind::Patrol);
    if (roles.size() < 3U) {
        return roles;
    }
    AgentIndex tanker = 0;
    std::int64_t bestCentrality = std::numeric_limits<std::int64_t>::max();
    for (AgentIndex agentIndex = 0; agentIndex < config_.agent_count(); ++agentIndex) {
        std::int64_t centrality = 0;
        const CellId start =
            config_.initialAgents.at(static_cast<std::size_t>(agentIndex));
        for (const Spot& spot : config_.spots) {
            centrality += grid_distance(config_, start, spot.position);
        }
        if (centrality < bestCentrality) {
            bestCentrality = centrality;
            tanker = agentIndex;
        }
    }
    roles.at(static_cast<std::size_t>(tanker)) = AgentKind::Tanker;
    return roles;
}

DayPlan Planner::solve_day(
    const DayState& state,
    const MatchLedger& ledger,
    std::chrono::milliseconds budget,
    Diagnostics& diagnostics) {
    diagnostics = Diagnostics{};
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() +
        std::max(std::chrono::milliseconds{1}, budget - std::chrono::milliseconds{5});
    switch (method_) {
    case Method::EventConflict:
        return solve_event_conflict(
            config_,
            state,
            ledger,
            router_,
            simulator_,
            validator_,
            deadline,
            diagnostics);
    case Method::BackwardDeadline:
        return solve_backward_deadline(
            config_,
            state,
            ledger,
            router_,
            simulator_,
            validator_,
            deadline,
            diagnostics);
    case Method::MacroMcts:
        return solve_macro_mcts(
            config_,
            state,
            ledger,
            router_,
            simulator_,
            validator_,
            deadline,
            diagnostics);
    case Method::Portfolio: {
        const std::chrono::steady_clock::time_point eventDeadline =
            std::chrono::steady_clock::now() +
            std::max(std::chrono::milliseconds{1}, budget / 2);
        Diagnostics eventDiagnostics;
        const DayPlan eventPlan = solve_event_conflict(
            config_,
            state,
            ledger,
            router_,
            simulator_,
            validator_,
            std::min(eventDeadline, deadline),
            eventDiagnostics);
        Diagnostics mctsDiagnostics;
        const DayPlan mctsPlan = solve_macro_mcts(
            config_,
            state,
            ledger,
            router_,
            simulator_,
            validator_,
            deadline,
            mctsDiagnostics);
        merge_diagnostics(diagnostics, eventDiagnostics);
        merge_diagnostics(diagnostics, mctsDiagnostics);
        Evaluation eventEvaluation = evaluate_plan(
            config_,
            state,
            ledger,
            eventPlan,
            simulator_,
            validator_,
            diagnostics,
            true,
            Evaluation{});
        Evaluation mctsEvaluation = evaluate_plan(
            config_,
            state,
            ledger,
            mctsPlan,
            simulator_,
            validator_,
            diagnostics,
            true,
            eventEvaluation);
        return better_evaluation(mctsEvaluation, eventEvaluation)
            ? mctsPlan
            : eventPlan;
    }
    }
    return wait_plan(config_, state);
}

}
