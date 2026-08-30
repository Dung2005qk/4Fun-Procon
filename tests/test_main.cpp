#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "udon/audit.hpp"
#include "udon/btc_protocol.hpp"
#include "udon/decision.hpp"
#include "udon/graph.hpp"
#include "udon/orienteering.hpp"
#include "udon/protocol.hpp"
#include "udon/runtime.hpp"
#include "udon/simulator.hpp"
#include "udon/slack_refiner.hpp"
#include "udon/validator.hpp"

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Function>
void require_throws(Function&& function, const std::string& message) {
    try {
        function();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}

[[nodiscard]] std::string fixture_config() {
    return R"({
        "startsAt":1778227200,
        "daySeconds":[5,5,5,5],
        "daySteps":[16,16,16,16],
        "map":{"height":8,"width":8,"cells":[
            [1,1,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0]
        ]},
        "spots":[
            {"brand":10,"pos":16,"stocks":1},
            {"brand":11,"pos":17,"stocks":1},
            {"brand":12,"pos":24,"stocks":1}
        ],
        "agents":[8,9,10],
        "fuelLimits":20,
        "players":2,
        "busyThreshold":2,
        "jammedThreshold":4
    })";
}

[[nodiscard]] udon::JsonValue extended_brand_config_document(std::int32_t stock = 3) {
    udon::JsonValue::Array rows;
    for (std::int32_t row = 0; row < 9; ++row) {
        udon::JsonValue::Array columns;
        for (std::int32_t column = 0; column < 8; ++column) {
            columns.emplace_back(std::int64_t{0});
        }
        rows.emplace_back(std::move(columns));
    }

    udon::JsonValue::Array spots;
    for (std::int32_t index = 0; index < 65; ++index) {
        udon::JsonValue::Object spot;
        spot.emplace("brand", udon::JsonValue(static_cast<std::int64_t>(index)));
        spot.emplace("pos", udon::JsonValue(static_cast<std::int64_t>(index)));
        spot.emplace("stocks", udon::JsonValue(static_cast<std::int64_t>(stock)));
        spots.emplace_back(std::move(spot));
    }

    udon::JsonValue::Object map;
    map.emplace("height", udon::JsonValue(std::int64_t{9}));
    map.emplace("width", udon::JsonValue(std::int64_t{8}));
    map.emplace("cells", udon::JsonValue(std::move(rows)));

    udon::JsonValue::Array agents;
    agents.emplace_back(std::int64_t{65});
    agents.emplace_back(std::int64_t{66});
    agents.emplace_back(std::int64_t{67});

    udon::JsonValue::Array daySeconds;
    udon::JsonValue::Array daySteps;
    for (const std::int64_t value : {15, 30, 45, 60}) {
        daySeconds.emplace_back(value);
    }
    for (const std::int64_t value : {20, 32, 48, 100}) {
        daySteps.emplace_back(value);
    }

    udon::JsonValue::Object config;
    config.emplace("startsAt", udon::JsonValue(std::int64_t{1778227200}));
    config.emplace("daySeconds", udon::JsonValue(std::move(daySeconds)));
    config.emplace("daySteps", udon::JsonValue(std::move(daySteps)));
    config.emplace("map", udon::JsonValue(std::move(map)));
    config.emplace("spots", udon::JsonValue(std::move(spots)));
    config.emplace("agents", udon::JsonValue(std::move(agents)));
    config.emplace("fuelLimits", udon::JsonValue(std::int64_t{20}));
    config.emplace("players", udon::JsonValue(std::int64_t{8}));
    config.emplace("busyThreshold", udon::JsonValue(std::int64_t{7}));
    config.emplace("jammedThreshold", udon::JsonValue(std::int64_t{20}));
    return udon::JsonValue(std::move(config));
}

[[nodiscard]] udon::DayState fixture_state(const udon::MatchConfig& config) {
    return udon::parse_day_state(config, udon::JsonValue::parse(R"({
        "endsAt":1778227205,
        "day":1,
        "agents":[
            {"kind":0,"pos":16,"fuel":20},
            {"kind":1,"pos":16,"fuel":20},
            {"kind":0,"pos":18,"fuel":20}
        ],
        "others":[],
        "traffics":[
            {"pos":0,"status":0},
            {"pos":1,"status":0}
        ]
    })"));
}

void test_btc_official_wire_adapter() {
    const udon::JsonValue setup = udon::JsonValue::parse(R"({
        "daySteps":[16,16,16,16],
        "map":{"height":8,"width":8,"cells":[
            [1,1,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0]
        ]},
        "spots":[
            {"brand":10,"pos":16,"stocks":1},
            {"brand":11,"pos":17,"stocks":1},
            {"brand":12,"pos":24,"stocks":1}
        ],
        "agents":[8,9,10],
        "fuelLimits":20,
        "players":2,
        "busyThreshold":2,
        "jammedThreshold":4,
        "serverMetadata":{"ignored":true}
    })");
    const udon::MatchConfig config = udon::parse_btc_setup(
        setup,
        udon::BtcAdapterOptions{5000});
    require(config.daySeconds == std::vector<std::int32_t>({5, 5, 5, 5}),
        "BTC setup must synthesize only missing response seconds without changing day steps");
    const udon::MatchConfig oversizedWindowConfig = udon::parse_btc_setup(
        setup,
        udon::BtcAdapterOptions{60000});
    require(
        oversizedWindowConfig.daySeconds ==
            std::vector<std::int32_t>({5, 5, 5, 5}),
        "a 60000-ms server window must not enlarge the BTC adapter compute budget");
    require(udon::classify_btc_frame(setup) == udon::BtcFrameKind::Setup,
        "BTC setup frame classification failed");

    const udon::JsonValue stateDocument = udon::JsonValue::parse(R"({
        "endsAt":1778227208123,
        "day":0,
        "agents":[
            {"kind":0,"pos":16,"fuel":20},
            {"kind":1,"pos":17,"fuel":null},
            {"kind":0,"pos":18,"fuel":19}
        ],
        "others":[{
            "teamId":7,
            "agents":[
                {"kind":0,"pos":32},
                {"kind":1,"pos":33,"fuel":null},
                {"kind":0,"pos":34,"fuel":3}
            ]
        }],
        "traffics":[{"pos":1,"status":2}],
        "serverMetadata":"ignored"
    })");
    const udon::DayState state = udon::parse_btc_day_state(
        config,
        stateDocument,
        std::chrono::system_clock::time_point{std::chrono::seconds{1778227200}},
        udon::BtcAdapterOptions{5000});
    require(state.dayNumber == 1, "BTC wire day zero must map to internal day one");
    require(state.endsAt == 1778227205,
        "BTC deadline must use the local response budget instead of an unsynchronized server clock");
    const udon::DayState oversizedWindowState = udon::parse_btc_day_state(
        oversizedWindowConfig,
        stateDocument,
        std::chrono::system_clock::time_point{
            std::chrono::seconds{1778227200}},
        udon::BtcAdapterOptions{60000});
    require(
        oversizedWindowState.endsAt == 1778227205,
        "the BTC day adapter must retain the 5000-ms hard cap under an oversized outer window");
    udon::MatchConfig longOuterConfig = config;
    longOuterConfig.daySeconds.assign(4U, 60);
    const auto receivedAt = std::chrono::system_clock::time_point{
        std::chrono::seconds{1778227200}};
    require(
        udon::btc_authoritative_action_deadline_ms(
            longOuterConfig,
            stateDocument,
            receivedAt,
            udon::BtcAdapterOptions{5000}) == 1778227208123LL,
        "the outer action deadline must preserve an authoritative millisecond epoch inside the configured day window");
    udon::JsonValue secondDeadlineDocument = stateDocument;
    secondDeadlineDocument.object().at("endsAt") =
        udon::JsonValue(std::int64_t{1778227245});
    require(
        udon::btc_authoritative_action_deadline_ms(
            longOuterConfig,
            secondDeadlineDocument,
            receivedAt,
            udon::BtcAdapterOptions{5000}) == 1778227245000LL,
        "the outer action deadline must normalize an authoritative second epoch without changing the 5000-ms solver cap");
    secondDeadlineDocument.object().erase("endsAt");
    require(
        udon::btc_authoritative_action_deadline_ms(
            longOuterConfig,
            secondDeadlineDocument,
            receivedAt,
            udon::BtcAdapterOptions{5000}) == 1778227205000LL,
        "a missing authoritative deadline must fail closed to the 5000-ms response budget");
    require(state.agents.at(1).fuel == config.fuelLimit,
        "BTC tanker with null fuel must normalize without affecting patrol fuel checks");
    require(state.others.at(0).agents.at(0).fuel == config.fuelLimit,
        "hidden opponent fuel must conservatively assume maximum reachability");
    require(state.roadStatuses.at(0) == udon::RoadStatus::Smooth &&
            state.roadStatuses.at(1) == udon::RoadStatus::Jammed,
        "sparse BTC traffic must default omitted road cells to smooth");
    require(udon::classify_btc_frame(stateDocument) == udon::BtcFrameKind::DayState,
        "BTC day frame classification failed");

    const udon::DayPlan waitPlan = udon::make_wait_plan(config, 1);
    require(waitPlan.actions.size() == 3 && waitPlan.actions.front().front().wire_value() == -16,
        "BTC recovery WAIT must exactly fill every agent budget");
    require(udon::btc_action_result_accepted(udon::JsonValue::parse(R"({"valid":true})")),
        "BTC valid action result must be accepted");
    require(!udon::btc_action_result_accepted(udon::JsonValue(nullptr)),
        "an empty BTC action result must fail closed");
    require(!udon::btc_action_result_accepted(udon::JsonValue("not-json")),
        "a non-object BTC action result must fail closed without throwing");
    require(!udon::btc_action_result_accepted(udon::JsonValue::parse(R"({})")),
        "an unrecognized BTC action result object must fail closed");
    require(!udon::btc_action_result_accepted(
            udon::JsonValue::parse(R"({"reason":"E_NO_FUEL"})")),
        "BTC reason code must reject an invalid action result");
    require(!udon::btc_action_result_accepted(
            udon::JsonValue::parse(R"({"valid":true,"reason":"E_STALE"})")),
        "a rejection reason must override a conflicting positive status");
    require(
        udon::btc_action_result_day(
            udon::JsonValue::parse(R"({"day":3,"valid":true})")) == 3,
        "BTC action result must expose its authoritative accepted day");
    require(
        !udon::btc_action_result_day(
             udon::JsonValue::parse(R"({"valid":true})")).has_value(),
        "BTC action result without a day must remain explicitly unknown");

    const udon::JsonValue maximumInteger = udon::JsonValue::parse("9223372036854775807");
    require(
        maximumInteger.integer() == std::numeric_limits<std::int64_t>::max() &&
            maximumInteger.dump() == "9223372036854775807",
        "JSON must preserve the signed 64-bit maximum exactly");
    const udon::JsonValue minimumInteger = udon::JsonValue::parse("-9223372036854775808");
    require(
        minimumInteger.integer() == std::numeric_limits<std::int64_t>::min() &&
            minimumInteger.dump() == "-9223372036854775808",
        "JSON must preserve the signed 64-bit minimum exactly");
    const udon::JsonValue outsideInteger = udon::JsonValue::parse("9223372036854775808");
    require_throws(
        [&]() { static_cast<void>(outsideInteger.integer()); },
        "JSON integer conversion must reject values above the signed 64-bit range");

    require_throws(
        [&]() {
            static_cast<void>(udon::parse_btc_day_state(
                config,
                udon::JsonValue::parse(R"({
                    "day":0,
                    "agents":[
                        {"kind":0,"pos":16},
                        {"kind":1,"pos":17,"fuel":null},
                        {"kind":0,"pos":18,"fuel":19}
                    ],
                    "others":[],
                    "traffics":[]
                })"),
                std::chrono::system_clock::now(),
                udon::BtcAdapterOptions{5000}));
        },
        "BTC adapter must fail closed when own patrol fuel is absent");
}

void test_incomplete_long_horizon_role_fallback() {
    udon::MatchConfig longHorizonConfig = udon::parse_match_config(
        udon::JsonValue::parse(fixture_config()));
    longHorizonConfig.daySeconds.assign(10U, 5);
    longHorizonConfig.daySteps.assign(10U, 16);
    longHorizonConfig.fuelLimit = 160;

    udon::RoleAssignment allPatrol;
    allPatrol.roles.assign(3U, udon::AgentKind::Patrol);
    allPatrol.patrolCount = 3;
    allPatrol.rolloutValid = true;
    allPatrol.rolloutScore = udon::OfficialScore{3, 12, 40};

    udon::RoleAssignment weakerSingleTanker;
    weakerSingleTanker.roles = {
        udon::AgentKind::Tanker,
        udon::AgentKind::Patrol,
        udon::AgentKind::Patrol,
    };
    weakerSingleTanker.patrolCount = 2;
    weakerSingleTanker.rolloutValid = true;
    weakerSingleTanker.rolloutScore = udon::OfficialScore{3, 10, 30};

    udon::RoleAssignment betterSingleTanker = weakerSingleTanker;
    betterSingleTanker.roles = {
        udon::AgentKind::Patrol,
        udon::AgentKind::Tanker,
        udon::AgentKind::Patrol,
    };
    betterSingleTanker.rolloutScore = udon::OfficialScore{3, 11, 20};

    std::vector<udon::RoleAssignment> incompleteBeam{
        allPatrol,
        weakerSingleTanker,
        betterSingleTanker,
    };
    require(
        udon::apply_incomplete_long_horizon_role_fallback(
            longHorizonConfig,
            false,
            incompleteBeam),
        "incomplete long-horizon comparison must apply the one-tanker fallback");
    require(
        incompleteBeam.front().roles == betterSingleTanker.roles,
        "fallback must select the best existing one-tanker assignment by the official comparator");

    std::vector<udon::RoleAssignment> completeBeam{
        allPatrol,
        weakerSingleTanker,
        betterSingleTanker,
    };
    require(
        !udon::apply_incomplete_long_horizon_role_fallback(
            longHorizonConfig,
            true,
            completeBeam) &&
            completeBeam.front().roles == allPatrol.roles,
        "complete full-horizon evidence must preserve the selected all-patrol assignment");

    udon::MatchConfig shortHorizonConfig = longHorizonConfig;
    shortHorizonConfig.daySeconds.assign(5U, 5);
    shortHorizonConfig.daySteps.assign(5U, 16);
    std::vector<udon::RoleAssignment> shortHorizonBeam{
        allPatrol,
        weakerSingleTanker,
        betterSingleTanker,
    };
    require(
        !udon::apply_incomplete_long_horizon_role_fallback(
            shortHorizonConfig,
            false,
            shortHorizonBeam) &&
            shortHorizonBeam.front().roles == allPatrol.roles,
        "protected 4/5-day horizons must preserve the parent role selection");

    // SCORE-ROLE-SHORT-HORIZON-221: opt-in short-horizon fallback.
    udon::RoleAssignment noisyTwoTanker;
    noisyTwoTanker.roles = {
        udon::AgentKind::Tanker,
        udon::AgentKind::Tanker,
        udon::AgentKind::Patrol,
    };
    noisyTwoTanker.patrolCount = 1;
    noisyTwoTanker.rolloutValid = true;
    noisyTwoTanker.rolloutScore = udon::OfficialScore{3, 13, 18};

    std::vector<udon::RoleAssignment> shortNoisyBeam{
        noisyTwoTanker,
        betterSingleTanker,
        weakerSingleTanker,
        allPatrol,
    };
    require(
        udon::apply_incomplete_long_horizon_role_fallback(
            shortHorizonConfig,
            false,
            shortNoisyBeam,
            true) &&
            shortNoisyBeam.front().roles == betterSingleTanker.roles,
        "opt-in short-horizon fallback must displace a noisy two-tanker leader "
        "with the best single-tanker floor");

    std::vector<udon::RoleAssignment> shortSingleFloorBeam{
        noisyTwoTanker,
        weakerSingleTanker,
        betterSingleTanker,
        allPatrol,
    };
    require(
        udon::apply_incomplete_long_horizon_role_fallback(
            shortHorizonConfig,
            false,
            shortSingleFloorBeam,
            true) &&
            shortSingleFloorBeam.front().roles == betterSingleTanker.roles,
        "opt-in short-horizon fallback must never lift an all-patrol row over "
        "the single-tanker floor");

    std::vector<udon::RoleAssignment> shortCompleteBeam{
        noisyTwoTanker,
        betterSingleTanker,
        allPatrol,
    };
    require(
        !udon::apply_incomplete_long_horizon_role_fallback(
            shortHorizonConfig,
            true,
            shortCompleteBeam,
            true) &&
            shortCompleteBeam.front().roles == noisyTwoTanker.roles,
        "complete short-horizon evidence must stay authoritative even with the "
        "opt-in fallback");

    std::vector<udon::RoleAssignment> shortParentAllPatrolBeam{
        allPatrol,
        betterSingleTanker,
        weakerSingleTanker,
    };
    require(
        !udon::apply_incomplete_long_horizon_role_fallback(
            shortHorizonConfig,
            false,
            shortParentAllPatrolBeam,
            true) &&
            shortParentAllPatrolBeam.front().roles == allPatrol.roles,
        "an all-patrol parent selection must survive the opt-in short-horizon fallback");

    std::vector<udon::RoleAssignment> shortParentSingleTankerBeam{
        betterSingleTanker,
        noisyTwoTanker,
        allPatrol,
    };
    require(
        !udon::apply_incomplete_long_horizon_role_fallback(
            shortHorizonConfig,
            false,
            shortParentSingleTankerBeam,
            true) &&
            shortParentSingleTankerBeam.front().roles == betterSingleTanker.roles,
        "a single-tanker parent selection must survive the opt-in short-horizon fallback");

    udon::MatchConfig lowFuelShortConfig = shortHorizonConfig;
    lowFuelShortConfig.fuelLimit = 16;
    std::vector<udon::RoleAssignment> lowFuelAllPatrolBeam{
        allPatrol,
        weakerSingleTanker,
        betterSingleTanker,
    };
    require(
        udon::apply_incomplete_long_horizon_role_fallback(
            lowFuelShortConfig,
            false,
            lowFuelAllPatrolBeam,
            true) &&
            lowFuelAllPatrolBeam.front().roles == betterSingleTanker.roles,
        "a low-fuel all-patrol front must yield to the single-tanker floor on short horizons");

    std::vector<udon::RoleAssignment> lowFuelSingleTankerBeam{
        betterSingleTanker,
        allPatrol,
        weakerSingleTanker,
    };
    require(
        !udon::apply_incomplete_long_horizon_role_fallback(
            lowFuelShortConfig,
            false,
            lowFuelSingleTankerBeam,
            true) &&
            lowFuelSingleTankerBeam.front().roles == betterSingleTanker.roles,
        "a low-fuel single-tanker parent must survive the short-horizon fallback");

    std::vector<udon::RoleAssignment> longFlagOnBeam{
        allPatrol,
        weakerSingleTanker,
        betterSingleTanker,
    };
    require(
        udon::apply_incomplete_long_horizon_role_fallback(
            longHorizonConfig,
            false,
            longFlagOnBeam,
            true) &&
            longFlagOnBeam.front().roles == betterSingleTanker.roles,
        "long-horizon behavior must be identical under the opt-in flag");

    udon::MatchConfig lowFuelConfig = longHorizonConfig;
    lowFuelConfig.initialAgents = {8, 9, 10, 11, 12};
    lowFuelConfig.fuelLimit = 16;
    require(
        udon::role_comparison_beam_width(lowFuelConfig, 8) == 10,
        "long low-fuel comparison width must admit tanker-count diversity");
    require(
        udon::role_comparison_beam_width(longHorizonConfig, 8) == 8 &&
            udon::role_comparison_beam_width(shortHorizonConfig, 8) == 8,
        "high-fuel and protected short horizons must preserve requested beam width");

    udon::RoleAssignment fivePatrol;
    fivePatrol.roles.assign(5U, udon::AgentKind::Patrol);
    fivePatrol.patrolCount = 5;
    fivePatrol.rolloutValid = true;
    fivePatrol.rolloutScore = udon::OfficialScore{3, 30, 320};
    udon::RoleAssignment oneTanker;
    oneTanker.roles = {
        udon::AgentKind::Tanker,
        udon::AgentKind::Patrol,
        udon::AgentKind::Patrol,
        udon::AgentKind::Patrol,
        udon::AgentKind::Patrol,
    };
    oneTanker.patrolCount = 4;
    oneTanker.rolloutValid = true;
    oneTanker.rolloutScore = udon::OfficialScore{3, 30, 300};
    udon::RoleAssignment weakerTwoTanker;
    weakerTwoTanker.roles = {
        udon::AgentKind::Tanker,
        udon::AgentKind::Tanker,
        udon::AgentKind::Patrol,
        udon::AgentKind::Patrol,
        udon::AgentKind::Patrol,
    };
    weakerTwoTanker.patrolCount = 3;
    weakerTwoTanker.rolloutValid = true;
    weakerTwoTanker.rolloutScore = udon::OfficialScore{3, 30, 240};
    udon::RoleAssignment betterTwoTanker = weakerTwoTanker;
    betterTwoTanker.roles = {
        udon::AgentKind::Tanker,
        udon::AgentKind::Patrol,
        udon::AgentKind::Tanker,
        udon::AgentKind::Patrol,
        udon::AgentKind::Patrol,
    };
    betterTwoTanker.rolloutScore = udon::OfficialScore{3, 30, 250};
    std::vector<udon::RoleAssignment> tierPreservingBeam{
        fivePatrol,
        oneTanker,
        weakerTwoTanker,
        betterTwoTanker,
    };
    require(
        udon::apply_incomplete_long_horizon_role_fallback(
            lowFuelConfig,
            false,
            tierPreservingBeam) &&
            tierPreservingBeam.front().roles == betterTwoTanker.roles,
        "low-fuel incomplete evidence must select the best tier-preserving two-tanker assignment");

    udon::RoleAssignment dailyRegressingTwoTanker = betterTwoTanker;
    dailyRegressingTwoTanker.rolloutScore = udon::OfficialScore{3, 29, 400};
    std::vector<udon::RoleAssignment> dailyProtectedBeam{
        fivePatrol,
        oneTanker,
        dailyRegressingTwoTanker,
    };
    require(
        udon::apply_incomplete_long_horizon_role_fallback(
            lowFuelConfig,
            false,
            dailyProtectedBeam) &&
            dailyProtectedBeam.front().roles == oneTanker.roles,
        "a two-tanker daily regression must preserve the one-tanker fallback");

    udon::MatchConfig insufficientPatrolConfig = lowFuelConfig;
    insufficientPatrolConfig.brandValues.push_back(99);
    require(
        udon::role_comparison_beam_width(insufficientPatrolConfig, 8) == 8,
        "two tankers must not expand the comparison pool when patrols cannot cover every brand");
    std::vector<udon::RoleAssignment> insufficientPatrolBeam{
        fivePatrol,
        oneTanker,
        betterTwoTanker,
    };
    require(
        udon::apply_incomplete_long_horizon_role_fallback(
            insufficientPatrolConfig,
            false,
            insufficientPatrolBeam) &&
            insufficientPatrolBeam.front().roles == oneTanker.roles,
        "capacity-infeasible two-tanker assignments must preserve one tanker");
}

void test_even_row_geometry(const udon::MatchConfig& config) {
    const udon::CellId origin = 16;
    require(config.map.neighbors.at(static_cast<std::size_t>(origin)).at(0) == 8, "even row upper-left is wrong");
    require(config.map.neighbors.at(static_cast<std::size_t>(origin)).at(1) == 9, "even row upper-right is wrong");
    require(config.map.neighbors.at(static_cast<std::size_t>(origin)).at(3) == 25, "even row lower-right is wrong");
    require(config.map.neighbors.at(static_cast<std::size_t>(origin)).at(4) == 24, "even row lower-left is wrong");
    require(
        config.map.neighbors.at(0).at(0) == udon::kInvalidCell &&
            config.map.neighbors.at(0).at(5) == udon::kInvalidCell,
        "even-r geometry must not wrap across map boundaries");
    require(
        config.move_cost(0, udon::RoadStatus::Jammed).steps == 4 &&
            config.move_cost(16, udon::RoadStatus::Jammed).steps == 2,
        "movement cost must be determined by source terrain rather than destination terrain");
}

void test_cube_distance_and_pareto_pruning_equivalence(const udon::MatchConfig& config) {
    for (udon::CellId source = 0; source < config.map.cell_count(); ++source) {
        require(config.map.hex_distance(source, source) == 0, "cube distance to self must be zero");
        for (const udon::CellId neighbor : config.map.neighbors.at(static_cast<std::size_t>(source))) {
            if (neighbor == udon::kInvalidCell) {
                continue;
            }
            require(config.map.hex_distance(source, neighbor) == 1, "every even-r neighbor must be one cube hop away");
            require(
                config.map.hex_distance(source, neighbor) == config.map.hex_distance(neighbor, source),
                "cube distance must be symmetric");
        }
    }

    const udon::ParetoRouter router(config);
    std::vector<udon::RoadStatus> statuses(
        static_cast<std::size_t>(config.map.cell_count()),
        udon::RoadStatus::Smooth);
    statuses.at(0) = udon::RoadStatus::Busy;
    statuses.at(1) = udon::RoadStatus::Jammed;
    udon::ParetoSearchOptions pruned;
    pruned.maximumTravelSteps = 16;
    pruned.maximumPatrolFuel = 20;
    pruned.maximumLabelsPerCell = 64;
    pruned.maximumPaths = 8;
    pruned.patrol = true;
    pruned.useGeometricLowerBound = true;
    pruned.criticalRoads = {0, 1};
    udon::ParetoSearchOptions reference = pruned;
    reference.useGeometricLowerBound = false;

    const auto same_paths = [](const std::vector<udon::ParetoPath>& left, const std::vector<udon::ParetoPath>& right) {
        if (left.size() != right.size()) {
            return false;
        }
        for (std::size_t pathIndex = 0; pathIndex < left.size(); ++pathIndex) {
            const udon::ParetoPath& leftPath = left.at(pathIndex);
            const udon::ParetoPath& rightPath = right.at(pathIndex);
            if (leftPath.directions != rightPath.directions ||
                leftPath.travelSteps != rightPath.travelSteps ||
                leftPath.patrolFuel != rightPath.patrolFuel ||
                leftPath.heuristicFootprint.entries != rightPath.heuristicFootprint.entries) {
                return false;
            }
        }
        return true;
    };

    for (udon::CellId source = 0; source < config.map.cell_count(); ++source) {
        for (udon::CellId target = 0; target < config.map.cell_count(); ++target) {
            require(
                same_paths(
                    router.find_paths(source, target, statuses, pruned),
                    router.find_paths(source, target, statuses, reference)),
                "admissible cube pruning changed a Pareto route result");
        }
    }

    udon::ParetoSearchOptions impossible = pruned;
    impossible.maximumTravelSteps = config.map.hex_distance(0, config.map.cell_count() - 1) - 1;
    impossible.maximumPatrolFuel = std::numeric_limits<std::int32_t>::max();
    require(
        router.find_paths(0, config.map.cell_count() - 1, statuses, impossible).empty(),
        "cube lower bound must reject a geometrically impossible step budget");
}

