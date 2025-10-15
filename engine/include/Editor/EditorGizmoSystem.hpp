#pragma once

#ifdef BUILD_WITH_EDITOR

#include <entt/entt.hpp>

#include <raylib.h>

namespace PiiXeL {

class Engine;
class EditorCamera;
class EditorCommandSystem;

enum class GizmoMode { Translate, Rotate, Scale };

enum class GizmoAxis { None, X, Y };

class EditorGizmoSystem {
public:
    EditorGizmoSystem();

    void HandleGizmoInteraction(Engine* engine, EditorCamera* editorCamera, entt::entity selectedEntity,
                                float viewportPosX, float viewportPosY, float viewportWidth, float viewportHeight,
                                EditorCommandSystem* commandSystem);

    void RenderGizmos(Engine* engine, EditorCamera* editorCamera, entt::entity selectedEntity);

    [[nodiscard]] GizmoMode GetGizmoMode() const { return m_GizmoMode; }
    void SetGizmoMode(GizmoMode mode) { m_GizmoMode = mode; }

    [[nodiscard]] bool IsDragging() const { return m_IsDragging; }

private:
    GizmoMode m_GizmoMode{GizmoMode::Translate};
    GizmoAxis m_SelectedAxis{GizmoAxis::None};
    bool m_IsDragging{false};
    Vector2 m_DragStartPos{0.0f, 0.0f};
    Vector2 m_EntityStartPos{0.0f, 0.0f};
};

} // namespace PiiXeL

#endif
