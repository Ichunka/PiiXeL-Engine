#ifndef PIIXELENGINE_SCRIPT_HPP
#define PIIXELENGINE_SCRIPT_HPP

#include <memory>
#include <string>
#include <typeindex>

namespace PiiXeL {

class ScriptComponent;

struct Script {
    std::shared_ptr<ScriptComponent> instance;
    std::string scriptName;
    std::type_index typeIndex{typeid(void)};

    Script() = default;

    template<typename T>
    explicit Script(std::shared_ptr<T> scriptInstance)
        : instance{std::move(scriptInstance)}
        , scriptName{typeid(T).name()}
        , typeIndex{typeid(T)}
    {}

    template<typename T>
    T* As() {
        if (typeIndex == std::type_index(typeid(T))) {
            return static_cast<T*>(instance.get());
        }
        return nullptr;
    }

    template<typename T>
    bool Is() const {
        return typeIndex == std::type_index(typeid(T));
    }
};

} // namespace PiiXeL

#endif // PIIXELENGINE_SCRIPT_HPP
