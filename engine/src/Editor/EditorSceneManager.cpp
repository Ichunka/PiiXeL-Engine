#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorSceneManager.hpp"

#include "Components/Script.hpp"
#include "Core/Engine.hpp"
#include "Core/Logger.hpp"
#include "Reflection/Reflection.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneSerializer.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Systems/ScriptSystem.hpp"

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <raylib.h>

namespace PiiXeL {

EditorSceneManager::EditorSceneManager(Engine* engine) : m_Engine{engine}, m_CurrentScenePath{} {}

void EditorSceneManager::NewScene() {
    if (m_Engine && m_Engine->GetActiveScene()) {
        Scene* scene = m_Engine->GetActiveScene();
        entt::registry& registry = scene->GetRegistry();
        registry.clear();

        scene->SetName("Untitled Scene");
        m_CurrentScenePath.clear();

        PX_LOG_INFO(EDITOR, "New scene created");
    }
}

void EditorSceneManager::SaveScene() {
    if (m_CurrentScenePath.empty()) {
        SaveSceneAs();
        return;
    }

    if (m_Engine && m_Engine->GetActiveScene()) {
        Scene* scene = m_Engine->GetActiveScene();
        SceneSerializer serializer{scene};
        serializer.Serialize(m_CurrentScenePath);
    }
}

void EditorSceneManager::SaveSceneAs() {
    if (!m_Engine || !m_Engine->GetActiveScene()) {
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    std::string filename = scene->GetName();

    for (char& c : filename) {
        if (c == ' ') {
            c = '_';
        }
    }

    m_CurrentScenePath = "content/scenes/" + filename + ".scene";

    SceneSerializer serializer{scene};
    serializer.Serialize(m_CurrentScenePath);
}

void EditorSceneManager::LoadScene() {
    if (!m_Engine || !m_Engine->GetActiveScene()) {
        return;
    }

    if (m_CurrentScenePath.empty()) {
        PX_LOG_WARNING(EDITOR, "No scene path set. Save the scene first.");
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    SceneSerializer serializer{scene};
    serializer.Deserialize(m_CurrentScenePath);

    RestoreScriptPropertiesFromFile(m_CurrentScenePath);
}

void EditorSceneManager::RestoreScriptPropertiesFromFile(const std::string& filepath) {
    if (!m_Engine || !m_Engine->GetActiveScene() || !m_Engine->GetScriptSystem()) {
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();
    ScriptSystem* scriptSystem = m_Engine->GetScriptSystem();

    try {
        std::ifstream file{filepath};
        if (!file.is_open()) {
            return;
        }

        nlohmann::json sceneJson{};
        file >> sceneJson;
        file.close();

        if (!sceneJson.contains("entities") || !sceneJson["entities"].is_array()) {
            return;
        }

        size_t entityIndex = 0;
        const std::vector<entt::entity>& entityOrder = scene->GetEntityOrder();

        for (entt::entity entity : entityOrder) {
            if (entityIndex >= sceneJson["entities"].size())
                break;

            const nlohmann::json& entityJson = sceneJson["entities"][entityIndex];

            if (registry.all_of<Script>(entity)) {
                Script& scriptComponent = registry.get<Script>(entity);

                if (entityJson.contains("Scripts") && entityJson["Scripts"].is_array()) {
                    const nlohmann::json& scriptsArray = entityJson["Scripts"];
                    size_t scriptCount = std::min(scriptComponent.scripts.size(), scriptsArray.size());

                    for (size_t i = 0; i < scriptCount; ++i) {
                        ScriptInstance& script = scriptComponent.scripts[i];
                        const nlohmann::json& scriptJson = scriptsArray[i];

                        if (!script.scriptName.empty() && !script.instance) {
                            script.instance = scriptSystem->CreateScript(script.scriptName);
                            if (script.instance) {
                                script.instance->Initialize(entity, scene);
                            }
                        }

                        if (script.instance && scriptJson.contains("properties")) {
                            const nlohmann::json& propertiesJson = scriptJson["properties"];
                            const Reflection::TypeInfo* typeInfo =
                                Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));

                            if (typeInfo) {
                                for (const Reflection::FieldInfo& field : typeInfo->GetFields()) {
                                    if ((field.flags & Reflection::FieldFlags::Serializable) &&
                                        propertiesJson.contains(field.name)) {
                                        void* fieldPtr = field.getPtr(static_cast<void*>(script.instance.get()));
                                        Reflection::JsonSerializer::DeserializeField(field, propertiesJson[field.name],
                                                                                     fieldPtr);
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
                            }
                        }

                        if (script.instance && scriptJson.contains("properties")) {
                            const nlohmann::json& propertiesJson = scriptJson["properties"];
                            const Reflection::TypeInfo* typeInfo =
                                Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));

                            if (typeInfo) {
                                for (const Reflection::FieldInfo& field : typeInfo->GetFields()) {
                                    if ((field.flags & Reflection::FieldFlags::Serializable) &&
                                        propertiesJson.contains(field.name)) {
                                        void* fieldPtr = field.getPtr(static_cast<void*>(script.instance.get()));
                                        Reflection::JsonSerializer::DeserializeField(field, propertiesJson[field.name],
                                                                                     fieldPtr);
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
    catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Failed to restore script properties: %s", e.what());
    }
}

} // namespace PiiXeL

#endif
