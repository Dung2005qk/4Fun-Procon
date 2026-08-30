#include "udon/simulator.hpp"

#include <bit>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace udon {

namespace {

struct ScheduledAction {
    ActionKind kind = ActionKind::Wait;
    std::int32_t completionStep = 0;
    CellId target = kInvalidCell;
    std::int32_t patrolFuelCost = 0;
};

struct RuntimeAgent {
    AgentState state;
    std::size_t actionCursor = 0;
    std::optional<ScheduledAction> pending;
};

[[nodiscard]] SimulationResult failed(
    SimulationErrorCode code,
    AgentIndex agent,
    std::int32_t step,
    std::string message) {
    SimulationResult result;
    result.valid = false;
    result.error = SimulationError{code, agent, step, std::move(message)};
    return result;
}

void record_trace(
    StepTrace& trace,
    std::int32_t step,
    const std::vector<RuntimeAgent>& agents) {
    if (trace.stepCount == 0) {
        return;
    }
    for (AgentIndex agentIndex = 0; agentIndex < trace.agentCount; ++agentIndex) {
        const std::size_t offset = static_cast<std::size_t>(step * trace.agentCount + agentIndex);
        trace.positions.at(offset) = agents.at(static_cast<std::size_t>(agentIndex)).state.position;
        trace.fuels.at(offset) = agents.at(static_cast<std::size_t>(agentIndex)).state.fuel;
    }
}

[[nodiscard]] bool schedule_next_action(
    const MatchConfig& config,
    const DayState& state,
    const DayPlan& plan,
    std::int32_t stepCount,
    std::int32_t actionStartStep,
    AgentIndex agentIndex,
    RuntimeAgent& runtime,
    SimulationResult& failureResult) {
    const AgentPlan& actions = plan.actions.at(static_cast<std::size_t>(agentIndex));
    if (runtime.actionCursor >= actions.size()) {
        failureResult = failed(
            SimulationErrorCode::DurationMismatch,
            agentIndex,
            actionStartStep,
            "agent action durations end before the day ends");
        return false;
    }
    const PlanAction& action = actions.at(runtime.actionCursor++);
    if (action.kind == ActionKind::Wait) {
        if (action.value <= 0) {
            failureResult = failed(
                SimulationErrorCode::InvalidWaitDuration,
                agentIndex,
                actionStartStep,
                "wait duration must be positive");
            return false;
        }
        if (action.value > stepCount - actionStartStep) {
            failureResult = failed(
                SimulationErrorCode::DurationMismatch,
                agentIndex,
                actionStartStep,
                "wait extends past the final step");
            return false;
        }
        runtime.pending = ScheduledAction{
            ActionKind::Wait,
            actionStartStep + action.value,
            runtime.state.position,
            0,
        };
        return true;
    }
    if (action.kind != ActionKind::Move || action.value < 0 || action.value >= kDirectionCount) {
        failureResult = failed(
            SimulationErrorCode::InvalidDirection,
            agentIndex,
            actionStartStep,
            "movement direction is outside the official range");
        return false;
    }
    const CellId source = runtime.state.position;
    const CellId target = config.map.neighbors.at(static_cast<std::size_t>(source)).at(static_cast<std::size_t>(action.value));
    if (target == kInvalidCell || config.map.terrain.at(static_cast<std::size_t>(target)) == Terrain::Pond) {
        failureResult = failed(
            SimulationErrorCode::InvalidDestination,
            agentIndex,
            actionStartStep,
            "movement enters a map boundary or pond");
        return false;
    }
    const MoveCost cost = config.move_cost(source, state.roadStatuses.at(static_cast<std::size_t>(source)));
    if (cost.steps <= 0 || cost.patrolFuel < 0) {
        failureResult = failed(
            SimulationErrorCode::InternalInvariant,
            agentIndex,
            actionStartStep,
            "movement starts from an impassable terrain");
        return false;
    }
    if (cost.steps > stepCount - actionStartStep) {
        failureResult = failed(
            SimulationErrorCode::MovementPastDeadline,
            agentIndex,
            actionStartStep,
            "movement cannot finish before the final step");
        return false;
    }
    if (runtime.state.kind == AgentKind::Patrol && runtime.state.fuel < cost.patrolFuel) {
        failureResult = failed(
            SimulationErrorCode::InsufficientFuel,
            agentIndex,
            actionStartStep,
            "patrol lacks fuel when accepting a movement");
        return false;
    }
    runtime.pending = ScheduledAction{
        ActionKind::Move,
        actionStartStep + cost.steps,
        target,
        cost.patrolFuel,
    };
    return true;
}

} 

ExactStepSimulator::ExactStepSimulator(const MatchConfig& config)
    : config_(config) {}

