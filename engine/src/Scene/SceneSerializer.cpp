#include "Scene/SceneSerializer.hpp"
#include "Scene/Scene.hpp"
#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Components/Sprite.hpp"
#include "Components/Camera.hpp"
#include "Components/RigidBody2D.hpp"
#include "Components/BoxCollider2D.hpp"
#include "Components/Script.hpp"
#include "Components/UUID.hpp"
#include "Scene/EntityRegistry.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Resources/AssetManager.hpp"
#include "Reflection/Reflection.hpp"
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
                TraceLog(LOG_ERROR, "Failed to create directory: %s", directory.c_str());
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
        TraceLog(LOG_ERROR, "Failed to open file for writing: %s", filepath.c_str());
        return false;
    }

    file << sceneJson.dump(4);
    file.close();

    TraceLog(LOG_INFO, "Scene saved to: %s", filepath.c_str());
    return true;
}

bool SceneSerializer::Deserialize(const std::string& filepath) {
    if (!m_Scene) {
        return false;
    }

    std::ifstream file{filepath};
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open file for reading: %s", filepath.c_str());
        return false;
    }

    nlohmann::json sceneJson{};
    try {
        file >> sceneJson;
    } catch (const nlohmann::json::exception& e) {
        TraceLog(LOG_ERROR, "Failed to parse JSON: %s", e.what());
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

    TraceLog(LOG_INFO, "Scene loaded from: %s", filepath.c_str());
    return true;
}

nlohmann::json SceneSerializer::SerializeEntity(entt::entity entity) {
    entt::registry& registry = m_Scene->GetRegistry();
    nlohmann::json entityJson{};

    if (registry.all_of<UUID>(entity)) {
        UUID uuid = registry.get<UUID>(entity);
        entityJson["uuid"] = uuid.Get();
    }

    if (registry.all_of<Tag>(entity)) {
        const Tag& tag = registry.get<Tag>(entity);
        entityJson["Tag"] = {
            {"name", tag.name}
        };
    }

    if (registry.all_of<Transform>(entity)) {
        const Transform& transform = registry.get<Transform>(entity);
        entityJson["Transform"] = {
            {"position", {transform.position.x, transform.position.y}},
            {"rotation", transform.rotation},
            {"scale", {transform.scale.x, transform.scale.y}}
        };
    }

    if (registry.all_of<Sprite>(entity)) {
        const Sprite& sprite = registry.get<Sprite>(entity);
        entityJson["Sprite"] = {
            {"texturePath", sprite.texturePath},
            {"sourceRect", {sprite.sourceRect.x, sprite.sourceRect.y, sprite.sourceRect.width, sprite.sourceRect.height}},
            {"tint", {sprite.tint.r, sprite.tint.g, sprite.tint.b, sprite.tint.a}},
            {"layer", sprite.layer},
            {"origin", {sprite.origin.x, sprite.origin.y}}
        };
    }

    if (registry.all_of<Camera>(entity)) {
        const Camera& camera = registry.get<Camera>(entity);
        entityJson["Camera"] = {
            {"isPrimary", camera.isPrimary},
            {"zoom", camera.zoom},
            {"offset", {camera.offset.x, camera.offset.y}},
            {"rotation", camera.rotation}
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

    if (registry.all_of<BoxCollider2D>(entity)) {
        const BoxCollider2D& collider = registry.get<BoxCollider2D>(entity);
        entityJson["BoxCollider2D"] = {
            {"size", {collider.size.x, collider.size.y}},
            {"offset", {collider.offset.x, collider.offset.y}},
            {"isTrigger", collider.isTrigger}
        };
    }

    if (registry.all_of<Script>(entity)) {
        const Script& script = registry.get<Script>(entity);
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

        entityJson["Script"] = scriptJson;
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

    if (entityJson.contains("Tag")) {
        const nlohmann::json& tagJson = entityJson["Tag"];
        Tag tag{};
        tag.name = tagJson.value("name", "Entity");
        registry.emplace<Tag>(entity, tag);
    }

    if (entityJson.contains("Transform")) {
        const nlohmann::json& transformJson = entityJson["Transform"];
        Transform transform{};

        if (transformJson.contains("position") && transformJson["position"].is_array() && transformJson["position"].size() == 2) {
            transform.position.x = transformJson["position"][0].get<float>();
            transform.position.y = transformJson["position"][1].get<float>();
        }

        if (transformJson.contains("rotation")) {
            transform.rotation = transformJson["rotation"].get<float>();
        }

        if (transformJson.contains("scale") && transformJson["scale"].is_array() && transformJson["scale"].size() == 2) {
            transform.scale.x = transformJson["scale"][0].get<float>();
            transform.scale.y = transformJson["scale"][1].get<float>();
        }

        registry.emplace<Transform>(entity, transform);
    }

    if (entityJson.contains("Sprite")) {
        const nlohmann::json& spriteJson = entityJson["Sprite"];
        Sprite sprite{};

        sprite.texturePath = spriteJson.value("texturePath", "");

        // Only load texture in editor mode - in game mode, GamePackageLoader handles it
#ifdef BUILD_WITH_EDITOR
        if (!sprite.texturePath.empty()) {
            sprite.texture = AssetManager::Instance().LoadTexture(sprite.texturePath);
        }
#endif

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

    if (entityJson.contains("Camera")) {
        const nlohmann::json& cameraJson = entityJson["Camera"];
        Camera camera{};

        camera.isPrimary = cameraJson.value("isPrimary", false);
        camera.zoom = cameraJson.value("zoom", 1.0f);
        camera.rotation = cameraJson.value("rotation", 0.0f);

        if (cameraJson.contains("offset") && cameraJson["offset"].is_array() && cameraJson["offset"].size() == 2) {
            camera.offset.x = cameraJson["offset"][0].get<float>();
            camera.offset.y = cameraJson["offset"][1].get<float>();
        }

        registry.emplace<Camera>(entity, camera);
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

    if (entityJson.contains("BoxCollider2D")) {
        const nlohmann::json& colliderJson = entityJson["BoxCollider2D"];
        BoxCollider2D collider{};

        if (colliderJson.contains("size") && colliderJson["size"].is_array() && colliderJson["size"].size() == 2) {
            collider.size.x = colliderJson["size"][0].get<float>();
            collider.size.y = colliderJson["size"][1].get<float>();
        }

        if (colliderJson.contains("offset") && colliderJson["offset"].is_array() && colliderJson["offset"].size() == 2) {
            collider.offset.x = colliderJson["offset"][0].get<float>();
            collider.offset.y = colliderJson["offset"][1].get<float>();
        }

        collider.isTrigger = colliderJson.value("isTrigger", false);

        registry.emplace<BoxCollider2D>(entity, collider);
    }

    if (entityJson.contains("Script")) {
        const nlohmann::json& scriptJson = entityJson["Script"];
        Script script{};
        script.scriptName = scriptJson.value("scriptName", "");
        registry.emplace<Script>(entity, script);
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
        TraceLog(LOG_ERROR, "Failed to parse JSON from string: %s", e.what());
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

    TraceLog(LOG_INFO, "Scene loaded from memory snapshot");
    return true;
}

} // namespace PiiXeL
