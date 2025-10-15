#ifdef BUILD_WITH_EDITOR

#include "Editor/Panels/HierarchyPanel.hpp"

#include "Components/Tag.hpp"
#include "Components/UUID.hpp"
#include "Core/Engine.hpp"
#include "Debug/Profiler.hpp"
#include "Editor/AnimatorControllerEditorPanel.hpp"
#include "Editor/EditorCommandSystem.hpp"
#include "Editor/EditorCommands.hpp"
#include "Scene/Scene.hpp"

#include <imgui.h>

namespace PiiXeL {

HierarchyPanel::HierarchyPanel(Engine* engine, EditorCommandSystem* commandSystem, entt::entity* selectedEntity,
                               bool* inspectorLocked, UUID* selectedAssetUUID, std::string* selectedAssetPath,
                               AnimatorControllerEditorPanel* animatorControllerEditor) :
    m_Engine{engine}, m_CommandSystem{commandSystem}, m_SelectedEntity{selectedEntity},
    m_InspectorLocked{inspectorLocked}, m_SelectedAssetUUID{selectedAssetUUID}, m_SelectedAssetPath{selectedAssetPath},
    m_AnimatorControllerEditor{animatorControllerEditor} {}

void HierarchyPanel::SetDuplicateEntityCallback(std::function<entt::entity(entt::entity)> callback) {
    m_DuplicateEntityCallback = callback;
}

void HierarchyPanel::SetCopyEntityCallback(std::function<void(entt::entity)> callback) {
    m_CopyEntityCallback = callback;
}

void HierarchyPanel::OnImGuiRender() {
    PROFILE_FUNCTION();
    ImGui::Begin("Hierarchy", &m_IsOpen);

    m_IsDraggingEntity = false;

    if (m_Engine && m_Engine->GetActiveScene()) {
        Scene* scene = m_Engine->GetActiveScene();
        entt::registry& registry = scene->GetRegistry();

        if (ImGui::Button("+ Create Entity")) {
            m_CommandSystem->ExecuteCommand(std::make_unique<CreateEntityCommand>(scene, "New Entity"));
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
            if (*m_SelectedEntity == entity) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uint64_t>(static_cast<uint32_t>(entity))), flags,
                              "%s", tag.name.c_str());

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
                if (!*m_InspectorLocked) {
                    *m_SelectedEntity = entity;
                    *m_SelectedAssetUUID = UUID{0};
                    if (m_AnimatorControllerEditor) {
                        m_AnimatorControllerEditor->ClearSelection();
                    }
                    m_SelectedAssetPath->clear();
                }
            }

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                    if (m_DuplicateEntityCallback) {
                        entt::entity newEntity = m_DuplicateEntityCallback(entity);
                        *m_SelectedEntity = newEntity;
                        *m_SelectedAssetUUID = UUID{0};
                        m_SelectedAssetPath->clear();
                    }
                }

                if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                    if (m_CopyEntityCallback) {
                        m_CopyEntityCallback(entity);
                    }
                }

                if (ImGui::MenuItem("Delete", "Delete")) {
                    if (*m_SelectedEntity == entity) {
                        *m_SelectedEntity = entt::null;
                    }
                    scene->DestroyEntity(entity);
                }

                ImGui::EndPopup();
            }
        }
    }

    ImGui::End();
}

} // namespace PiiXeL

#endif
