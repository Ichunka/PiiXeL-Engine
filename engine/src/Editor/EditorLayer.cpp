#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorLayer.hpp"
#include "Editor/ConsoleLogger.hpp"
#include "Editor/BuildPanel.hpp"
#include "Editor/SpriteSheetEditorPanel.hpp"
#include "Editor/AnimationClipEditorPanel.hpp"
#include "Editor/AnimatorControllerEditorPanel.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/AssetImporter.hpp"
#include "Resources/TextureAsset.hpp"
#include "Resources/AudioAsset.hpp"
#include "Animation/AnimationSerializer.hpp"
#include "Animation/SpriteSheet.hpp"
#include "Animation/AnimationClip.hpp"
#include "Animation/AnimatorController.hpp"
#include "Core/Engine.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneSerializer.hpp"
#include "Scene/EntityRegistry.hpp"
#include "Systems/RenderSystem.hpp"
#include "Resources/AssetManager.hpp"
#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Components/Sprite.hpp"
#include "Components/Camera.hpp"
#include "Components/RigidBody2D.hpp"
#include "Components/BoxCollider2D.hpp"
#include "Components/Script.hpp"
#include "Components/Animator.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Systems/ScriptSystem.hpp"
#include "Scripting/ScriptRegistry.hpp"
#include "Editor/EditorCommands.hpp"
#include "Project/ProjectSettings.hpp"
#include "Reflection/Reflection.hpp"
#include "Debug/DebugDraw.hpp"
#include "Debug/Profiler.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <rlImGui.h>
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
    , m_DockingLayoutInitialized{false}
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
        TraceLog(LOG_INFO, "Default white texture created: %d (64x64)", m_DefaultWhiteTexture.id);
    } else {
        TraceLog(LOG_ERROR, "Failed to create default white texture");
    }

    m_BuildPanel = std::make_unique<BuildPanel>();
    m_SpriteSheetEditor = std::make_unique<SpriteSheetEditorPanel>();
    m_AnimationClipEditor = std::make_unique<AnimationClipEditorPanel>();
    m_AnimatorControllerEditor = std::make_unique<AnimatorControllerEditorPanel>();
    SetupDarkTheme();

    m_Engine->SetScriptsEnabled(false);

    SetTraceLogCallback(ConsoleLogger::RaylibLogCallback);

    LOG_ENGINE_INFO("Console logger initialized");
    LOG_ENGINE_DEBUG("Editor viewport size: 1920x1080");
    LOG_GAME_INFO("Game subsystem ready");
    LOG_GAME_DEBUG("Physics engine enabled");

    TraceLog(LOG_TRACE, "Trace level message from Raylib");
    TraceLog(LOG_WARNING, "This is a warning test");
    TraceLog(LOG_ERROR, "This is an error test (not a real error)");

    ProjectSettings& settings = ProjectSettings::Instance();
    settings.Load("game.config.json");

    std::string defaultScenePath = "content/scenes/Default_Scene.scene";
    if (FileExists(defaultScenePath.c_str()) && m_Engine && m_Engine->GetActiveScene()) {
        Scene* scene = m_Engine->GetActiveScene();
        SceneSerializer serializer{scene};
        if (serializer.Deserialize(defaultScenePath)) {
            m_CurrentScenePath = defaultScenePath;
            RestoreScriptPropertiesFromFile(defaultScenePath);
            TraceLog(LOG_INFO, "Loaded default scene: %s", defaultScenePath.c_str());
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

void EditorLayer::SetupDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.1f, 0.1f, 1.0f};
    colors[ImGuiCol_ChildBg] = ImVec4{0.12f, 0.12f, 0.12f, 1.0f};
    colors[ImGuiCol_PopupBg] = ImVec4{0.11f, 0.11f, 0.11f, 1.0f};

    colors[ImGuiCol_Border] = ImVec4{0.25f, 0.25f, 0.25f, 1.0f};
    colors[ImGuiCol_FrameBg] = ImVec4{0.16f, 0.16f, 0.16f, 1.0f};
    colors[ImGuiCol_FrameBgHovered] = ImVec4{0.20f, 0.20f, 0.20f, 1.0f};
    colors[ImGuiCol_FrameBgActive] = ImVec4{0.25f, 0.25f, 0.25f, 1.0f};

    colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.15f, 0.15f, 1.0f};
    colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.15f, 0.15f, 1.0f};
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.15f, 0.15f, 1.0f};

    colors[ImGuiCol_MenuBarBg] = ImVec4{0.12f, 0.12f, 0.12f, 1.0f};

    colors[ImGuiCol_Header] = ImVec4{0.20f, 0.20f, 0.20f, 1.0f};
    colors[ImGuiCol_HeaderHovered] = ImVec4{0.25f, 0.25f, 0.25f, 1.0f};
    colors[ImGuiCol_HeaderActive] = ImVec4{0.30f, 0.30f, 0.30f, 1.0f};

    colors[ImGuiCol_Button] = ImVec4{0.20f, 0.20f, 0.20f, 1.0f};
    colors[ImGuiCol_ButtonHovered] = ImVec4{0.28f, 0.28f, 0.28f, 1.0f};
    colors[ImGuiCol_ButtonActive] = ImVec4{0.35f, 0.35f, 0.35f, 1.0f};

    colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.15f, 0.15f, 1.0f};
    colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.38f, 0.38f, 1.0f};
    colors[ImGuiCol_TabSelected] = ImVec4{0.28f, 0.28f, 0.28f, 1.0f};
    colors[ImGuiCol_TabDimmed] = ImVec4{0.15f, 0.15f, 0.15f, 1.0f};
    colors[ImGuiCol_TabDimmedSelected] = ImVec4{0.20f, 0.20f, 0.20f, 1.0f};

    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 2.0f;
    style.WindowPadding = ImVec2{8.0f, 8.0f};
    style.FramePadding = ImVec2{4.0f, 3.0f};
    style.ItemSpacing = ImVec2{8.0f, 4.0f};
}

void EditorLayer::OnUpdate(float deltaTime) {
    (void)deltaTime;
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

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N, false)) {
        NewScene();
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

    BeginDockspace();

    RenderMenuBar();
    RenderToolbar();
    RenderHierarchy();
    RenderInspector();
    RenderViewport();
    RenderGameViewport();
    RenderContentBrowser();
    RenderConsole();
    RenderProjectSettings();
    RenderProfiler();
    RenderBuildPanel();

    if (m_SpriteSheetEditor) {
        m_SpriteSheetEditor->Render();
    }

    if (m_AnimationClipEditor) {
        m_AnimationClipEditor->Render();
    }

    if (m_AnimatorControllerEditor) {
        m_AnimatorControllerEditor->Render();
    }

    EndDockspace();
}

void EditorLayer::BeginDockspace() {
    PROFILE_FUNCTION();
    static bool dockspaceOpen = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        if (!m_DockingLayoutInitialized) {
            SetupDockingLayout();
            m_DockingLayoutInitialized = true;
        }
    }
}

void EditorLayer::SetupDockingLayout() {
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    ImGuiID dock_id_top = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.08f, nullptr, &dockspace_id);
    ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, nullptr, &dockspace_id);
    ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);
    ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);

    ImGui::DockBuilderDockWindow("Toolbar", dock_id_top);
    ImGui::DockBuilderDockWindow("Hierarchy", dock_id_left);
    ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
    ImGui::DockBuilderDockWindow("Scene", dockspace_id);
    ImGui::DockBuilderDockWindow("Game", dockspace_id);
    ImGui::DockBuilderDockWindow("Content Browser", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Console", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Build & Export", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Profiler", dock_id_right);

    ImGui::DockBuilderFinish(dockspace_id);
}

void EditorLayer::EndDockspace() {
    PROFILE_FUNCTION();
    ImGui::End();
}

void EditorLayer::RenderMenuBar() {
    PROFILE_FUNCTION();
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                NewScene();
            }
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

