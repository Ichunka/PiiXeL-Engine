#ifdef BUILD_WITH_EDITOR

#include "Editor/Panels/MenuBarPanel.hpp"

#include "Core/Engine.hpp"
#include "Editor/EditorCommands.hpp"
#include "Editor/EditorCommandSystem.hpp"
#include "Editor/EditorSceneManager.hpp"
#include "Editor/Panels/ProjectSettingsPanel.hpp"
#include "Scene/Scene.hpp"

#include <imgui.h>

namespace PiiXeL {

MenuBarPanel::MenuBarPanel(Engine* engine, EditorCommandSystem* commandSystem, EditorSceneManager* sceneManager,
                           ProjectSettingsPanel* projectSettingsPanel) :
    m_Engine{engine}, m_CommandSystem{commandSystem}, m_SceneManager{sceneManager},
    m_ProjectSettingsPanel{projectSettingsPanel} {}

void MenuBarPanel::OnImGuiRender() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {
                m_SceneManager->LoadScene();
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                m_SceneManager->SaveScene();
            }
            if (ImGui::MenuItem("Save Scene As...")) {
                m_SceneManager->SaveSceneAs();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_CommandSystem->CanUndo())) {
                m_CommandSystem->Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_CommandSystem->CanRedo())) {
                m_CommandSystem->Redo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Entity")) {
            if (ImGui::MenuItem("Create Empty")) {
                Scene* scene = m_Engine->GetActiveScene();
                if (m_Engine && scene) {
                    m_CommandSystem->ExecuteCommand(std::make_unique<CreateEntityCommand>(scene, "Entity"));
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Project")) {
            if (ImGui::MenuItem("Settings")) {
                m_ProjectSettingsPanel->SetOpen(true);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

} // namespace PiiXeL

#endif
