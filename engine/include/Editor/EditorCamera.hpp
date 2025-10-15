#pragma once

#ifdef BUILD_WITH_EDITOR

#include <raylib.h>

namespace PiiXeL {

class EditorCamera {
public:
    EditorCamera();

    void HandleInput(bool isViewportHovered, bool isViewportFocused);

    Camera2D GetCamera2D(float viewportWidth, float viewportHeight) const;
    Vector2 ScreenToWorld(Vector2 screenPos, float viewportWidth, float viewportHeight) const;

    [[nodiscard]] Vector2 GetPosition() const { return m_Position; }
    [[nodiscard]] float GetZoom() const { return m_Zoom; }

    void SetPosition(Vector2 position) { m_Position = position; }
    void SetZoom(float zoom) { m_Zoom = zoom; }

    [[nodiscard]] bool IsPanning() const { return m_IsPanning; }

private:
    Vector2 m_Position{0.0f, 0.0f};
    float m_Zoom{1.0f};
    Vector2 m_LastMousePos{0.0f, 0.0f};
    bool m_IsPanning{false};
};

} // namespace PiiXeL

#endif
