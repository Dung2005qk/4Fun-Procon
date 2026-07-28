#pragma once

#include "udon/decision.hpp"
#include "udon/json.hpp"

namespace udon {

[[nodiscard]] JsonValue serialize_decision_replay(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    const DecisionResult& decision);

} 
