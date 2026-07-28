#pragma once

#include "udon/types.hpp"

namespace udon {

class ExactStepSimulator {
public:
    explicit ExactStepSimulator(const MatchConfig& config);

    [[nodiscard]] SimulationResult simulate(
        const DayState& state,
        const DayPlan& plan,
        bool captureTrace = true) const;

private:
    const MatchConfig& config_;
};

}
