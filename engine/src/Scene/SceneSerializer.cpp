#include "Scene/SceneSerializer.hpp"
#include "Core/Logger.hpp"
#include "Scene/Scene.hpp"
#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Components/Sprite.hpp"
#include "Components/Camera.hpp"
#include "Components/RigidBody2D.hpp"
#include "Components/BoxCollider2D.hpp"
#include "Components/CircleCollider2D.hpp"
#include "Components/Script.hpp"
#include "Components/Animator.hpp"
#include "Components/UUID.hpp"
#include "Scene/EntityRegistry.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Resources/AssetManager.hpp"
#include "Reflection/Reflection.hpp"
#include "Components/ComponentModuleRegistry.hpp"
#include <fstream>
#include <filesystem>
#include <raylib.h>

namespace PiiXeL {

SceneSerializer::SceneSerializer(Scene* scene)
    : m_Scene{scene}
{
}

bool SceneSerializer::Serialize(const std::string& filepath) {
    if (!m_Scene) {
        return false;
    }

    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string directory = filepath.substr(0, lastSlash);

        if (!DirectoryExists(directory.c_str())) {
            if (!std::filesystem::create_directories(directory)) {
                PX_LOG_ERROR(SCENE, "Failed to create directory: %s", directory.c_str());
                return false;
            }
        }
    }

    nlohmann::json sceneJson{};
    sceneJson["scene"] = m_Scene->GetName();
    sceneJson["entities"] = nlohmann::json::array();

    const std::vector<entt::entity>& entityOrder = m_Scene->GetEntityOrder();
    for (entt::entity entity : entityOrder) {
        nlohmann::json entityJson = SerializeEntity(entity);
        sceneJson["entities"].push_back(entityJson);
    }

    std::ofstream file{filepath};
    if (!file.is_open()) {
        PX_LOG_ERROR(SCENE, "Failed to open file for writing: %s", filepath.c_str());
        return false;
    }

    file << sceneJson.dump(4);
    file.close();

    PX_LOG_INFO(SCENE, "Scene saved to: %s", filepath.c_str());
    return true;
}

bool SceneSerializer::Deserialize(const std::string& filepath) {
    if (!m_Scene) {
        return false;
    }

    std::ifstream file{filepath};
    if (!file.is_open()) {
        PX_LOG_ERROR(SCENE, "Failed to open file for reading: %s", filepath.c_str());
        return false;
    }

    nlohmann::json sceneJson{};
    try {
        file >> sceneJson;
    } catch (const nlohmann::json::exception& e) {
        PX_LOG_ERROR(SCENE, "Failed to parse JSON: %s", e.what());
        return false;
    }
    file.close();

    entt::registry& registry = m_Scene->GetRegistry();
    registry.clear();
    EntityRegistry::Instance().Clear();
    m_Scene->GetEntityOrder().clear();

    if (sceneJson.contains("scene")) {
        m_Scene->SetName(sceneJson["scene"].get<std::string>());
    }

    if (sceneJson.contains("entities") && sceneJson["entities"].is_array()) {
        for (const nlohmann::json& entityJson : sceneJson["entities"]) {
            entt::entity entity = DeserializeEntity(entityJson);
            if (entity != entt::null) {
                m_Scene->GetEntityOrder().push_back(entity);
            }
        }
    }

    PX_LOG_INFO(SCENE, "Scene loaded from: %s", filepath.c_str());
    return true;
}

