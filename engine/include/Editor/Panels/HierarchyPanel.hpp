#ifndef PIIXELENGINE_HIERARCHYPANEL_HPP
#define PIIXELENGINE_HIERARCHYPANEL_HPP

#ifdef BUILD_WITH_EDITOR

#include <entt/entt.hpp>

#include <functional>

#include "EditorPanel.hpp"

namespace PiiXeL {

class Engine;
class EditorCommandSystem;
class AnimatorControllerEditorPanel;
class UUID;

class HierarchyPanel : public EditorPanel {
public:
    HierarchyPanel(Engine* engine, EditorCommandSystem* commandSystem, entt::entity* selectedEntity, bool* inspectorLocked,
                   UUID* selectedAssetUUID, std::string* selectedAssetPath,
                   AnimatorControllerEditorPanel* animatorControllerEditor);

    void OnImGuiRender() override;
    const char* GetTitle() const override { return "Hierarchy"; }
    bool IsOpen() const override { return m_IsOpen; }
    void SetOpen(bool open) override { m_IsOpen = open; }

    void SetDuplicateEntityCallback(std::function<entt::entity(entt::entity)> callback);
    void SetCopyEntityCallback(std::function<void(entt::entity)> callback);

    bool IsDraggingEntity() const { return m_IsDraggingEntity; }

private:
    Engine* m_Engine;
    EditorCommandSystem* m_CommandSystem;
    entt::entity* m_SelectedEntity;
    bool* m_InspectorLocked;
    UUID* m_SelectedAssetUUID;
    std::string* m_SelectedAssetPath;
    AnimatorControllerEditorPanel* m_AnimatorControllerEditor;

    bool m_IsOpen{true};
    bool m_IsDraggingEntity{false};

    std::function<entt::entity(entt::entity)> m_DuplicateEntityCallback;
    std::function<void(entt::entity)> m_CopyEntityCallback;
};

} // namespace PiiXeL

#endif

#endif
