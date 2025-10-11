#ifndef PIIXELENGINE_SCRIPTSYSTEM_HPP
#define PIIXELENGINE_SCRIPTSYSTEM_HPP

#include <entt/entt.hpp>
#include <functional>
#include <unordered_map>
#include <string>
#include <memory>
#include <typeindex>
#include <vector>

namespace PiiXeL {

class Scene;
class ScriptComponent;

using ScriptFactory = std::function<std::shared_ptr<ScriptComponent>()>;
using ScriptRegistration = std::pair<std::string, ScriptFactory>;

class ScriptSystem {
public:
    ScriptSystem();
    ~ScriptSystem();

    void OnUpdate(Scene* scene, float deltaTime);
    void OnFixedUpdate(Scene* scene, float fixedDeltaTime);

    template<typename T>
    void RegisterScript(const std::string& name) {
        m_ScriptFactories[name] = []() -> std::shared_ptr<ScriptComponent> {
            return std::make_shared<T>();
        };
        m_ScriptTypeMap[std::type_index(typeid(T))] = name;
    }

    std::shared_ptr<ScriptComponent> CreateScript(const std::string& name);

    template<typename T>
    std::string GetScriptName() {
        auto it = m_ScriptTypeMap.find(std::type_index(typeid(T)));
        if (it != m_ScriptTypeMap.end()) {
            return it->second;
        }
        return "";
    }

    [[nodiscard]] const std::unordered_map<std::string, ScriptFactory>& GetRegisteredScripts() const {
        return m_ScriptFactories;
    }

    static std::vector<ScriptRegistration>& GetPendingRegistrations();
    void ProcessPendingRegistrations();

private:
    std::unordered_map<std::string, ScriptFactory> m_ScriptFactories;
    std::unordered_map<std::type_index, std::string> m_ScriptTypeMap;
};

class ScriptRegistrar {
public:
    ScriptRegistrar(const std::string& name, ScriptFactory factory);
};

} // namespace PiiXeL

#define REGISTER_SCRIPT(ScriptClass, ScriptName) \
    namespace { \
        static ::PiiXeL::ScriptRegistrar __script_registrar_##ScriptClass( \
            ScriptName, \
            []() -> std::shared_ptr<::PiiXeL::ScriptComponent> { \
                return std::make_shared<ScriptClass>(); \
            } \
        ); \
    }

#endif // PIIXELENGINE_SCRIPTSYSTEM_HPP
