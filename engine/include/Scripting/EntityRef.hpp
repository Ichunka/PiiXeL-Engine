#ifndef PIIXELENGINE_ENTITYREF_HPP
#define PIIXELENGINE_ENTITYREF_HPP

#include "Components/UUID.hpp"

#include <entt/entt.hpp>

namespace PiiXeL {

class EntityRef {
public:
    EntityRef() = default;
    explicit EntityRef(UUID uuid) : m_UUID{uuid} {}
    explicit EntityRef(entt::entity entity);

    [[nodiscard]] entt::entity Get() const;
    void Set(entt::entity entity);
    void SetUUID(UUID uuid) { m_UUID = uuid; }
    [[nodiscard]] UUID GetUUID() const { return m_UUID; }

    [[nodiscard]] bool IsValid() const;

    operator entt::entity() const { return Get(); }

    bool operator==(const EntityRef& other) const { return m_UUID == other.m_UUID; }
    bool operator!=(const EntityRef& other) const { return m_UUID != other.m_UUID; }

    UUID m_UUID{0};
};

} // namespace PiiXeL

#endif