void EditorLayer::RenderViewport() {
    PROFILE_FUNCTION();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin("Scene");

    m_ViewportFocused = ImGui::IsWindowFocused();
    m_ViewportHovered = ImGui::IsWindowHovered();

    m_ViewportPos = ImGui::GetCursorScreenPos();
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    m_ViewportSize = viewportPanelSize;

    if (m_ViewportHovered && m_ViewportFocused) {
        Vector2 mousePos = GetMousePosition();
        float wheel = GetMouseWheelMove();

        if (wheel != 0.0f) {
            m_CameraZoom += wheel * 0.1f * m_CameraZoom;
            if (m_CameraZoom < 0.1f) m_CameraZoom = 0.1f;
            if (m_CameraZoom > 10.0f) m_CameraZoom = 10.0f;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
            m_IsPanning = true;
            m_LastMousePos = mousePos;
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) {
            m_IsPanning = false;
        }

        if (m_IsPanning) {
            Vector2 delta{mousePos.x - m_LastMousePos.x, mousePos.y - m_LastMousePos.y};
            m_CameraPosition.x -= delta.x / m_CameraZoom;
            m_CameraPosition.y -= delta.y / m_CameraZoom;
            m_LastMousePos = mousePos;
        }

        HandleGizmoInteraction();
        HandleEntitySelection();
    }

    if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
        if (static_cast<int>(viewportPanelSize.x) != m_ViewportTexture.texture.width ||
            static_cast<int>(viewportPanelSize.y) != m_ViewportTexture.texture.height) {
            UnloadRenderTexture(m_ViewportTexture);
            m_ViewportTexture = LoadRenderTexture(
                static_cast<int>(viewportPanelSize.x),
                static_cast<int>(viewportPanelSize.y)
            );
        }

        m_ViewportBounds = Rectangle{
            0, 0,
            viewportPanelSize.x,
            viewportPanelSize.y
        };

        BeginTextureMode(m_ViewportTexture);
        ClearBackground(Color{45, 45, 48, 255});

        Camera2D camera{};
        camera.offset = Vector2{viewportPanelSize.x / 2.0f, viewportPanelSize.y / 2.0f};
        camera.target = m_CameraPosition;
        camera.rotation = 0.0f;
        camera.zoom = m_CameraZoom;

        BeginMode2D(camera);

        DrawLine(-10000, 0, 10000, 0, Color{80, 80, 80, 255});
        DrawLine(0, -10000, 0, 10000, Color{80, 80, 80, 255});

        for (int i = -100; i <= 100; i++) {
            if (i == 0) continue;
            DrawLine(i * 100, -10000, i * 100, 10000, Color{50, 50, 50, 255});
            DrawLine(-10000, i * 100, 10000, i * 100, Color{50, 50, 50, 255});
        }

        if (m_Engine) {
            m_Engine->Render();
        }

        RenderGizmos();

        EndMode2D();

        DrawText(
            TextFormat("Zoom: %.2f | Pos: (%.0f, %.0f)", m_CameraZoom, m_CameraPosition.x, m_CameraPosition.y),
            10, 10, 16, RAYWHITE
        );

        EndTextureMode();

        Rectangle sourceRec{
            0.0f, 0.0f,
            static_cast<float>(m_ViewportTexture.texture.width),
            -static_cast<float>(m_ViewportTexture.texture.height)
        };

        rlImGuiImageRect(&m_ViewportTexture.texture,
                        static_cast<int>(viewportPanelSize.x),
                        static_cast<int>(viewportPanelSize.y),
                        sourceRec);

        if (m_EditorState == EditorState::Edit && ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE")) {
                AssetInfo* assetInfo = *static_cast<AssetInfo**>(payload->Data);
                if (assetInfo && m_Engine && m_Engine->GetActiveScene()) {
                    Scene* scene = m_Engine->GetActiveScene();
                    entt::registry& registry = scene->GetRegistry();

                    ImVec2 mouseImGui = ImGui::GetMousePos();
                    Vector2 mouseViewportPos{
                        mouseImGui.x - m_ViewportPos.x,
                        mouseImGui.y - m_ViewportPos.y
                    };

                    Camera2D dropCamera{};
                    dropCamera.offset = Vector2{viewportPanelSize.x / 2.0f, viewportPanelSize.y / 2.0f};
                    dropCamera.target = m_CameraPosition;
                    dropCamera.rotation = 0.0f;
                    dropCamera.zoom = m_CameraZoom;

                    Vector2 worldPos = ScreenToWorld(mouseViewportPos, dropCamera);

                    std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAssetFromPath(assetInfo->path);
                    if (asset && asset->GetMetadata().type == AssetType::Texture) {
                        entt::entity newEntity = registry.create();

                        UUID uuid{};
                        registry.emplace<UUID>(newEntity, uuid);
                        EntityRegistry::Instance().RegisterEntity(uuid, newEntity);

                        std::string entityName = assetInfo->filename;
                        size_t dotPos = entityName.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            entityName = entityName.substr(0, dotPos);
                        }

                        Tag tag{};
                        tag.name = entityName;
                        registry.emplace<Tag>(newEntity, tag);

                        Transform transform{};
                        transform.position = worldPos;
                        transform.rotation = 0.0f;
                        transform.scale = Vector2{1.0f, 1.0f};
                        registry.emplace<Transform>(newEntity, transform);

                        Sprite sprite{};
                        sprite.SetTexture(asset->GetUUID());
                        sprite.tint = WHITE;
                        sprite.origin = Vector2{0.5f, 0.5f};
                        registry.emplace<Sprite>(newEntity, sprite);

                        scene->GetEntityOrder().push_back(newEntity);

                        m_SelectedEntity = newEntity;

                        TraceLog(LOG_INFO, "Entity created from texture: %s at (%.0f, %.0f)",
                                assetInfo->filename.c_str(), worldPos.x, worldPos.y);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorLayer::RenderHierarchy() {
    PROFILE_FUNCTION();
    ImGui::Begin("Hierarchy");

    m_IsDraggingEntity = false;

    if (m_Engine && m_Engine->GetActiveScene()) {
        Scene* scene = m_Engine->GetActiveScene();
        entt::registry& registry = scene->GetRegistry();

        if (ImGui::Button("+ Create Entity")) {
            m_CommandHistory.ExecuteCommand(
                std::make_unique<CreateEntityCommand>(scene, "New Entity")
            );
        }

        ImGui::Separator();

        std::vector<entt::entity>& entityOrder = scene->GetEntityOrder();

        for (size_t i = 0; i < entityOrder.size(); ++i) {
            entt::entity entity = entityOrder[i];

            if (!registry.valid(entity) || !registry.all_of<Tag>(entity)) {
                continue;
            }

            Tag& tag = registry.get<Tag>(entity);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (m_SelectedEntity == entity) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            ImGui::TreeNodeEx(
                reinterpret_cast<void*>(static_cast<uint64_t>(static_cast<uint32_t>(entity))),
                flags,
                "%s",
                tag.name.c_str()
            );

            bool itemHovered = ImGui::IsItemHovered();

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                m_IsDraggingEntity = true;
                size_t draggedIndex = i;
                ImGui::SetDragDropPayload("ENTITY_REORDER", &draggedIndex, sizeof(size_t));
                ImGui::Text("%s", tag.name.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_REORDER")) {
                    if (payload->DataSize == sizeof(size_t)) {
                        size_t draggedIndex = *static_cast<size_t*>(payload->Data);
                        size_t targetIndex = i;

                        if (draggedIndex != targetIndex && draggedIndex < entityOrder.size()) {
                            entt::entity draggedEntity = entityOrder[draggedIndex];
                            entityOrder.erase(entityOrder.begin() + draggedIndex);
                            if (draggedIndex < targetIndex) {
                                targetIndex--;
                            }
                            entityOrder.insert(entityOrder.begin() + targetIndex, draggedEntity);
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (itemHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !m_IsDraggingEntity) {
                if (!m_InspectorLocked) {
                    m_SelectedEntity = entity;
                    m_SelectedAssetUUID = UUID{0};
                    m_SelectedAssetPath.clear();
                }
            }

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                    entt::entity newEntity = DuplicateEntity(entity);
                    m_SelectedEntity = newEntity;
                    m_SelectedAssetUUID = UUID{0};
                    m_SelectedAssetPath.clear();
                }

                if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                    CopyEntity(entity);
                }

                if (ImGui::MenuItem("Delete", "Delete")) {
                    if (m_SelectedEntity == entity) {
                        m_SelectedEntity = entt::null;
                    }
                    scene->DestroyEntity(entity);
                }

                ImGui::EndPopup();
            }
        }
    }

    ImGui::End();
}

void EditorLayer::RenderInspector() {
    PROFILE_FUNCTION();
    ImGui::Begin("Inspector");

    if (m_SelectedAssetUUID.Get() != 0 || !m_SelectedAssetPath.empty()) {
        ImGui::TextColored(ImVec4{0.4f, 0.8f, 1.0f, 1.0f}, "Asset");
        ImGui::Separator();

        std::shared_ptr<Asset> asset = AssetRegistry::Instance().GetAsset(m_SelectedAssetUUID);

        bool isSceneFile = false;
        if (m_SelectedAssetPath.size() >= 6) {
            std::string extension = m_SelectedAssetPath.substr(m_SelectedAssetPath.size() - 6);
            isSceneFile = (extension == ".scene");
        }

        if (!asset && m_SelectedAssetUUID.Get() == 0 && !m_SelectedAssetPath.empty() && !isSceneFile) {
            auto result = AssetRegistry::Instance().LoadAssetFromPath(m_SelectedAssetPath);
            if (result) {
                asset = result;
                m_SelectedAssetUUID = asset->GetUUID();
            }
        }

        if (asset) {
            const AssetMetadata& metadata = asset->GetMetadata();

            ImGui::Text("Name: %s", metadata.name.c_str());
            ImGui::Text("UUID: %" PRIu64, metadata.uuid.Get());

            const char* typeStr = "Unknown";
            switch (metadata.type) {
                case AssetType::Texture: typeStr = "Texture"; break;
                case AssetType::Audio: typeStr = "Audio"; break;
                case AssetType::SpriteSheet: typeStr = "Sprite Sheet"; break;
                case AssetType::AnimationClip: typeStr = "Animation Clip"; break;
                case AssetType::AnimatorController: typeStr = "Animator Controller"; break;
                case AssetType::Scene: typeStr = "Scene"; break;
                default: break;
            }
            ImGui::Text("Type: %s", typeStr);

            ImGui::Text("Source: %s", metadata.sourceFile.c_str());
            ImGui::Text("Memory: %zu bytes", asset->GetMemoryUsage());
            ImGui::Text("Loaded: %s", asset->IsLoaded() ? "Yes" : "No");

            ImGui::Separator();

            if (metadata.type == AssetType::Texture) {
                TextureAsset* texAsset = dynamic_cast<TextureAsset*>(asset.get());
                if (texAsset) {
                    ImGui::Text("Dimensions: %dx%d", texAsset->GetWidth(), texAsset->GetHeight());
                    ImGui::Text("Mipmaps: %d", texAsset->GetMipmaps());
                    ImGui::Text("Format: %d", texAsset->GetFormat());

                    ImGui::Separator();
                    ImGui::Text("Preview:");

                    Texture2D texture = texAsset->GetTexture();
                    if (texture.id != 0) {
                        float maxWidth = ImGui::GetContentRegionAvail().x - 20.0f;
                        float maxHeight = 256.0f;

                        float aspectRatio = static_cast<float>(texture.width) / static_cast<float>(texture.height);
                        float displayWidth = maxWidth;
                        float displayHeight = displayWidth / aspectRatio;

                        if (displayHeight > maxHeight) {
                            displayHeight = maxHeight;
                            displayWidth = displayHeight * aspectRatio;
                        }

                        rlImGuiImageSize(&texture, static_cast<int>(displayWidth), static_cast<int>(displayHeight));
                    }
                }
            } else if (metadata.type == AssetType::Audio) {
                AudioAsset* audioAsset = dynamic_cast<AudioAsset*>(asset.get());
                if (audioAsset) {
                    ImGui::Text("Frames: %u", audioAsset->GetFrameCount());

                    ImGui::Separator();

                    Sound sound = audioAsset->GetSound();
                    if (sound.frameCount > 0) {
                        if (ImGui::Button("Play")) {
                            PlaySound(sound);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Stop")) {
                            StopSound(sound);
                        }
                    }
                }
            } else if (metadata.type == AssetType::SpriteSheet) {
                SpriteSheet* spriteSheet = dynamic_cast<SpriteSheet*>(asset.get());
                if (spriteSheet) {
                    ImGui::Text("Texture UUID: %" PRIu64, spriteSheet->GetTextureUUID().Get());
                    ImGui::Text("Grid: %dx%d", spriteSheet->GetGridColumns(), spriteSheet->GetGridRows());
                    ImGui::Text("Frame Count: %zu", spriteSheet->GetFrames().size());
                }
            } else if (metadata.type == AssetType::AnimationClip) {
                AnimationClip* animClip = dynamic_cast<AnimationClip*>(asset.get());
                if (animClip) {
                    ImGui::Text("Sprite Sheet UUID: %" PRIu64, animClip->GetSpriteSheetUUID().Get());
                    ImGui::Text("Frame Count: %zu", animClip->GetFrames().size());
                    ImGui::Text("Frame Rate: %.1f fps", animClip->GetFrameRate());
                    ImGui::Text("Duration: %.2f seconds", animClip->GetTotalDuration());

                    const char* wrapModeStr = "Unknown";
                    switch (animClip->GetWrapMode()) {
                        case AnimationWrapMode::Once: wrapModeStr = "Once"; break;
                        case AnimationWrapMode::Loop: wrapModeStr = "Loop"; break;
                        case AnimationWrapMode::PingPong: wrapModeStr = "Ping Pong"; break;
                    }
                    ImGui::Text("Wrap Mode: %s", wrapModeStr);
                }
            } else if (metadata.type == AssetType::AnimatorController) {
                AnimatorController* controller = dynamic_cast<AnimatorController*>(asset.get());
                if (controller) {
                    ImGui::Text("Default State: %s", controller->GetDefaultState().c_str());
                    ImGui::Text("Parameters: %zu", controller->GetParameters().size());
                    ImGui::Text("States: %zu", controller->GetStates().size());
                    ImGui::Text("Transitions: %zu", controller->GetTransitions().size());
                }
            }

            ImGui::Separator();

            if (ImGui::Button("Reimport")) {
                AssetRegistry::Instance().ReimportAsset(metadata.sourceFile);
            }
        } else {
            ImGui::Text("Path: %s", m_SelectedAssetPath.c_str());
            ImGui::TextColored(ImVec4{0.6f, 0.6f, 0.6f, 1.0f}, "Scene file (not an importable asset)");
        }

        ImGui::End();
        return;
    }

    if (ImGui::Button(m_InspectorLocked ? "Unlock" : "Lock")) {
        m_InspectorLocked = !m_InspectorLocked;
        if (m_InspectorLocked) {
            m_LockedEntity = m_SelectedEntity;
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled(m_InspectorLocked ? "(Inspector Locked)" : "(Inspector Unlocked)");

    entt::entity inspectedEntity = m_InspectorLocked ? m_LockedEntity : m_SelectedEntity;

    if (inspectedEntity != entt::null && m_Engine && m_Engine->GetActiveScene()) {
        Scene* scene = m_Engine->GetActiveScene();
        entt::registry& registry = scene->GetRegistry();

        if (registry.valid(inspectedEntity)) {
            if (registry.all_of<Tag>(inspectedEntity)) {
                Tag& tag = registry.get<Tag>(inspectedEntity);

                char buffer[256]{};
                size_t copyLen = (tag.name.length() < sizeof(buffer) - 1) ? tag.name.length() : sizeof(buffer) - 1;
                std::memcpy(buffer, tag.name.c_str(), copyLen);
                buffer[sizeof(buffer) - 1] = '\0';

                if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
                    tag.name = std::string(buffer);
                }
            }

            ImGui::Separator();

            if (registry.all_of<Transform>(inspectedEntity)) {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    Transform& transform = registry.get<Transform>(inspectedEntity);

                    if (!m_IsModifyingTransform) {
                        m_CachedTransform = transform;
                    }

                    bool posModified = ImGui::DragFloat2("Position", &transform.position.x, 1.0f);
                    bool rotModified = ImGui::DragFloat("Rotation", &transform.rotation, 0.5f);
                    bool scaleModified = ImGui::DragFloat2("Scale", &transform.scale.x, 1.0f);

                    if (posModified || rotModified || scaleModified) {
                        if (!m_IsModifyingTransform) {
                            m_IsModifyingTransform = true;
                        }
                    }

                    if (m_IsModifyingTransform && !ImGui::IsAnyItemActive()) {
                        if (m_CachedTransform.position.x != transform.position.x ||
                            m_CachedTransform.position.y != transform.position.y ||
                            m_CachedTransform.rotation != transform.rotation ||
                            m_CachedTransform.scale.x != transform.scale.x ||
                            m_CachedTransform.scale.y != transform.scale.y) {

                            m_CommandHistory.AddCommand(
                                std::make_unique<ModifyTransformCommand>(&registry, inspectedEntity, m_CachedTransform, transform)
                            );
                        }
                        m_IsModifyingTransform = false;
                    }
                }
            }

            if (registry.all_of<Camera>(inspectedEntity)) {
                ImGui::Separator();
                bool removeCamera = false;

                ImGui::AlignTextToFramePadding();
                bool cameraOpen = ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 25);
                if (ImGui::SmallButton("X##RemoveCamera")) {
                    removeCamera = true;
                }

                if (cameraOpen) {
                    Camera& camera = registry.get<Camera>(inspectedEntity);

                    bool wasPrimary = camera.isPrimary;
                    Reflection::ImGuiRenderer::RenderProperties(camera, [this](const char* label, entt::entity* entity) {
                        return RenderEntityPicker(label, entity);
                    });

                    if (camera.isPrimary != wasPrimary && camera.isPrimary) {
                        registry.view<Camera>().each([inspectedEntity, &registry](entt::entity entity, Camera& otherCamera) {
                            if (entity != inspectedEntity) {
                                otherCamera.isPrimary = false;
                            }
                        });
                    }

                    ImGui::TreePop();
                }

                if (removeCamera) {
                    m_CommandHistory.ExecuteCommand(
                        std::make_unique<RemoveComponentCommand<Camera>>(&registry, inspectedEntity)
                    );
                }
            }

            if (registry.all_of<Sprite>(inspectedEntity)) {
                ImGui::Separator();
                bool removeSprite = false;

                ImGui::AlignTextToFramePadding();
                bool spriteOpen = ImGui::TreeNodeEx("Sprite", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 25);
                if (ImGui::SmallButton("X##RemoveSprite")) {
                    removeSprite = true;
                }

                if (spriteOpen) {
                    Sprite& sprite = registry.get<Sprite>(inspectedEntity);

                    ImGui::Text("Texture");
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.2f, 0.2f, 1.0f});

                    Texture2D displayTexture = m_DefaultWhiteTexture;
                    float previewWidth = 100.0f;
                    float previewHeight = 100.0f;
                    bool isDefaultTexture = true;

                    if (sprite.IsValid()) {
                        Texture2D texture = sprite.GetTexture();
                        if (texture.id != 0) {
                            displayTexture = texture;
                            isDefaultTexture = false;
                            float aspectRatio = static_cast<float>(texture.width) / static_cast<float>(texture.height);
                            previewHeight = previewWidth / aspectRatio;
                        }
                    }

                    if (displayTexture.id == 0) {
                        ImGui::TextColored(ImVec4{1.0f, 0.0f, 0.0f, 1.0f}, "ERROR: No texture to display!");
                    } else {
                        ImTextureID texId = static_cast<ImTextureID>(static_cast<intptr_t>(displayTexture.id));
                        if (ImGui::ImageButton("##TexturePreview", texId, ImVec2{previewWidth, previewHeight})) {
                        }

                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE")) {
                                AssetInfo* assetInfo = *static_cast<AssetInfo**>(payload->Data);
                                if (assetInfo) {
                                    std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAssetFromPath(assetInfo->path);
                                    if (asset && asset->GetMetadata().type == AssetType::Texture) {
                                        sprite.SetTexture(asset->GetUUID());
                                        TraceLog(LOG_INFO, "Texture assigned: %s", assetInfo->path.c_str());
                                    }
                                }
                            }
                            ImGui::EndDragDropTarget();
                        }

                        if (isDefaultTexture) {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "(None)");
                        }
                    }

                    ImGui::PopStyleColor();

                    if (sprite.IsValid()) {
                        Texture2D texture = sprite.GetTexture();
                        if (texture.id != 0) {
                            ImGui::Text("%dx%d", texture.width, texture.height);
                        }
                    }

                    Reflection::ImGuiRenderer::RenderProperties(sprite,
                        [this](const char* label, entt::entity* entity) {
                            return RenderEntityPicker(label, entity);
                        },
                        [this](const char* label, UUID* uuid, const std::string& assetType) {
                            return RenderAssetPicker(label, uuid, assetType);
                        });

                    ImGui::TreePop();
                }

                if (removeSprite) {
                    m_CommandHistory.ExecuteCommand(
                        std::make_unique<RemoveComponentCommand<Sprite>>(&registry, inspectedEntity)
                    );
                }
            }

            if (registry.all_of<RigidBody2D>(inspectedEntity)) {
                ImGui::Separator();
                bool removeRigidBody = false;

                ImGui::AlignTextToFramePadding();
                bool rigidBodyOpen = ImGui::TreeNodeEx("Rigid Body 2D", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 25);
                if (ImGui::SmallButton("X##RemoveRigidBody")) {
                    removeRigidBody = true;
                }

                if (rigidBodyOpen) {
                    RigidBody2D& rb = registry.get<RigidBody2D>(inspectedEntity);

                    const char* bodyTypes[] = {"Static", "Dynamic", "Kinematic"};
                    int currentType = static_cast<int>(rb.type);
                    if (ImGui::Combo("Type", &currentType, bodyTypes, 3)) {
                        rb.type = static_cast<BodyType>(currentType);
                    }

                    Reflection::ImGuiRenderer::RenderProperties(rb, [this](const char* label, entt::entity* entity) {
                        return RenderEntityPicker(label, entity);
                    });

                    ImGui::TreePop();
                }

                if (removeRigidBody) {
                    m_CommandHistory.ExecuteCommand(
                        std::make_unique<RemoveComponentCommand<RigidBody2D>>(&registry, inspectedEntity)
                    );
                }
            }

            if (registry.all_of<BoxCollider2D>(inspectedEntity)) {
                ImGui::Separator();
                bool removeBoxCollider = false;

                ImGui::AlignTextToFramePadding();
                bool boxColliderOpen = ImGui::TreeNodeEx("Box Collider 2D", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 25);
                if (ImGui::SmallButton("X##RemoveBoxCollider")) {
                    removeBoxCollider = true;
                }

                if (boxColliderOpen) {
                    BoxCollider2D& collider = registry.get<BoxCollider2D>(inspectedEntity);

                    Reflection::ImGuiRenderer::RenderProperties(collider, [this](const char* label, entt::entity* entity) {
                        return RenderEntityPicker(label, entity);
                    });

                    if (registry.all_of<Sprite>(inspectedEntity)) {
                        if (ImGui::Button("Fit to Sprite")) {
                            const Sprite& sprite = registry.get<Sprite>(inspectedEntity);
                            Vector2 spriteSize = sprite.GetSize();
                            collider.size = spriteSize;
                            collider.offset = Vector2{0.0f, 0.0f};
                        }
                    }

                    ImGui::TreePop();
                }

                if (removeBoxCollider) {
                    m_CommandHistory.ExecuteCommand(
                        std::make_unique<RemoveComponentCommand<BoxCollider2D>>(&registry, inspectedEntity)
                    );
                }
            }

            if (registry.all_of<Script>(inspectedEntity)) {
                ImGui::Separator();
                bool removeScript = false;

                ImGui::AlignTextToFramePadding();
                bool scriptOpen = ImGui::TreeNodeEx("Script", ImGuiTreeNodeFlags_DefaultOpen);

                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
                if (ImGui::SmallButton("X##RemoveScript")) {
                    removeScript = true;
                }

                if (scriptOpen) {
                    Script& script = registry.get<Script>(inspectedEntity);

                    if (script.instance) {
                        ImGui::Text("Script: %s", script.scriptName.c_str());
                        ImGui::Checkbox("Enabled", &script.instance->m_Enabled);
                        ImGui::Spacing();

                        const Reflection::TypeInfo* typeInfo = Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));
                        if (typeInfo) {
                            for (const Reflection::FieldInfo& field : typeInfo->GetFields()) {
                                if (field.flags & Reflection::FieldFlags::ReadOnly) continue;
                                void* fieldPtr = field.getPtr(static_cast<void*>(script.instance.get()));
                                Reflection::ImGuiRenderer::RenderField(field, fieldPtr,
                                    [this](const char* label, entt::entity* entity) {
                                        return RenderEntityPicker(label, entity);
                                    },
                                    [this](const char* label, UUID* uuid, const std::string& assetType) {
                                        return RenderAssetPicker(label, uuid, assetType);
                                    });
                            }
                        }
                    } else {
                        ImGui::TextColored(ImVec4{1.0f, 0.5f, 0.0f, 1.0f}, "No script instance");

                        if (m_Engine && m_Engine->GetScriptSystem()) {
                            ScriptSystem* scriptSystem = m_Engine->GetScriptSystem();
                            const auto& registeredScripts = ScriptRegistry::Instance().GetAllScripts();

                            if (ImGui::BeginCombo("##SelectScript", "Select Script...")) {
                                for (const auto& [name, factory] : registeredScripts) {
                                    if (ImGui::Selectable(name.c_str())) {
                                        script.instance = scriptSystem->CreateScript(name);
                                        script.scriptName = name;
                                        if (script.instance) {
                                            script.instance->Initialize(inspectedEntity, m_Engine->GetActiveScene());
                                        }
                                    }
                                }
                                ImGui::EndCombo();
                            }
                        }
                    }

                    ImGui::TreePop();
                }

                if (removeScript) {
                    registry.remove<Script>(inspectedEntity);
                }
            }

            if (registry.all_of<Animator>(inspectedEntity)) {
                ImGui::Separator();
                bool removeAnimator = false;

                ImGui::AlignTextToFramePadding();
                bool animatorOpen = ImGui::TreeNodeEx("Animator", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 25);
                if (ImGui::SmallButton("X##RemoveAnimator")) {
                    removeAnimator = true;
                }

                if (animatorOpen) {
                    Animator& animator = registry.get<Animator>(inspectedEntity);

                    if (RenderAssetPicker("Controller", &animator.controllerUUID, "AnimatorController")) {
                    }

                    ImGui::Checkbox("Is Playing", &animator.isPlaying);
                    ImGui::DragFloat("Playback Speed", &animator.playbackSpeed, 0.01f, 0.0f, 10.0f);

                    if (animator.controllerUUID.Get() != 0) {
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "State: %s", animator.currentState.empty() ? "(None)" : animator.currentState.c_str());
                        ImGui::Text("Time: %.2f", animator.stateTime);
                        ImGui::Text("Frame: %zu", animator.currentFrameIndex);
                    }

                    ImGui::TreePop();
                }

                if (removeAnimator) {
                    m_CommandHistory.ExecuteCommand(
                        std::make_unique<RemoveComponentCommand<Animator>>(&registry, inspectedEntity)
                    );
                }
            }

            ImGui::Separator();

            if (ImGui::Button("Add Component")) {
                ImGui::OpenPopup("AddComponentPopup");
            }

            if (ImGui::BeginPopup("AddComponentPopup")) {
                if (ImGui::MenuItem("Camera")) {
                    if (!registry.all_of<Camera>(inspectedEntity)) {
                        bool hasPrimaryCamera = false;
                        registry.view<Camera>().each([&hasPrimaryCamera](const Camera& cam) {
                            if (cam.isPrimary) {
                                hasPrimaryCamera = true;
                            }
                        });

                        Camera newCamera{};
                        newCamera.isPrimary = !hasPrimaryCamera;
                        m_CommandHistory.ExecuteCommand(
                            std::make_unique<AddComponentCommand<Camera>>(&registry, inspectedEntity, newCamera)
                        );
                    }
                }
                if (ImGui::MenuItem("Sprite")) {
                    if (!registry.all_of<Sprite>(inspectedEntity)) {
                        m_CommandHistory.ExecuteCommand(
                            std::make_unique<AddComponentCommand<Sprite>>(&registry, inspectedEntity, Sprite{})
                        );
                    }
                }
                if (ImGui::MenuItem("Rigid Body 2D")) {
                    if (!registry.all_of<RigidBody2D>(inspectedEntity)) {
                        m_CommandHistory.ExecuteCommand(
                            std::make_unique<AddComponentCommand<RigidBody2D>>(&registry, inspectedEntity, RigidBody2D{})
                        );
                    }
                }
                if (ImGui::MenuItem("Box Collider 2D")) {
                    if (!registry.all_of<BoxCollider2D>(inspectedEntity)) {
                        BoxCollider2D collider{};

                        if (registry.all_of<Sprite>(inspectedEntity)) {
                            const Sprite& sprite = registry.get<Sprite>(inspectedEntity);
                            collider.size = Vector2{sprite.sourceRect.width, sprite.sourceRect.height};
                        }

                        m_CommandHistory.ExecuteCommand(
                            std::make_unique<AddComponentCommand<BoxCollider2D>>(&registry, inspectedEntity, collider)
                        );
                    }
                }
                if (ImGui::MenuItem("Script")) {
                    if (!registry.all_of<Script>(inspectedEntity)) {
                        registry.emplace<Script>(inspectedEntity);
                    }
                }
                if (ImGui::MenuItem("Animator")) {
                    if (!registry.all_of<Animator>(inspectedEntity)) {
                        m_CommandHistory.ExecuteCommand(
                            std::make_unique<AddComponentCommand<Animator>>(&registry, inspectedEntity, Animator{})
                        );
                    }
                }
                ImGui::EndPopup();
            }
        }
    }

    ImGui::End();
}

