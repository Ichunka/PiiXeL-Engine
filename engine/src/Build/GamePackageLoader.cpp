#include "Build/GamePackageLoader.hpp"
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
#include "Systems/ScriptSystem.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Reflection/Reflection.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/AssetPackage.hpp"
#include <raylib.h>
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace PiiXeL {

GamePackageLoader::GamePackageLoader()
    : m_Package{}
    , m_IsLoaded{false}
    , m_LoadedTextures{}
{
}

GamePackageLoader::~GamePackageLoader() {
    UnloadAllTextures();
}

bool GamePackageLoader::LoadPackage(const std::string& filepath) {
    if (!m_Package.LoadFromFile(filepath)) {
        return false;
    }

    m_IsLoaded = true;
    TraceLog(LOG_INFO, "Game package loaded successfully");
    return true;
}

std::unique_ptr<Scene> GamePackageLoader::LoadScene(const std::string& sceneName, ScriptSystem* scriptSystem) {
    if (!m_IsLoaded) {
        TraceLog(LOG_ERROR, "No package loaded");
        return nullptr;
    }

    const nlohmann::json* sceneData = nullptr;
    for (const std::pair<std::string, nlohmann::json>& scenePair : m_Package.GetScenes()) {
        if (scenePair.first == sceneName) {
            sceneData = &scenePair.second;
            break;
        }
    }

    if (!sceneData) {
        TraceLog(LOG_ERROR, "Scene not found in package: %s", sceneName.c_str());
        return nullptr;
    }

    std::unique_ptr<Scene> scene = std::make_unique<Scene>(sceneName);
    entt::registry& registry = scene->GetRegistry();

    EntityRegistry::Instance().Clear();

    if (sceneData->contains("entities") && (*sceneData)["entities"].is_array()) {
        for (const nlohmann::json& entityJson : (*sceneData)["entities"]) {
            entt::entity entity = registry.create();

            UUID uuid{};
            if (entityJson.contains("uuid")) {
                uuid = UUID(entityJson["uuid"].get<uint64_t>());
            }
            registry.emplace<UUID>(entity, uuid);
            EntityRegistry::Instance().RegisterEntity(uuid, entity);
            scene->GetEntityOrder().push_back(entity);

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
        }
    }

    if (scriptSystem && sceneData->contains("entities") && (*sceneData)["entities"].is_array()) {
        size_t entityIndex = 0;
        for (entt::entity entity : scene->GetEntityOrder()) {
            if (entityIndex >= (*sceneData)["entities"].size()) break;

            const nlohmann::json& entityJson = (*sceneData)["entities"][entityIndex];

            if (entityJson.contains("Script") && registry.all_of<Script>(entity)) {
                const nlohmann::json& scriptJson = entityJson["Script"];
                Script& script = registry.get<Script>(entity);

                if (!script.scriptName.empty() && !script.instance) {
                    script.instance = scriptSystem->CreateScript(script.scriptName);

                    if (script.instance) {
                        script.instance->Initialize(entity, scene.get());

                        if (scriptJson.contains("properties")) {
                            const nlohmann::json& propertiesJson = scriptJson["properties"];
                            const Reflection::TypeInfo* typeInfo = Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));

                            if (typeInfo) {
                                for (const Reflection::FieldInfo& field : typeInfo->GetFields()) {
                                    if ((field.flags & Reflection::FieldFlags::Serializable) && propertiesJson.contains(field.name)) {
                                        void* fieldPtr = field.getPtr(static_cast<void*>(script.instance.get()));
                                        Reflection::JsonSerializer::DeserializeField(field, propertiesJson[field.name], fieldPtr);
                                    }
                                }
                            }
                        }

                        if (scriptJson.contains("enabled")) {
                            script.instance->m_Enabled = scriptJson["enabled"].get<bool>();
                        }
                    }
                }
            }

            entityIndex++;
        }
    }

    TraceLog(LOG_INFO, "Scene loaded from package: %s", sceneName.c_str());
    return scene;
}

Texture2D GamePackageLoader::LoadTexture(const std::string& assetPath) {
    auto it = m_LoadedTextures.find(assetPath);
    if (it != m_LoadedTextures.end()) {
        return it->second;
    }

    const AssetData* asset = m_Package.GetAsset(assetPath);
    if (!asset || asset->type != "texture") {
        TraceLog(LOG_ERROR, "Texture not found in package: %s", assetPath.c_str());
        return Texture2D{};
    }

    Image image = LoadImageFromMemory(".png", asset->data.data(), static_cast<int>(asset->data.size()));
    if (image.data == nullptr) {
        TraceLog(LOG_ERROR, "Failed to load texture from package: %s", assetPath.c_str());
        return Texture2D{};
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);

    m_LoadedTextures[assetPath] = texture;
    TraceLog(LOG_INFO, "Texture loaded from package: %s", assetPath.c_str());
    return texture;
}

void GamePackageLoader::UnloadAllTextures() {
    for (std::pair<const std::string, Texture2D>& pair : m_LoadedTextures) {
        UnloadTexture(pair.second);
    }
    m_LoadedTextures.clear();
}

void GamePackageLoader::InitializeAssetRegistry() {
    if (!m_IsLoaded) {
        TraceLog(LOG_ERROR, "Cannot initialize AssetRegistry: no package loaded");
        return;
    }

    const AssetData* uuidCache = m_Package.GetAsset("datas/.asset_uuid_cache");
    if (uuidCache && !uuidCache->data.empty()) {
        AssetRegistry::Instance().LoadUUIDCacheFromMemory(uuidCache->data.data(), uuidCache->data.size());
        TraceLog(LOG_INFO, "Loaded UUID cache from package");
    }

    size_t registeredCount = 0;
    for (const AssetData& asset : m_Package.GetAssets()) {
        std::string normalizedPath = asset.path;
        std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

        if (normalizedPath.ends_with(".pxa")) {
            AssetMetadata metadata{};
            std::vector<uint8_t> assetData{};

            AssetPackage package{};
            if (package.LoadFromMemory(asset.data.data(), asset.data.size(), metadata, assetData, normalizedPath)) {
                AssetRegistry::Instance().RegisterAssetFromMemory(metadata.uuid, metadata.sourceFile, asset.data);
                registeredCount++;
            }
        }
    }

    TraceLog(LOG_INFO, "Registered %zu .pxa assets from package (in-memory)", registeredCount);
}

} // namespace PiiXeL
