#pragma once
#include "core/Color.h"

namespace ops {

enum class BlendMode {
    Normal = 0,
    Additive = 1,
    Multiply = 2,
    Screen = 3,
    Difference = 4
};

// TODO(A1): Implement RGB blend modes (alpha handled elsewhere).
core::Color blendRGB(const core::Color& fg, const core::Color& bg, BlendMode mode);

} // namespace ops