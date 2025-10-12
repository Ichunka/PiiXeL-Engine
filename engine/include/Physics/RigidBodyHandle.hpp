#ifndef PIIXELENGINE_RIGIDBODYHANDLE_HPP
#define PIIXELENGINE_RIGIDBODYHANDLE_HPP

#include <entt/entt.hpp>
#include <raylib.h>

namespace PiiXeL {

class Scene;
struct RigidBody2D;

class RigidBodyHandle {
public:
    RigidBodyHandle(Scene* scene, entt::entity entity, RigidBody2D* component);

    [[nodiscard]] bool IsValid() const;

    void SetVelocity(Vector2 velocity);
    [[nodiscard]] Vector2 GetVelocity() const;

    void AddForce(Vector2 force);
    void AddImpulse(Vector2 impulse);

    void SetKinematicTarget(Vector2 targetPosition);
    void MoveKinematic(Vector2 translation);

    void SetAngularVelocity(float velocity);
    [[nodiscard]] float GetAngularVelocity() const;

    [[nodiscard]] bool IsGrounded(float checkDistance = 5.0f) const;

private:
    Scene* m_Scene;
    entt::entity m_Entity;
    RigidBody2D* m_Component;
};

} // namespace PiiXeL

#endif
