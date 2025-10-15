#ifndef PIIXELENGINE_COMPONENTREGISTRY_HPP
#define PIIXELENGINE_COMPONENTREGISTRY_HPP

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <string>
#include <unordered_map>

namespace PiiXeL {

class ComponentRegistry {
public:
    using DeserializeFunc = std::function<void(entt::registry&, entt::entity, const nlohmann::json&)>;

    static ComponentRegistry& Instance() {
        static ComponentRegistry instance;
        return instance;
    }

    void RegisterComponent(const std::string& name, DeserializeFunc func) { m_Components[name] = func; }

    bool HasComponent(const std::string& name) const { return m_Components.find(name) != m_Components.end(); }

    void DeserializeComponent(const std::string& name, entt::registry& registry, entt::entity entity,
                              const nlohmann::json& data) {
        auto it = m_Components.find(name);
        if (it != m_Components.end()) {
            it->second(registry, entity, data);
        }
    }

    const std::unordered_map<std::string, DeserializeFunc>& GetAll() const { return m_Components; }

private:
    ComponentRegistry() = default;
    std::unordered_map<std::string, DeserializeFunc> m_Components;
};

void RegisterAllComponents();

} // namespace PiiXeL

#endif
