#include "Scene/EntityRegistry.hpp"

namespace PiiXeL {

EntityRegistry& EntityRegistry::Instance() {
    static EntityRegistry instance;
    return instance;
}

void EntityRegistry::RegisterEntity(UUID uuid, entt::entity entity) {
    m_UUIDToEntity[uuid] = entity;
    m_EntityToUUID[static_cast<uint32_t>(entity)] = uuid;
}

void EntityRegistry::UnregisterEntity(UUID uuid) {
    auto it = m_UUIDToEntity.find(uuid);
    if (it != m_UUIDToEntity.end()) {
        m_EntityToUUID.erase(static_cast<uint32_t>(it->second));
        m_UUIDToEntity.erase(it);
    }
}

void EntityRegistry::Clear() {
    m_UUIDToEntity.clear();
    m_EntityToUUID.clear();
}

entt::entity EntityRegistry::GetEntity(UUID uuid) const {
    auto it = m_UUIDToEntity.find(uuid);
    return (it != m_UUIDToEntity.end()) ? it->second : entt::null;
}

UUID EntityRegistry::GetUUID(entt::entity entity) const {
    auto it = m_EntityToUUID.find(static_cast<uint32_t>(entity));
    return (it != m_EntityToUUID.end()) ? it->second : UUID(0);
}

bool EntityRegistry::HasEntity(UUID uuid) const {
    return m_UUIDToEntity.find(uuid) != m_UUIDToEntity.end();
}

}
