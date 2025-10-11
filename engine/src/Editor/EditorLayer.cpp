#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorLayer.hpp"
#include "Editor/ConsoleLogger.hpp"
#include "Core/Engine.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneSerializer.hpp"
#include "Systems/RenderSystem.hpp"
#include "Resources/AssetManager.hpp"
#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Components/Sprite.hpp"
#include "Components/Camera.hpp"
#include "Components/RigidBody2D.hpp"
#include "Components/BoxCollider2D.hpp"
#include "Components/Script.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Systems/ScriptSystem.hpp"
#include "Editor/EditorCommands.hpp"
#include "Project/ProjectSettings.hpp"
#include "Reflection/Reflection.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <rlImGui.h>
#include <entt/entt.hpp>
#include <filesystem>
#include <algorithm>

namespace PiiXeL {

EditorLayer::EditorLayer(Engine* engine)
    : m_Engine{engine}
    , m_ViewportTexture{}
    , m_GameViewportTexture{}
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

    EndDockspace();
}

void EditorLayer::BeginDockspace() {
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

    ImGui::DockBuilderFinish(dockspace_id);
}

void EditorLayer::EndDockspace() {
    ImGui::End();
}

void EditorLayer::RenderMenuBar() {
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
                    entt::registry& registry = scene->GetRegistry();
                    m_CommandHistory.ExecuteCommand(
                        std::make_unique<CreateEntityCommand>(&registry, "Entity")
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
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void EditorLayer::RenderViewport() {
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

                    Texture2D newTexture = AssetManager::Instance().LoadTexture(assetInfo->path);
                    if (newTexture.id != 0) {
                        entt::entity newEntity = registry.create();

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
                        sprite.texture = newTexture;
                        sprite.texturePath = assetInfo->path;
                        sprite.sourceRect = Rectangle{
                            0.0f, 0.0f,
                            static_cast<float>(newTexture.width),
                            static_cast<float>(newTexture.height)
                        };
                        sprite.tint = WHITE;
                        sprite.origin = Vector2{0.5f, 0.5f};
                        registry.emplace<Sprite>(newEntity, sprite);

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
    ImGui::Begin("Hierarchy");

    if (m_Engine && m_Engine->GetActiveScene()) {
        Scene* scene = m_Engine->GetActiveScene();
        entt::registry& registry = scene->GetRegistry();

        if (ImGui::Button("+ Create Entity")) {
            m_CommandHistory.ExecuteCommand(
                std::make_unique<CreateEntityCommand>(&registry, "New Entity")
            );
        }

        ImGui::Separator();

        registry.view<Tag>().each([this, &registry](entt::entity entity, Tag& tag) {
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

            if (ImGui::IsItemClicked()) {
                m_SelectedEntity = entity;
            }

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                    entt::entity newEntity = DuplicateEntity(entity);
                    m_SelectedEntity = newEntity;
                }

                if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                    CopyEntity(entity);
                }

                if (ImGui::MenuItem("Delete", "Delete")) {
                    if (m_SelectedEntity == entity) {
                        m_SelectedEntity = entt::null;
                    }
                    registry.destroy(entity);
                }

                ImGui::EndPopup();
            }
        });
    }

    ImGui::End();
}

