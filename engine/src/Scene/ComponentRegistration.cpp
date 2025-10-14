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
#include "Scene/ComponentRegistry.hpp"

#include <nlohmann/json.hpp>

namespace PiiXeL {

void RegisterAllComponents() {
    ComponentRegistry& registry = ComponentRegistry::Instance();

    registry.RegisterComponent("Tag", [](entt::registry& reg, entt::entity entity, const nlohmann::json& data) {
        Tag tag{};
        tag.name = data.value("name", "Entity");
        reg.emplace<Tag>(entity, tag);
    });

    registry.RegisterComponent("Transform", [](entt::registry& reg, entt::entity entity, const nlohmann::json& data) {
        Transform transform{};

        if (data.contains("position") && data["position"].is_array() && data["position"].size() == 2) {
            transform.position.x = data["position"][0].get<float>();
            transform.position.y = data["position"][1].get<float>();
        }

        if (data.contains("rotation")) {
            transform.rotation = data["rotation"].get<float>();
        }

        if (data.contains("scale") && data["scale"].is_array() && data["scale"].size() == 2) {
            transform.scale.x = data["scale"][0].get<float>();
            transform.scale.y = data["scale"][1].get<float>();
        }

        reg.emplace<Transform>(entity, transform);
    });

    registry.RegisterComponent("Sprite", [](entt::registry& reg, entt::entity entity, const nlohmann::json& data) {
        Sprite sprite{};

        if (data.contains("textureAssetUUID")) {
            sprite.textureAssetUUID = UUID{data["textureAssetUUID"].get<uint64_t>()};
        }

        if (data.contains("sourceRect") && data["sourceRect"].is_array() && data["sourceRect"].size() == 4) {
            sprite.sourceRect.x = data["sourceRect"][0].get<float>();
            sprite.sourceRect.y = data["sourceRect"][1].get<float>();
            sprite.sourceRect.width = data["sourceRect"][2].get<float>();
            sprite.sourceRect.height = data["sourceRect"][3].get<float>();
        }

        if (data.contains("tint") && data["tint"].is_array() && data["tint"].size() == 4) {
            sprite.tint.r = data["tint"][0].get<unsigned char>();
            sprite.tint.g = data["tint"][1].get<unsigned char>();
            sprite.tint.b = data["tint"][2].get<unsigned char>();
            sprite.tint.a = data["tint"][3].get<unsigned char>();
        }

        sprite.layer = data.value("layer", 0);

        if (data.contains("origin") && data["origin"].is_array() && data["origin"].size() == 2) {
            sprite.origin.x = data["origin"][0].get<float>();
            sprite.origin.y = data["origin"][1].get<float>();
        }

        reg.emplace<Sprite>(entity, sprite);
    });

    registry.RegisterComponent("Camera", [](entt::registry& reg, entt::entity entity, const nlohmann::json& data) {
        Camera camera{};

        camera.isPrimary = data.value("isPrimary", false);
        camera.zoom = data.value("zoom", 1.0f);
        camera.rotation = data.value("rotation", 0.0f);

        if (data.contains("offset") && data["offset"].is_array() && data["offset"].size() == 2) {
            camera.offset.x = data["offset"][0].get<float>();
            camera.offset.y = data["offset"][1].get<float>();
        }

        reg.emplace<Camera>(entity, camera);
    });

    registry.RegisterComponent("RigidBody2D", [](entt::registry& reg, entt::entity entity, const nlohmann::json& data) {
        RigidBody2D rb{};

        rb.type = static_cast<BodyType>(data.value("type", 0));
        rb.mass = data.value("mass", 1.0f);
        rb.friction = data.value("friction", 0.3f);
        rb.restitution = data.value("restitution", 0.0f);
        rb.fixedRotation = data.value("fixedRotation", false);

        reg.emplace<RigidBody2D>(entity, rb);
    });

    registry.RegisterComponent(
        "BoxCollider2D", [](entt::registry& reg, entt::entity entity, const nlohmann::json& data) {
            BoxCollider2D collider{};

            if (data.contains("size") && data["size"].is_array() && data["size"].size() == 2) {
                collider.size.x = data["size"][0].get<float>();
                collider.size.y = data["size"][1].get<float>();
            }

            if (data.contains("offset") && data["offset"].is_array() && data["offset"].size() == 2) {
                collider.offset.x = data["offset"][0].get<float>();
                collider.offset.y = data["offset"][1].get<float>();
            }

            collider.isTrigger = data.value("isTrigger", false);

            reg.emplace<BoxCollider2D>(entity, collider);
        });

    registry.RegisterComponent(
        "CircleCollider2D", [](entt::registry& reg, entt::entity entity, const nlohmann::json& data) {
            CircleCollider2D collider{};

            if (data.contains("radius")) {
                collider.radius = data["radius"].get<float>();
            }

            if (data.contains("offset") && data["offset"].is_array() && data["offset"].size() == 2) {
                collider.offset.x = data["offset"][0].get<float>();
                collider.offset.y = data["offset"][1].get<float>();
            }

            collider.isTrigger = data.value("isTrigger", false);

            reg.emplace<CircleCollider2D>(entity, collider);
        });

    registry.RegisterComponent("Script", [](entt::registry& reg, entt::entity entity, const nlohmann::json& data) {
        Script script{};
        std::string scriptName = data.value("scriptName", "");
        if (!scriptName.empty()) {
            script.AddScript(scriptName);
        }
        reg.emplace<Script>(entity, script);
    });

    registry.RegisterComponent("Animator", [](entt::registry& reg, entt::entity entity, const nlohmann::json& data) {
        Animator animator{};

        if (data.contains("controllerUUID")) {
            animator.controllerUUID = UUID{data["controllerUUID"].get<uint64_t>()};
        }

        animator.isPlaying = data.value("isPlaying", true);
        animator.playbackSpeed = data.value("playbackSpeed", 1.0f);

        reg.emplace<Animator>(entity, animator);
    });
}

} // namespace PiiXeL
