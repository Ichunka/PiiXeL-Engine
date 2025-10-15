#ifdef BUILD_WITH_EDITOR

#include "Editor/Panels/ToolbarPanel.hpp"

#include "Core/Engine.hpp"
#include "Debug/DebugDraw.hpp"
#include "Editor/EditorGizmoSystem.hpp"
#include "Editor/EditorStateManager.hpp"
#include "Systems/RenderSystem.hpp"

#include <imgui.h>

namespace PiiXeL {

ToolbarPanel::ToolbarPanel(Engine* engine, EditorGizmoSystem* gizmoSystem, EditorStateManager* stateManager) :
    m_Engine{engine}, m_GizmoSystem{gizmoSystem}, m_StateManager{stateManager} {}

void ToolbarPanel::OnImGuiRender() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{8, 8});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{8, 4});

    ImGui::Begin("Toolbar", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar);

    if (m_StateManager->IsEditMode()) {
        if (ImGui::Button("Play")) {
            if (m_OnPlayCallback) {
                m_OnPlayCallback();
            }
        }
    }
    else {
        if (ImGui::Button("Stop")) {
            if (m_OnStopCallback) {
                m_OnStopCallback();
            }
        }
    }

    ImGui::SameLine();
    ImGui::Text("| State: %s", m_StateManager->IsEditMode() ? "Edit" : "Play");

    ImGui::SameLine();
    ImGui::Separator();

    if (m_StateManager->IsEditMode()) {
        ImGui::SameLine();
        ImGui::Text("Gizmo:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Translate", m_GizmoSystem->GetGizmoMode() == GizmoMode::Translate)) {
            m_GizmoSystem->SetGizmoMode(GizmoMode::Translate);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", m_GizmoSystem->GetGizmoMode() == GizmoMode::Rotate)) {
            m_GizmoSystem->SetGizmoMode(GizmoMode::Rotate);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", m_GizmoSystem->GetGizmoMode() == GizmoMode::Scale)) {
            m_GizmoSystem->SetGizmoMode(GizmoMode::Scale);
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

} // namespace PiiXeL

#endif
