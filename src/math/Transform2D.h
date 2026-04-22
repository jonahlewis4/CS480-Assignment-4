#pragma once

#include <array>
#include <vector>

namespace a2 {

    struct Vec2 {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Vec3 {
        float x = 0.0f;
        float y = 0.0f;
        float w = 1.0f;
    };

    struct Mat3 {
        // Row-major storage.
        float m[3][3]{};
    };

    Mat3 makeIdentity();
    Mat3 makeTranslation(float tx, float ty);
    Mat3 makeScale(float sx, float sy);
    Mat3 makeRotationDeg(float degrees);
    Mat3 makeShearX(float kx);
    Mat3 makeShearY(float ky);
    Mat3 multiply(const Mat3& A, const Mat3& B);
    Vec3 transform(const Mat3& M, const Vec3& p);

    Mat3 makeRotationAboutPivot(float degrees, float cx, float cy);
    Mat3 makeScaleAboutPivot(float sx, float sy, float cx, float cy);

    Mat3 buildStandardTransform(float tx, float ty,
        float rotationDeg,
        float sx, float sy,
        float shearX, float shearY);

    Mat3 buildLinearOnlyTransform(float rotationDeg,
        float sx, float sy,
        float shearX, float shearY);

    Vec3 makePoint(float x, float y);
    Vec3 makeVector(float x, float y);

    std::vector<Vec2> makeTriangle();
    std::vector<Vec2> makeSquare();
    std::vector<Vec2> makeHouse();
    std::vector<Vec2> makeCar();

    std::vector<Vec2> transformShape(const Mat3& M, const std::vector<Vec2>& pts);

    std::array<Vec2, 2> transformedBasisX(const Mat3& M);
    std::array<Vec2, 2> transformedBasisY(const Mat3& M);

} // namespace a2