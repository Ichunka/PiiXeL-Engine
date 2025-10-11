#include "Scene/Scene.hpp"
#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Components/Sprite.hpp"
#include "Components/UUID.hpp"
#include "Scene/EntityRegistry.hpp"

namespace PiiXeL {

Scene::Scene(const std::string& name)
    : m_Name{name}
    , m_Registry{}
{
}

void Scene::CreateDemoEntities() {
}

Scene::~Scene() {
    m_Registry.clear();
}

void Scene::OnUpdate(float deltaTime) {
    (void)deltaTime;
}

void Scene::OnRender() {
}

entt::entity Scene::CreateEntity(const std::string& name) {
    entt::entity entity{m_Registry.create()};

    UUID uuid;
    m_Registry.emplace<UUID>(entity, uuid);
    EntityRegistry::Instance().RegisterEntity(uuid, entity);

    m_Registry.emplace<Tag>(entity, name);
    m_Registry.emplace<Transform>(entity);

    return entity;
}

void Scene::DestroyEntity(entt::entity entity) {
    if (m_Registry.all_of<UUID>(entity)) {
        UUID uuid = m_Registry.get<UUID>(entity);
        EntityRegistry::Instance().UnregisterEntity(uuid);
    }
    m_Registry.destroy(entity);
}

} // namespace PiiXeL
