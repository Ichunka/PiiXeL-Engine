#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorThemeManager.hpp"

#include <imgui.h>

namespace PiiXeL {

void EditorThemeManager::SetupDarkTheme() {
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

} // namespace PiiXeL

#endif
