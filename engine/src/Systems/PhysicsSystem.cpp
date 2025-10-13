#include "Systems/PhysicsSystem.hpp"
#include "Core/Logger.hpp"
#include "Scene/Scene.hpp"
#include "Components/Transform.hpp"
#include "Components/RigidBody2D.hpp"
#include "Components/BoxCollider2D.hpp"
#include "Components/CircleCollider2D.hpp"
#include "Components/Script.hpp"
#include "Scripting/ScriptComponent.hpp"
#include <iostream>

namespace PiiXeL {

PhysicsSystem::PhysicsSystem()
    : m_WorldId{b2_nullWorldId}
    , m_TimeAccumulator{0.0f}
{
}

PhysicsSystem::~PhysicsSystem() {
    Shutdown();
}

void PhysicsSystem::Initialize() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = b2Vec2{0.0f, 9.8f};
    m_WorldId = b2CreateWorld(&worldDef);
}

void PhysicsSystem::Shutdown() {
    if (B2_IS_NON_NULL(m_WorldId)) {
        b2DestroyWorld(m_WorldId);
        m_WorldId = b2_nullWorldId;
    }
}

void PhysicsSystem::Update(float deltaTime, entt::registry& registry) {
    if (B2_IS_NULL(m_WorldId)) {
        return;
    }

    m_TimeAccumulator += deltaTime;

    while (m_TimeAccumulator >= m_FixedTimeStep) {
        b2World_Step(m_WorldId, m_FixedTimeStep, m_SubStepCount);
        m_TimeAccumulator -= m_FixedTimeStep;
    }

    SyncTransforms(registry);
}

void PhysicsSystem::CreateBody(entt::registry& registry, entt::entity entity) {
    if (B2_IS_NULL(m_WorldId)) {
        return;
    }

    if (!registry.all_of<Transform, RigidBody2D>(entity)) {
        return;
    }

    Transform& transform = registry.get<Transform>(entity);
    RigidBody2D& rb = registry.get<RigidBody2D>(entity);

    if (B2_IS_NON_NULL(rb.box2dBodyId)) {
        b2DestroyBody(rb.box2dBodyId);
        rb.box2dBodyId = b2_nullBodyId;
    }

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.position = b2Vec2{transform.position.x / m_PixelsToMeters, transform.position.y / m_PixelsToMeters};
    bodyDef.rotation = b2MakeRot(transform.rotation * DEG2RAD);
    bodyDef.fixedRotation = rb.fixedRotation;
    bodyDef.enableSleep = true;
    bodyDef.sleepThreshold = 0.05f;
    bodyDef.isAwake = true;
    bodyDef.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(entity));

    switch (rb.type) {
        case BodyType::Static:
            bodyDef.type = b2_staticBody;
            break;
        case BodyType::Dynamic:
            bodyDef.type = b2_dynamicBody;
            bodyDef.linearDamping = 0.01f;
            bodyDef.angularDamping = 0.01f;
            break;
        case BodyType::Kinematic:
            bodyDef.type = b2_kinematicBody;
            break;
    }

    b2BodyId bodyId = b2CreateBody(m_WorldId, &bodyDef);
    rb.box2dBodyId = bodyId;

    if (registry.all_of<BoxCollider2D>(entity)) {
        BoxCollider2D& collider = registry.get<BoxCollider2D>(entity);

        float scaledWidth = collider.size.x * transform.scale.x;
        float scaledHeight = collider.size.y * transform.scale.y;

        float halfWidth = (scaledWidth * 0.5f) / m_PixelsToMeters;
        float halfHeight = (scaledHeight * 0.5f) / m_PixelsToMeters;
        b2Polygon box = b2MakeBox(halfWidth, halfHeight);

        b2ShapeDef shapeDef = b2DefaultShapeDef();

        float areaPixels = scaledWidth * scaledHeight;
        float areaMeters = areaPixels / (m_PixelsToMeters * m_PixelsToMeters);
        shapeDef.density = areaMeters > 0.0f ? (rb.mass / areaMeters) : 1.0f;

        shapeDef.material.friction = rb.friction;
        shapeDef.material.restitution = rb.restitution;
        shapeDef.isSensor = collider.isTrigger;
        shapeDef.enableSensorEvents = true;
        shapeDef.enableContactEvents = true;

        b2CreatePolygonShape(bodyId, &shapeDef, &box);
    }
    if (registry.all_of<CircleCollider2D>(entity))
    {
        CircleCollider2D& collider = registry.get<CircleCollider2D>(entity);

        const float scaledRadius = collider.radius * transform.scale.x;

        b2Circle circle{.center = {collider.offset.x / m_PixelsToMeters,
        collider.offset.y / m_PixelsToMeters}
            , .radius = scaledRadius / m_PixelsToMeters};

        b2ShapeDef shapeDef = b2DefaultShapeDef();

        float radiusMeters = scaledRadius / m_PixelsToMeters;
        float areaMeters = PI * radiusMeters * radiusMeters;
        shapeDef.density = areaMeters > 0.0f ? (rb.mass / areaMeters) : 1.0f;

        shapeDef.material.friction = rb.friction;
        shapeDef.material.restitution = rb.restitution;
        shapeDef.isSensor = collider.isTrigger;
        shapeDef.enableSensorEvents = true;
        shapeDef.enableContactEvents = true;

        b2CreateCircleShape(bodyId, &shapeDef, &circle);
    }
}

