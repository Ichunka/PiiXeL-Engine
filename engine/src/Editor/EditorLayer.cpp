#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorLayer.hpp"
#include "Core/Logger.hpp"
#include "Editor/ConsoleLogger.hpp"
#include "Editor/BuildPanel.hpp"
#include "Editor/SpriteSheetEditorPanel.hpp"
#include "Editor/AnimationClipEditorPanel.hpp"
#include "Editor/AnimatorControllerEditorPanel.hpp"
#include "Editor/EditorThemeManager.hpp"
#include "Editor/EditorSceneManager.hpp"
#include "Editor/EditorPanelManager.hpp"
#include "Editor/Panels/HierarchyPanel.hpp"
#include "Editor/Panels/InspectorPanel.hpp"
#include "Editor/Panels/ContentBrowserPanel.hpp"
#include "Editor/Panels/ConsolePanel.hpp"
#include "Editor/Panels/ProfilerPanel.hpp"
#include "Editor/Panels/GameViewportPanel.hpp"
#include "Editor/Panels/SceneViewportPanel.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/TextureAsset.hpp"
#include "Animation/SpriteSheet.hpp"
#include "Animation/AnimationClip.hpp"
#include "Animation/AnimatorController.hpp"
#include "Core/Engine.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneSerializer.hpp"
#include "Systems/RenderSystem.hpp"
#include "Systems/AnimationSystem.hpp"
#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Components/Sprite.hpp"
#include "Components/Camera.hpp"
#include "Components/Animator.hpp"
#include "Components/ComponentModuleRegistry.hpp"
#include "Components/Script.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Systems/ScriptSystem.hpp"
#include "Editor/EditorCommands.hpp"
#include "Project/ProjectSettings.hpp"
#include "Reflection/Reflection.hpp"
#include "Debug/DebugDraw.hpp"
#include "Debug/Profiler.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <entt/entt.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cinttypes>

