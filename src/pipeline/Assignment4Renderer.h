#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

class Assignment4Renderer {
public:
    enum class RenderMode {
        Points = 1,
        PrimitiveTriangle = 2,
        FilledNoDepth = 3,
        FilledDepth = 4,
  
    };

    struct Vertex {
        float px, py, pz;
        float r, g, b;
    };

    bool initialize(const std::string& assetRoot, std::string& errorMessage);
    void shutdown();
    void render(int framebufferWidth,
                int framebufferHeight,
                RenderMode mode,
                bool rotationEnabled,
                float timeSeconds);

private:
    struct Mesh {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        GLsizei indexCount = 0;
    };

    struct SceneObject {
        Mesh mesh;
        glm::mat4 modelMatrix{1.0f};
    };

    GLuint m_program = 0;
    GLint  m_uModelLoc = -1;
    GLint  m_uViewLoc = -1;
    GLint  m_uProjLoc = -1;
    GLint  m_uDepthViewLoc = -1;
    GLint  m_uNearLoc = -1;
    GLint  m_uFarLoc = -1;

    SceneObject m_cube;
    SceneObject m_pyramid;
    SceneObject m_ground;

    bool loadShaders(const std::string& assetRoot, std::string& errorMessage);
    bool buildScene();
    bool uploadMesh(Mesh& mesh, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    void destroyMesh(Mesh& mesh);
    void drawObject(const SceneObject& object) const;
    void setRenderModeState(RenderMode mode) const;
    void destroyProgram();
};
