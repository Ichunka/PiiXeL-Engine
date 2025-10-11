#include "Scene/Scene.hpp"
#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Components/Sprite.hpp"

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

    m_Registry.emplace<Tag>(entity, name);
    m_Registry.emplace<Transform>(entity);

    return entity;
}

void Scene::DestroyEntity(entt::entity entity) {
    m_Registry.destroy(entity);
}

} // namespace PiiXeL
