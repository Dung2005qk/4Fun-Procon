#include "udon/validator.hpp"

#include <bit>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace udon {

namespace {

struct ActiveOrder {
    PlanAction action;
    CellId target = kInvalidCell;
    std::int32_t remainingSteps = 0;
    std::int32_t fuelCost = 0;
};

struct ValidationAgent {
    AgentState state;
    std::size_t nextAction = 0;
    std::optional<ActiveOrder> active;
};

[[nodiscard]] SimulationResult reject(
    SimulationErrorCode code,
    AgentIndex agent,
    std::int32_t step,
    std::string message) {
    SimulationResult result;
    result.error = SimulationError{code, agent, step, std::move(message)};
    return result;
}

void write_snapshot(
    StepTrace& trace,
    std::int32_t step,
    const std::vector<ValidationAgent>& agents) {
    if (trace.stepCount == 0) {
        return;
    }
    for (AgentIndex agentIndex = 0; agentIndex < trace.agentCount; ++agentIndex) {
        const std::size_t index = static_cast<std::size_t>(step * trace.agentCount + agentIndex);
        const ValidationAgent& agent = agents.at(static_cast<std::size_t>(agentIndex));
        trace.positions.at(index) = agent.state.position;
        trace.fuels.at(index) = agent.state.fuel;
    }
}

[[nodiscard]] bool accept_order(
    const MatchConfig& config,
    const DayState& state,
    const DayPlan& plan,
    std::int32_t finalStep,
    std::int32_t currentStep,
    AgentIndex agentIndex,
    ValidationAgent& agent,
    SimulationResult& rejection) {
    const AgentPlan& actionList = plan.actions.at(static_cast<std::size_t>(agentIndex));
    if (agent.nextAction >= actionList.size()) {
        rejection = reject(
            SimulationErrorCode::DurationMismatch,
            agentIndex,
            currentStep,
            "validator found insufficient actions to fill the day");
        return false;
    }
    const PlanAction& action = actionList.at(agent.nextAction++);
    if (action.kind == ActionKind::Wait) {
        if (action.value <= 0) {
            rejection = reject(
                SimulationErrorCode::InvalidWaitDuration,
                agentIndex,
                currentStep,
                "validator found a non-positive wait duration");
            return false;
        }
        if (action.value > finalStep - currentStep) {
            rejection = reject(
                SimulationErrorCode::DurationMismatch,
                agentIndex,
                currentStep,
                "validator found a wait past the final step");
            return false;
        }
        agent.active = ActiveOrder{action, agent.state.position, action.value, 0};
        return true;
    }
    if (action.kind != ActionKind::Move || action.value < 0 || action.value >= kDirectionCount) {
        rejection = reject(
            SimulationErrorCode::InvalidDirection,
            agentIndex,
            currentStep,
            "validator found an invalid movement direction");
        return false;
    }
    const CellId source = agent.state.position;
    const CellId destination = config.map.neighbors.at(static_cast<std::size_t>(source)).at(static_cast<std::size_t>(action.value));
    if (destination == kInvalidCell || config.map.terrain.at(static_cast<std::size_t>(destination)) == Terrain::Pond) {
        rejection = reject(
            SimulationErrorCode::InvalidDestination,
            agentIndex,
            currentStep,
            "validator found a movement outside the walkable map");
        return false;
    }
    const MoveCost cost = config.move_cost(source, state.roadStatuses.at(static_cast<std::size_t>(source)));
    if (cost.steps <= 0 || cost.steps > finalStep - currentStep) {
        rejection = reject(
            SimulationErrorCode::MovementPastDeadline,
            agentIndex,
            currentStep,
            "validator found an unfinished movement");
        return false;
    }
    if (agent.state.kind == AgentKind::Patrol && agent.state.fuel < cost.patrolFuel) {
        rejection = reject(
            SimulationErrorCode::InsufficientFuel,
            agentIndex,
            currentStep,
            "validator found an accepted movement without fuel");
        return false;
    }
    agent.active = ActiveOrder{action, destination, cost.steps, cost.patrolFuel};
    return true;
}

[[nodiscard]] bool same_agent(const AgentState& left, const AgentState& right) {
    return left.kind == right.kind && left.position == right.position && left.fuel == right.fuel;
}

[[nodiscard]] bool same_claim(const ClaimEvent& left, const ClaimEvent& right) {
    return left.agent == right.agent && left.spot == right.spot && left.step == right.step && left.served == right.served;
}

} 