void test_exact_step_order(const udon::MatchConfig& config, const udon::DayState& state) {
    const udon::DayPlan plan = udon::parse_day_plan(config, udon::JsonValue::parse(R"([
        [2,-14],
        [2,-14],
        [-16]
    ])"));
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::SimulationResult simulation = simulator.simulate(state, plan, true);
    const udon::SimulationResult validation = validator.validate(state, plan, true);
    std::string mismatch;
    require(simulation.valid, "exact simulator rejected a legal escort route");
    require(validator.agrees_with(simulation, validation, mismatch), mismatch);
    require(simulation.score.servings == 1, "action-completion spot collection is wrong");
    require(simulation.score.dailyDistinct == 1, "daily brand accounting is wrong");
    require(simulation.finalAgents.at(0).fuel == config.fuelLimit, "escort refuel is wrong");
    require(simulation.trace.position_at(1, 0) == 16, "plain movement reflected too early");
    require(simulation.trace.position_at(2, 0) == 17, "plain movement reflected too late");
}

void test_refuel_requires_full_colocation_step(
    const udon::MatchConfig& config,
    const udon::DayState& sourceState) {
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const auto verify = [&](const udon::DayState& state, const udon::DayPlan& plan) {
        const udon::SimulationResult simulation = simulator.simulate(state, plan, true);
        const udon::SimulationResult validation = validator.validate(state, plan, true);
        std::string mismatch;
        const std::string simulationError = simulation.error.has_value()
            ? simulation.error->message
            : "ok";
        const std::string validationError = validation.error.has_value()
            ? validation.error->message
            : "ok";
        require(
            simulation.valid && validator.agrees_with(simulation, validation, mismatch),
            "refuel timing fixture must agree across independent engines: simulator=" +
                simulationError + ", validator=" + validationError + ", mismatch=" + mismatch);
        return simulation;
    };

    udon::DayState arrivalOnly = sourceState;
    arrivalOnly.agents.at(0) = udon::AgentState{udon::AgentKind::Patrol, 8, 10};
    arrivalOnly.agents.at(1) = udon::AgentState{udon::AgentKind::Tanker, 9, config.fuelLimit};
    arrivalOnly.agents.at(2) = udon::AgentState{udon::AgentKind::Patrol, 18, config.fuelLimit};
    const udon::DayPlan arrivalOnlyPlan = udon::parse_day_plan(
        config,
        udon::JsonValue::parse(R"([
            [1,3,-13],
            [0,-14],
            [-16]
        ])"));
    const udon::SimulationResult arrivalOnlyResult = verify(arrivalOnly, arrivalOnlyPlan);
    require(
        arrivalOnlyResult.trace.position_at(2, 0) == 0 &&
            arrivalOnlyResult.trace.position_at(2, 1) == 0 &&
            arrivalOnlyResult.finalAgents.at(0).fuel < config.fuelLimit,
        "an instantaneous rendezvous followed by immediate departure must not refuel");

    const udon::DayPlan dwellPlan = udon::parse_day_plan(
        config,
        udon::JsonValue::parse(R"([
            [1,-1,3,-12],
            [0,-14],
            [-16]
        ])"));
    const udon::SimulationResult dwellResult = verify(arrivalOnly, dwellPlan);
    require(
        dwellResult.trace.fuel_at(2, 0) < config.fuelLimit &&
            dwellResult.trace.fuel_at(3, 0) == config.fuelLimit &&
            dwellResult.finalAgents.at(0).fuel <= config.fuelLimit,
        "one completed step of continuous co-location must refuel before departure");

    udon::DayState lockstep = sourceState;
    lockstep.agents.at(0) = udon::AgentState{udon::AgentKind::Patrol, 16, 1};
    lockstep.agents.at(1) = udon::AgentState{udon::AgentKind::Tanker, 16, config.fuelLimit};
    const udon::DayPlan lockstepPlan = udon::parse_day_plan(
        config,
        udon::JsonValue::parse(R"([
            [3,-14],
            [3,-14],
            [-16]
        ])"));
    const udon::SimulationResult lockstepResult = verify(lockstep, lockstepPlan);
    require(
        lockstepResult.trace.position_at(2, 0) == 25 &&
            lockstepResult.finalAgents.at(0).fuel == config.fuelLimit,
        "continuous lockstep movement from an existing rendezvous must refuel");

    udon::DayState terminalArrival = sourceState;
    terminalArrival.agents.at(0) = udon::AgentState{
        udon::AgentKind::Patrol,
        8,
        1,
    };
    terminalArrival.agents.at(1) = udon::AgentState{
        udon::AgentKind::Tanker,
        9,
        config.fuelLimit,
    };
    terminalArrival.agents.at(2) = udon::AgentState{
        udon::AgentKind::Patrol,
        18,
        5,
    };
    const udon::DayPlan terminalArrivalPlan = udon::parse_day_plan(
        config,
        udon::JsonValue::parse(R"([
            [-14,2],
            [-16],
            [-16]
        ])"));
    const udon::SimulationResult terminalArrivalResult = verify(
        terminalArrival,
        terminalArrivalPlan);
    require(
        terminalArrivalResult.trace.position_at(15, 0) == 8 &&
            terminalArrivalResult.trace.fuel_at(15, 0) == 1 &&
            terminalArrivalResult.trace.position_at(16, 0) == 9 &&
            terminalArrivalResult.trace.fuel_at(16, 0) == config.fuelLimit &&
            terminalArrivalResult.finalAgents.at(0).fuel == config.fuelLimit,
        "terminal co-location must refill only the next-day boundary state");
    require(
        terminalArrivalResult.finalAgents.at(2).position == 18 &&
            terminalArrivalResult.finalAgents.at(2).fuel == 5,
        "a non-colocated patrol must carry its remaining fuel across the day boundary");
}

void test_invalid_duration_rejected(const udon::MatchConfig& config, const udon::DayState& state) {
    const udon::DayPlan plan = udon::parse_day_plan(config, udon::JsonValue::parse(R"([
        [2,-13],
        [-16],
        [-16]
    ])"));
    const udon::ExactStepSimulator simulator(config);
    const udon::SimulationResult result = simulator.simulate(state, plan, false);
    require(!result.valid, "short action plan must be rejected");
    require(result.error.has_value() && result.error->code == udon::SimulationErrorCode::DurationMismatch,
            "short action plan has the wrong failure code");
}

void test_stock_order_repeat_visit_and_non_refuel(
    const udon::MatchConfig& config,
    const udon::DayState& sourceState) {
    udon::DayState contested = sourceState;
    contested.agents.at(0) = udon::AgentState{
        udon::AgentKind::Patrol,
        16,
        5,
    };
    contested.agents.at(1) = udon::AgentState{
        udon::AgentKind::Tanker,
        17,
        config.fuelLimit,
    };
    contested.agents.at(2) = udon::AgentState{
        udon::AgentKind::Patrol,
        16,
        config.fuelLimit,
    };
    const udon::DayPlan contestedPlan = udon::parse_day_plan(
        config,
        udon::JsonValue::parse(R"([
            [2,-14],
            [2,-14],
            [2,-14]
        ])"));
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::SimulationResult first = simulator.simulate(
        contested,
        contestedPlan,
        true);
    const udon::SimulationResult checked = validator.validate(
        contested,
        contestedPlan,
        true);
    std::string mismatch;
    require(
        first.valid && validator.agrees_with(first, checked, mismatch),
        "stock-order fixture must pass both independent engines");
    std::vector<udon::ClaimEvent> contestedSpotClaims;
    std::copy_if(
        first.claims.begin(),
        first.claims.end(),
        std::back_inserter(contestedSpotClaims),
        [](const udon::ClaimEvent& claim) {
            return claim.spot == 1 && claim.step == 2;
        });
    require(
        contestedSpotClaims.size() == 2U &&
            contestedSpotClaims.at(0).agent == 0 &&
            contestedSpotClaims.at(0).served &&
            contestedSpotClaims.at(1).agent == 2 &&
            !contestedSpotClaims.at(1).served,
        "simultaneous completed-action claims must follow official agent-list order");
    require(
        first.finalAgents.at(0).fuel == 4,
        "a tanker one cell ahead must not refuel the patrol behind it");

    udon::DayState repeatState = sourceState;
    repeatState.agents.at(1).position = 18;
    repeatState.agents.at(2).position = 18;
    const udon::DayPlan repeatPlan = udon::parse_day_plan(
        config,
        udon::JsonValue::parse(R"([
            [2,5,-12],
            [-16],
            [-16]
        ])"));
    const udon::SimulationResult repeated = simulator.simulate(
        repeatState,
        repeatPlan,
        false);
    require(
        repeated.valid &&
            std::count_if(
                repeated.claims.begin(),
                repeated.claims.end(),
                [](const udon::ClaimEvent& claim) {
                    return claim.agent == 0 && claim.spot == 0;
                }) == 1,
        "leaving and revisiting a spot in one day must not create a second claim");

    contested.dayNumber = 2;
    const udon::SimulationResult nextDay = simulator.simulate(
        contested,
        contestedPlan,
        false);
    require(
        nextDay.valid &&
            std::any_of(
                nextDay.claims.begin(),
                nextDay.claims.end(),
                [](const udon::ClaimEvent& claim) {
                    return claim.spot == 1 && claim.step == 2 &&
                        claim.agent == 0 && claim.served;
                }),
        "spot stock and per-agent visited state must reset on the next day");

    const udon::JsonValue serialized = udon::serialize_day_plan(repeatPlan);
    const udon::DayPlan roundTrip = udon::parse_day_plan(config, serialized);
    require(
        udon::canonical_plan_bytes(roundTrip) ==
            udon::canonical_plan_bytes(repeatPlan),
        "plan serialization round-trip must preserve action semantics exactly");
    const udon::SimulationResult deterministic = simulator.simulate(
        repeatState,
        roundTrip,
        false);
    const bool sameFinalAgents =
        deterministic.finalAgents.size() == repeated.finalAgents.size() &&
        std::equal(
            deterministic.finalAgents.begin(),
            deterministic.finalAgents.end(),
            repeated.finalAgents.begin(),
            [](const udon::AgentState& left, const udon::AgentState& right) {
                return left.kind == right.kind &&
                    left.position == right.position &&
                    left.fuel == right.fuel;
            });
    require(
        deterministic.valid &&
            deterministic.score.brands == repeated.score.brands &&
            deterministic.score.servings == repeated.score.servings &&
            sameFinalAgents &&
            deterministic.roadFootprint == repeated.roadFootprint,
        "exact simulation must be deterministic under plan round-trip");
}

void test_ledger_round_trip(const udon::MatchConfig& config) {
    udon::MatchLedger ledger;
    ledger.lifetimeBrands = udon::brand_bit(0) | udon::brand_bit(2);
    ledger.totalDailyDistinct = 7;
    ledger.totalServings = 11;
    const udon::MatchLedger parsed = udon::parse_match_ledger(
        config,
        udon::serialize_match_ledger(config, ledger));
    require(parsed.lifetimeBrands == ledger.lifetimeBrands, "ledger brand round-trip changed coverage");
    require(parsed.totalDailyDistinct == ledger.totalDailyDistinct, "ledger daily total round-trip changed");
    require(parsed.totalServings == ledger.totalServings, "ledger serving total round-trip changed");
}

void test_full_official_brand_and_variable_day_domain() {
    const udon::MatchConfig config = udon::parse_match_config(
        extended_brand_config_document());
    require(config.spots.size() == 65U, "parser must accept more than one spot per map side");
    require(config.brand_count() == 65, "parser must retain every published franchise chain");
    require(
        config.daySeconds == std::vector<std::int32_t>({15, 30, 45, 60}) &&
            config.daySteps == std::vector<std::int32_t>({20, 32, 48, 100}),
        "per-day response and step schedules must remain distinct");
    require(
        config.busyThreshold == 7 && config.jammedThreshold == 20,
        "match-specific traffic thresholds must not be clipped to undocumented local ranges");

    udon::BrandMask mask = udon::brand_bit(0) | udon::brand_bit(64);
    require(
        udon::brand_count(mask) == 2 && udon::has_brand(mask, 0) &&
            udon::has_brand(mask, 64),
        "brand coverage must preserve bits on both sides of the historical 64-chain boundary");
    require(
        udon::brand_intersection_count(mask, udon::brand_bit(64)) == 1 &&
            udon::brand_difference_count(mask, udon::brand_bit(0)) == 1,
        "extended brand-mask set operations must remain exact");

    udon::DayState state;
    state.endsAt = config.startsAt + config.daySeconds.front();
    state.dayNumber = 1;
    state.roadStatuses.assign(
        static_cast<std::size_t>(config.map.cell_count()),
        udon::RoadStatus::Smooth);
    for (const udon::CellId start : config.initialAgents) {
        state.agents.push_back(udon::AgentState{udon::AgentKind::Patrol, start, config.fuelLimit});
    }

    udon::DayPlan pickupPlan;
    pickupPlan.actions = {
        {udon::PlanAction::move(5), udon::PlanAction::wait(18)},
        {udon::PlanAction::wait(20)},
        {udon::PlanAction::wait(20)},
    };
    const udon::ExactStepSimulator simulator(config);
    const udon::SimulationResult pickup = simulator.simulate(state, pickupPlan, true);
    require(pickup.valid, "65-chain fixture must execute through the exact simulator");
    require(
        pickup.score.dailyDistinct == 1 && pickup.score.servings == 1 &&
            udon::has_brand(pickup.score.brands, 64),
        "the exact simulator must score a chain beyond index 63");

    udon::MatchLedger ledger;
    ledger.apply(pickup.score);
    ledger.lifetimeBrands |= udon::brand_bit(0);
    const udon::MatchLedger parsed = udon::parse_match_ledger(
        config,
        udon::serialize_match_ledger(config, ledger));
    require(
        parsed.lifetimeBrands == ledger.lifetimeBrands && parsed.lifetime_distinct() == 2,
        "extended lifetime coverage must survive the protocol ledger round-trip");

    const udon::ParetoRouter router(config);
    const udon::RouteColumnGenerator generator(config, router);
    udon::ColumnGenerationOptions columnOptions;
    columnOptions.enableExactHarvestOrienteering = true;
    columnOptions.allowUncachedHarvestTargets = true;
    columnOptions.maximumPathsPerTarget = 1;
    columnOptions.maximumColumnsPerAgent = 12;
    columnOptions.maximumTargetSpots = 8;
    udon::MatchLedger planningLedger;
    for (std::int32_t brand = 0; brand < 64; ++brand) {
        planningLedger.lifetimeBrands |= udon::brand_bit(brand);
    }
    const udon::RoutePortfolio portfolio = generator.generate(
        state,
        planningLedger,
        columnOptions);
    require(
        portfolio.columnsByAgent.size() == state.agents.size() &&
            std::all_of(
                portfolio.columnsByAgent.begin(),
                portfolio.columnsByAgent.end(),
                [](const std::vector<udon::RouteColumn>& columns) {
                    return !columns.empty();
                }),
        "the general column generator must remain active beyond exact 16/32-spot accelerators");
    require(
        std::any_of(
            portfolio.columnsByAgent.begin(),
            portfolio.columnsByAgent.end(),
            [](const std::vector<udon::RouteColumn>& columns) {
                return std::any_of(
                    columns.begin(),
                    columns.end(),
                    [](const udon::RouteColumn& column) {
                        return udon::has_brand(column.estimatedBrands, 64);
                    });
            }),
        "the production column path must expose a collectible chain beyond index 63");

    udon::UdonShieldEngine engine(config);
    const udon::DecisionResult decision = engine.solve_day(
        state,
        planningLedger,
        std::chrono::milliseconds{100});
    require(
        decision.candidate.simulation.valid &&
            udon::IndependentDayValidator(config)
                .validate(state, decision.candidate.plan, false)
                .valid,
        "the end-to-end decision path must return a valid plan on the full chain domain");

    state.dayNumber = 2;
    state.endsAt += config.daySeconds.at(1);
    require(
        simulator.simulate(state, udon::make_wait_plan(config, 2), true).valid,
        "day two must use its own published step budget");
    udon::DayPlan staleDayOneLength;
    staleDayOneLength.actions.assign(
        static_cast<std::size_t>(config.agent_count()),
        udon::AgentPlan{udon::PlanAction::wait(config.steps_for_day(1))});
    require(
        !simulator.simulate(state, staleDayOneLength, true).valid,
        "a day-one action length must not be accepted on a longer day two");
    udon::DayState longDay = state;
    longDay.dayNumber = 4;
    require(
        simulator.simulate(longDay, udon::make_wait_plan(config, 4), true).valid,
        "an official positive day-step budget must not be rejected by a fabricated map-dependent cap");

    require_throws(
        []() {
            static_cast<void>(udon::parse_match_config(
                extended_brand_config_document(4)));
        },
        "spot stock above the match agent count must be rejected");
}

void test_protocol_fail_closed_schema(const udon::MatchConfig& config) {
    udon::JsonValue configDocument = udon::JsonValue::parse(fixture_config());
    configDocument.object().emplace("riskMode", udon::JsonValue("adaptive"));
    require_throws(
        [&configDocument]() {
            static_cast<void>(udon::parse_match_config(configDocument));
        },
        "competition config must reject unknown risk-mode fields");

    udon::JsonValue versionedConfig = udon::JsonValue::parse(fixture_config());
    versionedConfig.object().emplace("protocolVersion", udon::JsonValue("future-v99"));
    require_throws(
        [&versionedConfig]() {
            static_cast<void>(udon::parse_match_config(versionedConfig));
        },
        "unsupported protocol versions must fail closed");

    udon::JsonValue dayDocument = udon::JsonValue::parse(R"({
        "endsAt":1778227205,
        "day":1,
        "agents":[
            {"kind":0,"pos":16,"fuel":20},
            {"kind":1,"pos":16,"fuel":20},
            {"kind":0,"pos":18,"fuel":20}
        ],
        "others":[],
        "traffics":[{"pos":0,"status":0},{"pos":1,"status":0}],
        "privateRouteHint":[1,2,3]
    })");
    require_throws(
        [&config, &dayDocument]() {
            static_cast<void>(udon::parse_day_state(config, dayDocument));
        },
        "authoritative state must reject private or unknown adapter fields");
}

void test_road_traffic_timing(const udon::MatchConfig& config) {
    udon::DayState state = fixture_state(config);
    state.agents.at(0).position = 0;
    state.agents.at(0).fuel = 20;
    state.agents.at(1).position = 16;
    state.agents.at(2).position = 18;
    const udon::DayPlan plan = udon::parse_day_plan(config, udon::JsonValue::parse(R"([
        [2,-15],
        [-16],
        [-16]
    ])"));
    const udon::ExactStepSimulator simulator(config);
    const udon::SimulationResult result = simulator.simulate(state, plan, true);
    require(result.valid, "road route should be valid");
    require(result.roadFootprint.at(0) == 0, "step zero must not count traffic");
    require(result.roadFootprint.at(1) == 16, "final reflected road occupancy must count traffic");
}

void test_pareto_cache_respects_full_query_key(const udon::MatchConfig& config) {
    const udon::ParetoRouter router(config);
    std::vector<udon::RoadStatus> smooth(static_cast<std::size_t>(config.map.cell_count()), udon::RoadStatus::Smooth);
    udon::ParetoSearchOptions options;
    options.maximumTravelSteps = 1;
    options.maximumPatrolFuel = 2;
    options.maximumLabelsPerCell = 4;
    options.maximumPaths = 1;
    options.patrol = true;

    const std::vector<udon::ParetoPath> smoothPaths = router.find_paths(0, 8, smooth, options);
    require(!smoothPaths.empty(), "smooth road query should reach the adjacent plain cell");

    std::vector<udon::RoadStatus> jammed = smooth;
    jammed.at(0) = udon::RoadStatus::Jammed;
    require(
        router.find_paths(0, 8, jammed, options).empty(),
        "route cache must not reuse a smooth query under jammed road status");

    udon::ParetoSearchOptions fuelLimited = options;
    fuelLimited.maximumPatrolFuel = 1;
    require(
        router.find_paths(0, 8, smooth, fuelLimited).empty(),
        "route cache must not reuse a query with a stricter fuel cap");

    udon::ParetoSearchOptions zeroFuel = options;
    zeroFuel.maximumPatrolFuel = 0;
    require(
        router.find_paths(0, 8, smooth, zeroFuel).empty(),
        "a patrol with zero fuel must not receive an unlimited Pareto route");

    udon::ParetoSearchOptions criticalRoads = options;
    criticalRoads.criticalRoads.push_back(0);
    const std::vector<udon::ParetoPath> criticalPaths = router.find_paths(0, 8, smooth, criticalRoads);
    require(
        !criticalPaths.empty() && criticalPaths.front().heuristicFootprint.at(0) == 1,
        "route cache must retain the critical-road footprint dimension");
    const std::vector<udon::ParetoPath> smoothAgain = router.find_paths(0, 8, smooth, options);
    require(
        !smoothAgain.empty() && smoothAgain.front().directions == smoothPaths.front().directions &&
            smoothAgain.front().travelSteps == smoothPaths.front().travelSteps &&
            smoothAgain.front().patrolFuel == smoothPaths.front().patrolFuel &&
            smoothAgain.front().heuristicFootprint.entries == smoothPaths.front().heuristicFootprint.entries,
        "route cache must preserve the exact baseline result after a critical-road query");

    const udon::ParetoRouter observedRouter(config);
    udon::ParetoSearchDiagnostics firstDiagnostics;
    const std::vector<udon::ParetoPath> observedPaths =
        observedRouter.find_paths(0, 8, smooth, options, &firstDiagnostics);
    require(
        !observedPaths.empty() &&
            observedPaths.front().directions == smoothPaths.front().directions &&
            observedPaths.front().travelSteps == smoothPaths.front().travelSteps &&
            observedPaths.front().patrolFuel == smoothPaths.front().patrolFuel &&
            observedPaths.front().heuristicFootprint.entries == smoothPaths.front().heuristicFootprint.entries,
        "Pareto diagnostics must not change the returned routes");
    require(
        firstDiagnostics.queries == 1 && firstDiagnostics.cacheMisses == 1 &&
            firstDiagnostics.cacheHits == 0 && firstDiagnostics.labelsGenerated > 0 &&
            firstDiagnostics.resourceBoundCacheMisses == 1,
        "Pareto diagnostics must account for an uncached search");
    udon::ParetoSearchOptions alternateBudget = options;
    alternateBudget.maximumTravelSteps = 2;
    udon::ParetoSearchDiagnostics lowerBoundCachedDiagnostics;
    const std::vector<udon::ParetoPath> lowerBoundCachedPaths =
        observedRouter.find_paths(0, 8, smooth, alternateBudget, &lowerBoundCachedDiagnostics);
    require(
        !lowerBoundCachedPaths.empty() &&
            lowerBoundCachedDiagnostics.cacheMisses == 1 &&
            lowerBoundCachedDiagnostics.resourceBoundCacheHits == 1 &&
            lowerBoundCachedDiagnostics.resourceBoundCacheMisses == 0,
        "Pareto routing must reuse exact resource lower bounds across query budgets");
    udon::ParetoSearchDiagnostics cachedDiagnostics;
    const std::vector<udon::ParetoPath> cachedObservedPaths =
        observedRouter.find_paths(0, 8, smooth, options, &cachedDiagnostics);
    require(
        !cachedObservedPaths.empty() &&
            cachedObservedPaths.front().directions == observedPaths.front().directions &&
            cachedDiagnostics.queries == 1 && cachedDiagnostics.cacheHits == 1 &&
            cachedDiagnostics.cacheMisses == 0 && cachedDiagnostics.labelsGenerated == 0,
        "Pareto diagnostics must distinguish cache hits without changing results");
}

