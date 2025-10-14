#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorCamera.hpp"

#include <raylib.h>

namespace PiiXeL {

EditorCamera::EditorCamera() : m_Position{0.0f, 0.0f}, m_Zoom{1.0f}, m_LastMousePos{0.0f, 0.0f}, m_IsPanning{false} {}

void EditorCamera::HandleInput(bool isViewportHovered, bool isViewportFocused) {
    if (!isViewportHovered || !isViewportFocused)
    { return; }

    Vector2 mousePos = GetMousePosition();
    float wheel = GetMouseWheelMove();

    if (wheel != 0.0f)
    {
        m_Zoom += wheel * 0.1f * m_Zoom;
        if (m_Zoom < 0.1f)
            m_Zoom = 0.1f;
        if (m_Zoom > 10.0f)
            m_Zoom = 10.0f;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
    {
        m_IsPanning = true;
        m_LastMousePos = mousePos;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE))
    { m_IsPanning = false; }

    if (m_IsPanning)
    {
        Vector2 delta{mousePos.x - m_LastMousePos.x, mousePos.y - m_LastMousePos.y};
        m_Position.x -= delta.x / m_Zoom;
        m_Position.y -= delta.y / m_Zoom;
        m_LastMousePos = mousePos;
    }
}

Camera2D EditorCamera::GetCamera2D(float viewportWidth, float viewportHeight) const {
    Camera2D camera{};
    camera.offset = Vector2{viewportWidth / 2.0f, viewportHeight / 2.0f};
    camera.target = m_Position;
    camera.rotation = 0.0f;
    camera.zoom = m_Zoom;
    return camera;
}

Vector2 EditorCamera::ScreenToWorld(Vector2 screenPos, float viewportWidth, float viewportHeight) const {
    Camera2D camera = GetCamera2D(viewportWidth, viewportHeight);
    Vector2 worldPos{};
    worldPos.x = (screenPos.x - camera.offset.x) / camera.zoom + camera.target.x;
    worldPos.y = (screenPos.y - camera.offset.y) / camera.zoom + camera.target.y;
    return worldPos;
}

} // namespace PiiXeL

#endif