void PhysicsSystem::SyncTransforms(entt::registry& registry) {
    registry.view<Transform, RigidBody2D>().each([this](Transform& transform, RigidBody2D& rb) {
        if (B2_IS_NULL(rb.box2dBodyId)) {
            return;
        }

        b2Vec2 position = b2Body_GetPosition(rb.box2dBodyId);
        b2Rot rotation = b2Body_GetRotation(rb.box2dBodyId);
        float angle = b2Rot_GetAngle(rotation);

        transform.position.x = position.x * m_PixelsToMeters;
        transform.position.y = position.y * m_PixelsToMeters;
        transform.rotation = angle * RAD2DEG;

        b2Vec2 velocity = b2Body_GetLinearVelocity(rb.box2dBodyId);
        rb.velocity.x = velocity.x * m_PixelsToMeters;
        rb.velocity.y = velocity.y * m_PixelsToMeters;
    });
}

void PhysicsSystem::DestroyAllBodies(entt::registry& registry) {
    if (B2_IS_NULL(m_WorldId)) {
        return;
    }

    registry.view<RigidBody2D>().each([](RigidBody2D& rb) {
        rb.box2dBodyId = b2_nullBodyId;
    });

    b2DestroyWorld(m_WorldId);
    m_WorldId = b2_nullWorldId;

    Initialize();
}

void PhysicsSystem::SetGravity(const Vector2& gravity) {
    if (B2_IS_NON_NULL(m_WorldId)) {
        b2World_SetGravity(m_WorldId, b2Vec2{gravity.x, gravity.y});
        PX_LOG_INFO(PHYSICS, "Physics gravity set to: (%.2f, %.2f)", gravity.x, gravity.y);
    }
}

Vector2 PhysicsSystem::GetGravity() const {
    if (B2_IS_NON_NULL(m_WorldId)) {
        b2Vec2 gravity = b2World_GetGravity(m_WorldId);
        return Vector2{gravity.x, gravity.y};
    }
    return Vector2{0.0f, 0.0f};
}

