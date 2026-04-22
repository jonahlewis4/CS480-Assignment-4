#include "Transform3D.h"

#include <cmath>

namespace a3 {

    namespace {
        constexpr float kPi = 3.14159265358979323846f;

        float degToRad(float degrees) {
            return degrees * kPi / 180.0f;
        }
    } // namespace

    Mat4 makeIdentity() {
        Mat4 I{};
        I.m[0][0] = 1.0f;
        I.m[1][1] = 1.0f;
        I.m[2][2] = 1.0f;
        I.m[3][3] = 1.0f;
        return I;
    }

    Mat4 makeTranslation(float tx, float ty, float tz) {
        Mat4 T = makeIdentity();
        T.m[0][3] = tx;
        T.m[1][3] = ty;
        T.m[2][3] = tz;
        return T;
    }

    Mat4 makeScale(float sx, float sy, float sz) {
        Mat4 S = makeIdentity();
        S.m[0][0] = sx;
        S.m[1][1] = sy;
        S.m[2][2] = sz;
        return S;
    }

    Mat4 makeRotationXDeg(float degrees) {
        const float r = degToRad(degrees);
        const float c = std::cos(r);
        const float s = std::sin(r);

        Mat4 R = makeIdentity();
        R.m[1][1] = c;
        R.m[1][2] = -s;
        R.m[2][1] = s;
        R.m[2][2] = c;
        return R;
    }

    Mat4 makeRotationYDeg(float degrees) {
        const float r = degToRad(degrees);
        const float c = std::cos(r);
        const float s = std::sin(r);

        Mat4 R = makeIdentity();
        R.m[0][0] = c;
        R.m[0][2] = s;
        R.m[2][0] = -s;
        R.m[2][2] = c;
        return R;
    }

    Mat4 makeRotationZDeg(float degrees) {
        const float r = degToRad(degrees);
        const float c = std::cos(r);
        const float s = std::sin(r);

        Mat4 R = makeIdentity();
        R.m[0][0] = c;
        R.m[0][1] = -s;
        R.m[1][0] = s;
        R.m[1][1] = c;
        return R;
    }

    Mat4 multiply(const Mat4& A, const Mat4& B) {
        Mat4 C{};
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                C.m[i][j] = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    C.m[i][j] += A.m[i][k] * B.m[k][j];
                }
            }
        }
        return C;
    }

    Vec4 transform(const Mat4& M, const Vec4& p) {
        Vec4 out{};
        out.x = M.m[0][0] * p.x + M.m[0][1] * p.y + M.m[0][2] * p.z + M.m[0][3] * p.w;
        out.y = M.m[1][0] * p.x + M.m[1][1] * p.y + M.m[1][2] * p.z + M.m[1][3] * p.w;
        out.z = M.m[2][0] * p.x + M.m[2][1] * p.y + M.m[2][2] * p.z + M.m[2][3] * p.w;
        out.w = M.m[3][0] * p.x + M.m[3][1] * p.y + M.m[3][2] * p.z + M.m[3][3] * p.w;
        return out;
    }

    Vec4 makePoint(float x, float y, float z) {
        return { x, y, z, 1.0f };
    }

    Vec4 makeVector(float x, float y, float z) {
        return { x, y, z, 0.0f };
    }

    Mat4 buildModelTransform(float tx, float ty, float tz,
        float rxDeg, float ryDeg, float rzDeg,
        float sx, float sy, float sz) {
        // M = T * Rz * Ry * Rx * S
        return multiply(makeTranslation(tx, ty, tz),
            multiply(makeRotationZDeg(rzDeg),
                multiply(makeRotationYDeg(ryDeg),
                    multiply(makeRotationXDeg(rxDeg),
                        makeScale(sx, sy, sz)))));
    }

    Vec3 add(const Vec3& a, const Vec3& b) {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    Vec3 sub(const Vec3& a, const Vec3& b) {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    Vec3 mul(const Vec3& v, float s) {
        return { v.x * s, v.y * s, v.z * s };
    }

    float dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    Vec3 cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    float length(const Vec3& v) {
        return std::sqrt(dot(v, v));
    }

    Vec3 normalize(const Vec3& v) {
        const float len = length(v);
        if (len <= 1e-7f) return { 0.0f, 0.0f, 0.0f };
        return { v.x / len, v.y / len, v.z / len };
    }

    Mat4 makeLookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        const Vec3 zaxis = normalize(sub(eye, target));
        const Vec3 xaxis = normalize(cross(up, zaxis));
        const Vec3 yaxis = cross(zaxis, xaxis);

        Mat4 V = makeIdentity();
        V.m[0][0] = xaxis.x; V.m[0][1] = xaxis.y; V.m[0][2] = xaxis.z; V.m[0][3] = -dot(xaxis, eye);
        V.m[1][0] = yaxis.x; V.m[1][1] = yaxis.y; V.m[1][2] = yaxis.z; V.m[1][3] = -dot(yaxis, eye);
        V.m[2][0] = zaxis.x; V.m[2][1] = zaxis.y; V.m[2][2] = zaxis.z; V.m[2][3] = -dot(zaxis, eye);
        return V;
    }

    Mat4 makeOrtho(float left, float right,
        float bottom, float top,
        float zNear, float zFar) {
        Mat4 P{};
        P.m[0][0] = 2.0f / (right - left);
        P.m[1][1] = 2.0f / (top - bottom);
        P.m[2][2] = -2.0f / (zFar - zNear);
        P.m[3][3] = 1.0f;

        P.m[0][3] = -(right + left) / (right - left);
        P.m[1][3] = -(top + bottom) / (top - bottom);
        P.m[2][3] = -(zFar + zNear) / (zFar - zNear);
        return P;
    }

    Mat4 makePerspective(float fovYDeg, float aspect,
        float zNear, float zFar) {
        const float f = 1.0f / std::tan(degToRad(fovYDeg) * 0.5f);

        Mat4 P{};
        P.m[0][0] = f / aspect;
        P.m[1][1] = f;
        P.m[2][2] = (zFar + zNear) / (zNear - zFar);
        P.m[2][3] = (2.0f * zFar * zNear) / (zNear - zFar);
        P.m[3][2] = -1.0f;
        return P;
    }

    Vec3 perspectiveDivide(const Vec4& p) {
        if (std::fabs(p.w) <= 1e-7f) {
            return { p.x, p.y, p.z };
        }
        return { p.x / p.w, p.y / p.w, p.z / p.w };
    }

    std::vector<Vec3> makeCubeVertices() {
        return {
            {-1.f, -1.f, -1.f}, { 1.f, -1.f, -1.f},
            { 1.f,  1.f, -1.f}, {-1.f,  1.f, -1.f},
            {-1.f, -1.f,  1.f}, { 1.f, -1.f,  1.f},
            { 1.f,  1.f,  1.f}, {-1.f,  1.f,  1.f}
        };
    }

    std::vector<std::array<int, 2>> makeCubeEdges() {
        return {
            {0,1},{1,2},{2,3},{3,0},
            {4,5},{5,6},{6,7},{7,4},
            {0,4},{1,5},{2,6},{3,7}
        };
    }

    std::vector<Vec3> makePyramidVertices() {
        return {
            {-1.f, -1.f, -1.f},
            { 1.f, -1.f, -1.f},
            { 1.f, -1.f,  1.f},
            {-1.f, -1.f,  1.f},
            { 0.f,  1.f,  0.f}
        };
    }

    std::vector<std::array<int, 2>> makePyramidEdges() {
        return {
            {0,1},{1,2},{2,3},{3,0},
            {0,4},{1,4},{2,4},{3,4}
        };
    }

} // namespace a3