void test_alns_preserves_escort_group_atomicity(const udon::MatchConfig& config, const udon::DayState& state) {
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    udon::RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(static_cast<std::size_t>(config.agent_count()));
    for (udon::AgentIndex agentIndex = 0; agentIndex < config.agent_count(); ++agentIndex) {
        udon::RouteColumn wait;
        wait.columnId = agentIndex;
        wait.agent = agentIndex;
        wait.actions = udon::AgentPlan{udon::PlanAction::wait(config.steps_for_day(state.dayNumber))};
        wait.terminalCell = state.agents.at(static_cast<std::size_t>(agentIndex)).position;
        portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex)).push_back(std::move(wait));
    }
    for (udon::AgentIndex agentIndex : {udon::AgentIndex{0}, udon::AgentIndex{1}}) {
        udon::RouteColumn escort;
        escort.columnId = 10 + agentIndex;
        escort.agent = agentIndex;
        escort.actions = udon::AgentPlan{udon::PlanAction::move(2), udon::PlanAction::wait(14)};
        escort.terminalCell = 17;
        escort.escortGroup = 7;
        escort.lockstepEscort = true;
        escort.priority = 100;
        portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex)).push_back(std::move(escort));
    }
    const udon::DayPlan waitPlan = udon::emergency_wait_plan(config, state);
    const std::optional<udon::MasterCandidate> seed = master.evaluate_exact_plan(
        state,
        udon::MatchLedger{},
        waitPlan);
    require(seed.has_value(), "ALNS escort fixture must start from an exact valid incumbent");

    udon::AdaptiveRouteImprover improver(config, master);
    udon::AlnsOptions options;
    options.maximumIterations = 8;
    options.maximumAlternativesPerIteration = 1;
    options.maximumCandidates = 4;
    udon::AlnsDiagnostics diagnostics;
    const std::vector<udon::MasterCandidate> candidates = improver.improve(
        state,
        udon::MatchLedger{},
        portfolio,
        {*seed},
        options,
        diagnostics);
    const auto coordinated = std::find_if(
        candidates.begin(),
        candidates.end(),
        [](const udon::MasterCandidate& candidate) {
            return candidate.plan.actions.at(0).front().kind == udon::ActionKind::Move &&
                candidate.plan.actions.at(1).front().kind == udon::ActionKind::Move;
        });
    require(
        diagnostics.accepted > 0 && coordinated != candidates.end(),
        "escort ALNS mutation must replace both synchronized agents atomically");
    const udon::SimulationResult simulation = simulator.simulate(state, coordinated->plan, true);
    const udon::SimulationResult validation = validator.validate(state, coordinated->plan, true);
    std::string mismatch;
    require(simulation.valid && validator.agrees_with(simulation, validation, mismatch),
            "coordinated escort mutation must remain independently valid");
}

void test_alns_synthesizes_route_outside_portfolio(
    const udon::MatchConfig& config,
    const udon::DayState& state) {
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    udon::RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(static_cast<std::size_t>(config.agent_count()));
    for (udon::AgentIndex agentIndex = 0; agentIndex < config.agent_count(); ++agentIndex) {
        udon::RouteColumn wait;
        wait.columnId = agentIndex;
        wait.agent = agentIndex;
        wait.actions = {udon::PlanAction::wait(config.steps_for_day(state.dayNumber))};
        wait.terminalCell = state.agents.at(static_cast<std::size_t>(agentIndex)).position;
        portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex)).push_back(std::move(wait));
    }
    const std::optional<udon::MasterCandidate> seed = master.evaluate_exact_plan(
        state,
        udon::MatchLedger{},
        udon::emergency_wait_plan(config, state));
    require(seed.has_value(), "ALNS synthesis fixture requires a valid wait incumbent");

    udon::AdaptiveRouteImprover improver(config, master);
    udon::AlnsOptions options;
    options.maximumIterations = 1;
    options.maximumAlternativesPerIteration = 4;
    options.maximumCandidates = 16;
    options.brandSlack.assign(static_cast<std::size_t>(config.brand_count()), 0);
    options.latestSafeDayByBrand.assign(static_cast<std::size_t>(config.brand_count()), 1);
    udon::AlnsDiagnostics diagnostics;
    const std::vector<udon::MasterCandidate> candidates = improver.improve(
        state,
        udon::MatchLedger{},
        portfolio,
        {*seed},
        options,
        diagnostics);
    const bool hasNewAgentPlan = std::any_of(
        candidates.begin(),
        candidates.end(),
        [&portfolio](const udon::MasterCandidate& candidate) {
            for (std::size_t agentIndex = 0; agentIndex < candidate.plan.actions.size(); ++agentIndex) {
                const bool existed = std::any_of(
                    portfolio.columnsByAgent.at(agentIndex).begin(),
                    portfolio.columnsByAgent.at(agentIndex).end(),
                    [&candidate, agentIndex](const udon::RouteColumn& column) {
                        const udon::AgentPlan& actions = candidate.plan.actions.at(agentIndex);
                        return column.actions.size() == actions.size() &&
                            std::equal(
                                column.actions.begin(),
                                column.actions.end(),
                                actions.begin(),
                                [](const udon::PlanAction& left, const udon::PlanAction& right) {
                                    return left.kind == right.kind && left.value == right.value;
                                });
                    });
                if (!existed) {
                    return true;
                }
            }
            return false;
        });
    require(
        diagnostics.synthesizedRoutes > 0 && diagnostics.synthesizedAccepted > 0 && hasNewAgentPlan,
        "ALNS destroy-repair must synthesize and exact-accept a route absent from the input portfolio");

    udon::AlnsOptions proofOptions = options;
    proofOptions.maximumProofGuidedIterations = 6;
    proofOptions.proofUpperBound = udon::OfficialScore{
        config.brand_count() + 1,
        config.brand_count() + 1,
        100,
    };
    udon::AlnsDiagnostics proofDiagnostics;
    const std::vector<udon::MasterCandidate> proofCandidates = improver.improve(
        state,
        udon::MatchLedger{},
        portfolio,
        {*seed},
        proofOptions,
        proofDiagnostics);
    require(
        proofDiagnostics.proofGuidedIterations > 0,
        "proof-guided repair must execute only after the baseline ALNS schedule completes");
    require(
        udon::compare_lexicographic(
            proofCandidates.front().scoreAfterToday,
            candidates.front().scoreAfterToday) >= 0,
        "proof-guided candidate supplementation must not worsen the baseline ALNS leader");
    require(
        std::any_of(
            proofCandidates.begin(),
            proofCandidates.end(),
            [&candidates](const udon::MasterCandidate& candidate) {
                return candidate.stableId == candidates.front().stableId;
            }),
        "proof-guided supplementation must retain the baseline ALNS leader");
}

void test_route_pool_recombines_elite_agent_routes(
    const udon::MatchConfig& config,
    const udon::DayState& sourceState) {
    udon::DayState state = sourceState;
    const auto empty_neighbor = [&config](udon::CellId target, udon::CellId excluded) {
        for (const udon::CellId neighbor :
             config.map.neighbors.at(static_cast<std::size_t>(target))) {
            if (neighbor != udon::kInvalidCell && neighbor != excluded &&
                config.map.terrain.at(static_cast<std::size_t>(neighbor)) == udon::Terrain::Plain &&
                config.spotAtCell.at(static_cast<std::size_t>(neighbor)) == udon::kInvalidSpot) {
                return neighbor;
            }
        }
        return udon::kInvalidCell;
    };
    const auto direction_to = [&config](udon::CellId origin, udon::CellId target) {
        const auto& neighbors = config.map.neighbors.at(static_cast<std::size_t>(origin));
        for (std::int32_t direction = 0; direction < udon::kDirectionCount; ++direction) {
            if (neighbors.at(static_cast<std::size_t>(direction)) == target) {
                return direction;
            }
        }
        return -1;
    };

    const udon::CellId firstTarget = config.spots.at(0).position;
    const udon::CellId secondTarget = config.spots.at(2).position;
    const udon::CellId firstStart = empty_neighbor(firstTarget, udon::kInvalidCell);
    const udon::CellId secondStart = empty_neighbor(secondTarget, firstStart);
    require(
        firstStart != udon::kInvalidCell && secondStart != udon::kInvalidCell,
        "route-pool fixture requires two empty target neighbors");
    state.agents.at(0).position = firstStart;
    state.agents.at(0).fuel = config.fuelLimit;
    state.agents.at(1).position = firstStart;
    state.agents.at(2).position = secondStart;
    state.agents.at(2).fuel = config.fuelLimit;

    const std::int32_t firstDirection = direction_to(firstStart, firstTarget);
    const std::int32_t secondDirection = direction_to(secondStart, secondTarget);
    require(firstDirection >= 0 && secondDirection >= 0, "route-pool fixture targets must be adjacent");
    const std::int32_t daySteps = config.steps_for_day(state.dayNumber);
    const std::int32_t firstMoveSteps =
        config.move_cost(firstStart, state.roadStatuses.at(static_cast<std::size_t>(firstStart))).steps;
    const std::int32_t secondMoveSteps =
        config.move_cost(secondStart, state.roadStatuses.at(static_cast<std::size_t>(secondStart))).steps;
    const udon::AgentPlan firstVisit{
        udon::PlanAction::move(firstDirection),
        udon::PlanAction::wait(daySteps - firstMoveSteps)};
    const udon::AgentPlan secondVisit{
        udon::PlanAction::move(secondDirection),
        udon::PlanAction::wait(daySteps - secondMoveSteps)};
    const udon::AgentPlan firstWait{udon::PlanAction::wait(daySteps)};
    const udon::AgentPlan secondWait{udon::PlanAction::wait(daySteps)};
    const udon::AgentPlan tankerWait{udon::PlanAction::wait(daySteps)};

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    const udon::DayPlan firstElitePlan{{firstVisit, tankerWait, secondWait}};
    const udon::DayPlan secondElitePlan{{firstWait, tankerWait, secondVisit}};
    const udon::SimulationResult firstEliteSimulation = simulator.simulate(state, firstElitePlan, true);
    const udon::SimulationResult secondEliteSimulation = simulator.simulate(state, secondElitePlan, true);
    require(
        firstEliteSimulation.valid && secondEliteSimulation.valid,
        "route-pool elite plans must be valid: first=" +
            (firstEliteSimulation.error.has_value() ? firstEliteSimulation.error->message : "ok") +
            ", second=" +
            (secondEliteSimulation.error.has_value() ? secondEliteSimulation.error->message : "ok"));
    const std::optional<udon::MasterCandidate> firstElite = master.evaluate_exact_plan(
        state,
        udon::MatchLedger{},
        firstElitePlan);
    const std::optional<udon::MasterCandidate> secondElite = master.evaluate_exact_plan(
        state,
        udon::MatchLedger{},
        secondElitePlan);
    require(
        firstElite.has_value() && secondElite.has_value() &&
            firstElite->simulation.score.dailyDistinct == 1 &&
            secondElite->simulation.score.dailyDistinct == 1,
        "route-pool elite seeds must each collect exactly one distinct brand: first=" +
            std::to_string(firstElite.has_value() ? firstElite->simulation.score.dailyDistinct : -1) +
            ", second=" +
            std::to_string(secondElite.has_value() ? secondElite->simulation.score.dailyDistinct : -1));

    udon::RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(static_cast<std::size_t>(config.agent_count()));
    const std::array<udon::AgentPlan, 3> waits{firstWait, tankerWait, secondWait};
    for (udon::AgentIndex agentIndex = 0; agentIndex < config.agent_count(); ++agentIndex) {
        udon::RouteColumn column;
        column.columnId = agentIndex;
        column.agent = agentIndex;
        column.actions = waits.at(static_cast<std::size_t>(agentIndex));
        column.terminalCell = state.agents.at(static_cast<std::size_t>(agentIndex)).position;
        column.terminalFuel = state.agents.at(static_cast<std::size_t>(agentIndex)).fuel;
        portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex)).push_back(std::move(column));
    }

    const udon::ParetoRouter router(config);
    const udon::RouteColumnGenerator generator(config, router);
    udon::RoutePoolAugmentation augmentation = generator.augment_with_candidate_routes(
        state,
        std::move(portfolio),
        {*firstElite, *secondElite},
        4);
    require(
        augmentation.novelRoutes == 2 && augmentation.retainedNovelRoutes == 2,
        "route pool must retain both novel elite agent routes");

    udon::MasterOptions options;
    options.maximumCombinations = 64;
    options.maximumCandidates = 8;
    udon::MasterDiagnostics diagnostics;
    const std::vector<udon::MasterCandidate> recombined = master.solve(
        state,
        udon::MatchLedger{},
        augmentation.portfolio,
        options,
        diagnostics);
    require(
        !recombined.empty() && recombined.front().simulation.score.dailyDistinct == 2,
        "set-packing recombination must combine complementary elite routes into a better exact plan");
}

void test_exact_orienteering_terminal_frontier() {
    const udon::MatchConfig config = udon::parse_match_config(udon::JsonValue::parse(R"({
        "startsAt":1778227200,
        "daySeconds":[5,5,5,5],
        "daySteps":[32,32,32,32],
        "map":{"height":8,"width":8,"cells":[
            [0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0]
        ]},
        "spots":[
            {"brand":10,"pos":18,"stocks":1},
            {"brand":11,"pos":19,"stocks":1},
            {"brand":12,"pos":34,"stocks":1},
            {"brand":13,"pos":35,"stocks":1}
        ],
        "agents":[27,0,63],
        "fuelLimits":64,
        "players":2,
        "busyThreshold":2,
        "jammedThreshold":4
    })"));
    const udon::DayState state = udon::parse_day_state(
        config,
        udon::JsonValue::parse(R"({
            "endsAt":1778227205,
            "day":1,
            "agents":[
                {"kind":0,"pos":27,"fuel":64},
                {"kind":1,"pos":0,"fuel":64},
                {"kind":1,"pos":63,"fuel":64}
            ],
            "others":[],
            "traffics":[]
        })"));
    const udon::ExactOrienteeringReachability reachability =
        udon::enumerate_exact_high_fuel_routes(config, state, 0);
    require(
        reachability.supported && reachability.complete &&
            !reachability.maximalRoutes.empty() &&
            !reachability.terminalVariants.empty(),
        "exact high-fuel reachability must preserve terminal variants separately from maximal masks");
    std::set<udon::CellId> terminalCells;
    const udon::ExactStepSimulator simulator(config);
    const auto verify_route = [&](const udon::ExactOrienteeringRoute& route) {
        udon::DayPlan plan;
        plan.actions = {
            route.actions,
            {udon::PlanAction::wait(32)},
            {udon::PlanAction::wait(32)},
        };
        const udon::SimulationResult simulation = simulator.simulate(state, plan);
        require(simulation.valid, "every exact terminal variant must pass the independent simulator");
        require(
            simulation.finalAgents.front().position == route.terminalCell &&
                state.agents.front().fuel - simulation.finalAgents.front().fuel == route.patrolFuel,
            "exact terminal metadata must equal independently simulated position and fuel");
        std::uint32_t claimedMask = 0;
        for (const udon::ClaimEvent& claim : simulation.claims) {
            claimedMask |= std::uint32_t{1} << static_cast<std::uint32_t>(claim.spot);
        }
        require(
            claimedMask == route.spotMask,
            "exact route witness mask must equal independently simulated claims");
        terminalCells.insert(route.terminalCell);
    };
    for (const udon::ExactOrienteeringRoute& route : reachability.maximalRoutes) {
        verify_route(route);
    }
    for (const udon::ExactOrienteeringRoute& route : reachability.terminalVariants) {
        verify_route(route);
    }
    require(
        reachability.servedSpotFuelRoutes.size() == config.spots.size(),
        "complete exact reachability must expose one fuel-first route for every reachable served spot");
    std::set<udon::CellId> servedSpotTerminals;
    for (const udon::ExactOrienteeringRoute& route :
         reachability.servedSpotFuelRoutes) {
        verify_route(route);
        require(
            route.terminalOnSpot &&
                servedSpotTerminals.insert(route.terminalCell).second,
            "served-spot fuel frontier routes must end on distinct public spot cells");
    }
    require(
        terminalCells.size() >= 2U,
        "terminal frontier must retain distinct end positions for the same exact harvest search");
    require(
        std::any_of(
            terminalCells.begin(),
            terminalCells.end(),
            [&config](udon::CellId cell) {
                return config.map.hex_distance(cell, 0) == 1 ||
                    config.map.hex_distance(cell, 63) == 1;
            }),
        "terminal frontier must retain a same-mask endpoint adjacent to a reachable tanker rendezvous");

    udon::DayState duplicateStartState = state;
    duplicateStartState.agents.at(1) = duplicateStartState.agents.front();
    const udon::ParetoRouter router(config);
    const udon::RouteColumnGenerator generator(config, router);
    udon::ColumnGenerationOptions options;
    options.enableExactHarvestOrienteering = true;
    options.allowUncachedHarvestTargets = true;
    options.maximumPathsPerTarget = 1;
    options.maximumColumnsPerAgent = 4;
    options.maximumTargetSpots = 4;
    udon::ColumnGenerationDiagnostics diagnostics;
    const udon::RoutePortfolio unboundedPortfolio = generator.generate(
        duplicateStartState,
        udon::MatchLedger{},
        options,
        &diagnostics);
    require(
        diagnostics.exactOrienteeringSupportedAgents == 2 &&
            diagnostics.exactOrienteeringCompleteAgents == 2 &&
            diagnostics.exactOrienteeringCacheHits == 1,
        "identical high-fuel patrol starts must reuse exact reachability without changing logical coverage");
    const auto portfolio_action_keys = [](const udon::RoutePortfolio& portfolio) {
        std::vector<std::multiset<std::string>> keys(
            portfolio.columnsByAgent.size());
        for (std::size_t agent = 0;
             agent < portfolio.columnsByAgent.size();
             ++agent) {
            for (const udon::RouteColumn& column :
                 portfolio.columnsByAgent.at(agent)) {
                std::string key;
                for (const udon::PlanAction& action : column.actions) {
                    key += std::to_string(action.wire_value());
                    key.push_back(',');
                }
                keys.at(agent).insert(std::move(key));
            }
        }
        return keys;
    };
    udon::ColumnGenerationOptions generousDeadlineOptions = options;
    generousDeadlineOptions.deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds{10};
    udon::ColumnGenerationDiagnostics generousDeadlineDiagnostics;
    const udon::RoutePortfolio generousDeadlinePortfolio = generator.generate(
        duplicateStartState,
        udon::MatchLedger{},
        generousDeadlineOptions,
        &generousDeadlineDiagnostics);
    require(
        !generousDeadlineDiagnostics.deadlineReached &&
            portfolio_action_keys(generousDeadlinePortfolio) ==
                portfolio_action_keys(unboundedPortfolio),
        "cooperative deadline checks must preserve the byte-equivalent action portfolio when exact work completes in time");

    udon::ColumnGenerationOptions expiredOptions = options;
    expiredOptions.deadline = std::chrono::steady_clock::now();
    udon::ColumnGenerationDiagnostics expiredDiagnostics;
    const auto expiredStarted = std::chrono::steady_clock::now();
    const udon::RoutePortfolio expiredPortfolio = generator.generate(
        duplicateStartState,
        udon::MatchLedger{},
        expiredOptions,
        &expiredDiagnostics);
    const std::chrono::milliseconds expiredElapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - expiredStarted);
    require(
        expiredDiagnostics.deadlineReached &&
            expiredDiagnostics.exactOrienteeringBundles == 0 &&
            expiredElapsed < std::chrono::milliseconds{500},
        "an expired exact-orienteering deadline must cancel finalization promptly without materializing a partial team bundle");
    require(
        std::all_of(
            expiredPortfolio.columnsByAgent.begin(),
            expiredPortfolio.columnsByAgent.end(),
            [](const std::vector<udon::RouteColumn>& columns) {
                return !columns.empty() &&
                    std::none_of(
                        columns.begin(),
                        columns.end(),
                        [](const udon::RouteColumn& column) {
                            return column.exactOrienteering;
                        });
            }),
        "deadline cancellation must retain the exact-valid regular fallback portfolio and omit unfinished exact bundles");

    udon::UdonShieldEngine officialFuelEngine(
        config,
        {},
        {},
        udon::RoutePoolSearch::SinglePass,
        7,
        false,
        7);
    const udon::DecisionResult officialFuelDecision = officialFuelEngine.solve_day(
        state,
        udon::MatchLedger{},
        std::chrono::milliseconds{5000});
    require(
        officialFuelDecision.audit.columnGeneration.exactOrienteeringSupportedAgents == 1 &&
            officialFuelDecision.audit.columnGeneration.exactOrienteeringCompleteAgents == 1,
        "the runtime must enable exact reachability at its proven two-times-steps fuel threshold");

    udon::DayState depletedFinalState = state;
    depletedFinalState.dayNumber = config.day_count();
    depletedFinalState.agents.front().fuel = 48;
    udon::UdonShieldEngine depletedFuelEngine(
        config,
        {},
        {},
        udon::RoutePoolSearch::SinglePass,
        7,
        false,
        7);
    const udon::DecisionResult depletedFuelDecision =
        depletedFuelEngine.solve_day(
            depletedFinalState,
            udon::MatchLedger{},
            std::chrono::milliseconds{5000});
    require(
        depletedFuelDecision.audit.columnGeneration
                .exactOrienteeringSupportedAgents == 1 &&
            depletedFuelDecision.audit.columnGeneration
                .exactOrienteeringCompleteAgents == 0,
        "final-day exact routing must use each patrol's current fuel rather than the match fuel limit");

    udon::MatchConfig lowFuelConfig = config;
    lowFuelConfig.fuelLimit = 48;
    udon::DayState lowFuelFinalState = state;
    lowFuelFinalState.dayNumber = lowFuelConfig.day_count();
    for (udon::AgentState& agent : lowFuelFinalState.agents) {
        agent.fuel = lowFuelConfig.fuelLimit;
    }
    const udon::ExactOrienteeringReachability fullLowFuel =
        udon::enumerate_exact_resource_routes(
            lowFuelConfig,
            lowFuelFinalState,
            0);
    const udon::ExactOrienteeringReachability anytimeLowFuel =
        udon::enumerate_anytime_resource_routes(
            lowFuelConfig,
            lowFuelFinalState,
            0,
            3,
            8,
            100000);
    require(
        fullLowFuel.supported && fullLowFuel.complete &&
            anytimeLowFuel.supported &&
            !anytimeLowFuel.maximalRoutes.empty(),
        "low-fuel anytime orienteering must emit exact-feasible routes without requiring a full proof");
    require(
        !fullLowFuel.servedSpotFuelRoutes.empty() &&
            fullLowFuel.servedSpotFuelRoutes.size() <=
                lowFuelConfig.spots.size(),
        "complete low-fuel reachability must keep the served-spot frontier within the public spot bound");
    const auto maximum_spots = [](const udon::ExactOrienteeringReachability& exact) {
        std::int32_t maximum = 0;
        for (const udon::ExactOrienteeringRoute& route : exact.maximalRoutes) {
            maximum = std::max(
                maximum,
                static_cast<std::int32_t>(
                    std::popcount(route.spotMask)));
        }
        return maximum;
    };
    require(
        maximum_spots(anytimeLowFuel) == maximum_spots(fullLowFuel),
        "a sufficient anytime state budget must recover the full low-fuel spot cardinality");
    const udon::ExactStepSimulator lowFuelSimulator(lowFuelConfig);
    for (const udon::ExactOrienteeringRoute& route :
         fullLowFuel.servedSpotFuelRoutes) {
        udon::DayPlan plan;
        plan.actions = {
            route.actions,
            {udon::PlanAction::wait(32)},
            {udon::PlanAction::wait(32)},
        };
        const udon::SimulationResult simulation =
            lowFuelSimulator.simulate(lowFuelFinalState, plan);
        require(
            simulation.valid && route.terminalOnSpot &&
                simulation.finalAgents.front().position ==
                    route.terminalCell &&
                lowFuelFinalState.agents.front().fuel -
                        simulation.finalAgents.front().fuel ==
                    route.patrolFuel,
            "every complete low-fuel served-spot route must preserve exact terminal semantics");
    }
    for (const udon::ExactOrienteeringRoute& route :
         anytimeLowFuel.maximalRoutes) {
        udon::DayPlan plan;
        plan.actions = {
            route.actions,
            {udon::PlanAction::wait(32)},
            {udon::PlanAction::wait(32)},
        };
        const udon::SimulationResult simulation =
            lowFuelSimulator.simulate(lowFuelFinalState, plan);
        require(
            simulation.valid &&
                simulation.finalAgents.front().position ==
                    route.terminalCell &&
                lowFuelFinalState.agents.front().fuel -
                        simulation.finalAgents.front().fuel ==
                    route.patrolFuel,
            "every low-fuel anytime route must preserve exact terminal semantics");
    }
    for (std::int32_t trial = 0; trial < 16; ++trial) {
        udon::MatchConfig trialConfig = lowFuelConfig;
        trialConfig.roadCells.clear();
        for (udon::CellId cell = 0;
             cell < trialConfig.map.cell_count();
             ++cell) {
            udon::Terrain terrain = udon::Terrain::Plain;
            if ((cell + trial) % 7 == 0) {
                terrain = udon::Terrain::Mountain;
            } else if ((3 * cell + trial) % 11 == 0) {
                terrain = udon::Terrain::Road;
                trialConfig.roadCells.push_back(cell);
            }
            trialConfig.map.terrain.at(static_cast<std::size_t>(cell)) =
                terrain;
        }
        udon::DayState trialState = lowFuelFinalState;
        trialState.agents.front().position =
            (11 * trial + 3) % trialConfig.map.cell_count();
        trialState.agents.front().fuel = 8 + (7 * trial) % 41;
        for (std::size_t cell = 0;
             cell < trialState.roadStatuses.size();
             ++cell) {
            trialState.roadStatuses.at(cell) =
                static_cast<udon::RoadStatus>((cell + trial) % 3);
        }
        const udon::ExactOrienteeringReachability trialFull =
            udon::enumerate_exact_resource_routes(
                trialConfig,
                trialState,
                0);
        const udon::ExactOrienteeringReachability trialAnytime =
            udon::enumerate_anytime_resource_routes(
                trialConfig,
                trialState,
                0,
                1,
                64,
                1000000);
        require(
            trialFull.supported && trialFull.complete &&
                trialAnytime.supported &&
                maximum_spots(trialAnytime) ==
                    maximum_spots(trialFull),
            "bounded exact-state routing must match the full oracle on deterministic mixed-terrain fixtures");
        const udon::ExactStepSimulator trialSimulator(trialConfig);
        for (const udon::ExactOrienteeringRoute& route :
             trialAnytime.maximalRoutes) {
            udon::DayPlan plan;
            plan.actions = {
                route.actions,
                {udon::PlanAction::wait(32)},
                {udon::PlanAction::wait(32)},
            };
            const udon::SimulationResult simulation =
                trialSimulator.simulate(trialState, plan);
            std::uint32_t claimedMask = 0U;
            for (const udon::ClaimEvent& claim : simulation.claims) {
                if (claim.agent == 0) {
                    claimedMask |= std::uint32_t{1} <<
                        static_cast<std::uint32_t>(claim.spot);
                }
            }
            require(
                simulation.valid &&
                    claimedMask == route.spotMask &&
                    simulation.finalAgents.front().position ==
                        route.terminalCell &&
                    trialState.agents.front().fuel -
                            simulation.finalAgents.front().fuel ==
                        route.patrolFuel,
                "bounded exact-state routes must match independent simulation across mixed terrain and traffic");
        }
    }
    udon::UdonShieldEngine lowFuelEngine(
        lowFuelConfig,
        {},
        {},
        udon::RoutePoolSearch::SinglePass,
        7,
        false,
        7);
    const udon::DecisionResult lowFuelFinalDecision =
        lowFuelEngine.solve_day(
            lowFuelFinalState,
            udon::MatchLedger{},
            std::chrono::milliseconds{5000});
    require(
        lowFuelFinalDecision.audit.columnGeneration
                .exactOrienteeringSupportedAgents == 1,
        "mode seven must enable bounded exact-state low-fuel routes on the final day");
    udon::DayState lowFuelNonFinalState = lowFuelFinalState;
    lowFuelNonFinalState.dayNumber = 1;
    udon::UdonShieldEngine lowFuelNonFinalEngine(
        lowFuelConfig,
        {},
        {},
        udon::RoutePoolSearch::SinglePass,
        7,
        false,
        7);
    const udon::DecisionResult lowFuelNonFinalDecision =
        lowFuelNonFinalEngine.solve_day(
            lowFuelNonFinalState,
            udon::MatchLedger{},
            std::chrono::milliseconds{5000});
    require(
        lowFuelNonFinalDecision.audit.columnGeneration
                .exactOrienteeringSupportedAgents == 0,
        "bounded low-fuel routes must not consume search budget before the final day");
}

