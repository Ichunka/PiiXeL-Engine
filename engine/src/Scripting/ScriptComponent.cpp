#include "Scripting/ScriptComponent.hpp"

#include "Components/Transform.hpp"
#include "Scene/Scene.hpp"

namespace PiiXeL {

Vector2 ScriptComponent::GetPosition() {
    if (m_Scene && m_Entity != entt::null)
    {
        entt::registry& registry = m_Scene->GetRegistry();
        if (registry.all_of<Transform>(m_Entity))
        { return registry.get<Transform>(m_Entity).position; }
    }
    return {0.0f, 0.0f};
}

void ScriptComponent::SetPosition(Vector2 position) {
    if (m_Scene && m_Entity != entt::null)
    {
        entt::registry& registry = m_Scene->GetRegistry();
        if (registry.all_of<Transform>(m_Entity))
        { registry.get<Transform>(m_Entity).position = position; }
    }
}

void ScriptComponent::Translate(Vector2 offset) {
    if (m_Scene && m_Entity != entt::null)
    {
        entt::registry& registry = m_Scene->GetRegistry();
        if (registry.all_of<Transform>(m_Entity))
        {
            Transform& transform = registry.get<Transform>(m_Entity);
            transform.position.x += offset.x;
            transform.position.y += offset.y;
        }
    }
}

} // namespace PiiXeL
