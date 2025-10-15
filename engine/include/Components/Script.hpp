#ifndef PIIXELENGINE_SCRIPT_HPP
#define PIIXELENGINE_SCRIPT_HPP

#include <memory>
#include <string>
#include <typeindex>
#include <vector>

namespace PiiXeL {

class ScriptComponent;

struct ScriptInstance {
    std::shared_ptr<ScriptComponent> instance;
    std::string scriptName;
    std::type_index typeIndex{typeid(void)};

    ScriptInstance() = default;

    template <typename T>
    explicit ScriptInstance(std::shared_ptr<T> scriptInstance) :
        instance{std::move(scriptInstance)}, scriptName{typeid(T).name()}, typeIndex{typeid(T)} {}

    template <typename T>
    T* As() {
        if (typeIndex == std::type_index(typeid(T))) {
            return static_cast<T*>(instance.get());
        }
        return nullptr;
    }

    template <typename T>
    bool Is() const {
        return typeIndex == std::type_index(typeid(T));
    }
};

struct Script {
    std::vector<ScriptInstance> scripts;

    Script() = default;

    void AddScript(const std::string& scriptName);
    void AddScript(std::shared_ptr<ScriptComponent> instance, const std::string& scriptName);
    void RemoveScript(size_t index);
    void RemoveScript(const std::string& scriptName);
    ScriptInstance* GetScript(size_t index);
    ScriptInstance* GetScript(const std::string& scriptName);
    size_t GetScriptCount() const { return scripts.size(); }

    template <typename T>
    T* GetScriptOfType() {
        for (ScriptInstance& script : scripts) {
            if (T* typedScript = script.As<T>()) {
                return typedScript;
            }
        }
        return nullptr;
    }

    template <typename T>
    bool HasScriptOfType() const {
        for (const ScriptInstance& script : scripts) {
            if (script.Is<T>()) {
                return true;
            }
        }
        return false;
    }
};

} // namespace PiiXeL

#endif // PIIXELENGINE_SCRIPT_HPP
