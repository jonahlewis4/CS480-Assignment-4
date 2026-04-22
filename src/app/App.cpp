#include "app/App.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "third_party/tinyfiledialogs.h"

#include "ops/Brightness.h"
#include "ops/Composite.h"
#include "ops/BlendModes.h"
#include "pipeline/Assignment4Renderer.h"
using core::Image;

static void glfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static bool endsWithPng(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    return s.size() >= 4 && s.substr(s.size() - 4) == ".png";
}

namespace {

    ImVec2 worldToScreen(const a2::Vec2& p, const ImVec2& origin, float scale) {
        return ImVec2(origin.x + p.x * scale, origin.y - p.y * scale);
    }

    void drawPolyline(ImDrawList* dl,
        const std::vector<a2::Vec2>& pts,
        const ImVec2& origin,
        float scale,
        ImU32 color,
        bool closed = true,
        float thickness = 2.0f) {
        if (pts.size() < 2) return;

        for (size_t i = 0; i + 1 < pts.size(); ++i) {
            dl->AddLine(worldToScreen(pts[i], origin, scale),
                worldToScreen(pts[i + 1], origin, scale),
                color, thickness);
        }

        if (closed && pts.size() >= 3) {
            dl->AddLine(worldToScreen(pts.back(), origin, scale),
                worldToScreen(pts.front(), origin, scale),
                color, thickness);
        }
    }

    void drawArrow(ImDrawList* dl,
        const a2::Vec2& a,
        const a2::Vec2& b,
        const ImVec2& origin,
        float scale,
        ImU32 color,
        float thickness = 2.0f) {
        ImVec2 p0 = worldToScreen(a, origin, scale);
        ImVec2 p1 = worldToScreen(b, origin, scale);

        dl->AddLine(p0, p1, color, thickness);

        const float dx = p1.x - p0.x;
        const float dy = p1.y - p0.y;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-6f) return;

        const float ux = dx / len;
        const float uy = dy / len;
        const float head = 10.0f;

        ImVec2 left(p1.x - head * (ux + 0.4f * uy), p1.y - head * (uy - 0.4f * ux));
        ImVec2 right(p1.x - head * (ux - 0.4f * uy), p1.y - head * (uy + 0.4f * ux));