void EditorLayer::RenderInspector() {
    ImGui::Begin("Inspector");

    if (m_SelectedEntity != entt::null && m_Engine && m_Engine->GetActiveScene()) {
        Scene* scene = m_Engine->GetActiveScene();
        entt::registry& registry = scene->GetRegistry();

        if (registry.valid(m_SelectedEntity)) {
            if (registry.all_of<Tag>(m_SelectedEntity)) {
                Tag& tag = registry.get<Tag>(m_SelectedEntity);

                char buffer[256]{};
                size_t copyLen = (tag.name.length() < sizeof(buffer) - 1) ? tag.name.length() : sizeof(buffer) - 1;
                std::memcpy(buffer, tag.name.c_str(), copyLen);
                buffer[sizeof(buffer) - 1] = '\0';

                if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
                    tag.name = std::string(buffer);
                }
            }

            ImGui::Separator();

            if (registry.all_of<Transform>(m_SelectedEntity)) {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    Transform& transform = registry.get<Transform>(m_SelectedEntity);

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
                                std::make_unique<ModifyTransformCommand>(&registry, m_SelectedEntity, m_CachedTransform, transform)
                            );
                        }
                        m_IsModifyingTransform = false;
                    }
                }
            }

            if (registry.all_of<Camera>(m_SelectedEntity)) {
                ImGui::Separator();
                bool removeCamera = false;

                ImGui::AlignTextToFramePadding();
                bool cameraOpen = ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 25);
                if (ImGui::SmallButton("X##RemoveCamera")) {
                    removeCamera = true;
                }

                if (cameraOpen) {
                    Camera& camera = registry.get<Camera>(m_SelectedEntity);

                    bool wasPrimary = camera.isPrimary;
                    Reflection::ImGuiRenderer::RenderProperties(camera, [this](const char* label, entt::entity* entity) {
                        return RenderEntityPicker(label, entity);
                    });

                    if (camera.isPrimary != wasPrimary && camera.isPrimary) {
                        registry.view<Camera>().each([this, &registry](entt::entity entity, Camera& otherCamera) {
                            if (entity != m_SelectedEntity) {
                                otherCamera.isPrimary = false;
                            }
                        });
                    }

                    ImGui::TreePop();
                }

                if (removeCamera) {
                    m_CommandHistory.ExecuteCommand(
                        std::make_unique<RemoveComponentCommand<Camera>>(&registry, m_SelectedEntity)
                    );
                }
            }

            if (registry.all_of<Sprite>(m_SelectedEntity)) {
                ImGui::Separator();
                bool removeSprite = false;

                ImGui::AlignTextToFramePadding();
                bool spriteOpen = ImGui::TreeNodeEx("Sprite", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 25);
                if (ImGui::SmallButton("X##RemoveSprite")) {
                    removeSprite = true;
                }

                if (spriteOpen) {
                    Sprite& sprite = registry.get<Sprite>(m_SelectedEntity);

                    ImGui::Text("Texture");
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.2f, 0.2f, 1.0f});

                    if (sprite.IsValid()) {
                        float aspectRatio = static_cast<float>(sprite.texture.width) / static_cast<float>(sprite.texture.height);
                        float previewWidth = 100.0f;
                        float previewHeight = previewWidth / aspectRatio;

                        ImTextureID texId = static_cast<ImTextureID>(static_cast<intptr_t>(sprite.texture.id));

                        if (ImGui::ImageButton("##TexturePreview", texId, ImVec2{previewWidth, previewHeight})) {
                        }
                    } else {
                        if (ImGui::Button("Drop Texture Here", ImVec2{150, 100})) {
                        }
                    }

                    ImGui::PopStyleColor();

                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE")) {
                            AssetInfo* assetInfo = *static_cast<AssetInfo**>(payload->Data);
                            if (assetInfo) {
                                Texture2D newTexture = AssetManager::Instance().LoadTexture(assetInfo->path);
                                if (newTexture.id != 0) {
                                    sprite.texture = newTexture;
                                    sprite.texturePath = assetInfo->path;
                                    sprite.sourceRect = Rectangle{
                                        0.0f, 0.0f,
                                        static_cast<float>(newTexture.width),
                                        static_cast<float>(newTexture.height)
                                    };
                                    TraceLog(LOG_INFO, "Texture assigned: %s", assetInfo->path.c_str());
                                }
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }

                    if (sprite.IsValid()) {
                        ImGui::Text("%dx%d", sprite.texture.width, sprite.texture.height);
                    }

                    Reflection::ImGuiRenderer::RenderProperties(sprite, [this](const char* label, entt::entity* entity) {
                        return RenderEntityPicker(label, entity);
                    });

                    ImGui::TreePop();
                }

                if (removeSprite) {
                    m_CommandHistory.ExecuteCommand(
                        std::make_unique<RemoveComponentCommand<Sprite>>(&registry, m_SelectedEntity)
                    );
                }
            }

            if (registry.all_of<RigidBody2D>(m_SelectedEntity)) {
                ImGui::Separator();
                bool removeRigidBody = false;

                ImGui::AlignTextToFramePadding();
                bool rigidBodyOpen = ImGui::TreeNodeEx("Rigid Body 2D", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 25);
                if (ImGui::SmallButton("X##RemoveRigidBody")) {
                    removeRigidBody = true;
                }

                if (rigidBodyOpen) {
                    RigidBody2D& rb = registry.get<RigidBody2D>(m_SelectedEntity);

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
                        std::make_unique<RemoveComponentCommand<RigidBody2D>>(&registry, m_SelectedEntity)
                    );
                }
            }

            if (registry.all_of<BoxCollider2D>(m_SelectedEntity)) {
                ImGui::Separator();
                bool removeBoxCollider = false;

                ImGui::AlignTextToFramePadding();
                bool boxColliderOpen = ImGui::TreeNodeEx("Box Collider 2D", ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 25);
                if (ImGui::SmallButton("X##RemoveBoxCollider")) {
                    removeBoxCollider = true;
                }

                if (boxColliderOpen) {
                    BoxCollider2D& collider = registry.get<BoxCollider2D>(m_SelectedEntity);

                    Reflection::ImGuiRenderer::RenderProperties(collider, [this](const char* label, entt::entity* entity) {
                        return RenderEntityPicker(label, entity);
                    });

                    if (registry.all_of<Sprite>(m_SelectedEntity)) {
                        if (ImGui::Button("Fit to Sprite")) {
                            const Sprite& sprite = registry.get<Sprite>(m_SelectedEntity);
                            collider.size = Vector2{sprite.sourceRect.width, sprite.sourceRect.height};
                            collider.offset = Vector2{0.0f, 0.0f};
                        }
                    }

                    ImGui::TreePop();
                }

                if (removeBoxCollider) {
                    m_CommandHistory.ExecuteCommand(
                        std::make_unique<RemoveComponentCommand<BoxCollider2D>>(&registry, m_SelectedEntity)
                    );
                }
            }

            if (registry.all_of<Script>(m_SelectedEntity)) {
                ImGui::Separator();
                bool removeScript = false;

                ImGui::AlignTextToFramePadding();
                bool scriptOpen = ImGui::TreeNodeEx("Script", ImGuiTreeNodeFlags_DefaultOpen);

                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
                if (ImGui::SmallButton("X##RemoveScript")) {
                    removeScript = true;
                }

                if (scriptOpen) {
                    Script& script = registry.get<Script>(m_SelectedEntity);

                    if (script.instance) {
                        ImGui::Text("Script: %s", script.scriptName.c_str());
                        ImGui::Checkbox("Enabled", &script.instance->m_Enabled);
                        ImGui::Spacing();

                        const Reflection::TypeInfo* typeInfo = Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));
                        if (typeInfo) {
                            for (const Reflection::FieldInfo& field : typeInfo->GetFields()) {
                                if (field.flags & Reflection::FieldFlags::ReadOnly) continue;
                                void* fieldPtr = field.getPtr(static_cast<void*>(script.instance.get()));
                                Reflection::ImGuiRenderer::RenderField(field, fieldPtr, [this](const char* label, entt::entity* entity) {
                                    return RenderEntityPicker(label, entity);
                                });
                            }
                        }
                    } else {
                        ImGui::TextColored(ImVec4{1.0f, 0.5f, 0.0f, 1.0f}, "No script instance");

                        if (m_Engine && m_Engine->GetScriptSystem()) {
                            ScriptSystem* scriptSystem = m_Engine->GetScriptSystem();
                            const auto& registeredScripts = scriptSystem->GetRegisteredScripts();

                            if (ImGui::BeginCombo("##SelectScript", "Select Script...")) {
                                for (const auto& [name, factory] : registeredScripts) {
                                    if (ImGui::Selectable(name.c_str())) {
                                        script.instance = scriptSystem->CreateScript(name);
                                        script.scriptName = name;
                                        if (script.instance) {
                                            script.instance->Initialize(m_SelectedEntity, m_Engine->GetActiveScene());
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
                    registry.remove<Script>(m_SelectedEntity);
                }
            }

            ImGui::Separator();

            if (ImGui::Button("Add Component")) {
                ImGui::OpenPopup("AddComponentPopup");
            }

            if (ImGui::BeginPopup("AddComponentPopup")) {
                if (ImGui::MenuItem("Camera")) {
                    if (!registry.all_of<Camera>(m_SelectedEntity)) {
                        bool hasPrimaryCamera = false;
                        registry.view<Camera>().each([&hasPrimaryCamera](const Camera& cam) {
                            if (cam.isPrimary) {
                                hasPrimaryCamera = true;
                            }
                        });

                        Camera newCamera{};
                        newCamera.isPrimary = !hasPrimaryCamera;
                        m_CommandHistory.ExecuteCommand(
                            std::make_unique<AddComponentCommand<Camera>>(&registry, m_SelectedEntity, newCamera)
                        );
                    }
                }
                if (ImGui::MenuItem("Sprite")) {
                    if (!registry.all_of<Sprite>(m_SelectedEntity)) {
                        m_CommandHistory.ExecuteCommand(
                            std::make_unique<AddComponentCommand<Sprite>>(&registry, m_SelectedEntity, Sprite{})
                        );
                    }
                }
                if (ImGui::MenuItem("Rigid Body 2D")) {
                    if (!registry.all_of<RigidBody2D>(m_SelectedEntity)) {
                        m_CommandHistory.ExecuteCommand(
                            std::make_unique<AddComponentCommand<RigidBody2D>>(&registry, m_SelectedEntity, RigidBody2D{})
                        );
                    }
                }
                if (ImGui::MenuItem("Box Collider 2D")) {
                    if (!registry.all_of<BoxCollider2D>(m_SelectedEntity)) {
                        BoxCollider2D collider{};

                        if (registry.all_of<Sprite>(m_SelectedEntity)) {
                            const Sprite& sprite = registry.get<Sprite>(m_SelectedEntity);
                            collider.size = Vector2{sprite.sourceRect.width, sprite.sourceRect.height};
                        }

                        m_CommandHistory.ExecuteCommand(
                            std::make_unique<AddComponentCommand<BoxCollider2D>>(&registry, m_SelectedEntity, collider)
                        );
                    }
                }
                if (ImGui::MenuItem("Script")) {
                    if (!registry.all_of<Script>(m_SelectedEntity)) {
                        registry.emplace<Script>(m_SelectedEntity);
                    }
                }
                ImGui::EndPopup();
            }
        }
    }

    ImGui::End();
}

void EditorLayer::RenderContentBrowser() {
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

                    if (info.extension == ".png" || info.extension == ".jpg" || info.extension == ".jpeg" || info.extension == ".bmp" || info.extension == ".tga") {
                        info.type = "texture";
                        Image img = LoadImage(info.path.c_str());
                        if (img.data != nullptr) {
                            info.width = img.width;
                            info.height = img.height;
                            UnloadImage(img);
                        }
                    } else if (info.extension == ".scene") {
                        info.type = "scene";
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

        for (const std::string& dirPath : directories) {
            std::string dirName = dirPath;
            size_t lastSlash = dirPath.find_last_of('/');
            if (lastSlash != std::string::npos) {
                dirName = dirPath.substr(lastSlash + 1);
            }

            ImGui::PushID(dirPath.c_str());
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
                    #ifdef _WIN32
                    std::string cmd = "rmdir /s /q \"" + dirPath + "\"";
                    #else
                    std::string cmd = "rm -rf \"" + dirPath + "\"";
                    #endif
                    system(cmd.c_str());
                    needsRefresh = true;
                    TraceLog(LOG_INFO, "Deleted: %s", dirPath.c_str());
                }

                ImGui::EndPopup();
            }

            ImGui::PopStyleColor(3);
            ImGui::TextWrapped("%s", dirName.c_str());
            ImGui::EndGroup();
            ImGui::NextColumn();
            ImGui::PopID();
        }

        for (AssetInfo& asset : files) {
            ImGui::PushID(asset.path.c_str());
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

                    ImGui::Image(texId, imageSize);

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
                            #ifdef _WIN32
                            std::string cmd = "del /f \"" + asset.path + "\"";
                            #else
                            std::string cmd = "rm -f \"" + asset.path + "\"";
                            #endif
                            system(cmd.c_str());
                            needsRefresh = true;
                            TraceLog(LOG_INFO, "Deleted: %s", asset.path.c_str());
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

                ImGui::Button("SCENE", ImVec2{static_cast<float>(thumbnailSize), static_cast<float>(thumbnailSize)});

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (m_Engine && m_Engine->GetActiveScene()) {
                        Scene* scene = m_Engine->GetActiveScene();
                        SceneSerializer serializer{scene};
                        if (serializer.Deserialize(asset.path)) {
                            m_CurrentScenePath = asset.path;
                            m_SelectedEntity = entt::null;
                            m_CommandHistory.Clear();
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
                        #ifdef _WIN32
                        std::string cmd = "del /f \"" + asset.path + "\"";
                        #else
                        std::string cmd = "rm -f \"" + asset.path + "\"";
                        #endif
                        system(cmd.c_str());
                        needsRefresh = true;
                        TraceLog(LOG_INFO, "Deleted: %s", asset.path.c_str());
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopStyleColor(3);
            } else {
                ImGui::Button("FILE", ImVec2{static_cast<float>(thumbnailSize), static_cast<float>(thumbnailSize)});

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
                        #ifdef _WIN32
                        std::string cmd = "del /f \"" + asset.path + "\"";
                        #else
                        std::string cmd = "rm -f \"" + asset.path + "\"";
                        #endif
                        system(cmd.c_str());
                        needsRefresh = true;
                        TraceLog(LOG_INFO, "Deleted: %s", asset.path.c_str());
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

        if (ImGui::MenuItem("New Scene")) {
            showNewScenePopup = true;
            std::memset(newItemName, 0, sizeof(newItemName));
        }

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

                #ifdef _WIN32
                std::string cmd = "mkdir \"" + newFolderPath + "\"";
                #else
                std::string cmd = "mkdir -p \"" + newFolderPath + "\"";
                #endif

                system(cmd.c_str());
                needsRefresh = true;
                TraceLog(LOG_INFO, "Created folder: %s", newFolderPath.c_str());
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

        if (m_Engine && m_Engine->GetScriptSystem()) {
            entt::registry& registry = scene->GetRegistry();
            ScriptSystem* scriptSystem = m_Engine->GetScriptSystem();

            registry.view<Script>().each([&](entt::entity entity, Script& script) {
                if (!script.scriptName.empty() && !script.instance) {
                    script.instance = scriptSystem->CreateScript(script.scriptName);
                    if (script.instance) {
                        script.instance->Initialize(entity, scene);
                    }
                }
            });
        }
    }
}

void EditorLayer::RenderProjectSettings() {
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

    return changed;
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

    entt::entity newEntity = registry.create();

    if (registry.all_of<Tag>(entity)) {
        const Tag& originalTag = registry.get<Tag>(entity);
        Tag newTag;
        newTag.name = originalTag.name + " (Copy)";
        registry.emplace<Tag>(newEntity, newTag);
    }

    if (registry.all_of<Transform>(entity)) {
        const Transform& originalTransform = registry.get<Transform>(entity);
        Transform newTransform;
        newTransform.position.x = originalTransform.position.x + 20.0f;
        newTransform.position.y = originalTransform.position.y + 20.0f;
        newTransform.rotation = originalTransform.rotation;
        newTransform.scale = originalTransform.scale;
        registry.emplace<Transform>(newEntity, newTransform);
    }

    if (registry.all_of<Sprite>(entity)) {
        const Sprite& originalSprite = registry.get<Sprite>(entity);
        Sprite newSprite;

        if (!originalSprite.texturePath.empty()) {
            newSprite.texture = AssetManager::Instance().LoadTexture(originalSprite.texturePath);
            newSprite.texturePath = originalSprite.texturePath;
        } else {
            newSprite.texture = originalSprite.texture;
        }

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

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR
