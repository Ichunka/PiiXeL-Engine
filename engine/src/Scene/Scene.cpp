#include "Scene/Scene.hpp"

#include "Components/UUID.hpp"
#include "Scene/EntityFactory.hpp"
#include "Scene/EntityRegistry.hpp"

namespace PiiXeL {

Scene::Scene(const std::string& name) : m_Name{name}, m_Registry{} {}

void Scene::CreateDemoEntities() {}

Scene::~Scene() {
    m_Registry.clear();
}

void Scene::OnUpdate(float deltaTime) {
    (void)deltaTime;
}

void Scene::OnRender() {}

entt::entity Scene::CreateEntity(const std::string& name) {
    return EntityFactory::CreateEntity(this, name);
}

void Scene::DestroyEntity(entt::entity entity) {
    if (m_Registry.all_of<UUID>(entity)) {
        UUID uuid = m_Registry.get<UUID>(entity);
        EntityRegistry::Instance().UnregisterEntity(uuid);
    }

    auto it = std::find(m_EntityOrder.begin(), m_EntityOrder.end(), entity);
    if (it != m_EntityOrder.end()) {
        m_EntityOrder.erase(it);
    }

    m_Registry.destroy(entity);
}

} // namespace PiiXeL
