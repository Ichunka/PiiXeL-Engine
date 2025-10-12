#ifndef PIIXELENGINE_ENGINE_HPP
#define PIIXELENGINE_ENGINE_HPP

#include <entt/entt.hpp>
#include <memory>

namespace PiiXeL {

class Scene;
class RenderSystem;
class PhysicsSystem;
class ScriptSystem;
class GamePackageLoader;

class Engine {
public:
    Engine();
    ~Engine();

    // Non-copyable
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void Initialize();
    void Update(float deltaTime);
    void Render();
    void Shutdown();

    [[nodiscard]] entt::registry& GetRegistry() { return m_Registry; }
    [[nodiscard]] Scene* GetActiveScene() const { return m_ActiveScene.get(); }
    [[nodiscard]] PhysicsSystem* GetPhysicsSystem() const { return m_PhysicsSystem.get(); }
    [[nodiscard]] RenderSystem* GetRenderSystem() const { return m_RenderSystem.get(); }
    [[nodiscard]] ScriptSystem* GetScriptSystem() const { return m_ScriptSystem.get(); }

    void SetActiveScene(std::unique_ptr<Scene> scene);
    void SetPhysicsEnabled(bool enabled) { m_PhysicsEnabled = enabled; }
    void SetScriptsEnabled(bool enabled) { m_ScriptsEnabled = enabled; }
    void CreatePhysicsBodies();
    void DestroyAllPhysicsBodies();

    bool LoadSceneFromFile(const std::string& filepath);
    bool LoadFromPackage(const std::string& packagePath, const std::string& sceneName);

    void InvalidatePrimaryCameraCache() { m_PrimaryCameraCached = false; }

private:
    entt::entity FindPrimaryCamera();

    entt::registry m_Registry;
    std::unique_ptr<Scene> m_ActiveScene;
    std::unique_ptr<RenderSystem> m_RenderSystem;
    std::unique_ptr<PhysicsSystem> m_PhysicsSystem;
    std::unique_ptr<ScriptSystem> m_ScriptSystem;
    std::unique_ptr<GamePackageLoader> m_PackageLoader;
    bool m_PhysicsEnabled{false};
    bool m_ScriptsEnabled{true};

    entt::entity m_PrimaryCamera{entt::null};
    bool m_PrimaryCameraCached{false};
};

} // namespace PiiXeL

#endif // PIIXELENGINE_ENGINE_HPP
