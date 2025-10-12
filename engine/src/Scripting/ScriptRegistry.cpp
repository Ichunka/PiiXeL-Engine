#include "Scripting/ScriptRegistry.hpp"
#include "Scripting/ScriptComponent.hpp"
#include <raylib.h>

namespace PiiXeL {

ScriptRegistry& ScriptRegistry::Instance() {
    static ScriptRegistry instance;
    return instance;
}

void ScriptRegistry::RegisterScript(const std::string& name, ScriptFactoryFunc factory) {
    if (m_Scripts.find(name) != m_Scripts.end()) {
        TraceLog(LOG_WARNING, "[ScriptRegistry] Script '%s' already registered, overwriting", name.c_str());
    }
    m_Scripts[name] = factory;
    TraceLog(LOG_INFO, "[ScriptRegistry] Registered script: %s", name.c_str());
}

std::shared_ptr<ScriptComponent> ScriptRegistry::CreateScript(const std::string& name) {
    auto it = m_Scripts.find(name);
    if (it == m_Scripts.end()) {
        TraceLog(LOG_ERROR, "[ScriptRegistry] Script '%s' not found", name.c_str());
        return nullptr;
    }
    return it->second();
}

bool ScriptRegistry::HasScript(const std::string& name) const {
    return m_Scripts.find(name) != m_Scripts.end();
}

} // namespace PiiXeL
