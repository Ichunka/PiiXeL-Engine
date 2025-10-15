#ifdef BUILD_WITH_EDITOR

#include "Editor/Panels/GameViewportPanel.hpp"

#include "Components/Camera.hpp"
#include "Components/Transform.hpp"
#include "Core/Engine.hpp"
#include "Editor/EditorStateManager.hpp"
#include "Scene/Scene.hpp"
#include "Systems/RenderSystem.hpp"

#include <imgui.h>
#include <rlImGui.h>

namespace PiiXeL {

GameViewportPanel::GameViewportPanel(Engine* engine, RenderTexture2D* gameViewportTexture,
                                     EditorStateManager* stateManager) :
    m_Engine{engine},
    m_GameViewportTexture{gameViewportTexture}, m_StateManager{stateManager} {}

void GameViewportPanel::SetGetPrimaryCameraCallback(std::function<entt::entity()> callback) {
    m_GetPrimaryCameraCallback = callback;
}

void GameViewportPanel::OnImGuiRender() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin("Game");

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

    entt::entity primaryCameraEntity = m_GetPrimaryCameraCallback();

    if (primaryCameraEntity == entt::null) {
        ImVec2 windowSize = ImGui::GetWindowSize();
        const char* message = "No Primary Camera";
        ImVec2 textSize = ImGui::CalcTextSize(message);
        ImGui::SetCursorPos(ImVec2{(windowSize.x - textSize.x) * 0.5f, (windowSize.y - textSize.y) * 0.5f});
        ImGui::TextDisabled("%s", message);
    }
    else if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
        if (static_cast<int>(viewportPanelSize.x) != m_GameViewportTexture->texture.width ||
            static_cast<int>(viewportPanelSize.y) != m_GameViewportTexture->texture.height)
        {
            UnloadRenderTexture(*m_GameViewportTexture);
            *m_GameViewportTexture =
                LoadRenderTexture(static_cast<int>(viewportPanelSize.x), static_cast<int>(viewportPanelSize.y));
        }

        BeginTextureMode(*m_GameViewportTexture);
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

        Rectangle sourceRec{0.0f, 0.0f, static_cast<float>(m_GameViewportTexture->texture.width),
                            -static_cast<float>(m_GameViewportTexture->texture.height)};

        rlImGuiImageRect(&m_GameViewportTexture->texture, static_cast<int>(viewportPanelSize.x),
                         static_cast<int>(viewportPanelSize.y), sourceRec);
    }

    if (m_StateManager && m_StateManager->IsPlayMode()) {
        if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered()) {
            ImGuiIO& io = ImGui::GetIO();
            io.WantCaptureMouse = false;
            io.WantCaptureKeyboard = false;
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace PiiXeL

#endif