IndependentDayValidator::IndependentDayValidator(const MatchConfig& config)
    : config_(config) {}

SimulationResult IndependentDayValidator::validate(
    const DayState& state,
    const DayPlan& plan,
    bool captureTrace) const {
    if (state.dayNumber < 1 || state.dayNumber > config_.day_count()) {
        return reject(SimulationErrorCode::InvalidDay, kInvalidAgent, -1, "validator received an invalid day");
    }
    const std::int32_t agentCount = config_.agent_count();
    if (static_cast<std::int32_t>(state.agents.size()) != agentCount ||
        static_cast<std::int32_t>(plan.actions.size()) != agentCount) {
        return reject(SimulationErrorCode::AgentCountMismatch, kInvalidAgent, -1, "validator agent count mismatch");
    }
    if (state.roadStatuses.size() != static_cast<std::size_t>(config_.map.cell_count())) {
        return reject(SimulationErrorCode::InternalInvariant, kInvalidAgent, -1, "validator road status size mismatch");
    }

    const std::int32_t finalStep = config_.steps_for_day(state.dayNumber);
    std::vector<ValidationAgent> agents;
    agents.reserve(static_cast<std::size_t>(agentCount));
    for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        const AgentState& initial = state.agents.at(static_cast<std::size_t>(agentIndex));
        if (!config_.map.contains(initial.position) ||
            config_.map.terrain.at(static_cast<std::size_t>(initial.position)) == Terrain::Pond || initial.fuel < 0 ||
            initial.fuel > config_.fuelLimit) {
            return reject(SimulationErrorCode::InternalInvariant, agentIndex, 0, "validator input state is invalid");
        }
        if (plan.actions.at(static_cast<std::size_t>(agentIndex)).empty()) {
            return reject(SimulationErrorCode::EmptyActionPlan, agentIndex, 0, "validator found an empty action plan");
        }
        agents.push_back(ValidationAgent{initial, 0U, std::nullopt});
    }

    SimulationResult result;
    result.roadFootprint.assign(static_cast<std::size_t>(config_.map.cell_count()), 0);
    if (captureTrace) {
        result.trace.stepCount = finalStep + 1;
        result.trace.agentCount = agentCount;
        const std::size_t traceSize = static_cast<std::size_t>((finalStep + 1) * agentCount);
        result.trace.positions.assign(traceSize, kInvalidCell);
        result.trace.fuels.assign(traceSize, 0);
        write_snapshot(result.trace, 0, agents);
    }

    for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        if (!accept_order(
                config_,
                state,
                plan,
                finalStep,
                0,
                agentIndex,
                agents.at(static_cast<std::size_t>(agentIndex)),
                result)) {
            return result;
        }
    }

    std::vector<std::int32_t> stocks;
    stocks.reserve(config_.spots.size());
    for (const Spot& spot : config_.spots) {
        stocks.push_back(spot.stock);
    }
    std::vector<std::vector<std::uint8_t>> seen(
        static_cast<std::size_t>(agentCount),
        std::vector<std::uint8_t>(config_.spots.size(), 0U));
    std::vector<std::uint8_t> hasTanker(static_cast<std::size_t>(config_.map.cell_count()), 0U);
    std::vector<CellId> occupiedByTanker;
    occupiedByTanker.reserve(static_cast<std::size_t>(agentCount));
    std::vector<std::uint8_t> completedThisStep(static_cast<std::size_t>(agentCount), 0U);
    std::vector<std::uint8_t> coLocatedPreviousStep(static_cast<std::size_t>(agentCount), 0U);
    for (const ValidationAgent& agent : agents) {
        if (agent.state.kind == AgentKind::Tanker) {
            hasTanker.at(static_cast<std::size_t>(agent.state.position)) = 1U;
            occupiedByTanker.push_back(agent.state.position);
        }
    }
    for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        const ValidationAgent& agent = agents.at(static_cast<std::size_t>(agentIndex));
        coLocatedPreviousStep.at(static_cast<std::size_t>(agentIndex)) =
            agent.state.kind == AgentKind::Patrol &&
            hasTanker.at(static_cast<std::size_t>(agent.state.position)) != 0U
            ? std::uint8_t{1}
            : std::uint8_t{0};
    }
    for (const CellId position : occupiedByTanker) {
        hasTanker.at(static_cast<std::size_t>(position)) = 0U;
    }
    occupiedByTanker.clear();

    for (std::int32_t step = 1; step <= finalStep; ++step) {
        std::fill(completedThisStep.begin(), completedThisStep.end(), std::uint8_t{0});
        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            ValidationAgent& agent = agents.at(static_cast<std::size_t>(agentIndex));
            if (!agent.active.has_value()) {
                return reject(SimulationErrorCode::InternalInvariant, agentIndex, step, "validator found no active order");
            }
            --agent.active->remainingSteps;
            if (agent.active->remainingSteps < 0) {
                return reject(SimulationErrorCode::InternalInvariant, agentIndex, step, "validator order underflow");
            }
        }

        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            ValidationAgent& agent = agents.at(static_cast<std::size_t>(agentIndex));
            if (agent.active->remainingSteps != 0 || agent.active->action.kind != ActionKind::Move ||
                agent.state.kind != AgentKind::Patrol) {
                continue;
            }
            agent.state.fuel -= agent.active->fuelCost;
            if (agent.state.fuel < 0) {
                return reject(SimulationErrorCode::InsufficientFuel, agentIndex, step, "validator fuel underflow");
            }
        }

        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            ValidationAgent& agent = agents.at(static_cast<std::size_t>(agentIndex));
            if (agent.active->remainingSteps != 0) {
                continue;
            }
            if (agent.active->action.kind == ActionKind::Move) {
                agent.state.position = agent.active->target;
            }
            completedThisStep.at(static_cast<std::size_t>(agentIndex)) = 1U;
            agent.active.reset();
        }

        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            ValidationAgent& agent = agents.at(static_cast<std::size_t>(agentIndex));
            if (agent.state.kind != AgentKind::Patrol ||
                completedThisStep.at(static_cast<std::size_t>(agentIndex)) == 0U) {
                continue;
            }
            const SpotIndex spot = config_.spotAtCell.at(static_cast<std::size_t>(agent.state.position));
            if (spot == kInvalidSpot || seen.at(static_cast<std::size_t>(agentIndex)).at(static_cast<std::size_t>(spot)) != 0U) {
                continue;
            }
            seen.at(static_cast<std::size_t>(agentIndex)).at(static_cast<std::size_t>(spot)) = 1U;
            const bool receivesServing = stocks.at(static_cast<std::size_t>(spot)) > 0;
            result.claims.push_back(ClaimEvent{agentIndex, spot, step, receivesServing});
            if (receivesServing) {
                --stocks.at(static_cast<std::size_t>(spot));
                const Spot& data = config_.spots.at(static_cast<std::size_t>(spot));
                result.score.brands |= brand_bit(data.brandIndex);
                ++result.score.servings;
            }
        }

        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            const ValidationAgent& agent = agents.at(static_cast<std::size_t>(agentIndex));
            if (agent.state.kind != AgentKind::Tanker) {
                continue;
            }
            const CellId position = agent.state.position;
            if (hasTanker.at(static_cast<std::size_t>(position)) == 0U) {
                hasTanker.at(static_cast<std::size_t>(position)) = 1U;
                occupiedByTanker.push_back(position);
            }
        }
        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            ValidationAgent& agent = agents.at(static_cast<std::size_t>(agentIndex));
            if (agent.state.kind != AgentKind::Patrol) {
                continue;
            }
            const bool coLocatedNow = hasTanker.at(static_cast<std::size_t>(agent.state.position)) != 0U;
            if (coLocatedNow && coLocatedPreviousStep.at(static_cast<std::size_t>(agentIndex)) != 0U) {
                agent.state.fuel = config_.fuelLimit;
            }
            coLocatedPreviousStep.at(static_cast<std::size_t>(agentIndex)) = coLocatedNow
                ? std::uint8_t{1}
                : std::uint8_t{0};
        }
        for (const CellId position : occupiedByTanker) {
            hasTanker.at(static_cast<std::size_t>(position)) = 0U;
        }
        occupiedByTanker.clear();

        for (const ValidationAgent& agent : agents) {
            if (config_.map.terrain.at(static_cast<std::size_t>(agent.state.position)) == Terrain::Road) {
                ++result.roadFootprint.at(static_cast<std::size_t>(agent.state.position));
            }
        }
        if (captureTrace) {
            write_snapshot(result.trace, step, agents);
        }

        if (step == finalStep) {
            continue;
        }
        for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
            ValidationAgent& agent = agents.at(static_cast<std::size_t>(agentIndex));
            if (agent.active.has_value()) {
                continue;
            }
            if (!accept_order(config_, state, plan, finalStep, step, agentIndex, agent, result)) {
                return result;
            }
        }
    }

    for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        const ValidationAgent& agent = agents.at(
            static_cast<std::size_t>(agentIndex));
        if (agent.active.has_value() ||
            agent.nextAction !=
                plan.actions.at(static_cast<std::size_t>(agentIndex)).size()) {
            return reject(
                SimulationErrorCode::DurationMismatch,
                agentIndex,
                finalStep,
                "validator found a plan that does not exactly consume the day");
        }
    }

    std::fill(hasTanker.begin(), hasTanker.end(), std::uint8_t{0});
    for (const ValidationAgent& agent : agents) {
        if (agent.state.kind == AgentKind::Tanker) {
            hasTanker.at(static_cast<std::size_t>(agent.state.position)) =
                std::uint8_t{1};
        }
    }
    for (ValidationAgent& agent : agents) {
        if (agent.state.kind == AgentKind::Patrol &&
            hasTanker.at(static_cast<std::size_t>(agent.state.position)) != 0U) {
            agent.state.fuel = config_.fuelLimit;
        }
    }
    if (captureTrace) {
        write_snapshot(result.trace, finalStep, agents);
    }

    for (AgentIndex agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        const ValidationAgent& agent = agents.at(static_cast<std::size_t>(agentIndex));
        result.finalAgents.push_back(agent.state);
    }
    result.score.dailyDistinct = brand_count(result.score.brands);
    result.valid = true;
    return result;
}

