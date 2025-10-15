#include "Scripting/EntityRef.hpp"

#include "Scene/EntityRegistry.hpp"

namespace PiiXeL {

EntityRef::EntityRef(entt::entity entity) {
    m_UUID = EntityRegistry::Instance().GetUUID(entity);
}

entt::entity EntityRef::Get() const {
    if (m_UUID.Get() == 0) {
        return entt::null;
    }
    return EntityRegistry::Instance().GetEntity(m_UUID);
}

void EntityRef::Set(entt::entity entity) {
    m_UUID = EntityRegistry::Instance().GetUUID(entity);
}

bool EntityRef::IsValid() const {
    if (m_UUID.Get() == 0) {
        return false;
    }
    entt::entity entity = Get();
    return entity != entt::null && EntityRegistry::Instance().HasEntity(m_UUID);
}

} // namespace PiiXeL
