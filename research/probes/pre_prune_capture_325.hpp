// Research-only upstream observation; canonical bundle rules remain untouched.
#pragma once
#include "pre_f0_capture_324.hpp"
namespace udon::capture325 {
inline thread_local std::string phase;
inline void start(const char* label) {
    phase = capture324::captured.active ? label : "";
}
inline void end() { phase.clear(); }
inline void raw(const std::vector<RouteColumn>& columns) {
    if (!phase.empty() && capture324::captured.active) {
        auto& recorded = capture324::captured.portfolios[phase].columnsByAgent;
        if (recorded.size() >= capture324::captured.root.agents.size())
            throw std::logic_error("duplicate raw portfolio observation");
        recorded.push_back(columns);
    }
}
}