        dl->AddLine(p1, left, color, thickness);
        dl->AddLine(p1, right, color, thickness);
    }

    void drawGrid(ImDrawList* dl, const ImVec2& pmin, const ImVec2& pmax, const ImVec2& origin, float stepPx) {
        const ImU32 gridColor = IM_COL32(70, 70, 76, 255);
        const ImU32 axisColor = IM_COL32(180, 180, 190, 255);

        for (float x = origin.x; x <= pmax.x; x += stepPx)
            dl->AddLine(ImVec2(x, pmin.y), ImVec2(x, pmax.y), gridColor, 1.0f);
        for (float x = origin.x; x >= pmin.x; x -= stepPx)
            dl->AddLine(ImVec2(x, pmin.y), ImVec2(x, pmax.y), gridColor, 1.0f);

        for (float y = origin.y; y <= pmax.y; y += stepPx)
            dl->AddLine(ImVec2(pmin.x, y), ImVec2(pmax.x, y), gridColor, 1.0f);
        for (float y = origin.y; y >= pmin.y; y -= stepPx)
            dl->AddLine(ImVec2(pmin.x, y), ImVec2(pmax.x, y), gridColor, 1.0f);

        dl->AddLine(ImVec2(pmin.x, origin.y), ImVec2(pmax.x, origin.y), axisColor, 2.0f);
        dl->AddLine(ImVec2(origin.x, pmin.y), ImVec2(origin.x, pmax.y), axisColor, 2.0f);
    }

    void drawPivot(ImDrawList* dl, float cx, float cy, const ImVec2& origin, float scale) {
        const ImVec2 p = worldToScreen(a2::Vec2{ cx, cy }, origin, scale);
        const ImU32 yellow = IM_COL32(240, 220, 70, 255);

        dl->AddCircleFilled(p, 5.0f, yellow);
        dl->AddLine(ImVec2(p.x - 8, p.y), ImVec2(p.x + 8, p.y), yellow, 2.0f);
        dl->AddLine(ImVec2(p.x, p.y - 8), ImVec2(p.x, p.y + 8), yellow, 2.0f);
    }

    std::vector<a2::Vec2> getShapeByMode(App::Assignment2ShapeMode mode) {
        switch (mode) {
        case App::Assignment2ShapeMode::Triangle: return a2::makeTriangle();
        case App::Assignment2ShapeMode::Square:   return a2::makeSquare();
        case App::Assignment2ShapeMode::House:    return a2::makeHouse();
        case App::Assignment2ShapeMode::Car:      return a2::makeCar();
        default:                                  return a2::makeTriangle();
        }
    }

    const char* objectModeName(App::Assignment2ObjectMode mode) {
        switch (mode) {
        case App::Assignment2ObjectMode::Point:  return "Point";
        case App::Assignment2ObjectMode::Vector: return "Vector";
        case App::Assignment2ObjectMode::Shape:  return "Shape";
        default: return "";
        }
    }


    a2::Vec2 add(const a2::Vec2& a, const a2::Vec2& b) {
        return { a.x + b.x, a.y + b.y };
    }

    a2::Vec2 sub(const a2::Vec2& a, const a2::Vec2& b) {
        return { a.x - b.x, a.y - b.y };
    }

    a2::Vec2 mul(const a2::Vec2& a, float s) {
        return { a.x * s, a.y * s };
    }

    float length(const a2::Vec2& v) {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }

    a2::Vec2 lerp(const a2::Vec2& a, const a2::Vec2& b, float t) {
        return add(mul(a, 1.0f - t), mul(b, t));
    }

    a2::Vec2 screenToWorld(const ImVec2& p, const ImVec2& origin, float scale) {
        return a2::Vec2{ (p.x - origin.x) / scale, (origin.y - p.y) / scale };
    }


    std::vector<a2::Vec2> activeControlPoints(const std::array<a2::Vec2, 10>& cp, int count) {
        count = std::clamp(count, 2, 10);
        return std::vector<a2::Vec2>(cp.begin(), cp.begin() + count);
    }

    a2::Vec2 evalBezierGeneral(const std::vector<a2::Vec2>& cp, float t) {
        if (cp.empty()) return {0.0f, 0.0f};
        std::vector<a2::Vec2> level = cp;
        for (int n = static_cast<int>(level.size()) - 1; n > 0; --n) {
            for (int i = 0; i < n; ++i) level[i] = lerp(level[i], level[i + 1], t);
        }
        return level[0];
    }

    a2::Vec2 derivBezierGeneral(const std::vector<a2::Vec2>& cp, float t) {
        const int n = static_cast<int>(cp.size()) - 1;
        if (n <= 0) return {0.0f, 0.0f};
        std::vector<a2::Vec2> dcp;
        dcp.reserve(static_cast<size_t>(n));
        for (int i = 0; i < n; ++i) dcp.push_back(mul(sub(cp[i + 1], cp[i]), static_cast<float>(n)));
        return evalBezierGeneral(dcp, t);
    }

    std::vector<a2::Vec2> sampleBezierGeneral(const std::vector<a2::Vec2>& cp, int samples) {
        samples = (std::max)(samples, 2);
        std::vector<a2::Vec2> pts;
        pts.reserve(static_cast<size_t>(samples) + 1);
        for (int i = 0; i <= samples; ++i) pts.push_back(evalBezierGeneral(cp, static_cast<float>(i) / static_cast<float>(samples)));
        return pts;
    }

    float approximateArcLength(const std::vector<a2::Vec2>& pts);

    float mapNormalizedArcToTGeneral(const std::vector<a2::Vec2>& cp, float s01, int samples = 300) {
        s01 = std::clamp(s01, 0.0f, 1.0f);
        auto pts = sampleBezierGeneral(cp, samples);
        const float total = approximateArcLength(pts);
        if (total <= 1e-6f) return s01;
        const float target = s01 * total;
        float accum = 0.0f;
        for (int i = 0; i < samples; ++i) {
            const float seg = length(sub(pts[i + 1], pts[i]));
            if (accum + seg >= target && seg > 1e-6f) {
                const float local = (target - accum) / seg;
                const float t0 = static_cast<float>(i) / static_cast<float>(samples);
                const float t1 = static_cast<float>(i + 1) / static_cast<float>(samples);
                return t0 + local * (t1 - t0);
            }
            accum += seg;
        }
        return 1.0f;
    }

    std::vector<std::vector<a2::Vec2>> buildDeCasteljauLevelsGeneral(const std::vector<a2::Vec2>& cp, float t) {
        std::vector<std::vector<a2::Vec2>> levels;
        if (cp.empty()) return levels;
        levels.push_back(cp);
        while (levels.back().size() > 1) {
            const auto& prev = levels.back();
            std::vector<a2::Vec2> next;
            next.reserve(prev.size() - 1);
            for (size_t i = 0; i + 1 < prev.size(); ++i) next.push_back(lerp(prev[i], prev[i + 1], t));
            levels.push_back(std::move(next));
        }
        return levels;
    }

    struct GeneralSubdivision {
        std::vector<a2::Vec2> left;
        std::vector<a2::Vec2> right;
    };

    GeneralSubdivision splitBezierGeneral(const std::vector<a2::Vec2>& cp, float t) {
        GeneralSubdivision out;
        if (cp.empty()) return out;
        auto levels = buildDeCasteljauLevelsGeneral(cp, t);
        out.left.reserve(levels.size());
        out.right.reserve(levels.size());
        for (size_t i = 0; i < levels.size(); ++i) {
            out.left.push_back(levels[i].front());
            out.right.push_back(levels[levels.size() - 1 - i].back());
        }
        return out;
    }



    std::vector<float> makeClampedUniformKnots(int controlCount, int degree) {
        const int n = controlCount - 1;
        const int m = n + degree + 1;
        std::vector<float> knots(static_cast<size_t>(m + 1), 0.0f);
        for (int i = 0; i <= degree; ++i) knots[static_cast<size_t>(i)] = 0.0f;
        for (int i = m - degree; i <= m; ++i) knots[static_cast<size_t>(i)] = 1.0f;
        const int internalCount = n - degree;
        for (int j = 1; j <= internalCount; ++j) {
            knots[static_cast<size_t>(degree + j)] = static_cast<float>(j) / static_cast<float>(internalCount + 1);
        }
        return knots;
    }

    float coxDeBoorBasis(int i, int degree, float t, const std::vector<float>& knots) {
        if (degree == 0) {
            if ((knots[static_cast<size_t>(i)] <= t && t < knots[static_cast<size_t>(i + 1)]) ||
                (t >= 1.0f && knots[static_cast<size_t>(i + 1)] == 1.0f && knots[static_cast<size_t>(i)] < 1.0f)) {
                return 1.0f;
            }
            return 0.0f;
        }
        float left = 0.0f;
        const float leftDen = knots[static_cast<size_t>(i + degree)] - knots[static_cast<size_t>(i)];
        if (std::abs(leftDen) > 1e-8f) {
            left = (t - knots[static_cast<size_t>(i)]) / leftDen * coxDeBoorBasis(i, degree - 1, t, knots);
        }
        float right = 0.0f;
        const float rightDen = knots[static_cast<size_t>(i + degree + 1)] - knots[static_cast<size_t>(i + 1)];
        if (std::abs(rightDen) > 1e-8f) {
            right = (knots[static_cast<size_t>(i + degree + 1)] - t) / rightDen * coxDeBoorBasis(i + 1, degree - 1, t, knots);
        }
        return left + right;
    }

    std::vector<float> bSplineBasisWeights(int controlCount, int degree, float t) {
        std::vector<float> w(static_cast<size_t>(controlCount), 0.0f);
        if (controlCount <= degree) return w;
        const auto knots = makeClampedUniformKnots(controlCount, degree);
        for (int i = 0; i < controlCount; ++i) w[static_cast<size_t>(i)] = coxDeBoorBasis(i, degree, t, knots);
        return w;
    }

    a2::Vec2 evalBSpline(const std::vector<a2::Vec2>& cp, int degree, float t) {
        if (cp.empty()) return {0.0f, 0.0f};
        if (static_cast<int>(cp.size()) <= degree) return cp.front();
        const auto w = bSplineBasisWeights(static_cast<int>(cp.size()), degree, t);
        a2::Vec2 out{0.0f, 0.0f};
        for (size_t i = 0; i < cp.size(); ++i) out = add(out, mul(cp[i], w[i]));
        return out;
    }

    a2::Vec2 derivBSpline(const std::vector<a2::Vec2>& cp, int degree, float t) {
        const float eps = 1e-3f;
        const float t0 = std::clamp(t - eps, 0.0f, 1.0f);
        const float t1 = std::clamp(t + eps, 0.0f, 1.0f);
        if (std::abs(t1 - t0) < 1e-6f) return {0.0f, 0.0f};
        return mul(sub(evalBSpline(cp, degree, t1), evalBSpline(cp, degree, t0)), 1.0f / (t1 - t0));
    }

    std::vector<a2::Vec2> sampleBSplineCurve(const std::vector<a2::Vec2>& cp, int degree, int samples) {
        samples = (std::max)(samples, 2);
        std::vector<a2::Vec2> pts;
        pts.reserve(static_cast<size_t>(samples) + 1);
        for (int i = 0; i <= samples; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(samples);
            pts.push_back(evalBSpline(cp, degree, t));
        }
        return pts;
    }
    std::vector<float> bernsteinWeights(int degree, float t) {
        std::vector<float> w(static_cast<size_t>(degree + 1), 0.0f);
        const float u = 1.0f - t;
        auto binom = [](int n, int k) {
            double r = 1.0;
            for (int i = 1; i <= k; ++i) r = r * (n - (k - i)) / i;
            return r;
        };
        for (int i = 0; i <= degree; ++i) {
            w[static_cast<size_t>(i)] = static_cast<float>(binom(degree, i) * std::pow(u, degree - i) * std::pow(t, i));
        }
        return w;
    }

    a2::Vec2 evalHermite(const std::array<a2::Vec2, 4>& cp, float t) {
        const a2::Vec2& p0 = cp[0];
        const a2::Vec2& t0Tip = cp[1];
        const a2::Vec2& t1Tip = cp[2];
        const a2::Vec2& p1 = cp[3];
        const a2::Vec2 m0 = sub(t0Tip, p0);
        const a2::Vec2 m1 = sub(t1Tip, p1);
        const float t2 = t * t;
        const float t3 = t2 * t;
        const float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
        const float h10 = t3 - 2.0f * t2 + t;
        const float h01 = -2.0f * t3 + 3.0f * t2;
        const float h11 = t3 - t2;
        return add(add(mul(p0, h00), mul(m0, h10)), add(mul(p1, h01), mul(m1, h11)));
    }

    a2::Vec2 derivHermite(const std::array<a2::Vec2, 4>& cp, float t) {
        const a2::Vec2& p0 = cp[0];
        const a2::Vec2& t0Tip = cp[1];
        const a2::Vec2& t1Tip = cp[2];
        const a2::Vec2& p1 = cp[3];
        const a2::Vec2 m0 = sub(t0Tip, p0);
        const a2::Vec2 m1 = sub(t1Tip, p1);
        const float t2 = t * t;
        const float dh00 = 6.0f * t2 - 6.0f * t;
        const float dh10 = 3.0f * t2 - 4.0f * t + 1.0f;
        const float dh01 = -6.0f * t2 + 6.0f * t;
        const float dh11 = 3.0f * t2 - 2.0f * t;
        return add(add(mul(p0, dh00), mul(m0, dh10)), add(mul(p1, dh01), mul(m1, dh11)));
    }

    std::vector<a2::Vec2> sampleHermiteCurve(const std::array<a2::Vec2, 4>& cp, int samples) {
        samples = (std::max)(samples, 2);
        std::vector<a2::Vec2> pts;
        pts.reserve(static_cast<size_t>(samples) + 1);
        for (int i = 0; i <= samples; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(samples);
            pts.push_back(evalHermite(cp, t));
        }
        return pts;
    }

    void enforcePiecewiseC1(std::array<a2::Vec2, 10>& cp) {
        cp[4] = add(cp[3], sub(cp[3], cp[2]));
    }

    a2::Vec2 evalPiecewiseBezier(const std::array<a2::Vec2, 10>& cp, float t) {
        std::array<a2::Vec2, 4> left{cp[0], cp[1], cp[2], cp[3]};
        std::array<a2::Vec2, 4> right{cp[3], cp[4], cp[5], cp[6]};
        if (t <= 0.5f) {
            return evalBezierGeneral(std::vector<a2::Vec2>{left[0], left[1], left[2], left[3]}, t * 2.0f);
        }
        return evalBezierGeneral(std::vector<a2::Vec2>{right[0], right[1], right[2], right[3]}, (t - 0.5f) * 2.0f);
    }

    a2::Vec2 derivPiecewiseBezier(const std::array<a2::Vec2, 10>& cp, float t) {
        std::array<a2::Vec2, 4> left{cp[0], cp[1], cp[2], cp[3]};
        std::array<a2::Vec2, 4> right{cp[3], cp[4], cp[5], cp[6]};
        if (t <= 0.5f) {
            return mul(derivBezierGeneral(std::vector<a2::Vec2>{left[0], left[1], left[2], left[3]}, t * 2.0f), 2.0f);
        }
        return mul(derivBezierGeneral(std::vector<a2::Vec2>{right[0], right[1], right[2], right[3]}, (t - 0.5f) * 2.0f), 2.0f);
    }

    std::vector<a2::Vec2> samplePiecewiseBezierCurve(const std::array<a2::Vec2, 10>& cp, int samples) {
        samples = (std::max)(samples, 2);
        std::vector<a2::Vec2> pts;
        pts.reserve(static_cast<size_t>(samples) + 1);
        for (int i = 0; i <= samples; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(samples);
            pts.push_back(evalPiecewiseBezier(cp, t));
        }
        return pts;
    }

    struct PiecewiseConstructionState {
        std::vector<std::vector<a2::Vec2>> levels;
        bool secondSegment = false;
    };

    PiecewiseConstructionState buildPiecewiseConstruction(const std::array<a2::Vec2, 10>& cp, float t) {
        PiecewiseConstructionState out{};
        out.secondSegment = (t > 0.5f);
        const float localT = out.secondSegment ? (t - 0.5f) * 2.0f : t * 2.0f;
        std::vector<a2::Vec2> seg = out.secondSegment
            ? std::vector<a2::Vec2>{cp[3], cp[4], cp[5], cp[6]}
            : std::vector<a2::Vec2>{cp[0], cp[1], cp[2], cp[3]};
        out.levels = buildDeCasteljauLevelsGeneral(seg, localT);
        return out;
    }

    a2::Vec2 evalQuadraticBezier(const std::array<a2::Vec2, 4>& cp, float t) {
        const float u = 1.0f - t;
        return add(add(mul(cp[0], u * u), mul(cp[1], 2.0f * u * t)), mul(cp[2], t * t));
    }

    a2::Vec2 evalCubicBezier(const std::array<a2::Vec2, 4>& cp, float t) {
        const float u = 1.0f - t;
        return add(add(mul(cp[0], u * u * u), mul(cp[1], 3.0f * u * u * t)),
                   add(mul(cp[2], 3.0f * u * t * t), mul(cp[3], t * t * t)));
    }

    a2::Vec2 derivQuadraticBezier(const std::array<a2::Vec2, 4>& cp, float t) {
        return add(mul(sub(cp[1], cp[0]), 2.0f * (1.0f - t)), mul(sub(cp[2], cp[1]), 2.0f * t));
    }

    a2::Vec2 derivCubicBezier(const std::array<a2::Vec2, 4>& cp, float t) {
        const float u = 1.0f - t;
        return add(add(mul(sub(cp[1], cp[0]), 3.0f * u * u), mul(sub(cp[2], cp[1]), 6.0f * u * t)),
                   mul(sub(cp[3], cp[2]), 3.0f * t * t));
    }

    std::vector<a2::Vec2> sampleBezierCurve(const std::array<a2::Vec2, 4>& cp, bool cubic, int samples) {
        samples = (std::max)(samples, 2);
        std::vector<a2::Vec2> pts;
        pts.reserve(static_cast<size_t>(samples) + 1);
        for (int i = 0; i <= samples; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(samples);
            pts.push_back(cubic ? evalCubicBezier(cp, t) : evalQuadraticBezier(cp, t));
        }
        return pts;
    }

    float approximateArcLength(const std::vector<a2::Vec2>& pts) {
        float total = 0.0f;
        for (size_t i = 0; i + 1 < pts.size(); ++i) total += length(sub(pts[i + 1], pts[i]));
        return total;
    }

    float mapNormalizedArcToT(const std::array<a2::Vec2, 4>& cp, bool cubic, float s01, int samples = 300) {
        s01 = std::clamp(s01, 0.0f, 1.0f);
        auto pts = sampleBezierCurve(cp, cubic, samples);
        const float total = approximateArcLength(pts);
        if (total <= 1e-6f) return s01;
        const float target = s01 * total;
        float accum = 0.0f;
        for (int i = 0; i < samples; ++i) {
            const float seg = length(sub(pts[i + 1], pts[i]));
            if (accum + seg >= target && seg > 1e-6f) {
                const float local = (target - accum) / seg;
                const float t0 = static_cast<float>(i) / static_cast<float>(samples);
                const float t1 = static_cast<float>(i + 1) / static_cast<float>(samples);
                return t0 + local * (t1 - t0);
            }
            accum += seg;
        }
        return 1.0f;
    }

    std::vector<a2::Vec2> convexHull(std::vector<a2::Vec2> pts) {
        if (pts.size() <= 1) return pts;
        std::sort(pts.begin(), pts.end(), [](const a2::Vec2& a, const a2::Vec2& b) {
            if (a.x != b.x) return a.x < b.x;
            return a.y < b.y;
        });
        auto cross = [](const a2::Vec2& o, const a2::Vec2& a, const a2::Vec2& b) {
            return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
        };
        std::vector<a2::Vec2> h;
        for (const auto& p : pts) {
            while (h.size() >= 2 && cross(h[h.size()-2], h[h.size()-1], p) <= 0.0f) h.pop_back();
            h.push_back(p);
        }
        size_t lower = h.size();
        for (int i = static_cast<int>(pts.size()) - 2; i >= 0; --i) {
            const auto& p = pts[i];
            while (h.size() > lower && cross(h[h.size()-2], h[h.size()-1], p) <= 0.0f) h.pop_back();
            h.push_back(p);
        }
        if (!h.empty()) h.pop_back();
        return h;
    }

    struct SubdividedBezier {
        std::vector<a2::Vec2> left;
        std::vector<a2::Vec2> right;
    };

    SubdividedBezier splitBezier(const std::array<a2::Vec2, 4>& cp, bool cubic, float t) {
        SubdividedBezier out;
        if (cubic) {
            const a2::Vec2 q0 = lerp(cp[0], cp[1], t);
            const a2::Vec2 q1 = lerp(cp[1], cp[2], t);
            const a2::Vec2 q2 = lerp(cp[2], cp[3], t);
            const a2::Vec2 r0 = lerp(q0, q1, t);
            const a2::Vec2 r1 = lerp(q1, q2, t);
            const a2::Vec2 s = lerp(r0, r1, t);
            out.left = {cp[0], q0, r0, s};
            out.right = {s, r1, q2, cp[3]};
        } else {
            const a2::Vec2 q0 = lerp(cp[0], cp[1], t);
            const a2::Vec2 q1 = lerp(cp[1], cp[2], t);
            const a2::Vec2 s = lerp(q0, q1, t);
            out.left = {cp[0], q0, s};
            out.right = {s, q1, cp[2]};
        }
        return out;
    }

    std::array<float,4> cubicBernstein(float t) {
        const float u = 1.0f - t;
        return {u*u*u, 3.0f*u*u*t, 3.0f*u*t*t, t*t*t};
    }

    std::array<float,3> quadBernstein(float t) {
        const float u = 1.0f - t;
        return {u*u, 2.0f*u*t, t*t};
    }

    struct DeCasteljauState {
        a2::Vec2 b;
        std::vector<a2::Vec2> level1;
        std::vector<a2::Vec2> level2;
        std::vector<a2::Vec2> level3;
    };

    DeCasteljauState buildDeCasteljau(const std::array<a2::Vec2, 4>& cp, bool cubic, float t) {
        DeCasteljauState s{};
        if (cubic) {
            s.level1 = { lerp(cp[0], cp[1], t), lerp(cp[1], cp[2], t), lerp(cp[2], cp[3], t) };
            s.level2 = { lerp(s.level1[0], s.level1[1], t), lerp(s.level1[1], s.level1[2], t) };
            s.level3 = { lerp(s.level2[0], s.level2[1], t) };
            s.b = s.level3[0];
        } else {
            s.level1 = { lerp(cp[0], cp[1], t), lerp(cp[1], cp[2], t) };
            s.level2 = { lerp(s.level1[0], s.level1[1], t) };
            s.b = s.level2[0];
        }
        return s;
    }

    void drawPoint(ImDrawList* dl, const a2::Vec2& p, const ImVec2& origin, float scale, ImU32 color, float radius = 5.0f) {
        dl->AddCircleFilled(worldToScreen(p, origin, scale), radius, color);
    }

    void drawOpenPolyline(ImDrawList* dl,
        const std::vector<a2::Vec2>& pts,
        const ImVec2& origin,
        float scale,
        ImU32 color,
        float thickness = 2.0f) {
        if (pts.size() < 2) return;
        for (size_t i = 0; i + 1 < pts.size(); ++i) {
            dl->AddLine(worldToScreen(pts[i], origin, scale),
                        worldToScreen(pts[i + 1], origin, scale),
                        color, thickness);
        }
    }

    ImVec2 ndcToScreen(const a3::Vec3& p, const ImVec2& pmin, const ImVec2& pmax) {
        const float x = pmin.x + (p.x * 0.5f + 0.5f) * (pmax.x - pmin.x);
        const float y = pmin.y + (-p.y * 0.5f + 0.5f) * (pmax.y - pmin.y);
        return ImVec2(x, y);
    }


} // namespace

