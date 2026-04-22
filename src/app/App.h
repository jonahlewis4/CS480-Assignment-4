#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "core/Image.h"
#include "math/Transform2D.h"
#include "math/Transform3D.h"
#include "pipeline/Assignment4Renderer.h"

struct GLFWwindow;

class App {
public:
    App() = default;
    ~App() noexcept = default;

    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

    int run(int argc, char** argv);

    enum class Assignment2ObjectMode {
        Point = 0,
        Vector = 1,
        Shape = 2,
    };

    enum class Assignment2ShapeMode {
        Triangle = 0,
        Square = 1,
        House = 2,
        Car = 3,
    };

    enum class Assignment3ObjectMode {
        Cube = 0,
        Pyramid = 1,
    };

    enum class Assignment3ProjectionMode {
        Orthographic = 0,
        Perspective = 1,
    };

private:

    enum class CurveMode {
        Bezier = 0,
        Hermite = 1,
        PiecewiseBezier = 2,
        BSpline = 3,
    };

    enum class BezierMode {
        Linear = 0,
        Quadratic = 1,
        Cubic = 2,
        General = 3,
    };

    enum class ToolMode {
        Launcher = 0,
        Assignment1 = 1,
        Assignment2 = 2,
        Assignment3 = 3,
        Assignment4 = 4,
        Assignment5 = 5,
        Assignment7 = 6,
    };

    bool initWindow();
    void initImGui();
    void shutdown();

    void drawUI();
    void drawLauncherUI();
    void drawAssignment1UI();
    void drawAssignment2UI();
    void drawAssignment3UI();
    void drawAssignment4UI();
    void drawAssignment7UI();
    void drawImagePanel();

    void loadImage(const std::string& path, bool asForeground);
    void saveImage(const std::string& path);
    void rebuildOutputTexture();

    std::string openImageDialog();
    std::string savePngDialog();
    void setStatus(const std::string& msg);

private:
    GLFWwindow* m_window = nullptr;
    int m_winW = 1600;
    int m_winH = 900;

    unsigned m_outTex = 0;
    int m_outTexW = 0;
    int m_outTexH = 0;

    ToolMode m_mode = ToolMode::Launcher;

    std::string m_bgPath;
    std::string m_fgPath;
    std::string m_savePath;

    // Assignment 1 state
    bool  m_doLinearWorkflow = true;
    bool  m_showChecker = true;
    bool  m_applyBrightness = false;
    float m_brightness = 0.0f;
    float m_fgOpacity = 1.0f;
    int   m_blendMode = 0;
    bool  m_saveWithAlpha = false;
    std::string m_status;
    std::string m_studentName;
    bool  m_stampName = true;
    std::unique_ptr<core::Image> m_bg;
    std::unique_ptr<core::Image> m_fg;
    std::unique_ptr<core::Image> m_out;

    // Assignment 2 state
    Assignment2ObjectMode m_a2ObjectMode = Assignment2ObjectMode::Shape;
    Assignment2ShapeMode  m_a2ShapeMode = Assignment2ShapeMode::Triangle;

    // Assignment 2 input point/vector
    float m_a2InputX = 0.6f;
    float m_a2InputY = 0.2f;

    // Assignment 2 transform parameters
    float m_tx = 0.0f;
    float m_ty = 0.0f;
    float m_sx = 1.0f;
    float m_sy = 1.0f;
    float m_thetaDeg = 0.0f;
    float m_shearX = 0.0f;
    float m_shearY = 0.0f;

    // Assignment 2 pivot controls
    bool  m_usePivotRotation = false;
    bool  m_usePivotScale = false;
    float m_pivotX = 0.0f;
    float m_pivotY = 0.0f;

    // Assignment 2 visualization toggles
    bool  m_showOriginal = true;
    bool  m_showTransformed = true;
    bool  m_showBasis = true;
    bool  m_showGrid = true;
    bool  m_showStepStages = true;

