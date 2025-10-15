#ifndef PIIXELENGINE_COMPONENTMODULEREGISTRY_HPP
#define PIIXELENGINE_COMPONENTMODULEREGISTRY_HPP

#include "Components/ComponentModule.hpp"

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace PiiXeL {

class ComponentModuleRegistry {
public:
    static ComponentModuleRegistry& Instance() {
        static ComponentModuleRegistry instance;
        return instance;
    }

    template <typename T>
    void RegisterModule(std::shared_ptr<ComponentModule<T>> module) {
        std::type_index typeIdx = std::type_index(typeid(T));
        m_ModulesByType[typeIdx] = module;
        m_ModulesByName[module->GetName()] = module;
        m_AllModules.push_back(module);
    }

    template <typename T>
    ComponentModule<T>* GetModule() {
        std::type_index typeIdx = std::type_index(typeid(T));
        auto it = m_ModulesByType.find(typeIdx);
        if (it != m_ModulesByType.end()) {
            return static_cast<ComponentModule<T>*>(it->second.get());
        }
        return nullptr;
    }

    IComponentModule* GetModuleByName(const std::string& name) {
        auto it = m_ModulesByName.find(name);
        if (it != m_ModulesByName.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    const std::vector<std::shared_ptr<IComponentModule>>& GetAllModules() const { return m_AllModules; }

    nlohmann::json SerializeEntity(entt::registry& registry, entt::entity entity);
    void DeserializeComponent(const std::string& componentName, entt::registry& registry, entt::entity entity,
                              const nlohmann::json& data);

#ifdef BUILD_WITH_EDITOR
    using EntityPickerFunc = IComponentModule::EntityPickerFunc;
    using AssetPickerFunc = IComponentModule::AssetPickerFunc;

    void RenderInspectorForEntity(entt::registry& registry, entt::entity entity, EditorCommandSystem& commandSystem,
                                  EntityPickerFunc entityPicker, AssetPickerFunc assetPicker);
    void RenderAddComponentMenu(entt::registry& registry, entt::entity entity, EditorCommandSystem& commandSystem);
    void DuplicateAllComponents(entt::registry& registry, entt::entity srcEntity, entt::entity dstEntity);
#endif

private:
    ComponentModuleRegistry() = default;

    std::unordered_map<std::type_index, std::shared_ptr<IComponentModule>> m_ModulesByType;
    std::unordered_map<std::string, std::shared_ptr<IComponentModule>> m_ModulesByName;
    std::vector<std::shared_ptr<IComponentModule>> m_AllModules;
};

} // namespace PiiXeL

#endif
