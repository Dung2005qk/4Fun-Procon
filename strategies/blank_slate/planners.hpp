#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

#include "udon/graph.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"

namespace udon::blank_slate {

enum class Method : std::uint8_t {
    EventConflict,
    BackwardDeadline,
    MacroMcts,
    Portfolio,
};

struct Diagnostics {
    std::int32_t hubsConsidered = 0;
    std::int32_t routesGenerated = 0;
    std::int32_t plansEvaluated = 0;
    std::int32_t incumbentImprovements = 0;
    std::int32_t resourceConflicts = 0;
    std::int32_t searchNodes = 0;
    std::int32_t rollouts = 0;
    bool deadlineReached = false;
};

class Planner {
public:
    Planner(const MatchConfig& config, Method method);

    [[nodiscard]] std::vector<AgentKind> select_roles() const;

    [[nodiscard]] DayPlan solve_day(
        const DayState& state,
        const MatchLedger& ledger,
        std::chrono::milliseconds budget,
        Diagnostics& diagnostics);

private:
    const MatchConfig& config_;
    Method method_;
    ParetoRouter router_;
    ExactStepSimulator simulator_;
    IndependentDayValidator validator_;
};

}
