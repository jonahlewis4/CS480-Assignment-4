#pragma once
#include <algorithm>

namespace core {

struct Color {
    float r=0, g=0, b=0, a=1;
};

inline float clamp01(float x) { return std::min(1.0f, std::max(0.0f, x)); }

} // namespace core
