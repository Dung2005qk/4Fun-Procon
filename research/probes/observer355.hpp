#pragma once
#include "udon/orienteering.hpp"
#include <cstdint>
#include <vector>
namespace udon {
struct ObservedRoute355 {ExactOrienteeringRoute route;std::vector<std::int64_t> rank;};
inline std::vector<ObservedRoute355> observed355;
}
