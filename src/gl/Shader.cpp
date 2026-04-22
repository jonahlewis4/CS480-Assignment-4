#include "Shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <cstdio>

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static unsigned compile(GLenum type, const std::string& src) {
    unsigned s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);
    int ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(s, 4096, nullptr, log);
        std::fprintf(stderr, "Shader compile error: %s\n", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

Shader::~Shader() {
    if (m_id) glDeleteProgram(m_id);
}

bool Shader::load(const std::string& vsPath, const std::string& fsPath) {
    std::string vs = readFile(vsPath);
    std::string fs = readFile(fsPath);
    unsigned v = compile(GL_VERTEX_SHADER, vs);
    unsigned f = compile(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) return false;

    unsigned p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    glDeleteShader(v);
    glDeleteShader(f);

    int ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(p, 4096, nullptr, log);
        std::fprintf(stderr, "Program link error: %s\n", log);
        glDeleteProgram(p);
        return false;
    }

    if (m_id) glDeleteProgram(m_id);
    m_id = p;
    return true;
}

void Shader::bind() const {
    glUseProgram(m_id);
}