void App::setStatus(const std::string& msg) {
    m_status = msg;
    std::fprintf(stderr, "[App] %s\n", msg.c_str());
}

int App::run(int /*argc*/, char** /*argv*/) {
    if (!initWindow()) return 1;
    initImGui();

    m_bg = std::make_unique<Image>(512, 512, 4);
    m_bg->fillCheckerboard();
    m_out = std::make_unique<Image>(*m_bg);

    setStatus("Launcher: choose an assignment tool.");
    rebuildOutputTexture();

    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        int display_w = 0, display_h = 0;
        glfwGetFramebufferSize(m_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (m_mode == ToolMode::Assignment4) {
            if (!m_a4Initialized) {
                std::string err;
                if (m_a4Renderer.initialize("assets", err)) {
                    m_a4Initialized = true;
                    setStatus("Assignment 4 loaded.");
                } else {
                    setStatus("Assignment 4 renderer failed to initialize: " + err);
                    m_mode = ToolMode::Launcher;
                }
            }
            if (m_a4Initialized) {
                m_a4Renderer.render(display_w, display_h, m_a4RenderMode, m_a4Rotate, static_cast<float>(glfwGetTime()));
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        drawUI();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }

    shutdown();
    return 0;
}

bool App::initWindow() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_window = glfwCreateWindow(m_winW, m_winH, "CS 480/680 - ImageProcessing Project", nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "Failed to initialize GLAD\n");
        return false;
    }
    return true;
}

void App::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    io.FontGlobalScale = 1.25f;
    ImGui::GetStyle().ScaleAllSizes(1.25f);
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void App::shutdown() {
    if (m_outTex) {
        glDeleteTextures(1, &m_outTex);
        m_outTex = 0;
    }
    if (m_a4Initialized) {
        m_a4Renderer.shutdown();
        m_a4Initialized = false;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

std::string App::openImageDialog() {
    const char* patterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.tga" };
    const char* path = tinyfd_openFileDialog("Select an image", "", (int)(sizeof(patterns) / sizeof(patterns[0])), patterns, "Image files", 0);
    return path ? std::string(path) : std::string();
}

std::string App::savePngDialog() {
    const char* patterns[] = { "*.png" };
    const char* path = tinyfd_saveFileDialog("Save output PNG", "output.png", (int)(sizeof(patterns) / sizeof(patterns[0])), patterns, "PNG image");
    if (!path) return {};
    std::string p(path);
    if (!endsWithPng(p)) p += ".png";
    return p;
}

void App::drawUI() {
    switch (m_mode) {
    case ToolMode::Launcher:    drawLauncherUI();    break;
    case ToolMode::Assignment1: drawAssignment1UI(); break;
    case ToolMode::Assignment2: drawAssignment2UI(); break;
    case ToolMode::Assignment3: drawAssignment3UI(); break;
    case ToolMode::Assignment4: drawAssignment4UI(); break;
    case ToolMode::Assignment5:

        m_mode = ToolMode::Launcher;
        setStatus("That assignment tool is coming soon.");
        drawLauncherUI();
        break;
    case ToolMode::Assignment7: drawAssignment7UI(); break;
    default:
        m_mode = ToolMode::Launcher;
        setStatus("That assignment tool is not implemented yet.");
        drawLauncherUI();
        break;
    }
}

void App::drawLauncherUI() {
    ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(520, 460), ImGuiCond_FirstUseEver);
    ImGui::Begin("CS 480/680 - Project Launcher", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::TextWrapped("Choose which tool to run.");
    ImGui::Separator();
    if (!m_status.empty()) {
        ImGui::TextWrapped("%s", m_status.c_str());
        ImGui::Separator();
    }

    ImVec2 btn(460, 0);
    if (ImGui::Button("Assignment 1: Image Processing", btn)) {
        m_mode = ToolMode::Assignment1;
        setStatus("Assignment 1 loaded.");
        rebuildOutputTexture();
    }
    if (ImGui::Button("Assignment 2: Interactive 2D Transformations", btn)) {
        m_mode = ToolMode::Assignment2;
        setStatus("Assignment 2 loaded.");
    }
    if (ImGui::Button("Assignment 3: 3D Transformations, Viewing, and Projection", btn)) {
        m_mode = ToolMode::Assignment3;
        setStatus("Assignment 3 loaded.");
    }
    if (ImGui::Button("Assignment 4: OpenGL Graphics Pipeline Explorer", btn)) {
        m_mode = ToolMode::Assignment4;
        setStatus("Assignment 4 loaded.");
    }
    ImGui::BeginDisabled(true);
    ImGui::Button("Assignment 5: OpenGL Mesh Rendering (Coming Soon)", btn);
    ImGui::Button("Assignment 6: Lighting and Shading (Coming Soon)", btn);
    ImGui::EndDisabled();
    if (ImGui::Button("Interactive Bezier Curves", btn)) {
        m_mode = ToolMode::Assignment7;
        setStatus("Assignment 7 loaded.");
    }

    ImGui::Separator();
    if (ImGui::Button("Quit", btn)) {
        glfwSetWindowShouldClose(m_window, 1);
    }

    ImGui::End();
}

void App::drawAssignment1UI() {
    const ImVec2 bigBtn(180, 0);

    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 840), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags controlsFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
    ImGui::Begin("Assignment 1 - Controls", nullptr, controlsFlags);

    if (ImGui::Button("< Back to Launcher", bigBtn)) {
        m_mode = ToolMode::Launcher;
        setStatus("Launcher: choose an assignment tool.");
        ImGui::End();
        return;
    }

    ImGui::Separator();
    if (!m_status.empty()) ImGui::TextWrapped("%s", m_status.c_str());
    ImGui::Separator();

    ImGui::Checkbox("Linear workflow (sRGB <-> Linear)", &m_doLinearWorkflow);
    ImGui::Checkbox("Checkerboard background for preview", &m_showChecker);
    ImGui::Checkbox("Save with alpha (otherwise flatten to opaque)", &m_saveWithAlpha);

    ImGui::Separator();
    ImGui::Text("Background image");
    if (ImGui::Button("Choose BG...", bigBtn)) {
        std::string p = openImageDialog();
        if (!p.empty()) { m_bgPath = p; loadImage(m_bgPath, false); }
    }
    ImGui::SameLine();
    if (ImGui::Button("Use checker BG", bigBtn)) {
        m_bg = std::make_unique<Image>(512, 512, 4);
        m_bg->fillCheckerboard();
        setStatus("Using checkerboard background.");
        rebuildOutputTexture();
    }
    ImGui::TextWrapped("BG: %s", m_bgPath.empty() ? "(none)" : m_bgPath.c_str());

    ImGui::Separator();
    ImGui::Text("Foreground image (with alpha)");
    if (ImGui::Button("Choose FG...", bigBtn)) {
        std::string p = openImageDialog();
        if (!p.empty()) { m_fgPath = p; loadImage(m_fgPath, true); }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear FG", bigBtn)) {
        m_fg.reset();
        setStatus("Foreground cleared.");
        rebuildOutputTexture();
    }
    ImGui::TextWrapped("FG: %s", m_fgPath.empty() ? "(none)" : m_fgPath.c_str());

    ImGui::SliderFloat("Extra FG opacity", &m_fgOpacity, 0.0f, 1.0f);

    ImGui::Separator();
    const char* modes[] = { "Normal (alpha over)", "Additive", "Multiply", "Screen", "Difference" };
    ImGui::Combo("Blend mode", &m_blendMode, modes, IM_ARRAYSIZE(modes));

    ImGui::Separator();
    ImGui::Checkbox("Apply brightness to output", &m_applyBrightness);
    ImGui::SliderFloat("Brightness", &m_brightness, -1.0f, 1.0f, "%.3f");
    if (ImGui::Button("Recompute output", bigBtn)) rebuildOutputTexture();

    ImGui::Separator();
    ImGui::Text("Submission");
    ImGui::InputText("Student name", &m_studentName);
    ImGui::Checkbox("Stamp name on saved image", &m_stampName);

    ImGui::Separator();
    ImGui::Text("Save output");
    if (ImGui::Button("Save PNG...", bigBtn)) {
        std::string p = savePngDialog();
        if (!p.empty()) { m_savePath = p; saveImage(m_savePath); }
    }
    ImGui::TextWrapped("Save: %s", m_savePath.empty() ? "(none)" : m_savePath.c_str());
    ImGui::End();

    drawImagePanel();
}

void App::drawAssignment2UI() {
    using namespace a2;

    const ImVec2 bigBtn(220, 0);

    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430, 840), ImGuiCond_FirstUseEver);
    ImGui::Begin("Assignment 2 - Controls", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

    if (ImGui::Button("< Back to Launcher", bigBtn)) {
        m_mode = ToolMode::Launcher;
        setStatus("Launcher: choose an assignment tool.");
        ImGui::End();
        return;
    }

    ImGui::Separator();
    if (!m_status.empty()) ImGui::TextWrapped("%s", m_status.c_str());
    ImGui::Separator();

    ImGui::Text("Object Type");
    int objectMode = static_cast<int>(m_a2ObjectMode);
    ImGui::RadioButton("Point##obj", &objectMode, static_cast<int>(Assignment2ObjectMode::Point));  ImGui::SameLine();
    ImGui::RadioButton("Vector##obj", &objectMode, static_cast<int>(Assignment2ObjectMode::Vector)); ImGui::SameLine();
    ImGui::RadioButton("Shape##obj", &objectMode, static_cast<int>(Assignment2ObjectMode::Shape));
    m_a2ObjectMode = static_cast<Assignment2ObjectMode>(objectMode);

    if (m_a2ObjectMode == Assignment2ObjectMode::Shape) {
        const char* shapes[] = { "Triangle", "Square", "House", "Car" };
        int shapeMode = static_cast<int>(m_a2ShapeMode);
        ImGui::Combo("Shape##combo", &shapeMode, shapes, IM_ARRAYSIZE(shapes));
        m_a2ShapeMode = static_cast<Assignment2ShapeMode>(shapeMode);
    }
    else {
        ImGui::SliderFloat("Input X", &m_a2InputX, -8.0f, 8.0f, "%.2f");
        ImGui::SliderFloat("Input Y", &m_a2InputY, -8.0f, 8.0f, "%.2f");
    }

    ImGui::Separator();
    ImGui::Text("Transform Controls");
    ImGui::SliderFloat("Translate X", &m_tx, -8.0f, 8.0f, "%.2f");
    ImGui::SliderFloat("Translate Y", &m_ty, -8.0f, 8.0f, "%.2f");
    ImGui::SliderFloat("Scale X", &m_sx, 0.1f, 3.0f, "%.2f");
    ImGui::SliderFloat("Scale Y", &m_sy, 0.1f, 3.0f, "%.2f");
    ImGui::SliderFloat("Rotation (deg)", &m_thetaDeg, -180.0f, 180.0f, "%.1f");
    ImGui::SliderFloat("Shear X", &m_shearX, -2.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Shear Y", &m_shearY, -2.0f, 2.0f, "%.2f");

    ImGui::Separator();
    ImGui::Checkbox("Use Pivot Rotation", &m_usePivotRotation);
    ImGui::Checkbox("Use Pivot Scale", &m_usePivotScale);
    ImGui::BeginDisabled(!(m_usePivotRotation || m_usePivotScale));
    ImGui::SliderFloat("Pivot X", &m_pivotX, -8.0f, 8.0f, "%.2f");
    ImGui::SliderFloat("Pivot Y", &m_pivotY, -8.0f, 8.0f, "%.2f");
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::Checkbox("Show Original", &m_showOriginal);
    ImGui::Checkbox("Show Transformed", &m_showTransformed);
    ImGui::Checkbox("Show Basis", &m_showBasis);
    ImGui::Checkbox("Show Grid", &m_showGrid);
    ImGui::Checkbox("Show Step Stages", &m_showStepStages);

    if (ImGui::Button("Reset", bigBtn)) {
        m_a2ObjectMode = Assignment2ObjectMode::Shape;
        m_a2ShapeMode = Assignment2ShapeMode::Triangle;

        m_a2InputX = 0.6f;
        m_a2InputY = 0.2f;

        m_tx = 0.0f;
        m_ty = 0.0f;
        m_sx = 1.0f;
        m_sy = 1.0f;
        m_thetaDeg = 0.0f;
        m_shearX = 0.0f;
        m_shearY = 0.0f;

        m_usePivotRotation = false;
        m_usePivotScale = false;
        m_pivotX = 0.0f;
        m_pivotY = 0.0f;

        m_showOriginal = true;
        m_showTransformed = true;
        m_showBasis = true;
        m_showGrid = true;
        m_showStepStages = true;
    }

    Mat3 M = buildStandardTransform(m_tx, m_ty, m_thetaDeg, m_sx, m_sy, m_shearX, m_shearY);

    if (m_usePivotRotation) {
        M = multiply(makeTranslation(m_tx, m_ty),
            multiply(makeRotationAboutPivot(m_thetaDeg, m_pivotX, m_pivotY),
                multiply(makeShearY(m_shearY),
                    multiply(makeShearX(m_shearX),
                        makeScale(m_sx, m_sy)))));
    }

    if (m_usePivotScale) {
        M = multiply(makeTranslation(m_tx, m_ty),
            multiply(makeRotationDeg(m_thetaDeg),
                multiply(makeShearY(m_shearY),
                    multiply(makeShearX(m_shearX),
                        makeScaleAboutPivot(m_sx, m_sy, m_pivotX, m_pivotY)))));
    }

    Mat3 linearOnly = buildLinearOnlyTransform(m_thetaDeg, m_sx, m_sy, m_shearX, m_shearY);

    ImGui::Separator();
    ImGui::Text("Current Object: %s", objectModeName(m_a2ObjectMode));
    ImGui::Text("Final matrix M");
    for (int r = 0; r < 3; ++r) {
        ImGui::Text("[ %8.3f  %8.3f  %8.3f ]", M.m[r][0], M.m[r][1], M.m[r][2]);
    }

    if (m_a2ObjectMode == Assignment2ObjectMode::Point) {
        Vec3 p = makePoint(m_a2InputX, m_a2InputY);
        Vec3 tp = transform(M, p);
        ImGui::Separator();
        ImGui::Text("Point input       = (%.3f, %.3f, %.1f)", p.x, p.y, p.w);
        ImGui::Text("Point transformed = (%.3f, %.3f, %.3f)", tp.x, tp.y, tp.w);
    }
    else if (m_a2ObjectMode == Assignment2ObjectMode::Vector) {
        Vec3 v = makeVector(m_a2InputX, m_a2InputY);
        Vec3 tv = transform(M, v);
        ImGui::Separator();
        ImGui::Text("Vector input       = (%.3f, %.3f, %.1f)", v.x, v.y, v.w);
        ImGui::Text("Vector transformed = (%.3f, %.3f, %.3f)", tv.x, tv.y, tv.w);
        ImGui::TextWrapped("Because w = 0, translation should not change the vector's position.");
    }

    ImGui::Separator();
    ImGui::TextWrapped("Default order used here: M = T * R * ShY * ShX * S");
    ImGui::TextWrapped("Points use w = 1. Vectors use w = 0.");

    if (m_showStepStages) {
        ImGui::TextWrapped("Comparing geometric behavior under scale, shear, rotation, and translation.");
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(470, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1090, 840), ImGuiCond_FirstUseEver);
    ImGui::Begin("Assignment 2 - Canvas", nullptr, ImGuiWindowFlags_NoCollapse);

    ImVec2 canvasP0 = ImGui::GetCursorScreenPos();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 50.0f) avail.x = 50.0f;
    if (avail.y < 50.0f) avail.y = 50.0f;
    ImVec2 canvasP1(canvasP0.x + avail.x, canvasP0.y + avail.y);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(canvasP0, canvasP1, IM_COL32(24, 24, 28, 255));
    dl->AddRect(canvasP0, canvasP1, IM_COL32(90, 90, 100, 255));

    const ImVec2 origin((canvasP0.x + canvasP1.x) * 0.5f, (canvasP0.y + canvasP1.y) * 0.5f);
    const float scalePx = (std::min)(avail.x, avail.y) / 16.0f;

    if (m_showGrid) {
        drawGrid(dl, canvasP0, canvasP1, origin, scalePx);
    }

    if (m_showBasis) {
        auto bx = transformedBasisX(linearOnly);
        auto by = transformedBasisY(linearOnly);

        drawArrow(dl, { 0.0f, 0.0f }, { 3.0f, 0.0f }, origin, scalePx, IM_COL32(235, 85, 85, 255), 2.0f);
        drawArrow(dl, { 0.0f, 0.0f }, { 0.0f, 3.0f }, origin, scalePx, IM_COL32(80, 210, 120, 255), 2.0f);

        drawArrow(dl, bx[0], bx[1], origin, scalePx, IM_COL32(255, 120, 120, 255), 3.0f);
        drawArrow(dl, by[0], by[1], origin, scalePx, IM_COL32(120, 255, 120, 255), 3.0f);
    }

    if (m_usePivotRotation || m_usePivotScale) {
        drawPivot(dl, m_pivotX, m_pivotY, origin, scalePx);
    }

    if (m_a2ObjectMode == Assignment2ObjectMode::Point) {
        Vec3 p = makePoint(m_a2InputX, m_a2InputY);
        Vec3 tp = transform(M, p);

        if (m_showOriginal) {
            dl->AddCircleFilled(worldToScreen({ p.x, p.y }, origin, scalePx), 5.0f,
                IM_COL32(180, 180, 255, 255));
        }
        if (m_showTransformed) {
            dl->AddCircleFilled(worldToScreen({ tp.x, tp.y }, origin, scalePx), 6.0f,
                IM_COL32(255, 200, 80, 255));
        }
    }
    else if (m_a2ObjectMode == Assignment2ObjectMode::Vector) {
        Vec3 v = makeVector(m_a2InputX, m_a2InputY);
        Vec3 tv = transform(M, v);

        if (m_showOriginal) {
            drawArrow(dl, { 0.0f, 0.0f }, { v.x, v.y }, origin, scalePx,
                IM_COL32(180, 180, 255, 255), 2.5f);
        }
        if (m_showTransformed) {
            drawArrow(dl, { 0.0f, 0.0f }, { tv.x, tv.y }, origin, scalePx,
                IM_COL32(255, 200, 80, 255), 3.0f);
        }
    }
    else {
        std::vector<a2::Vec2> original = getShapeByMode(m_a2ShapeMode);
        std::vector<a2::Vec2> transformed = transformShape(M, original);

        if (m_showOriginal) {
            drawPolyline(dl, original, origin, scalePx, IM_COL32(170, 170, 180, 255), true, 2.0f);
        }
        if (m_showTransformed) {
            drawPolyline(dl, transformed, origin, scalePx, IM_COL32(80, 180, 255, 255), true, 3.0f);
        }
    }

    ImGui::Dummy(avail);
    ImGui::End();
}

