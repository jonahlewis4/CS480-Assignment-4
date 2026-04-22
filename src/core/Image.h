#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace core {

    class Image {
    public:
        Image() = default;
        Image(int w, int h, int c = 4);

        int width()  const { return m_w; }
        int height() const { return m_h; }
        int channels() const { return m_c; }

        std::vector<float>& data() { return m_data; }
        const std::vector<float>& data() const { return m_data; }

        void clear(float r, float g, float b, float a = 1.0f);
        void fillCheckerboard(int cell = 16);

        bool loadFromFile(const std::string& path);
        bool saveToPng(const std::string& path) const;

        void srgbToLinearInPlace();
        void linearToSrgbInPlace();

        std::vector<std::uint8_t> toRGBA8() const;

        void setPixel(int x, int y, float r, float g, float b, float a);
        void getPixel(int x, int y, float& r, float& g, float& b, float& a) const;

        // Helpers used by App.cpp
        void forceOpaqueAlpha();                 // alpha = 1 everywhere
        void alphaOverInPlace(const Image& src); // this = src OVER this

        // Simple stamping helpers
        void drawRect(int x0, int y0, int w, int h, float r, float g, float b, float a);
        void drawText5x7(int x, int y, const std::string& text,
            int scale, float r, float g, float b, float a,
            bool drawBg = true);

    private:
        int m_w = 0;
        int m_h = 0;
        int m_c = 4;
        std::vector<float> m_data; // size = w*h*c
    };

} // namespace core
