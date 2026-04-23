#include "pipeline/Assignment4Renderer.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string readTextFile(const fs::path& path) {
    std::ifstream file(path, std::ios::in);
    if (!file) return {};
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

const char* kFallbackVertexShader = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vColor;
out float vViewDepth;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    // TODO (Assignment 4 - Vertex Processing):
    // Apply the real graphics pipeline transform here:
    //   clipPosition = uProjection * uView * uModel * vec4(aPos, 1.0)
    // Then write the result to gl_Position.
    // Also compute a depth-related value from view space for later use.

    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
    vViewDepth = 1.0;
}
)GLSL";

const char* kFallbackFragmentShader = R"GLSL(
#version 330 core
in vec3 vColor;
in float vViewDepth;
out vec4 FragColor;

uniform bool uDepthView;
uniform float uNearPlane;
uniform float uFarPlane;

void main()
{
    // TODO (Assignment 4 - Fragment Processing):
    // Use the interpolated vertex color for the final fragment color.
    // Keep the depth-view branch disabled for the student version.
    FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
)GLSL";

bool compileShader(GLuint shader, const std::string& source, std::string& errorMessage) {
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_TRUE) return true;

    GLint logLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
    std::string log(static_cast<size_t>(logLen > 0 ? logLen : 1), '\0');
    glGetShaderInfoLog(shader, logLen, nullptr, log.data());
    errorMessage = log;
    return false;
}

std::pair<std::string, std::string> findShaderSources(const std::string& assetRoot) {
    const std::vector<fs::path> roots = {
        fs::path(assetRoot),
        fs::path(".") / assetRoot,
        fs::path("..") / assetRoot,
        fs::path("..") / ".." / assetRoot,
        fs::path("..") / ".." / ".." / assetRoot
    };

    for (const auto& base : roots) {
        const fs::path vert = base / "shaders" / "a4_pipeline.vert";
        const fs::path frag = base / "shaders" / "a4_pipeline.frag";
        if (fs::exists(vert) && fs::exists(frag)) {
            const std::string vs = readTextFile(vert);
            const std::string fsSrc = readTextFile(frag);
            if (!vs.empty() && !fsSrc.empty()) {
                return {vs, fsSrc};
            }
        }
    }

    return {std::string(kFallbackVertexShader), std::string(kFallbackFragmentShader)};
}

std::vector<Assignment4Renderer::Vertex> makeCubeVertices() {
    return {
        {-0.8f, -0.8f,  0.8f, 1.0f, 0.2f, 0.2f},
        { 0.8f, -0.8f,  0.8f, 0.2f, 1.0f, 0.2f},
        { 0.8f,  0.8f,  0.8f, 0.2f, 0.2f, 1.0f},
        {-0.8f,  0.8f,  0.8f, 1.0f, 1.0f, 0.2f},
        {-0.8f, -0.8f, -0.8f, 1.0f, 0.2f, 1.0f},
        { 0.8f, -0.8f, -0.8f, 0.2f, 1.0f, 1.0f},
        { 0.8f,  0.8f, -0.8f, 0.9f, 0.5f, 0.1f},
        {-0.8f,  0.8f, -0.8f, 0.6f, 0.3f, 0.9f}
    };
}

std::vector<unsigned int> makeCubeIndices() {
    return {
        0, 1, 2,  2, 3, 0,
        1, 5, 6,  6, 2, 1,
        5, 4, 7,  7, 6, 5,
        4, 0, 3,  3, 7, 4,
        3, 2, 6,  6, 7, 3,
        4, 5, 1,  1, 0, 4
    };
}

std::vector<Assignment4Renderer::Vertex> makePyramidVertices() {
    return {
        {-0.9f, 0.0f, -0.9f, 0.9f, 0.2f, 0.2f},
        { 0.9f, 0.0f, -0.9f, 0.2f, 0.9f, 0.2f},
        { 0.9f, 0.0f,  0.9f, 0.2f, 0.4f, 1.0f},
        {-0.9f, 0.0f,  0.9f, 1.0f, 0.8f, 0.2f},
        { 0.0f, 1.6f,  0.0f, 1.0f, 0.5f, 0.8f}
    };
}