bool IndependentDayValidator::agrees_with(
    const SimulationResult& simulatorResult,
    const SimulationResult& validatorResult,
    std::string& mismatch) const {
    if (simulatorResult.valid != validatorResult.valid) {
        mismatch = "simulator and validator disagree on validity";
        return false;
    }
    if (!simulatorResult.valid) {
        if (!simulatorResult.error.has_value() || !validatorResult.error.has_value() ||
            simulatorResult.error->code != validatorResult.error->code ||
            simulatorResult.error->agent != validatorResult.error->agent) {
            mismatch = "simulator and validator reject for different reasons";
            return false;
        }
        return true;
    }
    if (simulatorResult.score.brands != validatorResult.score.brands ||
        simulatorResult.score.dailyDistinct != validatorResult.score.dailyDistinct ||
        simulatorResult.score.servings != validatorResult.score.servings) {
        mismatch = "simulator and validator disagree on score";
        return false;
    }
    if (simulatorResult.roadFootprint != validatorResult.roadFootprint) {
        mismatch = "simulator and validator disagree on road footprint";
        return false;
    }
    if (simulatorResult.finalAgents.size() != validatorResult.finalAgents.size()) {
        mismatch = "simulator and validator disagree on final agent count";
        return false;
    }
    for (std::size_t agentOffset = 0; agentOffset < simulatorResult.finalAgents.size(); ++agentOffset) {
        if (!same_agent(simulatorResult.finalAgents.at(agentOffset), validatorResult.finalAgents.at(agentOffset))) {
            mismatch = "simulator and validator disagree on final agent state";
            return false;
        }
    }
    if (simulatorResult.claims.size() != validatorResult.claims.size()) {
        mismatch = "simulator and validator disagree on claim count";
        return false;
    }
    for (std::size_t claimOffset = 0; claimOffset < simulatorResult.claims.size(); ++claimOffset) {
        if (!same_claim(simulatorResult.claims.at(claimOffset), validatorResult.claims.at(claimOffset))) {
            mismatch = "simulator and validator disagree on claim order";
            return false;
        }
    }
    if (simulatorResult.trace.positions != validatorResult.trace.positions ||
        simulatorResult.trace.fuels != validatorResult.trace.fuels) {
        mismatch = "simulator and validator disagree on step trace";
        return false;
    }
    return true;
}

}
