#include "Text5x7.h"
#include "Image.h"
#include "Font5x7.h"

#include <cctype>

namespace core {

    static void putBlock(Image& img, int x, int y, int scale,
        float r, float g, float b, float a) {
        for (int dy = 0; dy < scale; ++dy)
            for (int dx = 0; dx < scale; ++dx)
                img.setPixel(x + dx, y + dy, r, g, b, a);
    }

    void drawChar5x7(Image& img, int x, int y, char c,
        float r, float g, float b, float a,
        int scale, bool outline) {
        // Force supported set: A–Z, 0–9, space, - _ .
        // If lowercase arrives, uppercase it.
        unsigned char uc = (unsigned char)c;
        if (std::islower(uc)) c = (char)std::toupper(uc);

        const Glyph5x7* glyph = findGlyph5x7(c);
        if (!glyph) glyph = findGlyph5x7(' ');

        // Optional outline for readability (black stroke behind)
        if (outline) {
            for (int oy = -1; oy <= 1; ++oy) {
                for (int ox = -1; ox <= 1; ++ox) {
                    if (ox == 0 && oy == 0) continue;
                    for (int row = 0; row < 7; ++row) {
                        std::uint8_t bits = glyph->row[row];
                        for (int col = 0; col < 5; ++col) {
                            if (bits & (1u << col)) {
                                putBlock(img,
                                    x + ox * scale + col * scale,
                                    y + oy * scale + row * scale,
                                    scale,
                                    0.0f, 0.0f, 0.0f, a);
                            }
                        }
                    }
                }
            }
        }

        // Main glyph
        for (int row = 0; row < 7; ++row) {
            std::uint8_t bits = glyph->row[row];
            for (int col = 0; col < 5; ++col) {
                if (bits & (1u << col)) {
                    putBlock(img,
                        x + col * scale,
                        y + row * scale,
                        scale,
                        r, g, b, a);
                }
            }
        }
    }

    void drawText5x7(Image& img, int x, int y, const std::string& text,
        float r, float g, float b, float a,
        int scale, int spacing, bool outline) {
        int cursorX = x;
        for (char c : text) {
            drawChar5x7(img, cursorX, y, c, r, g, b, a, scale, outline);
            cursorX += (5 * scale) + (spacing * scale);
        }
    }

} // namespace core