void test_anytime_orienteering_preserves_lexicographic_brands() {
    const udon::MatchConfig config = udon::parse_match_config(
        udon::JsonValue::parse(R"({
            "startsAt":1778227200,
            "daySeconds":[5,5,5,5],
            "daySteps":[18,18,18,18],
            "map":{"height":8,"width":8,"cells":[
                [0,2,2,2,2,2,2,2],
                [2,2,2,2,2,2,2,2],
                [2,2,2,2,2,2,2,2],
                [2,2,2,0,0,0,2,2],
                [2,2,2,0,0,2,2,2],
                [2,2,2,2,2,2,2,2],
                [2,2,2,2,2,2,2,2],
                [0,2,2,2,2,2,2,2]
            ]},
            "spots":[
                {"brand":10,"pos":28,"stocks":3},
                {"brand":10,"pos":29,"stocks":3},
                {"brand":11,"pos":56,"stocks":3}
            ],
            "agents":[27,35,36],
            "fuelLimits":20,
            "players":2,
            "busyThreshold":2,
            "jammedThreshold":4
        })"));
    const udon::DayState state = udon::parse_day_state(
        config,
        udon::JsonValue::parse(R"({
            "endsAt":1778227205,
            "day":4,
            "agents":[
                {"kind":0,"pos":27,"fuel":20},
                {"kind":0,"pos":35,"fuel":20},
                {"kind":0,"pos":36,"fuel":20}
            ],
            "others":[],
            "traffics":[]
        })"));
    const auto route_brands = [&config](
                                  const udon::ExactOrienteeringRoute& route) {
        udon::BrandMask brands;
        for (std::size_t spot = 0; spot < config.spots.size(); ++spot) {
            if ((route.spotMask & (std::uint32_t{1} << spot)) != 0U) {
                brands |= udon::brand_bit(config.spots.at(spot).brandIndex);
            }
        }
        return brands;
    };
    const udon::ExactOrienteeringReachability diverse =
        udon::enumerate_anytime_resource_routes(
            config,
            state,
            0,
            1,
            2,
            100000,
            std::nullopt,
            udon::brand_bit(0) | udon::brand_bit(1));
    udon::BrandMask retainedBrands;
    for (const udon::ExactOrienteeringRoute& route :
         diverse.maximalRoutes) {
        retainedBrands |= route_brands(route);
    }
    for (const udon::ExactOrienteeringRoute& route :
         diverse.supplementalRoutes) {
        retainedBrands |= route_brands(route);
    }
    require(
        diverse.supported &&
            diverse.maximalRoutes.size() == 2U &&
            udon::has_brand(retainedBrands, 0) &&
            udon::has_brand(retainedBrands, 1),
        "the legacy-plus-supplemental frontier must preserve every reachable brand");

    const udon::ExactOrienteeringReachability legacy =
        udon::enumerate_anytime_resource_routes(
            config,
            state,
            0,
            1,
            1,
            100000);
    const udon::ExactOrienteeringReachability preferred =
        udon::enumerate_anytime_resource_routes(
            config,
            state,
            0,
            1,
            1,
            100000,
            std::nullopt,
            udon::brand_bit(1));
    require(
        preferred.supported &&
            preferred.maximalRoutes.size() == 1U &&
            preferred.supplementalRoutes.size() == 1U &&
            udon::has_brand(
                route_brands(preferred.supplementalRoutes.front()),
                1) &&
            legacy.maximalRoutes.size() == 1U &&
            preferred.maximalRoutes.front().spotMask ==
                legacy.maximalRoutes.front().spotMask &&
            preferred.maximalRoutes.front().usedSteps ==
                legacy.maximalRoutes.front().usedSteps &&
            preferred.maximalRoutes.front().patrolFuel ==
                legacy.maximalRoutes.front().patrolFuel &&
            preferred.maximalRoutes.front().terminalCell ==
                legacy.maximalRoutes.front().terminalCell &&
            preferred.maximalRoutes.front().actions.size() ==
                legacy.maximalRoutes.front().actions.size() &&
            std::equal(
                preferred.maximalRoutes.front().actions.begin(),
                preferred.maximalRoutes.front().actions.end(),
                legacy.maximalRoutes.front().actions.begin(),
                [](const udon::PlanAction& left,
                   const udon::PlanAction& right) {
                    return left.kind == right.kind &&
                        left.value == right.value;
                }),
        "a one-slot supplemental frontier must preserve the missing lifetime brand without replacing the legacy route");
    const udon::ExactStepSimulator simulator(config);
    for (const udon::ExactOrienteeringRoute& route :
         preferred.supplementalRoutes) {
        udon::DayPlan plan;
        plan.actions = {
            route.actions,
            {udon::PlanAction::wait(18)},
            {udon::PlanAction::wait(18)},
        };
        require(
            simulator.simulate(state, plan).valid,
            "lexicographic route retention must not alter exact action feasibility");
    }
}

void test_emergency_contract(const udon::MatchConfig& config, const udon::DayState& state) {
    udon::UdonShieldEngine engine(config);
    const udon::DecisionResult decision = engine.solve_day(state, udon::MatchLedger{}, std::chrono::milliseconds{1});
    require(decision.emergency, "insufficient deadline must select emergency profile");
    const udon::ExactStepSimulator simulator(config);
    const udon::SimulationResult result = simulator.simulate(state, decision.candidate.plan, false);
    require(result.valid, "emergency wait plan must be valid");
    engine.record_submitted(decision, std::chrono::milliseconds{100});
    require(
        !engine.may_submit(decision) &&
            engine.response_ledger().totalResponse == std::chrono::milliseconds{100},
        "an acknowledged duplicate response must be suppressed before it can worsen response timing");

    udon::UdonShieldEngine absoluteDeadlineEngine(config);
    const std::chrono::system_clock::time_point receivedAt =
        std::chrono::system_clock::time_point{std::chrono::seconds{state.endsAt}} - std::chrono::milliseconds{1};
    const udon::DecisionResult absoluteDeadline = absoluteDeadlineEngine.solve_day_until(
        state,
        udon::MatchLedger{},
        receivedAt);
    require(
        absoluteDeadline.deadline.total == std::chrono::milliseconds{1} && absoluteDeadline.emergency,
        "absolute endsAt conversion must freeze the same emergency profile as an explicit millisecond budget");

    const udon::DecisionResult improved = engine.solve_day(
        state,
        udon::MatchLedger{},
        std::chrono::milliseconds{1000});
    require(
        engine.may_submit(improved),
        "a same-day exact certified score upgrade must remain eligible for resend");
    engine.record_submitted(improved, std::chrono::milliseconds{40});
    require(
        engine.response_ledger().totalResponse == std::chrono::milliseconds{40},
        "same-day resend must replace, not add to, cumulative response time");

    udon::DayState dayTwo = state;
    dayTwo.dayNumber = 2;
    dayTwo.agents = improved.candidate.simulation.finalAgents;
    udon::MatchLedger dayTwoLedger;
    dayTwoLedger.apply(improved.candidate.simulation.score);
    const udon::DecisionResult dayTwoDecision = engine.solve_day(
        dayTwo,
        dayTwoLedger,
        std::chrono::milliseconds{1});
    engine.record_submitted(dayTwoDecision, std::chrono::milliseconds{30});
    require(
        engine.response_ledger().totalResponse == std::chrono::milliseconds{70},
        "response ledger must sum the last valid response time of each day");
}

void test_competition_compute_hard_cap(
    const udon::MatchConfig& config,
    const udon::DayState& state) {
    require(
        udon::competition_compute_budget(std::chrono::milliseconds{-1}) ==
                std::chrono::milliseconds{0} &&
            udon::competition_compute_budget(std::chrono::milliseconds{1}) ==
                std::chrono::milliseconds{1} &&
            udon::competition_compute_budget(std::chrono::milliseconds{500}) ==
                std::chrono::milliseconds{500} &&
            udon::competition_compute_budget(std::chrono::milliseconds{1200}) ==
                std::chrono::milliseconds{1200} &&
            udon::competition_compute_budget(std::chrono::milliseconds{2500}) ==
                std::chrono::milliseconds{2500} &&
            udon::competition_compute_budget(std::chrono::milliseconds{5000}) ==
                udon::kCompetitionComputeHardCap &&
            udon::competition_compute_budget(std::chrono::milliseconds{60000}) ==
                udon::kCompetitionComputeHardCap,
        "the canonical compute budget must preserve every protected lane and clamp only oversized requests");

    udon::UdonShieldEngine directEngine(config);
    const udon::DecisionResult direct = directEngine.solve_day(
        state,
        udon::MatchLedger{},
        std::chrono::milliseconds{60000});
    require(
        direct.deadline.total == udon::kCompetitionComputeHardCap,
        "direct solver callers must not turn a 60000-ms outer window into extra compute");

    udon::DayState outerWindowState = state;
    const std::chrono::system_clock::time_point receivedAt =
        std::chrono::system_clock::time_point{
            std::chrono::seconds{outerWindowState.endsAt}} -
        std::chrono::milliseconds{60000};
    udon::UdonShieldEngine absoluteEngine(config);
    const udon::DecisionResult absolute = absoluteEngine.solve_day_until(
        outerWindowState,
        udon::MatchLedger{},
        receivedAt);
    require(
        absolute.deadline.total == udon::kCompetitionComputeHardCap,
        "absolute server deadlines must retain the same 5000-ms internal cap");

    udon::MatchSession session(config);
    const udon::SessionDecision sessionDecision = session.on_authoritative_state_for(
        state,
        udon::MatchLedger{},
        std::chrono::milliseconds{60000});
    require(
        sessionDecision.decision.deadline.total == udon::kCompetitionComputeHardCap,
        "the canonical match session must enforce the engine cap for every transport");
    static_cast<void>(session.acknowledge_submitted(std::chrono::milliseconds{10}));
    const std::chrono::milliseconds backgroundBudget =
        session.remaining_post_ack_compute_budget();
    require(
        backgroundBudget >= std::chrono::milliseconds{0} &&
            backgroundBudget <= udon::kCompetitionComputeHardCap &&
            backgroundBudget +
                    udon::competition_compute_budget(
                        sessionDecision.decision.timing.total) ==
                udon::kCompetitionComputeHardCap,
        "post-ACK search must receive only the unused part of the same daily compute cap");
}

void test_protected_slack_refiner(
    const udon::MatchConfig& config,
    const udon::DayState& state) {
    const udon::MatchLedger acknowledgementBaseline{
        1ULL << 0,
        10,
        100,
    };
    const udon::MatchLedger lexicographicallyBetterFinal{
        (1ULL << 0) | (1ULL << 1),
        9,
        90,
    };
    require(
        !udon::protected_slack_ledger_relation_for_day(
            acknowledgementBaseline,
            lexicographicallyBetterFinal,
            false) &&
            udon::protected_slack_ledger_relation_for_day(
                acknowledgementBaseline,
                lexicographicallyBetterFinal,
                true) &&
            !udon::protected_slack_ledger_relation_for_day(
                lexicographicallyBetterFinal,
                acknowledgementBaseline,
                true),
        "an acknowledged nonterminal continuation must remain componentwise safe, while the final day must follow official lexicographic order in both directions");
    udon::DayPlan parent;
    parent.actions.assign(
        static_cast<std::size_t>(config.agent_count()),
        udon::AgentPlan{
            udon::PlanAction::wait(config.steps_for_day(state.dayNumber))});
    const udon::ExactStepSimulator simulator(config);
    const udon::SimulationResult parentSimulation =
        simulator.simulate(state, parent, false);
    require(parentSimulation.valid, "protected-slack parent must be valid");
    const udon::ProtectedSlackRefiner refiner(config);
    const udon::ProtectedSlackResult refined = refiner.refine_wait_detours(
        state,
        udon::MatchLedger{},
        parent,
        parentSimulation,
        std::chrono::steady_clock::now() + std::chrono::milliseconds{500});
    require(
        refined.improved &&
            udon::protected_slack_transition_dominates(
                parentSimulation,
                refined.simulation) &&
            parentSimulation.score.dailyDistinct <=
                refined.simulation.score.dailyDistinct &&
            parentSimulation.score.servings <=
                refined.simulation.score.servings,
        "protected WAIT detours must produce only an exact componentwise improvement with the parent terminal transition preserved");

    const udon::ProtectedSlackResult expired = refiner.refine_wait_detours(
        state,
        udon::MatchLedger{},
        parent,
        parentSimulation,
        std::chrono::steady_clock::now() - std::chrono::milliseconds{1});
    require(
        !expired.improved && expired.diagnostics.deadlineReached &&
            udon::canonical_plan_bytes(expired.plan) ==
                udon::canonical_plan_bytes(parent),
        "an expired protected-slack budget must return the byte-identical parent plan");
    const udon::ProtectedSlackResult nonterminalSparse =
        refiner.refine_terminal_sparse(
            state,
            udon::MatchLedger{},
            parent,
            parentSimulation,
            std::chrono::steady_clock::now() +
                std::chrono::milliseconds{500});
    require(
        !nonterminalSparse.improved &&
            !nonterminalSparse.diagnostics.terminalSparse &&
            nonterminalSparse.diagnostics.terminalSparseRounds == 0 &&
            udon::canonical_plan_bytes(nonterminalSparse.plan) ==
                udon::canonical_plan_bytes(parent),
        "terminal sparse refinement must be unreachable before the final day and preserve the byte-identical parent");

    // SCORE-MIDDAY-CHAIN-ADOPTION-210 / SCORE-MIDDAY-PAIR-EXCHANGE-211.
    const udon::ProtectedSlackResult middayFlagOff = refiner.refine_midday_chains(
        state,
        udon::MatchLedger{},
        parent,
        parentSimulation,
        std::chrono::steady_clock::now() + std::chrono::milliseconds{500});
    require(
        !middayFlagOff.improved && !middayFlagOff.diagnostics.middayChain &&
            middayFlagOff.diagnostics.middayRoutes == 0 &&
            udon::canonical_plan_bytes(middayFlagOff.plan) ==
                udon::canonical_plan_bytes(parent),
        "the mid-day chain lane must be byte-inert while its flag is off");
    udon::ProtectedSlackRefiner middayRefiner(config);
    middayRefiner.enableMiddayChainAdoption = true;
    middayRefiner.enableMiddayPairExchange = true;
    const udon::ProtectedSlackResult middayExpired =
        middayRefiner.refine_midday_chains(
            state,
            udon::MatchLedger{},
            parent,
            parentSimulation,
            std::chrono::steady_clock::now() - std::chrono::milliseconds{1});
    require(
        !middayExpired.improved && middayExpired.diagnostics.deadlineReached &&
            udon::canonical_plan_bytes(middayExpired.plan) ==
                udon::canonical_plan_bytes(parent),
        "an expired mid-day budget must return the byte-identical parent plan");
    const udon::ProtectedSlackResult middayWaitParent =
        middayRefiner.refine_midday_chains(
            state,
            udon::MatchLedger{},
            parent,
            parentSimulation,
            std::chrono::steady_clock::now() + std::chrono::milliseconds{900});
    require(
        !middayWaitParent.improved &&
            middayWaitParent.diagnostics.middayChain &&
            !middayWaitParent.diagnostics.middayTargetFollowup &&
            udon::canonical_plan_bytes(middayWaitParent.plan) ==
                udon::canonical_plan_bytes(parent),
        "the certificate must reject every fuel-spending challenger against a full-fuel WAIT parent (fuel >= is unsatisfiable)");
    middayRefiner.enableMiddayTargetTerminalFollowup = true;
    const udon::ProtectedSlackResult middayTargetWaitParent =
        middayRefiner.refine_midday_chains(
            state,
            udon::MatchLedger{},
            parent,
            parentSimulation,
            std::chrono::steady_clock::now() +
                std::chrono::milliseconds{900});
    require(
        middayTargetWaitParent.diagnostics.middayTargetFollowup &&
            ((!middayTargetWaitParent.improved &&
              udon::canonical_plan_bytes(middayTargetWaitParent.plan) ==
                  udon::canonical_plan_bytes(parent)) ||
             (middayTargetWaitParent.improved &&
              udon::protected_slack_transition_dominates(
                  parentSimulation,
                  middayTargetWaitParent.simulation) &&
              (udon::OfficialScore::after_day(
                   udon::MatchLedger{},
                   parentSimulation.score) <
               middayTargetWaitParent.scoreAfterToday))),
        "the target-terminal suffix must either retain its certified global-prefix incumbent or prove a strict protected improvement");
    udon::UdonShieldEngine middayEngine(config);
    const udon::DecisionResult middayIncumbent = middayEngine.solve_day(
        state,
        udon::MatchLedger{},
        std::chrono::milliseconds{800});
    const udon::SimulationResult incumbentSimulation = simulator.simulate(
        state,
        middayIncumbent.candidate.plan,
        false);
    if (incumbentSimulation.valid) {
        const udon::ProtectedSlackResult middayLive =
            middayRefiner.refine_midday_chains(
                state,
                udon::MatchLedger{},
                middayIncumbent.candidate.plan,
                incumbentSimulation,
                std::chrono::steady_clock::now() +
                    std::chrono::milliseconds{1500});
        if (middayLive.improved) {
            require(
                udon::protected_slack_transition_dominates(
                    incumbentSimulation,
                    middayLive.simulation) &&
                    middayLive.simulation.score.dailyDistinct >=
                        incumbentSimulation.score.dailyDistinct &&
                    middayLive.simulation.score.servings >=
                        incumbentSimulation.score.servings &&
                    (middayLive.simulation.score.dailyDistinct >
                         incumbentSimulation.score.dailyDistinct ||
                     middayLive.simulation.score.servings >
                         incumbentSimulation.score.servings) &&
                    !udon::brand_difference(
                         incumbentSimulation.score.brands,
                         middayLive.simulation.score.brands).any(),
                "every mid-day acceptance must satisfy the full future-domain dominance certificate with a strict lexicographic day gain");
            const udon::ProtectedSlackResult middayFixedPoint =
                middayRefiner.refine_midday_chains(
                    state,
                    udon::MatchLedger{},
                    middayLive.plan,
                    middayLive.simulation,
                    std::chrono::steady_clock::now() +
                        std::chrono::milliseconds{1500});
            require(
                !middayFixedPoint.improved ||
                    udon::protected_slack_transition_dominates(
                        middayLive.simulation,
                        middayFixedPoint.simulation),
                "re-refining a mid-day fixed point may only ever produce another certified improvement");
        }
    }
}

void test_deadline_floors() {
    const udon::DeadlineCalibration defaults;
    require(
        defaults.normalProofGuidedPasses == 2 && defaults.longProofGuidedPasses == 4,
        "production proof-guided scales must retain the safety-gated 2x/4x defaults");
    const udon::DeadlineScheduler scheduler;
    const udon::DeadlineProfile exactFloors = scheduler.classify(std::chrono::milliseconds{105});
    require(exactFloors.deadlineClass == udon::DeadlineClass::Emergency, "floor-only budget must use emergency");
    require(exactFloors.meetsMinimumFloors, "exact floor budget must be marked feasible");
    require(
        exactFloors.seed + exactFloors.fastViability + exactFloors.search + exactFloors.certification +
                exactFloors.network ==
            exactFloors.total,
        "emergency profile must account for every millisecond");
    require(
        exactFloors.seed >= std::chrono::milliseconds{30} &&
            exactFloors.certification >= std::chrono::milliseconds{25} &&
            exactFloors.network >= std::chrono::milliseconds{50},
        "emergency profile must preserve all minimum floors when they fit");

    const udon::DeadlineProfile normal = scheduler.classify(std::chrono::milliseconds{1000});
    require(normal.deadlineClass == udon::DeadlineClass::Normal, "one-second budget must use normal profile");
    require(normal.meetsMinimumFloors && normal.search > std::chrono::milliseconds{0},
            "normal profile must retain a nonzero search window after floors");
    require(
        scheduler.classify(defaults.shortThreshold).deadlineClass ==
                udon::DeadlineClass::Short &&
            scheduler.classify(defaults.normalThreshold).deadlineClass ==
                udon::DeadlineClass::Normal,
        "exact deadline thresholds must remain in the lower class so sub-millisecond "
        "rounding cannot switch candidate limits");
    require(
        normal.searchSoft > std::chrono::milliseconds{0} &&
            normal.searchSoft <= normal.search &&
            !normal.p99Calibrated && !normal.competitionReady,
        "uncalibrated research profile must expose a bounded soft cutoff without claiming readiness");
    require(
        normal.seed + normal.fastViability + normal.search + normal.certification + normal.network == normal.total,
        "normal profile must account for exactly 100 percent of the budget");
    for (const std::chrono::milliseconds available :
         {std::chrono::milliseconds{300}, std::chrono::milliseconds{6000}}) {
        const udon::DeadlineProfile profile = scheduler.classify(available);
        require(
            profile.seed + profile.fastViability + profile.search +
                    profile.certification + profile.network ==
                profile.total,
            "every non-emergency deadline class must account for exactly 100 percent of its budget");
        require(
            profile.searchSoft <= profile.search,
            "a calibrated soft cutoff cannot consume validation or network reserve");
    }

    const udon::DeadlineProfile impossible = scheduler.classify(std::chrono::milliseconds{100});
    require(!impossible.meetsMinimumFloors, "sub-floor budget must fail the readiness gate");

    udon::DeadlineCalibration strict;
    strict.requireCompetitionReadiness = true;
    require_throws(
        [&strict]() {
            static_cast<void>(udon::DeadlineScheduler(strict));
        },
        "competition mode must fail startup without measured p99 calibration");
    strict.p99Calibrated = true;
    strict.version = "fixture-p99-v1";
    const udon::DeadlineScheduler calibrated(strict);
    require(
        calibrated.competition_ready() &&
            calibrated.classify(std::chrono::milliseconds{1000}).competitionReady,
        "a frozen measured p99 artifact must open the explicit readiness gate");

    udon::DeadlineCalibration invalidProofScale;
    invalidProofScale.normalProofGuidedPasses = -1;
    require_throws(
        [&invalidProofScale]() {
            static_cast<void>(udon::DeadlineScheduler(invalidProofScale));
        },
        "negative proof-guided calibration must fail startup");
}