void App::drawAssignment3UI() {
    using namespace a3;

    const ImVec2 bigBtn(260, 0);

    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430, 900), ImGuiCond_FirstUseEver);
    ImGui::Begin("Assignment 3 - Controls", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

    if (ImGui::Button("< Back to Launcher", bigBtn)) {
        m_mode = ToolMode::Launcher;
        setStatus("Launcher: choose an assignment tool.");
        ImGui::End();
        return;
    }

    ImGui::Separator();
    if (!m_status.empty()) ImGui::TextWrapped("%s", m_status.c_str());
    ImGui::Separator();

    int obj = static_cast<int>(m_a3ObjectMode);
    const char* objNames[] = { "Cube", "Pyramid" };
    ImGui::Combo("3D Object", &obj, objNames, IM_ARRAYSIZE(objNames));
    m_a3ObjectMode = static_cast<Assignment3ObjectMode>(obj);

    int proj = static_cast<int>(m_a3ProjectionMode);
    const char* projNames[] = { "Orthographic", "Perspective" };
    ImGui::Combo("Projection", &proj, projNames, IM_ARRAYSIZE(projNames));
    m_a3ProjectionMode = static_cast<Assignment3ProjectionMode>(proj);

    ImGui::Separator();
    ImGui::Text("Model Transform");
    ImGui::SliderFloat("Tx", &m_a3Tx, -5.0f, 5.0f);
    ImGui::SliderFloat("Ty", &m_a3Ty, -5.0f, 5.0f);
    ImGui::SliderFloat("Tz", &m_a3Tz, -5.0f, 5.0f);

    ImGui::SliderFloat("Rx", &m_a3RxDeg, -180.0f, 180.0f);
    ImGui::SliderFloat("Ry", &m_a3RyDeg, -180.0f, 180.0f);
    ImGui::SliderFloat("Rz", &m_a3RzDeg, -180.0f, 180.0f);

    ImGui::SliderFloat("Sx", &m_a3Sx, 0.1f, 3.0f);
    ImGui::SliderFloat("Sy", &m_a3Sy, 0.1f, 3.0f);
    ImGui::SliderFloat("Sz", &m_a3Sz, 0.1f, 3.0f);

    ImGui::Separator();
    ImGui::Text("Camera");
    ImGui::SliderFloat("Eye X", &m_a3EyeX, -10.0f, 10.0f);
    ImGui::SliderFloat("Eye Y", &m_a3EyeY, -10.0f, 10.0f);
    ImGui::SliderFloat("Eye Z", &m_a3EyeZ, -10.0f, 10.0f);

    ImGui::SliderFloat("Target X", &m_a3TargetX, -5.0f, 5.0f);
    ImGui::SliderFloat("Target Y", &m_a3TargetY, -5.0f, 5.0f);
    ImGui::SliderFloat("Target Z", &m_a3TargetZ, -5.0f, 5.0f);

    if (m_a3ProjectionMode == Assignment3ProjectionMode::Perspective) {
        ImGui::SliderFloat("FOV Y", &m_a3FovYDeg, 20.0f, 120.0f);
    }
    else {
        ImGui::SliderFloat("Ortho Scale", &m_a3OrthoScale, 1.0f, 10.0f);
    }

    ImGui::SliderFloat("Near", &m_a3Near, 0.05f, 5.0f);
    ImGui::SliderFloat("Far", &m_a3Far, 5.0f, 100.0f);

    ImGui::Separator();
    ImGui::Checkbox("Show Axes", &m_a3ShowAxes);
    ImGui::Checkbox("Show Original", &m_a3ShowOriginal);
    ImGui::Checkbox("Show Transformed", &m_a3ShowTransformed);
    ImGui::Checkbox("Show Pipeline for Vertex 0", &m_a3ShowPipeline);

    if (ImGui::Button("Reset##a3", bigBtn)) {
        m_a3ObjectMode = Assignment3ObjectMode::Cube;
        m_a3ProjectionMode = Assignment3ProjectionMode::Perspective;

        m_a3Tx = 0.0f;
        m_a3Ty = 0.0f;
        m_a3Tz = 0.0f;

        m_a3RxDeg = 0.0f;
        m_a3RyDeg = 0.0f;
        m_a3RzDeg = 0.0f;

        m_a3Sx = 1.0f;
        m_a3Sy = 1.0f;
        m_a3Sz = 1.0f;

        m_a3EyeX = 4.0f;
        m_a3EyeY = 3.0f;
        m_a3EyeZ = 6.0f;

        m_a3TargetX = 0.0f;
        m_a3TargetY = 0.0f;
        m_a3TargetZ = 0.0f;

        m_a3FovYDeg = 60.0f;
        m_a3Near = 0.1f;
        m_a3Far = 50.0f;
        m_a3OrthoScale = 4.0f;

        m_a3ShowAxes = true;
        m_a3ShowOriginal = true;
        m_a3ShowTransformed = true;
        m_a3ShowPipeline = true;
    }

    const Mat4 M = buildModelTransform(
        m_a3Tx, m_a3Ty, m_a3Tz,
        m_a3RxDeg, m_a3RyDeg, m_a3RzDeg,
        m_a3Sx, m_a3Sy, m_a3Sz);

    const Vec3 eye{ m_a3EyeX, m_a3EyeY, m_a3EyeZ };
    const Vec3 target{ m_a3TargetX, m_a3TargetY, m_a3TargetZ };
    const Vec3 up{ 0.0f, 1.0f, 0.0f };

    const Mat4 V = makeLookAt(eye, target, up);

    ImGui::Separator();
    ImGui::Text("Model Matrix");
    for (int r = 0; r < 4; ++r) {
        ImGui::Text("[ %7.3f %7.3f %7.3f %7.3f ]", M.m[r][0], M.m[r][1], M.m[r][2], M.m[r][3]);
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(470, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1090, 900), ImGuiCond_FirstUseEver);
    ImGui::Begin("Assignment 3 - Canvas", nullptr, ImGuiWindowFlags_NoCollapse);

    ImVec2 pmin = ImGui::GetCursorScreenPos();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 50.0f) avail.x = 50.0f;
    if (avail.y < 50.0f) avail.y = 50.0f;
    ImVec2 pmax(pmin.x + avail.x, pmin.y + avail.y);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pmin, pmax, IM_COL32(24, 24, 28, 255));
    dl->AddRect(pmin, pmax, IM_COL32(90, 90, 100, 255));

    const float aspect = avail.x / avail.y;
    Mat4 P = (m_a3ProjectionMode == Assignment3ProjectionMode::Perspective)
        ? makePerspective(m_a3FovYDeg, aspect, m_a3Near, m_a3Far)
        : makeOrtho(-m_a3OrthoScale * aspect, m_a3OrthoScale * aspect,
            -m_a3OrthoScale, m_a3OrthoScale,
            m_a3Near, m_a3Far);

    std::vector<Vec3> verts = (m_a3ObjectMode == Assignment3ObjectMode::Cube)
        ? makeCubeVertices()
        : makePyramidVertices();

    std::vector<std::array<int, 2>> edges = (m_a3ObjectMode == Assignment3ObjectMode::Cube)
        ? makeCubeEdges()
        : makePyramidEdges();

    auto projectPoint = [&](const Vec3& v, bool transformed) -> ImVec2 {
        Vec4 pObj = makePoint(v.x, v.y, v.z);
        Vec4 pWorld = transformed ? transform(M, pObj) : pObj;
        Vec4 pView = transform(V, pWorld);
        Vec4 pClip = transform(P, pView);
        Vec3 pNdc = perspectiveDivide(pClip);
        return ndcToScreen(pNdc, pmin, pmax);
        };

    if (m_a3ShowAxes) {
        auto projectAxisPoint = [&](const Vec3& v) -> ImVec2 {
            Vec4 pObj = makePoint(v.x, v.y, v.z);
            Vec4 pView = transform(V, pObj);
            Vec4 pClip = transform(P, pView);
            Vec3 pNdc = perspectiveDivide(pClip);
            return ndcToScreen(pNdc, pmin, pmax);
            };

        dl->AddLine(projectAxisPoint({ 0.f, 0.f, 0.f }), projectAxisPoint({ 2.f, 0.f, 0.f }), IM_COL32(235, 85, 85, 255), 2.0f);
        dl->AddLine(projectAxisPoint({ 0.f, 0.f, 0.f }), projectAxisPoint({ 0.f, 2.f, 0.f }), IM_COL32(80, 210, 120, 255), 2.0f);
        dl->AddLine(projectAxisPoint({ 0.f, 0.f, 0.f }), projectAxisPoint({ 0.f, 0.f, 2.f }), IM_COL32(80, 160, 255, 255), 2.0f);
    }

    if (m_a3ShowOriginal) {
        for (const auto& e : edges) {
            dl->AddLine(projectPoint(verts[e[0]], false),
                projectPoint(verts[e[1]], false),
                IM_COL32(170, 170, 180, 255), 2.0f);
        }
    }

    if (m_a3ShowTransformed) {
        for (const auto& e : edges) {
            dl->AddLine(projectPoint(verts[e[0]], true),
                projectPoint(verts[e[1]], true),
                IM_COL32(80, 180, 255, 255), 3.0f);
        }
    }

    if (m_a3ShowPipeline && !verts.empty()) {
        const Vec3 v0 = verts[0];

        Vec4 pObj = makePoint(v0.x, v0.y, v0.z);
        Vec4 pWorld = transform(M, pObj);
        Vec4 pView = transform(V, pWorld);
        Vec4 pClip = transform(P, pView);
        Vec3 pNdc = perspectiveDivide(pClip);
        ImVec2 pScreen = ndcToScreen(pNdc, pmin, pmax);

        ImGui::SetCursorScreenPos(ImVec2(pmin.x + 12.0f, pmin.y + 12.0f));
        ImGui::BeginChild("A3PipelineInfo", ImVec2(430.0f, 185.0f), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::Text("Vertex 0 Coordinate Pipeline");
        ImGui::Separator();
        ImGui::Text("Object : (%.3f, %.3f, %.3f, %.3f)", pObj.x, pObj.y, pObj.z, pObj.w);
        ImGui::Text("World  : (%.3f, %.3f, %.3f, %.3f)", pWorld.x, pWorld.y, pWorld.z, pWorld.w);
        ImGui::Text("View   : (%.3f, %.3f, %.3f, %.3f)", pView.x, pView.y, pView.z, pView.w);
        ImGui::Text("Clip   : (%.3f, %.3f, %.3f, %.3f)", pClip.x, pClip.y, pClip.z, pClip.w);
        ImGui::Text("NDC    : (%.3f, %.3f, %.3f)", pNdc.x, pNdc.y, pNdc.z);
        ImGui::Text("Screen : (%.1f, %.1f)", pScreen.x, pScreen.y);
        ImGui::EndChild();
    }

    ImGui::Dummy(avail);
    ImGui::End();
}