std::vector<unsigned int> makePyramidIndices() {
    return {
        0, 1, 2,  2, 3, 0,
        0, 1, 4,
        1, 2, 4,
        2, 3, 4,
        3, 0, 4
    };
}

std::vector<Assignment4Renderer::Vertex> makeGroundVertices() {
    return {
        {-5.0f, 0.0f, -5.0f, 0.35f, 0.35f, 0.38f},
        { 5.0f, 0.0f, -5.0f, 0.35f, 0.35f, 0.38f},
        { 5.0f, 0.0f,  5.0f, 0.45f, 0.45f, 0.48f},
        {-5.0f, 0.0f,  5.0f, 0.45f, 0.45f, 0.48f}
    };
}

std::vector<unsigned int> makeGroundIndices() {
    return {0, 1, 2, 2, 3, 0};
}

} // namespace

bool Assignment4Renderer::initialize(const std::string& assetRoot, std::string& errorMessage) {
    if (!loadShaders(assetRoot, errorMessage)) return false;

    if (!buildScene()) {
        errorMessage = "Failed to upload Assignment 4 scene meshes.";
        shutdown();
        return false;
    }

    return true;
}

void Assignment4Renderer::shutdown() {
    destroyMesh(m_cube.mesh);
    destroyMesh(m_pyramid.mesh);
    destroyMesh(m_ground.mesh);
    destroyProgram();
}

bool Assignment4Renderer::loadShaders(const std::string& assetRoot, std::string& errorMessage) {
    destroyProgram();

    auto [vertexSource, fragmentSource] = findShaderSources(assetRoot);
    if (vertexSource.empty() || fragmentSource.empty()) {
        errorMessage = "Could not load Assignment 4 shaders.";
        return false;
    }

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fsShader = glCreateShader(GL_FRAGMENT_SHADER);

    if (!compileShader(vs, vertexSource, errorMessage) ||
        !compileShader(fsShader, fragmentSource, errorMessage)) {
        glDeleteShader(vs);
        glDeleteShader(fsShader);
        return false;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fsShader);
    glLinkProgram(m_program);

    glDeleteShader(vs);
    glDeleteShader(fsShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint logLen = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<size_t>(logLen > 0 ? logLen : 1), '\0');
        glGetProgramInfoLog(m_program, logLen, nullptr, log.data());
        errorMessage = log;
        destroyProgram();
        return false;
    }

    m_uModelLoc = glGetUniformLocation(m_program, "uModel");
    m_uViewLoc = glGetUniformLocation(m_program, "uView");
    m_uProjLoc = glGetUniformLocation(m_program, "uProjection");
    m_uDepthViewLoc = glGetUniformLocation(m_program, "uDepthView");
    m_uNearLoc = glGetUniformLocation(m_program, "uNearPlane");
    m_uFarLoc = glGetUniformLocation(m_program, "uFarPlane");

    return true;
}

bool Assignment4Renderer::buildScene() {
    m_cube.modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-1.2f, 0.95f, 0.2f));
    m_pyramid.modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.15f, 0.0f, -0.35f));
    m_ground.modelMatrix = glm::mat4(1.0f);

    return uploadMesh(m_cube.mesh, makeCubeVertices(), makeCubeIndices()) &&
           uploadMesh(m_pyramid.mesh, makePyramidVertices(), makePyramidIndices()) &&
           uploadMesh(m_ground.mesh, makeGroundVertices(), makeGroundIndices());
}

bool Assignment4Renderer::uploadMesh(Mesh& mesh,
                                     const std::vector<Vertex>& vertices,
                                     const std::vector<unsigned int>& indices) {
    destroyMesh(mesh);
    if (vertices.empty() || indices.empty()) return false;

    // TODO (Assignment 4 - CPU to GPU data upload):
    // 1. Create and bind the VAO, VBO, and EBO.
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    // 2. Copy vertex data into GL_ARRAY_BUFFER.
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size()) * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    // 3. Copy index data into GL_ELEMENT_ARRAY_BUFFER.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size()) * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    // 4. Describe attribute 0 as position (x, y, z).
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizeiptr>(sizeof(Vertex)), nullptr);
    // 5. Describe attribute 1 as color (r, g, b).
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizeiptr>(sizeof(Vertex)), reinterpret_cast<void *>(3 * sizeof(float)));
    // 6. Enable both vertex attributes.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    // Expected layout of one Vertex:
    //   px py pz r g b
    //
    // The instructor version uses glGenVertexArrays, glGenBuffers,
    // glBindVertexArray, glBindBuffer, glBufferData,
    // glVertexAttribPointer, and glEnableVertexAttribArray.

    mesh.indexCount = static_cast<GLsizei>(indices.size());
    return true;
}

