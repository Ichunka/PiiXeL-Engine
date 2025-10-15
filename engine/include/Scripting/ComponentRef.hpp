#ifndef PIIXELENGINE_COMPONENTREF_HPP
#define PIIXELENGINE_COMPONENTREF_HPP

#include "Components/UUID.hpp"
#include "Scripting/EntityRef.hpp"

#include <entt/entt.hpp>

#include <optional>

namespace PiiXeL {

template <typename T>
class ComponentRef {
public:
    ComponentRef() = default;
    explicit ComponentRef(UUID entityUUID) : m_EntityRef{entityUUID} {}
    explicit ComponentRef(entt::entity entity) : m_EntityRef{entity} {}

    void SetEntity(entt::entity entity) { m_EntityRef.Set(entity); }

    void SetEntityUUID(UUID uuid) { m_EntityRef.SetUUID(uuid); }

    [[nodiscard]] UUID GetEntityUUID() const { return m_EntityRef.GetUUID(); }

    [[nodiscard]] entt::entity GetEntity() const { return m_EntityRef.Get(); }

    [[nodiscard]] bool IsValid() const { return m_EntityRef.IsValid(); }

    [[nodiscard]] bool HasComponent() const;

    [[nodiscard]] T* Get();
    [[nodiscard]] const T* Get() const;

    T* operator->() { return Get(); }
    const T* operator->() const { return Get(); }

    explicit operator bool() const { return IsValid() && HasComponent(); }

private:
    EntityRef m_EntityRef;
};

} // namespace PiiXeL

#endif
