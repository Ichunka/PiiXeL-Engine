#ifndef PIIXELENGINE_SCENE_HPP
#define PIIXELENGINE_SCENE_HPP

#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace PiiXeL {

class Scene {
public:
    explicit Scene(const std::string& name = "Untitled Scene");
    ~Scene();

    void OnUpdate(float deltaTime);
    void OnRender();

    [[nodiscard]] entt::entity CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(entt::entity entity);

    [[nodiscard]] const std::string& GetName() const { return m_Name; }
    void SetName(const std::string& name) { m_Name = name; }

    [[nodiscard]] entt::registry& GetRegistry() { return m_Registry; }
    [[nodiscard]] const std::vector<entt::entity>& GetEntityOrder() const { return m_EntityOrder; }
    [[nodiscard]] std::vector<entt::entity>& GetEntityOrder() { return m_EntityOrder; }

private:
    void CreateDemoEntities();

private:
    std::string m_Name;
    entt::registry m_Registry;
    std::vector<entt::entity> m_EntityOrder;
};

} // namespace PiiXeL

#endif // PIIXELENGINE_SCENE_HPP
