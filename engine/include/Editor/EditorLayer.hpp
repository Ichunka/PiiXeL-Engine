#ifndef PIIXELENGINE_EDITORLAYER_HPP
#define PIIXELENGINE_EDITORLAYER_HPP

#ifdef BUILD_WITH_EDITOR

#include <raylib.h>
#include <entt/entt.hpp>
#include <imgui.h>
#include <memory>
#include <unordered_map>
#include "Components/Transform.hpp"
#include "Components/UUID.hpp"
#include "CommandHistory.hpp"
#include "Debug/Profiler.hpp"

namespace PiiXeL {

class Engine;
class Scene;
class BuildPanel;
class SpriteSheetEditorPanel;
class AnimationClipEditorPanel;
class AnimatorControllerEditorPanel;

enum class EditorState {
    Edit,
    Play
};

enum class GizmoMode {
    Translate,
    Rotate,
    Scale
};

enum class GizmoAxis {
    None,
    X,
    Y
};

struct EntityState {
    Transform transform;
    Vector2 velocity{0.0f, 0.0f};
};

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
    void SetupDarkTheme();
    void SetupDockingLayout();
    void BeginDockspace();
    void EndDockspace();
    void DeleteAssetWithPackage(const std::string& assetPath);

    void RenderMenuBar();
    void RenderToolbar();
    void RenderViewport();
    void RenderGameViewport();
    void RenderHierarchy();
    void RenderInspector();
    void RenderContentBrowser();
    void RenderConsole();
    void RenderProjectSettings();
    void RenderProfiler();
    void RenderBuildPanel();

    Vector2 ScreenToWorld(Vector2 screenPos, const Camera2D& camera);
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

private:
    Engine* m_Engine;
    RenderTexture2D m_ViewportTexture;
    RenderTexture2D m_GameViewportTexture;
    Texture2D m_DefaultWhiteTexture;
    Rectangle m_ViewportBounds{0, 0, 1920, 1080};
    bool m_ViewportHovered{false};
    bool m_ViewportFocused{false};
    bool m_DockingLayoutInitialized{false};
    entt::entity m_SelectedEntity{entt::null};

    Vector2 m_CameraPosition{0.0f, 0.0f};
    float m_CameraZoom{1.0f};
    Vector2 m_LastMousePos{0.0f, 0.0f};
    bool m_IsPanning{false};

    bool m_IsDragging{false};
    Vector2 m_DragStartPos{0.0f, 0.0f};
    Vector2 m_EntityStartPos{0.0f, 0.0f};
    ImVec2 m_ViewportPos{0.0f, 0.0f};
    ImVec2 m_ViewportSize{0.0f, 0.0f};

    EditorState m_EditorState{EditorState::Edit};
    GizmoMode m_GizmoMode{GizmoMode::Translate};
    GizmoAxis m_SelectedAxis{GizmoAxis::None};
    std::unordered_map<entt::entity, EntityState> m_SavedStates;

    CommandHistory m_CommandHistory;
    Transform m_CachedTransform;
    bool m_IsModifyingTransform{false};

    bool m_IsDraggingEntity{false};
    bool m_InspectorLocked{false};
    entt::entity m_LockedEntity{entt::null};

    std::string m_CurrentScenePath{};

    std::string m_PlayModeSnapshot{};

    bool m_ShowProjectSettings{false};

    // Console state
    bool m_ConsoleShowEngine{true};
    bool m_ConsoleShowGame{true};
    bool m_ConsoleShowTrace{true};
    bool m_ConsoleShowDebug{true};
    bool m_ConsoleShowInfo{true};
    bool m_ConsoleShowWarning{true};
    bool m_ConsoleShowError{true};
    bool m_ConsoleAutoScroll{true};
    int m_ConsoleSelectedTab{0}; // 0=All, 1=Engine, 2=Game
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

    UUID m_SelectedAssetUUID{0};
    std::string m_SelectedAssetPath;
};

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR

#endif // PIIXELENGINE_EDITORLAYER_HPP
