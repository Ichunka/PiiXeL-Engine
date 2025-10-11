#include "Core/Engine.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneSerializer.hpp"
#include "Build/GamePackageLoader.hpp"
#include "Systems/RenderSystem.hpp"
#include "Systems/PhysicsSystem.hpp"
#include "Systems/ScriptSystem.hpp"
#include "Components/Transform.hpp"
#include "Components/Camera.hpp"
#include "Components/RigidBody2D.hpp"
#include "Components/BoxCollider2D.hpp"
#include <raylib.h>

namespace PiiXeL {

Engine::Engine()
    : m_Registry{}
    , m_ActiveScene{nullptr}
    , m_RenderSystem{nullptr}
    , m_PhysicsSystem{nullptr}
    , m_ScriptSystem{nullptr}
{
}

Engine::~Engine() {
    Shutdown();
}

void Engine::Initialize() {
    m_RenderSystem = std::make_unique<RenderSystem>();
    m_PhysicsSystem = std::make_unique<PhysicsSystem>();
    m_PhysicsSystem->Initialize();

    m_ScriptSystem = std::make_unique<ScriptSystem>();

#ifdef BUILD_WITH_EDITOR
    m_ActiveScene = std::make_unique<Scene>("Default Scene");
#else
    if (FileExists("datas/game.package")) {
        if (LoadFromPackage("datas/game.package", "Default_Scene")) {
            TraceLog(LOG_INFO, "✓ Game package loaded successfully - all assets embedded");
        } else {
            TraceLog(LOG_ERROR, "✗ Failed to load datas/game.package");
            m_ActiveScene = std::make_unique<Scene>("Empty Scene");
        }
    } else {
        TraceLog(LOG_ERROR, "✗ datas/game.package not found! Cannot run without package.");
        TraceLog(LOG_ERROR, "   Build the package first using: build_package.bat");
        m_ActiveScene = std::make_unique<Scene>("Empty Scene");
    }
#endif
}

void Engine::Update(float deltaTime) {
    if (m_ActiveScene) {
        m_ActiveScene->OnUpdate(deltaTime);
    }

    if (m_ScriptsEnabled && m_ScriptSystem && m_ActiveScene) {
        m_ScriptSystem->OnUpdate(m_ActiveScene.get(), deltaTime);
    }

    if (m_PhysicsEnabled && m_PhysicsSystem && m_ActiveScene) {
        m_PhysicsSystem->SetScene(m_ActiveScene.get());
        m_PhysicsSystem->Update(deltaTime, m_ActiveScene->GetRegistry());
        m_PhysicsSystem->ProcessCollisionEvents(m_ActiveScene->GetRegistry());
        if (m_ScriptsEnabled && m_ScriptSystem) {
            m_ScriptSystem->OnFixedUpdate(m_ActiveScene.get(), deltaTime);
        }
    }
}

void Engine::Render() {
    if (m_ActiveScene) {
        m_ActiveScene->OnRender();
    }

    if (m_RenderSystem && m_ActiveScene) {
#ifdef BUILD_WITH_EDITOR
        m_RenderSystem->Render(m_ActiveScene->GetRegistry());
#else
        entt::registry& registry = m_ActiveScene->GetRegistry();
        entt::entity primaryCamera = entt::null;

        registry.view<Camera, Transform>().each([&](entt::entity entity, const Camera& camera, const Transform&) {
            if (camera.isPrimary) {
                primaryCamera = entity;
            }
        });

        if (primaryCamera != entt::null && registry.all_of<Camera, Transform>(primaryCamera)) {
            const Camera& cameraComp = registry.get<Camera>(primaryCamera);
            const Transform& transform = registry.get<Transform>(primaryCamera);

            Camera2D raylibCamera = cameraComp.ToRaylib(transform.position);
            int screenWidth = GetScreenWidth();
            int screenHeight = GetScreenHeight();
            raylibCamera.offset = Vector2{static_cast<float>(screenWidth) / 2.0f, static_cast<float>(screenHeight) / 2.0f};

            m_RenderSystem->RenderWithCamera(registry, raylibCamera);
        } else {
            m_RenderSystem->Render(registry);
        }
#endif
    }
}

void Engine::Shutdown() {
    m_ActiveScene.reset();

    if (m_PhysicsSystem) {
        m_PhysicsSystem->Shutdown();
    }

    m_PhysicsSystem.reset();
    m_ScriptSystem.reset();
    m_RenderSystem.reset();
    m_PackageLoader.reset();
}

void Engine::SetActiveScene(std::unique_ptr<Scene> scene) {
    m_ActiveScene = std::move(scene);
}

void Engine::CreatePhysicsBodies() {
    if (!m_ActiveScene || !m_PhysicsSystem) {
        return;
    }

    entt::registry& registry = m_ActiveScene->GetRegistry();

    registry.view<Transform, RigidBody2D>().each([this, &registry](entt::entity entity, const Transform&, const RigidBody2D&) {
        m_PhysicsSystem->CreateBody(registry, entity);
    });
}

void Engine::DestroyAllPhysicsBodies() {
    if (!m_ActiveScene || !m_PhysicsSystem) {
        return;
    }

    entt::registry& registry = m_ActiveScene->GetRegistry();
    m_PhysicsSystem->DestroyAllBodies(registry);
}

bool Engine::LoadSceneFromFile(const std::string& filepath) {
    if (!m_ActiveScene) {
        return false;
    }

    SceneSerializer serializer{m_ActiveScene.get()};
    return serializer.Deserialize(filepath);
}

bool Engine::LoadFromPackage(const std::string& packagePath, const std::string& sceneName) {
    m_PackageLoader = std::make_unique<GamePackageLoader>();

    if (!m_PackageLoader->LoadPackage(packagePath)) {
        return false;
    }

    m_ActiveScene = m_PackageLoader->LoadScene(sceneName);
    return m_ActiveScene != nullptr;
}

} // namespace PiiXeL
