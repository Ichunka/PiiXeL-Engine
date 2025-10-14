#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorPanelManager.hpp"

#include "Debug/Profiler.hpp"

#include <imgui.h>
#include <imgui_internal.h>

namespace PiiXeL {

EditorPanelManager::EditorPanelManager() : m_DockingLayoutInitialized{false} {}

void EditorPanelManager::BeginDockspace() {
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
    window_flags |=
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        if (!m_DockingLayoutInitialized)
        {
            SetupDockingLayout();
            m_DockingLayoutInitialized = true;
        }
    }
}

void EditorPanelManager::EndDockspace() {
    PROFILE_FUNCTION();
    ImGui::End();
}

void EditorPanelManager::SetupDockingLayout() {
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
    ImGui::DockBuilderDockWindow("Animator Controller Editor", dockspace_id);
    ImGui::DockBuilderDockWindow("Content Browser", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Console", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Build & Export", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Profiler", dock_id_right);

    ImGui::DockBuilderFinish(dockspace_id);
}

} // namespace PiiXeL

#endif
