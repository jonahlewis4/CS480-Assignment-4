#include "Brightness.h"
#include <algorithm>

namespace ops {

void applyBrightness(core::Image& img, float amount) {
    auto& d = img.data();
    const int c = img.channels();
    if (c < 3) return;

    const size_t N = (size_t)img.width() * (size_t)img.height();
    for (size_t i = 0; i < N; ++i) {
        d[i*c + 0] = d[i*c + 0] + amount;
        d[i*c + 1] = d[i*c + 1] + amount;
        d[i*c + 2] = d[i*c + 2] + amount;
        // alpha unchanged
    }
}

} // namespace ops
