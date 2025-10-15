#ifndef PIIXELENGINE_ENTITYREGISTRY_HPP
#define PIIXELENGINE_ENTITYREGISTRY_HPP

#include "Components/UUID.hpp"

#include <entt/entt.hpp>

#include <unordered_map>

namespace PiiXeL {

class EntityRegistry {
public:
    static EntityRegistry& Instance();

    void RegisterEntity(UUID uuid, entt::entity entity);
    void UnregisterEntity(UUID uuid);
    void Clear();

    [[nodiscard]] entt::entity GetEntity(UUID uuid) const;
    [[nodiscard]] UUID GetUUID(entt::entity entity) const;
    [[nodiscard]] bool HasEntity(UUID uuid) const;

private:
    EntityRegistry() = default;

    std::unordered_map<UUID, entt::entity> m_UUIDToEntity;
    std::unordered_map<uint32_t, UUID> m_EntityToUUID;
};

} // namespace PiiXeL

#endif