void test_public_traffic_scenarios(const udon::MatchConfig& config, const udon::DayState& state) {
    const udon::ScenarioGenerator generator(config);
    udon::TrafficBelief noOpponentBelief(config);
    noOpponentBelief.observe(state);
    const udon::ScenarioManifest noOpponent = generator.freeze_manifest(state, noOpponentBelief);
    const udon::ScenarioManifest repeatedNoOpponent = generator.freeze_manifest(state, noOpponentBelief);
    const auto fallback = std::find_if(
        noOpponent.scenarios.begin(),
        noOpponent.scenarios.end(),
        [](const udon::TrafficScenario& scenario) { return scenario.pessimisticFallback; });
    require(
        noOpponent.scenarios.size() == 2U && fallback != noOpponent.scenarios.end() &&
            !fallback->jointFeasible &&
            fallback->opponentCurrentFootprint == std::vector<std::int32_t>(
                static_cast<std::size_t>(config.map.cell_count()), 0),
        "missing public adversary evidence must use a bound-only fallback instead of inventing a route");
    require(
        !noOpponent.requiredClassesCovered && !noOpponent.survivalSignatureEnabled &&
            noOpponent.totalWeight == 10000U,
        "a missing required class must disable the survival signature while retaining complete mass");
    require(
        noOpponent.scenarios == repeatedNoOpponent.scenarios &&
            noOpponent.totalWeight == repeatedNoOpponent.totalWeight &&
            noOpponent.version == repeatedNoOpponent.version,
        "a frozen scenario manifest must be deterministic for identical public state and belief");

    udon::DayState reachableOpponent = state;
    reachableOpponent.others.push_back(udon::OtherTeamState{
        1,
        {udon::AgentState{udon::AgentKind::Patrol, 0, config.fuelLimit}},
    });
    udon::TrafficBelief reachableBelief(config);
    reachableBelief.observe(reachableOpponent);
    const udon::ScenarioManifest reachable = generator.freeze_manifest(reachableOpponent, reachableBelief);
    const auto adversarial = std::find_if(
        reachable.scenarios.begin(),
        reachable.scenarios.end(),
        [](const udon::TrafficScenario& scenario) {
            return scenario.adversarial && !scenario.pessimisticFallback;
        });
    require(adversarial != reachable.scenarios.end(), "reachable opponent road dwell must produce an adversarial scenario");
    require(
        adversarial->jointFeasible && !adversarial->construction.empty(),
        "weighted adversarial scenarios must come from one jointly feasible opponent assignment");
    require(adversarial->opponentCurrentFootprint.at(0) == config.steps_for_day(1),
            "adversarial dwell must include all legal road stay-steps from a road start");

    udon::DayState fuelBlocked = reachableOpponent;
    fuelBlocked.others.front().agents.front().position = 2;
    fuelBlocked.others.front().agents.front().fuel = 0;
    udon::TrafficBelief blockedBelief(config);
    blockedBelief.observe(fuelBlocked);
    const udon::ScenarioManifest blocked = generator.freeze_manifest(fuelBlocked, blockedBelief);
    require(
        std::none_of(
            blocked.scenarios.begin(),
            blocked.scenarios.end(),
            [](const udon::TrafficScenario& scenario) {
                return scenario.adversarial && !scenario.pessimisticFallback;
            }),
        "fuel-impossible patrol must not create a public adversarial dwell scenario");

    const std::vector<udon::TrafficScenario> privateSensitivity = generator.private_sensitivity_scenarios(
        reachableOpponent,
        reachableBelief,
        {0});
    require(
        privateSensitivity.size() == 1U && privateSensitivity.front().privateSensitivity &&
            privateSensitivity.front().jointFeasible &&
            privateSensitivity.front().weight == 0U &&
            privateSensitivity.front().opponentCurrentFootprint.at(0) == config.steps_for_day(1),
        "private-route sensitivity must remain an unweighted exogenous channel");
    const udon::ScenarioManifest reachableAfterSensitivity = generator.freeze_manifest(
        reachableOpponent,
        reachableBelief);
    require(
        reachableAfterSensitivity.totalWeight == reachable.totalWeight &&
            reachableAfterSensitivity.scenarios.size() == reachable.scenarios.size() &&
            std::equal(
                reachable.scenarios.begin(),
                reachable.scenarios.end(),
                reachableAfterSensitivity.scenarios.begin(),
                [](const udon::TrafficScenario& left, const udon::TrafficScenario& right) {
                    return left.scenarioId == right.scenarioId && left.scenarioClass == right.scenarioClass &&
                        left.weight == right.weight && left.adversarial == right.adversarial &&
                        left.opponentCurrentFootprint == right.opponentCurrentFootprint &&
                        left.opponentCarryFootprint == right.opponentCarryFootprint;
                }),
        "private sensitivity must not mutate the frozen public scenario manifest");

    udon::MatchConfig roadlessConfig = config;
    for (const udon::CellId road : roadlessConfig.roadCells) {
        roadlessConfig.map.terrain.at(static_cast<std::size_t>(road)) = udon::Terrain::Plain;
    }
    roadlessConfig.roadCells.clear();
    udon::TrafficBelief roadlessBelief(roadlessConfig);
    roadlessBelief.observe(state);
    const udon::ScenarioManifest roadless = udon::ScenarioGenerator(roadlessConfig).freeze_manifest(
        state,
        roadlessBelief);
    require(
        roadless.scenarios.size() == 1U && roadless.totalWeight == 10000U &&
            !roadless.usedStaticFallback && !roadless.survivalSignatureEnabled,
        "a roadless map must freeze one complete deterministic manifest without a false fallback");
    require(
        roadless.scenarios.front().scenarioClass == "deterministic-no-road" &&
            roadless.scenarios.front().weight == 10000U &&
            !roadless.scenarios.front().pessimisticFallback &&
            roadless.scenarios.front().jointFeasible,
        "the roadless scenario must be exact public state, not missing-adversary uncertainty");
}

void test_same_day_resend_preserves_prior_traffic_memory(
    const udon::MatchConfig& config,
    const udon::DayState& state) {
    udon::TrafficBelief belief(config);
    udon::DayState dayOne = state;
    dayOne.dayNumber = 1;
    belief.observe(dayOne);
    std::vector<std::int32_t> dayOneFootprint(static_cast<std::size_t>(config.map.cell_count()), 0);
    dayOneFootprint.at(0) = 3;
    belief.record_own_footprint(1, dayOneFootprint);

    udon::DayState dayTwo = state;
    dayTwo.dayNumber = 2;
    belief.observe(dayTwo);
    udon::TrafficScenario scenario;
    scenario.opponentCurrentFootprint.assign(static_cast<std::size_t>(config.map.cell_count()), 0);
    scenario.opponentCarryFootprint.assign(static_cast<std::size_t>(config.map.cell_count()), 0);
    std::vector<std::int32_t> candidateFootprint(static_cast<std::size_t>(config.map.cell_count()), 0);
    candidateFootprint.at(0) = 1;
    const std::vector<udon::RoadStatus> beforeResend = udon::predict_next_road_statuses(
        config,
        belief,
        scenario,
        candidateFootprint);
    std::vector<std::int32_t> submittedFootprint(static_cast<std::size_t>(config.map.cell_count()), 0);
    belief.record_own_footprint(2, submittedFootprint);
    const std::vector<udon::RoadStatus> afterResend = udon::predict_next_road_statuses(
        config,
        belief,
        scenario,
        candidateFootprint);
    require(
        beforeResend == afterResend && beforeResend.at(0) == udon::RoadStatus::Busy,
        "a same-day resend must keep the d-1 own footprint in traffic prediction");

    udon::DayState dayThree = state;
    dayThree.dayNumber = 3;
    belief.observe(dayThree);
    require(
        belief.previous_own_footprint() == submittedFootprint,
        "the next authoritative day must advance traffic memory to the acknowledged footprint");
}

void test_fast_viability_bounds(const udon::MatchConfig& config, const udon::DayState& state) {
    const udon::FastViabilityAnalyzer analyzer(config);
    const udon::ViabilityBounds bounds = analyzer.analyze(state, udon::MatchLedger{});
    require(
        bounds.optimisticLatestDayByBrand.size() == static_cast<std::size_t>(config.brand_count()) &&
            bounds.latestSafeDayByBrand.size() == static_cast<std::size_t>(config.brand_count()) &&
            bounds.optimisticAgentDaySlotsByBrand.size() == static_cast<std::size_t>(config.brand_count()) &&
            bounds.safeAgentDaySlotsByBrand.size() == static_cast<std::size_t>(config.brand_count()),
        "fast viability must retain reachability, latest-day, and agent-day slot bounds per brand");
    for (std::size_t brandIndex = 0; brandIndex < bounds.latestSafeDayByBrand.size(); ++brandIndex) {
        require(
            bounds.safeAgentDaySlotsByBrand.at(brandIndex) <=
                bounds.optimisticAgentDaySlotsByBrand.at(brandIndex),
            "pessimistic reachability cannot create more agent-day slots than optimistic reachability");
        if (bounds.latestSafeDayByBrand.at(brandIndex) >= state.dayNumber) {
            require(
                bounds.optimisticLatestDayByBrand.at(brandIndex) >= bounds.latestSafeDayByBrand.at(brandIndex),
                "a safe latest-start day cannot be later than the optimistic latest-start day");
        }
    }
    for (const udon::MandatoryReservation& reservation : bounds.reservations) {
        const std::size_t brandIndex = static_cast<std::size_t>(reservation.brandIndex);
        require(
            reservation.latestSafeDay == state.dayNumber &&
                (bounds.optimisticAgentDaySlotsByBrand.at(brandIndex) == 1 ||
                 bounds.safeAgentDaySlotsByBrand.at(brandIndex) == 1),
            "a fast mandatory reservation must come from a single remaining relaxed agent-day slot");
    }
    require(
        bounds.upperBound.totalDailyDistinct >= config.brand_count() * config.day_count(),
        "tier-two relaxation must remain a valid upper bound independent of lifetime coverage");

    const udon::ViabilityBounds expired = analyzer.analyze(
        state,
        udon::MatchLedger{},
        std::chrono::steady_clock::now() - std::chrono::milliseconds{1});
    require(
        expired.deadlineReached && expired.reservations.empty() &&
            expired.upperBound.lifetimeDistinct == config.brand_count(),
        "a timed-out fast viability pass must return conservative bounds without reservations");
}

void test_fast_viability_no_tanker_claim_upper() {
    const udon::MatchConfig config = udon::parse_match_config(udon::JsonValue::parse(R"({
        "startsAt":1,
        "daySeconds":[5,5,5,5],
        "daySteps":[16,16,16,16],
        "map":{"height":8,"width":8,"cells":[
            [0,0,3,3,3,3,3,3],
            [3,3,3,3,3,3,3,3],
            [0,0,0,0,0,0,0,3],
            [3,3,3,3,3,3,3,3],
            [3,3,3,3,3,3,3,3],
            [3,3,3,3,3,3,3,3],
            [3,3,3,3,3,3,3,3],
            [3,3,3,3,3,3,3,3]
        ]},
        "spots":[
            {"brand":10,"pos":16,"stocks":3},
            {"brand":11,"pos":17,"stocks":3},
            {"brand":12,"pos":18,"stocks":3},
            {"brand":13,"pos":19,"stocks":3},
            {"brand":14,"pos":20,"stocks":3},
            {"brand":15,"pos":21,"stocks":3}
        ],
        "agents":[22,0,1],
        "fuelLimits":1,
        "players":1,
        "busyThreshold":2,
        "jammedThreshold":4
    })"));
    const udon::DayState noTanker = udon::parse_day_state(config, udon::JsonValue::parse(R"({
        "endsAt":6,
        "day":1,
        "agents":[
            {"kind":0,"pos":16,"fuel":0},
            {"kind":0,"pos":0,"fuel":0},
            {"kind":0,"pos":1,"fuel":0}
        ],
        "others":[],
        "traffics":[]
    })"));
    const udon::FastViabilityAnalyzer analyzer(config);
    const udon::ViabilityBounds capped = analyzer.analyze(noTanker, udon::MatchLedger{});
    require(
        capped.upperBound.totalDailyDistinct == config.day_count() &&
            capped.upperBound.totalServings == config.day_count(),
        "without a tanker, one fuel-zero patrol at a spot can claim at most once per remaining day");

    udon::DayState withTanker = noTanker;
    withTanker.agents.at(1).kind = udon::AgentKind::Tanker;
    const udon::ViabilityBounds relaxed = analyzer.analyze(withTanker, udon::MatchLedger{});
    require(
        relaxed.upperBound.totalDailyDistinct == config.brand_count() * config.day_count() &&
            relaxed.upperBound.totalServings > capped.upperBound.totalServings,
        "the no-tanker claim cap must not tighten the established tanker relaxation");
}

void test_fast_viability_emits_proven_unique_reservation() {
    const udon::MatchConfig config = udon::parse_match_config(udon::JsonValue::parse(R"({
        "startsAt":1,
        "daySeconds":[5,5,5,5],
        "daySteps":[16,16,16,16],
        "map":{"height":8,"width":8,"cells":[
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0]
        ]},
        "spots":[{"brand":10,"pos":16,"stocks":1}],
        "agents":[8,9,10],
        "fuelLimits":2,
        "players":1,
        "busyThreshold":2,
        "jammedThreshold":4
    })"));
    const udon::DayState state = udon::parse_day_state(config, udon::JsonValue::parse(R"({
        "endsAt":6,
        "day":4,
        "agents":[
            {"kind":0,"pos":16,"fuel":2},
            {"kind":1,"pos":9,"fuel":2},
            {"kind":1,"pos":10,"fuel":2}
        ],
        "others":[],
        "traffics":[]
    })"));
    const udon::ViabilityBounds bounds =
        udon::FastViabilityAnalyzer(config).analyze(state, udon::MatchLedger{});
    require(
        bounds.reservations.size() == 1U &&
            udon::is_proven_reservation(bounds.reservations.front()) &&
            bounds.reservations.front().representativeSpot == 0,
        "a unique brand with one necessary agent-day and one spot must produce a proven reservation");
}

void test_latest_safe_day_uses_suffix_budget() {
    const udon::MatchConfig config = udon::parse_match_config(udon::JsonValue::parse(R"({
        "startsAt":1,
        "daySeconds":[5,5,5,5],
        "daySteps":[16,16,16,16],
        "map":{"height":8,"width":8,"cells":[
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0]
        ]},
        "spots":[{"brand":1,"pos":63,"stocks":1}],
        "agents":[0,8,16],
        "fuelLimits":50,
        "players":2,
        "busyThreshold":2,
        "jammedThreshold":4
    })"));
    const udon::DayState state = udon::parse_day_state(config, udon::JsonValue::parse(R"({
        "endsAt":6,
        "day":1,
        "agents":[
            {"kind":0,"pos":0,"fuel":50},
            {"kind":0,"pos":0,"fuel":50},
            {"kind":0,"pos":0,"fuel":50}
        ],
        "others":[],
        "traffics":[]
    })"));
    const udon::ViabilityBounds bounds = udon::FastViabilityAnalyzer(config).analyze(state, udon::MatchLedger{});
    require(
        bounds.optimisticLatestDayByBrand.at(0) == 3 && bounds.latestSafeDayByBrand.at(0) == 3,
        "latest safe day must use remaining suffix steps, not cumulative steps already available today");
}

void test_provisional_key_is_history_independent() {
    udon::ScenarioManifest manifest;
    manifest.totalWeight = 10000;
    manifest.scenarios.push_back(udon::TrafficScenario{0, "likely", 5000});
    manifest.scenarios.push_back(udon::TrafficScenario{1, "stress", 5000, true});
    const auto evaluation = [](std::string stableId, udon::OfficialScore first, udon::OfficialScore second) {
        udon::CandidateEvaluation result;
        result.candidate.stableId = std::move(stableId);
        result.candidate.scoreAfterToday = first;
        result.profile.validUpperBound = udon::OfficialScore{3, 6, 9};
        result.profile.hasValidUpperBound = true;
        result.profile.provisional = true;
        result.profile.outcomes.push_back(udon::ScenarioOutcome{
            first,
            udon::FutureWitness{{}, first, false},
        });
        result.profile.outcomes.push_back(udon::ScenarioOutcome{
            second,
            udon::FutureWitness{{}, second, false},
        });
        return result;
    };
    std::vector<udon::CandidateEvaluation> ordered;
    ordered.push_back(evaluation("candidate-a", udon::OfficialScore{2, 3, 4}, udon::OfficialScore{1, 3, 4}));
    ordered.push_back(evaluation("candidate-b", udon::OfficialScore{2, 2, 9}, udon::OfficialScore{2, 2, 8}));
    const udon::LexicographicRiskComparator comparator(udon::RiskPolicy{});
    comparator.finalize_profiles(ordered, manifest);
    ordered.at(0).profile.certifiedLowerBound = udon::OfficialScore{99, 99, 99};
    const std::string orderedWinner = ordered.at(comparator.choose_provisional(ordered)).candidate.stableId;
    std::reverse(ordered.begin(), ordered.end());
    const std::string reversedWinner = ordered.at(comparator.choose_provisional(ordered)).candidate.stableId;
    require(
        orderedWinner == reversedWinner && orderedWinner == "candidate-b",
        "Key0 must ignore certificate history and remain stable when candidate order changes");
}

void test_final_choice_rejects_unproven_current_regret() {
    const auto evaluation = [](
                                std::string stableId,
                                udon::OfficialScore today,
                                udon::OfficialScore projected) {
        udon::CandidateEvaluation result;
        result.candidate.stableId = std::move(stableId);
        result.candidate.scoreAfterToday = today;
        result.profile.quantiles.fill(projected);
        result.profile.certifiedLowerBound = projected;
        result.profile.validUpperBound = udon::OfficialScore{6, 60, 562};
        result.profile.hasValidUpperBound = true;
        result.profile.confidenceCoverage = 6;
        return result;
    };
    std::vector<udon::CandidateEvaluation> evaluations;
    evaluations.push_back(evaluation(
        "higher-current-overlapping-future",
        udon::OfficialScore{6, 6, 31},
        udon::OfficialScore{6, 44, 204}));
    evaluations.push_back(evaluation(
        "lower-current-stronger-witness",
        udon::OfficialScore{6, 6, 28},
        udon::OfficialScore{6, 46, 203}));
    const udon::LexicographicRiskComparator comparator(udon::RiskPolicy{});
    require(
        evaluations.at(comparator.choose(evaluations)).candidate.stableId ==
            "lower-current-stronger-witness",
        "the fixture must expose the unresolved-witness false-precision risk");
    require(
        evaluations.at(comparator.choose(evaluations, true)).candidate.stableId ==
            "higher-current-overlapping-future",
        "final selection must preserve the best undominated current score when "
        "future valid intervals still overlap");
    std::reverse(evaluations.begin(), evaluations.end());
    require(
        evaluations.at(comparator.choose(evaluations, true)).candidate.stableId ==
            "higher-current-overlapping-future",
        "the undominated current floor must be independent of candidate order");
}

void test_risk_policy_and_manifest_guards() {
    udon::RiskPolicy invalidPolicy;
    invalidPolicy.quantileBasisPoints = {8000, 9500, 5000, 2000, 500};
    bool rejectedPolicy = false;
    try {
        static_cast<void>(udon::LexicographicRiskComparator(invalidPolicy));
    } catch (const std::invalid_argument&) {
        rejectedPolicy = true;
    }
    require(rejectedPolicy, "a risk policy with an ascending quantile ladder must be rejected");

    udon::ScenarioManifest privateManifest;
    privateManifest.totalWeight = 10000;
    udon::TrafficScenario privateScenario;
    privateScenario.weight = 10000;
    privateScenario.privateSensitivity = true;
    privateManifest.scenarios.push_back(std::move(privateScenario));
    udon::CandidateEvaluation privateEvaluation;
    privateEvaluation.candidate.stableId = "private";
    privateEvaluation.profile.outcomes.push_back(udon::ScenarioOutcome{
        udon::OfficialScore{1, 1, 1},
        udon::FutureWitness{{}, udon::OfficialScore{1, 1, 1}, false},
    });
    bool rejectedPrivateManifest = false;
    try {
        std::vector<udon::CandidateEvaluation> evaluations;
        evaluations.push_back(std::move(privateEvaluation));
        udon::LexicographicRiskComparator(udon::RiskPolicy{}).finalize_profiles(evaluations, privateManifest);
    } catch (const std::invalid_argument&) {
        rejectedPrivateManifest = true;
    }
    require(rejectedPrivateManifest, "private sensitivity must not be admitted into the public risk comparator");

    udon::ScenarioManifest fallbackManifest;
    fallbackManifest.totalWeight = 10000;
    fallbackManifest.survivalSignatureEnabled = true;
    fallbackManifest.requiredClassesCovered = false;
    udon::TrafficScenario fallbackScenario;
    fallbackScenario.weight = 10000;
    fallbackScenario.adversarial = true;
    fallbackScenario.pessimisticFallback = true;
    fallbackManifest.scenarios.push_back(std::move(fallbackScenario));
    udon::CandidateEvaluation fallbackEvaluation;
    fallbackEvaluation.candidate.stableId = "fallback";
    fallbackEvaluation.profile.outcomes.push_back(udon::ScenarioOutcome{
        udon::OfficialScore{1, 1, 1},
        udon::FutureWitness{{}, udon::OfficialScore{1, 1, 1}, true, true},
    });
    std::vector<udon::CandidateEvaluation> fallbackEvaluations;
    fallbackEvaluations.push_back(std::move(fallbackEvaluation));
    udon::LexicographicRiskComparator(udon::RiskPolicy{}).finalize_profiles(
        fallbackEvaluations,
        fallbackManifest);
    require(
        !fallbackEvaluations.front().profile.signatureEnabled,
        "a pessimistic fallback must disable survival-signature tie breaking");

    const auto candidate_from_scores = [](
                                           std::string stableId,
                                           std::initializer_list<udon::OfficialScore> scores) {
        udon::CandidateEvaluation evaluation;
        evaluation.candidate.stableId = std::move(stableId);
        for (const udon::OfficialScore& score : scores) {
            evaluation.profile.outcomes.push_back(udon::ScenarioOutcome{
                score,
                udon::FutureWitness{{}, score, false},
            });
        }
        return evaluation;
    };
    udon::ScenarioManifest baseManifest;
    baseManifest.totalWeight = 10000;
    baseManifest.scenarios.push_back(
        udon::TrafficScenario{0, "likely", 5000});
    baseManifest.scenarios.push_back(
        udon::TrafficScenario{1, "adversarial", 5000, true});
    std::vector<udon::CandidateEvaluation> baseCandidates;
    baseCandidates.push_back(candidate_from_scores(
        "risky",
        {udon::OfficialScore{3, 3, 3},
         udon::OfficialScore{1, 1, 1}}));
    baseCandidates.push_back(candidate_from_scores(
        "safe",
        {udon::OfficialScore{2, 2, 2},
         udon::OfficialScore{2, 2, 2}}));
    const udon::LexicographicRiskComparator comparator(
        udon::RiskPolicy{});
    comparator.finalize_profiles(baseCandidates, baseManifest);
    const std::string baseWinner =
        baseCandidates.at(comparator.choose(baseCandidates)).candidate.stableId;

    udon::ScenarioManifest duplicatedManifest;
    duplicatedManifest.totalWeight = 10000;
    duplicatedManifest.scenarios.push_back(
        udon::TrafficScenario{0, "likely", 2500});
    duplicatedManifest.scenarios.push_back(
        udon::TrafficScenario{1, "likely", 2500});
    duplicatedManifest.scenarios.push_back(
        udon::TrafficScenario{2, "adversarial", 5000, true});
    std::vector<udon::CandidateEvaluation> duplicatedCandidates;
    duplicatedCandidates.push_back(candidate_from_scores(
        "risky",
        {udon::OfficialScore{3, 3, 3},
         udon::OfficialScore{3, 3, 3},
         udon::OfficialScore{1, 1, 1}}));
    duplicatedCandidates.push_back(candidate_from_scores(
        "safe",
        {udon::OfficialScore{2, 2, 2},
         udon::OfficialScore{2, 2, 2},
         udon::OfficialScore{2, 2, 2}}));
    comparator.finalize_profiles(
        duplicatedCandidates,
        duplicatedManifest);
    require(
        baseWinner == "safe" &&
            duplicatedCandidates.at(
                comparator.choose(duplicatedCandidates))
                    .candidate.stableId == baseWinner,
        "duplicating samples inside one scenario class must not change its total probability or the decision");

    std::vector<udon::CandidateEvaluation> trafficTie;
    trafficTie.push_back(candidate_from_scores(
        "a-risky",
        {udon::OfficialScore{2, 2, 2},
         udon::OfficialScore{2, 2, 2}}));
    trafficTie.push_back(candidate_from_scores(
        "z-safe",
        {udon::OfficialScore{2, 2, 2},
         udon::OfficialScore{2, 2, 2}}));
    trafficTie.front().candidate.trafficSafety.thresholdCrossings = 1;
    comparator.finalize_profiles(trafficTie, baseManifest);
    require(
        trafficTie.at(comparator.choose(trafficTie)).candidate.stableId ==
            "z-safe",
        "exact self-induced threshold crossings must break a full score/slack tie before stable ID");

    udon::ScenarioManifest mismatchedManifest = baseManifest;
    mismatchedManifest.riskPolicyVersion = "different-policy";
    mismatchedManifest.confidenceBasisPoints = 8000;
    mismatchedManifest.safetySlack = 1;
    mismatchedManifest.resolutionBasisPoints = 100;
    mismatchedManifest.quantileBasisPoints = {9500, 8000, 5000, 2000, 500};
    mismatchedManifest.evaluatorHash = "fixture";
    require_throws(
        [&comparator, &mismatchedManifest, &candidate_from_scores]() {
            std::vector<udon::CandidateEvaluation> candidates;
            candidates.push_back(candidate_from_scores(
                "mismatch",
                {udon::OfficialScore{1, 1, 1}, udon::OfficialScore{1, 1, 1}}));
            comparator.finalize_profiles(candidates, mismatchedManifest);
        },
        "a frozen manifest must reject a risk-policy version mismatch");
}

void test_confidence_gate_and_conditional_tiers() {
    udon::ScenarioManifest manifest;
    manifest.totalWeight = 10000;
    for (std::int32_t scenarioIndex = 0; scenarioIndex < 20; ++scenarioIndex) {
        manifest.scenarios.push_back(udon::TrafficScenario{
            scenarioIndex,
            "catalog",
            500,
            scenarioIndex >= 18,
        });
    }
    const auto make_candidate = [](
                                    std::string stableId,
                                    const std::vector<udon::OfficialScore>& scores) {
        udon::CandidateEvaluation evaluation;
        evaluation.candidate.stableId = std::move(stableId);
        evaluation.candidate.scoreAfterToday = scores.front();
        for (const udon::OfficialScore& score : scores) {
            evaluation.profile.outcomes.push_back(udon::ScenarioOutcome{
                score,
                udon::FutureWitness{{}, score, true},
            });
        }
        return evaluation;
    };

    std::vector<udon::OfficialScore> safeScores(20, udon::OfficialScore{2, 4, 4});
    std::vector<udon::OfficialScore> riskyScores(18, udon::OfficialScore{3, 3, 3});
    riskyScores.insert(riskyScores.end(), 2, udon::OfficialScore{1, 8, 8});
    std::vector<udon::CandidateEvaluation> frontier;
    frontier.push_back(make_candidate("safe-m-minus-one", safeScores));
    frontier.push_back(make_candidate("risky-m", riskyScores));
    const udon::LexicographicRiskComparator comparator(udon::RiskPolicy{});
    comparator.finalize_profiles(frontier, manifest);
    require(
        frontier.front().profile.coverageCap == 2 &&
            frontier.back().profile.coverageCap == 3 &&
            frontier.at(comparator.choose(frontier)).candidate.stableId == "safe-m-minus-one",
        "confidence gate must retain and select a safe M-1 candidate instead of hard-filtering by Kcap");

    std::vector<udon::CandidateEvaluation> tierTwo;
    tierTwo.push_back(make_candidate(
        "daily-low",
        std::vector<udon::OfficialScore>(20, udon::OfficialScore{2, 4, 100})));
    tierTwo.push_back(make_candidate(
        "daily-high",
        std::vector<udon::OfficialScore>(20, udon::OfficialScore{2, 5, 0})));
    comparator.finalize_profiles(tierTwo, manifest);
    require(
        tierTwo.at(comparator.choose(tierTwo)).candidate.stableId == "daily-high",
        "conditional tier-2 score must outrank any serving gain after lifetime coverage ties");
    const auto coverageTwo = std::find_if(
        tierTwo.back().profile.conditionalTiers.begin(),
        tierTwo.back().profile.conditionalTiers.end(),
        [](const udon::ConditionalTierBounds& bounds) {
            return bounds.coverage == 2;
        });
    require(
        coverageTwo != tierTwo.back().profile.conditionalTiers.end() &&
            coverageTwo->witnessBacked &&
            coverageTwo->lowerDailyDistinct == 5,
        "conditional LB2(K) must be materialized from a certified full-score witness");
}

