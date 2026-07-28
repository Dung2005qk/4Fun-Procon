#pragma once

#include <string>

#include "udon/json.hpp"
#include "udon/types.hpp"

namespace udon {

class ProtocolError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[nodiscard]] MatchConfig parse_match_config(const JsonValue& document);
[[nodiscard]] DayState parse_day_state(const MatchConfig& config, const JsonValue& document);
[[nodiscard]] std::vector<AgentKind> parse_role_selection(
    const MatchConfig& config,
    const JsonValue& document);
[[nodiscard]] DayPlan parse_day_plan(const MatchConfig& config, const JsonValue& document);
[[nodiscard]] MatchLedger parse_match_ledger(const MatchConfig& config, const JsonValue& document);
[[nodiscard]] JsonValue serialize_role_selection(const std::vector<AgentKind>& roles);
[[nodiscard]] JsonValue serialize_day_plan(const DayPlan& plan);
[[nodiscard]] JsonValue serialize_match_ledger(const MatchConfig& config, const MatchLedger& ledger);
[[nodiscard]] std::string canonical_plan_bytes(const DayPlan& plan);

}