SimulationResult ExactStepSimulator::simulate(
    const DayState& state,
    const DayPlan& plan,
    bool captureTrace) const {
    if (state.dayNumber < 1 || state.dayNumber > config_.day_count()) {
        return failed(SimulationErrorCode::InvalidDay, kInvalidAgent, -1, "day number is outside match bounds");
    }
    const std::int32_t agentCount = config_.agent_count();
    if (static_cast<std::int32_t>(state.agents.size()) != agentCount ||
        static_cast<std::int32_t>(plan.actions.size()) != agentCount) {
        return failed(SimulationErrorCode::AgentCountMismatch, kInvalidAgent, -1, "agent count does not match config");
    }
    if (state.roadStatuses.size() != static_cast<std::size_t>(config_.map.cell_count())) {
        return failed(SimulationErrorCode::InternalInvariant, kInvalidAgent, -1, "road status vector has invalid size");
    }

    const std::int32_t stepCount = config_.steps_for_day(state.dayNumber);
    std::vector<RuntimeAgent> runtimeAgents;
    runtimeAgents.reserve(static_cast<std::size_t>(agentCount));
    for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        const AgentState& input = state.agents.at(static_cast<std::size_t>(agentIndex));
        if (!config_.map.contains(input.position) ||
            config_.map.terrain.at(static_cast<std::size_t>(input.position)) == Terrain::Pond || input.fuel < 0 ||
            input.fuel > config_.fuelLimit) {
            return failed(SimulationErrorCode::InternalInvariant, agentIndex, 0, "input agent state is invalid");
        }
        if (plan.actions.at(static_cast<std::size_t>(agentIndex)).empty()) {
            return failed(SimulationErrorCode::EmptyActionPlan, agentIndex, 0, "agent action list cannot be empty");
        }
        runtimeAgents.push_back(RuntimeAgent{input, 0U, std::nullopt});
    }

    SimulationResult result;
    result.valid = false;
    result.roadFootprint.assign(static_cast<std::size_t>(config_.map.cell_count()), 0);
    if (captureTrace) {
        result.trace.stepCount = stepCount + 1;
        result.trace.agentCount = agentCount;
        const std::size_t traceSize = static_cast<std::size_t>((stepCount + 1) * agentCount);
        result.trace.positions.assign(traceSize, kInvalidCell);
        result.trace.fuels.assign(traceSize, 0);
        record_trace(result.trace, 0, runtimeAgents);
    }

    for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        if (!schedule_next_action(
                config_,
                state,
                plan,
                stepCount,
                0,
                agentIndex,
                runtimeAgents.at(static_cast<std::size_t>(agentIndex)),
                result)) {
            return result;
        }
    }

    std::vector<std::int32_t> remainingStock;
    remainingStock.reserve(config_.spots.size());
    for (const Spot& spot : config_.spots) {
        remainingStock.push_back(spot.stock);
    }
    std::vector<std::vector<bool>> visited(
        static_cast<std::size_t>(agentCount),
        std::vector<bool>(config_.spots.size(), false));
    std::vector<bool> tankerPresent(static_cast<std::size_t>(config_.map.cell_count()), false);
    std::vector<CellId> touchedTankerCells;
    touchedTankerCells.reserve(static_cast<std::size_t>(agentCount));
    std::vector<bool> completedThisStep(static_cast<std::size_t>(agentCount), false);
    std::vector<bool> coLocatedPreviousStep(static_cast<std::size_t>(agentCount), false);
    for (const RuntimeAgent& runtime : runtimeAgents) {
        if (runtime.state.kind == AgentKind::Tanker) {
            tankerPresent.at(static_cast<std::size_t>(runtime.state.position)) = true;
            touchedTankerCells.push_back(runtime.state.position);
        }
    }
    for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        const RuntimeAgent& runtime = runtimeAgents.at(static_cast<std::size_t>(agentIndex));
        coLocatedPreviousStep.at(static_cast<std::size_t>(agentIndex)) =
            runtime.state.kind == AgentKind::Patrol &&
            tankerPresent.at(static_cast<std::size_t>(runtime.state.position));
    }
    for (const CellId position : touchedTankerCells) {
        tankerPresent.at(static_cast<std::size_t>(position)) = false;
    }
    touchedTankerCells.clear();

    for (std::int32_t step = 1; step <= stepCount; ++step) {
        std::fill(completedThisStep.begin(), completedThisStep.end(), false);
        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            RuntimeAgent& runtime = runtimeAgents.at(static_cast<std::size_t>(agentIndex));
            if (!runtime.pending.has_value() || runtime.pending->completionStep != step ||
                runtime.pending->kind != ActionKind::Move || runtime.state.kind != AgentKind::Patrol) {
                continue;
            }
            runtime.state.fuel -= runtime.pending->patrolFuelCost;
            if (runtime.state.fuel < 0) {
                return failed(
                    SimulationErrorCode::InsufficientFuel,
                    agentIndex,
                    step,
                    "fuel became negative during a reflected movement");
            }
        }

        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            RuntimeAgent& runtime = runtimeAgents.at(static_cast<std::size_t>(agentIndex));
            if (!runtime.pending.has_value() || runtime.pending->completionStep != step) {
                continue;
            }
            if (runtime.pending->kind == ActionKind::Move) {
                runtime.state.position = runtime.pending->target;
            }
            completedThisStep.at(static_cast<std::size_t>(agentIndex)) = true;
            runtime.pending.reset();
        }

        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            RuntimeAgent& runtime = runtimeAgents.at(static_cast<std::size_t>(agentIndex));
            if (runtime.state.kind != AgentKind::Patrol ||
                !completedThisStep.at(static_cast<std::size_t>(agentIndex))) {
                continue;
            }
            const SpotIndex spot = config_.spotAtCell.at(static_cast<std::size_t>(runtime.state.position));
            if (spot == kInvalidSpot || visited.at(static_cast<std::size_t>(agentIndex)).at(static_cast<std::size_t>(spot))) {
                continue;
            }
            visited.at(static_cast<std::size_t>(agentIndex)).at(static_cast<std::size_t>(spot)) = true;
            const bool served = remainingStock.at(static_cast<std::size_t>(spot)) > 0;
            result.claims.push_back(ClaimEvent{agentIndex, spot, step, served});
            if (!served) {
                continue;
            }
            --remainingStock.at(static_cast<std::size_t>(spot));
            const Spot& spotData = config_.spots.at(static_cast<std::size_t>(spot));
            result.score.brands |= brand_bit(spotData.brandIndex);
            ++result.score.servings;
        }

        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            const RuntimeAgent& runtime = runtimeAgents.at(static_cast<std::size_t>(agentIndex));
            if (runtime.state.kind != AgentKind::Tanker) {
                continue;
            }
            const CellId position = runtime.state.position;
            if (!tankerPresent.at(static_cast<std::size_t>(position))) {
                tankerPresent.at(static_cast<std::size_t>(position)) = true;
                touchedTankerCells.push_back(position);
            }
        }
        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            RuntimeAgent& runtime = runtimeAgents.at(static_cast<std::size_t>(agentIndex));
            if (runtime.state.kind != AgentKind::Patrol) {
                continue;
            }
            const bool coLocatedNow = tankerPresent.at(static_cast<std::size_t>(runtime.state.position));
            if (coLocatedNow && coLocatedPreviousStep.at(static_cast<std::size_t>(agentIndex))) {
                runtime.state.fuel = config_.fuelLimit;
            }
            coLocatedPreviousStep.at(static_cast<std::size_t>(agentIndex)) = coLocatedNow;
        }
        for (const CellId position : touchedTankerCells) {
            tankerPresent.at(static_cast<std::size_t>(position)) = false;
        }
        touchedTankerCells.clear();

        for (const RuntimeAgent& runtime : runtimeAgents) {
            if (config_.map.terrain.at(static_cast<std::size_t>(runtime.state.position)) == Terrain::Road) {
                ++result.roadFootprint.at(static_cast<std::size_t>(runtime.state.position));
            }
        }

        if (captureTrace) {
            record_trace(result.trace, step, runtimeAgents);
        }

        if (step == stepCount) {
            continue;
        }
        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            RuntimeAgent& runtime = runtimeAgents.at(static_cast<std::size_t>(agentIndex));
            if (runtime.pending.has_value()) {
                continue;
            }
            if (!schedule_next_action(
                    config_,
                    state,
                    plan,
                    stepCount,
                    step,
                    agentIndex,
                    runtime,
                    result)) {
                return result;
            }
        }
    }

    for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        const RuntimeAgent& runtime = runtimeAgents.at(
            static_cast<std::size_t>(agentIndex));
        if (runtime.pending.has_value() ||
            runtime.actionCursor !=
                plan.actions.at(static_cast<std::size_t>(agentIndex)).size()) {
            return failed(
                SimulationErrorCode::DurationMismatch,
                agentIndex,
                stepCount,
                "agent action durations do not exactly fill the day");
        }
    }

    for (const RuntimeAgent& runtime : runtimeAgents) {
        if (runtime.state.kind == AgentKind::Tanker) {
            tankerPresent.at(static_cast<std::size_t>(runtime.state.position)) = true;
        }
    }
    for (RuntimeAgent& runtime : runtimeAgents) {
        if (runtime.state.kind == AgentKind::Patrol &&
            tankerPresent.at(static_cast<std::size_t>(runtime.state.position))) {
            runtime.state.fuel = config_.fuelLimit;
        }
    }
    if (captureTrace) {
        record_trace(result.trace, stepCount, runtimeAgents);
    }

    for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        const RuntimeAgent& runtime = runtimeAgents.at(static_cast<std::size_t>(agentIndex));
        result.finalAgents.push_back(runtime.state);
    }
    result.score.dailyDistinct = brand_count(result.score.brands);
    result.valid = true;
    return result;
}

}
