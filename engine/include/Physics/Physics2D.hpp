#ifndef PIIXELENGINE_PHYSICS2D_HPP
#define PIIXELENGINE_PHYSICS2D_HPP

#include <entt/entt.hpp>

#include <raylib.h>

namespace PiiXeL {

class Scene;

class Physics2D {
public:
    static void SetVelocity(Scene* scene, entt::entity entity, Vector2 velocity);
    static Vector2 GetVelocity(Scene* scene, entt::entity entity);

    static void AddForce(Scene* scene, entt::entity entity, Vector2 force);
    static void AddImpulse(Scene* scene, entt::entity entity, Vector2 impulse);

    static void SetKinematicTarget(Scene* scene, entt::entity entity, Vector2 targetPosition);
    static void MoveKinematic(Scene* scene, entt::entity entity, Vector2 translation);

    static void SetAngularVelocity(Scene* scene, entt::entity entity, float velocity);
    static float GetAngularVelocity(Scene* scene, entt::entity entity);

    static bool IsGrounded(Scene* scene, entt::entity entity, float checkDistance = 5.0f);

private:
    Physics2D() = delete;
};

} // namespace PiiXeL

#endif
