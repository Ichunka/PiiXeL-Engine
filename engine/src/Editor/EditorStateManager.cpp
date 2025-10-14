#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorStateManager.hpp"
#include "Editor/EditorSceneManager.hpp"
#include "Editor/EditorSelectionManager.hpp"
#include "Editor/CommandHistory.hpp"
#include "Core/Engine.hpp"
#include "Core/Logger.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneSerializer.hpp"
#include "Components/Script.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Systems/ScriptSystem.hpp"
#include "Systems/AnimationSystem.hpp"
#include "Reflection/Reflection.hpp"
#include <nlohmann/json.hpp>
#include <entt/entt.hpp>

namespace PiiXeL {

EditorStateManager::EditorStateManager()
    : m_EditorState{EditorState::Edit}
    , m_PlayModeSnapshot{}
{
}

void EditorStateManager::OnPlayButtonPressed(
    Engine* engine,
    EditorSceneManager* sceneManager,
    EditorSelectionManager* selectionManager,
    CommandHistory* commandHistory
) {
    if (!engine || !engine->GetActiveScene()) {
        return;
    }

    if (sceneManager->GetCurrentScenePath().empty()) {
        sceneManager->SaveSceneAs();
    }
    else {
        sceneManager->SaveScene();
    }

    Scene* scene = engine->GetActiveScene();
    SceneSerializer serializer{scene};
    m_PlayModeSnapshot = serializer.SerializeToString();

    m_EditorState = EditorState::Play;
    commandHistory->Clear();
    selectionManager->SetSelectedEntity(entt::null);

    engine->CreatePhysicsBodies();
    engine->SetPhysicsEnabled(true);
    engine->SetScriptsEnabled(true);
    engine->SetAnimationEnabled(true);

    AnimationSystem::ResetAnimators(scene->GetRegistry());

    PX_LOG_INFO(EDITOR, "Play mode started with memory snapshot");
}

void EditorStateManager::OnStopButtonPressed(
    Engine* engine,
    EditorSelectionManager* selectionManager,
    CommandHistory* commandHistory
) {
    if (!engine || !engine->GetActiveScene()) {
        return;
    }

    engine->SetPhysicsEnabled(false);
    engine->SetScriptsEnabled(false);
    engine->SetAnimationEnabled(false);
    engine->DestroyAllPhysicsBodies();

    if (!m_PlayModeSnapshot.empty()) {
        Scene* scene = engine->GetActiveScene();
        SceneSerializer serializer{scene};
        if (serializer.DeserializeFromString(m_PlayModeSnapshot)) {
            m_EditorState = EditorState::Edit;
            commandHistory->Clear();
            selectionManager->SetSelectedEntity(entt::null);

            if (engine->GetScriptSystem()) {
                entt::registry& registry = scene->GetRegistry();
                ScriptSystem* scriptSystem = engine->GetScriptSystem();

                try {
                    nlohmann::json snapshotJson = nlohmann::json::parse(m_PlayModeSnapshot);

                    if (snapshotJson.contains("entities") && snapshotJson["entities"].is_array()) {
                        size_t entityIndex = 0;
                        const std::vector<entt::entity>& entityOrder = scene->GetEntityOrder();

                        for (entt::entity entity : entityOrder) {
                            if (entityIndex >= snapshotJson["entities"].size())
                                break;

                            const nlohmann::json& entityJson = snapshotJson["entities"][entityIndex];

                            if (registry.all_of<Script>(entity)) {
                                Script& scriptComponent = registry.get<Script>(entity);

                                if (entityJson.contains("Scripts") && entityJson["Scripts"].is_array()) {
                                    const nlohmann::json& scriptsArray = entityJson["Scripts"];
                                    for (size_t i = 0;
                                         i < scriptComponent.scripts.size() && i < scriptsArray.size(); ++i) {
                                        const nlohmann::json& scriptJson = scriptsArray[i];
                                        ScriptInstance& script = scriptComponent.scripts[i];

                                        if (!script.scriptName.empty() && !script.instance) {
                                            script.instance = scriptSystem->CreateScript(script.scriptName);
                                            if (script.instance) {
                                                script.instance->Initialize(entity, scene);

                                                if (scriptJson.contains("properties")) {
                                                    const nlohmann::json& propertiesJson = scriptJson["properties"];
                                                    const Reflection::TypeInfo* typeInfo =
                                                        Reflection::TypeRegistry::Instance().GetTypeInfo(
                                                            typeid(*script.instance));

                                                    if (typeInfo) {
                                                        for (const Reflection::FieldInfo& field :
                                                             typeInfo->GetFields()) {
                                                            if ((field.flags &
                                                                 Reflection::FieldFlags::Serializable) &&
                                                                propertiesJson.contains(field.name)) {
                                                                void* fieldPtr = field.getPtr(
                                                                    static_cast<void*>(script.instance.get()));
                                                                Reflection::JsonSerializer::DeserializeField(
                                                                    field, propertiesJson[field.name], fieldPtr);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                else if (entityJson.contains("Script")) {
                                    const nlohmann::json& scriptJson = entityJson["Script"];
                                    if (scriptComponent.scripts.size() > 0) {
                                        ScriptInstance& script = scriptComponent.scripts[0];

                                        if (!script.scriptName.empty() && !script.instance) {
                                            script.instance = scriptSystem->CreateScript(script.scriptName);
                                            if (script.instance) {
                                                script.instance->Initialize(entity, scene);

                                                if (scriptJson.contains("properties")) {
                                                    const nlohmann::json& propertiesJson = scriptJson["properties"];
                                                    const Reflection::TypeInfo* typeInfo =
                                                        Reflection::TypeRegistry::Instance().GetTypeInfo(
                                                            typeid(*script.instance));

                                                    if (typeInfo) {
                                                        for (const Reflection::FieldInfo& field :
                                                             typeInfo->GetFields()) {
                                                            if ((field.flags &
                                                                 Reflection::FieldFlags::Serializable) &&
                                                                propertiesJson.contains(field.name)) {
                                                                void* fieldPtr = field.getPtr(
                                                                    static_cast<void*>(script.instance.get()));
                                                                Reflection::JsonSerializer::DeserializeField(
                                                                    field, propertiesJson[field.name], fieldPtr);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            entityIndex++;
                        }
                    }
                }
                catch (const nlohmann::json::exception& e) {
                    PX_LOG_ERROR(EDITOR, "Failed to restore script properties: %s", e.what());
                }
            }

            PX_LOG_INFO(EDITOR, "Scene restored from memory snapshot - edit mode");
        }
    }
    else {
        m_EditorState = EditorState::Edit;
        commandHistory->Clear();
    }
}

}

#endif
