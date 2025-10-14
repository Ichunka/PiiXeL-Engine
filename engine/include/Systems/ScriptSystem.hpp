#ifndef PIIXELENGINE_SCRIPTSYSTEM_HPP
#define PIIXELENGINE_SCRIPTSYSTEM_HPP

#include <entt/entt.hpp>

#include <memory>
#include <string>

namespace PiiXeL {

class Scene;
class ScriptComponent;

class ScriptSystem {
public:
    ScriptSystem();
    ~ScriptSystem();

    void OnUpdate(Scene* scene, float deltaTime);
    void OnFixedUpdate(Scene* scene, float fixedDeltaTime);

    std::shared_ptr<ScriptComponent> CreateScript(const std::string& name);
};

} // namespace PiiXeL

#endif // PIIXELENGINE_SCRIPTSYSTEM_HPP
