#include "Physics/RigidBodyHandle.hpp"
#include "Physics/Physics2D.hpp"
#include "Scene/Scene.hpp"
#include "Components/RigidBody2D.hpp"

namespace PiiXeL {

RigidBodyHandle::RigidBodyHandle(Scene* scene, entt::entity entity, RigidBody2D* component)
    : m_Scene{scene}
    , m_Entity{entity}
    , m_Component{component}
{
}

bool RigidBodyHandle::IsValid() const {
    return m_Scene && m_Component && m_Entity != entt::null;
}

void RigidBodyHandle::SetVelocity(Vector2 velocity) {
    Physics2D::SetVelocity(m_Scene, m_Entity, velocity);
}

Vector2 RigidBodyHandle::GetVelocity() const {
    return Physics2D::GetVelocity(m_Scene, m_Entity);
}

void RigidBodyHandle::AddForce(Vector2 force) {
    Physics2D::AddForce(m_Scene, m_Entity, force);
}

void RigidBodyHandle::AddImpulse(Vector2 impulse) {
    Physics2D::AddImpulse(m_Scene, m_Entity, impulse);
}

void RigidBodyHandle::SetKinematicTarget(Vector2 targetPosition) {
    Physics2D::SetKinematicTarget(m_Scene, m_Entity, targetPosition);
}

void RigidBodyHandle::MoveKinematic(Vector2 translation) {
    Physics2D::MoveKinematic(m_Scene, m_Entity, translation);
}

void RigidBodyHandle::SetAngularVelocity(float velocity) {
    Physics2D::SetAngularVelocity(m_Scene, m_Entity, velocity);
}

float RigidBodyHandle::GetAngularVelocity() const {
    return Physics2D::GetAngularVelocity(m_Scene, m_Entity);
}

bool RigidBodyHandle::IsGrounded(float checkDistance) const {
    return Physics2D::IsGrounded(m_Scene, m_Entity, checkDistance);
}

} // namespace PiiXeL
