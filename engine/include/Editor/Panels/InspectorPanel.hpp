#ifndef PIIXELENGINE_INSPECTORPANEL_HPP
#define PIIXELENGINE_INSPECTORPANEL_HPP

#ifdef BUILD_WITH_EDITOR

#include "Components/Transform.hpp"

#include <entt/entt.hpp>

#include <functional>
#include <raylib.h>

#include "EditorPanel.hpp"

namespace PiiXeL {

class Engine;
class CommandHistory;
class AnimatorControllerEditorPanel;
class UUID;

class InspectorPanel : public EditorPanel {
public:
    InspectorPanel(Engine* engine, CommandHistory* commandHistory, entt::entity* selectedEntity, bool* inspectorLocked,
                   entt::entity* lockedEntity, UUID* selectedAssetUUID, std::string* selectedAssetPath,
                   AnimatorControllerEditorPanel* animatorControllerEditor, Transform* cachedTransform,
                   bool* isModifyingTransform, Texture2D* defaultWhiteTexture);

    void OnImGuiRender() override;
    const char* GetTitle() const override { return "Inspector"; }
    bool IsOpen() const override { return m_IsOpen; }
    void SetOpen(bool open) override { m_IsOpen = open; }

    void SetRenderEntityPickerCallback(std::function<bool(const char*, entt::entity*)> callback);
    void SetRenderAssetPickerCallback(std::function<bool(const char*, UUID*, const std::string&)> callback);

private:
    void RenderAssetInspector();
    void RenderEntityInspector();
    void RenderTransformComponent(entt::registry& registry, entt::entity entity);
    void RenderSpriteComponent(entt::registry& registry, entt::entity entity);
    void RenderScriptComponent(entt::registry& registry, entt::entity entity);
    void RenderAddComponentMenu(entt::registry& registry, entt::entity entity);

private:
    Engine* m_Engine;
    CommandHistory* m_CommandHistory;
    entt::entity* m_SelectedEntity;
    bool* m_InspectorLocked;
    entt::entity* m_LockedEntity;
    UUID* m_SelectedAssetUUID;
    std::string* m_SelectedAssetPath;
    AnimatorControllerEditorPanel* m_AnimatorControllerEditor;
    Transform* m_CachedTransform;
    bool* m_IsModifyingTransform;
    Texture2D* m_DefaultWhiteTexture;

    bool m_IsOpen{true};

    std::function<bool(const char*, entt::entity*)> m_RenderEntityPickerCallback;
    std::function<bool(const char*, UUID*, const std::string&)> m_RenderAssetPickerCallback;
};

} // namespace PiiXeL

#endif

#endif