void App::drawAssignment4UI() {
    const ImVec2 bigBtn(240, 0);

    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(470, 760), ImGuiCond_FirstUseEver);
    ImGui::Begin("Assignment 4 - OpenGL Graphics Pipeline", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

    if (ImGui::Button("< Back to Launcher", bigBtn)) {
        if (m_a4Initialized) {
            m_a4Renderer.shutdown();
            m_a4Initialized = false;
        }
        m_mode = ToolMode::Launcher;
        setStatus("Launcher: choose an assignment tool.");
        ImGui::End();
        return;
    }

    ImGui::Separator();
    if (!m_status.empty()) ImGui::TextWrapped("%s", m_status.c_str());
    ImGui::Separator();

    ImGui::TextWrapped("This assignment uses real OpenGL/GPU rendering to demonstrate the graphics pipeline.");

    int renderMode = static_cast<int>(m_a4RenderMode);
    ImGui::RadioButton("1 Points", &renderMode, static_cast<int>(Assignment4Renderer::RenderMode::Points));
    ImGui::RadioButton("2 Primitive / Triangle View", &renderMode, static_cast<int>(Assignment4Renderer::RenderMode::PrimitiveTriangle));
    ImGui::RadioButton("3 Filled (No Depth)", &renderMode, static_cast<int>(Assignment4Renderer::RenderMode::FilledNoDepth));
    ImGui::RadioButton("4 Filled (Depth Test)", &renderMode, static_cast<int>(Assignment4Renderer::RenderMode::FilledDepth));
    m_a4RenderMode = static_cast<Assignment4Renderer::RenderMode>(renderMode);

    ImGui::Separator();
    ImGui::Checkbox("Rotate objects", &m_a4Rotate);
    ImGui::Checkbox("Show help / learning notes", &m_a4ShowHelp);

    if (ImGui::Button("Reset to default mode", bigBtn)) {
        m_a4RenderMode = Assignment4Renderer::RenderMode::FilledDepth;
        m_a4Rotate = true;
        m_a4ShowHelp = true;
    }

    if (m_a4ShowHelp) {
        ImGui::Separator();
        ImGui::TextWrapped("Pipeline connection:");
        ImGui::BulletText("CPU scene data: cube, pyramid, ground plane");
        ImGui::BulletText("GPU upload: VAO, VBO, EBO");
        ImGui::BulletText("Vertex processing: model-view-projection in the vertex shader");
        ImGui::BulletText("Primitive assembly: vertices form triangles");
        ImGui::BulletText("Fragment processing: color output or depth visualization");
        ImGui::BulletText("Framebuffer output: final image after optional depth testing");
        ImGui::Separator();
        ImGui::TextWrapped("Keyboard shortcuts: 1-4 switch modes, R toggles rotation, Esc quits.");
    }

    ImGui::End();

    if (ImGui::IsKeyPressed(ImGuiKey_1)) m_a4RenderMode = Assignment4Renderer::RenderMode::Points;
    if (ImGui::IsKeyPressed(ImGuiKey_2)) m_a4RenderMode = Assignment4Renderer::RenderMode::PrimitiveTriangle;
    if (ImGui::IsKeyPressed(ImGuiKey_3)) m_a4RenderMode = Assignment4Renderer::RenderMode::FilledNoDepth;
    if (ImGui::IsKeyPressed(ImGuiKey_4)) m_a4RenderMode = Assignment4Renderer::RenderMode::FilledDepth;
    if (ImGui::IsKeyPressed(ImGuiKey_R)) m_a4Rotate = !m_a4Rotate;
}

