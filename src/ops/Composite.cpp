#include "Composite.h"
#include "core/Color.h"
#include <algorithm>

namespace ops {

void compositePixel(float fr, float fg, float fb, float fa,
                    float br, float bg, float bb, float ba,
                    BlendMode mode,
                    float& out_r, float& out_g, float& out_b, float& out_a)
{
    // Clamp alpha to [0,1] (defensive)
    fa = std::min(1.0f, std::max(0.0f, fa));
    ba = std::min(1.0f, std::max(0.0f, ba));

    // Standard Porter-Duff "over" for alpha:
    // a_out = a_f + (1-a_f)*a_b
    out_a = fa + (1.0f - fa) * ba;

    // Choose the RGB contribution for the "foreground result" before alpha mixing.
    core::Color F{fr, fg, fb, fa};
    core::Color B{br, bg, bb, ba};
    core::Color blend = (mode == BlendMode::Normal) ? F : blendRGB(F, B, mode);

    // Straight-alpha compositing for RGB:
    // c_out = (a_f * c_f + (1-a_f) * a_b * c_b) / a_out   (if a_out > 0)
    // If a_b==1, this reduces to: c_out = a_f*c_f + (1-a_f)*c_b
    if (out_a > 0.0f) {
        float num_r = fa * blend.r + (1.0f - fa) * ba * br;
        float num_g = fa * blend.g + (1.0f - fa) * ba * bg;
        float num_b = fa * blend.b + (1.0f - fa) * ba * bb;

        out_r = num_r / out_a;
        out_g = num_g / out_a;
        out_b = num_b / out_a;
    } else {
        // fully transparent; color is irrelevant
        out_r = 0.0f; out_g = 0.0f; out_b = 0.0f;
    }
}

void compositeOverCentered(core::Image& dst, const core::Image& fgImg, const CompositeParams& params) {
    if (dst.channels() < 4 || fgImg.channels() < 4) return;

    const int dx0 = (dst.width()  - fgImg.width())  / 2;
    const int dy0 = (dst.height() - fgImg.height()) / 2;

    for (int y = 0; y < fgImg.height(); ++y) {
        for (int x = 0; x < fgImg.width(); ++x) {
            int dx = dx0 + x;
            int dy = dy0 + y;
            if (dx < 0 || dy < 0 || dx >= dst.width() || dy >= dst.height()) continue;

            float fr, fg, fb, fa;
            float br, bg, bb, ba;
            fgImg.getPixel(x, y, fr, fg, fb, fa);
            dst.getPixel(dx, dy, br, bg, bb, ba);

            fa *= params.opacityMultiplier;

            float or_, og_, ob_, oa_;
            compositePixel(fr, fg, fb, fa, br, bg, bb, ba, params.mode, or_, og_, ob_, oa_);
            dst.setPixel(dx, dy, or_, og_, ob_, oa_);
        }
    }
}

} // namespace ops
