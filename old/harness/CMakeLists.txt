cmake_minimum_required(VERSION 3.24)

project(udon_shield_historical_tournament LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SNAPSHOT_ROOT "" CACHE PATH "Path to an exported UDON-SHIELD checkpoint")
if(NOT SNAPSHOT_ROOT)
    message(FATAL_ERROR "SNAPSHOT_ROOT is required")
endif()

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(UDONSHIELD_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
add_subdirectory("${SNAPSHOT_ROOT}" snapshot EXCLUDE_FROM_ALL)

add_executable(historical_tournament historical_tournament.cpp)
target_link_libraries(historical_tournament PRIVATE udon_shield)

if(MSVC)
    target_compile_options(historical_tournament PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(historical_tournament PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()