void App::drawAssignment7UI() {
    using namespace a2;

    const ImVec2 bigBtn(260, 0);

    if (m_a7Animate) {
        m_a7ParamT += ImGui::GetIO().DeltaTime * m_a7AnimationSpeed;
        if (m_a7LoopAnimation) {
            while (m_a7ParamT > 1.0f) m_a7ParamT -= 1.0f;
        } else if (m_a7ParamT > 1.0f) {
            m_a7ParamT = 1.0f;
            m_a7Animate = false;
        }
    }
    m_a7ParamT = std::clamp(m_a7ParamT, 0.0f, 1.0f);
    m_a7StepSize = std::clamp(m_a7StepSize, 0.01f, 0.5f);

    auto applyPreset = [&]() {
        if (m_a7CurveMode == CurveMode::Bezier) {
            if (m_a7BezierMode == BezierMode::Linear) {
                if (m_a7Preset == 0)      m_a7ControlPoints = { Vec2{-6.0f,-2.0f}, Vec2{6.0f,2.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
                else if (m_a7Preset == 1) m_a7ControlPoints = { Vec2{-6.0f,-1.5f}, Vec2{6.0f,-1.5f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
                else if (m_a7Preset == 2) m_a7ControlPoints = { Vec2{-6.0f,-1.0f}, Vec2{6.0f,1.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
                else                      m_a7ControlPoints = { Vec2{-6.0f,2.5f}, Vec2{6.0f,-2.5f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
            } else if (m_a7BezierMode == BezierMode::Quadratic) {
                if (m_a7Preset == 0)      m_a7ControlPoints = { Vec2{-6.0f,-2.0f}, Vec2{0.0f,4.5f}, Vec2{6.0f,2.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
                else if (m_a7Preset == 1) m_a7ControlPoints = { Vec2{-6.0f,-1.5f}, Vec2{0.0f,5.0f}, Vec2{6.0f,-1.5f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
                else if (m_a7Preset == 2) m_a7ControlPoints = { Vec2{-6.0f,-1.0f}, Vec2{0.0f,-0.1f}, Vec2{6.0f,1.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
                else                      m_a7ControlPoints = { Vec2{-6.0f,0.0f}, Vec2{0.0f,-5.5f}, Vec2{6.0f,0.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
            } else if (m_a7BezierMode == BezierMode::Cubic) {
                if (m_a7Preset == 0)      m_a7ControlPoints = { Vec2{-6.0f,-2.0f}, Vec2{-3.0f,3.5f}, Vec2{2.0f,-3.5f}, Vec2{6.0f,2.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
                else if (m_a7Preset == 1) m_a7ControlPoints = { Vec2{-6.0f,-1.5f}, Vec2{-2.5f,4.5f}, Vec2{2.5f,4.5f}, Vec2{6.0f,-1.5f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
                else if (m_a7Preset == 2) m_a7ControlPoints = { Vec2{-6.0f,-1.0f}, Vec2{-2.0f,-0.2f}, Vec2{2.0f,0.2f}, Vec2{6.0f,1.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
                else                      m_a7ControlPoints = { Vec2{-6.0f,0.0f}, Vec2{2.0f,5.0f}, Vec2{-2.0f,-5.0f}, Vec2{6.0f,0.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
            } else {
                if (m_a7Preset == 0)      { m_a7ControlPoints = { Vec2{-7.0f,-1.5f}, Vec2{-5.0f,2.0f}, Vec2{-3.0f,3.8f}, Vec2{-1.0f,-1.2f}, Vec2{1.0f,-3.8f}, Vec2{3.0f,2.0f}, Vec2{5.0f,3.6f}, Vec2{7.0f,-0.5f}, Vec2{0,0}, Vec2{0,0} }; m_a7GeneralPointCount = 8; }
                else if (m_a7Preset == 1) { m_a7ControlPoints = { Vec2{-7.0f,0.0f}, Vec2{-5.0f,4.0f}, Vec2{-3.0f,1.0f}, Vec2{-1.0f,-2.0f}, Vec2{1.0f,-2.0f}, Vec2{3.0f,1.0f}, Vec2{5.0f,4.0f}, Vec2{7.0f,0.0f}, Vec2{0,0}, Vec2{0,0} }; m_a7GeneralPointCount = 8; }
                else if (m_a7Preset == 2) { m_a7ControlPoints = { Vec2{-7.0f,-1.0f}, Vec2{-5.0f,-0.2f}, Vec2{-3.0f,0.3f}, Vec2{-1.0f,0.6f}, Vec2{1.0f,0.2f}, Vec2{3.0f,-0.2f}, Vec2{5.0f,0.4f}, Vec2{7.0f,1.0f}, Vec2{0,0}, Vec2{0,0} }; m_a7GeneralPointCount = 8; }
                else                      { m_a7ControlPoints = { Vec2{-7.0f,2.0f}, Vec2{-5.5f,-3.0f}, Vec2{-3.0f,4.0f}, Vec2{-1.0f,-2.0f}, Vec2{1.0f,3.5f}, Vec2{3.0f,-4.0f}, Vec2{5.0f,2.5f}, Vec2{7.0f,-1.0f}, Vec2{0,0}, Vec2{0,0} }; m_a7GeneralPointCount = 8; }
            }
        } else if (m_a7CurveMode == CurveMode::Hermite) {
            if (m_a7Preset == 0)      m_a7ControlPoints = { Vec2{-6.0f,-2.0f}, Vec2{-2.0f,3.0f}, Vec2{2.0f,-3.0f}, Vec2{6.0f,2.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
            else if (m_a7Preset == 1) m_a7ControlPoints = { Vec2{-6.0f,-1.5f}, Vec2{-2.5f,4.0f}, Vec2{2.5f,4.0f}, Vec2{6.0f,-1.5f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
            else if (m_a7Preset == 2) m_a7ControlPoints = { Vec2{-6.0f,-1.0f}, Vec2{-1.0f,-0.2f}, Vec2{1.0f,0.2f}, Vec2{6.0f,1.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
            else                      m_a7ControlPoints = { Vec2{-6.0f,0.0f}, Vec2{-6.0f,5.0f}, Vec2{6.0f,-5.0f}, Vec2{6.0f,0.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
        } else if (m_a7CurveMode == CurveMode::PiecewiseBezier) {
            if (m_a7Preset == 0)      m_a7ControlPoints = { Vec2{-7.0f,-2.0f}, Vec2{-5.0f,3.0f}, Vec2{-2.0f,2.0f}, Vec2{0.0f,0.0f}, Vec2{2.0f,-2.0f}, Vec2{5.0f,3.0f}, Vec2{7.0f,1.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
            else if (m_a7Preset == 1) m_a7ControlPoints = { Vec2{-7.0f,-1.5f}, Vec2{-5.5f,4.0f}, Vec2{-2.5f,4.0f}, Vec2{0.0f,-0.5f}, Vec2{2.5f,-5.0f}, Vec2{5.0f,2.0f}, Vec2{7.0f,0.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
            else if (m_a7Preset == 2) m_a7ControlPoints = { Vec2{-7.0f,-1.0f}, Vec2{-5.0f,-0.2f}, Vec2{-2.5f,0.2f}, Vec2{0.0f,0.6f}, Vec2{2.5f,1.0f}, Vec2{5.0f,0.8f}, Vec2{7.0f,1.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
            else                      m_a7ControlPoints = { Vec2{-7.0f,0.0f}, Vec2{-4.0f,5.0f}, Vec2{-1.0f,-4.0f}, Vec2{1.0f,0.0f}, Vec2{3.0f,4.0f}, Vec2{5.0f,-2.0f}, Vec2{7.0f,1.0f}, Vec2{0,0}, Vec2{0,0}, Vec2{0,0} };
            if (m_a7PiecewiseEnforceC1) enforcePiecewiseC1(m_a7ControlPoints);
        } else {
            if (m_a7Preset == 0)      { m_a7ControlPoints = { Vec2{-7.0f,-1.0f}, Vec2{-5.5f,3.0f}, Vec2{-3.0f,4.0f}, Vec2{-1.0f,0.0f}, Vec2{1.0f,-3.5f}, Vec2{3.5f,-2.0f}, Vec2{5.5f,2.5f}, Vec2{7.0f,1.0f}, Vec2{0,0}, Vec2{0,0} }; m_a7GeneralPointCount = 8; }
            else if (m_a7Preset == 1) { m_a7ControlPoints = { Vec2{-7.0f,0.0f}, Vec2{-5.0f,4.0f}, Vec2{-3.0f,4.5f}, Vec2{-1.0f,0.5f}, Vec2{1.0f,-3.5f}, Vec2{3.0f,-3.0f}, Vec2{5.0f,1.0f}, Vec2{7.0f,0.0f}, Vec2{0,0}, Vec2{0,0} }; m_a7GeneralPointCount = 8; }
            else if (m_a7Preset == 2) { m_a7ControlPoints = { Vec2{-7.0f,-1.0f}, Vec2{-5.0f,-0.6f}, Vec2{-3.0f,0.3f}, Vec2{-1.0f,1.0f}, Vec2{1.0f,0.8f}, Vec2{3.0f,0.1f}, Vec2{5.0f,0.6f}, Vec2{7.0f,1.0f}, Vec2{0,0}, Vec2{0,0} }; m_a7GeneralPointCount = 8; }
            else                      { m_a7ControlPoints = { Vec2{-7.0f,1.0f}, Vec2{-5.5f,-3.5f}, Vec2{-3.0f,4.0f}, Vec2{-1.0f,-1.0f}, Vec2{1.0f,3.5f}, Vec2{3.0f,-4.0f}, Vec2{5.5f,3.0f}, Vec2{7.0f,-0.5f}, Vec2{0,0}, Vec2{0,0} }; m_a7GeneralPointCount = 8; }
        }
        m_a7DraggedPoint = -1;
    };

    auto modePointCount = [&]() -> int {
        if (m_a7CurveMode == CurveMode::Hermite) return 4;
        if (m_a7CurveMode == CurveMode::PiecewiseBezier) return 7;
        if (m_a7CurveMode == CurveMode::BSpline) return std::clamp(m_a7GeneralPointCount, 4, 10);
        return (m_a7BezierMode == BezierMode::General) ? std::clamp(m_a7GeneralPointCount, 2, 10) : ((m_a7BezierMode == BezierMode::Linear) ? 2 : ((m_a7BezierMode == BezierMode::Quadratic) ? 3 : 4));
    };

    const int pointCount = modePointCount();
    const std::vector<Vec2> controlPts = activeControlPoints(m_a7ControlPoints, pointCount);
    const float evalT = (m_a7CurveMode == CurveMode::Bezier && m_a7BezierMode == BezierMode::General && m_a7UseApproxArcLength)
        ? mapNormalizedArcToTGeneral(controlPts, m_a7ParamT)
        : m_a7ParamT;

    std::vector<Vec2> curvePts;
    std::vector<Vec2> lowCurvePts;
    std::vector<std::vector<Vec2>> levels;
    Vec2 pointOnCurve{0.0f, 0.0f};
    Vec2 tangent{0.0f, 0.0f};
    std::vector<float> weights;
    GeneralSubdivision split{};
    bool canSplit = false;
    bool piecewiseSecondSegment = false;

    if (m_a7CurveMode == CurveMode::Bezier) {
        curvePts = sampleBezierGeneral(controlPts, m_a7Samples);
        lowCurvePts = sampleBezierGeneral(controlPts, m_a7LowResSamples);
        levels = buildDeCasteljauLevelsGeneral(controlPts, evalT);
        pointOnCurve = levels.empty() ? Vec2{0.0f,0.0f} : levels.back().front();
        tangent = derivBezierGeneral(controlPts, evalT);
        weights = bernsteinWeights(pointCount - 1, evalT);
        split = splitBezierGeneral(controlPts, evalT);
        canSplit = (pointCount >= 2);
    } else if (m_a7CurveMode == CurveMode::Hermite) {
        std::array<Vec2,4> cp{m_a7ControlPoints[0], m_a7ControlPoints[1], m_a7ControlPoints[2], m_a7ControlPoints[3]};
        curvePts = sampleHermiteCurve(cp, m_a7Samples);
        lowCurvePts = sampleHermiteCurve(cp, m_a7LowResSamples);
        pointOnCurve = evalHermite(cp, evalT);
        tangent = derivHermite(cp, evalT);
    } else if (m_a7CurveMode == CurveMode::PiecewiseBezier) {
        if (m_a7PiecewiseEnforceC1) enforcePiecewiseC1(m_a7ControlPoints);
        curvePts = samplePiecewiseBezierCurve(m_a7ControlPoints, m_a7Samples);
        lowCurvePts = samplePiecewiseBezierCurve(m_a7ControlPoints, m_a7LowResSamples);
        pointOnCurve = evalPiecewiseBezier(m_a7ControlPoints, evalT);
        tangent = derivPiecewiseBezier(m_a7ControlPoints, evalT);
        auto state = buildPiecewiseConstruction(m_a7ControlPoints, evalT);
        levels = state.levels;
        piecewiseSecondSegment = state.secondSegment;
    } else {
        curvePts = sampleBSplineCurve(controlPts, 3, m_a7Samples);
        lowCurvePts = sampleBSplineCurve(controlPts, 3, m_a7LowResSamples);
        pointOnCurve = evalBSpline(controlPts, 3, evalT);
        tangent = derivBSpline(controlPts, 3, evalT);
        weights = bSplineBasisWeights(pointCount, 3, evalT);
    }
    const auto hull = convexHull(controlPts);

    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 940), ImGuiCond_FirstUseEver);
    ImGui::Begin("Assignment 7 - Curve Controls", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

    if (ImGui::Button("< Back to Launcher", bigBtn)) {
        m_mode = ToolMode::Launcher;
        setStatus("Launcher: choose an assignment tool.");
        ImGui::End();
        return;
    }

    ImGui::Separator();
    if (!m_status.empty()) ImGui::TextWrapped("%s", m_status.c_str());
    ImGui::Separator();
    ImGui::TextWrapped("This curve lab now supports Bezier, Hermite, piecewise Bezier, and B-spline modes. Use it to connect equations, geometry, and implementation.");

    int curveMode = static_cast<int>(m_a7CurveMode);
    ImGui::RadioButton("Bezier", &curveMode, static_cast<int>(CurveMode::Bezier)); ImGui::SameLine();
    ImGui::RadioButton("Hermite", &curveMode, static_cast<int>(CurveMode::Hermite)); ImGui::SameLine();
    ImGui::RadioButton("Piecewise Bezier", &curveMode, static_cast<int>(CurveMode::PiecewiseBezier)); ImGui::SameLine();
    ImGui::RadioButton("B-spline", &curveMode, static_cast<int>(CurveMode::BSpline));
    if (curveMode != static_cast<int>(m_a7CurveMode)) {
        m_a7CurveMode = static_cast<CurveMode>(curveMode);
        applyPreset();
    }

    if (m_a7CurveMode == CurveMode::Bezier) {
        int mode = static_cast<int>(m_a7BezierMode);
        ImGui::RadioButton("Linear (2 control points)", &mode, static_cast<int>(BezierMode::Linear));
        ImGui::RadioButton("Quadratic (3 control points)", &mode, static_cast<int>(BezierMode::Quadratic));
        ImGui::RadioButton("Cubic (4 control points)", &mode, static_cast<int>(BezierMode::Cubic));
        ImGui::RadioButton("General n-point (2 to 10)", &mode, static_cast<int>(BezierMode::General));
        m_a7BezierMode = static_cast<BezierMode>(mode);
        if (m_a7BezierMode == BezierMode::General) ImGui::SliderInt("General control-point count", &m_a7GeneralPointCount, 2, 10);
    } else if (m_a7CurveMode == CurveMode::PiecewiseBezier) {
        ImGui::Checkbox("Enforce C1 mirror handle at join", &m_a7PiecewiseEnforceC1);
    }

    const char* presets[] = { "Default", "Arch", "Near-line", "Expressive" };
    int oldPreset = m_a7Preset;
    ImGui::Combo("Preset", &m_a7Preset, presets, IM_ARRAYSIZE(presets));
    if (m_a7Preset != oldPreset) applyPreset();

    ImGui::Separator();
    ImGui::Text("Animation and Sampling");
    ImGui::SliderFloat((m_a7CurveMode == CurveMode::Bezier && m_a7BezierMode == BezierMode::General && m_a7UseApproxArcLength) ? "Normalized progress s" : "Parameter t", &m_a7ParamT, 0.0f, 1.0f, "%.3f");
    if (ImGui::Button(m_a7Animate ? "Pause" : "Play")) m_a7Animate = !m_a7Animate;
    ImGui::SameLine();
    if (ImGui::Button("Stop")) { m_a7Animate = false; m_a7ParamT = 0.0f; }
    ImGui::SameLine();
    if (ImGui::Button("Restart")) { m_a7ParamT = 0.0f; m_a7Animate = true; }
    ImGui::SameLine();
    if (ImGui::Button("< Step")) { m_a7Animate = false; m_a7ParamT = std::max(0.0f, m_a7ParamT - m_a7StepSize); }
    ImGui::SameLine();
    if (ImGui::Button("Step >")) { m_a7Animate = false; m_a7ParamT = std::min(1.0f, m_a7ParamT + m_a7StepSize); }
    ImGui::SliderFloat("Step size", &m_a7StepSize, 0.01f, 0.25f, "%.2f");
    ImGui::Checkbox("Loop animation", &m_a7LoopAnimation); ImGui::SameLine();
    ImGui::SliderFloat("Speed", &m_a7AnimationSpeed, 0.02f, 1.2f, "%.2f");
    ImGui::SliderInt("High-res samples", &m_a7Samples, 8, 300);
    ImGui::SliderInt("Low-res samples", &m_a7LowResSamples, 2, 80);
    if (m_a7CurveMode == CurveMode::Bezier && m_a7BezierMode == BezierMode::General) ImGui::Checkbox("Approximate equal-distance progress", &m_a7UseApproxArcLength);

    ImGui::Separator();
    ImGui::Text("Display");
    ImGui::Checkbox("Show curve", &m_a7ShowCurve); ImGui::SameLine();
    ImGui::Checkbox("Show low-res comparison", &m_a7ShowLowResCurve);
    ImGui::Checkbox("Show control polygon", &m_a7ShowControlPolygon); ImGui::SameLine();
    ImGui::Checkbox("Show control points", &m_a7ShowControlPoints);
    ImGui::Checkbox("Show point labels", &m_a7ShowPointLabels); ImGui::SameLine();
    ImGui::Checkbox("Show sample points", &m_a7ShowSamplePoints);
    ImGui::Checkbox("Show grid", &m_a7ShowGrid); ImGui::SameLine();
    ImGui::Checkbox("Show convex hull", &m_a7ShowConvexHull);
    ImGui::Checkbox("Show tangent at b(t)", &m_a7ShowTangent); ImGui::SameLine();
    ImGui::Checkbox("Show stage highlight", &m_a7ShowStageHighlight);
    ImGui::Checkbox("Show de Casteljau construction", &m_a7ShowConstruction);
    if (m_a7CurveMode != CurveMode::Hermite && m_a7CurveMode != CurveMode::BSpline) {
        ImGui::SameLine();
        ImGui::Checkbox("Step-by-step levels", &m_a7StepByStep);
        if (m_a7StepByStep) ImGui::SliderInt("Construction levels", &m_a7ConstructionLevel, 1, 9);
    }
    if (m_a7CurveMode == CurveMode::Bezier) {
        ImGui::Checkbox("Show subdivision at current t", &m_a7ShowSubdivision); ImGui::SameLine();
        ImGui::Checkbox("Show basis weights", &m_a7ShowBasisWeights);
    } else if (m_a7CurveMode == CurveMode::BSpline) {
        ImGui::Checkbox("Show basis weights", &m_a7ShowBasisWeights);
    }

    if (ImGui::Button("Reset Curve Demo", bigBtn)) {
        m_a7CurveMode = CurveMode::Bezier;
        m_a7BezierMode = BezierMode::Cubic;
        m_a7GeneralPointCount = 6;
        m_a7Samples = 64; m_a7LowResSamples = 12; m_a7ParamT = 0.35f; m_a7AnimationSpeed = 0.25f; m_a7Animate = true; m_a7LoopAnimation = true;
        m_a7ShowCurve = true; m_a7ShowControlPolygon = true; m_a7ShowControlPoints = true; m_a7ShowConstruction = true; m_a7ShowTangent = true;
        m_a7ShowPointLabels = true; m_a7ShowSamplePoints = false; m_a7ShowGrid = true; m_a7ShowConvexHull = false; m_a7ShowSubdivision = false;
        m_a7ShowLowResCurve = false; m_a7ShowBasisWeights = false; m_a7StepByStep = false; m_a7ConstructionLevel = 3; m_a7UseApproxArcLength = false;
        m_a7ShowStageHighlight = true; m_a7ShowModeSummary = true; m_a7ShowEquationPanel = true; m_a7StepSize = 0.1f; m_a7PiecewiseEnforceC1 = true;
        m_a7Preset = 0; m_a7DraggedPoint = -1;
        applyPreset();
    }

    ImGui::Separator();
    ImGui::Text("Current Teaching Notes");
    if (m_a7CurveMode == CurveMode::Bezier) {
        ImGui::BulletText("Bezier curves interpolate the first and last control points.");
        ImGui::BulletText("Interior control points shape the curve but usually are not on the curve.");
        ImGui::BulletText("de Casteljau evaluates one point using repeated linear interpolation.");
        ImGui::BulletText("General n-point Bezier shows why high degree becomes harder to control.");
        ImGui::BulletText("Subdivision shows one Bezier can split into two exact Bezier pieces.");
    } else if (m_a7CurveMode == CurveMode::Hermite) {
        ImGui::BulletText("Hermite curves use endpoints plus tangent vectors at those endpoints.");
        ImGui::BulletText("The tangent tips act like handles that control direction and strength.");
        ImGui::BulletText("Unlike Bezier, the main language here is endpoints plus tangents.");
    } else if (m_a7CurveMode == CurveMode::PiecewiseBezier) {
        ImGui::BulletText("Piecewise Bezier joins two cubic Bezier segments to build a longer shape.");
        ImGui::BulletText("The shared join point is where continuity matters most.");
        ImGui::BulletText("C1 continuity mirrors the handle across the join so tangent direction matches.");
    } else {
        ImGui::BulletText("B-splines are piecewise smooth curves with local control.");
        ImGui::BulletText("This mode uses an open uniform cubic B-spline with control polygon guidance.");
        ImGui::BulletText("Moving one control point usually affects only a nearby part of the curve.");
    }

    ImGui::Separator();
    ImGui::Text("Current Values");
    ImGui::Text("Displayed progress = %.3f", m_a7ParamT);
    if (m_a7CurveMode == CurveMode::Bezier && m_a7BezierMode == BezierMode::General && m_a7UseApproxArcLength) ImGui::Text("Mapped parameter t = %.3f", evalT);
    if (m_a7CurveMode == CurveMode::Bezier) ImGui::Text("degree = %d", pointCount - 1);
    if (m_a7CurveMode == CurveMode::BSpline) ImGui::Text("degree = 3 (cubic), control points = %d", pointCount);
    ImGui::Text("b(t) = (%.3f, %.3f)", pointOnCurve.x, pointOnCurve.y);
    ImGui::Text("tangent = (%.3f, %.3f)", tangent.x, tangent.y);

    if (m_a7ShowEquationPanel) {
        ImGui::Separator();
        ImGui::Text("Live equation guide");
        if (m_a7CurveMode == CurveMode::Bezier) {
            if (m_a7BezierMode == BezierMode::Linear) {
                ImGui::BulletText("b(t) = (1 - t) p0 + t p1");
                ImGui::TextWrapped("One interpolation on one segment.");
            } else if (m_a7BezierMode == BezierMode::Quadratic) {
                ImGui::BulletText("q0 = (1 - t) p0 + t p1");
                ImGui::BulletText("q1 = (1 - t) p1 + t p2");
                ImGui::BulletText("b(t) = (1 - t) q0 + t q1");
                ImGui::TextWrapped("Quadratic Bezier is interpolation of interpolations.");
            } else if (m_a7BezierMode == BezierMode::Cubic) {
                ImGui::BulletText("q0, q1, q2: interpolate neighboring control points");
                ImGui::BulletText("r0, r1: interpolate the q-points");
                ImGui::BulletText("b(t) = (1 - t) r0 + t r1");
                ImGui::TextWrapped("Cubic Bezier adds one more interpolation level for more flexibility.");
            } else {
                ImGui::BulletText("General n-point Bezier repeats linear interpolation until one point remains.");
                ImGui::TextWrapped("This mode is useful for showing why very high degree is harder to control than piecewise curves.");
            }
        } else if (m_a7CurveMode == CurveMode::Hermite) {
            ImGui::BulletText("Endpoints: p0 and p1");
            ImGui::BulletText("Tangents: m0 = t0Tip - p0,  m1 = t1Tip - p1");
            ImGui::BulletText("b(t) = h00 p0 + h10 m0 + h01 p1 + h11 m1");
            ImGui::TextWrapped("Hermite uses endpoints and tangent vectors instead of Bezier control-point blending.");
        } else if (m_a7CurveMode == CurveMode::PiecewiseBezier) {
            ImGui::BulletText("Segment 1: cubic from P0 to P3");
            ImGui::BulletText("Segment 2: cubic from P3 to P6");
            ImGui::BulletText("Shared point P3 is the join.");
            ImGui::TextWrapped(piecewiseSecondSegment ? "Current t is in the second segment." : "Current t is in the first segment.");
        } else {
            ImGui::BulletText("Open uniform cubic B-spline: b(t) = sum_i N_{i,3}(t) C_i");
            ImGui::BulletText("Each basis function N_{i,3}(t) is local, so only a few control points are active at a time.");
            ImGui::TextWrapped("Compared with one high-degree Bezier curve, B-splines are better for long shapes because they provide piecewise smoothness and local control.");
        }
    }

    if ((m_a7CurveMode == CurveMode::Bezier || m_a7CurveMode == CurveMode::BSpline) && m_a7ShowBasisWeights) {
        ImGui::Separator();
        ImGui::Text(m_a7CurveMode == CurveMode::BSpline ? "B-spline basis weights at current parameter" : "Bernstein basis weights at current parameter");
        for (int i = 0; i < static_cast<int>(weights.size()); ++i) {
            std::string label = "B" + std::to_string(i);
            ImGui::ProgressBar(weights[static_cast<size_t>(i)], ImVec2(-1.0f, 0.0f), label.c_str());
        }
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(530, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1030, 900), ImGuiCond_FirstUseEver);
    ImGui::Begin("Assignment 7 - Curve Canvas", nullptr, ImGuiWindowFlags_NoCollapse);

    ImVec2 canvasP0 = ImGui::GetCursorScreenPos();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 50.0f) avail.x = 50.0f;
    if (avail.y < 50.0f) avail.y = 50.0f;
    ImVec2 canvasP1(canvasP0.x + avail.x, canvasP0.y + avail.y);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(canvasP0, canvasP1, IM_COL32(24, 24, 28, 255));
    dl->AddRect(canvasP0, canvasP1, IM_COL32(90, 90, 100, 255));

    ImGui::InvisibleButton("BezierCanvas", avail, ImGuiButtonFlags_MouseButtonLeft);
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 origin((canvasP0.x + canvasP1.x) * 0.5f, (canvasP0.y + canvasP1.y) * 0.5f);
    const float scalePx = (std::min)(avail.x, avail.y) / 18.0f;

    if (m_a7ShowGrid) drawGrid(dl, canvasP0, canvasP1, origin, scalePx);

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        float bestDist = 36.0f;
        m_a7DraggedPoint = -1;
        for (int i = pointCount - 1; i >= 0; --i) {
            if (m_a7CurveMode == CurveMode::PiecewiseBezier && m_a7PiecewiseEnforceC1 && i == 4) continue;
            const ImVec2 sp = worldToScreen(m_a7ControlPoints[i], origin, scalePx);
            const float dx = mouse.x - sp.x;
            const float dy = mouse.y - sp.y;
            const float d = std::sqrt(dx * dx + dy * dy);
            if (d < bestDist) { bestDist = d; m_a7DraggedPoint = i; }
        }
    }
    if (m_a7DraggedPoint >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        m_a7ControlPoints[m_a7DraggedPoint] = screenToWorld(mouse, origin, scalePx);
        if (m_a7CurveMode == CurveMode::PiecewiseBezier && m_a7PiecewiseEnforceC1 && (m_a7DraggedPoint == 2 || m_a7DraggedPoint == 3)) enforcePiecewiseC1(m_a7ControlPoints);
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) m_a7DraggedPoint = -1;

    if (m_a7CurveMode == CurveMode::PiecewiseBezier && m_a7PiecewiseEnforceC1) enforcePiecewiseC1(m_a7ControlPoints);

    if (m_a7ShowConvexHull && hull.size() >= 3) {
        for (size_t i = 0; i < hull.size(); ++i) {
            const auto& a = hull[i];
            const auto& b = hull[(i + 1) % hull.size()];
            dl->AddLine(worldToScreen(a, origin, scalePx), worldToScreen(b, origin, scalePx), IM_COL32(180, 120, 255, 180), 1.5f);
        }
    }

    if (m_a7ShowControlPolygon) {
        if (m_a7CurveMode == CurveMode::Hermite) {
            drawOpenPolyline(dl, std::vector<Vec2>{m_a7ControlPoints[0], m_a7ControlPoints[1]}, origin, scalePx, IM_COL32(170, 170, 170, 255), 2.0f);
            drawOpenPolyline(dl, std::vector<Vec2>{m_a7ControlPoints[3], m_a7ControlPoints[2]}, origin, scalePx, IM_COL32(170, 170, 170, 255), 2.0f);
            drawOpenPolyline(dl, std::vector<Vec2>{m_a7ControlPoints[0], m_a7ControlPoints[3]}, origin, scalePx, IM_COL32(90, 90, 110, 255), 1.0f);
        } else if (m_a7CurveMode == CurveMode::PiecewiseBezier) {
            drawOpenPolyline(dl, std::vector<Vec2>{m_a7ControlPoints[0],m_a7ControlPoints[1],m_a7ControlPoints[2],m_a7ControlPoints[3]}, origin, scalePx, IM_COL32(150,150,160,255), 2.0f);
            drawOpenPolyline(dl, std::vector<Vec2>{m_a7ControlPoints[3],m_a7ControlPoints[4],m_a7ControlPoints[5],m_a7ControlPoints[6]}, origin, scalePx, IM_COL32(150,150,160,255), 2.0f);
        } else {
            drawOpenPolyline(dl, controlPts, origin, scalePx, IM_COL32(150,150,160,255), 2.0f);
        }
    }

    if (m_a7ShowLowResCurve) drawOpenPolyline(dl, lowCurvePts, origin, scalePx, IM_COL32(255, 110, 110, 255), 1.8f);
    if (m_a7ShowCurve) drawOpenPolyline(dl, curvePts, origin, scalePx, IM_COL32(80, 180, 255, 255), 3.0f);
    if (m_a7ShowSamplePoints) for (const Vec2& p : curvePts) drawPoint(dl, p, origin, scalePx, IM_COL32(120,220,255,180), 2.5f);

    if (m_a7CurveMode == CurveMode::Bezier && m_a7ShowSubdivision && canSplit && split.left.size() >= 2 && split.right.size() >= 2) {
        drawOpenPolyline(dl, split.left, origin, scalePx, IM_COL32(255, 160, 100, 255), 2.0f);
        drawOpenPolyline(dl, split.right, origin, scalePx, IM_COL32(120, 255, 160, 255), 2.0f);
    }

    if (m_a7ShowConstruction && !levels.empty()) {
        const int maxShow = m_a7StepByStep ? std::clamp(m_a7ConstructionLevel, 1, static_cast<int>(levels.size()) - 1) : static_cast<int>(levels.size()) - 1;
        static const ImU32 levelColors[10] = {
            IM_COL32(255,170,70,255), IM_COL32(150,255,120,255), IM_COL32(255,80,80,255), IM_COL32(120,220,255,255),
            IM_COL32(255,120,220,255), IM_COL32(220,220,120,255), IM_COL32(180,255,255,255), IM_COL32(255,180,180,255), IM_COL32(180,180,255,255), IM_COL32(255,255,255,255)
        };
        const int highlightedLevel = m_a7StepByStep ? maxShow : static_cast<int>(levels.size()) - 1;
        for (int level = 1; level <= maxShow && level < static_cast<int>(levels.size()); ++level) {
            const bool highlight = m_a7ShowStageHighlight && (level == highlightedLevel);
            const float thickness = highlight ? 3.5f : 2.0f;
            drawOpenPolyline(dl, levels[static_cast<size_t>(level)], origin, scalePx, levelColors[level - 1], thickness);
            for (const Vec2& p : levels[static_cast<size_t>(level)]) drawPoint(dl, p, origin, scalePx, levelColors[level - 1], highlight ? 5.5f : 4.0f);
        }
    }

    if (m_a7ShowControlPoints) {
        static const ImU32 colors[10] = {
            IM_COL32(255,120,120,255), IM_COL32(255,210,80,255), IM_COL32(120,255,120,255), IM_COL32(255,120,220,255), IM_COL32(120,220,255,255),
            IM_COL32(255,180,120,255), IM_COL32(180,255,180,255), IM_COL32(180,180,255,255), IM_COL32(255,180,255,255), IM_COL32(255,255,180,255)
        };
        for (int i = 0; i < pointCount; ++i) {
            const bool lockedMirror = (m_a7CurveMode == CurveMode::PiecewiseBezier && m_a7PiecewiseEnforceC1 && i == 4);
            drawPoint(dl, m_a7ControlPoints[i], origin, scalePx, lockedMirror ? IM_COL32(180,180,180,255) : colors[i % 10], (i == m_a7DraggedPoint ? 9.0f : 7.5f));
            if (m_a7ShowPointLabels) {
                const ImVec2 sp = worldToScreen(m_a7ControlPoints[i], origin, scalePx);
                std::string label;
                if (m_a7CurveMode == CurveMode::Hermite) {
                    static const char* hermiteLabels[4] = {"P0", "T0", "T1", "P1"};
                    label = hermiteLabels[i];
                } else if (m_a7CurveMode == CurveMode::PiecewiseBezier) {
                    label = "P" + std::to_string(i);
                    if (i == 3) label += " (join)";
                    if (lockedMirror) label += " C1";
                } else if (m_a7CurveMode == CurveMode::BSpline) {
                    label = "C" + std::to_string(i);
                } else {
                    label = "P" + std::to_string(i);
                }
                dl->AddText(ImVec2(sp.x + 8.0f, sp.y - 18.0f), IM_COL32(240, 240, 240, 255), label.c_str());
            }
        }
    }

    drawPoint(dl, pointOnCurve, origin, scalePx, IM_COL32(255,255,255,255), 6.5f);
    if (m_a7ShowPointLabels) {
        const ImVec2 sp = worldToScreen(pointOnCurve, origin, scalePx);
        dl->AddText(ImVec2(sp.x + 8.0f, sp.y + 6.0f), IM_COL32(255,255,255,255), "b(t)");
    }

    if (m_a7ShowTangent) {
        const float len = length(tangent);
        if (len > 1e-5f) {
            const Vec2 dir = mul(tangent, 1.0f / len);
            drawArrow(dl, sub(pointOnCurve, mul(dir, 1.2f)), add(pointOnCurve, mul(dir, 1.2f)), origin, scalePx, IM_COL32(255,255,120,255), 2.5f);
        }
        if (m_a7CurveMode == CurveMode::Bezier && m_a7BezierMode == BezierMode::Cubic) {
            drawArrow(dl, m_a7ControlPoints[0], add(m_a7ControlPoints[0], mul(sub(m_a7ControlPoints[1], m_a7ControlPoints[0]), 0.6f)), origin, scalePx, IM_COL32(180,255,180,255), 2.0f);
            drawArrow(dl, m_a7ControlPoints[3], add(m_a7ControlPoints[3], mul(sub(m_a7ControlPoints[3], m_a7ControlPoints[2]), 0.6f)), origin, scalePx, IM_COL32(180,255,180,255), 2.0f);
        }
    }

    ImGui::SetCursorScreenPos(ImVec2(canvasP0.x + 12.0f, canvasP1.y - 287.0f));
    ImGui::BeginChild("BezierLegend", ImVec2(450.0f, 275.0f), true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav);
    ImGui::Text("Curve Teaching Guide");
    ImGui::Separator();
    ImGui::BulletText("Gray lines: control polygon or tangent handles");
    ImGui::BulletText("Blue curve: higher-resolution sampled rendered curve");
    ImGui::BulletText("Red curve: lower-resolution approximation when enabled");
    ImGui::BulletText("Colored inner polygons: de Casteljau intermediate levels");
    ImGui::BulletText("White point: current evaluation point b(t)");
    ImGui::BulletText("Yellow arrow: tangent direction at b(t)");
    ImGui::BulletText("Purple outline: convex hull when enabled");
    if (m_a7CurveMode == CurveMode::Bezier) ImGui::BulletText("Orange/green split polygons: exact subdivision at current t");
    if (m_a7CurveMode == CurveMode::Bezier) {
        ImGui::TextWrapped("Suggested lecture flow: linear -> quadratic -> cubic -> drag points -> animate -> show de Casteljau -> compare low/high sampling -> enable general n-point mode.");
    } else if (m_a7CurveMode == CurveMode::Hermite) {
        ImGui::TextWrapped("Suggested lecture flow: move endpoints first, then change tangent handles to see how Hermite differs from Bezier control-point shaping.");
    } else if (m_a7CurveMode == CurveMode::PiecewiseBezier) {
        ImGui::TextWrapped("Suggested lecture flow: show the join, then enable C1 and drag the handle near the join to discuss smooth multi-segment curves.");
    } else {
        ImGui::TextWrapped("Suggested lecture flow: drag one control point and observe the local change. Compare this with a high-degree Bezier to discuss local control and piecewise smoothness.");
    }
    ImGui::EndChild();

    ImGui::Dummy(avail);
    ImGui::End();
}


void App::drawImagePanel() {
    ImGui::SetNextWindowPos(ImVec2(460, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1100, 840), ImGuiCond_FirstUseEver);
    ImGui::Begin("Preview", nullptr, ImGuiWindowFlags_NoCollapse);

    if (!m_out || !m_outTex) {
        ImGui::Text("No output image.");
        ImGui::End();
        return;
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float w = (float)m_outTexW;
    float h = (float)m_outTexH;
    if (w <= 0 || h <= 0) {
        ImGui::Text("Invalid texture.");
        ImGui::End();
        return;
    }

    float sx = avail.x / w;
    float sy = avail.y / h;
    float scale = (sx < sy) ? sx : sy;
    if (scale <= 0.0f) scale = 1.0f;
    ImGui::Image((ImTextureID)(intptr_t)m_outTex, ImVec2(w * scale, h * scale));
    ImGui::End();
}

void App::loadImage(const std::string& path, bool asForeground) {
    if (path.empty()) return;
    auto img = std::make_unique<Image>();
    if (!img->loadFromFile(path)) {
        setStatus(std::string("Failed to load: ") + path);
        return;
    }
    if (m_doLinearWorkflow) img->srgbToLinearInPlace();
    if (asForeground) {
        m_fg = std::move(img);
        setStatus(std::string("Loaded FG: ") + path);
    }
    else {
        m_bg = std::move(img);
        setStatus(std::string("Loaded BG: ") + path);
    }
    rebuildOutputTexture();
}

void App::saveImage(const std::string& path) {
    if (!m_out) {
        setStatus("Save failed: no output image.");
        return;
    }
    if (path.empty()) {
        setStatus("Save canceled (empty path).");
        return;
    }

    Image tmp = *m_out;
    if (m_doLinearWorkflow) tmp.linearToSrgbInPlace();
    if (!m_saveWithAlpha) tmp.forceOpaqueAlpha();
    if (m_stampName && !m_studentName.empty()) {
        int scale = 3;
        int pad = 10;
        int x = pad;
        int y = tmp.height() - pad - 7 * scale;
        tmp.drawText5x7(x, y, m_studentName, scale, 1.0f, 1.0f, 1.0f, 0.95f, true);
    }
    if (!tmp.saveToPng(path)) {
        setStatus(std::string("Failed to save: ") + path);
        return;
    }
    setStatus(std::string("Saved output: ") + path);
}

void App::rebuildOutputTexture() {
    if (!m_bg) return;
    m_out = std::make_unique<Image>(*m_bg);
    if (m_fg) {
        ops::CompositeParams params;
        params.opacityMultiplier = m_fgOpacity;
        params.mode = static_cast<ops::BlendMode>(m_blendMode);
        ops::compositeOverCentered(*m_out, *m_fg, params);
    }
    if (m_applyBrightness) ops::applyBrightness(*m_out, m_brightness);

    Image preview = *m_out;
    if (m_doLinearWorkflow) preview.linearToSrgbInPlace();
    if (m_showChecker) {
        Image under(preview.width(), preview.height(), 4);
        under.fillCheckerboard();
        under.alphaOverInPlace(preview);
        preview = under;
    }

    auto bytes = preview.toRGBA8();
    if (!m_outTex) glGenTextures(1, &m_outTex);
    glBindTexture(GL_TEXTURE_2D, m_outTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_outTexW = preview.width();
    m_outTexH = preview.height();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_outTexW, m_outTexH, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}