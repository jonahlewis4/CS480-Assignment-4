#pragma once
#include <vector>
#include <cstdint>

class Texture2D {
public:
    Texture2D() = default;
    ~Texture2D();

    void uploadRGBA8(int w, int h, const std::vector<std::uint8_t>& rgba);
    unsigned id() const { return m_id; }
    int width() const { return m_w; }
    int height() const { return m_h; }

private:
    unsigned m_id = 0;
    int m_w = 0, m_h = 0;
};