namespace PiiXeL {

EditorLayer::EditorLayer(Engine* engine)
    : m_Engine{engine}
    , m_ViewportTexture{}
    , m_GameViewportTexture{}
    , m_DefaultWhiteTexture{}
    , m_ViewportBounds{0, 0, 1920, 1080}
    , m_ViewportHovered{false}
    , m_ViewportFocused{false}
    , m_SelectedEntity{entt::null}
    , m_CameraPosition{0.0f, 0.0f}
    , m_CameraZoom{1.0f}
    , m_LastMousePos{0.0f, 0.0f}
    , m_IsPanning{false}
{
    m_ViewportTexture = LoadRenderTexture(1920, 1080);
    m_GameViewportTexture = LoadRenderTexture(1920, 1080);

    Image whiteImage = GenImageColor(64, 64, WHITE);
    m_DefaultWhiteTexture = LoadTextureFromImage(whiteImage);
    UnloadImage(whiteImage);

    if (m_DefaultWhiteTexture.id != 0) {
        SetTextureWrap(m_DefaultWhiteTexture, TEXTURE_WRAP_CLAMP);
        PX_LOG_INFO(EDITOR, "Default white texture created: %d (64x64)", m_DefaultWhiteTexture.id);
    } else {
        PX_LOG_ERROR(EDITOR, "Failed to create default white texture");
    }

    m_BuildPanel = std::make_unique<BuildPanel>();
    m_SpriteSheetEditor = std::make_unique<SpriteSheetEditorPanel>();
    m_AnimationClipEditor = std::make_unique<AnimationClipEditorPanel>();
    m_AnimatorControllerEditor = std::make_unique<AnimatorControllerEditorPanel>();

    m_AnimatorControllerEditor->SetOnSelectionChangedCallback([this]() {
        m_SelectedEntity = entt::null;
        m_SelectedAssetUUID = UUID{0};
        m_SelectedAssetPath.clear();
    });

    m_HierarchyPanel = std::make_unique<HierarchyPanel>(
        m_Engine,
        &m_CommandHistory,
        &m_SelectedEntity,
        &m_InspectorLocked,
        &m_SelectedAssetUUID,
        &m_SelectedAssetPath,
        m_AnimatorControllerEditor.get()
    );

    m_InspectorPanel = std::make_unique<InspectorPanel>(
        m_Engine,
        &m_CommandHistory,
        &m_SelectedEntity,
        &m_InspectorLocked,
        &m_LockedEntity,
        &m_SelectedAssetUUID,
        &m_SelectedAssetPath,
        m_AnimatorControllerEditor.get(),
        &m_CachedTransform,
        &m_IsModifyingTransform,
        &m_DefaultWhiteTexture
    );

    m_ContentBrowserPanel = std::make_unique<ContentBrowserPanel>(
        &m_SelectedAssetUUID,
        &m_SelectedAssetPath,
        &m_SelectedEntity,
        m_SpriteSheetEditor.get(),
        m_AnimationClipEditor.get(),
        m_AnimatorControllerEditor.get()
    );

    m_ConsolePanel = std::make_unique<ConsolePanel>(
        &m_ConsoleFilters,
        &m_ConsoleAutoScroll,
        &m_ConsoleSelectedTab,
        &m_ConsoleSelectedLines,
        &m_ConsoleLastClickedLine
    );

    m_ProfilerPanel = std::make_unique<ProfilerPanel>(
        &m_ProfilerPaused,
        &m_ProfilerPausedSnapshot,
        &m_ProfilerSelectedFrame,
        &m_ProfilerSelectedFrameSnapshot,
        &m_ProfilerFlameGraphZoom,
        &m_ProfilerFlameGraphScroll
    );

    m_GameViewportPanel = std::make_unique<GameViewportPanel>(
        m_Engine,
        &m_GameViewportTexture,
        &m_EditorState
    );

    m_SceneViewportPanel = std::make_unique<SceneViewportPanel>(
        m_Engine,
        &m_ViewportTexture,
        &m_ViewportBounds,
        &m_ViewportHovered,
        &m_ViewportFocused,
        &m_CameraPosition,
        &m_CameraZoom,
        &m_LastMousePos,
        &m_IsPanning,
        &m_ViewportPos,
        &m_ViewportSize
    );

    m_HierarchyPanel->SetDuplicateEntityCallback([this](entt::entity entity) {
        return DuplicateEntity(entity);
    });
    m_HierarchyPanel->SetCopyEntityCallback([this](entt::entity entity) {
        CopyEntity(entity);
    });

    m_InspectorPanel->SetRenderEntityPickerCallback([this](const char* label, entt::entity* entity) {
        return RenderEntityPicker(label, entity);
    });
    m_InspectorPanel->SetRenderAssetPickerCallback([this](const char* label, UUID* uuid, const std::string& assetType) {
        return RenderAssetPicker(label, uuid, assetType);
    });

    m_ContentBrowserPanel->SetDeleteAssetCallback([this](const std::string& path) {
        DeleteAssetWithPackage(path);
    });
    m_ContentBrowserPanel->SetLoadSceneCallback([this]() {
        LoadScene();
    });

    m_GameViewportPanel->SetGetPrimaryCameraCallback([this]() {
        return GetPrimaryCamera();
    });

    m_SceneViewportPanel->SetHandleGizmoInteractionCallback([this]() {
        HandleGizmoInteraction();
    });
    m_SceneViewportPanel->SetHandleEntitySelectionCallback([this]() {
        HandleEntitySelection();
    });
    m_SceneViewportPanel->SetRenderGizmosCallback([this]() {
        RenderGizmos();
    });

    m_SceneManager = std::make_unique<EditorSceneManager>(m_Engine);
    m_PanelManager = std::make_unique<EditorPanelManager>();

    EditorThemeManager::SetupDarkTheme();

    m_Engine->SetScriptsEnabled(false);
    m_Engine->SetAnimationEnabled(false);

    SetTraceLogCallback(ConsoleLogger::RaylibLogCallback);

    LOG_ENGINE_INFO("Console logger initialized");
    LOG_ENGINE_DEBUG("Editor viewport size: 1920x1080");
    LOG_GAME_INFO("Game subsystem ready");
    LOG_GAME_DEBUG("Physics engine enabled");

    PX_LOG_INFO(EDITOR, "Trace level message from Raylib");
    PX_LOG_WARNING(EDITOR, "This is a warning test");
    PX_LOG_ERROR(EDITOR, "This is an error test (not a real error)");

    ProjectSettings& settings = ProjectSettings::Instance();
    settings.Load("game.config.json");

    std::string defaultScenePath = "content/scenes/Default_Scene.scene";
    if (FileExists(defaultScenePath.c_str()) && m_Engine && m_Engine->GetActiveScene()) {
        Scene* scene = m_Engine->GetActiveScene();
        SceneSerializer serializer{scene};
        if (serializer.Deserialize(defaultScenePath)) {
            m_SceneManager->SetCurrentScenePath(defaultScenePath);
            RestoreScriptPropertiesFromFile(defaultScenePath);
            PX_LOG_INFO(EDITOR, "Loaded default scene: %s", defaultScenePath.c_str());
        }
    }
}

EditorLayer::~EditorLayer() {
    if (m_ViewportTexture.id != 0) {
        UnloadRenderTexture(m_ViewportTexture);
    }
    if (m_GameViewportTexture.id != 0) {
        UnloadRenderTexture(m_GameViewportTexture);
    }
    if (m_DefaultWhiteTexture.id != 0) {
        UnloadTexture(m_DefaultWhiteTexture);
    }
}

void EditorLayer::OnUpdate(float deltaTime) {
    (void)deltaTime;

    if (m_EditorState == EditorState::Edit) {
        UpdateAnimatorPreviewInEditMode();
    }
}

void EditorLayer::OnRender() {
}

void EditorLayer::OnImGuiRender() {
    PROFILE_FUNCTION();

    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_W, false)) {
        m_CommandHistory.Undo();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
        m_CommandHistory.Redo();
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        SaveScene();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
        LoadScene();
    }

    if (m_EditorState == EditorState::Edit && m_SelectedEntity != entt::null) {
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C, false)) {
            CopyEntity(m_SelectedEntity);
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false)) {
            PasteEntity();
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
            entt::entity newEntity = DuplicateEntity(m_SelectedEntity);
            m_SelectedEntity = newEntity;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            if (m_Engine && m_Engine->GetActiveScene()) {
                Scene* scene = m_Engine->GetActiveScene();
                entt::registry& registry = scene->GetRegistry();
                if (registry.valid(m_SelectedEntity)) {
                    registry.destroy(m_SelectedEntity);
                    m_SelectedEntity = entt::null;
                }
            }
        }
    }

    m_PanelManager->BeginDockspace();

    RenderMenuBar();
    RenderToolbar();

    if (m_HierarchyPanel) m_HierarchyPanel->OnImGuiRender();
    if (m_InspectorPanel) m_InspectorPanel->OnImGuiRender();
    if (m_SceneViewportPanel) m_SceneViewportPanel->OnImGuiRender();
    if (m_GameViewportPanel) m_GameViewportPanel->OnImGuiRender();
    if (m_ContentBrowserPanel) m_ContentBrowserPanel->OnImGuiRender();
    if (m_ConsolePanel) m_ConsolePanel->OnImGuiRender();
    RenderProjectSettings();
    if (m_ProfilerPanel) m_ProfilerPanel->OnImGuiRender();
    if (m_BuildPanel) m_BuildPanel->Render();

    if (m_SpriteSheetEditor) {
        m_SpriteSheetEditor->Render();
    }

    if (m_AnimationClipEditor) {
        m_AnimationClipEditor->Render();
    }

    if (m_AnimatorControllerEditor) {
        m_AnimatorControllerEditor->Render();
    }

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
    } catch (const std::filesystem::filesystem_error& e) {
        PX_LOG_ERROR(EDITOR, "Failed to delete: %s", e.what());
    }
}