    // Assignment 3 state
    Assignment3ObjectMode m_a3ObjectMode = Assignment3ObjectMode::Cube;
    Assignment3ProjectionMode m_a3ProjectionMode = Assignment3ProjectionMode::Perspective;

    // Assignment 3 model transform
    float m_a3Tx = 0.0f;
    float m_a3Ty = 0.0f;
    float m_a3Tz = 0.0f;

    float m_a3RxDeg = 0.0f;
    float m_a3RyDeg = 0.0f;
    float m_a3RzDeg = 0.0f;

    float m_a3Sx = 1.0f;
    float m_a3Sy = 1.0f;
    float m_a3Sz = 1.0f;

    // Assignment 3 camera
    float m_a3EyeX = 4.0f;
    float m_a3EyeY = 3.0f;
    float m_a3EyeZ = 6.0f;

    float m_a3TargetX = 0.0f;
    float m_a3TargetY = 0.0f;
    float m_a3TargetZ = 0.0f;

    // Assignment 3 projection
    float m_a3FovYDeg = 60.0f;
    float m_a3Near = 0.1f;
    float m_a3Far = 50.0f;
    float m_a3OrthoScale = 4.0f;

    // Assignment 3 visualization toggles
    bool  m_a3ShowAxes = true;
    bool  m_a3ShowOriginal = true;
    bool  m_a3ShowTransformed = true;
    bool  m_a3ShowPipeline = true;

    // Assignment 4 state
    Assignment4Renderer m_a4Renderer;
    bool  m_a4Initialized = false;
    Assignment4Renderer::RenderMode m_a4RenderMode = Assignment4Renderer::RenderMode::FilledDepth;
    bool  m_a4Rotate = true;
    bool  m_a4ShowHelp = true;

    // Assignment 7 state - curve lab
    CurveMode m_a7CurveMode = CurveMode::Bezier;
    BezierMode m_a7BezierMode = BezierMode::Cubic;
    int   m_a7GeneralPointCount = 6;
    std::array<a2::Vec2, 10> m_a7ControlPoints = {
        a2::Vec2{-6.0f, -2.0f},
        a2::Vec2{-4.0f,  2.5f},
        a2::Vec2{-2.0f,  3.5f},
        a2::Vec2{ 0.0f, -1.0f},
        a2::Vec2{ 2.0f, -3.5f},
        a2::Vec2{ 4.0f,  2.5f},
        a2::Vec2{ 6.0f,  2.0f},
        a2::Vec2{ 6.5f, -1.0f},
        a2::Vec2{ 7.0f,  1.0f},
        a2::Vec2{ 7.5f,  0.0f}
    };
    int   m_a7Samples = 64;
    float m_a7ParamT = 0.35f;
    float m_a7AnimationSpeed = 0.25f;
    bool  m_a7Animate = true;
    bool  m_a7LoopAnimation = true;
    bool  m_a7ShowCurve = true;
    bool  m_a7ShowControlPolygon = true;
    bool  m_a7ShowControlPoints = true;
    bool  m_a7ShowConstruction = true;
    bool  m_a7ShowTangent = true;
    bool  m_a7ShowPointLabels = true;
    bool  m_a7ShowSamplePoints = false;
    bool  m_a7ShowGrid = true;
    bool  m_a7ShowConvexHull = false;
    bool  m_a7ShowSubdivision = false;
    bool  m_a7ShowLowResCurve = false;
    bool  m_a7ShowBasisWeights = false;
    bool  m_a7ShowStageHighlight = true;
    bool  m_a7ShowModeSummary = true;
    bool  m_a7ShowEquationPanel = true;
    float m_a7StepSize = 0.1f;
    bool  m_a7PiecewiseEnforceC1 = true;
    bool  m_a7StepByStep = false;
    int   m_a7ConstructionLevel = 3;
    int   m_a7LowResSamples = 12;
    bool  m_a7UseApproxArcLength = false;
    int   m_a7Preset = 0;
    int   m_a7DraggedPoint = -1;
};
