#include "Scripting/ComponentRef.hpp"
#include "Scene/EntityRegistry.hpp"

namespace PiiXeL {

template<typename T>
bool ComponentRef<T>::HasComponent() const {
    if (!m_EntityRef.IsValid()) {
        return false;
    }

    entt::registry& registry = EntityRegistry::GetRegistry();
    entt::entity entity = m_EntityRef.Get();

    if (!registry.valid(entity)) {
        return false;
    }

    return registry.all_of<T>(entity);
}

template<typename T>
T* ComponentRef<T>::Get() {
    if (!HasComponent()) {
        return nullptr;
    }

    entt::registry& registry = EntityRegistry::GetRegistry();
    entt::entity entity = m_EntityRef.Get();

    return &registry.get<T>(entity);
}

template<typename T>
const T* ComponentRef<T>::Get() const {
    if (!HasComponent()) {
        return nullptr;
    }

    entt::registry& registry = EntityRegistry::GetRegistry();
    entt::entity entity = m_EntityRef.Get();

    return &registry.get<T>(entity);
}

} // namespace PiiXeL
