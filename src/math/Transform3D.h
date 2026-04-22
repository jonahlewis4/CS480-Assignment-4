#pragma once

#include <array>
#include <vector>

namespace a3 {

    struct Vec2 {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Vec3 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct Vec4 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 1.0f;
    };

    struct Mat4 {
        float m[4][4]{};
    };

    Mat4 makeIdentity();
    Mat4 makeTranslation(float tx, float ty, float tz);
    Mat4 makeScale(float sx, float sy, float sz);
    Mat4 makeRotationXDeg(float degrees);
    Mat4 makeRotationYDeg(float degrees);
    Mat4 makeRotationZDeg(float degrees);
    Mat4 multiply(const Mat4& A, const Mat4& B);
    Vec4 transform(const Mat4& M, const Vec4& p);

    Vec4 makePoint(float x, float y, float z);
    Vec4 makeVector(float x, float y, float z);

    Mat4 buildModelTransform(float tx, float ty, float tz,
        float rxDeg, float ryDeg, float rzDeg,
        float sx, float sy, float sz);

    Mat4 makeLookAt(const Vec3& eye, const Vec3& target, const Vec3& up);
    Mat4 makeOrtho(float left, float right,
        float bottom, float top,
        float zNear, float zFar);
    Mat4 makePerspective(float fovYDeg, float aspect,
        float zNear, float zFar);

    Vec3 perspectiveDivide(const Vec4& p);

    Vec3 add(const Vec3& a, const Vec3& b);
    Vec3 sub(const Vec3& a, const Vec3& b);
    Vec3 mul(const Vec3& v, float s);
    float dot(const Vec3& a, const Vec3& b);
    Vec3 cross(const Vec3& a, const Vec3& b);
    float length(const Vec3& v);
    Vec3 normalize(const Vec3& v);

    std::vector<Vec3> makeCubeVertices();
    std::vector<std::array<int, 2>> makeCubeEdges();

    std::vector<Vec3> makePyramidVertices();
    std::vector<std::array<int, 2>> makePyramidEdges();

} // namespace a3