void Assignment4Renderer::destroyMesh(Mesh& mesh) {
    if (mesh.ebo) glDeleteBuffers(1, &mesh.ebo);
    if (mesh.vbo) glDeleteBuffers(1, &mesh.vbo);
    if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
    mesh = Mesh{};
}

void Assignment4Renderer::destroyProgram() {
    if (m_program) {
        glDeleteProgram(m_program);
    }
    m_program = 0;
}

void Assignment4Renderer::setRenderModeState(RenderMode mode) const {
    // TODO (Assignment 4 - primitive/rasterization/output state):
    // Implement the four rendering modes from class:
    //   1 Points
    //   2 Primitive / Triangle View
    //   3 Filled (No Depth)
    //   4 Filled (Depth Test)
    //

    switch (mode) {
        case RenderMode::Points: {
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            glPointSize(5);
            break;
        }
        case RenderMode::PrimitiveTriangle: {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        }
        case RenderMode::FilledNoDepth: {
            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        }
        case RenderMode::FilledDepth: {
            glEnable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDepthFunc(GL_LESS);
        }
    }

    // Suggested OpenGL functions:
    //   glPolygonMode(...)
    //   glEnable(GL_DEPTH_TEST)
    //   glDisable(GL_DEPTH_TEST)
    //   glDepthFunc(GL_LESS)
    //   glPointSize(...)
}

void Assignment4Renderer::drawObject(const SceneObject& object) const {
    glUniformMatrix4fv(m_uModelLoc, 1, GL_FALSE, glm::value_ptr(object.modelMatrix));
    glBindVertexArray(object.mesh.vao);
    glDrawElements(GL_TRIANGLES, object.mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Assignment4Renderer::render(int framebufferWidth,
                                 int framebufferHeight,
                                 RenderMode mode,
                                 bool rotationEnabled,
                                 float timeSeconds) {
    if (!m_program || framebufferWidth <= 0 || framebufferHeight <= 0) return;

    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, framebufferWidth, framebufferHeight);

    glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setRenderModeState(mode);
    glUseProgram(m_program);

    const float aspect = static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);
    const float nearPlane = 0.1f;
    const float farPlane = 40.0f;

    const glm::mat4 view = glm::lookAt(glm::vec3(5.2f, 4.0f, 7.3f),
                                       glm::vec3(0.0f, 0.7f, 0.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));

    const glm::mat4 projection =
        glm::perspective(glm::radians(50.0f), aspect, nearPlane, farPlane);

    glm::mat4 cubeModel =
        glm::translate(glm::mat4(1.0f), glm::vec3(-1.2f, 0.95f, 0.2f));
    glm::mat4 pyramidModel =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.15f, 0.0f, -0.35f));

    if (rotationEnabled) {
        cubeModel = glm::rotate(cubeModel, timeSeconds * 0.9f, glm::vec3(0.2f, 1.0f, 0.1f));
        pyramidModel = glm::rotate(pyramidModel, -timeSeconds * 0.7f, glm::vec3(0.0f, 1.0f, 0.0f));
    } else {
        cubeModel = glm::rotate(cubeModel, glm::radians(28.0f), glm::vec3(0.2f, 1.0f, 0.1f));
        pyramidModel = glm::rotate(pyramidModel, glm::radians(-22.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glUniformMatrix4fv(m_uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(m_uProjLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(m_uDepthViewLoc, 0);
    glUniform1f(m_uNearLoc, nearPlane);
    glUniform1f(m_uFarLoc, farPlane);

    m_ground.modelMatrix = glm::mat4(1.0f);
    m_cube.modelMatrix = cubeModel;
    m_pyramid.modelMatrix = pyramidModel;

    drawObject(m_ground);
    drawObject(m_cube);
    drawObject(m_pyramid);

    glUseProgram(0);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
