#include "Systems/ScriptSystem.hpp"

#include "Components/Script.hpp"
#include "Scene/Scene.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Scripting/ScriptRegistry.hpp"

#include <raylib.h>

namespace PiiXeL {

ScriptSystem::ScriptSystem() = default;

ScriptSystem::~ScriptSystem() = default;

void ScriptSystem::OnUpdate(Scene* scene, float deltaTime) {
    if (!scene)
        return;

    entt::registry& registry = scene->GetRegistry();

    auto view = registry.view<Script>();
    for (auto entity : view) {
        Script& scriptComponent = view.get<Script>(entity);

        for (ScriptInstance& script : scriptComponent.scripts) {
            if (!script.instance && !script.scriptName.empty()) {
                script.instance = CreateScript(script.scriptName);
                if (script.instance) {
                    script.instance->Initialize(entity, scene);
                }
            }

            if (script.instance) {
                if (!script.instance->GetScene()) {
                    script.instance->Initialize(entity, scene);
                }

                script.instance->ExecuteUpdate(deltaTime);
            }
        }
    }
}

void ScriptSystem::OnFixedUpdate(Scene* scene, float fixedDeltaTime) {
    if (!scene)
        return;

    entt::registry& registry = scene->GetRegistry();

    auto view = registry.view<Script>();
    for (auto entity : view) {
        Script& scriptComponent = view.get<Script>(entity);

        for (ScriptInstance& script : scriptComponent.scripts) {
            if (!script.instance && !script.scriptName.empty()) {
                script.instance = CreateScript(script.scriptName);
                if (script.instance) {
                    script.instance->Initialize(entity, scene);
                }
            }

            if (script.instance && script.instance->GetScene()) {
                script.instance->ExecuteFixedUpdate(fixedDeltaTime);
            }
        }
    }
}

std::shared_ptr<ScriptComponent> ScriptSystem::CreateScript(const std::string& name) {
    return ScriptRegistry::Instance().CreateScript(name);
}

} // namespace PiiXeL