void PhysicsSystem::ProcessCollisionEvents(entt::registry& registry) {
    if (B2_IS_NULL(m_WorldId) || !m_Scene) {
        return;
    }

    b2ContactEvents contactEvents = b2World_GetContactEvents(m_WorldId);

    for (int i = 0; i < contactEvents.beginCount; ++i) {
        b2ContactBeginTouchEvent* beginEvent = contactEvents.beginEvents + i;

        b2BodyId bodyIdA = b2Shape_GetBody(beginEvent->shapeIdA);
        b2BodyId bodyIdB = b2Shape_GetBody(beginEvent->shapeIdB);

        void* userDataA = b2Body_GetUserData(bodyIdA);
        void* userDataB = b2Body_GetUserData(bodyIdB);

        if (userDataA && userDataB) {
            entt::entity entityA = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(userDataA));
            entt::entity entityB = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(userDataB));

            if (registry.valid(entityA) && registry.valid(entityB)) {
                auto pair = std::minmax(entityA, entityB);
                auto [it, inserted] = m_ActiveCollisions.insert(pair);

                if (inserted) {
                    if (registry.all_of<Script>(entityA)) {
                        Script& scriptComponent = registry.get<Script>(entityA);
                        for (ScriptInstance& script : scriptComponent.scripts) {
                            if (script.instance) {
                                script.instance->OnCollisionEnter(entityB);
                            }
                        }
                    }

                    if (registry.all_of<Script>(entityB)) {
                        Script& scriptComponent = registry.get<Script>(entityB);
                        for (ScriptInstance& script : scriptComponent.scripts) {
                            if (script.instance) {
                                script.instance->OnCollisionEnter(entityA);
                            }
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < contactEvents.endCount; ++i) {
        b2ContactEndTouchEvent* endEvent = contactEvents.endEvents + i;

        b2BodyId bodyIdA = b2Shape_GetBody(endEvent->shapeIdA);
        b2BodyId bodyIdB = b2Shape_GetBody(endEvent->shapeIdB);

        void* userDataA = b2Body_GetUserData(bodyIdA);
        void* userDataB = b2Body_GetUserData(bodyIdB);

        if (userDataA && userDataB) {
            entt::entity entityA = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(userDataA));
            entt::entity entityB = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(userDataB));

            if (registry.valid(entityA) && registry.valid(entityB)) {
                auto pair = std::minmax(entityA, entityB);
                size_t erased = m_ActiveCollisions.erase(pair);

                if (erased > 0) {
                    if (registry.all_of<Script>(entityA)) {
                        Script& scriptComponent = registry.get<Script>(entityA);
                        for (ScriptInstance& script : scriptComponent.scripts) {
                            if (script.instance) {
                                script.instance->OnCollisionExit(entityB);
                            }
                        }
                    }

                    if (registry.all_of<Script>(entityB)) {
                        Script& scriptComponent = registry.get<Script>(entityB);
                        for (ScriptInstance& script : scriptComponent.scripts) {
                            if (script.instance) {
                                script.instance->OnCollisionExit(entityA);
                            }
                        }
                    }
                }
            }
        }
    }

    for (const auto& [entityA, entityB] : m_ActiveCollisions) {
        if (registry.valid(entityA) && registry.valid(entityB)) {
            if (registry.all_of<Script>(entityA)) {
                Script& scriptComponent = registry.get<Script>(entityA);
                for (ScriptInstance& script : scriptComponent.scripts) {
                    if (script.instance) {
                        script.instance->OnCollisionStay(entityB);
                    }
                }
            }

            if (registry.all_of<Script>(entityB)) {
                Script& scriptComponent = registry.get<Script>(entityB);
                for (ScriptInstance& script : scriptComponent.scripts) {
                    if (script.instance) {
                        script.instance->OnCollisionStay(entityA);
                    }
                }
            }
        }
    }

    b2SensorEvents sensorEvents = b2World_GetSensorEvents(m_WorldId);

    for (int i = 0; i < sensorEvents.beginCount; ++i) {
        b2SensorBeginTouchEvent* beginEvent = sensorEvents.beginEvents + i;

        b2BodyId visitorBodyId = b2Shape_GetBody(beginEvent->visitorShapeId);
        b2BodyId sensorBodyId = b2Shape_GetBody(beginEvent->sensorShapeId);

        void* visitorData = b2Body_GetUserData(visitorBodyId);
        void* sensorData = b2Body_GetUserData(sensorBodyId);

        if (visitorData && sensorData) {
            entt::entity visitorEntity = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(visitorData));
            entt::entity sensorEntity = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(sensorData));

            if (registry.valid(visitorEntity) && registry.valid(sensorEntity)) {
                auto pair = std::minmax(visitorEntity, sensorEntity);
                auto [it, inserted] = m_ActiveTriggers.insert(pair);

                if (inserted) {
                    if (registry.all_of<Script>(sensorEntity)) {
                        Script& scriptComponent = registry.get<Script>(sensorEntity);
                        for (ScriptInstance& script : scriptComponent.scripts) {
                            if (script.instance) {
                                script.instance->OnTriggerEnter(visitorEntity);
                            }
                        }
                    }

                    if (registry.all_of<Script>(visitorEntity)) {
                        Script& scriptComponent = registry.get<Script>(visitorEntity);
                        for (ScriptInstance& script : scriptComponent.scripts) {
                            if (script.instance) {
                                script.instance->OnTriggerEnter(sensorEntity);
                            }
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < sensorEvents.endCount; ++i) {
        b2SensorEndTouchEvent* endEvent = sensorEvents.endEvents + i;

        b2BodyId visitorBodyId = b2Shape_GetBody(endEvent->visitorShapeId);
        b2BodyId sensorBodyId = b2Shape_GetBody(endEvent->sensorShapeId);

        void* visitorData = b2Body_GetUserData(visitorBodyId);
        void* sensorData = b2Body_GetUserData(sensorBodyId);

        if (visitorData && sensorData) {
            entt::entity visitorEntity = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(visitorData));
            entt::entity sensorEntity = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(sensorData));

            if (registry.valid(visitorEntity) && registry.valid(sensorEntity)) {
                auto pair = std::minmax(visitorEntity, sensorEntity);
                size_t erased = m_ActiveTriggers.erase(pair);

                if (erased > 0) {
                    if (registry.all_of<Script>(sensorEntity)) {
                        Script& scriptComponent = registry.get<Script>(sensorEntity);
                        for (ScriptInstance& script : scriptComponent.scripts) {
                            if (script.instance) {
                                script.instance->OnTriggerExit(visitorEntity);
                            }
                        }
                    }

                    if (registry.all_of<Script>(visitorEntity)) {
                        Script& scriptComponent = registry.get<Script>(visitorEntity);
                        for (ScriptInstance& script : scriptComponent.scripts) {
                            if (script.instance) {
                                script.instance->OnTriggerExit(sensorEntity);
                            }
                        }
                    }
                }
            }
        }
    }

    for (const auto& [entityA, entityB] : m_ActiveTriggers) {
        if (registry.valid(entityA) && registry.valid(entityB)) {
            if (registry.all_of<Script>(entityA)) {
                Script& scriptComponent = registry.get<Script>(entityA);
                for (ScriptInstance& script : scriptComponent.scripts) {
                    if (script.instance) {
                        script.instance->OnTriggerStay(entityB);
                    }
                }
            }

            if (registry.all_of<Script>(entityB)) {
                Script& scriptComponent = registry.get<Script>(entityB);
                for (ScriptInstance& script : scriptComponent.scripts) {
                    if (script.instance) {
                        script.instance->OnTriggerStay(entityA);
                    }
                }
            }
        }
    }
}

} // namespace PiiXeL
