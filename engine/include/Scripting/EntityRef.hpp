#ifndef PIIXELENGINE_ENTITYREF_HPP
#define PIIXELENGINE_ENTITYREF_HPP

#include <entt/entt.hpp>

namespace PiiXeL {

class EntityRef {
public:
    EntityRef() = default;
    explicit EntityRef(entt::entity entity) : m_Entity{entity} {}

    [[nodiscard]] entt::entity Get() const { return m_Entity; }
    void Set(entt::entity entity) { m_Entity = entity; }

    [[nodiscard]] bool IsValid() const { return m_Entity != entt::null; }

    operator entt::entity() const { return m_Entity; }

    bool operator==(const EntityRef& other) const { return m_Entity == other.m_Entity; }
    bool operator!=(const EntityRef& other) const { return m_Entity != other.m_Entity; }
    bool operator==(entt::entity entity) const { return m_Entity == entity; }
    bool operator!=(entt::entity entity) const { return m_Entity != entity; }

    entt::entity m_Entity{entt::null};
};

}

#endif
