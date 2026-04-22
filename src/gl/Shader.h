#pragma once
#include <string>

class Shader {
public:
    Shader() = default;
    ~Shader();

    bool load(const std::string& vsPath, const std::string& fsPath);
    void bind() const;

    unsigned id() const { return m_id; }

private:
    unsigned m_id = 0;
};
