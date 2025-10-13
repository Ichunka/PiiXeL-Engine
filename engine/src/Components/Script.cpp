#include "Components/Script.hpp"
#include "Scripting/ScriptComponent.hpp"

namespace PiiXeL {

void Script::AddScript(const std::string& scriptName) {
    ScriptInstance scriptInst{};
    scriptInst.scriptName = scriptName;
    scripts.push_back(scriptInst);
}

void Script::AddScript(std::shared_ptr<ScriptComponent> instance, const std::string& scriptName) {
    ScriptInstance scriptInst{};
    scriptInst.instance = instance;
    scriptInst.scriptName = scriptName;
    scripts.push_back(scriptInst);
}

void Script::RemoveScript(size_t index) {
    if (index < scripts.size()) {
        scripts.erase(scripts.begin() + static_cast<std::ptrdiff_t>(index));
    }
}

void Script::RemoveScript(const std::string& scriptName) {
    scripts.erase(
        std::remove_if(scripts.begin(), scripts.end(),
            [&scriptName](const ScriptInstance& script) {
                return script.scriptName == scriptName;
            }),
        scripts.end()
    );
}

ScriptInstance* Script::GetScript(size_t index) {
    if (index < scripts.size()) {
        return &scripts[index];
    }
    return nullptr;
}

ScriptInstance* Script::GetScript(const std::string& scriptName) {
    for (ScriptInstance& script : scripts) {
        if (script.scriptName == scriptName) {
            return &script;
        }
    }
    return nullptr;
}

} // namespace PiiXeL
