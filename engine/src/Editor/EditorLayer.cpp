#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorLayer.hpp"

#include "Animation/AnimationClip.hpp"
#include "Animation/AnimatorController.hpp"
#include "Components/Animator.hpp"
#include "Components/Camera.hpp"
#include "Components/ComponentModuleRegistry.hpp"
#include "Components/Transform.hpp"
#include "Core/Engine.hpp"
#include "Core/Logger.hpp"
#include "Debug/Profiler.hpp"
#include "Editor/AnimationClipEditorPanel.hpp"
#include "Editor/AnimatorControllerEditorPanel.hpp"
#include "Editor/BuildPanel.hpp"
#include "Editor/ConsoleLogger.hpp"
#include "Editor/EditorAnimatorPreviewSystem.hpp"
#include "Editor/EditorCamera.hpp"
#include "Editor/EditorCommandSystem.hpp"
#include "Editor/EditorGizmoSystem.hpp"
#include "Editor/EditorPanelManager.hpp"
#include "Editor/EditorSceneManager.hpp"
#include "Editor/EditorSelectionManager.hpp"
#include "Editor/EditorShortcutHandler.hpp"
#include "Editor/EditorStateManager.hpp"
#include "Editor/EditorThemeManager.hpp"
#include "Editor/Panels/ConsolePanel.hpp"
#include "Editor/Panels/ContentBrowserPanel.hpp"
#include "Editor/Panels/GameViewportPanel.hpp"
#include "Editor/Panels/HierarchyPanel.hpp"
#include "Editor/Panels/InspectorPanel.hpp"
#include "Editor/Panels/MenuBarPanel.hpp"
#include "Editor/Panels/ProfilerPanel.hpp"
#include "Editor/Panels/ProjectSettingsPanel.hpp"
#include "Editor/Panels/SceneViewportPanel.hpp"
#include "Editor/Panels/ToolbarPanel.hpp"
#include "Editor/SpriteSheetEditorPanel.hpp"
#include "Editor/Utilities/EditorAssetPickerUtility.hpp"
#include "Project/ProjectSettings.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneSerializer.hpp"

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <cinttypes>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <iomanip>
#include <sstream>

