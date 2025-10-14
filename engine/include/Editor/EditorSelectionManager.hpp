#pragma once

#ifdef BUILD_WITH_EDITOR

#include "Components/UUID.hpp"

#include <entt/entt.hpp>

#include <string>

namespace PiiXeL {

class Engine;
class EditorCamera;

class EditorSelectionManager {
public:
    EditorSelectionManager();

    void HandleEntitySelection(Engine* engine, EditorCamera* editorCamera, float viewportPosX, float viewportPosY,
                               float viewportWidth, float viewportHeight, bool isDragging);

    [[nodiscard]] entt::entity GetSelectedEntity() const { return m_SelectedEntity; }
    void SetSelectedEntity(entt::entity entity) { m_SelectedEntity = entity; }

    [[nodiscard]] bool IsInspectorLocked() const { return m_InspectorLocked; }
    void SetInspectorLocked(bool locked) { m_InspectorLocked = locked; }

    [[nodiscard]] entt::entity GetLockedEntity() const { return m_LockedEntity; }
    void SetLockedEntity(entt::entity entity) { m_LockedEntity = entity; }

    [[nodiscard]] UUID GetSelectedAssetUUID() const { return m_SelectedAssetUUID; }
    void SetSelectedAssetUUID(UUID uuid) { m_SelectedAssetUUID = uuid; }

    [[nodiscard]] const std::string& GetSelectedAssetPath() const { return m_SelectedAssetPath; }
    void SetSelectedAssetPath(const std::string& path) { m_SelectedAssetPath = path; }

    void ClearSelection();

    entt::entity* GetSelectedEntityPtr() { return &m_SelectedEntity; }
    bool* GetInspectorLockedPtr() { return &m_InspectorLocked; }
    entt::entity* GetLockedEntityPtr() { return &m_LockedEntity; }
    UUID* GetSelectedAssetUUIDPtr() { return &m_SelectedAssetUUID; }
    std::string* GetSelectedAssetPathPtr() { return &m_SelectedAssetPath; }

private:
    entt::entity m_SelectedEntity{entt::null};
    bool m_InspectorLocked{false};
    entt::entity m_LockedEntity{entt::null};
    UUID m_SelectedAssetUUID{0};
    std::string m_SelectedAssetPath;
};

} // namespace PiiXeL

#endif
