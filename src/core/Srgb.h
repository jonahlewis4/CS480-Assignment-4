#pragma once
namespace core {

// Approximate sRGB transfer function (piecewise). Input/output in [0,1].
float srgbToLinear(float cs);
float linearToSrgb(float cl);

} // namespace core


