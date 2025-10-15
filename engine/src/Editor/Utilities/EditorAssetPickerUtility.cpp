#ifdef BUILD_WITH_EDITOR

#include "Editor/Utilities/EditorAssetPickerUtility.hpp"

#include "Components/Tag.hpp"
#include "Components/UUID.hpp"
#include "Core/Engine.hpp"
#include "Resources/Asset.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Scene/Scene.hpp"

#include <filesystem>
#include <imgui.h>

namespace PiiXeL {

bool EditorAssetPickerUtility::RenderEntityPicker(const char* label, entt::entity* entity, Engine* engine) {
    if (!engine || !engine->GetActiveScene()) {
        return false;
    }

    Scene* scene = engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    std::string preview = "None";
    if (*entity != entt::null && registry.valid(*entity)) {
        if (registry.all_of<Tag>(*entity)) {
            Tag& tag = registry.get<Tag>(*entity);
            preview = tag.name + " [" + std::to_string(static_cast<uint32_t>(*entity)) + "]";
        }
        else {
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

bool EditorAssetPickerUtility::RenderAssetPicker(const char* label, UUID* uuid, const std::string& assetType) {
    std::string preview = "None";
    if (uuid->Get() != 0) {
        std::shared_ptr<Asset> asset = AssetRegistry::Instance().GetAsset(*uuid);
        if (asset) {
            preview = asset->GetMetadata().name;
        }
        else {
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
            std::string extension = std::filesystem::path{assetPath}.extension().string();
            bool matchesFilter = false;

            if (assetType == "texture" && (extension == ".png" || extension == ".jpg" || extension == ".jpeg")) {
                matchesFilter = true;
            }
            else if (assetType == "AnimatorController" && extension == ".animcontroller") {
                matchesFilter = true;
            }
            else if (assetType == "SpriteSheet" && extension == ".spritesheet") {
                matchesFilter = true;
            }
            else if (assetType == "AnimationClip" && extension == ".animclip") {
                matchesFilter = true;
            }
            else if (assetType == "Audio" && (extension == ".wav" || extension == ".mp3" || extension == ".ogg")) {
                matchesFilter = true;
            }

            if (matchesFilter) {
                ImGui::PushID(static_cast<int>(assetUUID.Get()));

                bool isSelected = (uuid->Get() == assetUUID.Get());
                std::filesystem::path fsPath{assetPath};
                std::string itemLabel = fsPath.stem().string();

                if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                    *uuid = assetUUID;
                    changed = true;
                }

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }

                ImGui::PopID();
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

} // namespace PiiXeL

#endif
