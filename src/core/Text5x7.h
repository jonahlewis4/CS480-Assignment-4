#pragma once
#include <string>

namespace core {
class Image;

void drawChar5x7(Image& img, int x, int y, char c,
                 float r, float g, float b, float a,
                 int scale = 2, bool outline = true);

void drawText5x7(Image& img, int x, int y, const std::string& text,
                 float r, float g, float b, float a,
                 int scale = 2, int spacing = 1, bool outline = true);
}