void EditorLayer::RenderMenuBar() {
    PROFILE_FUNCTION();
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {
                LoadScene();
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                SaveScene();
            }
            if (ImGui::MenuItem("Save Scene As...")) {
                SaveSceneAs();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_CommandHistory.CanUndo())) {
                m_CommandHistory.Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_CommandHistory.CanRedo())) {
                m_CommandHistory.Redo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Entity")) {
            if (ImGui::MenuItem("Create Empty")) {
                Scene* scene = m_Engine->GetActiveScene();
                if (m_Engine && scene) {
                    m_CommandHistory.ExecuteCommand(
                        std::make_unique<CreateEntityCommand>(scene, "Entity")
                    );
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Project")) {
            if (ImGui::MenuItem("Settings")) {
                m_ShowProjectSettings = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void EditorLayer::RenderToolbar() {
    PROFILE_FUNCTION();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{8, 8});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{8, 4});

    ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    if (m_EditorState == EditorState::Edit) {
        if (ImGui::Button("Play")) {
            OnPlayButtonPressed();
        }
    } else {
        if (ImGui::Button("Stop")) {
            OnStopButtonPressed();
        }
    }

    ImGui::SameLine();
    ImGui::Text("| State: %s", m_EditorState == EditorState::Edit ? "Edit" : "Play");

    ImGui::SameLine();
    ImGui::Separator();

    if (m_EditorState == EditorState::Edit) {
        ImGui::SameLine();
        ImGui::Text("Gizmo:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Translate", m_GizmoMode == GizmoMode::Translate)) {
            m_GizmoMode = GizmoMode::Translate;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", m_GizmoMode == GizmoMode::Rotate)) {
            m_GizmoMode = GizmoMode::Rotate;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", m_GizmoMode == GizmoMode::Scale)) {
            m_GizmoMode = GizmoMode::Scale;
        }
    }

    ImGui::SameLine();
    ImGui::Separator();

    ImGui::SameLine();
    if (m_Engine && m_Engine->GetRenderSystem()) {
        RenderSystem* renderSystem = m_Engine->GetRenderSystem();

        bool showColliders = renderSystem->GetShowColliders();
        if (ImGui::Checkbox("Show Colliders", &showColliders)) {
            renderSystem->SetShowColliders(showColliders);
        }

        ImGui::SameLine();
        bool showDebug = renderSystem->GetShowDebug();
        if (ImGui::Checkbox("Show Debug", &showDebug)) {
            renderSystem->SetShowDebug(showDebug);
        }

        ImGui::SameLine();
        bool showDebugRays = DebugDraw::Instance().IsEnabled();
        if (ImGui::Checkbox("Show Rays", &showDebugRays)) {
            DebugDraw::Instance().SetEnabled(showDebugRays);
        }
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}

Vector2 EditorLayer::ScreenToWorld(Vector2 screenPos, const Camera2D& camera) {
    Vector2 worldPos{};
    worldPos.x = (screenPos.x - camera.offset.x) / camera.zoom + camera.target.x;
    worldPos.y = (screenPos.y - camera.offset.y) / camera.zoom + camera.target.y;
    return worldPos;
}

void EditorLayer::HandleGizmoInteraction() {
    if (m_EditorState == EditorState::Play) {
        return;
    }

    if (!m_Engine || !m_Engine->GetActiveScene()) {
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    ImVec2 mouseImGui = ImGui::GetMousePos();
    Vector2 mouseViewportPos{
        mouseImGui.x - m_ViewportPos.x,
        mouseImGui.y - m_ViewportPos.y
    };

    if (mouseViewportPos.x < 0 || mouseViewportPos.y < 0 ||
        mouseViewportPos.x > m_ViewportSize.x || mouseViewportPos.y > m_ViewportSize.y) {
        return;
    }

    Camera2D camera{};
    camera.offset = Vector2{m_ViewportSize.x / 2.0f, m_ViewportSize.y / 2.0f};
    camera.target = m_CameraPosition;
    camera.rotation = 0.0f;
    camera.zoom = m_CameraZoom;

    Vector2 mouseWorldPos = ScreenToWorld(mouseViewportPos, camera);

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_IsPanning) {
        if (m_SelectedEntity != entt::null && registry.valid(m_SelectedEntity) && registry.all_of<Transform>(m_SelectedEntity)) {
            Transform& transform = registry.get<Transform>(m_SelectedEntity);

            m_SelectedAxis = GizmoAxis::None;

            float threshold = 10.0f / m_CameraZoom;
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
                float scaleHandleSize = 8.0f / m_CameraZoom;
                float centerRadius = 30.0f;
                float distanceSquared = deltaX * deltaX + deltaY * deltaY;

                Vector2 xHandle{transform.position.x + arrowLength, transform.position.y};
                Vector2 yHandle{transform.position.x, transform.position.y + arrowLength};

                if (std::abs(mouseWorldPos.x - xHandle.x) < scaleHandleSize &&
                    std::abs(mouseWorldPos.y - xHandle.y) < scaleHandleSize) {
                    m_SelectedAxis = GizmoAxis::X;
                    m_IsDragging = true;
                    m_DragStartPos = mouseWorldPos;
                    m_EntityStartPos = Vector2{transform.scale.x, transform.scale.y};
                }
                else if (std::abs(mouseWorldPos.x - yHandle.x) < scaleHandleSize &&
                         std::abs(mouseWorldPos.y - yHandle.y) < scaleHandleSize) {
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
                float rotateThickness = 10.0f / m_CameraZoom;

                if (std::abs(distance - rotateRadius) < rotateThickness) {
                    m_IsDragging = true;
                    m_DragStartPos = mouseWorldPos;
                    m_EntityStartPos.x = transform.rotation;
                }
            }
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (m_IsDragging && m_SelectedEntity != entt::null && registry.valid(m_SelectedEntity) && registry.all_of<Transform>(m_SelectedEntity)) {
            Transform& currentTransform = registry.get<Transform>(m_SelectedEntity);
            Transform oldTransform = currentTransform;

            if (m_GizmoMode == GizmoMode::Translate) {
                oldTransform.position = m_EntityStartPos;
            } else if (m_GizmoMode == GizmoMode::Scale) {
                oldTransform.scale.x = m_EntityStartPos.x;
                oldTransform.scale.y = m_EntityStartPos.y;
            } else if (m_GizmoMode == GizmoMode::Rotate) {
                oldTransform.rotation = m_EntityStartPos.x;
            }

            m_CommandHistory.AddCommand(
                std::make_unique<ModifyTransformCommand>(&registry, m_SelectedEntity, oldTransform, currentTransform)
            );
        }

        m_IsDragging = false;
        m_SelectedAxis = GizmoAxis::None;
    }

    if (m_IsDragging && m_SelectedEntity != entt::null && registry.valid(m_SelectedEntity) && registry.all_of<Transform>(m_SelectedEntity)) {
        Transform& transform = registry.get<Transform>(m_SelectedEntity);

        if (m_GizmoMode == GizmoMode::Translate) {
            Vector2 dragDelta{
                mouseWorldPos.x - m_DragStartPos.x,
                mouseWorldPos.y - m_DragStartPos.y
            };

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
            Vector2 dragDelta{
                mouseWorldPos.x - m_DragStartPos.x,
                mouseWorldPos.y - m_DragStartPos.y
            };

            float scaleDelta = (dragDelta.x + dragDelta.y) * 0.01f;

            if (m_SelectedAxis == GizmoAxis::X) {
                transform.scale.x = m_EntityStartPos.x + scaleDelta;
                if (transform.scale.x < 0.1f) transform.scale.x = 0.1f;
            }
            else if (m_SelectedAxis == GizmoAxis::Y) {
                transform.scale.y = m_EntityStartPos.y + scaleDelta;
                if (transform.scale.y < 0.1f) transform.scale.y = 0.1f;
            }
            else {
                transform.scale.x = m_EntityStartPos.x + scaleDelta;
                transform.scale.y = m_EntityStartPos.y + scaleDelta;
                if (transform.scale.x < 0.1f) transform.scale.x = 0.1f;
                if (transform.scale.y < 0.1f) transform.scale.y = 0.1f;
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

void EditorLayer::HandleEntitySelection() {
    if (m_EditorState == EditorState::Play || m_IsDragging || m_IsPanning) {
        return;
    }

    if (!m_Engine || !m_Engine->GetActiveScene()) {
        return;
    }

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    ImVec2 mouseImGui = ImGui::GetMousePos();
    Vector2 mouseViewportPos{
        mouseImGui.x - m_ViewportPos.x,
        mouseImGui.y - m_ViewportPos.y
    };

    if (mouseViewportPos.x < 0 || mouseViewportPos.y < 0 ||
        mouseViewportPos.x > m_ViewportSize.x || mouseViewportPos.y > m_ViewportSize.y) {
        return;
    }

    Camera2D camera{};
    camera.offset = Vector2{m_ViewportSize.x / 2.0f, m_ViewportSize.y / 2.0f};
    camera.target = m_CameraPosition;
    camera.rotation = 0.0f;
    camera.zoom = m_CameraZoom;

    Vector2 mouseWorldPos = ScreenToWorld(mouseViewportPos, camera);

    entt::entity clickedEntity = entt::null;
    float closestDistance = 99999.0f;

    registry.view<Transform, Sprite>().each([&](entt::entity entity, const Transform& transform, const Sprite& sprite) {
        if (!sprite.IsValid()) {
            return;
        }

        float halfW = sprite.sourceRect.width * transform.scale.x * 0.5f;
        float halfH = sprite.sourceRect.height * transform.scale.y * 0.5f;

        Vector2 localPos{
            mouseWorldPos.x - transform.position.x,
            mouseWorldPos.y - transform.position.y
        };

        float cosR = std::cos(-transform.rotation * DEG2RAD);
        float sinR = std::sin(-transform.rotation * DEG2RAD);
        Vector2 rotatedPos{
            localPos.x * cosR - localPos.y * sinR,
            localPos.x * sinR + localPos.y * cosR
        };

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

void EditorLayer::RenderGizmos() {
    if (m_EditorState == EditorState::Play) {
        return;
    }

    if (m_SelectedEntity == entt::null || !m_Engine || !m_Engine->GetActiveScene()) {
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    if (!registry.valid(m_SelectedEntity) || !registry.all_of<Transform>(m_SelectedEntity)) {
        return;
    }

    const Transform& transform = registry.get<Transform>(m_SelectedEntity);

    if (m_GizmoMode == GizmoMode::Translate) {
        DrawCircleV(transform.position, 5.0f / m_CameraZoom, Color{255, 200, 0, 255});

        Vector2 endX{transform.position.x + 50.0f, transform.position.y};
        DrawLineEx(transform.position, endX, 2.0f / m_CameraZoom, Color{255, 0, 0, 255});
        DrawCircleV(endX, 4.0f / m_CameraZoom, Color{255, 0, 0, 255});

        Vector2 endY{transform.position.x, transform.position.y + 50.0f};
        DrawLineEx(transform.position, endY, 2.0f / m_CameraZoom, Color{0, 255, 0, 255});
        DrawCircleV(endY, 4.0f / m_CameraZoom, Color{0, 255, 0, 255});
    }
    else if (m_GizmoMode == GizmoMode::Scale) {
        DrawCircleV(transform.position, 5.0f / m_CameraZoom, Color{255, 200, 0, 255});

        Vector2 endX{transform.position.x + 50.0f, transform.position.y};
        DrawLineEx(transform.position, endX, 2.0f / m_CameraZoom, Color{255, 0, 0, 255});
        float boxSize = 8.0f / m_CameraZoom;
        DrawRectangleV(
            Vector2{endX.x - boxSize, endX.y - boxSize},
            Vector2{boxSize * 2.0f, boxSize * 2.0f},
            Color{255, 0, 0, 255}
        );

        Vector2 endY{transform.position.x, transform.position.y + 50.0f};
        DrawLineEx(transform.position, endY, 2.0f / m_CameraZoom, Color{0, 255, 0, 255});
        DrawRectangleV(
            Vector2{endY.x - boxSize, endY.y - boxSize},
            Vector2{boxSize * 2.0f, boxSize * 2.0f},
            Color{0, 255, 0, 255}
        );
    }
    else if (m_GizmoMode == GizmoMode::Rotate) {
        float rotateRadius = 60.0f;
        DrawCircleLinesV(transform.position, rotateRadius, Color{100, 150, 255, 255});
        DrawCircleV(transform.position, 3.0f / m_CameraZoom, Color{255, 200, 0, 255});
    }
}

entt::entity EditorLayer::GetPrimaryCamera() {
    if (!m_Engine || !m_Engine->GetActiveScene()) {
        return entt::null;
    }

    Scene* scene = m_Engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    entt::entity primaryCamera = entt::null;
    registry.view<Camera, Transform>().each([&](entt::entity entity, const Camera& camera, const Transform&) {
        if (camera.isPrimary) {
            primaryCamera = entity;
        }
    });

    return primaryCamera;
}

void EditorLayer::OnPlayButtonPressed() {
    if (m_Engine && m_Engine->GetActiveScene()) {
        if (m_SceneManager->GetCurrentScenePath().empty()) {
            SaveSceneAs();
        } else {
            SaveScene();
        }

        Scene* scene = m_Engine->GetActiveScene();
        SceneSerializer serializer{scene};
        m_PlayModeSnapshot = serializer.SerializeToString();

        m_EditorState = EditorState::Play;
        m_CommandHistory.Clear();
        m_SelectedEntity = entt::null;

        m_Engine->CreatePhysicsBodies();
        m_Engine->SetPhysicsEnabled(true);
        m_Engine->SetScriptsEnabled(true);
        m_Engine->SetAnimationEnabled(true);

        AnimationSystem::ResetAnimators(scene->GetRegistry());

        PX_LOG_INFO(EDITOR, "Play mode started with memory snapshot");
    }
}

void EditorLayer::OnStopButtonPressed() {
    if (m_Engine && m_Engine->GetActiveScene()) {
        m_Engine->SetPhysicsEnabled(false);
        m_Engine->SetScriptsEnabled(false);
        m_Engine->SetAnimationEnabled(false);
        m_Engine->DestroyAllPhysicsBodies();

        if (!m_PlayModeSnapshot.empty()) {
            Scene* scene = m_Engine->GetActiveScene();
            SceneSerializer serializer{scene};
            if (serializer.DeserializeFromString(m_PlayModeSnapshot)) {
                m_EditorState = EditorState::Edit;
                m_CommandHistory.Clear();
                m_SelectedEntity = entt::null;

                if (m_Engine->GetScriptSystem()) {
                    entt::registry& registry = scene->GetRegistry();
                    ScriptSystem* scriptSystem = m_Engine->GetScriptSystem();

                    try {
                        nlohmann::json snapshotJson = nlohmann::json::parse(m_PlayModeSnapshot);

                        if (snapshotJson.contains("entities") && snapshotJson["entities"].is_array()) {
                            size_t entityIndex = 0;
                            const std::vector<entt::entity>& entityOrder = scene->GetEntityOrder();

                            for (entt::entity entity : entityOrder) {
                                if (entityIndex >= snapshotJson["entities"].size()) break;

                                const nlohmann::json& entityJson = snapshotJson["entities"][entityIndex];

                                if (registry.all_of<Script>(entity)) {
                                    Script& scriptComponent = registry.get<Script>(entity);

                                    if (entityJson.contains("Scripts") && entityJson["Scripts"].is_array()) {
                                        const nlohmann::json& scriptsArray = entityJson["Scripts"];
                                        for (size_t i = 0; i < scriptComponent.scripts.size() && i < scriptsArray.size(); ++i) {
                                            const nlohmann::json& scriptJson = scriptsArray[i];
                                            ScriptInstance& script = scriptComponent.scripts[i];

                                            if (!script.scriptName.empty() && !script.instance) {
                                                script.instance = scriptSystem->CreateScript(script.scriptName);
                                                if (script.instance) {
                                                    script.instance->Initialize(entity, scene);

                                                    if (scriptJson.contains("properties")) {
                                                        const nlohmann::json& propertiesJson = scriptJson["properties"];
                                                        const Reflection::TypeInfo* typeInfo = Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));

                                                        if (typeInfo) {
                                                            for (const Reflection::FieldInfo& field : typeInfo->GetFields()) {
                                                                if ((field.flags & Reflection::FieldFlags::Serializable) && propertiesJson.contains(field.name)) {
                                                                    void* fieldPtr = field.getPtr(static_cast<void*>(script.instance.get()));
                                                                    Reflection::JsonSerializer::DeserializeField(field, propertiesJson[field.name], fieldPtr);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    } else if (entityJson.contains("Script")) {
                                        const nlohmann::json& scriptJson = entityJson["Script"];
                                        if (scriptComponent.scripts.size() > 0) {
                                            ScriptInstance& script = scriptComponent.scripts[0];

                                            if (!script.scriptName.empty() && !script.instance) {
                                                script.instance = scriptSystem->CreateScript(script.scriptName);
                                                if (script.instance) {
                                                    script.instance->Initialize(entity, scene);

                                                    if (scriptJson.contains("properties")) {
                                                        const nlohmann::json& propertiesJson = scriptJson["properties"];
                                                        const Reflection::TypeInfo* typeInfo = Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));

                                                        if (typeInfo) {
                                                            for (const Reflection::FieldInfo& field : typeInfo->GetFields()) {
                                                                if ((field.flags & Reflection::FieldFlags::Serializable) && propertiesJson.contains(field.name)) {
                                                                    void* fieldPtr = field.getPtr(static_cast<void*>(script.instance.get()));
                                                                    Reflection::JsonSerializer::DeserializeField(field, propertiesJson[field.name], fieldPtr);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                entityIndex++;
                            }
                        }
                    } catch (const nlohmann::json::exception& e) {
                        PX_LOG_ERROR(EDITOR, "Failed to restore script properties: %s", e.what());
                    }
                }

                PX_LOG_INFO(EDITOR, "Scene restored from memory snapshot - edit mode");
            }
        } else {
            m_EditorState = EditorState::Edit;
            m_CommandHistory.Clear();
        }
    }
}

void EditorLayer::NewScene() {
    m_SceneManager->NewScene();
    m_SelectedEntity = entt::null;
    m_CommandHistory.Clear();
}

void EditorLayer::SaveScene() {
    m_SceneManager->SaveScene();
}

void EditorLayer::SaveSceneAs() {
    m_SceneManager->SaveSceneAs();
}

void EditorLayer::LoadScene() {
    m_SceneManager->LoadScene();
    m_SelectedEntity = entt::null;
    m_CommandHistory.Clear();
    RestoreScriptPropertiesFromFile(m_SceneManager->GetCurrentScenePath());
}

void EditorLayer::RenderProjectSettings() {
    PROFILE_FUNCTION();
    if (!m_ShowProjectSettings) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2{600, 500}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Project Settings", &m_ShowProjectSettings)) {
        ProjectSettings& settings = ProjectSettings::Instance();

        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("General")) {
                ImGui::SeparatorText("Project Info");

                char projectName[256];
                std::memcpy(projectName, settings.projectName.c_str(),
                    std::min(settings.projectName.size(), sizeof(projectName) - 1));
                projectName[std::min(settings.projectName.size(), sizeof(projectName) - 1)] = '\0';

                if (ImGui::InputText("Project Name", projectName, sizeof(projectName))) {
                    settings.projectName = std::string(projectName);
                }

                char version[64];
                std::memcpy(version, settings.version.c_str(),
                    std::min(settings.version.size(), sizeof(version) - 1));
                version[std::min(settings.version.size(), sizeof(version) - 1)] = '\0';

                if (ImGui::InputText("Version", version, sizeof(version))) {
                    settings.version = std::string(version);
                }

                char company[256];
                std::memcpy(company, settings.company.c_str(),
                    std::min(settings.company.size(), sizeof(company) - 1));
                company[std::min(settings.company.size(), sizeof(company) - 1)] = '\0';

                if (ImGui::InputText("Company", company, sizeof(company))) {
                    settings.company = std::string(company);
                }

                char startScene[256];
                std::memcpy(startScene, settings.startScene.c_str(),
                    std::min(settings.startScene.size(), sizeof(startScene) - 1));
                startScene[std::min(settings.startScene.size(), sizeof(startScene) - 1)] = '\0';

                if (ImGui::InputText("Start Scene", startScene, sizeof(startScene))) {
                    settings.startScene = std::string(startScene);
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Window")) {
                ImGui::SeparatorText("Window Settings");

                ImGui::DragInt("Width", &settings.window.width, 1.0f, 640, 3840);
                ImGui::DragInt("Height", &settings.window.height, 1.0f, 480, 2160);
                ImGui::DragInt("Target FPS", &settings.window.targetFPS, 1.0f, 30, 240);

                ImGui::Checkbox("Resizable", &settings.window.resizable);
                ImGui::Checkbox("VSync", &settings.window.vsync);
                ImGui::Checkbox("Fullscreen by Default", &settings.window.fullscreen);

                ImGui::SeparatorText("Window Icon");
                char iconPath[512];
                std::memcpy(iconPath, settings.window.icon.c_str(),
                    std::min(settings.window.icon.size(), sizeof(iconPath) - 1));
                iconPath[std::min(settings.window.icon.size(), sizeof(iconPath) - 1)] = '\0';
                if (ImGui::InputText("Icon Path", iconPath, sizeof(iconPath))) {
                    settings.window.icon = std::string(iconPath);
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Path to icon file (relative to game directory)\n\n"
                                     "Recommended: Use .ico format for both window and exe icon\n\n"
                                     "How it works:\n"
                                     "1. Set path to .ico file (e.g., content/assets/icon.ico)\n"
                                     "2. Engine tries to load .ico for window icon\n"
                                     "3. If .ico fails, automatically uses .png fallback\n"
                                     "4. Windows exe icon is embedded via CMake\n\n"
                                     "Fallback: If .ico doesn't work for window icon,\n"
                                     "create icon.png in same directory\n\n"
                                     "Any pixel format works (RGBA, RGB, Grayscale)\n"
                                     "- Engine auto-converts to required format");
                }

                ImGui::Spacing();
                ImGui::TextDisabled("Note: These settings apply to the built game.");
                ImGui::TextDisabled("Use F11 to toggle fullscreen at runtime.");

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Physics")) {
                ImGui::SeparatorText("Physics Settings");

                ImGui::DragFloat2("Gravity", &settings.physics.gravity.x, 0.1f, -100.0f, 100.0f);
                ImGui::DragFloat("Time Step", &settings.physics.timeStep, 0.001f, 0.001f, 0.1f, "%.4f");
                ImGui::DragInt("Velocity Iterations", &settings.physics.velocityIterations, 1.0f, 1, 20);
                ImGui::DragInt("Position Iterations", &settings.physics.positionIterations, 1.0f, 1, 20);

                ImGui::Spacing();
                if (ImGui::Button("Apply to Current Physics")) {
                    if (m_Engine && m_Engine->GetPhysicsSystem()) {
                        settings.ApplyToPhysics(m_Engine->GetPhysicsSystem());
                    }
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Build")) {
                ImGui::SeparatorText("Build Settings");

                ImGui::Text("Assets Mode:");
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Auto: Only include assets used in scenes");
                    ImGui::Text("All: Include all assets from content/assets");
                    ImGui::Text("Manual: Manually specify assets to include");
                    ImGui::EndTooltip();
                }

                const char* assetsModes[] = {"auto", "all", "manual"};
                int currentMode = 0;
                std::string currentModeStr = "auto";

                if (settings.buildConfig.contains("assetsMode")) {
                    currentModeStr = settings.buildConfig["assetsMode"].get<std::string>();
                    if (currentModeStr == "all") currentMode = 1;
                    else if (currentModeStr == "manual") currentMode = 2;
                }

                if (ImGui::Combo("##AssetsMode", &currentMode, assetsModes, 3)) {
                    settings.buildConfig["assetsMode"] = assetsModes[currentMode];
                }

                ImGui::Spacing();
                ImGui::SeparatorText("Scenes to Export");

                if (!settings.buildConfig.contains("scenes") || !settings.buildConfig["scenes"].is_array()) {
                    settings.buildConfig["scenes"] = nlohmann::json::array();
                }

                nlohmann::json& scenes = settings.buildConfig["scenes"];

                for (size_t i = 0; i < scenes.size(); ++i) {
                    ImGui::PushID(static_cast<int>(i));

                    std::string scenePath = scenes[i].get<std::string>();
                    char scenePathBuf[512];
                    std::memcpy(scenePathBuf, scenePath.c_str(), std::min(scenePath.size(), sizeof(scenePathBuf) - 1));
                    scenePathBuf[std::min(scenePath.size(), sizeof(scenePathBuf) - 1)] = '\0';

                    ImGui::SetNextItemWidth(-60.0f);
                    if (ImGui::InputText("##scene", scenePathBuf, sizeof(scenePathBuf))) {
                        scenes[i] = std::string(scenePathBuf);
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Remove")) {
                        scenes.erase(scenes.begin() + static_cast<int>(i));
                        --i;
                    }

                    ImGui::PopID();
                }

                if (ImGui::Button("Add Scene")) {
                    scenes.push_back("content/scenes/NewScene.scene");
                }

                if (currentMode == 2) {
                    ImGui::Spacing();
                    ImGui::SeparatorText("Assets to Export (Manual Mode)");

                    if (!settings.buildConfig.contains("assets") || !settings.buildConfig["assets"].is_array()) {
                        settings.buildConfig["assets"] = nlohmann::json::array();
                    }

                    nlohmann::json& assets = settings.buildConfig["assets"];

                    for (size_t i = 0; i < assets.size(); ++i) {
                        ImGui::PushID(1000 + static_cast<int>(i));

                        std::string assetPath = assets[i].get<std::string>();
                        char assetPathBuf[512];
                        std::memcpy(assetPathBuf, assetPath.c_str(), std::min(assetPath.size(), sizeof(assetPathBuf) - 1));
                        assetPathBuf[std::min(assetPath.size(), sizeof(assetPathBuf) - 1)] = '\0';

                        ImGui::SetNextItemWidth(-60.0f);
                        if (ImGui::InputText("##asset", assetPathBuf, sizeof(assetPathBuf))) {
                            assets[i] = std::string(assetPathBuf);
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("Remove")) {
                            assets.erase(assets.begin() + static_cast<int>(i));
                            --i;
                        }

                        ImGui::PopID();
                    }

                    if (ImGui::Button("Add Asset")) {
                        assets.push_back("content/assets/");
                    }
                } else {
                    ImGui::Spacing();
                    if (currentMode == 0) {
                        ImGui::TextDisabled("Assets will be automatically detected from scenes");
                    } else {
                        ImGui::TextDisabled("All assets from content/assets will be included");
                    }
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save Settings")) {
            settings.Save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Settings")) {
            settings.Load();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            m_ShowProjectSettings = false;
        }
    }
    ImGui::End();
}

bool EditorLayer::RenderEntityPicker(const char* label, entt::entity* entity) {
    if (!m_Engine || !m_Engine->GetActiveScene()) {
        return false;
    }

    Scene* scene = m_Engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    std::string preview = "None";
    if (*entity != entt::null && registry.valid(*entity)) {
        if (registry.all_of<Tag>(*entity)) {
            Tag& tag = registry.get<Tag>(*entity);
            preview = tag.name + " [" + std::to_string(static_cast<uint32_t>(*entity)) + "]";
        } else {
            preview = "Entity [" + std::to_string(static_cast<uint32_t>(*entity)) + "]";
        }
    }

    bool changed = false;
    if (ImGui::BeginCombo(label, preview.c_str())) {
        if (ImGui::Selectable("None", *entity == entt::null)) {
            *entity = entt::null;
            changed = true;
        }

        registry.view<Tag>().each([&](entt::entity e, Tag& tag) {
            bool isSelected = (*entity == e);
            std::string itemLabel = tag.name + " [" + std::to_string(static_cast<uint32_t>(e)) + "]";

            if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                *entity = e;
                changed = true;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        });

        ImGui::EndCombo();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_REORDER")) {
            if (payload->DataSize == sizeof(size_t)) {
                size_t draggedIndex = *static_cast<size_t*>(payload->Data);
                const std::vector<entt::entity>& entityOrder = scene->GetEntityOrder();
                if (draggedIndex < entityOrder.size()) {
                    entt::entity draggedEntity = entityOrder[draggedIndex];
                    if (registry.valid(draggedEntity)) {
                        *entity = draggedEntity;
                        changed = true;
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    return changed;
}

bool EditorLayer::RenderAssetPicker(const char* label, UUID* uuid, const std::string& assetType) {
    std::string preview = "None";
    if (uuid->Get() != 0) {
        std::shared_ptr<Asset> asset = AssetRegistry::Instance().GetAsset(*uuid);
        if (asset) {
            preview = asset->GetMetadata().name;
        } else {
            preview = "Missing [" + uuid->ToString().substr(0, 8) + "]";
        }
    }

    bool changed = false;
    if (ImGui::BeginCombo(label, preview.c_str())) {
        if (ImGui::Selectable("None", uuid->Get() == 0)) {
            *uuid = UUID{0};
            changed = true;
        }

        const auto& allKnownPaths = AssetRegistry::Instance().GetAllKnownAssetPaths();
        for (const auto& [assetUUID, assetPath] : allKnownPaths) {
            std::string extension = std::filesystem::path{assetPath}.extension().string();
            bool matchesFilter = false;

            if (assetType == "texture" && (extension == ".png" || extension == ".jpg" || extension == ".jpeg")) {
                matchesFilter = true;
            } else if (assetType == "AnimatorController" && extension == ".animcontroller") {
                matchesFilter = true;
            } else if (assetType == "SpriteSheet" && extension == ".spritesheet") {
                matchesFilter = true;
            } else if (assetType == "AnimationClip" && extension == ".animclip") {
                matchesFilter = true;
            } else if (assetType == "Audio" && (extension == ".wav" || extension == ".mp3" || extension == ".ogg")) {
                matchesFilter = true;
            }

            if (matchesFilter) {
                ImGui::PushID(static_cast<int>(assetUUID.Get()));

                bool isSelected = (uuid->Get() == assetUUID.Get());
                std::filesystem::path fsPath{assetPath};
                std::string itemLabel = fsPath.stem().string();

                if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                    *uuid = assetUUID;
                    changed = true;
                }

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }

                ImGui::PopID();
            }
        }

        ImGui::EndCombo();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
            if (payload->DataSize == sizeof(std::string)) {
                std::string draggedPath = *static_cast<std::string*>(payload->Data);
                UUID draggedUUID = AssetRegistry::Instance().GetUUIDFromPath(draggedPath);
                if (draggedUUID.Get() != 0) {
                    std::shared_ptr<Asset> asset = AssetRegistry::Instance().GetAsset(draggedUUID);
                    if (asset && asset->GetMetadata().type == AssetType::Texture && assetType == "texture") {
                        *uuid = draggedUUID;
                        changed = true;
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    return changed;
}

void EditorLayer::RestoreScriptPropertiesFromFile(const std::string& filepath) {
    if (!m_Engine || !m_Engine->GetActiveScene() || !m_Engine->GetScriptSystem()) {
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();
    ScriptSystem* scriptSystem = m_Engine->GetScriptSystem();

    try {
        std::ifstream file{filepath};
        if (!file.is_open()) {
            return;
        }

        nlohmann::json sceneJson{};
        file >> sceneJson;
        file.close();

        if (!sceneJson.contains("entities") || !sceneJson["entities"].is_array()) {
            return;
        }

        size_t entityIndex = 0;
        const std::vector<entt::entity>& entityOrder = scene->GetEntityOrder();

        for (entt::entity entity : entityOrder) {
            if (entityIndex >= sceneJson["entities"].size()) break;

            const nlohmann::json& entityJson = sceneJson["entities"][entityIndex];

            if (registry.all_of<Script>(entity)) {
                Script& scriptComponent = registry.get<Script>(entity);

                if (entityJson.contains("Scripts") && entityJson["Scripts"].is_array()) {
                    const nlohmann::json& scriptsArray = entityJson["Scripts"];
                    size_t scriptCount = std::min(scriptComponent.scripts.size(), scriptsArray.size());

                    for (size_t i = 0; i < scriptCount; ++i) {
                        ScriptInstance& script = scriptComponent.scripts[i];
                        const nlohmann::json& scriptJson = scriptsArray[i];

                        if (!script.scriptName.empty() && !script.instance) {
                            script.instance = scriptSystem->CreateScript(script.scriptName);
                            if (script.instance) {
                                script.instance->Initialize(entity, scene);
                            }
                        }

                        if (script.instance && scriptJson.contains("properties")) {
                            const nlohmann::json& propertiesJson = scriptJson["properties"];
                            const Reflection::TypeInfo* typeInfo = Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));

                            if (typeInfo) {
                                for (const Reflection::FieldInfo& field : typeInfo->GetFields()) {
                                    if ((field.flags & Reflection::FieldFlags::Serializable) && propertiesJson.contains(field.name)) {
                                        void* fieldPtr = field.getPtr(static_cast<void*>(script.instance.get()));
                                        Reflection::JsonSerializer::DeserializeField(field, propertiesJson[field.name], fieldPtr);
                                    }
                                }
                            }
                        }
                    }
                } else if (entityJson.contains("Script")) {
                    const nlohmann::json& scriptJson = entityJson["Script"];
                    if (scriptComponent.scripts.size() > 0) {
                        ScriptInstance& script = scriptComponent.scripts[0];

                        if (!script.scriptName.empty() && !script.instance) {
                            script.instance = scriptSystem->CreateScript(script.scriptName);
                            if (script.instance) {
                                script.instance->Initialize(entity, scene);
                            }
                        }

                        if (script.instance && scriptJson.contains("properties")) {
                            const nlohmann::json& propertiesJson = scriptJson["properties"];
                            const Reflection::TypeInfo* typeInfo = Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));

                            if (typeInfo) {
                                for (const Reflection::FieldInfo& field : typeInfo->GetFields()) {
                                    if ((field.flags & Reflection::FieldFlags::Serializable) && propertiesJson.contains(field.name)) {
                                        void* fieldPtr = field.getPtr(static_cast<void*>(script.instance.get()));
                                        Reflection::JsonSerializer::DeserializeField(field, propertiesJson[field.name], fieldPtr);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            entityIndex++;
        }
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Failed to restore script properties: %s", e.what());
    }
}

void EditorLayer::UpdateAnimatorPreviewInEditMode() {
    if (!m_Engine || !m_Engine->GetActiveScene()) {
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    auto view = registry.view<Animator, Sprite>();
    for (entt::entity entity : view) {
        Animator& animator = view.get<Animator>(entity);
        Sprite& sprite = view.get<Sprite>(entity);

        if (animator.controllerUUID.Get() == 0) {
            continue;
        }

        std::shared_ptr<Asset> controllerAsset = AssetRegistry::Instance().GetAsset(animator.controllerUUID);
        AnimatorController* controller = dynamic_cast<AnimatorController*>(controllerAsset.get());
        if (!controller) {
            continue;
        }

        std::string defaultStateName = controller->GetDefaultState();
        const AnimatorState* defaultState = controller->GetState(defaultStateName);
        if (!defaultState || defaultState->animationClipUUID.Get() == 0) {
            continue;
        }

        std::shared_ptr<Asset> clipAsset = AssetRegistry::Instance().GetAsset(defaultState->animationClipUUID);
        AnimationClip* clip = dynamic_cast<AnimationClip*>(clipAsset.get());
        if (!clip || clip->GetFrames().empty()) {
            continue;
        }

        std::shared_ptr<Asset> sheetAsset = AssetRegistry::Instance().GetAsset(clip->GetSpriteSheetUUID());
        SpriteSheet* sheet = dynamic_cast<SpriteSheet*>(sheetAsset.get());
        if (!sheet) {
            continue;
        }

        const AnimationFrame& firstFrame = clip->GetFrames()[0];
        const SpriteFrame* spriteFrame = sheet->GetFrame(firstFrame.frameIndex);
        if (spriteFrame) {
            sprite.textureAssetUUID = sheet->GetTextureUUID();
            sprite.sourceRect = spriteFrame->sourceRect;
            sprite.origin = spriteFrame->pivot;
        }
    }
}

entt::entity EditorLayer::DuplicateEntity(entt::entity entity) {
    if (!m_Engine || !m_Engine->GetActiveScene()) {
        return entt::null;
    }

    Scene* scene = m_Engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    if (!registry.valid(entity)) {
        return entt::null;
    }

    std::string newName = "Entity (Copy)";
    if (registry.all_of<Tag>(entity)) {
        const Tag& originalTag = registry.get<Tag>(entity);
        newName = originalTag.name + " (Copy)";
    }

    entt::entity newEntity = scene->CreateEntity(newName);

    if (registry.all_of<Sprite>(entity)) {
        const Sprite& originalSprite = registry.get<Sprite>(entity);
        Sprite newSprite;

        newSprite.textureAssetUUID = originalSprite.textureAssetUUID;
        newSprite.tint = originalSprite.tint;
        newSprite.sourceRect = originalSprite.sourceRect;
        newSprite.origin = originalSprite.origin;
        newSprite.layer = originalSprite.layer;

        registry.emplace<Sprite>(newEntity, newSprite);
    }

    ComponentModuleRegistry::Instance().DuplicateAllComponents(registry, entity, newEntity);

    if (registry.all_of<Script>(entity)) {
        const Script& originalScript = registry.get<Script>(entity);
        Script newScript;
        for (const ScriptInstance& script : originalScript.scripts) {
            newScript.AddScript(script.scriptName);
        }
        registry.emplace<Script>(newEntity, newScript);
    }

    PX_LOG_INFO(EDITOR, "Entity duplicated with all components");

    return newEntity;
}

void EditorLayer::CopyEntity(entt::entity entity) {
    if (!m_Engine || !m_Engine->GetActiveScene()) {
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    if (!registry.valid(entity)) {
        return;
    }

    m_CopiedEntity = entity;
    PX_LOG_INFO(EDITOR, "Entity copied to clipboard");
}

void EditorLayer::PasteEntity() {
    if (m_CopiedEntity == entt::null) {
        return;
    }

    if (!m_Engine || !m_Engine->GetActiveScene()) {
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    if (!registry.valid(m_CopiedEntity)) {
        m_CopiedEntity = entt::null;
        return;
    }

    entt::entity newEntity = DuplicateEntity(m_CopiedEntity);
    m_SelectedEntity = newEntity;

    PX_LOG_INFO(EDITOR, "Entity pasted");
}

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR
