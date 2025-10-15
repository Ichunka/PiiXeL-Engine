#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorGizmoSystem.hpp"

#include "Components/Transform.hpp"
#include "Core/Engine.hpp"
#include "Editor/EditorCamera.hpp"
#include "Editor/EditorCommands.hpp"
#include "Editor/EditorCommandSystem.hpp"
#include "Scene/Scene.hpp"

#include <cmath>
#include <imgui.h>
#include <raylib.h>

namespace PiiXeL {

EditorGizmoSystem::EditorGizmoSystem() :
    m_GizmoMode{GizmoMode::Translate}, m_SelectedAxis{GizmoAxis::None}, m_IsDragging{false}, m_DragStartPos{0.0f, 0.0f},
    m_EntityStartPos{0.0f, 0.0f} {}

void EditorGizmoSystem::HandleGizmoInteraction(Engine* engine, EditorCamera* editorCamera, entt::entity selectedEntity,
                                               float viewportPosX, float viewportPosY, float viewportWidth,
                                               float viewportHeight, EditorCommandSystem* commandSystem) {
    if (!engine || !engine->GetActiveScene()) {
        return;
    }

    Scene* scene = engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    ImVec2 mouseImGui = ImGui::GetMousePos();
    Vector2 mouseViewportPos{mouseImGui.x - viewportPosX, mouseImGui.y - viewportPosY};

    if (mouseViewportPos.x < 0 || mouseViewportPos.y < 0 || mouseViewportPos.x > viewportWidth ||
        mouseViewportPos.y > viewportHeight)
    {
        return;
    }

    Vector2 mouseWorldPos = editorCamera->ScreenToWorld(mouseViewportPos, viewportWidth, viewportHeight);
    float cameraZoom = editorCamera->GetZoom();

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !editorCamera->IsPanning()) {
        if (selectedEntity != entt::null && registry.valid(selectedEntity) &&
            registry.all_of<Transform>(selectedEntity))
        {
            Transform& transform = registry.get<Transform>(selectedEntity);

            m_SelectedAxis = GizmoAxis::None;

            float threshold = 10.0f / cameraZoom;
            float arrowLength = 50.0f;

            float deltaX = mouseWorldPos.x - transform.position.x;
            float deltaY = mouseWorldPos.y - transform.position.y;

            if (m_GizmoMode == GizmoMode::Translate) {
                float centerRadius = 30.0f;
                float distanceSquared = deltaX * deltaX + deltaY * deltaY;

                if (std::abs(deltaY) < threshold && deltaX > 0 && deltaX < arrowLength) {
                    m_SelectedAxis = GizmoAxis::X;
                    m_IsDragging = true;
                    m_DragStartPos = mouseWorldPos;
                    m_EntityStartPos = transform.position;
                }
                else if (std::abs(deltaX) < threshold && deltaY > 0 && deltaY < arrowLength) {
                    m_SelectedAxis = GizmoAxis::Y;
                    m_IsDragging = true;
                    m_DragStartPos = mouseWorldPos;
                    m_EntityStartPos = transform.position;
                }
                else if (distanceSquared <= centerRadius * centerRadius) {
                    m_SelectedAxis = GizmoAxis::None;
                    m_IsDragging = true;
                    m_DragStartPos = mouseWorldPos;
                    m_EntityStartPos = transform.position;
                }
            }
            else if (m_GizmoMode == GizmoMode::Scale) {
                float scaleHandleSize = 8.0f / cameraZoom;
                float centerRadius = 30.0f;
                float distanceSquared = deltaX * deltaX + deltaY * deltaY;

                Vector2 xHandle{transform.position.x + arrowLength, transform.position.y};
                Vector2 yHandle{transform.position.x, transform.position.y + arrowLength};

                if (std::abs(mouseWorldPos.x - xHandle.x) < scaleHandleSize &&
                    std::abs(mouseWorldPos.y - xHandle.y) < scaleHandleSize)
                {
                    m_SelectedAxis = GizmoAxis::X;
                    m_IsDragging = true;
                    m_DragStartPos = mouseWorldPos;
                    m_EntityStartPos = Vector2{transform.scale.x, transform.scale.y};
                }
                else if (std::abs(mouseWorldPos.x - yHandle.x) < scaleHandleSize &&
                         std::abs(mouseWorldPos.y - yHandle.y) < scaleHandleSize)
                {
                    m_SelectedAxis = GizmoAxis::Y;
                    m_IsDragging = true;
                    m_DragStartPos = mouseWorldPos;
                    m_EntityStartPos = Vector2{transform.scale.x, transform.scale.y};
                }
                else if (distanceSquared <= centerRadius * centerRadius) {
                    m_SelectedAxis = GizmoAxis::None;
                    m_IsDragging = true;
                    m_DragStartPos = mouseWorldPos;
                    m_EntityStartPos = Vector2{transform.scale.x, transform.scale.y};
                }
            }
            else if (m_GizmoMode == GizmoMode::Rotate) {
                float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
                float rotateRadius = 60.0f;
                float rotateThickness = 10.0f / cameraZoom;

                if (std::abs(distance - rotateRadius) < rotateThickness) {
                    m_IsDragging = true;
                    m_DragStartPos = mouseWorldPos;
                    m_EntityStartPos.x = transform.rotation;
                }
            }
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (m_IsDragging && selectedEntity != entt::null && registry.valid(selectedEntity) &&
            registry.all_of<Transform>(selectedEntity))
        {
            Transform& currentTransform = registry.get<Transform>(selectedEntity);
            Transform oldTransform = currentTransform;

            if (m_GizmoMode == GizmoMode::Translate) {
                oldTransform.position = m_EntityStartPos;
            }
            else if (m_GizmoMode == GizmoMode::Scale) {
                oldTransform.scale.x = m_EntityStartPos.x;
                oldTransform.scale.y = m_EntityStartPos.y;
            }
            else if (m_GizmoMode == GizmoMode::Rotate) {
                oldTransform.rotation = m_EntityStartPos.x;
            }

            commandSystem->ExecuteCommand(
                std::make_unique<ModifyTransformCommand>(&registry, selectedEntity, oldTransform, currentTransform));
        }

        m_IsDragging = false;
        m_SelectedAxis = GizmoAxis::None;
    }

    if (m_IsDragging && selectedEntity != entt::null && registry.valid(selectedEntity) &&
        registry.all_of<Transform>(selectedEntity))
    {
        Transform& transform = registry.get<Transform>(selectedEntity);

        if (m_GizmoMode == GizmoMode::Translate) {
            Vector2 dragDelta{mouseWorldPos.x - m_DragStartPos.x, mouseWorldPos.y - m_DragStartPos.y};

            if (m_SelectedAxis == GizmoAxis::X) {
                transform.position.x = m_EntityStartPos.x + dragDelta.x;
            }
            else if (m_SelectedAxis == GizmoAxis::Y) {
                transform.position.y = m_EntityStartPos.y + dragDelta.y;
            }
            else {
                transform.position.x = m_EntityStartPos.x + dragDelta.x;
                transform.position.y = m_EntityStartPos.y + dragDelta.y;
            }
        }
        else if (m_GizmoMode == GizmoMode::Scale) {
            Vector2 dragDelta{mouseWorldPos.x - m_DragStartPos.x, mouseWorldPos.y - m_DragStartPos.y};

            float scaleDelta = (dragDelta.x + dragDelta.y) * 0.01f;

            if (m_SelectedAxis == GizmoAxis::X) {
                transform.scale.x = m_EntityStartPos.x + scaleDelta;
                if (transform.scale.x < 0.1f)
                    transform.scale.x = 0.1f;
            }
            else if (m_SelectedAxis == GizmoAxis::Y) {
                transform.scale.y = m_EntityStartPos.y + scaleDelta;
                if (transform.scale.y < 0.1f)
                    transform.scale.y = 0.1f;
            }
            else {
                transform.scale.x = m_EntityStartPos.x + scaleDelta;
                transform.scale.y = m_EntityStartPos.y + scaleDelta;
                if (transform.scale.x < 0.1f)
                    transform.scale.x = 0.1f;
                if (transform.scale.y < 0.1f)
                    transform.scale.y = 0.1f;
            }
        }
        else if (m_GizmoMode == GizmoMode::Rotate) {
            float deltaX = mouseWorldPos.x - transform.position.x;
            float deltaY = mouseWorldPos.y - transform.position.y;
            float currentAngle = std::atan2(deltaY, deltaX) * RAD2DEG;

            float startDeltaX = m_DragStartPos.x - transform.position.x;
            float startDeltaY = m_DragStartPos.y - transform.position.y;
            float startAngle = std::atan2(startDeltaY, startDeltaX) * RAD2DEG;

            float angleDelta = currentAngle - startAngle;
            transform.rotation = m_EntityStartPos.x + angleDelta;
        }
    }
}

void EditorGizmoSystem::RenderGizmos(Engine* engine, EditorCamera* editorCamera, entt::entity selectedEntity) {
    if (selectedEntity == entt::null || !engine || !engine->GetActiveScene()) {
        return;
    }

    Scene* scene = engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    if (!registry.valid(selectedEntity) || !registry.all_of<Transform>(selectedEntity)) {
        return;
    }

    const Transform& transform = registry.get<Transform>(selectedEntity);
    float cameraZoom = editorCamera->GetZoom();

    if (m_GizmoMode == GizmoMode::Translate) {
        DrawCircleV(transform.position, 5.0f / cameraZoom, Color{255, 200, 0, 255});

        Vector2 endX{transform.position.x + 50.0f, transform.position.y};
        DrawLineEx(transform.position, endX, 2.0f / cameraZoom, Color{255, 0, 0, 255});
        DrawCircleV(endX, 4.0f / cameraZoom, Color{255, 0, 0, 255});

        Vector2 endY{transform.position.x, transform.position.y + 50.0f};
        DrawLineEx(transform.position, endY, 2.0f / cameraZoom, Color{0, 255, 0, 255});
        DrawCircleV(endY, 4.0f / cameraZoom, Color{0, 255, 0, 255});
    }
    else if (m_GizmoMode == GizmoMode::Scale) {
        DrawCircleV(transform.position, 5.0f / cameraZoom, Color{255, 200, 0, 255});

        Vector2 endX{transform.position.x + 50.0f, transform.position.y};
        DrawLineEx(transform.position, endX, 2.0f / cameraZoom, Color{255, 0, 0, 255});
        float boxSize = 8.0f / cameraZoom;
        DrawRectangleV(Vector2{endX.x - boxSize, endX.y - boxSize}, Vector2{boxSize * 2.0f, boxSize * 2.0f},
                       Color{255, 0, 0, 255});

        Vector2 endY{transform.position.x, transform.position.y + 50.0f};
        DrawLineEx(transform.position, endY, 2.0f / cameraZoom, Color{0, 255, 0, 255});
        DrawRectangleV(Vector2{endY.x - boxSize, endY.y - boxSize}, Vector2{boxSize * 2.0f, boxSize * 2.0f},
                       Color{0, 255, 0, 255});
    }
    else if (m_GizmoMode == GizmoMode::Rotate) {
        float rotateRadius = 60.0f;
        DrawCircleLinesV(transform.position, rotateRadius, Color{100, 150, 255, 255});
        DrawCircleV(transform.position, 3.0f / cameraZoom, Color{255, 200, 0, 255});
    }
}

} // namespace PiiXeL

#endif