void test_certified_stochastic_dominance() {
    udon::ScenarioManifest manifest;
    manifest.totalWeight = 10000;
    manifest.scenarios.push_back(udon::TrafficScenario{0, "class-a", 5000});
    manifest.scenarios.push_back(udon::TrafficScenario{1, "class-b", 5000});
    const auto profile_from = [](std::initializer_list<udon::OfficialScore> scores) {
        udon::CandidateProfile profile;
        for (const udon::OfficialScore& score : scores) {
            profile.outcomes.push_back(udon::ScenarioOutcome{
                score,
                udon::FutureWitness{{}, score, true},
            });
        }
        return profile;
    };
    udon::CandidateEvaluation dominant;
    dominant.profile = profile_from({udon::OfficialScore{3, 0, 0}, udon::OfficialScore{1, 0, 0}});
    udon::CandidateEvaluation dominated;
    dominated.profile = profile_from({udon::OfficialScore{2, 0, 0}, udon::OfficialScore{1, 0, 0}});
    dominated.profile.validUpperBound = udon::OfficialScore{1, 0, 0};
    dominated.profile.hasValidUpperBound = true;
    std::vector<udon::CandidateEvaluation> evaluations;
    evaluations.push_back(std::move(dominant));
    evaluations.push_back(std::move(dominated));
    const udon::LexicographicRiskComparator comparator(udon::RiskPolicy{});
    comparator.finalize_profiles(evaluations, manifest);
    require(
        comparator.certified_dominates(evaluations.at(0).profile, evaluations.at(1).profile),
        "full survival dominance must prune a profile even when worst-case bounds tie");
    require(
        !comparator.certified_dominates(evaluations.at(1).profile, evaluations.at(0).profile),
        "stochastic dominance must remain directional");
}

void test_survival_signature_completes_quantile_ties() {
    udon::ScenarioManifest manifest;
    manifest.totalWeight = 10000;
    manifest.survivalSignatureEnabled = true;
    manifest.requiredClassesCovered = true;
    for (std::int32_t scenarioIndex = 0; scenarioIndex < 20; ++scenarioIndex) {
        manifest.scenarios.push_back(udon::TrafficScenario{
            scenarioIndex,
            "catalog",
            500,
        });
    }
    const std::array<std::int32_t, 20> firstValues{
        0, 1, 1, 1, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5,
    };
    const std::array<std::int32_t, 20> secondValues{
        0, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5,
    };
    const auto make_evaluation = [](std::string id, const std::array<std::int32_t, 20>& values) {
        udon::CandidateEvaluation evaluation;
        evaluation.candidate.stableId = std::move(id);
        evaluation.candidate.scoreAfterToday = udon::OfficialScore{1, 1, 0};
        evaluation.profile.hasValidUpperBound = true;
        evaluation.profile.validUpperBound = udon::OfficialScore{1, 1, 5};
        for (const std::int32_t value : values) {
            const udon::OfficialScore score{1, 1, value};
            evaluation.profile.outcomes.push_back(udon::ScenarioOutcome{
                score,
                udon::FutureWitness{{}, score, true},
            });
        }
        return evaluation;
    };
    std::vector<udon::CandidateEvaluation> evaluations;
    evaluations.push_back(make_evaluation("weaker-tail", firstValues));
    evaluations.push_back(make_evaluation("stronger-tail", secondValues));
    const udon::LexicographicRiskComparator comparator(udon::RiskPolicy{});
    comparator.finalize_profiles(evaluations, manifest);
    require(
        evaluations.at(0).profile.quantiles == evaluations.at(1).profile.quantiles,
        "survival-signature fixture must keep all five policy quantiles tied");
    require(
        evaluations.at(0).profile.survivalSignature !=
            evaluations.at(1).profile.survivalSignature,
        "G must retain distribution information that the five quantiles do not encode");
    require(
        evaluations.at(comparator.choose(evaluations)).candidate.stableId == "stronger-tail",
        "G must choose the candidate with stronger survival inside an exact quantile tie-group");
}

void test_reconciliation_and_submission_gate(const udon::MatchConfig& config, const udon::DayState& state) {
    udon::UdonShieldEngine matchingEngine(config);
    const udon::DecisionResult dayOne = matchingEngine.solve_day(state, udon::MatchLedger{}, std::chrono::milliseconds{1});
    matchingEngine.record_submitted(dayOne, std::chrono::milliseconds{10});
    udon::DayState nextState = state;
    nextState.dayNumber = 2;
    nextState.agents = dayOne.candidate.simulation.finalAgents;
    const udon::DecisionResult matchingDayTwo = matchingEngine.solve_day(
        nextState,
        udon::MatchLedger{},
        std::chrono::milliseconds{1});
    require(!matchingDayTwo.reconciledAuthoritativeState,
            "server state matching the submitted simulation must not trigger reconciliation");

    udon::UdonShieldEngine reconciledEngine(config);
    const udon::DecisionResult submitted = reconciledEngine.solve_day(state, udon::MatchLedger{}, std::chrono::milliseconds{1});
    reconciledEngine.record_submitted(submitted, std::chrono::milliseconds{10});
    udon::DayState authoritative = nextState;
    authoritative.agents.front().fuel -= 1;
    const udon::DecisionResult reconciled = reconciledEngine.solve_day(
        authoritative,
        udon::MatchLedger{},
        std::chrono::milliseconds{1});
    require(reconciled.reconciledAuthoritativeState,
            "authoritative mismatch must be surfaced instead of silently reusing prediction state");

    udon::DecisionResult uncertifiedReplacement = submitted;
    uncertifiedReplacement.emergency = false;
    uncertifiedReplacement.candidate.stableId += "replacement";
    require(!reconciledEngine.may_submit(uncertifiedReplacement),
            "same-day replacement without a certified witness must be rejected");

    udon::UdonShieldEngine horizonGateEngine(config);
    udon::DecisionResult oldDecision = horizonGateEngine.solve_day(
        state,
        udon::MatchLedger{},
        std::chrono::milliseconds{1});
    oldDecision.profile.validUpperBound = udon::OfficialScore{3, 12, 20};
    oldDecision.profile.hasValidUpperBound = true;
    oldDecision.profile.scenarioValidUpperBounds.assign(
        oldDecision.profile.outcomes.size(),
        oldDecision.profile.validUpperBound);
    horizonGateEngine.record_submitted(
        oldDecision,
        std::chrono::milliseconds{10});
    udon::DecisionResult unsafeCurrentGain = oldDecision;
    unsafeCurrentGain.emergency = false;
    unsafeCurrentGain.candidate.stableId += "-unsafe-current-gain";
    ++unsafeCurrentGain.candidate.scoreAfterToday.lifetimeDistinct;
    for (udon::ScenarioOutcome& outcome : unsafeCurrentGain.profile.outcomes) {
        outcome.score = unsafeCurrentGain.candidate.scoreAfterToday;
        outcome.witness.score = outcome.score;
        outcome.witness.certified = true;
    }
    unsafeCurrentGain.profile.certifiedLowerBound =
        unsafeCurrentGain.candidate.scoreAfterToday;
    require(
        !horizonGateEngine.may_submit(unsafeCurrentGain),
        "an exact current-day gain must not resend when its full-horizon LB still trails the previous valid UB");
}

void test_column_events_and_stock_cuts(const udon::MatchConfig& config, const udon::DayState& state) {
    const udon::ParetoRouter router(config);
    const udon::RouteColumnGenerator generator(config, router);
    udon::ColumnGenerationOptions generationOptions;
    generationOptions.maximumPathsPerTarget = 1;
    generationOptions.maximumColumnsPerAgent = 64;
    generationOptions.maximumTargetSpots = 3;
    udon::ColumnGenerationDiagnostics generationDiagnostics;
    const udon::RoutePortfolio generated = generator.generate(
        state,
        udon::MatchLedger{},
        generationOptions,
        &generationDiagnostics);
    require(
        generationDiagnostics.agentMilliseconds.size() == static_cast<std::size_t>(config.agent_count()) &&
            generationDiagnostics.agentParetoQueries.size() == static_cast<std::size_t>(config.agent_count()),
        "column-generation diagnostics must report every agent");
    require(
        generationDiagnostics.criticalRoads.empty(),
        "smooth roads without promoted evidence must not inflate the adaptive critical set");
    require(
        std::accumulate(
            generationDiagnostics.agentParetoQueries.begin(),
            generationDiagnostics.agentParetoQueries.end(),
            generationDiagnostics.coordinationParetoQueries) == generationDiagnostics.pareto.queries,
        "column-generation diagnostics must attribute every Pareto query");
    bool foundInitialVisit = false;
    bool foundThreeSpotRoute = false;
    for (const udon::RouteColumn& column : generated.columnsByAgent.at(0)) {
        udon::SparseRoadFootprint reconstructed;
        for (std::size_t step = 1; step < column.timeline.size(); ++step) {
            const udon::CellId position = column.timeline.at(step).position;
            if (config.map.terrain.at(static_cast<std::size_t>(position)) == udon::Terrain::Road) {
                reconstructed.add(position, 1);
            }
        }
        for (const udon::CellId road : config.roadCells) {
            require(
                reconstructed.at(road) == column.fullFootprint.at(road),
                "every completed column must retain the exact sparse road footprint from its step timeline");
        }
        if (column.actions.size() == 1U && column.actions.front().kind == udon::ActionKind::Wait) {
            foundInitialVisit = column.firstVisits.size() == 1U && column.firstVisits.front().spot == 0 &&
                column.firstVisits.front().step == 16 &&
                column.timeline.size() == 17U &&
                column.terminalFeatures.overnightHarvestCandidate &&
                column.terminalFeatures.nearestUncollectedBrandSteps <
                    std::numeric_limits<std::int32_t>::max();
        }
        foundThreeSpotRoute = foundThreeSpotRoute || column.firstVisits.size() >= 3U;
    }
    require(
        foundInitialVisit,
        "route columns must retain exact metadata and action-completion first visits");
    require(
        foundThreeSpotRoute,
        "label/insertion generation must be able to emit a feasible three-spot patrol route");

    udon::ColumnGenerationOptions ordinaryFuelOptions;
    ordinaryFuelOptions.maximumPathsPerTarget = 1;
    ordinaryFuelOptions.maximumColumnsPerAgent = 12;
    ordinaryFuelOptions.maximumTargetSpots = 4;
    ordinaryFuelOptions.allowUncachedHarvestTargets = true;
    ordinaryFuelOptions.maximumHarvestExtensionSources = 4;
    ordinaryFuelOptions.maximumHarvestExtensionDepth = 4;
    const udon::RoutePortfolio ordinaryFuelBaseline = generator.generate(
        state,
        udon::MatchLedger{},
        ordinaryFuelOptions);
    ordinaryFuelOptions.enableHarvestOrienteering = true;
    ordinaryFuelOptions.enableExactHarvestOrienteering = true;
    const udon::RoutePortfolio ordinaryFuelChallenger = generator.generate(
        state,
        udon::MatchLedger{},
        ordinaryFuelOptions);
    const auto portfolio_action_keys = [](const udon::RoutePortfolio& portfolio) {
        std::vector<std::vector<std::string>> keys(
            portfolio.columnsByAgent.size());
        for (std::size_t agentOffset = 0;
             agentOffset < portfolio.columnsByAgent.size();
             ++agentOffset) {
            for (const udon::RouteColumn& column :
                 portfolio.columnsByAgent.at(agentOffset)) {
                std::string key;
                for (const udon::PlanAction& action : column.actions) {
                    key += std::to_string(action.wire_value());
                    key.push_back(',');
                }
                keys.at(agentOffset).push_back(std::move(key));
            }
            std::sort(
                keys.at(agentOffset).begin(),
                keys.at(agentOffset).end());
        }
        return keys;
    };
    require(
        portfolio_action_keys(ordinaryFuelBaseline) ==
            portfolio_action_keys(ordinaryFuelChallenger),
        "orienteering must be byte-equivalent at ordinary fuel levels");

    const udon::MatchConfig fourSpotConfig = udon::parse_match_config(udon::JsonValue::parse(R"({
        "startsAt":1778227200,
        "daySeconds":[5,5,5,5],
        "daySteps":[64,64,64,64],
        "map":{"height":8,"width":9,"cells":[
            [0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0],
            [0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0]
        ]},
        "spots":[
            {"brand":10,"pos":16,"stocks":1},{"brand":11,"pos":17,"stocks":1},
            {"brand":12,"pos":24,"stocks":1},{"brand":13,"pos":25,"stocks":1},
            {"brand":14,"pos":26,"stocks":1},{"brand":15,"pos":18,"stocks":1},
            {"brand":16,"pos":27,"stocks":1},{"brand":17,"pos":28,"stocks":1},
            {"brand":18,"pos":29,"stocks":1}
        ],
        "agents":[8,9,10],
        "fuelLimits":64,
        "players":2,
        "busyThreshold":2,
        "jammedThreshold":4
    })"));
    const udon::DayState fourSpotState = udon::parse_day_state(
        fourSpotConfig,
        udon::JsonValue::parse(R"({
            "endsAt":1778227205,
            "day":1,
            "agents":[
                {"kind":0,"pos":16,"fuel":64},
                {"kind":0,"pos":18,"fuel":64},
                {"kind":0,"pos":19,"fuel":64}
            ],
            "others":[],
            "traffics":[]
        })"));
    const udon::ParetoRouter fourSpotRouter(fourSpotConfig);
    const udon::RouteColumnGenerator fourSpotGenerator(fourSpotConfig, fourSpotRouter);
    udon::ColumnGenerationOptions fourSpotOptions;
    fourSpotOptions.maximumPathsPerTarget = 1;
    fourSpotOptions.maximumColumnsPerAgent = 12;
    fourSpotOptions.maximumTargetSpots = 4;
    const udon::RoutePortfolio fourSpotPortfolio = fourSpotGenerator.generate(
        fourSpotState,
        udon::MatchLedger{},
        fourSpotOptions);
    require(
        std::any_of(
            fourSpotPortfolio.columnsByAgent.front().begin(),
            fourSpotPortfolio.columnsByAgent.front().end(),
            [](const udon::RouteColumn& column) {
                return column.estimatedServings >= 4 && column.firstVisits.size() >= 4U;
            }),
        "bounded harvest augmentation must retain a feasible exact four-spot patrol route");

    fourSpotOptions.allowUncachedHarvestTargets = true;
    fourSpotOptions.maximumHarvestExtensionSources = 3;
    const udon::RoutePortfolio diversifiedHarvestPortfolio =
        fourSpotGenerator.generate(
            fourSpotState,
            udon::MatchLedger{},
            fourSpotOptions);
    std::set<std::string> diversifiedHarvestRoutes;
    std::set<std::int32_t> diversifiedSourceRanks;
    for (const udon::RouteColumn& column :
         diversifiedHarvestPortfolio.columnsByAgent.front()) {
        if (!column.harvestExtension) {
            continue;
        }
        std::string key;
        for (const udon::PlanAction& action : column.actions) {
            key += std::to_string(action.wire_value());
            key.push_back(',');
        }
        diversifiedHarvestRoutes.insert(std::move(key));
        diversifiedSourceRanks.insert(column.harvestExtensionSourceRank);
    }
    require(
        diversifiedHarvestRoutes.size() >= 2U,
        "research harvest expansion must retain multiple distinct multi-spot chains");
    require(
        diversifiedSourceRanks.contains(0) &&
            diversifiedSourceRanks.size() >= 2U,
        "research harvest expansion must preserve the baseline chain rank before extra sources");

    udon::ColumnGenerationOptions depthOptions;
    depthOptions.maximumPathsPerTarget = 1;
    depthOptions.maximumColumnsPerAgent = 12;
    depthOptions.maximumTargetSpots = 3;
    depthOptions.enableHarvestExtensions = true;
    depthOptions.allowUncachedHarvestTargets = true;
    depthOptions.maximumHarvestExtensionSources = 1;
    depthOptions.maximumHarvestExtensionDepth = 2;
    const udon::RoutePortfolio depthTwoPortfolio = fourSpotGenerator.generate(
        fourSpotState,
        udon::MatchLedger{},
        depthOptions);
    const auto maximum_servings = [](const udon::RoutePortfolio& candidatePortfolio) {
        std::int32_t maximum = 0;
        for (const udon::RouteColumn& column :
             candidatePortfolio.columnsByAgent.front()) {
            maximum = std::max(maximum, column.estimatedServings);
        }
        return maximum;
    };
    const std::int32_t depthTwoMaximum = maximum_servings(depthTwoPortfolio);
    depthOptions.maximumHarvestExtensionDepth = 3;
    const udon::RoutePortfolio depthThreePortfolio = fourSpotGenerator.generate(
        fourSpotState,
        udon::MatchLedger{},
        depthOptions);
    const std::int32_t depthThreeMaximum = maximum_servings(depthThreePortfolio);
    require(
        depthThreeMaximum > depthTwoMaximum,
        "depth-three harvest expansion must retain a strictly longer exact harvest chain: " +
            std::to_string(depthTwoMaximum) + " -> " +
            std::to_string(depthThreeMaximum));
    depthOptions.maximumHarvestExtensionDepth = 4;
    const udon::RoutePortfolio depthFourPortfolio = fourSpotGenerator.generate(
        fourSpotState,
        udon::MatchLedger{},
        depthOptions);
    const std::int32_t depthFourMaximum = maximum_servings(depthFourPortfolio);
    require(
        depthFourMaximum > depthThreeMaximum,
        "depth-four harvest expansion must retain a strictly longer exact harvest chain: " +
            std::to_string(depthThreeMaximum) + " -> " +
            std::to_string(depthFourMaximum));
    depthOptions.enableHarvestOrienteering = true;
    const udon::RoutePortfolio orienteeringPortfolio =
        fourSpotGenerator.generate(
            fourSpotState,
            udon::MatchLedger{},
            depthOptions);
    require(
        maximum_servings(orienteeringPortfolio) >= depthFourMaximum,
        "orienteering challenger must not lower the best exact harvest chain in its generated portfolio");

    udon::DayState contested = state;
    contested.agents.at(2).position = 18;
    const auto make_column = [](int id, int agent, udon::AgentPlan actions, int priority) {
        udon::RouteColumn column;
        column.columnId = id;
        column.agent = agent;
        column.actions = std::move(actions);
        column.priority = priority;
        return column;
    };
    udon::RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(3U);
    portfolio.columnsByAgent.at(0).push_back(make_column(0, 0, {udon::PlanAction::wait(16)}, 0));
    portfolio.columnsByAgent.at(1).push_back(make_column(1, 1, {udon::PlanAction::wait(16)}, 0));
    portfolio.columnsByAgent.at(2).push_back(make_column(
        2,
        2,
        {udon::PlanAction::move(5), udon::PlanAction::move(5), udon::PlanAction::wait(12)},
        100));
    portfolio.columnsByAgent.at(2).push_back(make_column(
        3,
        2,
        {udon::PlanAction::wait(1), udon::PlanAction::move(5), udon::PlanAction::move(5), udon::PlanAction::wait(11)},
        99));
    portfolio.columnsByAgent.at(2).push_back(make_column(4, 2, {udon::PlanAction::wait(16)}, 0));

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    udon::MasterDiagnostics diagnostics;
    udon::MasterOptions masterOptions;
    masterOptions.maximumCombinations = 3;
    masterOptions.maximumCandidates = 8;
    masterOptions.maximumResolveRounds = 1;
    const std::vector<udon::MasterCandidate> candidates = master.solve(
        contested,
        udon::MatchLedger{},
        portfolio,
        masterOptions,
        diagnostics);
    require(!candidates.empty(), "stock-overcount combinations remain valid candidates with exact score");
    require(diagnostics.capCuts == 1, "first stock overcount must create one CAP cut");
    require(diagnostics.prefixCuts == 2, "distinct failed service prefixes must create PREFIX cuts");
    require(diagnostics.hotspotPromotions == 1, "second distinct prefix conflict must promote a hotspot");
    require(diagnostics.cutRounds == 1, "fixed single resolve round must remain bounded");
    require(
        diagnostics.nativeExactStockCredits && diagnostics.exactCreditMismatches == 0,
        "native credit variables must reproduce exact stock order at every accepted leaf");
}

void test_unreachable_brand_staging_column() {
    const udon::MatchConfig config = udon::parse_match_config(
        udon::JsonValue::parse(R"({
            "startsAt":1778227200,
            "daySeconds":[5,5,5,5],
            "daySteps":[16,16,16,16],
            "map":{"height":8,"width":8,"cells":[
                [0,0,2,2,2,2,2,2],
                [0,2,2,2,2,2,2,2],
                [2,2,2,2,2,2,2,2],
                [2,2,2,2,2,2,2,2],
                [2,2,2,2,2,2,2,2],
                [2,2,2,2,2,2,2,2],
                [2,2,2,2,2,2,2,2],
                [2,2,2,2,2,2,2,0]
            ]},
            "spots":[{"brand":99,"pos":63,"stocks":1}],
            "agents":[0,1,8],
            "fuelLimits":40,
            "players":2,
            "busyThreshold":2,
            "jammedThreshold":4
        })"));
    const udon::DayState state = udon::parse_day_state(
        config,
        udon::JsonValue::parse(R"({
            "endsAt":1778227205,
            "day":1,
            "agents":[
                {"kind":0,"pos":0,"fuel":40},
                {"kind":0,"pos":1,"fuel":40},
                {"kind":0,"pos":8,"fuel":40}
            ],
            "others":[],
            "traffics":[]
        })"));
    const udon::ParetoRouter router(config);
    const udon::RouteColumnGenerator generator(config, router);
    udon::ColumnGenerationOptions options;
    options.maximumPathsPerTarget = 1;
    options.maximumColumnsPerAgent = 2;
    options.maximumTargetSpots = 1;
    const udon::RoutePortfolio portfolio = generator.generate(
        state,
        udon::MatchLedger{},
        options);
    const auto staging = std::find_if(
        portfolio.columnsByAgent.at(0).begin(),
        portfolio.columnsByAgent.at(0).end(),
        [&state](const udon::RouteColumn& column) {
            return column.firstVisits.empty() &&
                column.terminalCell != state.agents.at(0).position &&
                column.terminalFeatures.nearestUncollectedBrandSteps <
                    std::numeric_limits<std::int32_t>::max();
        });
    require(
        staging != portfolio.columnsByAgent.at(0).end(),
        "an unreachable-today brand must retain a partial staging route for future viability evaluation");
    const auto wait = std::find_if(
        portfolio.columnsByAgent.at(0).begin(),
        portfolio.columnsByAgent.at(0).end(),
        [](const udon::RouteColumn& column) {
            return column.actions.size() == 1U &&
                column.actions.front().kind == udon::ActionKind::Wait;
        });
    require(
        wait != portfolio.columnsByAgent.at(0).end() &&
            staging->terminalFeatures.nearestUncollectedBrandSteps <
                wait->terminalFeatures.nearestUncollectedBrandSteps,
        "staging route must strictly improve distance to the uncollected brand without removing the safe WAIT column");

    udon::DayPlan stagingPlan;
    stagingPlan.actions = {
        staging->actions,
        udon::AgentPlan{udon::PlanAction::wait(config.steps_for_day(state.dayNumber))},
        udon::AgentPlan{udon::PlanAction::wait(config.steps_for_day(state.dayNumber))},
    };
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    const std::optional<udon::MasterCandidate> staged = master.evaluate_exact_plan(
        state,
        udon::MatchLedger{},
        stagingPlan);
    require(
        staged.has_value() &&
            staged->scoreAfterToday == udon::OfficialScore{},
        "one-day master must not credit future brand or serving value for a staging terminal feature");
}

