#include "Systems/ScriptSystem.hpp"
#include "Scene/Scene.hpp"
#include "Components/Script.hpp"
#include "Scripting/ScriptComponent.hpp"
#include <raylib.h>

namespace PiiXeL {

std::vector<ScriptRegistration>& ScriptSystem::GetPendingRegistrations() {
    static std::vector<ScriptRegistration> pendingRegistrations;
    return pendingRegistrations;
}

void ScriptSystem::ProcessPendingRegistrations() {
    for (const ScriptRegistration& reg : GetPendingRegistrations()) {
        m_ScriptFactories[reg.first] = reg.second;
    }
}

ScriptRegistrar::ScriptRegistrar(const std::string& name, ScriptFactory factory) {
    ScriptSystem::GetPendingRegistrations().emplace_back(name, factory);
}

ScriptSystem::ScriptSystem() {
    ProcessPendingRegistrations();
}

ScriptSystem::~ScriptSystem() = default;

void ScriptSystem::OnUpdate(Scene* scene, float deltaTime) {
    if (!scene) return;

    entt::registry& registry = scene->GetRegistry();

    auto view = registry.view<Script>();
    for (auto entity : view) {
        Script& script = view.get<Script>(entity);

        if (script.instance) {
            if (!script.instance->GetScene()) {
                script.instance->Initialize(entity, scene);
            }

            script.instance->ExecuteUpdate(deltaTime);
        }
    }
}

void ScriptSystem::OnFixedUpdate(Scene* scene, float fixedDeltaTime) {
    if (!scene) return;

    entt::registry& registry = scene->GetRegistry();

    auto view = registry.view<Script>();
    for (auto entity : view) {
        Script& script = view.get<Script>(entity);

        if (script.instance && script.instance->GetScene()) {
            script.instance->ExecuteFixedUpdate(fixedDeltaTime);
        }
    }
}

std::shared_ptr<ScriptComponent> ScriptSystem::CreateScript(const std::string& name) {
    auto it = m_ScriptFactories.find(name);
    if (it != m_ScriptFactories.end()) {
        return it->second();
    }

    TraceLog(LOG_WARNING, "Script not found: %s", name.c_str());
    return nullptr;
}

} // namespace PiiXeL
