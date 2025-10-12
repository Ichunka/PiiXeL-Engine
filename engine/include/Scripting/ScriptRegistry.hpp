#ifndef PIIXELENGINE_SCRIPTREGISTRY_HPP
#define PIIXELENGINE_SCRIPTREGISTRY_HPP

#include <string>
#include <functional>
#include <unordered_map>
#include <memory>

namespace PiiXeL {

class ScriptComponent;

using ScriptFactoryFunc = std::function<std::shared_ptr<ScriptComponent>()>;

class ScriptRegistry {
public:
    static ScriptRegistry& Instance();

    void RegisterScript(const std::string& name, ScriptFactoryFunc factory);
    std::shared_ptr<ScriptComponent> CreateScript(const std::string& name);
    bool HasScript(const std::string& name) const;

    const std::unordered_map<std::string, ScriptFactoryFunc>& GetAllScripts() const { return m_Scripts; }

private:
    ScriptRegistry() = default;
    std::unordered_map<std::string, ScriptFactoryFunc> m_Scripts;
};

#define REGISTER_SCRIPT(ClassName) \
    namespace { \
        struct ClassName##Registrar { \
            ClassName##Registrar() { \
                PiiXeL::ScriptRegistry::Instance().RegisterScript(#ClassName, \
                    []() -> std::shared_ptr<PiiXeL::ScriptComponent> { \
                        return std::make_shared<ClassName>(); \
                    }); \
            } \
        }; \
        static ClassName##Registrar g_##ClassName##Registrar; \
    }

} // namespace PiiXeL

#endif
