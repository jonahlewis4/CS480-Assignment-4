#include "Image.h"
#include "Srgb.h"
#include "Color.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cctype>

#include <stb_image.h>
#include <stb_image_write.h>

namespace core {

    static inline float clamp01f(float x) { return clamp01(x); }

    // -----------------------------
    // 5x7 font: each glyph is 7 rows, each row is 5 bits (MSB on left).
    // Supported: A-Z, 0-9, space, '-', '_', '.'
    // -----------------------------
    static const uint8_t* glyph5x7(char c) {
        c = (char)std::toupper((unsigned char)c);

        static const uint8_t SPACE[7] = { 0,0,0,0,0,0,0 };
        static const uint8_t DOT[7] = { 0,0,0,0,0,0b00100,0 };
        static const uint8_t DASH[7] = { 0,0,0,0b11111,0,0,0 };
        static const uint8_t UND[7] = { 0,0,0,0,0,0,0b11111 };

        static const uint8_t D0[7] = { 0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110 };
        static const uint8_t D1[7] = { 0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110 };
        static const uint8_t D2[7] = { 0b01110,0b10001,0b00001,0b00010,0b00100,0b01000,0b11111 };
        static const uint8_t D3[7] = { 0b01110,0b10001,0b00001,0b00110,0b00001,0b10001,0b01110 };
        static const uint8_t D4[7] = { 0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010 };
        static const uint8_t D5[7] = { 0b11111,0b10000,0b11110,0b00001,0b00001,0b10001,0b01110 };
        static const uint8_t D6[7] = { 0b00110,0b01000,0b10000,0b11110,0b10001,0b10001,0b01110 };
        static const uint8_t D7[7] = { 0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000 };
        static const uint8_t D8[7] = { 0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110 };
        static const uint8_t D9[7] = { 0b01110,0b10001,0b10001,0b01111,0b00001,0b00010,0b01100 };

        static const uint8_t A[7] = { 0b01110,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001 };
        static const uint8_t B[7] = { 0b11110,0b10001,0b10001,0b11110,0b10001,0b10001,0b11110 };
        static const uint8_t C[7] = { 0b01110,0b10001,0b10000,0b10000,0b10000,0b10001,0b01110 };
        static const uint8_t D[7] = { 0b11110,0b10001,0b10001,0b10001,0b10001,0b10001,0b11110 };
        static const uint8_t E[7] = { 0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b11111 };
        static const uint8_t F[7] = { 0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b10000 };
        static const uint8_t G[7] = { 0b01110,0b10001,0b10000,0b10111,0b10001,0b10001,0b01110 };
        static const uint8_t H[7] = { 0b10001,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001 };
        static const uint8_t I[7] = { 0b01110,0b00100,0b00100,0b00100,0b00100,0b00100,0b01110 };
        static const uint8_t J[7] = { 0b00111,0b00010,0b00010,0b00010,0b00010,0b10010,0b01100 };
        static const uint8_t K[7] = { 0b10001,0b10010,0b10100,0b11000,0b10100,0b10010,0b10001 };
        static const uint8_t L[7] = { 0b10000,0b10000,0b10000,0b10000,0b10000,0b10000,0b11111 };
        static const uint8_t M[7] = { 0b10001,0b11011,0b10101,0b10101,0b10001,0b10001,0b10001 };
        static const uint8_t N[7] = { 0b10001,0b11001,0b10101,0b10011,0b10001,0b10001,0b10001 };
        static const uint8_t O[7] = { 0b01110,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110 };
        static const uint8_t P[7] = { 0b11110,0b10001,0b10001,0b11110,0b10000,0b10000,0b10000 };
        static const uint8_t Q[7] = { 0b01110,0b10001,0b10001,0b10001,0b10101,0b10010,0b01101 };
        static const uint8_t R[7] = { 0b11110,0b10001,0b10001,0b11110,0b10100,0b10010,0b10001 };
        static const uint8_t S[7] = { 0b01111,0b10000,0b10000,0b01110,0b00001,0b00001,0b11110 };
        static const uint8_t T[7] = { 0b11111,0b00100,0b00100,0b00100,0b00100,0b00100,0b00100 };
        static const uint8_t U[7] = { 0b10001,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110 };
        static const uint8_t V[7] = { 0b10001,0b10001,0b10001,0b10001,0b10001,0b01010,0b00100 };
        static const uint8_t W[7] = { 0b10001,0b10001,0b10001,0b10101,0b10101,0b10101,0b01010 };
        static const uint8_t X[7] = { 0b10001,0b01010,0b00100,0b00100,0b00100,0b01010,0b10001 };
        static const uint8_t Y[7] = { 0b10001,0b01010,0b00100,0b00100,0b00100,0b00100,0b00100 };
        static const uint8_t Z[7] = { 0b11111,0b00001,0b00010,0b00100,0b01000,0b10000,0b11111 };

        switch (c) {
        case ' ': return SPACE;
        case '.': return DOT;
        case '-': return DASH;
        case '_': return UND;

        case '0': return D0; case '1': return D1; case '2': return D2; case '3': return D3; case '4': return D4;
        case '5': return D5; case '6': return D6; case '7': return D7; case '8': return D8; case '9': return D9;

        case 'A': return A; case 'B': return B; case 'C': return C; case 'D': return D; case 'E': return E;
        case 'F': return F; case 'G': return G; case 'H': return H; case 'I': return I; case 'J': return J;
        case 'K': return K; case 'L': return L; case 'M': return M; case 'N': return N; case 'O': return O;
        case 'P': return P; case 'Q': return Q; case 'R': return R; case 'S': return S; case 'T': return T;
        case 'U': return U; case 'V': return V; case 'W': return W; case 'X': return X; case 'Y': return Y;
        case 'Z': return Z;
        default:  return SPACE;
        }
    }

