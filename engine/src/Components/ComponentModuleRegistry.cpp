#include "Components/ComponentModuleRegistry.hpp"

#include "Core/Logger.hpp"

#ifdef BUILD_WITH_EDITOR
#include <algorithm>
#include <imgui.h>
#endif

namespace PiiXeL {

nlohmann::json ComponentModuleRegistry::SerializeEntity(entt::registry& registry, entt::entity entity) {
    nlohmann::json entityJson;

    for (const auto& module : m_AllModules) {
        if (!module->HasComponent(registry, entity)) {
            continue;
        }

        nlohmann::json componentJson = module->Serialize(registry, entity);
        if (!componentJson.empty()) {
            entityJson[module->GetName()] = componentJson;
        }
    }

    return entityJson;
}

void ComponentModuleRegistry::DeserializeComponent(const std::string& componentName, entt::registry& registry,
                                                   entt::entity entity, const nlohmann::json& data) {
    IComponentModule* module = GetModuleByName(componentName);
    if (module) {
        module->Deserialize(registry, entity, data);
    }
    else {
        PX_LOG_WARNING(ENGINE, "ComponentModuleRegistry: Unknown component type '%s'", componentName.c_str());
    }
}

#ifdef BUILD_WITH_EDITOR

void ComponentModuleRegistry::RenderInspectorForEntity(entt::registry& registry, entt::entity entity,
                                                       EditorCommandSystem& commandSystem, EntityPickerFunc entityPicker,
                                                       AssetPickerFunc assetPicker) {
    std::vector<IComponentModule*> sortedModules;
    for (const auto& module : m_AllModules) {
        if (!module->IsRenderedByRegistry()) {
            continue;
        }
        if (module->HasComponent(registry, entity)) {
            sortedModules.push_back(module.get());
        }
    }

    std::sort(sortedModules.begin(), sortedModules.end(),
              [](IComponentModule* a, IComponentModule* b) { return a->GetDisplayOrder() < b->GetDisplayOrder(); });

    for (IComponentModule* module : sortedModules) {
        ImGui::Separator();
        bool removeComponent = false;

        ImGui::AlignTextToFramePadding();
        std::string headerLabel = std::string(module->GetName());
        bool isOpen = ImGui::TreeNodeEx(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 25);
        std::string removeButtonLabel = "X##Remove" + std::string(module->GetName());
        if (ImGui::SmallButton(removeButtonLabel.c_str())) {
            removeComponent = true;
        }

        if (isOpen) {
            module->RenderInspectorUI(registry, entity, commandSystem, entityPicker, assetPicker);
            ImGui::TreePop();
        }

        if (removeComponent) {
            module->RemoveComponent(registry, entity);
        }
    }
}

void ComponentModuleRegistry::RenderAddComponentMenu(entt::registry& registry, entt::entity entity,
                                                     EditorCommandSystem& commandSystem) {
    for (const auto& module : m_AllModules) {
        if (module->HasComponent(registry, entity)) {
            continue;
        }

        if (ImGui::MenuItem(module->GetName())) {
            module->AddComponentToEntity(registry, entity, commandSystem);
        }
    }
}

void ComponentModuleRegistry::DuplicateAllComponents(entt::registry& registry, entt::entity srcEntity,
                                                     entt::entity dstEntity) {
    for (const auto& module : m_AllModules) {
        if (module->HasComponent(registry, srcEntity)) {
            module->DuplicateComponent(registry, srcEntity, dstEntity);
        }
    }
}

#endif

} // namespace PiiXeL
