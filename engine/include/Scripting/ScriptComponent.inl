#ifndef PIIXELENGINE_SCRIPTCOMPONENT_INL
#define PIIXELENGINE_SCRIPTCOMPONENT_INL

#include "Scene/Scene.hpp"

namespace PiiXeL {

template<typename T>
T* ScriptComponent::GetComponent() {
    if (m_Scene && m_Entity != entt::null) {
        entt::registry& registry = m_Scene->GetRegistry();
        if (registry.all_of<T>(m_Entity)) {
            return &registry.get<T>(m_Entity);
        }
    }
    return nullptr;
}

template<typename T>
T* ScriptComponent::AddComponent() {
    if (m_Scene && m_Entity != entt::null) {
        entt::registry& registry = m_Scene->GetRegistry();
        if (!registry.all_of<T>(m_Entity)) {
            return &registry.emplace<T>(m_Entity);
        }
        return &registry.get<T>(m_Entity);
    }
    return nullptr;
}

template<typename T>
void ScriptComponent::RemoveComponent() {
    if (m_Scene && m_Entity != entt::null) {
        entt::registry& registry = m_Scene->GetRegistry();
        if (registry.all_of<T>(m_Entity)) {
            registry.remove<T>(m_Entity);
        }
    }
}

} // namespace PiiXeL

#endif // PIIXELENGINE_SCRIPTCOMPONENT_INL