nlohmann::json SceneSerializer::SerializeEntity(entt::entity entity) {
    entt::registry& registry = m_Scene->GetRegistry();
    nlohmann::json entityJson{};

    if (registry.all_of<UUID>(entity)) {
        UUID uuid = registry.get<UUID>(entity);
        entityJson["uuid"] = uuid.Get();
    }


    if (registry.all_of<Sprite>(entity)) {
        const Sprite& sprite = registry.get<Sprite>(entity);
        entityJson["Sprite"] = {
            {"textureAssetUUID", sprite.textureAssetUUID.Get()},
            {"sourceRect", {sprite.sourceRect.x, sprite.sourceRect.y, sprite.sourceRect.width, sprite.sourceRect.height}},
            {"tint", {sprite.tint.r, sprite.tint.g, sprite.tint.b, sprite.tint.a}},
            {"layer", sprite.layer},
            {"origin", {sprite.origin.x, sprite.origin.y}}
        };
    }

    if (registry.all_of<RigidBody2D>(entity)) {
        const RigidBody2D& rb = registry.get<RigidBody2D>(entity);
        entityJson["RigidBody2D"] = {
            {"type", static_cast<int>(rb.type)},
            {"mass", rb.mass},
            {"friction", rb.friction},
            {"restitution", rb.restitution},
            {"fixedRotation", rb.fixedRotation}
        };
    }

    nlohmann::json moduleJson = ComponentModuleRegistry::Instance().SerializeEntity(registry, entity);
    for (auto it = moduleJson.begin(); it != moduleJson.end(); ++it) {
        entityJson[it.key()] = it.value();
    }

    if (registry.all_of<Script>(entity)) {
        const Script& scriptComponent = registry.get<Script>(entity);
        nlohmann::json scriptsArray = nlohmann::json::array();

        for (const ScriptInstance& script : scriptComponent.scripts) {
            nlohmann::json scriptJson{};
            scriptJson["scriptName"] = script.scriptName;
            scriptJson["enabled"] = script.instance ? script.instance->m_Enabled : true;

            if (script.instance) {
                const Reflection::TypeInfo* typeInfo = Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));
                if (typeInfo) {
                    nlohmann::json propertiesJson{};
                    for (const Reflection::FieldInfo& field : typeInfo->GetFields()) {
                        if (field.flags & Reflection::FieldFlags::Serializable) {
                            void* fieldPtr = field.getPtr(static_cast<void*>(script.instance.get()));
                            propertiesJson[field.name] = Reflection::JsonSerializer::SerializeField(field, fieldPtr);
                        }
                    }
                    scriptJson["properties"] = propertiesJson;
                }
            }

            scriptsArray.push_back(scriptJson);
        }

        entityJson["Scripts"] = scriptsArray;
    }

    return entityJson;
}

