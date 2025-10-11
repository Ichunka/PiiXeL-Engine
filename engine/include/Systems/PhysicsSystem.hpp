#ifndef PIIXELENGINE_PHYSICSSYSTEM_HPP
#define PIIXELENGINE_PHYSICSSYSTEM_HPP

#include <entt/entt.hpp>
#include <box2d/box2d.h>
#include <raylib.h>
#include <functional>
#include <set>
#include <utility>

namespace PiiXeL {

class Scene;

class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void Update(float deltaTime, entt::registry& registry);
    void Initialize();
    void Shutdown();

    [[nodiscard]] b2WorldId GetWorldId() const { return m_WorldId; }

    void CreateBody(entt::registry& registry, entt::entity entity);
    void DestroyAllBodies(entt::registry& registry);

    void SetGravity(const Vector2& gravity);
    [[nodiscard]] Vector2 GetGravity() const;

    void SetScene(Scene* scene) { m_Scene = scene; }
    void ProcessCollisionEvents(entt::registry& registry);

private:
    void SyncTransforms(entt::registry& registry);

private:
    b2WorldId m_WorldId;
    Scene* m_Scene{nullptr};
    float m_TimeAccumulator{0.0f};
    const float m_FixedTimeStep{1.0f / 60.0f};
    const int m_SubStepCount{8};
    const float m_PixelsToMeters{100.0f};

    std::set<std::pair<entt::entity, entt::entity>> m_ActiveCollisions;
    std::set<std::pair<entt::entity, entt::entity>> m_ActiveTriggers;
};

} // namespace PiiXeL

#endif // PIIXELENGINE_PHYSICSSYSTEM_HPP
