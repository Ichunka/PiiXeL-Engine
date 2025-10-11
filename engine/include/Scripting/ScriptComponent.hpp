#ifndef PIIXELENGINE_SCRIPTCOMPONENT_HPP
#define PIIXELENGINE_SCRIPTCOMPONENT_HPP

#include <entt/entt.hpp>
#include <raylib.h>
#include <optional>
#include "Physics/ComponentHandles.hpp"

namespace PiiXeL {

class Scene;

class ScriptComponent {
public:
    ScriptComponent() = default;
    virtual ~ScriptComponent() = default;

    void Initialize(entt::entity entity, Scene* scene) {
        m_Entity = entity;
        m_Scene = scene;
        m_Initialized = true;
        OnAwake();
    }

    void ExecuteUpdate(float deltaTime) {
        if (m_Initialized && m_Enabled) {
            OnUpdate(deltaTime);
        }
    }

    void ExecuteFixedUpdate(float fixedDeltaTime) {
        if (m_Initialized && m_Enabled) {
            OnFixedUpdate(fixedDeltaTime);
        }
    }

    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    [[nodiscard]] bool IsEnabled() const { return m_Enabled; }
    [[nodiscard]] entt::entity GetEntity() const { return m_Entity; }
    [[nodiscard]] Scene* GetScene() const { return m_Scene; }

    template<typename T>
    T* GetComponent();

    template<typename T>
    T* AddComponent();

    template<typename T>
    void RemoveComponent();

    Vector2 GetPosition();
    void SetPosition(Vector2 position);
    void Translate(Vector2 offset);

    template<typename Component>
    [[nodiscard]] std::optional<typename ComponentHandle<Component>::Type> GetHandle();

    virtual void OnCollisionEnter(entt::entity other) { (void)other; }
    virtual void OnCollisionStay(entt::entity other) { (void)other; }
    virtual void OnCollisionExit(entt::entity other) { (void)other; }
    virtual void OnTriggerEnter(entt::entity other) { (void)other; }
    virtual void OnTriggerStay(entt::entity other) { (void)other; }
    virtual void OnTriggerExit(entt::entity other) { (void)other; }

    entt::entity m_Entity{entt::null};
    Scene* m_Scene{nullptr};
    bool m_Enabled{true};

protected:
    virtual void OnAwake() {}
    virtual void OnUpdate(float deltaTime) { (void)deltaTime; }
    virtual void OnFixedUpdate(float fixedDeltaTime) { (void)fixedDeltaTime; }
    virtual void OnDestroy() {}

private:
    bool m_Initialized{false};
};

} // namespace PiiXeL

#include "ScriptComponent.inl"

#endif // PIIXELENGINE_SCRIPTCOMPONENT_HPP