namespace PiiXeL {

EditorLayer::EditorLayer(Engine* engine) :
    m_Engine{engine}, m_ViewportTexture{}, m_GameViewportTexture{}, m_DefaultWhiteTexture{},
    m_ViewportBounds{0, 0, 1920, 1080}, m_ViewportHovered{false}, m_ViewportFocused{false} {
    m_ViewportTexture = LoadRenderTexture(1920, 1080);
    m_GameViewportTexture = LoadRenderTexture(1920, 1080);

    Image whiteImage = GenImageColor(64, 64, WHITE);
    m_DefaultWhiteTexture = LoadTextureFromImage(whiteImage);
    UnloadImage(whiteImage);

    if (m_DefaultWhiteTexture.id != 0)
        SetTextureWrap(m_DefaultWhiteTexture, TEXTURE_WRAP_CLAMP);
    else
        PX_LOG_ERROR(EDITOR, "Failed to create default white texture");

    m_EditorCamera = std::make_unique<EditorCamera>();
    m_SceneManager = std::make_unique<EditorSceneManager>(m_Engine);
    m_PanelManager = std::make_unique<EditorPanelManager>();
    m_GizmoSystem = std::make_unique<EditorGizmoSystem>();
    m_StateManager = std::make_unique<EditorStateManager>();
    m_CommandSystem = std::make_unique<EditorCommandSystem>();
    m_SelectionManager = std::make_unique<EditorSelectionManager>();

    m_BuildPanel = std::make_unique<BuildPanel>();
    m_SpriteSheetEditor = std::make_unique<SpriteSheetEditorPanel>();
    m_AnimationClipEditor = std::make_unique<AnimationClipEditorPanel>();
    m_AnimatorControllerEditor = std::make_unique<AnimatorControllerEditorPanel>();

    m_AnimatorControllerEditor->SetOnSelectionChangedCallback([this]() { m_SelectionManager->ClearSelection(); });

    m_HierarchyPanel = std::make_unique<HierarchyPanel>(
        m_Engine, m_CommandSystem.get(), m_SelectionManager->GetSelectedEntityPtr(),
        m_SelectionManager->GetInspectorLockedPtr(), m_SelectionManager->GetSelectedAssetUUIDPtr(),
        m_SelectionManager->GetSelectedAssetPathPtr(), m_AnimatorControllerEditor.get());

    m_InspectorPanel = std::make_unique<InspectorPanel>(
        m_Engine, m_CommandSystem.get(), m_SelectionManager->GetSelectedEntityPtr(),
        m_SelectionManager->GetInspectorLockedPtr(), m_SelectionManager->GetLockedEntityPtr(),
        m_SelectionManager->GetSelectedAssetUUIDPtr(), m_SelectionManager->GetSelectedAssetPathPtr(),
        m_AnimatorControllerEditor.get(), &m_DefaultWhiteTexture);

    m_ContentBrowserPanel = std::make_unique<ContentBrowserPanel>(
        m_SelectionManager->GetSelectedAssetUUIDPtr(), m_SelectionManager->GetSelectedAssetPathPtr(),
        m_SelectionManager->GetSelectedEntityPtr(), m_SpriteSheetEditor.get(), m_AnimationClipEditor.get(),
        m_AnimatorControllerEditor.get());

    m_ConsolePanel = std::make_unique<ConsolePanel>(&m_ConsoleFilters, &m_ConsoleAutoScroll, &m_ConsoleSelectedTab,
                                                    &m_ConsoleSelectedLines, &m_ConsoleLastClickedLine);

    m_ProfilerPanel = std::make_unique<ProfilerPanel>(&m_ProfilerPaused, &m_ProfilerPausedSnapshot,
                                                      &m_ProfilerSelectedFrame, &m_ProfilerSelectedFrameSnapshot,
                                                      &m_ProfilerFlameGraphZoom, &m_ProfilerFlameGraphScroll);

    m_GameViewportPanel = std::make_unique<GameViewportPanel>(m_Engine, &m_GameViewportTexture, m_StateManager.get());

    m_SceneViewportPanel =
        std::make_unique<SceneViewportPanel>(m_Engine, &m_ViewportTexture, &m_ViewportBounds, &m_ViewportHovered,
                                             &m_ViewportFocused, m_EditorCamera.get(), &m_ViewportPos, &m_ViewportSize);

    m_ProjectSettingsPanel = std::make_unique<ProjectSettingsPanel>(m_Engine);

    m_ToolbarPanel = std::make_unique<ToolbarPanel>(m_Engine, m_GizmoSystem.get(), m_StateManager.get());
    m_ToolbarPanel->SetOnPlayCallback([this]() { OnPlayButtonPressed(); });
    m_ToolbarPanel->SetOnStopCallback([this]() { OnStopButtonPressed(); });

    m_MenuBarPanel = std::make_unique<MenuBarPanel>(m_Engine, m_CommandSystem.get(), m_SceneManager.get(),
                                                    m_ProjectSettingsPanel.get());

    m_HierarchyPanel->SetDuplicateEntityCallback(
        [this](entt::entity e) { return m_SelectionManager->DuplicateEntity(m_Engine, e); });
    m_HierarchyPanel->SetCopyEntityCallback([this](entt::entity e) { m_SelectionManager->CopyEntity(m_Engine, e); });
    m_InspectorPanel->SetRenderEntityPickerCallback([this](const char* l, entt::entity* e) {
        return EditorAssetPickerUtility::RenderEntityPicker(l, e, m_Engine);
    });
    m_InspectorPanel->SetRenderAssetPickerCallback([](const char* l, UUID* u, const std::string& t) {
        return EditorAssetPickerUtility::RenderAssetPicker(l, u, t);
    });

    m_ContentBrowserPanel->SetDeleteAssetCallback([this](const std::string& path) { DeleteAssetWithPackage(path); });
    m_ContentBrowserPanel->SetLoadSceneCallback([this]() {
        m_SceneManager->LoadScene();
        m_SelectionManager->SetSelectedEntity(entt::null);
        m_CommandSystem->Clear();
    });

    m_GameViewportPanel->SetGetPrimaryCameraCallback([this]() { return GetPrimaryCamera(); });

    m_SceneViewportPanel->SetHandleGizmoInteractionCallback([this]() { HandleGizmoInteraction(); });
    m_SceneViewportPanel->SetHandleEntitySelectionCallback([this]() { HandleEntitySelection(); });
    m_SceneViewportPanel->SetRenderGizmosCallback([this]() { RenderGizmos(); });

    EditorThemeManager::SetupDarkTheme();

    m_Engine->SetScriptsEnabled(false);
    m_Engine->SetAnimationEnabled(false);

    SetTraceLogCallback(ConsoleLogger::RaylibLogCallback);
    ProjectSettings::Instance().Load("game.config.json");

    LoadDefaultScene();
}

void EditorLayer::LoadDefaultScene() {
    std::string defaultScenePath = "content/scenes/Default_Scene.scene";
    if (FileExists(defaultScenePath.c_str()) && m_Engine && m_Engine->GetActiveScene()) {
        Scene* scene = m_Engine->GetActiveScene();
        SceneSerializer serializer{scene};
        if (serializer.Deserialize(defaultScenePath)) {
            m_SceneManager->SetCurrentScenePath(defaultScenePath);
            m_SceneManager->RestoreScriptPropertiesFromFile(defaultScenePath);
            PX_LOG_INFO(EDITOR, "Loaded default scene: %s", defaultScenePath.c_str());
        }
    }
}

EditorLayer::~EditorLayer() {
    if (m_ViewportTexture.id != 0)
        UnloadRenderTexture(m_ViewportTexture);
    if (m_GameViewportTexture.id != 0)
        UnloadRenderTexture(m_GameViewportTexture);
    if (m_DefaultWhiteTexture.id != 0)
        UnloadTexture(m_DefaultWhiteTexture);
}

void EditorLayer::OnUpdate(float deltaTime) {
    (void)deltaTime;
    if (m_StateManager->IsEditMode())
        EditorAnimatorPreviewSystem::UpdateAnimatorPreviewInEditMode(m_Engine);
}

void EditorLayer::OnRender() {}

void EditorLayer::OnImGuiRender() {
    PROFILE_FUNCTION();

    EditorShortcutHandler::HandleShortcuts(m_Engine, m_CommandSystem.get(), m_SceneManager.get(),
                                           m_SelectionManager.get(), m_StateManager.get());

    m_PanelManager->BeginDockspace();

    m_MenuBarPanel->OnImGuiRender();
    m_ToolbarPanel->OnImGuiRender();
    m_HierarchyPanel->OnImGuiRender();
    m_InspectorPanel->OnImGuiRender();
    m_SceneViewportPanel->OnImGuiRender();
    m_GameViewportPanel->OnImGuiRender();
    m_ContentBrowserPanel->OnImGuiRender();
    m_ConsolePanel->OnImGuiRender();
    m_ProjectSettingsPanel->OnImGuiRender();
    m_ProfilerPanel->OnImGuiRender();
    m_BuildPanel->Render();
    m_SpriteSheetEditor->Render();
    m_AnimationClipEditor->Render();
    m_AnimatorControllerEditor->Render();

    m_PanelManager->EndDockspace();
}

void EditorLayer::DeleteAssetWithPackage(const std::string& assetPath) {
    try {
        std::filesystem::remove(assetPath);

        std::filesystem::path pxaPath{assetPath};
        pxaPath.replace_extension(".pxa");

        if (std::filesystem::exists(pxaPath)) {
            std::filesystem::remove(pxaPath);
            PX_LOG_INFO(EDITOR, "Deleted package: %s", pxaPath.string().c_str());
        }

        PX_LOG_INFO(EDITOR, "Deleted: %s", assetPath.c_str());
    }
    catch (const std::filesystem::filesystem_error& e) {
        PX_LOG_ERROR(EDITOR, "Failed to delete: %s", e.what());
    }
}

void EditorLayer::HandleGizmoInteraction() {
    if (m_StateManager->IsPlayMode())
        return;
    m_GizmoSystem->HandleGizmoInteraction(m_Engine, m_EditorCamera.get(), m_SelectionManager->GetSelectedEntity(),
                                          m_ViewportPos.x, m_ViewportPos.y, m_ViewportSize.x, m_ViewportSize.y,
                                          m_CommandSystem.get());
}

void EditorLayer::HandleEntitySelection() {
    if (m_StateManager->IsPlayMode())
        return;
    m_SelectionManager->HandleEntitySelection(m_Engine, m_EditorCamera.get(), m_ViewportPos.x, m_ViewportPos.y,
                                              m_ViewportSize.x, m_ViewportSize.y, m_GizmoSystem->IsDragging());
}

void EditorLayer::RenderGizmos() {
    if (m_StateManager->IsPlayMode())
        return;
    m_GizmoSystem->RenderGizmos(m_Engine, m_EditorCamera.get(), m_SelectionManager->GetSelectedEntity());
}

entt::entity EditorLayer::GetPrimaryCamera() {
    if (!m_Engine || !m_Engine->GetActiveScene())
        return entt::null;
    Scene* scene = m_Engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    entt::entity primaryCamera = entt::null;
    registry.view<Camera, Transform>().each([&](entt::entity entity, const Camera& camera, const Transform&) {
        if (camera.isPrimary)
            primaryCamera = entity;
    });
    return primaryCamera;
}

void EditorLayer::OnPlayButtonPressed() {
    m_StateManager->OnPlayButtonPressed(m_Engine, m_SceneManager.get(), m_SelectionManager.get(),
                                        m_CommandSystem.get());
}

void EditorLayer::OnStopButtonPressed() {
    m_StateManager->OnStopButtonPressed(m_Engine, m_SelectionManager.get(), m_CommandSystem.get());
}

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR
