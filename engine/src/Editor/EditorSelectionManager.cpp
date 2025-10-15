#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorSelectionManager.hpp"

#include "Core/Engine.hpp"
#include "Core/Logger.hpp"
#include "Editor/EditorCamera.hpp"
#include "Scene/EntityFactory.hpp"
#include "Scene/Scene.hpp"
#include "Components/Transform.hpp"
#include "Components/Sprite.hpp"

#include <cmath>
#include <imgui.h>
#include <raylib.h>

namespace PiiXeL {

EditorSelectionManager::EditorSelectionManager() :
    m_SelectedEntity{entt::null}, m_InspectorLocked{false}, m_LockedEntity{entt::null}, m_SelectedAssetUUID{0},
    m_SelectedAssetPath{} {}

void EditorSelectionManager::HandleEntitySelection(Engine* engine, EditorCamera* editorCamera, float viewportPosX,
                                                   float viewportPosY, float viewportWidth, float viewportHeight,
                                                   bool isDragging) {
    if (isDragging || editorCamera->IsPanning()) {
        return;
    }

    if (!engine || !engine->GetActiveScene()) {
        return;
    }

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
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

    entt::entity clickedEntity = entt::null;
    float closestDistance = 99999.0f;

    registry.view<Transform, Sprite>().each([&](entt::entity entity, const Transform& transform, const Sprite& sprite) {
        if (!sprite.IsValid()) {
            return;
        }

        float halfW = sprite.sourceRect.width * transform.scale.x * 0.5f;
        float halfH = sprite.sourceRect.height * transform.scale.y * 0.5f;

        Vector2 localPos{mouseWorldPos.x - transform.position.x, mouseWorldPos.y - transform.position.y};

        float cosR = std::cos(-transform.rotation * DEG2RAD);
        float sinR = std::sin(-transform.rotation * DEG2RAD);
        Vector2 rotatedPos{localPos.x * cosR - localPos.y * sinR, localPos.x * sinR + localPos.y * cosR};

        if (std::abs(rotatedPos.x) <= halfW && std::abs(rotatedPos.y) <= halfH) {
            float distance = localPos.x * localPos.x + localPos.y * localPos.y;
            if (distance < closestDistance) {
                closestDistance = distance;
                clickedEntity = entity;
            }
        }
    });

    if (clickedEntity != entt::null) {
        m_SelectedEntity = clickedEntity;
        m_SelectedAssetUUID = UUID{0};
        m_SelectedAssetPath.clear();
    }
}

void EditorSelectionManager::ClearSelection() {
    m_SelectedEntity = entt::null;
    m_SelectedAssetUUID = UUID{0};
    m_SelectedAssetPath.clear();
}

entt::entity EditorSelectionManager::DuplicateEntity(Engine* engine, entt::entity entity) {
    return EntityFactory::DuplicateEntity(engine, entity);
}

void EditorSelectionManager::CopyEntity(Engine* engine, entt::entity entity) {
    if (!engine || !engine->GetActiveScene()) {
        return;
    }

    Scene* scene = engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    if (!registry.valid(entity)) {
        return;
    }

    m_CopiedEntity = entity;
    PX_LOG_INFO(EDITOR, "Entity copied to clipboard");
}

void EditorSelectionManager::PasteEntity(Engine* engine) {
    if (m_CopiedEntity == entt::null) {
        return;
    }

    if (!engine || !engine->GetActiveScene()) {
        return;
    }

    Scene* scene = engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    if (!registry.valid(m_CopiedEntity)) {
        m_CopiedEntity = entt::null;
        return;
    }

    entt::entity newEntity = DuplicateEntity(engine, m_CopiedEntity);
    m_SelectedEntity = newEntity;

    PX_LOG_INFO(EDITOR, "Entity pasted");
}

} // namespace PiiXeL

#endif
