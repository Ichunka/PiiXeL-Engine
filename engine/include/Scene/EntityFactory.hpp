#pragma once

#include <entt/entt.hpp>

#include <string>

namespace PiiXeL {

class Scene;
class Engine;

class EntityFactory {
public:
    struct DuplicationOptions {
        bool duplicateTransform{true};
        bool duplicateSprite{true};
        bool duplicateScripts{true};
        bool duplicateModuleComponents{true};
        bool appendCopyToName{true};
    };

    static entt::entity CreateEntity(Scene* scene, const std::string& name = "Entity");

    static entt::entity DuplicateEntity(Scene* scene, entt::entity source, const DuplicationOptions& options);
    static entt::entity DuplicateEntity(Scene* scene, entt::entity source);

    static entt::entity DuplicateEntity(Engine* engine, entt::entity source, const DuplicationOptions& options);
    static entt::entity DuplicateEntity(Engine* engine, entt::entity source);

private:
    static void DuplicateBuiltInComponents(entt::registry& registry, entt::entity source, entt::entity destination,
                                           const DuplicationOptions& options);

    static void DuplicateModuleComponents(entt::registry& registry, entt::entity source, entt::entity destination);
};

} // namespace PiiXeL
