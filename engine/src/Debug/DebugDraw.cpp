#include "Debug/DebugDraw.hpp"

namespace PiiXeL {

DebugDraw& DebugDraw::Instance() {
    static DebugDraw instance;
    return instance;
}

void DebugDraw::DrawRay(Vector2 start, Vector2 end, Color color, bool hit, Vector2 hitPoint) {
    if (!m_Enabled)
        return;

    m_Rays.push_back({start, end, color, hit, hitPoint});
}

void DebugDraw::Clear() {
    m_Rays.clear();
}

void DebugDraw::Render() {
    if (!m_Enabled)
        return;

    for (const DebugRay& ray : m_Rays)
    {
        DrawLineV(ray.start, ray.end, ray.color);

        DrawCircleV(ray.start, 3.0f, ray.color);

        if (ray.hit)
        {
            DrawCircleV(ray.hitPoint, 5.0f, RED);
            DrawCircleLines(static_cast<int>(ray.hitPoint.x), static_cast<int>(ray.hitPoint.y), 8.0f, RED);
        }
        else
        { DrawCircleV(ray.end, 3.0f, ray.color); }
    }
}

} // namespace PiiXeL
