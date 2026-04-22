#include "Transform2D.h"

#include <array>
#include <cmath>
#include <vector>

namespace a2 {

    namespace {
        constexpr float kPi = 3.14159265358979323846f;

        float degToRad(float degrees) {
            return degrees * kPi / 180.0f;
        }
    } // namespace

    Mat3 makeIdentity() {
        Mat3 I{};
        I.m[0][0] = 1.0f; I.m[0][1] = 0.0f; I.m[0][2] = 0.0f;
        I.m[1][0] = 0.0f; I.m[1][1] = 1.0f; I.m[1][2] = 0.0f;
        I.m[2][0] = 0.0f; I.m[2][1] = 0.0f; I.m[2][2] = 1.0f;
        return I;
    }

    Mat3 makeTranslation(float tx, float ty) {
        Mat3 T = makeIdentity();
        T.m[0][2] = tx;
        T.m[1][2] = ty;
        return T;
    }

    Mat3 makeScale(float sx, float sy) {
        Mat3 S = makeIdentity();
        S.m[0][0] = sx;
        S.m[1][1] = sy;
        return S;
    }

    Mat3 makeRotationDeg(float degrees) {
        const float r = degToRad(degrees);
        const float c = std::cos(r);
        const float s = std::sin(r);

        Mat3 R = makeIdentity();
        R.m[0][0] = c;
        R.m[0][1] = -s;
        R.m[1][0] = s;
        R.m[1][1] = c;
        return R;
    }

    Mat3 makeShearX(float kx) {
        Mat3 H = makeIdentity();
        H.m[0][1] = kx;
        return H;
    }

    Mat3 makeShearY(float ky) {
        Mat3 H = makeIdentity();
        H.m[1][0] = ky;
        return H;
    }

    Mat3 multiply(const Mat3& A, const Mat3& B) {
        Mat3 C{};
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                C.m[i][j] = 0.0f;
                for (int k = 0; k < 3; ++k) {
                    C.m[i][j] += A.m[i][k] * B.m[k][j];
                }
            }
        }
        return C;
    }

    Vec3 transform(const Mat3& M, const Vec3& p) {
        Vec3 out{};
        out.x = M.m[0][0] * p.x + M.m[0][1] * p.y + M.m[0][2] * p.w;
        out.y = M.m[1][0] * p.x + M.m[1][1] * p.y + M.m[1][2] * p.w;
        out.w = M.m[2][0] * p.x + M.m[2][1] * p.y + M.m[2][2] * p.w;
        return out;
    }

    Mat3 makeRotationAboutPivot(float degrees, float cx, float cy) {
        return multiply(makeTranslation(cx, cy),
            multiply(makeRotationDeg(degrees),
                makeTranslation(-cx, -cy)));
    }

    Mat3 makeScaleAboutPivot(float sx, float sy, float cx, float cy) {
        return multiply(makeTranslation(cx, cy),
            multiply(makeScale(sx, sy),
                makeTranslation(-cx, -cy)));
    }

    Mat3 buildStandardTransform(float tx, float ty,
        float rotationDeg,
        float sx, float sy,
        float shearX, float shearY) {
        // M = T * R * ShY * ShX * S
        return multiply(makeTranslation(tx, ty),
            multiply(makeRotationDeg(rotationDeg),
                multiply(makeShearY(shearY),
                    multiply(makeShearX(shearX),
                        makeScale(sx, sy)))));
    }

    Mat3 buildLinearOnlyTransform(float rotationDeg,
        float sx, float sy,
        float shearX, float shearY) {
        // No translation.
        return multiply(makeRotationDeg(rotationDeg),
            multiply(makeShearY(shearY),
                multiply(makeShearX(shearX),
                    makeScale(sx, sy))));
    }

    Vec3 makePoint(float x, float y) {
        return { x, y, 1.0f };
    }

    Vec3 makeVector(float x, float y) {
        return { x, y, 0.0f };
    }

    std::vector<Vec2> makeTriangle() {
        return {
     {-2.0f, -1.4f},
     { 2.0f, -1.4f},
     { 0.0f,  2.2f}
        };
    }

    std::vector<Vec2> makeSquare() {
        return {
     {-2.0f, -2.0f},
     { 2.0f, -2.0f},
     { 2.0f,  2.0f},
     {-2.0f,  2.0f}
        };
    }

    std::vector<Vec2> makeHouse() {
        return {
    {-2.0f, -2.0f},
    { 2.0f, -2.0f},
    { 2.0f,  0.4f},
    { 0.0f,  2.4f},
    {-2.0f,  0.4f}
        };
    }

    std::vector<Vec2> makeCar() {
        return {
    {-3.0f, -1.0f},
    {-2.2f,  0.4f},
    {-0.8f,  0.4f},
    {-0.2f,  1.2f},
    { 1.4f,  1.2f},
    { 2.2f,  0.4f},
    { 3.2f,  0.4f},
    { 3.2f, -1.0f},
    {-3.0f, -1.0f}
        };
    }

    std::vector<Vec2> transformShape(const Mat3& M, const std::vector<Vec2>& pts) {
        std::vector<Vec2> out;
        out.reserve(pts.size());

        for (const auto& p : pts) {
            const Vec3 hp = makePoint(p.x, p.y);
            const Vec3 tp = transform(M, hp);
            out.push_back({ tp.x, tp.y });
        }

        return out;
    }

    std::array<Vec2, 2> transformedBasisX(const Mat3& M) {
        const Vec3 origin = transform(M, makeVector(0.0f, 0.0f));
        const Vec3 ex = transform(M, makeVector(3.0f, 0.0f));
        return { {{origin.x, origin.y}, {ex.x, ex.y}} };
    }

    std::array<Vec2, 2> transformedBasisY(const Mat3& M) {
        const Vec3 origin = transform(M, makeVector(0.0f, 0.0f));
        const Vec3 ey = transform(M, makeVector(0.0f, 3.0f));
        return { {{origin.x, origin.y}, {ey.x, ey.y}} };
    }

} // namespace a2