#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorShortcutHandler.hpp"

#include "Core/Engine.hpp"
#include "Editor/EditorCommandSystem.hpp"
#include "Editor/EditorSceneManager.hpp"
#include "Editor/EditorSelectionManager.hpp"
#include "Editor/EditorStateManager.hpp"
#include "Scene/Scene.hpp"

#include <imgui.h>

namespace PiiXeL {

void EditorShortcutHandler::HandleShortcuts(Engine* engine, EditorCommandSystem* commandSystem,
                                            EditorSceneManager* sceneManager, EditorSelectionManager* selectionManager,
                                            EditorStateManager* stateManager) {
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_W, false)) {
        commandSystem->Undo();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
        commandSystem->Redo();
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        sceneManager->SaveScene();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
        sceneManager->LoadScene();
        selectionManager->SetSelectedEntity(entt::null);
        commandSystem->Clear();
    }

    if (stateManager->IsEditMode() && selectionManager->GetSelectedEntity() != entt::null) {
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C, false)) {
            selectionManager->CopyEntity(engine, selectionManager->GetSelectedEntity());
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false)) {
            selectionManager->PasteEntity(engine);
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
            entt::entity newEntity = selectionManager->DuplicateEntity(engine, selectionManager->GetSelectedEntity());
            selectionManager->SetSelectedEntity(newEntity);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            if (engine && engine->GetActiveScene()) {
                Scene* scene = engine->GetActiveScene();
                entt::registry& registry = scene->GetRegistry();
                if (registry.valid(selectionManager->GetSelectedEntity())) {
                    registry.destroy(selectionManager->GetSelectedEntity());
                    selectionManager->SetSelectedEntity(entt::null);
                }
            }
        }
    }
}

} // namespace PiiXeL

#endif