    Image::Image(int w, int h, int c) : m_w(w), m_h(h), m_c(c) {
        m_data.assign((size_t)m_w * (size_t)m_h * (size_t)m_c, 0.0f);
    }

    void Image::clear(float r, float g, float b, float a) {
        for (int y = 0; y < m_h; ++y)
            for (int x = 0; x < m_w; ++x)
                setPixel(x, y, r, g, b, a);
    }

    void Image::fillCheckerboard(int cell) {
        for (int y = 0; y < m_h; ++y) {
            for (int x = 0; x < m_w; ++x) {
                int cx = x / cell;
                int cy = y / cell;
                bool odd = ((cx + cy) & 1) == 1;
                float v = odd ? 0.75f : 0.35f;
                setPixel(x, y, v, v, v, 1.0f);
            }
        }
    }

    bool Image::loadFromFile(const std::string& path) {
        int w = 0, h = 0, comp = 0;
        stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &comp, 4);
        if (!pixels) {
            std::fprintf(stderr, "stbi_load failed: %s\n", path.c_str());
            return false;
        }

        m_w = w; m_h = h; m_c = 4;
        m_data.resize((size_t)m_w * (size_t)m_h * 4);

        const size_t N = (size_t)m_w * (size_t)m_h;
        for (size_t i = 0; i < N; ++i) {
            m_data[i * 4 + 0] = pixels[i * 4 + 0] / 255.0f;
            m_data[i * 4 + 1] = pixels[i * 4 + 1] / 255.0f;
            m_data[i * 4 + 2] = pixels[i * 4 + 2] / 255.0f;
            m_data[i * 4 + 3] = pixels[i * 4 + 3] / 255.0f;
        }

