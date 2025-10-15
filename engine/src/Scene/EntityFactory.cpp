#include "Scene/EntityFactory.hpp"

#include "Components/ComponentModuleRegistry.hpp"
#include "Components/Script.hpp"
#include "Components/Sprite.hpp"
#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Components/UUID.hpp"
#include "Core/Engine.hpp"
#include "Core/Logger.hpp"
#include "Scene/EntityRegistry.hpp"
#include "Scene/Scene.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Scripting/ScriptRegistry.hpp"

namespace PiiXeL {

entt::entity EntityFactory::CreateEntity(Scene* scene, const std::string& name) {
    if (!scene) {
        PX_LOG_ERROR(SCENE, "Cannot create entity: scene is null");
        return entt::null;
    }

    entt::registry& registry = scene->GetRegistry();
    entt::entity entity = registry.create();

    UUID uuid;
    registry.emplace<UUID>(entity, uuid);
    EntityRegistry::Instance().RegisterEntity(uuid, entity);

    registry.emplace<Tag>(entity, name);
    registry.emplace<Transform>(entity);

    scene->GetEntityOrder().push_back(entity);

    return entity;
}

entt::entity EntityFactory::DuplicateEntity(Scene* scene, entt::entity source,
                                           const DuplicationOptions& options) {
    if (!scene) {
        PX_LOG_ERROR(SCENE, "Cannot duplicate entity: scene is null");
        return entt::null;
    }

    entt::registry& registry = scene->GetRegistry();

    if (!registry.valid(source)) {
        PX_LOG_ERROR(SCENE, "Cannot duplicate entity: source entity is invalid");
        return entt::null;
    }

    entt::entity destination = registry.create();

    DuplicateBuiltInComponents(registry, source, destination, options);

    if (options.duplicateModuleComponents) {
        DuplicateModuleComponents(registry, source, destination);
    }

    scene->GetEntityOrder().push_back(destination);

    PX_LOG_INFO(SCENE, "Entity duplicated successfully");
    return destination;
}

entt::entity EntityFactory::DuplicateEntity(Scene* scene, entt::entity source) {
    return DuplicateEntity(scene, source, DuplicationOptions{});
}

entt::entity EntityFactory::DuplicateEntity(Engine* engine, entt::entity source,
                                           const DuplicationOptions& options) {
    if (!engine || !engine->GetActiveScene()) {
        PX_LOG_ERROR(SCENE, "Cannot duplicate entity: engine or scene is null");
        return entt::null;
    }

    return DuplicateEntity(engine->GetActiveScene(), source, options);
}

entt::entity EntityFactory::DuplicateEntity(Engine* engine, entt::entity source) {
    return DuplicateEntity(engine, source, DuplicationOptions{});
}

void EntityFactory::DuplicateBuiltInComponents(entt::registry& registry, entt::entity source,
                                               entt::entity destination, const DuplicationOptions& options) {
    UUID uuid;
    registry.emplace<UUID>(destination, uuid);
    EntityRegistry::Instance().RegisterEntity(uuid, destination);

    if (registry.all_of<Tag>(source)) {
        const Tag& sourceTag = registry.get<Tag>(source);
        Tag destTag;
        destTag.name = options.appendCopyToName ? sourceTag.name + " (Copy)" : sourceTag.name;
        registry.emplace<Tag>(destination, destTag);
    }

    if (options.duplicateTransform && registry.all_of<Transform>(source)) {
        const Transform& sourceTransform = registry.get<Transform>(source);
        registry.emplace<Transform>(destination, sourceTransform);
    }

    if (options.duplicateSprite && registry.all_of<Sprite>(source)) {
        const Sprite& sourceSprite = registry.get<Sprite>(source);
        Sprite destSprite;
        destSprite.textureAssetUUID = sourceSprite.textureAssetUUID;
        destSprite.tint = sourceSprite.tint;
        destSprite.sourceRect = sourceSprite.sourceRect;
        destSprite.origin = sourceSprite.origin;
        destSprite.layer = sourceSprite.layer;
        registry.emplace<Sprite>(destination, destSprite);
    }

    if (options.duplicateScripts && registry.all_of<Script>(source)) {
        const Script& sourceScript = registry.get<Script>(source);
        Script destScript;
        for (const ScriptInstance& sourceScriptInstance : sourceScript.scripts) {
            if (!sourceScriptInstance.scriptName.empty()) {
                std::shared_ptr<ScriptComponent> newInstance =
                    ScriptRegistry::Instance().CreateScript(sourceScriptInstance.scriptName);
                if (newInstance) {
                    destScript.AddScript(newInstance, sourceScriptInstance.scriptName);
                } else {
                    destScript.AddScript(sourceScriptInstance.scriptName);
                }
            }
        }
        registry.emplace<Script>(destination, destScript);
    }
}

void EntityFactory::DuplicateModuleComponents(entt::registry& registry, entt::entity source,
                                              entt::entity destination) {
    ComponentModuleRegistry::Instance().DuplicateAllComponents(registry, source, destination);
}

} // namespace PiiXeL