void test_master_lexicographic_branch_and_bound(
    const udon::MatchConfig& config,
    const udon::DayState& state) {
    udon::DayState proofState = state;
    proofState.agents.at(1).kind = udon::AgentKind::Tanker;
    proofState.agents.at(2).kind = udon::AgentKind::Tanker;
    std::int32_t leavingDirection = -1;
    for (std::int32_t direction = 0; direction < udon::kDirectionCount; ++direction) {
        const udon::CellId neighbor = config.map.neighbors.at(16).at(static_cast<std::size_t>(direction));
        if (neighbor != udon::kInvalidCell &&
            config.spotAtCell.at(static_cast<std::size_t>(neighbor)) == udon::kInvalidSpot) {
            leavingDirection = direction;
            break;
        }
    }
    require(leavingDirection >= 0, "branch-and-bound fixture requires a route away from the serving spot");

    udon::RouteColumn serving;
    serving.columnId = 0;
    serving.agent = 0;
    serving.actions = {udon::PlanAction::wait(16)};
    serving.firstVisits = {udon::ColumnVisitEvent{0, 16, true}};
    serving.hasExactTimeline = true;
    serving.priority = 100;

    udon::RouteColumn empty;
    empty.columnId = 1;
    empty.agent = 0;
    empty.actions = {udon::PlanAction::move(leavingDirection), udon::PlanAction::wait(15)};
    empty.hasExactTimeline = true;

    udon::RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(3U);
    portfolio.columnsByAgent.at(0) = {serving, empty};
    for (udon::AgentIndex agentIndex = 1; agentIndex < 3; ++agentIndex) {
        udon::RouteColumn wait;
        wait.columnId = agentIndex + 1;
        wait.agent = agentIndex;
        wait.actions = {udon::PlanAction::wait(16)};
        wait.hasExactTimeline = true;
        portfolio.columnsByAgent.at(static_cast<std::size_t>(agentIndex)).push_back(std::move(wait));
    }

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    udon::DayPlan servingPlan;
    servingPlan.actions = {
        serving.actions,
        portfolio.columnsByAgent.at(1).front().actions,
        portfolio.columnsByAgent.at(2).front().actions,
    };
    const udon::SimulationResult servingResult = simulator.simulate(proofState, servingPlan, true);
    require(
        servingResult.valid && servingResult.score.servings == 1,
        "branch-and-bound serving fixture must produce one exact serving");
    udon::MasterOptions boundedOptions;
    boundedOptions.maximumCombinations = 10;
    boundedOptions.maximumCandidates = 1;
    boundedOptions.maximumResolveRounds = 1;
    udon::MasterDiagnostics boundedDiagnostics;
    const std::vector<udon::MasterCandidate> bounded = master.solve(
        proofState,
        udon::MatchLedger{},
        portfolio,
        boundedOptions,
        boundedDiagnostics);

    udon::MasterOptions exhaustiveOptions = boundedOptions;
    exhaustiveOptions.enableLexicographicBranchAndBound = false;
    udon::MasterDiagnostics exhaustiveDiagnostics;
    const std::vector<udon::MasterCandidate> exhaustive = master.solve(
        proofState,
        udon::MatchLedger{},
        portfolio,
        exhaustiveOptions,
        exhaustiveDiagnostics);
    require(
        !bounded.empty() && !exhaustive.empty() &&
            bounded.front().stableId == exhaustive.front().stableId,
        "lexicographic branch-and-bound must preserve the exact master optimum");
    require(
        boundedDiagnostics.searchComplete && boundedDiagnostics.branchesPruned > 0 &&
            boundedDiagnostics.combinationsVisited < exhaustiveDiagnostics.combinationsVisited,
        "branch-and-bound counters were complete=" + std::to_string(boundedDiagnostics.searchComplete) +
            " pruned=" + std::to_string(boundedDiagnostics.branchesPruned) +
            " bounded=" + std::to_string(boundedDiagnostics.combinationsVisited) +
            " exhaustive=" + std::to_string(exhaustiveDiagnostics.combinationsVisited) +
            " score=" + std::to_string(bounded.front().scoreAfterToday.lifetimeDistinct));
}

void test_master_bundle_aware_upper_bound(
    const udon::MatchConfig& config,
    const udon::DayState& state) {
    const udon::ParetoRouter router(config);
    const udon::RouteColumnGenerator generator(config, router);
    udon::ColumnGenerationOptions generationOptions;
    generationOptions.maximumPathsPerTarget = 4;
    generationOptions.maximumColumnsPerAgent = 64;
    generationOptions.maximumTargetSpots = 3;
    const udon::RoutePortfolio generated = generator.generate(
        state,
        udon::MatchLedger{},
        generationOptions);

    const auto servingRoute = std::find_if(
        generated.columnsByAgent.at(2).begin(),
        generated.columnsByAgent.at(2).end(),
        [&config](const udon::RouteColumn& column) {
            return column.hasExactTimeline &&
                column.firstVisits.size() == 1U &&
                column.firstVisits.front().claimedServing &&
                config.spots.at(
                    static_cast<std::size_t>(
                        column.firstVisits.front().spot))
                        .brandIndex != 0;
        });
    require(
        servingRoute != generated.columnsByAgent.at(2).end(),
        "bundle-bound fixture requires an exact single-brand route for the third agent");

    std::int32_t leavingDirection = -1;
    for (std::int32_t direction = 0;
         direction < udon::kDirectionCount;
         ++direction) {
        const udon::CellId neighbor =
            config.map.neighbors.at(16).at(
                static_cast<std::size_t>(direction));
        if (neighbor != udon::kInvalidCell &&
            config.spotAtCell.at(
                static_cast<std::size_t>(neighbor)) ==
                udon::kInvalidSpot &&
            config.map.terrain.at(
                static_cast<std::size_t>(neighbor)) !=
                udon::Terrain::Road) {
            leavingDirection = direction;
            break;
        }
    }
    require(
        leavingDirection >= 0,
        "bundle-bound fixture requires a non-road route away from the initial serving spot");

    const auto make_exact_column = [](
                                       std::int32_t id,
                                       udon::AgentIndex agent,
                                       udon::AgentPlan actions,
                                       std::int32_t bundle) {
        udon::RouteColumn column;
        column.columnId = id;
        column.agent = agent;
        column.actions = std::move(actions);
        column.contingencyBundle = bundle;
        column.hasExactTimeline = true;
        return column;
    };
    const std::int32_t daySteps =
        config.steps_for_day(state.dayNumber);
    const std::int32_t moveSteps =
        config.move_cost(
                  state.agents.at(0).position,
                  state.roadStatuses.at(
                      static_cast<std::size_t>(
                          state.agents.at(0).position)))
            .steps;
    require(
        moveSteps > 0 && moveSteps < daySteps,
        "bundle-bound fixture requires a feasible departure move");

    udon::RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(3U);

    udon::RouteColumn bundleTenServing = make_exact_column(
        0,
        0,
        {udon::PlanAction::wait(daySteps)},
        10);
    bundleTenServing.firstVisits = {
        udon::ColumnVisitEvent{0, daySteps, true, 0, false},
    };
    portfolio.columnsByAgent.at(0).push_back(
        std::move(bundleTenServing));
    portfolio.columnsByAgent.at(1).push_back(
        make_exact_column(
            1,
            1,
            {udon::PlanAction::wait(daySteps)},
            10));
    portfolio.columnsByAgent.at(2).push_back(
        make_exact_column(
            2,
            2,
            {udon::PlanAction::wait(daySteps)},
            10));

    portfolio.columnsByAgent.at(0).push_back(
        make_exact_column(
            3,
            0,
            {
                udon::PlanAction::move(leavingDirection),
                udon::PlanAction::wait(daySteps - moveSteps),
            },
            11));
    portfolio.columnsByAgent.at(1).push_back(
        make_exact_column(
            4,
            1,
            {udon::PlanAction::wait(daySteps)},
            11));
    udon::RouteColumn bundleElevenServing = *servingRoute;
    bundleElevenServing.columnId = 5;
    bundleElevenServing.contingencyBundle = 11;
    portfolio.columnsByAgent.at(2).push_back(
        std::move(bundleElevenServing));

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    udon::MasterOptions legacyOptions;
    legacyOptions.maximumCombinations = 32;
    legacyOptions.maximumCandidates = 1;
    legacyOptions.maximumResolveRounds = 1;
    legacyOptions.enableBundleAwareUpperBound = false;
    udon::MasterDiagnostics legacyDiagnostics;
    const std::vector<udon::MasterCandidate> legacy =
        master.solve(
            state,
            udon::MatchLedger{},
            portfolio,
            legacyOptions,
            legacyDiagnostics);

    udon::MasterOptions bundleOptions = legacyOptions;
    bundleOptions.enableBundleAwareUpperBound = true;
    udon::MasterDiagnostics bundleDiagnostics;
    const std::vector<udon::MasterCandidate> bounded =
        master.solve(
            state,
            udon::MatchLedger{},
            portfolio,
            bundleOptions,
            bundleDiagnostics);

    require(
        !legacy.empty() && !bounded.empty() &&
            legacy.front().stableId == bounded.front().stableId &&
            legacy.front().scoreAfterToday ==
                bounded.front().scoreAfterToday,
        "bundle-aware upper bound must preserve the exact master optimum");
    require(
        legacy.front().scoreAfterToday ==
            udon::OfficialScore{1, 1, 1},
        "each atomic bundle must expose exactly one serving brand");
    require(
        legacyDiagnostics.optimisticUpperBound ==
            udon::OfficialScore{2, 2, 2},
        "legacy root upper bound must expose the cross-bundle overestimate");
    require(
        bundleDiagnostics.optimisticUpperBound ==
                legacy.front().scoreAfterToday &&
            bundleDiagnostics.searchGuidanceUpperBound ==
                legacyDiagnostics.optimisticUpperBound &&
            bundleDiagnostics.bundleAwareUpperBound,
        "bundle-aware root upper bound must close the incompatible-mode gap exactly");
}

void test_master_agent_brand_frontier_upper_bound(
    const udon::MatchConfig& config,
    const udon::DayState& state) {
    const std::int32_t daySteps =
        config.steps_for_day(state.dayNumber);
    const std::int32_t moveSteps =
        config.move_cost(
                  state.agents.at(0).position,
                  state.roadStatuses.at(
                      static_cast<std::size_t>(
                          state.agents.at(0).position)))
            .steps;
    require(
        config.map.neighbors.at(16).at(2) == 17 &&
            moveSteps > 0 && moveSteps < daySteps,
        "agent-frontier fixture requires the adjacent second brand");
    const auto make_exact_column = [](
                                       std::int32_t id,
                                       udon::AgentIndex agent,
                                       udon::AgentPlan actions) {
        udon::RouteColumn column;
        column.columnId = id;
        column.agent = agent;
        column.actions = std::move(actions);
        column.hasExactTimeline = true;
        return column;
    };

    udon::RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(3U);
    udon::RouteColumn firstBrand = make_exact_column(
        0,
        0,
        {udon::PlanAction::wait(daySteps)});
    firstBrand.firstVisits = {
        udon::ColumnVisitEvent{0, daySteps, true, 0, false},
    };
    portfolio.columnsByAgent.at(0).push_back(
        std::move(firstBrand));
    udon::RouteColumn secondBrand = make_exact_column(
        1,
        0,
        {
            udon::PlanAction::move(2),
            udon::PlanAction::wait(daySteps - moveSteps),
        });
    secondBrand.firstVisits = {
        udon::ColumnVisitEvent{1, moveSteps, true, 1, false},
    };
    portfolio.columnsByAgent.at(0).push_back(
        std::move(secondBrand));
    portfolio.columnsByAgent.at(1).push_back(
        make_exact_column(
            2,
            1,
            {udon::PlanAction::wait(daySteps)}));
    portfolio.columnsByAgent.at(2).push_back(
        make_exact_column(
            3,
            2,
            {udon::PlanAction::wait(daySteps)}));

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    udon::MasterOptions legacyOptions;
    legacyOptions.maximumCombinations = 8;
    legacyOptions.maximumCandidates = 1;
    legacyOptions.maximumResolveRounds = 1;
    legacyOptions.enableBundleAwareUpperBound = false;
    udon::MasterDiagnostics legacyDiagnostics;
    const std::vector<udon::MasterCandidate> legacy =
        master.solve(
            state,
            udon::MatchLedger{},
            portfolio,
            legacyOptions,
            legacyDiagnostics);

    udon::MasterOptions frontierOptions = legacyOptions;
    frontierOptions.enableBundleAwareUpperBound = true;
    udon::MasterDiagnostics frontierDiagnostics;
    const std::vector<udon::MasterCandidate> bounded =
        master.solve(
            state,
            udon::MatchLedger{},
            portfolio,
            frontierOptions,
            frontierDiagnostics);

    require(
        !legacy.empty() && !bounded.empty() &&
            legacy.front().stableId == bounded.front().stableId &&
            legacy.front().scoreAfterToday ==
                bounded.front().scoreAfterToday,
        "agent brand frontier must preserve the exact master optimum");
    require(
        legacyDiagnostics.optimisticUpperBound ==
            udon::OfficialScore{2, 2, 1},
        "legacy upper bound must expose same-agent route mixing");
    require(
        frontierDiagnostics.optimisticUpperBound ==
                udon::OfficialScore{1, 1, 1} &&
            frontierDiagnostics.searchGuidanceUpperBound ==
                legacyDiagnostics.optimisticUpperBound &&
            frontierDiagnostics.bundleBrandFrontierStates > 0 &&
            frontierDiagnostics.bundleBrandFrontierFallbacks == 0,
        "agent brand antichain must remove same-agent route mixing without fallback");
}

void test_master_stock_capped_search_order() {
    const udon::MatchConfig config = udon::parse_match_config(
        udon::JsonValue::parse(R"({
            "startsAt":1778227200,
            "daySeconds":[5,5,5,5],
            "daySteps":[32,32,32,32],
            "map":{"height":8,"width":8,"cells":[
                [0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0],
                [0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0],
                [0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0],
                [0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0]
            ]},
            "spots":[
                {"brand":10,"pos":11,"stocks":1},
                {"brand":10,"pos":48,"stocks":1},{"brand":10,"pos":49,"stocks":1},
                {"brand":10,"pos":50,"stocks":1},{"brand":10,"pos":51,"stocks":1},
                {"brand":10,"pos":52,"stocks":1},{"brand":10,"pos":53,"stocks":1},
                {"brand":10,"pos":54,"stocks":1}
            ],
            "agents":[0,1,2,3,4,5,6,7],
            "fuelLimits":64,
            "players":2,
            "busyThreshold":2,
            "jammedThreshold":4
        })"));
    const udon::DayState state = udon::parse_day_state(
        config,
        udon::JsonValue::parse(R"({
            "endsAt":1778227205,
            "day":1,
            "agents":[
                {"kind":0,"pos":0,"fuel":64},{"kind":0,"pos":1,"fuel":64},
                {"kind":0,"pos":2,"fuel":64},{"kind":0,"pos":3,"fuel":64},
                {"kind":0,"pos":4,"fuel":64},{"kind":0,"pos":5,"fuel":64},
                {"kind":0,"pos":6,"fuel":64},{"kind":0,"pos":7,"fuel":64}
            ],
            "others":[],
            "traffics":[]
        })"));
    const udon::ParetoRouter router(config);
    const udon::RouteColumnGenerator generator(config, router);
    udon::ColumnGenerationOptions generationOptions;
    generationOptions.maximumPathsPerTarget = 4;
    generationOptions.maximumColumnsPerAgent = 128;
    generationOptions.maximumTargetSpots = 9;
    const udon::RoutePortfolio generated = generator.generate(
        state,
        udon::MatchLedger{},
        generationOptions);

    std::vector<const udon::RouteColumn*> contested(8U, nullptr);
    std::vector<std::vector<const udon::RouteColumn*>> uniqueByAgent(
        8U,
        std::vector<const udon::RouteColumn*>(7U, nullptr));
    for (std::size_t agentOffset = 0; agentOffset < 8U; ++agentOffset) {
        for (const udon::RouteColumn& column : generated.columnsByAgent.at(agentOffset)) {
            if (!column.hasExactTimeline || column.firstVisits.size() != 1U) {
                continue;
            }
            const udon::SpotIndex spot = column.firstVisits.front().spot;
            if (spot == 0 && contested.at(agentOffset) == nullptr) {
                contested.at(agentOffset) = &column;
            } else if (spot > 0 && spot <= 7 &&
                       uniqueByAgent.at(agentOffset).at(static_cast<std::size_t>(spot - 1)) == nullptr) {
                uniqueByAgent.at(agentOffset).at(static_cast<std::size_t>(spot - 1)) = &column;
            }
        }
        require(contested.at(agentOffset) != nullptr, "stock-order fixture requires a shared contested route");
    }

    std::vector<const udon::RouteColumn*> assignedUnique(8U, nullptr);
    std::vector<bool> assignedAgent(8U, false);
    std::function<bool(std::size_t)> assign = [&](std::size_t spotOffset) {
        if (spotOffset == 7U) {
            return true;
        }
        for (std::size_t agentOffset = 0; agentOffset < 8U; ++agentOffset) {
            if (assignedAgent.at(agentOffset) || uniqueByAgent.at(agentOffset).at(spotOffset) == nullptr) {
                continue;
            }
            assignedAgent.at(agentOffset) = true;
            assignedUnique.at(agentOffset) = uniqueByAgent.at(agentOffset).at(spotOffset);
            if (assign(spotOffset + 1U)) {
                return true;
            }
            assignedUnique.at(agentOffset) = nullptr;
            assignedAgent.at(agentOffset) = false;
        }
        return false;
    };
    std::string availability;
    for (std::size_t agentOffset = 0; agentOffset < 8U; ++agentOffset) {
        availability += " a" + std::to_string(agentOffset) + "=";
        for (std::size_t spotOffset = 0; spotOffset < 7U; ++spotOffset) {
            if (uniqueByAgent.at(agentOffset).at(spotOffset) != nullptr) {
                availability += std::to_string(spotOffset + 1U);
            }
        }
    }
    require(
        assign(0U),
        "stock-order fixture requires seven distinct feasible service routes:" + availability);

    udon::RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(8U);
    for (std::size_t agentOffset = 0; agentOffset < 8U; ++agentOffset) {
        udon::RouteColumn contestedColumn = *contested.at(agentOffset);
        contestedColumn.columnId = static_cast<std::int32_t>(agentOffset * 2U);
        contestedColumn.priority = 1000000;
        portfolio.columnsByAgent.at(agentOffset).push_back(std::move(contestedColumn));
        if (assignedUnique.at(agentOffset) != nullptr) {
            udon::RouteColumn uniqueColumn = *assignedUnique.at(agentOffset);
            uniqueColumn.columnId = static_cast<std::int32_t>(agentOffset * 2U + 1U);
            uniqueColumn.priority = 0;
            portfolio.columnsByAgent.at(agentOffset).push_back(std::move(uniqueColumn));
        }
    }

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    udon::MasterOptions options;
    options.maximumCombinations = 128;
    options.maximumCandidates = 1;
    options.maximumResolveRounds = 1;
    options.enableLexicographicBranchAndBound = false;
    udon::MasterOptions rawClaimOptions = options;
    rawClaimOptions.preferStockCappedSearchOrder = false;
    udon::MasterDiagnostics rawClaimDiagnostics;
    const std::vector<udon::MasterCandidate> rawClaimCandidates = master.solve(
        state,
        udon::MatchLedger{},
        portfolio,
        rawClaimOptions,
        rawClaimDiagnostics);
    udon::MasterDiagnostics diagnostics;
    const std::vector<udon::MasterCandidate> candidates = master.solve(
        state,
        udon::MatchLedger{},
        portfolio,
        options,
        diagnostics);
    require(
        !rawClaimCandidates.empty() && !rawClaimDiagnostics.stockCappedSearchOrder &&
            rawClaimCandidates.front().scoreAfterToday.totalServings < 8,
        "raw-claim search order must reproduce the bounded duplicated-stock miss");
    require(
        !candidates.empty() && diagnostics.stockCappedSearchOrder &&
            candidates.front().scoreAfterToday.totalServings == 8,
        "bounded master search must rank stock-capped credits above duplicated raw claims");
}

void test_viability_reservation_and_role_seeds(const udon::MatchConfig& config, const udon::DayState& state) {
    udon::RoleAssignment higherOfficialScore;
    higherOfficialScore.rolloutValid = true;
    higherOfficialScore.rolloutScore = udon::OfficialScore{6, 60, 154};
    higherOfficialScore.sustainableCoverage = 3;
    higherOfficialScore.cheapUpperBound = udon::OfficialScore{6, 60, 200};
    higherOfficialScore.patrolCount = 7;
    higherOfficialScore.roles = {
        udon::AgentKind::Tanker,
        udon::AgentKind::Patrol,
    };
    udon::RoleAssignment higherHeuristicCoverage = higherOfficialScore;
    higherHeuristicCoverage.rolloutScore = udon::OfficialScore{6, 53, 164};
    higherHeuristicCoverage.sustainableCoverage = 6;
    higherHeuristicCoverage.roles = {
        udon::AgentKind::Patrol,
        udon::AgentKind::Tanker,
    };
    require(
        udon::role_assignment_better_after_rollout(
            higherOfficialScore,
            higherHeuristicCoverage) &&
            !udon::role_assignment_better_after_rollout(
                higherHeuristicCoverage,
                higherOfficialScore),
        "validated role rollouts must follow official lexicographic score before "
        "the sustainable-coverage heuristic");
    higherHeuristicCoverage.rolloutScore = higherOfficialScore.rolloutScore;
    higherHeuristicCoverage.sustainableCoverage =
        higherOfficialScore.sustainableCoverage;
    require(
        !udon::role_assignment_better_after_rollout(
            higherOfficialScore,
            higherHeuristicCoverage) &&
            !udon::role_assignment_better_after_rollout(
                higherHeuristicCoverage,
                higherOfficialScore),
        "role identifiers must not displace a semantically tied central seed");
    const std::vector<udon::RoleAssignment> roles = udon::RoleAssignmentEnumerator(config).shortlist(3);
    const std::vector<udon::RoleAssignment> exhaustive =
        udon::RoleAssignmentEnumerator(config).shortlist(1 << config.agent_count());
    require(roles.size() == 3U, "role beam must retain its requested cardinality");
    require(
        exhaustive.size() == static_cast<std::size_t>((1 << config.agent_count()) - 1),
        "full cheap role scan must evaluate 2^N masks and prune only the zero-patrol mask against the feasible seed");
    const std::vector<udon::RoleAssignment> deadlineFallback =
        udon::RoleAssignmentEnumerator(config).shortlist(
            1 << config.agent_count(),
            std::chrono::steady_clock::now());
    require(
        deadlineFallback.size() == exhaustive.size() &&
            std::all_of(
                deadlineFallback.begin(),
                deadlineFallback.end(),
                [&config](const udon::RoleAssignment& assignment) {
                    return assignment.cheapUpperBound.lifetimeDistinct == config.brand_count();
                }),
        "deadline-limited role scan must retain conservative all-brand upper bounds for unknown reachability");
    require(
        std::equal(
            roles.begin(),
            roles.end(),
            exhaustive.begin(),
            [](const udon::RoleAssignment& left, const udon::RoleAssignment& right) {
                return left.roles == right.roles &&
                    left.cheapUpperBound == right.cheapUpperBound;
            }),
        "deep role beam must be the true top-B result of the complete mask scan");

    const auto make_column = [](int id, int agent, udon::AgentPlan actions, int priority) {
        udon::RouteColumn column;
        column.columnId = id;
        column.agent = agent;
        column.actions = std::move(actions);
        column.priority = priority;
        return column;
    };
    udon::RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(3U);
    portfolio.columnsByAgent.at(0).push_back(make_column(0, 0, {udon::PlanAction::wait(16)}, 0));
    portfolio.columnsByAgent.at(1).push_back(make_column(1, 1, {udon::PlanAction::wait(16)}, 0));
    portfolio.columnsByAgent.at(2).push_back(make_column(2, 2, {udon::PlanAction::wait(16)}, 100));
    portfolio.columnsByAgent.at(2).push_back(make_column(
        3,
        2,
        {
            udon::PlanAction::move(5),
            udon::PlanAction::move(4),
            udon::PlanAction::move(5),
            udon::PlanAction::wait(10),
        },
        0));

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    udon::MasterOptions options;
    options.maximumCombinations = 2;
    options.maximumCandidates = 1;
    options.maximumResolveRounds = 1;
    options.mandatoryReservations.push_back(udon::MandatoryReservation{2, 2, 1, true});
    udon::MasterDiagnostics diagnostics;
    const std::vector<udon::MasterCandidate> candidates = master.solve(
        state,
        udon::MatchLedger{},
        portfolio,
        options,
        diagnostics);
    require(!candidates.empty(), "proven reservation must retain an exact serving witness");
    require(
        std::any_of(
            candidates.front().simulation.claims.begin(),
            candidates.front().simulation.claims.end(),
            [](const udon::ClaimEvent& claim) { return claim.spot == 2 && claim.served; }),
        "master must enforce a proven reservation from exact simulator claims, not column annotations");
}

void test_role_selection_diagnostics_are_opt_in(
    const udon::MatchConfig& config) {
    udon::UdonShieldEngine engine(config);
    const udon::RoleSelectionDiagnostics diagnostics =
        engine.select_roles_until_with_diagnostics(
            std::chrono::milliseconds{1000},
            3);
    require(
        !diagnostics.assignments.empty() &&
            diagnostics.rolloutDailyDistinct.size() ==
                diagnostics.assignments.size(),
        "opt-in role diagnostics must stay aligned with ranked assignments");
    require(
        std::any_of(
            diagnostics.rolloutDailyDistinct.begin(),
            diagnostics.rolloutDailyDistinct.end(),
            [](const std::vector<std::int32_t>& trace) {
                return !trace.empty();
            }),
        "opt-in role diagnostics must retain the per-day rollout trace");
    require(
        std::all_of(
            diagnostics.rolloutDailyDistinct.begin(),
            diagnostics.rolloutDailyDistinct.end(),
            [&config](const std::vector<std::int32_t>& trace) {
                return trace.size() <=
                    static_cast<std::size_t>(config.day_count());
            }),
        "role diagnostic traces must not exceed the configured horizon");
}

void test_contingency_seed_bundle_is_atomic(const udon::MatchConfig& config, const udon::DayState& state) {
    const auto make_column = [](int id, int agent, udon::AgentPlan actions, int bundle, int priority) {
        udon::RouteColumn column;
        column.columnId = id;
        column.agent = agent;
        column.actions = std::move(actions);
        column.contingencyBundle = bundle;
        column.priority = priority;
        return column;
    };
    udon::RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(3U);
    portfolio.columnsByAgent.at(0).push_back(make_column(0, 0, {udon::PlanAction::wait(16)}, 9, 0));
    portfolio.columnsByAgent.at(1).push_back(make_column(1, 1, {udon::PlanAction::wait(16)}, 9, 0));
    portfolio.columnsByAgent.at(2).push_back(make_column(2, 2, {udon::PlanAction::wait(16)}, 9, 0));
    portfolio.columnsByAgent.at(2).push_back(make_column(
        3,
        2,
        {
            udon::PlanAction::move(5),
            udon::PlanAction::move(4),
            udon::PlanAction::move(5),
            udon::PlanAction::wait(10),
        },
        -1,
        100));

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    udon::MasterOptions options;
    options.maximumCombinations = 10;
    options.maximumCandidates = 4;
    options.maximumResolveRounds = 1;
    udon::MasterDiagnostics diagnostics;
    const std::vector<udon::MasterCandidate> candidates = master.solve(
        state,
        udon::MatchLedger{},
        portfolio,
        options,
        diagnostics);
    require(!candidates.empty(), "the complete contingency bundle must remain an available candidate");
    require(
        diagnostics.combinationsVisited == 1 && diagnostics.branchesPruned > 0,
        "atomic contingency incompatibility must be pruned before reaching master leaves");
    const bool mixedSeedClaimedSpot = std::any_of(
        candidates.front().simulation.claims.begin(),
        candidates.front().simulation.claims.end(),
        [](const udon::ClaimEvent& claim) { return claim.spot == 2 && claim.served; });
    require(!mixedSeedClaimedSpot, "a cached contingency bundle must not mix with a different agent route");
}