entt::entity SceneSerializer::DeserializeEntity(const nlohmann::json& entityJson) {
    entt::registry& registry = m_Scene->GetRegistry();
    entt::entity entity = registry.create();

    UUID uuid;
    if (entityJson.contains("uuid")) {
        uuid = UUID(entityJson["uuid"].get<uint64_t>());
    }
    registry.emplace<UUID>(entity, uuid);
    EntityRegistry::Instance().RegisterEntity(uuid, entity);


    if (entityJson.contains("Sprite")) {
        const nlohmann::json& spriteJson = entityJson["Sprite"];
        Sprite sprite{};

        if (spriteJson.contains("textureAssetUUID")) {
            sprite.textureAssetUUID = UUID{spriteJson["textureAssetUUID"].get<uint64_t>()};
        }

        if (spriteJson.contains("sourceRect") && spriteJson["sourceRect"].is_array() && spriteJson["sourceRect"].size() == 4) {
            sprite.sourceRect.x = spriteJson["sourceRect"][0].get<float>();
            sprite.sourceRect.y = spriteJson["sourceRect"][1].get<float>();
            sprite.sourceRect.width = spriteJson["sourceRect"][2].get<float>();
            sprite.sourceRect.height = spriteJson["sourceRect"][3].get<float>();
        }

        if (spriteJson.contains("tint") && spriteJson["tint"].is_array() && spriteJson["tint"].size() == 4) {
            sprite.tint.r = spriteJson["tint"][0].get<unsigned char>();
            sprite.tint.g = spriteJson["tint"][1].get<unsigned char>();
            sprite.tint.b = spriteJson["tint"][2].get<unsigned char>();
            sprite.tint.a = spriteJson["tint"][3].get<unsigned char>();
        }

        sprite.layer = spriteJson.value("layer", 0);

        if (spriteJson.contains("origin") && spriteJson["origin"].is_array() && spriteJson["origin"].size() == 2) {
            sprite.origin.x = spriteJson["origin"][0].get<float>();
            sprite.origin.y = spriteJson["origin"][1].get<float>();
        }

        registry.emplace<Sprite>(entity, sprite);
    }

    if (entityJson.contains("RigidBody2D")) {
        const nlohmann::json& rbJson = entityJson["RigidBody2D"];
        RigidBody2D rb{};

        rb.type = static_cast<BodyType>(rbJson.value("type", 0));
        rb.mass = rbJson.value("mass", 1.0f);
        rb.friction = rbJson.value("friction", 0.3f);
        rb.restitution = rbJson.value("restitution", 0.0f);
        rb.fixedRotation = rbJson.value("fixedRotation", false);

        registry.emplace<RigidBody2D>(entity, rb);
    }

    for (auto it = entityJson.begin(); it != entityJson.end(); ++it) {
        const std::string& componentName = it.key();
        if (componentName == "uuid" ||
            componentName == "Sprite" || componentName == "RigidBody2D" ||
            componentName == "Scripts" || componentName == "Script") {
            continue;
        }

        ComponentModuleRegistry::Instance().DeserializeComponent(componentName, registry, entity, it.value());
    }

    if (entityJson.contains("Scripts") && entityJson["Scripts"].is_array()) {
        Script scriptComponent{};
        for (const nlohmann::json& scriptJson : entityJson["Scripts"]) {
            std::string scriptName = scriptJson.value("scriptName", "");
            if (!scriptName.empty()) {
                scriptComponent.AddScript(scriptName);
            }
        }
        if (scriptComponent.GetScriptCount() > 0) {
            registry.emplace<Script>(entity, scriptComponent);
        }
    } else if (entityJson.contains("Script")) {
        Script scriptComponent{};
        const nlohmann::json& scriptJson = entityJson["Script"];
        std::string scriptName = scriptJson.value("scriptName", "");
        if (!scriptName.empty()) {
            scriptComponent.AddScript(scriptName);
            registry.emplace<Script>(entity, scriptComponent);
        }
    }

    return entity;
}

std::string SceneSerializer::SerializeToString() {
    if (!m_Scene) {
        return "";
    }

    nlohmann::json sceneJson{};
    sceneJson["scene"] = m_Scene->GetName();
    sceneJson["entities"] = nlohmann::json::array();

    const std::vector<entt::entity>& entityOrder = m_Scene->GetEntityOrder();
    for (entt::entity entity : entityOrder) {
        nlohmann::json entityJson = SerializeEntity(entity);
        sceneJson["entities"].push_back(entityJson);
    }

    return sceneJson.dump();
}

bool SceneSerializer::DeserializeFromString(const std::string& data) {
    if (!m_Scene || data.empty()) {
        return false;
    }

    nlohmann::json sceneJson{};
    try {
        sceneJson = nlohmann::json::parse(data);
    } catch (const nlohmann::json::exception& e) {
        PX_LOG_ERROR(SCENE, "Failed to parse JSON from string: %s", e.what());
        return false;
    }

    entt::registry& registry = m_Scene->GetRegistry();
    registry.clear();
    EntityRegistry::Instance().Clear();
    m_Scene->GetEntityOrder().clear();

    if (sceneJson.contains("scene")) {
        m_Scene->SetName(sceneJson["scene"].get<std::string>());
    }

    if (sceneJson.contains("entities") && sceneJson["entities"].is_array()) {
        for (const nlohmann::json& entityJson : sceneJson["entities"]) {
            entt::entity entity = DeserializeEntity(entityJson);
            if (entity != entt::null) {
                m_Scene->GetEntityOrder().push_back(entity);
            }
        }
    }

    PX_LOG_INFO(SCENE, "Scene loaded from memory snapshot");
    return true;
}

} // namespace PiiXeL
