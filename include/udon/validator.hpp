#pragma once

#include "udon/types.hpp"

namespace udon {

class IndependentDayValidator {
public:
    explicit IndependentDayValidator(const MatchConfig& config);

    [[nodiscard]] SimulationResult validate(
        const DayState& state,
        const DayPlan& plan,
        bool captureTrace = true) const;

    [[nodiscard]] bool agrees_with(
        const SimulationResult& simulatorResult,
        const SimulationResult& validatorResult,
        std::string& mismatch) const;

private:
    const MatchConfig& config_;
};

}
