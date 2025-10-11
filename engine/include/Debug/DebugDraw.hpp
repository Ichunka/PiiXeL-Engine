#ifndef PIIXELENGINE_DEBUGDRAW_HPP
#define PIIXELENGINE_DEBUGDRAW_HPP

#include <raylib.h>
#include <vector>

namespace PiiXeL {

struct DebugRay {
    Vector2 start;
    Vector2 end;
    Color color;
    bool hit;
    Vector2 hitPoint;
};

class DebugDraw {
public:
    static DebugDraw& Instance();

    void DrawRay(Vector2 start, Vector2 end, Color color, bool hit = false, Vector2 hitPoint = {0.0f, 0.0f});
    void Clear();
    void Render();

    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

private:
    DebugDraw() = default;

    std::vector<DebugRay> m_Rays;
    bool m_Enabled{false};
};

} // namespace PiiXeL

#endif
