#ifndef PIIXELENGINE_SCRIPTCOMPONENTHANDLES_INL
#define PIIXELENGINE_SCRIPTCOMPONENTHANDLES_INL

#include "ScriptComponent.hpp"
#include "Scene/Scene.hpp"
#include "Components/RigidBody2D.hpp"
#include "Physics/RigidBodyHandle.hpp"

namespace PiiXeL {

template<typename Component>
std::optional<typename ComponentHandle<Component>::Type> ScriptComponent::GetHandle() {
    static_assert(sizeof(Component) == 0, "No handle implementation for this component type");
    return std::nullopt;
}

template<>
inline std::optional<RigidBodyHandle> ScriptComponent::GetHandle<RigidBody2D>() {
    if (!m_Scene || m_Entity == entt::null) {
        return std::nullopt;
    }

    entt::registry& registry = m_Scene->GetRegistry();
    if (!registry.all_of<RigidBody2D>(m_Entity)) {
        return std::nullopt;
    }

    RigidBody2D* rb = &registry.get<RigidBody2D>(m_Entity);
    return RigidBodyHandle{m_Scene, m_Entity, rb};
}

} // namespace PiiXeL

#endif