void EditorLayer::RenderContentBrowser() {
    PROFILE_FUNCTION();
    ImGui::Begin("Content Browser");

    static std::vector<std::string> directories;
    static std::vector<AssetInfo> files;
    static std::string currentPath = "content";
    static bool needsRefresh = true;
    static AssetInfo* draggedAsset = nullptr;
    static int thumbnailSize = 96;
    static std::string rightClickedItem;
    static bool isRightClickFolder = false;
    static bool showNewScenePopup = false;
    static bool showNewFolderPopup = false;
    static bool showRenamePopup = false;
    static bool showNewSpriteSheetPopup = false;
    static bool showNewAnimClipPopup = false;
    static bool showNewAnimControllerPopup = false;
    static char newItemName[256] = "";

    if (needsRefresh) {
        directories.clear();
        files.clear();

        if (std::filesystem::exists(currentPath)) {
            for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(currentPath)) {
                if (entry.is_directory()) {
                    std::string dirPath = entry.path().string();
                    for (char& c : dirPath) {
                        if (c == '\\') c = '/';
                    }
                    directories.push_back(dirPath);
                } else if (entry.is_regular_file()) {
                    AssetInfo info{};
                    info.path = entry.path().string();
                    for (char& c : info.path) {
                        if (c == '\\') c = '/';
                    }
                    info.filename = entry.path().filename().string();
                    info.extension = entry.path().extension().string();
                    info.fileSize = std::filesystem::file_size(entry.path());

                    if (info.extension == ".pxa" || info.extension == ".package") {
                        continue;
                    }

                    if (info.extension == ".png" || info.extension == ".jpg" || info.extension == ".jpeg" || info.extension == ".bmp" || info.extension == ".tga") {
                        info.type = "texture";
                        Image img = LoadImage(info.path.c_str());
                        if (img.data != nullptr) {
                            info.width = img.width;
                            info.height = img.height;
                            UnloadImage(img);
                        }
                    } else if (info.extension == ".wav" || info.extension == ".ogg" || info.extension == ".mp3") {
                        info.type = "audio";
                    } else if (info.extension == ".scene") {
                        info.type = "scene";
                    } else if (info.extension == ".spritesheet") {
                        info.type = "spritesheet";
                    } else if (info.extension == ".animclip") {
                        info.type = "animclip";
                    } else if (info.extension == ".animcontroller") {
                        info.type = "animcontroller";
                    } else {
                        info.type = "unknown";
                    }

                    files.push_back(info);
                }
            }
        }
        needsRefresh = false;
    }

    if (ImGui::Button("<-") && currentPath != "content") {
        size_t lastSlash = currentPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            currentPath = currentPath.substr(0, lastSlash);
        } else {
            currentPath = "content";
        }
        needsRefresh = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        needsRefresh = true;
    }

    ImGui::SameLine();
    ImGui::Text("Path: %s", currentPath.c_str());

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderInt("Size", &thumbnailSize, 64, 256);

    ImGui::Separator();

    if (directories.empty() && files.empty()) {
        ImVec2 windowSize = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize("Empty folder");
        ImGui::SetCursorPos(ImVec2{
            (windowSize.x - textSize.x) * 0.5f,
            (windowSize.y - textSize.y) * 0.5f
        });
        ImGui::TextDisabled("Empty folder");
    } else {
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = static_cast<int>(panelWidth / (thumbnailSize + 10));
        if (columnCount < 1) columnCount = 1;

        ImGui::Columns(columnCount, nullptr, false);

        for (size_t dirIdx = 0; dirIdx < directories.size(); ++dirIdx) {
            const std::string& dirPath = directories[dirIdx];
            std::string dirName = dirPath;
            size_t lastSlash = dirPath.find_last_of('/');
            if (lastSlash != std::string::npos) {
                dirName = dirPath.substr(lastSlash + 1);
            }

            ImGui::PushID(static_cast<int>(dirIdx));
            ImGui::BeginGroup();

            ImVec4 folderColor{1.0f, 0.9f, 0.4f, 1.0f};
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.3f, 0.3f, 0.3f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.4f, 0.4f, 0.4f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_Text, folderColor);

            if (ImGui::Button(dirName.c_str(), ImVec2{static_cast<float>(thumbnailSize), static_cast<float>(thumbnailSize)})) {
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    currentPath = dirPath;
                    needsRefresh = true;
                }
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                currentPath = dirPath;
                needsRefresh = true;
            }

            if (ImGui::IsItemHovered() && ImGui::IsKeyPressed(ImGuiKey_F2)) {
                rightClickedItem = dirPath;
                isRightClickFolder = true;
                showRenamePopup = true;
                std::string itemName = dirName;
                std::memcpy(newItemName, itemName.c_str(), std::min(itemName.size(), sizeof(newItemName) - 1));
                newItemName[sizeof(newItemName) - 1] = '\0';
            }

            if (ImGui::BeginPopupContextItem()) {
                rightClickedItem = dirPath;
                isRightClickFolder = true;

                if (ImGui::MenuItem("Rename")) {
                    showRenamePopup = true;
                    std::string itemName = dirName;
                    std::memcpy(newItemName, itemName.c_str(), std::min(itemName.size(), sizeof(newItemName) - 1));
                    newItemName[sizeof(newItemName) - 1] = '\0';
                }

                if (ImGui::MenuItem("Delete")) {
                    try {
                        std::filesystem::remove_all(dirPath);
                        needsRefresh = true;
                        TraceLog(LOG_INFO, "Deleted: %s", dirPath.c_str());
                    } catch (const std::filesystem::filesystem_error& e) {
                        TraceLog(LOG_ERROR, "Failed to delete folder: %s", e.what());
                    }
                }

                ImGui::EndPopup();
            }

            ImGui::PopStyleColor(3);
            ImGui::TextWrapped("%s", dirName.c_str());
            ImGui::EndGroup();
            ImGui::NextColumn();
            ImGui::PopID();
        }

        for (size_t fileIdx = 0; fileIdx < files.size(); ++fileIdx) {
            AssetInfo& asset = files[fileIdx];
            ImGui::PushID(static_cast<int>(directories.size() + fileIdx));
            ImGui::BeginGroup();

            bool isScene = asset.extension == ".scene";

            if (asset.type == "texture") {
                Texture2D tex = AssetManager::Instance().LoadTexture(asset.path);

                if (tex.id != 0) {
                    float aspectRatio = static_cast<float>(tex.width) / static_cast<float>(tex.height);
                    float displayWidth = static_cast<float>(thumbnailSize);
                    float displayHeight = static_cast<float>(thumbnailSize);

                    if (aspectRatio > 1.0f) {
                        displayHeight = displayWidth / aspectRatio;
                    } else {
                        displayWidth = displayHeight * aspectRatio;
                    }

                    ImVec2 imageSize{displayWidth, displayHeight};
                    ImTextureID texId = static_cast<ImTextureID>(static_cast<intptr_t>(tex.id));

                    if (ImGui::ImageButton("##texture", texId, imageSize)) {
                        m_SelectedAssetPath = asset.path;

                        UUID existingUUID = AssetRegistry::Instance().GetUUIDFromPath(asset.path);
                        std::shared_ptr<Asset> existingAsset = existingUUID.Get() != 0
                            ? AssetRegistry::Instance().GetAsset(existingUUID)
                            : nullptr;

                        if (!existingAsset) {
                            std::shared_ptr<Asset> loadedAsset = AssetRegistry::Instance().LoadAssetFromPath(asset.path);
                            if (loadedAsset) {
                                m_SelectedAssetUUID = loadedAsset->GetUUID();
                            }
                        } else {
                            m_SelectedAssetUUID = existingUUID;
                        }

                        m_SelectedEntity = entt::null;
                    }

                    if (m_SelectedAssetPath == asset.path && ImGui::IsKeyPressed(ImGuiKey_F2)) {
                        rightClickedItem = asset.path;
                        isRightClickFolder = false;
                        showRenamePopup = true;
                        std::string itemName = asset.filename;
                        size_t dotPos = itemName.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            itemName = itemName.substr(0, dotPos);
                        }
                        std::memcpy(newItemName, itemName.c_str(), std::min(itemName.size(), sizeof(newItemName) - 1));
                        newItemName[sizeof(newItemName) - 1] = '\0';
                    }

                    if (ImGui::BeginPopupContextItem()) {
                        rightClickedItem = asset.path;
                        isRightClickFolder = false;

                        if (ImGui::MenuItem("Rename")) {
                            showRenamePopup = true;
                            std::string itemName = asset.filename;
                            size_t dotPos = itemName.find_last_of('.');
                            if (dotPos != std::string::npos) {
                                itemName = itemName.substr(0, dotPos);
                            }
                            std::memcpy(newItemName, itemName.c_str(), std::min(itemName.size(), sizeof(newItemName) - 1));
                            newItemName[sizeof(newItemName) - 1] = '\0';
                        }

                        if (ImGui::MenuItem("Delete")) {
                            try {
                                std::filesystem::remove(asset.path);
                                needsRefresh = true;
                                TraceLog(LOG_INFO, "Deleted: %s", asset.path.c_str());
                            } catch (const std::filesystem::filesystem_error& e) {
                                TraceLog(LOG_ERROR, "Failed to delete: %s", e.what());
                            }
                        }

                        ImGui::EndPopup();
                    }

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        draggedAsset = &asset;
                        ImGui::SetDragDropPayload("ASSET_TEXTURE", &draggedAsset, sizeof(AssetInfo*));
                        ImGui::Image(texId, ImVec2{64, 64});
                        ImGui::Text("%s", asset.filename.c_str());
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("File: %s", asset.filename.c_str());
                        ImGui::Text("Size: %dx%d", asset.width, asset.height);
                        ImGui::Text("Format: %s", asset.extension.c_str());
                        ImGui::Text("File size: %.2f KB", asset.fileSize / 1024.0f);
                        ImGui::EndTooltip();
                    }
                }
            } else if (isScene) {
                ImVec4 sceneColor{0.4f, 0.9f, 1.0f, 1.0f};
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.3f, 0.4f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.4f, 0.5f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_Text, sceneColor);

                if (ImGui::Button("SCENE", ImVec2{static_cast<float>(thumbnailSize), static_cast<float>(thumbnailSize)})) {
                    m_SelectedAssetPath = asset.path;
                    m_SelectedAssetUUID = UUID{0};
                    m_SelectedEntity = entt::null;
                }

                if (m_SelectedAssetPath == asset.path && ImGui::IsKeyPressed(ImGuiKey_F2)) {
                    rightClickedItem = asset.path;
                    isRightClickFolder = false;
                    showRenamePopup = true;
                    std::string itemName = asset.filename;
                    size_t dotPos = itemName.find_last_of('.');
                    if (dotPos != std::string::npos) {
                        itemName = itemName.substr(0, dotPos);
                    }
                    std::memcpy(newItemName, itemName.c_str(), std::min(itemName.size(), sizeof(newItemName) - 1));
                    newItemName[sizeof(newItemName) - 1] = '\0';
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (m_Engine && m_Engine->GetActiveScene()) {
                        Scene* scene = m_Engine->GetActiveScene();
                        SceneSerializer serializer{scene};
                        if (serializer.Deserialize(asset.path)) {
                            m_CurrentScenePath = asset.path;
                            m_SelectedEntity = entt::null;
                            m_CommandHistory.Clear();
                            RestoreScriptPropertiesFromFile(asset.path);
                            TraceLog(LOG_INFO, "Loaded scene: %s", asset.path.c_str());
                        }
                    }
                }

                if (ImGui::BeginPopupContextItem()) {
                    rightClickedItem = asset.path;
                    isRightClickFolder = false;

                    if (ImGui::MenuItem("Rename")) {
                        showRenamePopup = true;
                        std::string itemName = asset.filename;
                        size_t dotPos = itemName.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            itemName = itemName.substr(0, dotPos);
                        }
                        std::memcpy(newItemName, itemName.c_str(), std::min(itemName.size(), sizeof(newItemName) - 1));
                        newItemName[sizeof(newItemName) - 1] = '\0';
                    }

                    if (ImGui::MenuItem("Delete")) {
                        try {
                            std::filesystem::remove(asset.path);
                            needsRefresh = true;
                            TraceLog(LOG_INFO, "Deleted: %s", asset.path.c_str());
                        } catch (const std::filesystem::filesystem_error& e) {
                            TraceLog(LOG_ERROR, "Failed to delete: %s", e.what());
                        }
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopStyleColor(3);
            } else if (asset.type == "spritesheet" || asset.type == "animclip" || asset.type == "animcontroller") {
                ImVec4 animColor = asset.type == "spritesheet" ? ImVec4{0.4f, 0.8f, 0.6f, 1.0f} :
                                   asset.type == "animclip" ? ImVec4{0.6f, 0.8f, 0.4f, 1.0f} :
                                   ImVec4{0.8f, 0.6f, 0.4f, 1.0f};
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.3f, 0.3f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.4f, 0.4f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_Text, animColor);

                std::string buttonLabel = asset.type == "spritesheet" ? "SHEET" :
                                         asset.type == "animclip" ? "CLIP" : "CTRL";
                if (ImGui::Button(buttonLabel.c_str(), ImVec2{static_cast<float>(thumbnailSize), static_cast<float>(thumbnailSize)})) {
                    m_SelectedAssetPath = asset.path;
                    m_SelectedEntity = entt::null;

                    UUID existingUUID = AssetRegistry::Instance().GetUUIDFromPath(asset.path);
                    std::shared_ptr<Asset> existingAsset = existingUUID.Get() != 0
                        ? AssetRegistry::Instance().GetAsset(existingUUID)
                        : nullptr;

                    if (!existingAsset) {
                        std::shared_ptr<Asset> loadedAsset = AssetRegistry::Instance().LoadAssetFromPath(asset.path);
                        if (loadedAsset) {
                            m_SelectedAssetUUID = loadedAsset->GetUUID();
                        } else {
                            m_SelectedAssetUUID = UUID{0};
                        }
                    } else {
                        m_SelectedAssetUUID = existingUUID;
                    }
                }

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    draggedAsset = &asset;
                    ImGui::SetDragDropPayload("ASSET_ANIM", &draggedAsset, sizeof(AssetInfo*));
                    ImGui::Text("%s: %s", buttonLabel.c_str(), asset.filename.c_str());
                    ImGui::EndDragDropSource();
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (asset.type == "spritesheet" && m_SpriteSheetEditor) {
                        m_SpriteSheetEditor->Open(asset.path);
                    } else if (asset.type == "animclip" && m_AnimationClipEditor) {
                        m_AnimationClipEditor->Open(asset.path);
                    } else if (asset.type == "animcontroller" && m_AnimatorControllerEditor) {
                        m_AnimatorControllerEditor->Open(asset.path);
                    }
                }

                if (ImGui::BeginPopupContextItem()) {
                    rightClickedItem = asset.path;
                    isRightClickFolder = false;

                    if (ImGui::MenuItem("Rename")) {
                        showRenamePopup = true;
                        std::string itemName = asset.filename;
                        size_t dotPos = itemName.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            itemName = itemName.substr(0, dotPos);
                        }
                        std::memcpy(newItemName, itemName.c_str(), std::min(itemName.size(), sizeof(newItemName) - 1));
                        newItemName[sizeof(newItemName) - 1] = '\0';
                    }

                    if (ImGui::MenuItem("Delete")) {
                        try {
                            std::filesystem::remove(asset.path);
                            needsRefresh = true;
                            TraceLog(LOG_INFO, "Deleted: %s", asset.path.c_str());
                        } catch (const std::filesystem::filesystem_error& e) {
                            TraceLog(LOG_ERROR, "Failed to delete: %s", e.what());
                        }
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopStyleColor(3);
            } else {
                ImVec4 fileColor = asset.type == "audio" ? ImVec4{0.8f, 0.4f, 0.8f, 1.0f} : ImVec4{0.6f, 0.6f, 0.6f, 1.0f};
                ImGui::PushStyleColor(ImGuiCol_Text, fileColor);

                std::string buttonLabel = asset.type == "audio" ? "AUDIO" : "FILE";
                if (ImGui::Button(buttonLabel.c_str(), ImVec2{static_cast<float>(thumbnailSize), static_cast<float>(thumbnailSize)})) {
                    m_SelectedAssetPath = asset.path;

                    UUID existingUUID = AssetRegistry::Instance().GetUUIDFromPath(asset.path);
                    std::shared_ptr<Asset> existingAsset = existingUUID.Get() != 0
                        ? AssetRegistry::Instance().GetAsset(existingUUID)
                        : nullptr;

                    if (!existingAsset) {
                        std::shared_ptr<Asset> loadedAsset = AssetRegistry::Instance().LoadAssetFromPath(asset.path);
                        if (loadedAsset) {
                            m_SelectedAssetUUID = loadedAsset->GetUUID();
                        }
                    } else {
                        m_SelectedAssetUUID = existingUUID;
                    }

                    m_SelectedEntity = entt::null;
                }

                if (m_SelectedAssetPath == asset.path && ImGui::IsKeyPressed(ImGuiKey_F2)) {
                    rightClickedItem = asset.path;
                    isRightClickFolder = false;
                    showRenamePopup = true;
                    std::string itemName = asset.filename;
                    size_t dotPos = itemName.find_last_of('.');
                    if (dotPos != std::string::npos) {
                        itemName = itemName.substr(0, dotPos);
                    }
                    std::memcpy(newItemName, itemName.c_str(), std::min(itemName.size(), sizeof(newItemName) - 1));
                    newItemName[sizeof(newItemName) - 1] = '\0';
                }

                ImGui::PopStyleColor();

                if (ImGui::BeginPopupContextItem()) {
                    rightClickedItem = asset.path;
                    isRightClickFolder = false;

                    if (ImGui::MenuItem("Rename")) {
                        showRenamePopup = true;
                        std::string itemName = asset.filename;
                        size_t dotPos = itemName.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            itemName = itemName.substr(0, dotPos);
                        }
                        std::memcpy(newItemName, itemName.c_str(), std::min(itemName.size(), sizeof(newItemName) - 1));
                        newItemName[sizeof(newItemName) - 1] = '\0';
                    }

                    if (ImGui::MenuItem("Delete")) {
                        try {
                            std::filesystem::remove(asset.path);
                            needsRefresh = true;
                            TraceLog(LOG_INFO, "Deleted: %s", asset.path.c_str());
                        } catch (const std::filesystem::filesystem_error& e) {
                            TraceLog(LOG_ERROR, "Failed to delete: %s", e.what());
                        }
                    }

                    ImGui::EndPopup();
                }
            }

            ImGui::TextWrapped("%s", asset.filename.c_str());
            ImGui::EndGroup();
            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
    }

    if (ImGui::BeginPopupContextWindow("ContentContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        rightClickedItem.clear();

        if (ImGui::BeginMenu("Scene")) {
            if (ImGui::MenuItem("New Scene")) {
                showNewScenePopup = true;
                std::memset(newItemName, 0, sizeof(newItemName));
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Animation")) {
            if (ImGui::MenuItem("Sprite Sheet")) {
                showNewSpriteSheetPopup = true;
                std::memset(newItemName, 0, sizeof(newItemName));
            }

            if (ImGui::MenuItem("Animation Clip")) {
                showNewAnimClipPopup = true;
                std::memset(newItemName, 0, sizeof(newItemName));
            }

            if (ImGui::MenuItem("Animator Controller")) {
                showNewAnimControllerPopup = true;
                std::memset(newItemName, 0, sizeof(newItemName));
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("New Folder")) {
            showNewFolderPopup = true;
            std::memset(newItemName, 0, sizeof(newItemName));
        }

        ImGui::EndPopup();
    }

    if (showNewScenePopup) {
        ImGui::OpenPopup("New Scene");
        showNewScenePopup = false;
    }

    if (ImGui::BeginPopupModal("New Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter scene name:");
        ImGui::Separator();

        bool enterPressed = ImGui::InputText("##SceneName", newItemName, sizeof(newItemName), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2{120, 0}) || enterPressed) {
            if (std::strlen(newItemName) > 0) {
                Scene* scene = m_Engine->GetActiveScene();
                if (scene) {
                    std::string sceneName = std::string(newItemName);
                    std::string newScenePath = currentPath + "/" + sceneName + ".scene";

                    scene->SetName(sceneName);
                    entt::registry& registry = scene->GetRegistry();
                    registry.clear();

                    SceneSerializer serializer{scene};
                    serializer.Serialize(newScenePath);

                    m_CurrentScenePath = newScenePath;
                    needsRefresh = true;
                    TraceLog(LOG_INFO, "Created new scene: %s", newScenePath.c_str());
                }
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2{120, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (showNewFolderPopup) {
        ImGui::OpenPopup("New Folder");
        showNewFolderPopup = false;
    }

    if (ImGui::BeginPopupModal("New Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter folder name:");
        ImGui::Separator();

        bool enterPressed = ImGui::InputText("##FolderName", newItemName, sizeof(newItemName), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2{120, 0}) || enterPressed) {
            if (std::strlen(newItemName) > 0) {
                std::string folderName = std::string(newItemName);
                std::string newFolderPath = currentPath + "/" + folderName;

                try {
                    std::filesystem::create_directory(newFolderPath);
                    needsRefresh = true;
                    TraceLog(LOG_INFO, "Created folder: %s", newFolderPath.c_str());
                } catch (const std::filesystem::filesystem_error& e) {
                    TraceLog(LOG_ERROR, "Failed to create folder: %s", e.what());
                }
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2{120, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (showNewSpriteSheetPopup) {
        ImGui::OpenPopup("New Sprite Sheet");
        showNewSpriteSheetPopup = false;
    }

    if (ImGui::BeginPopupModal("New Sprite Sheet", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter sprite sheet name:");
        ImGui::Separator();

        bool enterPressed = ImGui::InputText("##SpriteSheetName", newItemName, sizeof(newItemName), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2{120, 0}) || enterPressed) {
            if (std::strlen(newItemName) > 0) {
                std::string name = std::string(newItemName);
                std::string newPath = currentPath + "/" + name + ".spritesheet";
                SpriteSheet spriteSheet{UUID{}, name};
                AnimationSerializer::SerializeSpriteSheet(spriteSheet, newPath);

                AssetImporter importer{};
                importer.ImportAsset(newPath);

                needsRefresh = true;
                TraceLog(LOG_INFO, "Created sprite sheet: %s", newPath.c_str());
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2{120, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (showNewAnimClipPopup) {
        ImGui::OpenPopup("New Animation Clip");
        showNewAnimClipPopup = false;
    }

    if (ImGui::BeginPopupModal("New Animation Clip", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter animation clip name:");
        ImGui::Separator();

        bool enterPressed = ImGui::InputText("##AnimClipName", newItemName, sizeof(newItemName), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2{120, 0}) || enterPressed) {
            if (std::strlen(newItemName) > 0) {
                std::string name = std::string(newItemName);
                std::string newPath = currentPath + "/" + name + ".animclip";
                AnimationClip clip{UUID{}, name};
                AnimationSerializer::SerializeAnimationClip(clip, newPath);

                AssetImporter importer{};
                importer.ImportAsset(newPath);

                needsRefresh = true;
                TraceLog(LOG_INFO, "Created animation clip: %s", newPath.c_str());
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2{120, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (showNewAnimControllerPopup) {
        ImGui::OpenPopup("New Animator Controller");
        showNewAnimControllerPopup = false;
    }

    if (ImGui::BeginPopupModal("New Animator Controller", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter animator controller name:");
        ImGui::Separator();

        bool enterPressed = ImGui::InputText("##AnimControllerName", newItemName, sizeof(newItemName), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2{120, 0}) || enterPressed) {
            if (std::strlen(newItemName) > 0) {
                std::string name = std::string(newItemName);
                std::string newPath = currentPath + "/" + name + ".animcontroller";
                AnimatorController controller{UUID{}, name};
                AnimationSerializer::SerializeAnimatorController(controller, newPath);

                AssetImporter importer{};
                importer.ImportAsset(newPath);

                needsRefresh = true;
                TraceLog(LOG_INFO, "Created animator controller: %s", newPath.c_str());
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2{120, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (showRenamePopup) {
        ImGui::OpenPopup("Rename");
        showRenamePopup = false;
    }

    if (ImGui::BeginPopupModal("Rename", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter new name:");
        ImGui::Separator();

        bool enterPressed = ImGui::InputText("##RenameName", newItemName, sizeof(newItemName), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2{120, 0}) || enterPressed) {
            if (std::strlen(newItemName) > 0 && !rightClickedItem.empty()) {
                std::string newName = std::string(newItemName);
                std::string parentPath = currentPath;

                size_t lastSlash = rightClickedItem.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    parentPath = rightClickedItem.substr(0, lastSlash);
                }

                std::string newPath;
                if (isRightClickFolder) {
                    newPath = parentPath + "/" + newName;
                } else {
                    std::string extension;
                    size_t dotPos = rightClickedItem.find_last_of('.');
                    if (dotPos != std::string::npos) {
                        extension = rightClickedItem.substr(dotPos);
                    }
                    newPath = parentPath + "/" + newName + extension;
                }

                try {
                    std::filesystem::rename(rightClickedItem, newPath);
                    needsRefresh = true;
                    TraceLog(LOG_INFO, "Renamed: %s -> %s", rightClickedItem.c_str(), newPath.c_str());
                    rightClickedItem.clear();
                } catch (const std::filesystem::filesystem_error& e) {
                    TraceLog(LOG_ERROR, "Failed to rename: %s", e.what());
                }

                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2{120, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::End();
}

void EditorLayer::RenderConsole() {
    PROFILE_FUNCTION();
    if (!ImGui::Begin("Console")) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear")) {
        ConsoleLogger::Instance().Clear();
        m_ConsoleSelectedLines.clear();
        m_ConsoleLastClickedLine = -1;
    }

    ImGui::SameLine();
    if (ImGui::Button("Copy Selected")) {
        if (!m_ConsoleSelectedLines.empty()) {
            std::string clipboardText;
            const auto& logs = ConsoleLogger::Instance().GetLogs();

            for (int selectedIdx : m_ConsoleSelectedLines) {
                if (selectedIdx >= 0 && selectedIdx < static_cast<int>(logs.size())) {
                    const auto& log = logs[selectedIdx];

                    const char* sourceStr = (log.source == LogSource::Engine) ? "[ENGINE]" : "[GAME]  ";
                    const char* levelStr;
                    switch (log.level) {
                        case LogLevel::Trace:   levelStr = "[TRACE]"; break;
                        case LogLevel::Debug:   levelStr = "[DEBUG]"; break;
                        case LogLevel::Info:    levelStr = "[INFO] "; break;
                        case LogLevel::Warning: levelStr = "[WARN] "; break;
                        case LogLevel::Error:   levelStr = "[ERROR]"; break;
                        default:                levelStr = "[?????]"; break;
                    }

                    clipboardText += sourceStr;
                    clipboardText += " ";
                    clipboardText += levelStr;
                    clipboardText += " ";
                    clipboardText += log.message;
                    clipboardText += "\n";
                }
            }

            ImGui::SetClipboardText(clipboardText.c_str());
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_ConsoleAutoScroll);

    ImGui::SameLine();
    ImGui::Separator();

    ImGui::SameLine();
    ImGui::Text("Show:");

    ImGui::SameLine();
    ImGui::Checkbox("Trace", &m_ConsoleShowTrace);

    ImGui::SameLine();
    ImGui::Checkbox("Debug", &m_ConsoleShowDebug);

    ImGui::SameLine();
    ImGui::Checkbox("Info", &m_ConsoleShowInfo);

    ImGui::SameLine();
    ImGui::Checkbox("Warning", &m_ConsoleShowWarning);

    ImGui::SameLine();
    ImGui::Checkbox("Error", &m_ConsoleShowError);

    ImGui::Separator();

    if (ImGui::BeginTabBar("ConsoleSourceTabs")) {
        if (ImGui::BeginTabItem("All")) {
            m_ConsoleSelectedTab = 0;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Engine")) {
            m_ConsoleSelectedTab = 1;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Game")) {
            m_ConsoleSelectedTab = 2;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2{0, 0}, false, ImGuiWindowFlags_HorizontalScrollbar);

    const auto& logs = ConsoleLogger::Instance().GetLogs();

    ImGuiIO& io = ImGui::GetIO();
    int visibleLineIndex = 0;

    for (size_t i = 0; i < logs.size(); ++i) {
        const auto& log = logs[i];

        bool showByLevel = false;
        switch (log.level) {
            case LogLevel::Trace:   showByLevel = m_ConsoleShowTrace; break;
            case LogLevel::Debug:   showByLevel = m_ConsoleShowDebug; break;
            case LogLevel::Info:    showByLevel = m_ConsoleShowInfo; break;
            case LogLevel::Warning: showByLevel = m_ConsoleShowWarning; break;
            case LogLevel::Error:   showByLevel = m_ConsoleShowError; break;
        }

        if (!showByLevel) {
            continue;
        }

        bool showBySource = false;
        if (m_ConsoleSelectedTab == 0) {
            showBySource = true;
        } else if (m_ConsoleSelectedTab == 1 && log.source == LogSource::Engine) {
            showBySource = true;
        } else if (m_ConsoleSelectedTab == 2 && log.source == LogSource::Game) {
            showBySource = true;
        }

        if (!showBySource) {
            continue;
        }

        bool isSelected = std::find(m_ConsoleSelectedLines.begin(), m_ConsoleSelectedLines.end(), static_cast<int>(i)) != m_ConsoleSelectedLines.end();

        ImVec4 bgColor;
        if (isSelected) {
            bgColor = ImVec4{0.3f, 0.5f, 0.8f, 0.5f}; // Blue highlight for selection
        } else if (visibleLineIndex % 2 == 0) {
            bgColor = ImVec4{0.15f, 0.15f, 0.15f, 0.3f}; // lightly darker
        } else {
            bgColor = ImVec4{0.12f, 0.12f, 0.12f, 0.3f}; // Default dark
        }

        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImVec2 lineSize = ImVec2{ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight()};
        ImGui::GetWindowDrawList()->AddRectFilled(cursorPos, ImVec2{cursorPos.x + lineSize.x, cursorPos.y + lineSize.y}, ImGui::ColorConvertFloat4ToU32(bgColor));

        ImGui::PushID(static_cast<int>(i));
        ImGui::Selectable("##line", isSelected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns, ImVec2{0, 0});

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            if (io.KeyCtrl) {
                auto it = std::find(m_ConsoleSelectedLines.begin(), m_ConsoleSelectedLines.end(), static_cast<int>(i));
                if (it != m_ConsoleSelectedLines.end()) {
                    m_ConsoleSelectedLines.erase(it);
                } else {
                    m_ConsoleSelectedLines.push_back(static_cast<int>(i));
                }
            } else if (io.KeyShift && m_ConsoleLastClickedLine != -1) {
                m_ConsoleSelectedLines.clear();
                int start = std::min(m_ConsoleLastClickedLine, static_cast<int>(i));
                int end = std::max(m_ConsoleLastClickedLine, static_cast<int>(i));
                for (int idx = start; idx <= end; ++idx) {
                    m_ConsoleSelectedLines.push_back(idx);
                }
            } else {
                m_ConsoleSelectedLines.clear();
                m_ConsoleSelectedLines.push_back(static_cast<int>(i));
            }
            m_ConsoleLastClickedLine = static_cast<int>(i);
        }

        ImGui::SameLine(0, 0);

        ImVec4 textColor;
        const char* levelStr;
        switch (log.level) {
            case LogLevel::Trace:
                textColor = ImVec4{0.6f, 0.6f, 0.6f, 1.0f};
                levelStr = "[TRACE]";
                break;
            case LogLevel::Debug:
                textColor = ImVec4{0.7f, 0.7f, 0.9f, 1.0f};
                levelStr = "[DEBUG]";
                break;
            case LogLevel::Info:
                textColor = ImVec4{0.9f, 0.9f, 0.9f, 1.0f};
                levelStr = "[INFO] ";
                break;
            case LogLevel::Warning:
                textColor = ImVec4{1.0f, 0.8f, 0.2f, 1.0f};
                levelStr = "[WARN] ";
                break;
            case LogLevel::Error:
                textColor = ImVec4{1.0f, 0.3f, 0.3f, 1.0f};
                levelStr = "[ERROR]";
                break;
            default:
                textColor = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
                levelStr = "[?????]";
                break;
        }

        ImVec4 sourceColor = (log.source == LogSource::Engine)
            ? ImVec4{0.4f, 0.7f, 1.0f, 1.0f}  // Blue for engine
            : ImVec4{0.4f, 1.0f, 0.4f, 1.0f}; // Green for game

        const char* sourceStr = (log.source == LogSource::Engine) ? "[ENGINE]" : "[GAME]  ";

        ImGui::PushStyleColor(ImGuiCol_Text, sourceColor);
        ImGui::Text("%s", sourceStr);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::Text("%s", levelStr);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::TextWrapped("%s", log.message.c_str());
        ImGui::PopStyleColor();

        ImGui::PopID();

        visibleLineIndex++;
    }

    if (m_ConsoleAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    ImGui::End();
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

void EditorLayer::RenderGameViewport() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin("Game");

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

    entt::entity primaryCameraEntity = GetPrimaryCamera();

    if (primaryCameraEntity == entt::null) {
        ImVec2 windowSize = ImGui::GetWindowSize();
        const char* message = "No Primary Camera";
        ImVec2 textSize = ImGui::CalcTextSize(message);
        ImGui::SetCursorPos(ImVec2{
            (windowSize.x - textSize.x) * 0.5f,
            (windowSize.y - textSize.y) * 0.5f
        });
        ImGui::TextDisabled("%s", message);
    }
    else if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
        if (static_cast<int>(viewportPanelSize.x) != m_GameViewportTexture.texture.width ||
            static_cast<int>(viewportPanelSize.y) != m_GameViewportTexture.texture.height) {
            UnloadRenderTexture(m_GameViewportTexture);
            m_GameViewportTexture = LoadRenderTexture(
                static_cast<int>(viewportPanelSize.x),
                static_cast<int>(viewportPanelSize.y)
            );
        }

        BeginTextureMode(m_GameViewportTexture);
        ClearBackground(Color{45, 45, 48, 255});

        Camera2D camera{};
        camera.offset = Vector2{viewportPanelSize.x / 2.0f, viewportPanelSize.y / 2.0f};
        camera.target = Vector2{0.0f, 0.0f};
        camera.rotation = 0.0f;
        camera.zoom = 1.0f;

        if (m_Engine && m_Engine->GetActiveScene()) {
            Scene* scene = m_Engine->GetActiveScene();
            entt::registry& registry = scene->GetRegistry();

            if (registry.valid(primaryCameraEntity) && registry.all_of<Camera, Transform>(primaryCameraEntity)) {
                const Camera& cameraComp = registry.get<Camera>(primaryCameraEntity);
                const Transform& transform = registry.get<Transform>(primaryCameraEntity);

                camera = cameraComp.ToRaylib(transform.position);
                camera.offset = Vector2{viewportPanelSize.x / 2.0f, viewportPanelSize.y / 2.0f};
            }
        }

        BeginMode2D(camera);

        if (m_Engine) {
            RenderSystem* renderSystem = m_Engine->GetRenderSystem();
            bool savedShowColliders = false;
            bool savedShowDebug = false;

            if (renderSystem) {
                savedShowColliders = renderSystem->GetShowColliders();
                savedShowDebug = renderSystem->GetShowDebug();
                renderSystem->SetShowColliders(false);
                renderSystem->SetShowDebug(false);
            }

            m_Engine->Render();

            if (renderSystem) {
                renderSystem->SetShowColliders(savedShowColliders);
                renderSystem->SetShowDebug(savedShowDebug);
            }
        }

        EndMode2D();

        EndTextureMode();

        Rectangle sourceRec{
            0.0f, 0.0f,
            static_cast<float>(m_GameViewportTexture.texture.width),
            -static_cast<float>(m_GameViewportTexture.texture.height)
        };

        rlImGuiImageRect(&m_GameViewportTexture.texture,
                        static_cast<int>(viewportPanelSize.x),
                        static_cast<int>(viewportPanelSize.y),
                        sourceRec);
    }

    if (m_EditorState == EditorState::Play) {
        if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered()) {
            ImGuiIO& io = ImGui::GetIO();
            io.WantCaptureMouse = false;
            io.WantCaptureKeyboard = false;
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorLayer::OnPlayButtonPressed() {
    if (m_Engine && m_Engine->GetActiveScene()) {
        if (m_CurrentScenePath.empty()) {
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

        TraceLog(LOG_INFO, "Play mode started with memory snapshot");
    }
}

void EditorLayer::OnStopButtonPressed() {
    if (m_Engine && m_Engine->GetActiveScene()) {
        m_Engine->SetPhysicsEnabled(false);
        m_Engine->SetScriptsEnabled(false);
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

                                if (registry.all_of<Script>(entity) && entityJson.contains("Script")) {
                                    Script& script = registry.get<Script>(entity);
                                    const nlohmann::json& scriptJson = entityJson["Script"];

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

                                entityIndex++;
                            }
                        }
                    } catch (const nlohmann::json::exception& e) {
                        TraceLog(LOG_ERROR, "Failed to restore script properties: %s", e.what());
                    }
                }

                TraceLog(LOG_INFO, "Scene restored from memory snapshot - edit mode");
            }
        } else {
            m_EditorState = EditorState::Edit;
            m_CommandHistory.Clear();
        }
    }
}

void EditorLayer::NewScene() {
    if (m_Engine && m_Engine->GetActiveScene()) {
        Scene* scene = m_Engine->GetActiveScene();
        entt::registry& registry = scene->GetRegistry();
        registry.clear();

        scene->SetName("Untitled Scene");
        m_CurrentScenePath.clear();
        m_SelectedEntity = entt::null;
        m_CommandHistory.Clear();

        TraceLog(LOG_INFO, "New scene created");
    }
}

void EditorLayer::SaveScene() {
    if (m_CurrentScenePath.empty()) {
        SaveSceneAs();
        return;
    }

    if (m_Engine && m_Engine->GetActiveScene()) {
        Scene* scene = m_Engine->GetActiveScene();
        SceneSerializer serializer{scene};
        serializer.Serialize(m_CurrentScenePath);
    }
}

void EditorLayer::SaveSceneAs() {
    if (!m_Engine || !m_Engine->GetActiveScene()) {
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    std::string filename = scene->GetName();

    for (char& c : filename) {
        if (c == ' ') {
            c = '_';
        }
    }

    m_CurrentScenePath = "content/scenes/" + filename + ".scene";

    SceneSerializer serializer{scene};
    serializer.Serialize(m_CurrentScenePath);
}

void EditorLayer::LoadScene() {
    if (!m_Engine || !m_Engine->GetActiveScene()) {
        return;
    }

    if (m_CurrentScenePath.empty()) {
        TraceLog(LOG_WARNING, "No scene path set. Save the scene first.");
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    SceneSerializer serializer{scene};

    if (serializer.Deserialize(m_CurrentScenePath)) {
        m_SelectedEntity = entt::null;
        m_CommandHistory.Clear();
        RestoreScriptPropertiesFromFile(m_CurrentScenePath);
    }
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
        if (!asset) {
            asset = AssetRegistry::Instance().LoadAsset(*uuid);
        }
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
            std::shared_ptr<Asset> asset = AssetRegistry::Instance().GetAsset(assetUUID);
            if (!asset) {
                asset = AssetRegistry::Instance().LoadAsset(assetUUID);
            }

            if (asset) {
                AssetType assetTypeEnum = asset->GetMetadata().type;
                bool matchesFilter = false;

                if (assetType == "texture" && assetTypeEnum == AssetType::Texture) {
                    matchesFilter = true;
                } else if (assetType == "AnimatorController" && assetTypeEnum == AssetType::AnimatorController) {
                    matchesFilter = true;
                } else if (assetType == "SpriteSheet" && assetTypeEnum == AssetType::SpriteSheet) {
                    matchesFilter = true;
                } else if (assetType == "AnimationClip" && assetTypeEnum == AssetType::AnimationClip) {
                    matchesFilter = true;
                } else if (assetType == "Audio" && assetTypeEnum == AssetType::Audio) {
                    matchesFilter = true;
                }

                if (matchesFilter) {
                    bool isSelected = (uuid->Get() == assetUUID.Get());
                    std::string itemLabel = asset->GetMetadata().name;

                    if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                        *uuid = assetUUID;
                        changed = true;
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
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

            if (registry.all_of<Script>(entity) && entityJson.contains("Script")) {
                Script& script = registry.get<Script>(entity);
                const nlohmann::json& scriptJson = entityJson["Script"];

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

            entityIndex++;
        }
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Failed to restore script properties: %s", e.what());
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

    if (registry.all_of<Transform>(entity)) {
        const Transform& originalTransform = registry.get<Transform>(entity);
        Transform& newTransform = registry.get<Transform>(newEntity);
        newTransform.position = originalTransform.position;
        newTransform.rotation = originalTransform.rotation;
        newTransform.scale = originalTransform.scale;
    }

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

    if (registry.all_of<Camera>(entity)) {
        const Camera& originalCamera = registry.get<Camera>(entity);
        Camera newCamera;
        newCamera.isPrimary = false;
        newCamera.zoom = originalCamera.zoom;
        newCamera.offset = originalCamera.offset;
        newCamera.rotation = originalCamera.rotation;
        registry.emplace<Camera>(newEntity, newCamera);
    }

    if (registry.all_of<RigidBody2D>(entity)) {
        const RigidBody2D& originalRb = registry.get<RigidBody2D>(entity);
        RigidBody2D newRb;
        newRb.type = originalRb.type;
        newRb.mass = originalRb.mass;
        newRb.friction = originalRb.friction;
        newRb.restitution = originalRb.restitution;
        newRb.fixedRotation = originalRb.fixedRotation;
        newRb.velocity = Vector2{0.0f, 0.0f};
        newRb.angularVelocity = 0.0f;
        newRb.box2dBodyId = b2_nullBodyId;
        registry.emplace<RigidBody2D>(newEntity, newRb);
    }

    if (registry.all_of<BoxCollider2D>(entity)) {
        const BoxCollider2D& originalCollider = registry.get<BoxCollider2D>(entity);
        BoxCollider2D newCollider;
        newCollider.size = originalCollider.size;
        newCollider.offset = originalCollider.offset;
        newCollider.isTrigger = originalCollider.isTrigger;
        registry.emplace<BoxCollider2D>(newEntity, newCollider);
    }

    if (registry.all_of<Script>(entity)) {
        const Script& originalScript = registry.get<Script>(entity);
        Script newScript;
        newScript.scriptName = originalScript.scriptName;
        registry.emplace<Script>(newEntity, newScript);
    }

    TraceLog(LOG_INFO, "Entity duplicated with all components");

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
    TraceLog(LOG_INFO, "Entity copied to clipboard");
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

    TraceLog(LOG_INFO, "Entity pasted");
}

void EditorLayer::RenderProfiler() {
    PROFILE_FUNCTION();
    ImGui::Begin("Profiler");

    Profiler& profiler = Profiler::Instance();

    bool enabled = profiler.IsEnabled();
    if (ImGui::Checkbox("Enable Profiler", &enabled)) {
        profiler.SetEnabled(enabled);
    }

    if (!profiler.IsEnabled()) {
        ImGui::End();
        return;
    }

    ImGui::SameLine();
    bool recording = profiler.IsRecording();
    if (ImGui::Checkbox("Record", &recording)) {
        profiler.SetRecording(recording);
    }

    ImGui::SameLine();
    if (ImGui::Button(m_ProfilerPaused ? "Resume" : "Pause")) {
        m_ProfilerPaused = !m_ProfilerPaused;
        if (m_ProfilerPaused) {
            m_ProfilerPausedSnapshot.results = profiler.GetResults();
            m_ProfilerPausedSnapshot.frameTime = profiler.GetFrameTime();
            m_ProfilerPausedSnapshot.fps = profiler.GetFPS();
        }
    }

    const FrameSnapshot* displaySnapshot = nullptr;
    if (m_ProfilerSelectedFrame >= 0) {
        displaySnapshot = &m_ProfilerSelectedFrameSnapshot;
    } else if (m_ProfilerPaused) {
        displaySnapshot = &m_ProfilerPausedSnapshot;
    }

    ImGui::SameLine();
    if (ImGui::Button("Copy Frame")) {
        if (displaySnapshot) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(3);
            ss << "Frame Time: " << displaySnapshot->frameTime << " ms\n";
            ss << "FPS: " << std::setprecision(1) << displaySnapshot->fps << "\n\n";
            ss << std::setprecision(3);
            ss << "Scope Timings:\n";
            ss << "----------------------------------------\n";

            for (const ProfileResult& result : displaySnapshot->results) {
                float percentage = static_cast<float>(result.duration / displaySnapshot->frameTime) * 100.0f;
                ss << std::string(result.depth * 2, ' ');
                ss << result.name << ": " << result.duration << " ms ("
                   << std::setprecision(1) << percentage << "%) ["
                   << result.callCount << " calls]\n";
                ss << std::setprecision(3);
            }

            SetClipboardText(ss.str().c_str());
        } else {
            profiler.CopyFrameToClipboard();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear History")) {
        profiler.ClearHistory();
        m_ProfilerSelectedFrame = -1;
    }

    ImGui::Separator();

    if (displaySnapshot) {
        if (m_ProfilerSelectedFrame >= 0) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Viewing Frame: %d", m_ProfilerSelectedFrame);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "PAUSED");
        }
    }

    if (displaySnapshot) {
        ImGui::Text("Frame Time: %.3f ms", displaySnapshot->frameTime);
        ImGui::Text("FPS: %.1f", displaySnapshot->fps);
    } else {
        ImGui::Text("Frame Time: %.3f ms", profiler.GetFrameTime());
        ImGui::Text("FPS: %.1f", profiler.GetFPS());
    }

    if (profiler.IsRecording()) {
        ImGui::Text("Recorded: %zu frames", profiler.GetFrameHistory().size());
    }

    ImGui::Separator();

    const std::vector<ProfileResult>* currentResults = nullptr;
    double currentFrameTime = 0.0;

    if (displaySnapshot) {
        currentResults = &displaySnapshot->results;
        currentFrameTime = displaySnapshot->frameTime;
    } else {
        currentResults = &profiler.GetResults();
        currentFrameTime = profiler.GetFrameTime();
    }

    if (ImGui::BeginTabBar("ProfilerTabs")) {
        if (ImGui::BeginTabItem("Current Frame")) {
            if (ImGui::BeginTable("ProfilerTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Time (ms)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableHeadersRow();

                for (const ProfileResult& result : *currentResults) {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Indent(result.depth * 15.0f);
                    ImGui::Text("%s", result.name.c_str());
                    ImGui::Unindent(result.depth * 15.0f);

                    ImGui::TableSetColumnIndex(1);
                    float percentage = static_cast<float>(result.duration / currentFrameTime) * 100.0f;
                    ImGui::Text("%.3f (%.1f%%)", result.duration, percentage);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%zu", result.callCount);
                }

                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Flame Graph")) {
            if (m_ProfilerSelectedFrame >= 0) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Viewing Frame: %d", m_ProfilerSelectedFrame);
            } else if (m_ProfilerPaused) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "PAUSED");
            }

            ImGui::Text("Frame Time: %.3f ms", currentFrameTime);
            ImGui::SameLine();
            ImGui::Text("Zoom: %.1fx", m_ProfilerFlameGraphZoom);
            ImGui::SameLine();
            if (ImGui::Button("Reset Zoom")) {
                m_ProfilerFlameGraphZoom = 1.0f;
                m_ProfilerFlameGraphScroll = 0.0f;
            }
            ImGui::SameLine();
            ImGui::Text("Use mouse wheel to zoom, middle-click drag to pan");

            ImVec2 canvasPos = ImGui::GetCursorScreenPos();
            ImVec2 canvasSize = ImGui::GetContentRegionAvail();
            canvasSize.y = 450.0f;

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                                   IM_COL32(20, 20, 20, 255));

            float barHeight = 25.0f;
            float frameTime = static_cast<float>(currentFrameTime);

            ImVec2 mousePos = ImGui::GetMousePos();
            bool isHovering = mousePos.x >= canvasPos.x && mousePos.x <= canvasPos.x + canvasSize.x &&
                             mousePos.y >= canvasPos.y && mousePos.y <= canvasPos.y + canvasSize.y;

            if (isHovering) {
                float mouseWheel = ImGui::GetIO().MouseWheel;
                if (mouseWheel != 0.0f) {
                    float oldZoom = m_ProfilerFlameGraphZoom;
                    m_ProfilerFlameGraphZoom *= (1.0f + mouseWheel * 0.1f);
                    m_ProfilerFlameGraphZoom = std::max(1.0f, std::min(m_ProfilerFlameGraphZoom, 500.0f));

                    float mouseRelativeX = (mousePos.x - canvasPos.x) / canvasSize.x;
                    float worldPosAtMouse = (mouseRelativeX + m_ProfilerFlameGraphScroll) / oldZoom;
                    m_ProfilerFlameGraphScroll = worldPosAtMouse * m_ProfilerFlameGraphZoom - mouseRelativeX;
                }

                if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                    ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
                    m_ProfilerFlameGraphScroll -= delta.x / canvasSize.x;
                    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
                }
            }

            m_ProfilerFlameGraphScroll = std::max(0.0f, std::min(m_ProfilerFlameGraphScroll, m_ProfilerFlameGraphZoom - 1.0f));

            drawList->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);

            const ProfileResult* hoveredResult = nullptr;

            for (const ProfileResult& result : *currentResults) {
                float normalizedStart = static_cast<float>(result.startTime / frameTime);
                float normalizedDuration = static_cast<float>(result.duration / frameTime);

                float xStart = canvasPos.x + (normalizedStart * m_ProfilerFlameGraphZoom - m_ProfilerFlameGraphScroll) * canvasSize.x;
                float width = normalizedDuration * m_ProfilerFlameGraphZoom * canvasSize.x;
                float yStart = canvasPos.y + result.depth * barHeight;

                if (xStart + width < canvasPos.x || xStart > canvasPos.x + canvasSize.x) continue;
                if (width < 0.5f) continue;

                bool isHovered = isHovering &&
                                mousePos.x >= xStart && mousePos.x <= xStart + width &&
                                mousePos.y >= yStart && mousePos.y <= yStart + barHeight - 2;

                if (isHovered) {
                    hoveredResult = &result;
                }

                ImU32 color = IM_COL32(
                    100 + (result.depth * 30) % 155,
                    150 + (result.depth * 40) % 105,
                    200 - (result.depth * 20) % 100,
                    isHovered ? 255 : 200
                );

                drawList->AddRectFilled(
                    ImVec2(xStart, yStart),
                    ImVec2(xStart + width, yStart + barHeight - 2),
                    color
                );

                drawList->AddRect(
                    ImVec2(xStart, yStart),
                    ImVec2(xStart + width, yStart + barHeight - 2),
                    isHovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 255, 255, 100),
                    0.0f,
                    0,
                    isHovered ? 2.0f : 1.0f
                );

                if (width > 30.0f) {
                    drawList->AddText(
                        ImVec2(xStart + 2, yStart + 4),
                        IM_COL32(255, 255, 255, 255),
                        result.name.c_str()
                    );
                }
            }

            drawList->PopClipRect();

            ImGui::Dummy(canvasSize);

            if (hoveredResult) {
                ImGui::BeginTooltip();
                ImGui::Text("Scope: %s", hoveredResult->name.c_str());
                ImGui::Text("Duration: %.3f ms", hoveredResult->duration);
                ImGui::Text("Percentage: %.1f%%", static_cast<float>(hoveredResult->duration / frameTime) * 100.0f);
                ImGui::Text("Calls: %zu", hoveredResult->callCount);
                ImGui::Text("Depth: %d", hoveredResult->depth);
                ImGui::EndTooltip();
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("History")) {
            const std::deque<FrameSnapshot>& history = profiler.GetFrameHistory();

            if (history.empty()) {
                ImGui::Text("No recorded frames. Enable 'Record' to capture frames.");
            } else {
                ImGui::Text("Click on a frame to inspect it. Right-click to deselect.");

                ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                ImVec2 canvasSize = ImGui::GetContentRegionAvail();
                canvasSize.y = 250.0f;

                ImDrawList* drawList = ImGui::GetWindowDrawList();

                drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                                       IM_COL32(20, 20, 20, 255));

                float maxFrameTime = 16.67f;
                for (const auto& snapshot : history) {
                    if (snapshot.frameTime > maxFrameTime) {
                        maxFrameTime = static_cast<float>(snapshot.frameTime);
                    }
                }

                float barWidth = canvasSize.x / static_cast<float>(history.size());

                ImVec2 mousePos = ImGui::GetMousePos();
                bool isHovering = mousePos.x >= canvasPos.x && mousePos.x <= canvasPos.x + canvasSize.x &&
                                 mousePos.y >= canvasPos.y && mousePos.y <= canvasPos.y + canvasSize.y;

                int hoveredFrame = -1;

                for (size_t i = 0; i < history.size(); ++i) {
                    const FrameSnapshot& snapshot = history[i];
                    float x = canvasPos.x + i * barWidth;
                    float height = static_cast<float>(snapshot.frameTime / maxFrameTime) * canvasSize.y;
                    float y = canvasPos.y + canvasSize.y - height;

                    bool isBarHovered = isHovering &&
                                       mousePos.x >= x && mousePos.x <= x + barWidth &&
                                       mousePos.y >= y;

                    if (isBarHovered) {
                        hoveredFrame = static_cast<int>(i);
                    }

                    ImU32 color;
                    if (static_cast<int>(i) == m_ProfilerSelectedFrame) {
                        color = IM_COL32(255, 255, 0, 255);
                    } else if (isBarHovered) {
                        color = snapshot.frameTime > 16.67f ? IM_COL32(255, 150, 150, 255) : IM_COL32(150, 255, 150, 255);
                    } else {
                        color = snapshot.frameTime > 16.67f ? IM_COL32(255, 100, 100, 255) : IM_COL32(100, 255, 100, 255);
                    }

                    drawList->AddRectFilled(
                        ImVec2(x, y),
                        ImVec2(x + barWidth - 1, canvasPos.y + canvasSize.y),
                        color
                    );
                }

                drawList->AddLine(
                    ImVec2(canvasPos.x, canvasPos.y + canvasSize.y - (16.67f / maxFrameTime) * canvasSize.y),
                    ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y - (16.67f / maxFrameTime) * canvasSize.y),
                    IM_COL32(255, 255, 0, 200),
                    2.0f
                );

                ImGui::Dummy(canvasSize);

                if (hoveredFrame >= 0 && hoveredFrame < static_cast<int>(history.size())) {
                    const FrameSnapshot& snapshot = history[hoveredFrame];
                    ImGui::BeginTooltip();
                    ImGui::Text("Frame: %d", hoveredFrame);
                    ImGui::Text("Time: %.3f ms", snapshot.frameTime);
                    ImGui::Text("FPS: %.1f", snapshot.fps);
                    ImGui::EndTooltip();

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        m_ProfilerSelectedFrame = hoveredFrame;
                        m_ProfilerSelectedFrameSnapshot = snapshot;
                    }
                }

                if (isHovering && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    m_ProfilerSelectedFrame = -1;
                }

                ImGui::Text("Yellow line: 60 FPS target (16.67ms)");
                if (m_ProfilerSelectedFrame >= 0) {
                    ImGui::Text("Selected Frame: %d", m_ProfilerSelectedFrame);
                }
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void EditorLayer::RenderBuildPanel() {
    if (m_BuildPanel) {
        m_BuildPanel->Update();
        m_BuildPanel->Render();
    }
}

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR
