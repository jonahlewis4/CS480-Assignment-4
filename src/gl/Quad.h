#pragma once
class Quad {
public:
    Quad();
    ~Quad();
    void draw() const;
private:
    unsigned m_vao = 0, m_vbo = 0, m_ebo = 0;
};
