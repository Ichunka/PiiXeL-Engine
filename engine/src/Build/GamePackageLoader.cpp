#include "Build/GamePackageLoader.hpp"

#include "Components/Animator.hpp"
#include "Components/BoxCollider2D.hpp"
#include "Components/Camera.hpp"
#include "Components/CircleCollider2D.hpp"
#include "Components/RigidBody2D.hpp"
#include "Components/Script.hpp"
#include "Components/Sprite.hpp"
#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Components/UUID.hpp"
#include "Core/Logger.hpp"
#include "Reflection/Reflection.hpp"
#include "Resources/AssetPackage.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Scene/ComponentRegistry.hpp"
#include "Scene/EntityRegistry.hpp"
#include "Scene/Scene.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Systems/ScriptSystem.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <raylib.h>

namespace PiiXeL {

GamePackageLoader::GamePackageLoader() : m_Package{}, m_IsLoaded{false}, m_LoadedTextures{} {}

GamePackageLoader::~GamePackageLoader() {
    UnloadAllTextures();
}

bool GamePackageLoader::LoadPackage(const std::string& filepath) {
    if (!m_Package.LoadFromFile(filepath)) {
        return false;
    }

    m_IsLoaded = true;
    PX_LOG_INFO(BUILD, "Game package loaded successfully");
    return true;
}

std::unique_ptr<Scene> GamePackageLoader::LoadScene(const std::string& sceneName, ScriptSystem* scriptSystem) {
    if (!m_IsLoaded) {
        PX_LOG_ERROR(BUILD, "No package loaded");
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
        PX_LOG_ERROR(BUILD, "Scene not found in package: %s", sceneName.c_str());
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

            ComponentRegistry& componentRegistry = ComponentRegistry::Instance();

            for (auto it = entityJson.begin(); it != entityJson.end(); ++it) {
                const std::string& componentName = it.key();
                const nlohmann::json& componentData = it.value();

                if (componentName == "uuid") {
                    continue;
                }

                if (componentName == "Scripts" && componentData.is_array()) {
                    Script script{};
                    for (const nlohmann::json& scriptJson : componentData) {
                        std::string scriptName = scriptJson.value("scriptName", "");
                        if (!scriptName.empty()) {
                            script.AddScript(scriptName);
                        }
                    }
                    if (script.GetScriptCount() > 0) {
                        registry.emplace<Script>(entity, script);
                    }
                    continue;
                }

                if (componentName == "Script") {
                    Script script{};
                    std::string scriptName = componentData.value("scriptName", "");
                    if (!scriptName.empty()) {
                        script.AddScript(scriptName);
                        registry.emplace<Script>(entity, script);
                    }
                    continue;
                }

                if (componentRegistry.HasComponent(componentName)) {
                    componentRegistry.DeserializeComponent(componentName, registry, entity, componentData);
                }
            }
        }
    }

    if (scriptSystem && sceneData->contains("entities") && (*sceneData)["entities"].is_array()) {
        size_t entityIndex = 0;
        for (entt::entity entity : scene->GetEntityOrder()) {
            if (entityIndex >= (*sceneData)["entities"].size())
                break;

            const nlohmann::json& entityJson = (*sceneData)["entities"][entityIndex];

            if (registry.all_of<Script>(entity)) {
                Script& scriptComponent = registry.get<Script>(entity);

                if (entityJson.contains("Scripts") && entityJson["Scripts"].is_array()) {
                    const nlohmann::json& scriptsArray = entityJson["Scripts"];
                    for (size_t i = 0; i < scriptComponent.scripts.size() && i < scriptsArray.size(); ++i) {
                        const nlohmann::json& scriptJson = scriptsArray[i];
                        ScriptInstance& script = scriptComponent.scripts[i];

                        if (!script.scriptName.empty() && !script.instance) {
                            script.instance = scriptSystem->CreateScript(script.scriptName);

                            if (script.instance) {
                                script.instance->Initialize(entity, scene.get());

                                if (scriptJson.contains("properties")) {
                                    const nlohmann::json& propertiesJson = scriptJson["properties"];
                                    const Reflection::TypeInfo* typeInfo =
                                        Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));

                                    if (typeInfo) {
                                        for (const Reflection::FieldInfo& field : typeInfo->GetFields()) {
                                            if ((field.flags & Reflection::FieldFlags::Serializable) &&
                                                propertiesJson.contains(field.name))
                                            {
                                                void* fieldPtr =
                                                    field.getPtr(static_cast<void*>(script.instance.get()));
                                                Reflection::JsonSerializer::DeserializeField(
                                                    field, propertiesJson[field.name], fieldPtr);
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
                }
                else if (entityJson.contains("Script")) {
                    const nlohmann::json& scriptJson = entityJson["Script"];
                    if (scriptComponent.scripts.size() > 0) {
                        ScriptInstance& script = scriptComponent.scripts[0];

                        if (!script.scriptName.empty() && !script.instance) {
                            script.instance = scriptSystem->CreateScript(script.scriptName);

                            if (script.instance) {
                                script.instance->Initialize(entity, scene.get());

                                if (scriptJson.contains("properties")) {
                                    const nlohmann::json& propertiesJson = scriptJson["properties"];
                                    const Reflection::TypeInfo* typeInfo =
                                        Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));

                                    if (typeInfo) {
                                        for (const Reflection::FieldInfo& field : typeInfo->GetFields()) {
                                            if ((field.flags & Reflection::FieldFlags::Serializable) &&
                                                propertiesJson.contains(field.name))
                                            {
                                                void* fieldPtr =
                                                    field.getPtr(static_cast<void*>(script.instance.get()));
                                                Reflection::JsonSerializer::DeserializeField(
                                                    field, propertiesJson[field.name], fieldPtr);
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
                }
            }

            entityIndex++;
        }
    }

    PX_LOG_INFO(BUILD, "Scene loaded from package: %s", sceneName.c_str());
    return scene;
}

Texture2D GamePackageLoader::LoadTexture(const std::string& assetPath) {
    auto it = m_LoadedTextures.find(assetPath);
    if (it != m_LoadedTextures.end()) {
        return it->second;
    }

    const AssetData* asset = m_Package.GetAsset(assetPath);
    if (!asset || asset->type != "texture") {
        PX_LOG_ERROR(BUILD, "Texture not found in package: %s", assetPath.c_str());
        return Texture2D{};
    }

    Image image = LoadImageFromMemory(".png", asset->data.data(), static_cast<int>(asset->data.size()));
    if (image.data == nullptr) {
        PX_LOG_ERROR(BUILD, "Failed to load texture from package: %s", assetPath.c_str());
        return Texture2D{};
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);

    m_LoadedTextures[assetPath] = texture;
    PX_LOG_INFO(BUILD, "Texture loaded from package: %s", assetPath.c_str());
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
        PX_LOG_ERROR(BUILD, "Cannot initialize AssetRegistry: no package loaded");
        return;
    }

    const AssetData* uuidCache = m_Package.GetAsset("datas/.asset_uuid_cache");
    if (uuidCache && !uuidCache->data.empty()) {
        AssetRegistry::Instance().LoadUUIDCacheFromMemory(uuidCache->data.data(), uuidCache->data.size());
        PX_LOG_INFO(BUILD, "Loaded UUID cache from package");
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

    PX_LOG_INFO(BUILD, "Registered %llu .pxa assets from package (in-memory)",
                static_cast<unsigned long long>(registeredCount));
}

} // namespace PiiXeL
