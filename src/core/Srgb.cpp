#include "Srgb.h"
#include <cmath>

namespace core {

// sRGB -> linear
float srgbToLinear(float cs) {
    if (cs <= 0.04045f) return cs / 12.92f;
    return std::pow((cs + 0.055f) / 1.055f, 2.4f);
}

// linear -> sRGB
float linearToSrgb(float cl) {
    if (cl <= 0.0031308f) return 12.92f * cl;
    return 1.055f * std::pow(cl, 1.0f / 2.4f) - 0.055f;
}

} // namespace core
