#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include "udon/json.hpp"
#include "udon/types.hpp"

namespace udon {

struct BtcAdapterOptions {
    std::int32_t responseBudgetMs = 5000;
};

enum class BtcFrameKind : std::uint8_t {
    Setup,
    DayState,
    ActionResult,
    MatchResult,
    Unknown,
};

[[nodiscard]] MatchConfig parse_btc_setup(
    const JsonValue& document,
    BtcAdapterOptions options = {});

[[nodiscard]] DayState parse_btc_day_state(
    const MatchConfig& config,
    const JsonValue& document,
    std::chrono::system_clock::time_point receivedAt,
    BtcAdapterOptions options = {});

[[nodiscard]] std::int64_t btc_authoritative_action_deadline_ms(
    const MatchConfig& config,
    const JsonValue& document,
    std::chrono::system_clock::time_point receivedAt,
    BtcAdapterOptions options = {});

[[nodiscard]] BtcFrameKind classify_btc_frame(const JsonValue& document);
[[nodiscard]] bool btc_action_result_accepted(const JsonValue& document);
[[nodiscard]] std::optional<std::int32_t> btc_action_result_day(const JsonValue& document);
[[nodiscard]] std::string btc_action_result_reason(const JsonValue& document);
[[nodiscard]] DayPlan make_wait_plan(const MatchConfig& config, std::int32_t dayNumber);

}
