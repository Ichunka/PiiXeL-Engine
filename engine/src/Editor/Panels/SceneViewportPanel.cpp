#ifdef BUILD_WITH_EDITOR

#include "Editor/Panels/SceneViewportPanel.hpp"

#include "Core/Engine.hpp"
#include "Debug/Profiler.hpp"
#include "Editor/EditorCamera.hpp"

#include <imgui.h>
#include <rlImGui.h>

namespace PiiXeL {

SceneViewportPanel::SceneViewportPanel(Engine* engine, RenderTexture2D* viewportTexture, Rectangle* viewportBounds,
                                       bool* viewportHovered, bool* viewportFocused, EditorCamera* editorCamera,
                                       ImVec2* viewportPos, ImVec2* viewportSize) :
    m_Engine{engine}, m_ViewportTexture{viewportTexture}, m_ViewportBounds{viewportBounds},
    m_ViewportHovered{viewportHovered}, m_ViewportFocused{viewportFocused}, m_EditorCamera{editorCamera},
    m_ViewportPos{viewportPos}, m_ViewportSize{viewportSize} {}

void SceneViewportPanel::SetHandleGizmoInteractionCallback(std::function<void()> callback) {
    m_HandleGizmoInteractionCallback = callback;
}

void SceneViewportPanel::SetHandleEntitySelectionCallback(std::function<void()> callback) {
    m_HandleEntitySelectionCallback = callback;
}

void SceneViewportPanel::SetRenderGizmosCallback(std::function<void()> callback) {
    m_RenderGizmosCallback = callback;
}

void SceneViewportPanel::OnImGuiRender() {
    PROFILE_FUNCTION();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin("Scene");

    *m_ViewportFocused = ImGui::IsWindowFocused();
    *m_ViewportHovered = ImGui::IsWindowHovered();

    *m_ViewportPos = ImGui::GetCursorScreenPos();
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    *m_ViewportSize = viewportPanelSize;

    if (m_EditorCamera) {
        m_EditorCamera->HandleInput(*m_ViewportHovered, *m_ViewportFocused);
    }

    if (*m_ViewportHovered && *m_ViewportFocused) {
        m_HandleGizmoInteractionCallback();
        m_HandleEntitySelectionCallback();
    }

    if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
        if (static_cast<int>(viewportPanelSize.x) != m_ViewportTexture->texture.width ||
            static_cast<int>(viewportPanelSize.y) != m_ViewportTexture->texture.height)
        {
            UnloadRenderTexture(*m_ViewportTexture);
            *m_ViewportTexture =
                LoadRenderTexture(static_cast<int>(viewportPanelSize.x), static_cast<int>(viewportPanelSize.y));
        }

        *m_ViewportBounds = Rectangle{0, 0, viewportPanelSize.x, viewportPanelSize.y};

        BeginTextureMode(*m_ViewportTexture);
        ClearBackground(Color{45, 45, 48, 255});

        Camera2D camera = m_EditorCamera->GetCamera2D(viewportPanelSize.x, viewportPanelSize.y);

        BeginMode2D(camera);

        DrawLine(-10000, 0, 10000, 0, Color{80, 80, 80, 255});
        DrawLine(0, -10000, 0, 10000, Color{80, 80, 80, 255});

        for (int i = -100; i <= 100; i++) {
            if (i == 0)
                continue;
            DrawLine(i * 100, -10000, i * 100, 10000, Color{50, 50, 50, 255});
            DrawLine(-10000, i * 100, 10000, i * 100, Color{50, 50, 50, 255});
        }

        if (m_Engine) {
            m_Engine->Render();
        }

        m_RenderGizmosCallback();

        EndMode2D();

        Vector2 camPos = m_EditorCamera->GetPosition();
        float camZoom = m_EditorCamera->GetZoom();
        DrawText(TextFormat("Zoom: %.2f | Pos: (%.0f, %.0f)", camZoom, camPos.x, camPos.y), 10, 10, 16, RAYWHITE);

        EndTextureMode();

        Rectangle sourceRec{0.0f, 0.0f, static_cast<float>(m_ViewportTexture->texture.width),
                            -static_cast<float>(m_ViewportTexture->texture.height)};

        rlImGuiImageRect(&m_ViewportTexture->texture, static_cast<int>(viewportPanelSize.x),
                         static_cast<int>(viewportPanelSize.y), sourceRec);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace PiiXeL

#endif