        stbi_image_free(pixels);
        return true;
    }

    bool Image::saveToPng(const std::string& path) const {
        auto bytes = toRGBA8();
        const int stride = m_w * 4;
        return stbi_write_png(path.c_str(), m_w, m_h, 4, bytes.data(), stride) != 0;
    }

    void Image::srgbToLinearInPlace() {
        if (m_c < 3) return;
        const size_t N = (size_t)m_w * (size_t)m_h;
        for (size_t i = 0; i < N; ++i) {
            m_data[i * 4 + 0] = srgbToLinear(m_data[i * 4 + 0]);
            m_data[i * 4 + 1] = srgbToLinear(m_data[i * 4 + 1]);
            m_data[i * 4 + 2] = srgbToLinear(m_data[i * 4 + 2]);
        }
    }

    void Image::linearToSrgbInPlace() {
        if (m_c < 3) return;
        const size_t N = (size_t)m_w * (size_t)m_h;
        for (size_t i = 0; i < N; ++i) {
            m_data[i * 4 + 0] = linearToSrgb(std::max(0.0f, m_data[i * 4 + 0]));
            m_data[i * 4 + 1] = linearToSrgb(std::max(0.0f, m_data[i * 4 + 1]));
            m_data[i * 4 + 2] = linearToSrgb(std::max(0.0f, m_data[i * 4 + 2]));
        }
    }

    std::vector<std::uint8_t> Image::toRGBA8() const {
        std::vector<std::uint8_t> out((size_t)m_w * (size_t)m_h * 4);
        const size_t N = (size_t)m_w * (size_t)m_h;
        for (size_t i = 0; i < N; ++i) {
            float r = m_data[i * 4 + 0];
            float g = m_data[i * 4 + 1];
            float b = m_data[i * 4 + 2];
            float a = m_data[i * 4 + 3];
            out[i * 4 + 0] = (std::uint8_t)std::round(clamp01f(r) * 255.0f);
            out[i * 4 + 1] = (std::uint8_t)std::round(clamp01f(g) * 255.0f);
            out[i * 4 + 2] = (std::uint8_t)std::round(clamp01f(b) * 255.0f);
            out[i * 4 + 3] = (std::uint8_t)std::round(clamp01f(a) * 255.0f);
        }
        return out;
    }

    void Image::setPixel(int x, int y, float r, float g, float b, float a) {
        if (x < 0 || y < 0 || x >= m_w || y >= m_h) return;
        const size_t idx = ((size_t)y * (size_t)m_w + (size_t)x) * (size_t)m_c;
        m_data[idx + 0] = r;
        if (m_c > 1) m_data[idx + 1] = g;
        if (m_c > 2) m_data[idx + 2] = b;
        if (m_c > 3) m_data[idx + 3] = a;
    }

    void Image::getPixel(int x, int y, float& r, float& g, float& b, float& a) const {
        r = g = b = 0.0f; a = 1.0f;
        if (x < 0 || y < 0 || x >= m_w || y >= m_h) return;
        const size_t idx = ((size_t)y * (size_t)m_w + (size_t)x) * (size_t)m_c;
        r = m_data[idx + 0];
        g = (m_c > 1) ? m_data[idx + 1] : r;
        b = (m_c > 2) ? m_data[idx + 2] : r;
        a = (m_c > 3) ? m_data[idx + 3] : 1.0f;
    }

    void Image::forceOpaqueAlpha() {
        if (m_c < 4) return;
        const size_t N = (size_t)m_w * (size_t)m_h;
        for (size_t i = 0; i < N; ++i) m_data[i * 4 + 3] = 1.0f;
    }

    void Image::alphaOverInPlace(const Image& src) {
        if (m_c < 4 || src.m_c < 4) return;
        if (m_w != src.m_w || m_h != src.m_h) return;

        const size_t N = (size_t)m_w * (size_t)m_h;
        for (size_t i = 0; i < N; ++i) {
            const float sr = src.m_data[i * 4 + 0];
            const float sg = src.m_data[i * 4 + 1];
            const float sb = src.m_data[i * 4 + 2];
            const float sa = clamp01f(src.m_data[i * 4 + 3]);

            const float dr = m_data[i * 4 + 0];
            const float dg = m_data[i * 4 + 1];
            const float db = m_data[i * 4 + 2];
            const float da = clamp01f(m_data[i * 4 + 3]);

            const float outA = sa + da * (1.0f - sa);
            const float outRp = sr * sa + dr * da * (1.0f - sa);
            const float outGp = sg * sa + dg * da * (1.0f - sa);
            const float outBp = sb * sa + db * da * (1.0f - sa);

            float outR = 0.0f, outG = 0.0f, outB = 0.0f;
            if (outA > 1e-8f) {
                outR = outRp / outA;
                outG = outGp / outA;
                outB = outBp / outA;
            }

            m_data[i * 4 + 0] = outR;
            m_data[i * 4 + 1] = outG;
            m_data[i * 4 + 2] = outB;
            m_data[i * 4 + 3] = outA;
        }
    }

    void Image::drawRect(int x0, int y0, int w, int h, float r, float g, float b, float a) {
        if (w <= 0 || h <= 0) return;
        const int x1 = x0 + w;
        const int y1 = y0 + h;
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                setPixel(x, y, r, g, b, a);
    }

    void Image::drawText5x7(int x, int y, const std::string& text,
        int scale, float r, float g, float b, float a,
        bool drawBg) {
        if (scale < 1) scale = 1;

        const int glyphW = 5;
        const int glyphH = 7;
        const int pad = scale;
        const int spacing = scale;

        int textW = 0;
        for (char c : text) (void)c, textW += glyphW * scale + spacing;
        if (!text.empty()) textW -= spacing;
        const int textH = glyphH * scale;

        if (drawBg) {
            const float bgA = std::min(1.0f, a * 0.65f);
            drawRect(x - pad, y - pad, textW + 2 * pad, textH + 2 * pad, 0.0f, 0.0f, 0.0f, bgA);
        }

        int penX = x;
        for (char c : text) {
            const uint8_t* rows = glyph5x7(c);
            for (int row = 0; row < glyphH; ++row) {
                uint8_t bits = rows[row];
                for (int col = 0; col < glyphW; ++col) {
                    bool on = (bits & (1u << (glyphW - 1 - col))) != 0;
                    if (!on) continue;
                    const int px0 = penX + col * scale;
                    const int py0 = y + row * scale;
                    drawRect(px0, py0, scale, scale, r, g, b, a);
                }
            }
            penX += glyphW * scale + spacing;
        }
    }

} // namespace core