void test_end_step_docking(const udon::MatchConfig& config, const udon::DayState& state) {
    udon::DayState dockingState = state;
    dockingState.agents.at(1).position = 18;
    const auto make_column = [](int id, int agent, udon::AgentPlan actions, int group, int terminal) {
        udon::RouteColumn column;
        column.columnId = id;
        column.agent = agent;
        column.actions = std::move(actions);
        column.escortGroup = group;
        column.terminalCell = terminal;
        return column;
    };
    udon::RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(3U);
    portfolio.columnsByAgent.at(0).push_back(make_column(
        0,
        0,
        {udon::PlanAction::move(2), udon::PlanAction::wait(14)},
        7,
        17));
    portfolio.columnsByAgent.at(1).push_back(make_column(
        1,
        1,
        {udon::PlanAction::move(5), udon::PlanAction::wait(14)},
        7,
        17));
    portfolio.columnsByAgent.at(2).push_back(make_column(
        2,
        2,
        {udon::PlanAction::wait(16)},
        -1,
        dockingState.agents.at(2).position));
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    udon::MasterDiagnostics diagnostics;
    udon::MasterOptions options;
    options.maximumCombinations = 1;
    options.maximumCandidates = 1;
    options.maximumResolveRounds = 1;
    const std::vector<udon::MasterCandidate> candidates = master.solve(
        dockingState,
        udon::MatchLedger{},
        portfolio,
        options,
        diagnostics);
    require(!candidates.empty(), "end-step docking pair must be accepted by the master");
    require(
        candidates.front().simulation.finalAgents.at(0).fuel == config.fuelLimit,
        "patrol must receive fuel from a tanker co-located at the final step");
}

void test_midday_rendezvous_refuel(const udon::MatchConfig& config, const udon::DayState& state) {
    udon::DayState rendezvousState = state;
    rendezvousState.agents.at(0).fuel = 1;
    rendezvousState.agents.at(1).position = 18;
    const auto make_column = [](int id, int agent, udon::AgentPlan actions) {
        udon::RouteColumn column;
        column.columnId = id;
        column.agent = agent;
        column.actions = std::move(actions);
        return column;
    };
    udon::RoutePortfolio portfolio;
    portfolio.columnsByAgent.resize(3U);
    udon::RouteColumn patrol = make_column(
        0,
        0,
        {
            udon::PlanAction::move(2),
            udon::PlanAction::wait(1),
            udon::PlanAction::move(4),
            udon::PlanAction::move(5),
            udon::PlanAction::wait(9),
        });
    patrol.requiredRefuels.push_back(udon::RefuelEvent{17, 3});
    portfolio.columnsByAgent.at(0).push_back(std::move(patrol));
    udon::RouteColumn tanker = make_column(
        1,
        1,
        {udon::PlanAction::move(5), udon::PlanAction::wait(14)});
    tanker.providedRefuels.push_back(udon::RefuelEvent{17, 3});
    portfolio.columnsByAgent.at(1).push_back(std::move(tanker));
    portfolio.columnsByAgent.at(2).push_back(make_column(2, 2, {udon::PlanAction::wait(16)}));

    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::RouteMaster master(config, simulator, validator);
    udon::MasterOptions options;
    options.maximumCombinations = 1;
    options.maximumCandidates = 1;
    options.maximumResolveRounds = 1;
    udon::MasterDiagnostics diagnostics;
    const std::vector<udon::MasterCandidate> candidates = master.solve(
        rendezvousState,
        udon::MatchLedger{},
        portfolio,
        options,
        diagnostics);
    require(!candidates.empty(), "timestamped tanker coverage must admit a valid split rendezvous route");
    require(candidates.front().simulation.finalAgents.at(0).fuel == config.fuelLimit - 2,
            "midday refuel must restore fuel before the patrol leaves the rendezvous");

    portfolio.columnsByAgent.at(1).front().providedRefuels.clear();
    const std::vector<udon::MasterCandidate> rejected = master.solve(
        rendezvousState,
        udon::MatchLedger{},
        portfolio,
        options,
        diagnostics);
    require(rejected.empty(), "master must reject a patrol refuel requirement without matching tanker coverage");

    const udon::ParetoRouter router(config);
    const udon::RouteColumnGenerator generator(config, router);
    udon::ColumnGenerationOptions generationOptions;
    generationOptions.maximumPathsPerTarget = 1;
    generationOptions.maximumColumnsPerAgent = 64;
    generationOptions.maximumTargetSpots = 3;
    generationOptions.maximumEscorts = 16;
    const udon::RoutePortfolio generated = generator.generate(
        rendezvousState,
        udon::MatchLedger{},
        generationOptions);
    bool generatedPair = false;
    bool generatedSharedSegment = false;
    for (const udon::RouteColumn& generatedPatrol : generated.columnsByAgent.at(0)) {
        for (const udon::RefuelEvent& required : generatedPatrol.requiredRefuels) {
            generatedPair = std::any_of(
                generated.columnsByAgent.at(1).begin(),
                generated.columnsByAgent.at(1).end(),
                [&required, &generatedPatrol, &generatedSharedSegment](
                    const udon::RouteColumn& generatedTanker) {
                    const bool covers = std::find(
                        generatedTanker.providedRefuels.begin(),
                        generatedTanker.providedRefuels.end(),
                        required) != generatedTanker.providedRefuels.end();
                    if (covers && generatedPatrol.escortGroup >= 0 &&
                        generatedPatrol.escortGroup == generatedTanker.escortGroup &&
                        !generatedPatrol.escortSegments.empty() &&
                        generatedPatrol.escortSegments.front().lastStep >
                            generatedPatrol.escortSegments.front().firstStep) {
                        generatedSharedSegment = true;
                    }
                    return covers;
                });
            if (generatedPair) {
                break;
            }
        }
        if (generatedPair) {
            break;
        }
    }
    require(generatedPair, "column generator must emit a compatible split-rendezvous pair when direct patrol fuel is insufficient");
    require(
        generatedSharedSegment,
        "split rendezvous generation must expose an exact atomic shared escort segment");
}

void test_multi_patrol_lockstep_escort(
    const udon::MatchConfig& config,
    const udon::DayState& state) {
    udon::DayState groupedState = state;
    groupedState.agents.at(2).position = groupedState.agents.at(1).position;
    const udon::ParetoRouter router(config);
    const udon::RouteColumnGenerator generator(config, router);
    udon::ColumnGenerationOptions options;
    options.maximumPathsPerTarget = 1;
    options.maximumColumnsPerAgent = 128;
    options.maximumTargetSpots = 3;
    options.maximumEscorts = 16;
    const udon::RoutePortfolio portfolio = generator.generate(
        groupedState,
        udon::MatchLedger{},
        options);
    bool foundGroup = false;
    for (const udon::RouteColumn& tanker : portfolio.columnsByAgent.at(1)) {
        if (tanker.escortGroup < 0 || !tanker.lockstepEscort) {
            continue;
        }
        const auto matching = [&tanker](const udon::RouteColumn& patrol) {
            return patrol.escortGroup == tanker.escortGroup &&
                patrol.lockstepEscort;
        };
        if (std::any_of(
                portfolio.columnsByAgent.at(0).begin(),
                portfolio.columnsByAgent.at(0).end(),
                matching) &&
            std::any_of(
                portfolio.columnsByAgent.at(2).begin(),
                portfolio.columnsByAgent.at(2).end(),
                matching)) {
            foundGroup = true;
            break;
        }
    }
    require(
        foundGroup,
        "one tanker must be able to expose an atomic lockstep segment to multiple co-located patrols");
}

void test_normal_decision_requires_certified_witnesses(const udon::MatchConfig& config, const udon::DayState& state) {
    udon::UdonShieldEngine engine(config);
    const udon::DecisionResult decision = engine.solve_day(
        state,
        udon::MatchLedger{},
        std::chrono::milliseconds{1000});
    require(!decision.emergency, "normal deadline must produce a normal decision");
    for (const udon::ScenarioOutcome& outcome : decision.profile.outcomes) {
        require(outcome.witness.certified, "normal decision must not send a bounds-only profile");
        if (!outcome.witness.lowerBoundOnly) {
            require(
                outcome.witness.futurePlans.size() ==
                    static_cast<std::size_t>(config.day_count() - state.dayNumber),
                "W1 must certify a complete remaining-horizon recourse witness");
        }
    }
    require(
        !decision.viability.conditionalTiers.empty() &&
            !decision.viability.frontier.empty() &&
            !decision.profile.conditionalTiers.empty() &&
            !decision.profile.frontier.empty(),
        "normal decisions must carry conditional tier bounds and a non-empty viability frontier");
    require(
        decision.audit.independentPlansEvaluated > 0 &&
            decision.timing.independentGenerators > std::chrono::milliseconds{0},
        "the independent blank-slate portfolio must participate through the production shield");
    const udon::ExactStepSimulator simulator(config);
    const udon::SimulationResult result = simulator.simulate(state, decision.candidate.plan, false);
    require(result.valid, "normal decision plan must be valid");
    const udon::JsonValue replay = udon::serialize_decision_replay(config, state, udon::MatchLedger{}, decision);
    const udon::JsonValue parsedReplay = udon::JsonValue::parse(replay.dump());
    require(
        parsedReplay.at("schema").string() == "udon-shield-replay-v1",
        "decision replay must use the stable audit schema");
    require(
        !parsedReplay.at("decision").at("candidate").at("simulation").at("trace").at("positions").array().empty(),
        "decision replay must retain the exact final simulator trace");
    require(
        parsedReplay.at("decision").at("riskPolicy").at("version").string() == "static-v1",
        "replay must freeze the versioned static risk policy used for the decision");
    require(
        parsedReplay.at("decision").at("manifest").at("riskPolicyVersion").string() == "static-v1" &&
            !parsedReplay.at("decision").at("manifest").at("evaluatorHash").string().empty(),
        "scenario manifest must freeze risk parameters and evaluator provenance");
    require(
        decision.audit.profileFinalization.passes == 2,
        "normal decision must finalize provisional and post-W1 profiles at exactly two checkpoints");
    require(
        parsedReplay.at("decision").at("timing").at("totalMs").integer() == decision.timing.total.count(),
        "replay must retain phase timing for deadline and performance review");
    require(
        decision.audit.optimalityGap.todayPortfolio.validEnvelope &&
            decision.audit.optimalityGap.candidateHorizon.validEnvelope &&
            decision.audit.optimalityGap.viabilityHorizon.validEnvelope &&
            decision.audit.optimalityGap.absoluteHorizon.validEnvelope,
        "all optimality gap diagnostics must preserve valid lower/upper envelopes");
    require(
        parsedReplay.at("decision").at("audit").at("optimalityGap")
                .at("todayPortfolio").at("firstOpenTier").integer() ==
            decision.audit.optimalityGap.todayPortfolio.firstOpenTier,
        "replay must retain the decomposed first open lexicographic tier");
    require(
        !decision.audit.selectionReason.empty() && !decision.audit.candidates.empty(),
        "normal decision must retain candidate dispositions for replay review");
    require(
        std::any_of(
            decision.audit.candidates.begin(),
            decision.audit.candidates.end(),
            [](const udon::CandidateAuditRecord& record) {
                return record.w1Role.find("floor-leader") != std::string::npos;
            }) &&
            std::any_of(
                decision.audit.candidates.begin(),
                decision.audit.candidates.end(),
                [](const udon::CandidateAuditRecord& record) {
                    return record.w1Role.find("incumbent") != std::string::npos ||
                        record.w1Role.find("last-sent") != std::string::npos;
                }),
        "W1 shortlist must contain the provisional floor leader and incumbent or last-sent candidate");
}

void test_w0_reuses_only_revalidated_cached_contingency(
    const udon::MatchConfig& config,
    const udon::DayState& state) {
    udon::UdonShieldEngine engine(config);
    udon::DecisionResult dayOne = engine.solve_day(state, udon::MatchLedger{}, std::chrono::milliseconds{1});
    udon::DayState dayTwo = state;
    dayTwo.dayNumber = 2;
    dayTwo.agents = dayOne.candidate.simulation.finalAgents;
    const udon::DayPlan cachedPlan = udon::emergency_wait_plan(config, dayTwo);
    dayOne.profile.outcomes.clear();
    dayOne.profile.outcomes.push_back(udon::ScenarioOutcome{
        dayOne.candidate.scoreAfterToday,
        udon::FutureWitness{{cachedPlan}, dayOne.candidate.scoreAfterToday, true},
    });
    engine.record_submitted(dayOne, std::chrono::milliseconds{10});
    require(
        engine.response_ledger().cachedContingencies.size() == 1U,
        "a certified next-day witness must be materialized into the response cache");
    udon::DecisionResult uncertifiedResend = dayOne;
    uncertifiedResend.profile.outcomes.front().witness.certified = false;
    uncertifiedResend.emergency = true;
    require(
        !engine.may_submit(uncertifiedResend) &&
        engine.response_ledger().cachedContingencies.size() == 1U &&
            engine.response_ledger().lastProfile->outcomes.front().witness.certified,
        "a duplicate response must be suppressed while retaining the prior certificate and contingency cache");

    udon::MatchLedger dayTwoLedger;
    dayTwoLedger.apply(dayOne.candidate.simulation.score);
    const udon::DecisionResult dayTwoDecision = engine.solve_day(
        dayTwo,
        dayTwoLedger,
        std::chrono::milliseconds{1000});
    require(!dayTwoDecision.emergency, "a normal deadline must retain the W0 path");
    require(
        dayTwoDecision.cacheRepair.eligibleContingencies == 1,
        "W0 must inspect only the contingency scheduled for the authoritative next day");
    require(
        dayTwoDecision.cacheRepair.reusedContingencies == 1 &&
            dayTwoDecision.cacheRepair.rejectedContingencies == 0,
        "a cached plan must enter the portfolio only after exact revalidation succeeds");

    udon::UdonShieldEngine invalidCacheEngine(config);
    udon::DecisionResult invalidDayOne = invalidCacheEngine.solve_day(
        state,
        udon::MatchLedger{},
        std::chrono::milliseconds{1});
    udon::DayPlan invalidCachedPlan = cachedPlan;
    invalidCachedPlan.actions.front() = udon::AgentPlan{udon::PlanAction::wait(1)};
    invalidDayOne.profile.outcomes.clear();
    invalidDayOne.profile.outcomes.push_back(udon::ScenarioOutcome{
        invalidDayOne.candidate.scoreAfterToday,
        udon::FutureWitness{{invalidCachedPlan}, invalidDayOne.candidate.scoreAfterToday, true},
    });
    invalidCacheEngine.record_submitted(invalidDayOne, std::chrono::milliseconds{10});
    udon::MatchLedger invalidDayTwoLedger;
    invalidDayTwoLedger.apply(invalidDayOne.candidate.simulation.score);
    const udon::DecisionResult invalidCacheDecision = invalidCacheEngine.solve_day(
        dayTwo,
        invalidDayTwoLedger,
        std::chrono::milliseconds{1000});
    require(
        invalidCacheDecision.cacheRepair.reusedContingencies == 0 &&
            invalidCacheDecision.cacheRepair.rejectedContingencies == 1,
        "an invalid cached witness must be rejected instead of being reused blindly");
}

void test_match_session_ack_and_precompute(const udon::MatchConfig& config, const udon::DayState& state) {
    udon::MatchSession deferredSession(config);
    const std::chrono::system_clock::time_point receivedAt =
        std::chrono::system_clock::time_point{std::chrono::seconds{state.endsAt}} - std::chrono::milliseconds{1000};
    const udon::SessionDecision deferredPending = deferredSession.on_authoritative_state(
        state,
        udon::MatchLedger{},
        receivedAt);
    require(deferredPending.maySubmit, "deferred post-ACK fixture must produce a submit-ready decision");
    const udon::PostAckWork deferredAck = deferredSession.acknowledge_submitted(
        std::chrono::milliseconds{10});
    require(
        deferredAck.cachedContingencies == 0 && deferredAck.completedProofs == 0 &&
            !deferredSession.has_pending_submission(),
        "a zero-budget ACK must close submission immediately without running synchronous idle work");
    static_cast<void>(deferredSession.precompute_until(std::chrono::milliseconds{50}));
    require(
        deferredSession.remaining_post_ack_compute_budget() <=
            udon::kCompetitionComputeHardCap,
        "incremental post-ACK work must never increase the remaining compute budget");

    udon::MatchSession session(config);
    const udon::SessionDecision pending = session.on_authoritative_state(
        state,
        udon::MatchLedger{},
        receivedAt);
    require(pending.maySubmit && session.has_pending_submission(),
            "a first authoritative decision must remain pending until the transport ACK arrives");
    const udon::PostAckWork postAck = session.acknowledge_submitted(
        std::chrono::milliseconds{10},
        std::chrono::milliseconds{50});
    require(!session.has_pending_submission(), "an ACK must close the pending submission lifecycle");
    require(
        postAck.cachedContingencies > 0 &&
            !session.response_ledger().cachedContingencies.empty(),
        "ACK idle budget must automatically cache at least the fail-closed next-day contingency");
    udon::MatchSession restoredArtifactsSession(config);
    restoredArtifactsSession.restore_response_artifacts(
        session.response_ledger().cachedContingencies,
        session.response_ledger().strongProofs);
    require(
        restoredArtifactsSession.response_ledger().cachedContingencies.size() ==
            session.response_ledger().cachedContingencies.size(),
        "a cold resume must retain every completed cached contingency");
    udon::DayState proofState = state;
    proofState.dayNumber = 3;
    udon::MatchSession proofSession(config);
    const udon::SessionDecision proofPending = proofSession.on_authoritative_state(
        proofState,
        udon::MatchLedger{},
        receivedAt);
    require(proofPending.maySubmit, "strong proof fixture must produce a submit-ready decision");
    const udon::PostAckWork proofWork = proofSession.acknowledge_submitted(
        std::chrono::milliseconds{10},
        std::chrono::milliseconds{3000});
    require(
        proofWork.completedProofs > 0 &&
            !proofSession.response_ledger().strongProofs.empty(),
        "ACK idle budget must complete at least one frozen-scenario horizon proof");
    restoredArtifactsSession.restore_response_artifacts(
        proofSession.response_ledger().cachedContingencies,
        proofSession.response_ledger().strongProofs);
    require(
        restoredArtifactsSession.response_ledger().strongProofs.size() ==
            proofSession.response_ledger().strongProofs.size() &&
            restoredArtifactsSession.response_ledger().strongProofs.front().bestScore ==
                proofSession.response_ledger().strongProofs.front().bestScore,
        "a cold resume must retain every completed strong-proof result");
    const udon::ResponseLedger::StrongProofRecord& proof =
        proofSession.response_ledger().strongProofs.front();
    require(
        proof.complete && !proof.infeasible &&
            proof.scope == "remaining-horizon-persistent-frozen-scenario-route-portfolios-v1" &&
            udon::compare_lexicographic(proof.bestScore, proof.upperBound) == 0,
        "completed strong proof must close its exact portfolio score gap");
}

void test_match_session_records_external_wait_traffic(
    const udon::MatchConfig& config,
    const udon::DayState& state) {
    udon::ExactStepSimulator simulator(config);
    udon::MatchSession session(config);
    udon::DayState dayOne = state;
    dayOne.dayNumber = 1;
    dayOne.agents.front().position = config.roadCells.front();
    const udon::DayPlan dayOneWait = udon::make_wait_plan(config, 1);
    const udon::SimulationResult dayOneSimulation = simulator.simulate(
        dayOne,
        dayOneWait,
        false);
    require(dayOneSimulation.valid,
        "external WAIT traffic fixture must be valid");
    require(
        dayOneSimulation.roadFootprint.at(
            static_cast<std::size_t>(config.roadCells.front())) > 0,
        "WAIT on a road must contribute to the exact own footprint");
    session.record_applied_transition(dayOne, dayOneSimulation);

    udon::DayState dayTwo = dayOne;
    dayTwo.dayNumber = 2;
    dayTwo.agents = dayOneSimulation.finalAgents;
    const udon::SimulationResult dayTwoSimulation = simulator.simulate(
        dayTwo,
        udon::make_wait_plan(config, 2),
        false);
    require(dayTwoSimulation.valid,
        "second external WAIT traffic fixture must be valid");
    session.record_applied_transition(dayTwo, dayTwoSimulation);
    require(
        session.previous_own_footprint() == dayOneSimulation.roadFootprint,
        "resume and fallback transitions must restore the previous-day WAIT footprint");
}

void test_expired_witness_respects_deadline(const udon::MatchConfig& config, const udon::DayState& state) {
    const udon::ExactStepSimulator simulator(config);
    const udon::IndependentDayValidator validator(config);
    const udon::ParetoRouter router(config);
    const udon::RouteColumnGenerator generator(config, router);
    const udon::RouteMaster master(config, simulator, validator);
    const udon::RoutePortfolio portfolio = generator.generate(state, udon::MatchLedger{}, udon::ColumnGenerationOptions{});
    udon::MasterDiagnostics diagnostics;
    udon::MasterOptions masterOptions;
    masterOptions.maximumCombinations = 200;
    masterOptions.maximumCandidates = 1;
    const std::vector<udon::MasterCandidate> candidates = master.solve(
        state,
        udon::MatchLedger{},
        portfolio,
        masterOptions,
        diagnostics);
    require(!candidates.empty(), "master must provide a seed candidate for witness repair");

    udon::TrafficBelief belief(config);
    belief.observe(state);
    udon::ScenarioManifest manifest;
    udon::TrafficScenario scenario;
    scenario.scenarioId = 0;
    scenario.scenarioClass = "test";
    scenario.weight = 10000;
    scenario.opponentCurrentFootprint.assign(static_cast<std::size_t>(config.map.cell_count()), 0);
    scenario.opponentCarryFootprint.assign(static_cast<std::size_t>(config.map.cell_count()), 0);
    manifest.scenarios.push_back(std::move(scenario));
    manifest.totalWeight = 10000;

    const udon::FutureWitnessRepairer repairer(config, generator, master, simulator, validator);
    udon::CandidateProfile profile = repairer.provisional_profile(
        candidates.front(),
        state,
        udon::MatchLedger{},
        belief,
        manifest,
        candidates.front().scoreAfterToday,
        32);
    repairer.repair_profile(
        profile,
        candidates.front(),
        state,
        udon::MatchLedger{},
        belief,
        manifest,
        1,
        std::chrono::steady_clock::now() - std::chrono::milliseconds{1});
    const udon::FutureWitness& witness = profile.outcomes.front().witness;
    require(
        witness.certified && witness.lowerBoundOnly,
        "an expired W1 budget must keep the prebuilt monotone WAIT certificate without borrowing later buckets");
    require(
        witness.futurePlans.size() ==
            static_cast<std::size_t>(config.day_count() - state.dayNumber),
        "the monotone fallback certificate must remain a complete, never-partial future WAIT schedule");
}

} 

int main() {
    try {
        const udon::MatchConfig config = udon::parse_match_config(udon::JsonValue::parse(fixture_config()));
        const udon::DayState state = fixture_state(config);
        test_btc_official_wire_adapter();
        test_incomplete_long_horizon_role_fallback();
        test_even_row_geometry(config);
        test_cube_distance_and_pareto_pruning_equivalence(config);
        test_protocol_fail_closed_schema(config);
        test_full_official_brand_and_variable_day_domain();
        test_exact_step_order(config, state);
        test_refuel_requires_full_colocation_step(config, state);
        test_invalid_duration_rejected(config, state);
        test_stock_order_repeat_visit_and_non_refuel(config, state);
        test_ledger_round_trip(config);
        test_road_traffic_timing(config);
        test_pareto_cache_respects_full_query_key(config);
        test_alns_preserves_escort_group_atomicity(config, state);
        test_alns_synthesizes_route_outside_portfolio(config, state);
        test_route_pool_recombines_elite_agent_routes(config, state);
        test_exact_orienteering_terminal_frontier();
        test_anytime_orienteering_preserves_lexicographic_brands();
        test_emergency_contract(config, state);
        test_competition_compute_hard_cap(config, state);
        test_protected_slack_refiner(config, state);
        test_deadline_floors();
        test_public_traffic_scenarios(config, state);
        test_same_day_resend_preserves_prior_traffic_memory(config, state);
        test_fast_viability_bounds(config, state);
        test_fast_viability_no_tanker_claim_upper();
        test_fast_viability_emits_proven_unique_reservation();
        test_latest_safe_day_uses_suffix_budget();
        test_provisional_key_is_history_independent();
        test_final_choice_rejects_unproven_current_regret();
        test_risk_policy_and_manifest_guards();
        test_confidence_gate_and_conditional_tiers();
        test_certified_stochastic_dominance();
        test_survival_signature_completes_quantile_ties();
        test_reconciliation_and_submission_gate(config, state);
        test_column_events_and_stock_cuts(config, state);
        test_unreachable_brand_staging_column();
        test_master_lexicographic_branch_and_bound(config, state);
        test_master_bundle_aware_upper_bound(config, state);
        test_master_agent_brand_frontier_upper_bound(config, state);
        test_master_stock_capped_search_order();
        test_viability_reservation_and_role_seeds(config, state);
        test_role_selection_diagnostics_are_opt_in(config);
        test_contingency_seed_bundle_is_atomic(config, state);
        test_end_step_docking(config, state);
        test_midday_rendezvous_refuel(config, state);
        test_multi_patrol_lockstep_escort(config, state);
        test_normal_decision_requires_certified_witnesses(config, state);
        test_w0_reuses_only_revalidated_cached_contingency(config, state);
        test_match_session_ack_and_precompute(config, state);
        test_match_session_records_external_wait_traffic(config, state);
        test_expired_witness_respects_deadline(config, state);
        std::cout << "all tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
