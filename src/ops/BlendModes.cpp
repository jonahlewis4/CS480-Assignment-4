#include "BlendModes.h"
#include <cmath>

namespace ops {

core::Color blendRGB(const core::Color& fg, const core::Color& bg, BlendMode mode) {
    core::Color out = bg;
    switch (mode) {
        case BlendMode::Normal:
            // Normal mode doesn't change rgb here; alpha composition will do the mix.
            out.r = fg.r;
            out.g = fg.g;
            out.b = fg.b;
            break;
        case BlendMode::Additive:
            out.r = fg.r + bg.r;
            out.g = fg.g + bg.g;
            out.b = fg.b + bg.b;
            break;
        case BlendMode::Multiply:
            out.r = fg.r * bg.r;
            out.g = fg.g * bg.g;
            out.b = fg.b * bg.b;
            break;
        case BlendMode::Screen:
            out.r = 1.0f - (1.0f - fg.r) * (1.0f - bg.r);
            out.g = 1.0f - (1.0f - fg.g) * (1.0f - bg.g);
            out.b = 1.0f - (1.0f - fg.b) * (1.0f - bg.b);
            break;
        case BlendMode::Difference:
            out.r = std::fabs(bg.r - fg.r);
            out.g = std::fabs(bg.g - fg.g);
            out.b = std::fabs(bg.b - fg.b);
            break;
    }
    return out;
}

} // namespace ops
