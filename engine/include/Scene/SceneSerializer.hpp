#ifndef PIIXELENGINE_SCENESERIALIZER_HPP
#define PIIXELENGINE_SCENESERIALIZER_HPP

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace PiiXeL {

class Scene;

class SceneSerializer {
public:
    explicit SceneSerializer(Scene* scene);

    bool Serialize(const std::string& filepath);
    bool Deserialize(const std::string& filepath);

    std::string SerializeToString();
    bool DeserializeFromString(const std::string& data);

private:
    Scene* m_Scene;

    nlohmann::json SerializeEntity(entt::entity entity);
    entt::entity DeserializeEntity(const nlohmann::json& entityJson);
};

} // namespace PiiXeL

#endif // PIIXELENGINE_SCENESERIALIZER_HPP
