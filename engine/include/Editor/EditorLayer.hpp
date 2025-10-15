#ifndef PIIXELENGINE_EDITORLAYER_HPP
#define PIIXELENGINE_EDITORLAYER_HPP

#ifdef BUILD_WITH_EDITOR

#include "Components/Transform.hpp"
#include "Components/UUID.hpp"
#include "Debug/Profiler.hpp"
#include "Editor/Panels/ConsolePanel.hpp"

#include <entt/entt.hpp>

#include <imgui.h>
#include <memory>
#include <raylib.h>
#include <unordered_map>

namespace PiiXeL {

class Engine;
class Scene;
class BuildPanel;
class SpriteSheetEditorPanel;
class AnimationClipEditorPanel;
class AnimatorControllerEditorPanel;
class HierarchyPanel;
class InspectorPanel;
class ContentBrowserPanel;
class ProfilerPanel;
class GameViewportPanel;
class SceneViewportPanel;
class EditorSceneManager;
class EditorPanelManager;
class EditorCamera;
class EditorSelectionManager;
class EditorGizmoSystem;
class EditorStateManager;
class EditorCommandSystem;

class EditorLayer {
public:
    explicit EditorLayer(Engine* engine);
    ~EditorLayer();

    void OnUpdate(float deltaTime);
    void OnRender();
    void OnImGuiRender();

    [[nodiscard]] const Rectangle& GetViewportBounds() const { return m_ViewportBounds; }
    [[nodiscard]] bool IsViewportHovered() const { return m_ViewportHovered; }
    [[nodiscard]] bool IsViewportFocused() const { return m_ViewportFocused; }

private:
    void DeleteAssetWithPackage(const std::string& assetPath);

    void RenderMenuBar();
    void RenderToolbar();
    void RenderProjectSettings();

    void HandleGizmoInteraction();
    void HandleEntitySelection();
    void RenderGizmos();

    void OnPlayButtonPressed();
    void OnStopButtonPressed();

    void NewScene();
    void SaveScene();
    void SaveSceneAs();
    void LoadScene();

    entt::entity GetPrimaryCamera();

    entt::entity DuplicateEntity(entt::entity entity);
    void CopyEntity(entt::entity entity);
    void PasteEntity();

    bool RenderEntityPicker(const char* label, entt::entity* entity);
    bool RenderAssetPicker(const char* label, UUID* uuid, const std::string& assetType);

    void RestoreScriptPropertiesFromFile(const std::string& filepath);
    void UpdateAnimatorPreviewInEditMode();

private:
    Engine* m_Engine;
    RenderTexture2D m_ViewportTexture;
    RenderTexture2D m_GameViewportTexture;
    Texture2D m_DefaultWhiteTexture;
    Rectangle m_ViewportBounds{0, 0, 1920, 1080};
    bool m_ViewportHovered{false};
    bool m_ViewportFocused{false};

    ImVec2 m_ViewportPos{0.0f, 0.0f};
    ImVec2 m_ViewportSize{0.0f, 0.0f};

    bool m_IsDraggingEntity{false};

    bool m_ShowProjectSettings{false};

    ConsoleFilters m_ConsoleFilters;
    bool m_ConsoleAutoScroll{true};
    int m_ConsoleSelectedTab{0};
    std::vector<int> m_ConsoleSelectedLines;
    int m_ConsoleLastClickedLine{-1};

    entt::entity m_CopiedEntity{entt::null};

    bool m_ProfilerPaused{false};
    FrameSnapshot m_ProfilerPausedSnapshot{};
    int m_ProfilerSelectedFrame{-1};
    FrameSnapshot m_ProfilerSelectedFrameSnapshot{};
    float m_ProfilerFlameGraphZoom{1.0f};
    float m_ProfilerFlameGraphScroll{0.0f};

    std::unique_ptr<BuildPanel> m_BuildPanel;
    std::unique_ptr<SpriteSheetEditorPanel> m_SpriteSheetEditor;
    std::unique_ptr<AnimationClipEditorPanel> m_AnimationClipEditor;
    std::unique_ptr<AnimatorControllerEditorPanel> m_AnimatorControllerEditor;

    std::unique_ptr<HierarchyPanel> m_HierarchyPanel;
    std::unique_ptr<InspectorPanel> m_InspectorPanel;
    std::unique_ptr<ContentBrowserPanel> m_ContentBrowserPanel;
    std::unique_ptr<ConsolePanel> m_ConsolePanel;
    std::unique_ptr<ProfilerPanel> m_ProfilerPanel;
    std::unique_ptr<GameViewportPanel> m_GameViewportPanel;
    std::unique_ptr<SceneViewportPanel> m_SceneViewportPanel;

    std::unique_ptr<EditorSceneManager> m_SceneManager;
    std::unique_ptr<EditorPanelManager> m_PanelManager;
    std::unique_ptr<EditorCamera> m_EditorCamera;
    std::unique_ptr<EditorSelectionManager> m_SelectionManager;
    std::unique_ptr<EditorGizmoSystem> m_GizmoSystem;
    std::unique_ptr<EditorStateManager> m_StateManager;
    std::unique_ptr<EditorCommandSystem> m_CommandSystem;
};

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR

#endif // PIIXELENGINE_EDITORLAYER_HPP
