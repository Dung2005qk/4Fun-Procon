cmake_minimum_required(VERSION 3.24)

project(udon_shield LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(udon_shield
    src/json.cpp
    src/protocol.cpp
    src/btc_protocol.cpp
    src/simulator.cpp
    src/validator.cpp
    src/graph.cpp
    src/slack_refiner.cpp
    src/orienteering.cpp
    src/planner.cpp
    src/decision.cpp
    src/audit.cpp
    src/runtime.cpp
    strategies/blank_slate/planners.cpp
)

target_include_directories(
    udon_shield
    PUBLIC include
    PRIVATE strategies)

if(MSVC)
    target_compile_options(udon_shield PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(udon_shield PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

add_executable(udonshield_cli src/main.cpp)
target_link_libraries(udonshield_cli PRIVATE udon_shield)

add_executable(udonshield_btc src/btc_main.cpp)
target_link_libraries(udonshield_btc PRIVATE udon_shield)
if(MSVC)
    target_compile_options(udonshield_btc PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(udonshield_btc PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()
if(WIN32)
    target_link_libraries(udonshield_btc PRIVATE winhttp)
endif()

option(UDONSHIELD_BUILD_BENCHMARKS "Build deterministic local performance benchmark" ON)
if(UDONSHIELD_BUILD_BENCHMARKS)
    add_executable(udonshield_bench bench/benchmark_main.cpp)
    target_link_libraries(udonshield_bench PRIVATE udon_shield)
    add_executable(
        udonshield_strategy_bench
        strategies/strategy_suite.cpp)
    target_link_libraries(udonshield_strategy_bench PRIVATE udon_shield)
    add_executable(udonshield_master_oracle strategies/master_oracle.cpp)
    target_link_libraries(udonshield_master_oracle PRIVATE udon_shield)
    add_executable(
        udonshield_population_oracle
        research/probes/population_oracle.cpp)
    target_link_libraries(udonshield_population_oracle PRIVATE udon_shield)
    add_executable(
        udonshield_orienteering_oracle
        strategies/orienteering_oracle.cpp)
    target_link_libraries(udonshield_orienteering_oracle PRIVATE udon_shield)
    add_executable(
        udonshield_ceiling_oracle
        research/probes/ceiling_oracle.cpp)
    target_link_libraries(udonshield_ceiling_oracle PRIVATE udon_shield)
    add_executable(
        udonshield_full_match_oracle
        research/probes/full_match_oracle.cpp)
    target_link_libraries(udonshield_full_match_oracle PRIVATE udon_shield)
    add_executable(
        udonshield_multi_patrol_oracle
        research/probes/multi_patrol_oracle.cpp)
    target_link_libraries(udonshield_multi_patrol_oracle PRIVATE udon_shield)
    add_executable(
        udonshield_claim_probe
        research/probes/claim_probe.cpp)
    target_link_libraries(udonshield_claim_probe PRIVATE udon_shield)
    add_executable(
        udonshield_refuel_midmove_probe
        research/probes/refuel_midmove_probe.cpp)
    target_link_libraries(udonshield_refuel_midmove_probe PRIVATE udon_shield)
    add_executable(
        udonshield_colgen_starvation_probe
        research/probes/colgen_starvation_probe.cpp)
    target_link_libraries(udonshield_colgen_starvation_probe PRIVATE udon_shield)
    add_executable(
        udonshield_midday_live_yield_probe
        research/probes/midday_live_yield_probe.cpp)
    target_link_libraries(udonshield_midday_live_yield_probe PRIVATE udon_shield)
    add_executable(
        udonshield_tanker_terminal_oracle
        research/probes/tanker_terminal_oracle.cpp)
    target_link_libraries(udonshield_tanker_terminal_oracle PRIVATE udon_shield)
    add_executable(
        udonshield_tanker_match_oracle
        research/probes/tanker_match_oracle.cpp)
    target_link_libraries(udonshield_tanker_match_oracle PRIVATE udon_shield)
    add_executable(
        udonshield_bidirectional_rcsp_probe
        research/probes/bidirectional_rcsp_probe.cpp)
    target_link_libraries(udonshield_bidirectional_rcsp_probe PRIVATE udon_shield)
    add_executable(
        udonshield_bidirectional_rcsp_boundary_probe
        research/probes/bidirectional_rcsp_boundary_probe.cpp)
    target_link_libraries(udonshield_bidirectional_rcsp_boundary_probe PRIVATE udon_shield)
    add_executable(
        udonshield_incumbent_goal_rcsp_probe
        research/probes/incumbent_goal_rcsp_probe.cpp)
    target_link_libraries(udonshield_incumbent_goal_rcsp_probe PRIVATE udon_shield)
    add_executable(
        udonshield_required_terminal_lower_bound_probe
        research/probes/required_terminal_lower_bound_probe.cpp)
    target_link_libraries(udonshield_required_terminal_lower_bound_probe PRIVATE udon_shield)
    add_executable(
        udonshield_historical_tournament
        old/harness/historical_tournament.cpp)
    target_link_libraries(udonshield_historical_tournament PRIVATE udon_shield)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(udonshield_tests tests/test_main.cpp)
    target_link_libraries(udonshield_tests PRIVATE udon_shield)
    add_test(NAME udonshield_tests COMMAND udonshield_tests)
    if(TARGET udonshield_strategy_bench)
        add_test(NAME udonshield_strategy_bench_smoke COMMAND udonshield_strategy_bench --smoke)
    endif()
    if(TARGET udonshield_master_oracle)
        add_test(NAME udonshield_master_oracle_smoke COMMAND udonshield_master_oracle --seeds 50)
    endif()
endif()
