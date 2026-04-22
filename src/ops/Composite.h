#pragma once
#include "core/Image.h"
#include "BlendModes.h"

namespace ops {

struct CompositeParams {
    BlendMode mode = BlendMode::Normal;
    float opacityMultiplier = 1.0f;   // multiplies foreground alpha (e.g., UI opacity slider)
};

// Composite fg onto dst, with fg positioned so its center aligns with dst center.
void compositeOverCentered(core::Image& dst, const core::Image& fg, const CompositeParams& params);

// Core 2-layer alpha compositing (straight alpha), with optional blend mode on RGB.
// If mode == Normal, rgb is standard alpha blend.
// For other modes, we first compute blendRGB(fgRGB, bgRGB, mode) and then alpha-blend that result.
void compositePixel(float fr, float fg, float fb, float fa,
                    float br, float bg, float bb, float ba,
                    BlendMode mode,
                    float& out_r, float& out_g, float& out_b, float& out_a);

} // namespace ops
