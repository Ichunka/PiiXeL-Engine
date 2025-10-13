#include "Physics/Physics2D.hpp"
#include "Scene/Scene.hpp"
#include "Components/RigidBody2D.hpp"
#include "Components/Transform.hpp"
#include "Components/BoxCollider2D.hpp"
#include "Components/CircleCollider2D.hpp"
#include "Debug/DebugDraw.hpp"
#include <box2d/box2d.h>
#include <raylib.h>

namespace PiiXeL {

constexpr float PIXELS_TO_METERS = 100.0f;

void Physics2D::SetVelocity(Scene* scene, entt::entity entity, Vector2 velocity) {
    if (!scene) return;

    entt::registry& registry = scene->GetRegistry();
    if (!registry.all_of<RigidBody2D>(entity)) return;

    RigidBody2D& rb = registry.get<RigidBody2D>(entity);
    if (B2_IS_NULL(rb.box2dBodyId)) return;

    b2Vec2 vel{velocity.x / PIXELS_TO_METERS, velocity.y / PIXELS_TO_METERS};
    b2Body_SetLinearVelocity(rb.box2dBodyId, vel);
    rb.velocity = velocity;
}

Vector2 Physics2D::GetVelocity(Scene* scene, entt::entity entity) {
    if (!scene) return Vector2{0.0f, 0.0f};

    entt::registry& registry = scene->GetRegistry();
    if (!registry.all_of<RigidBody2D>(entity)) return Vector2{0.0f, 0.0f};

    RigidBody2D& rb = registry.get<RigidBody2D>(entity);
    if (B2_IS_NULL(rb.box2dBodyId)) return Vector2{0.0f, 0.0f};

    b2Vec2 vel = b2Body_GetLinearVelocity(rb.box2dBodyId);
    return Vector2{vel.x * PIXELS_TO_METERS, vel.y * PIXELS_TO_METERS};
}

void Physics2D::AddForce(Scene* scene, entt::entity entity, Vector2 force) {
    if (!scene) return;

    entt::registry& registry = scene->GetRegistry();
    if (!registry.all_of<RigidBody2D>(entity)) return;

    RigidBody2D& rb = registry.get<RigidBody2D>(entity);
    if (B2_IS_NULL(rb.box2dBodyId)) return;

    float mass = b2Body_GetMass(rb.box2dBodyId);
    b2Vec2 forceVec{
        (force.x / PIXELS_TO_METERS) * mass,
        (force.y / PIXELS_TO_METERS) * mass
    };
    b2Vec2 center = b2Body_GetWorldCenterOfMass(rb.box2dBodyId);
    b2Body_ApplyForce(rb.box2dBodyId, forceVec, center, true);
}

void Physics2D::AddImpulse(Scene* scene, entt::entity entity, Vector2 impulse) {
    if (!scene) return;

    entt::registry& registry = scene->GetRegistry();
    if (!registry.all_of<RigidBody2D>(entity)) return;

    RigidBody2D& rb = registry.get<RigidBody2D>(entity);
    if (B2_IS_NULL(rb.box2dBodyId)) return;

    float mass = b2Body_GetMass(rb.box2dBodyId);
    b2Vec2 impulseVec{
        (impulse.x / PIXELS_TO_METERS) * mass,
        (impulse.y / PIXELS_TO_METERS) * mass
    };
    b2Vec2 center = b2Body_GetWorldCenterOfMass(rb.box2dBodyId);
    b2Body_ApplyLinearImpulse(rb.box2dBodyId, impulseVec, center, true);
}

void Physics2D::SetKinematicTarget(Scene* scene, entt::entity entity, Vector2 targetPosition) {
    if (!scene) return;

    entt::registry& registry = scene->GetRegistry();
    if (!registry.all_of<RigidBody2D, Transform>(entity)) return;

    RigidBody2D& rb = registry.get<RigidBody2D>(entity);
    Transform& transform = registry.get<Transform>(entity);

    if (B2_IS_NULL(rb.box2dBodyId) || rb.type != BodyType::Kinematic) return;

    b2Vec2 position{targetPosition.x / PIXELS_TO_METERS, targetPosition.y / PIXELS_TO_METERS};
    b2Rot rotation = b2MakeRot(transform.rotation * DEG2RAD);
    b2Body_SetTransform(rb.box2dBodyId, position, rotation);

    transform.position = targetPosition;
}

void Physics2D::MoveKinematic(Scene* scene, entt::entity entity, Vector2 translation) {
    if (!scene) return;

    entt::registry& registry = scene->GetRegistry();
    if (!registry.all_of<Transform>(entity)) return;

    Transform& transform = registry.get<Transform>(entity);
    Vector2 newPosition{transform.position.x + translation.x, transform.position.y + translation.y};

    SetKinematicTarget(scene, entity, newPosition);
}

void Physics2D::SetAngularVelocity(Scene* scene, entt::entity entity, float velocity) {
    if (!scene) return;

    entt::registry& registry = scene->GetRegistry();
    if (!registry.all_of<RigidBody2D>(entity)) return;

    RigidBody2D& rb = registry.get<RigidBody2D>(entity);
    if (B2_IS_NULL(rb.box2dBodyId)) return;

    float velocityRadians = velocity * DEG2RAD;
    b2Body_SetAngularVelocity(rb.box2dBodyId, velocityRadians);
    rb.angularVelocity = velocity;
}

float Physics2D::GetAngularVelocity(Scene* scene, entt::entity entity) {
    if (!scene) return 0.0f;

    entt::registry& registry = scene->GetRegistry();
    if (!registry.all_of<RigidBody2D>(entity)) return 0.0f;

    RigidBody2D& rb = registry.get<RigidBody2D>(entity);
    if (B2_IS_NULL(rb.box2dBodyId)) return 0.0f;

    float velocityRadians = b2Body_GetAngularVelocity(rb.box2dBodyId);
    return velocityRadians * RAD2DEG;
}

bool Physics2D::IsGrounded(Scene* scene, entt::entity entity, float checkDistance) {
    if (!scene) return false;

    entt::registry& registry = scene->GetRegistry();
    if (!registry.all_of<Transform, RigidBody2D>(entity)) return false;

    RigidBody2D& rb = registry.get<RigidBody2D>(entity);
    if (B2_IS_NULL(rb.box2dBodyId)) return false;

    Transform& transform = registry.get<Transform>(entity);

    float colliderHalfHeight = 0.0f;
    if (registry.all_of<BoxCollider2D>(entity)) {
        BoxCollider2D& collider = registry.get<BoxCollider2D>(entity);
        colliderHalfHeight = (collider.size.y * transform.scale.y * 0.5f) / PIXELS_TO_METERS;
    }
    if (registry.all_of<CircleCollider2D>(entity)) {
        CircleCollider2D& collider = registry.get<CircleCollider2D>(entity);
        colliderHalfHeight = (collider.radius * (transform.scale.x + transform.scale.y) * 0.5f) / PIXELS_TO_METERS;
    }

    b2Vec2 position = b2Body_GetPosition(rb.box2dBodyId);
    float startOffset = 0.001f;
    b2Vec2 startPoint{position.x, position.y + colliderHalfHeight - startOffset};
    b2Vec2 endPoint{position.x, position.y + colliderHalfHeight + (checkDistance / PIXELS_TO_METERS)};

    b2WorldId worldId = b2Body_GetWorld(rb.box2dBodyId);
    if (B2_IS_NULL(worldId)) return false;

    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.maskBits = 0xFFFF;

    b2RayResult result = b2World_CastRayClosest(worldId, startPoint, endPoint, filter);

    Vector2 startPixels{startPoint.x * PIXELS_TO_METERS, startPoint.y * PIXELS_TO_METERS};
    Vector2 endPixels{endPoint.x * PIXELS_TO_METERS, endPoint.y * PIXELS_TO_METERS};

    bool hitValid = false;
    Vector2 hitPixels{0.0f, 0.0f};

    if (result.hit) {
        b2BodyId hitBodyId = b2Shape_GetBody(result.shapeId);
        if (!B2_ID_EQUALS(hitBodyId, rb.box2dBodyId)) {
            hitValid = true;
            hitPixels = Vector2{result.point.x * PIXELS_TO_METERS, result.point.y * PIXELS_TO_METERS};
        }
    }

    DebugDraw::Instance().DrawRay(startPixels, endPixels, hitValid ? GREEN : YELLOW, hitValid, hitPixels);

    return hitValid;
}

} // namespace PiiXeL
