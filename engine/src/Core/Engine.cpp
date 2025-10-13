#include "Core/Engine.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneSerializer.hpp"
#include "Scene/ComponentRegistry.hpp"
#include "Build/GamePackageLoader.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Systems/RenderSystem.hpp"
#include "Systems/PhysicsSystem.hpp"
#include "Systems/ScriptSystem.hpp"
#include "Systems/AnimationSystem.hpp"
#include "Components/Transform.hpp"
#include "Components/Camera.hpp"
#include "Components/RigidBody2D.hpp"
#include "Components/BoxCollider2D.hpp"
#include "Components/Script.hpp"
#include "Debug/Profiler.hpp"
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
    RegisterAllComponents();

    m_RenderSystem = std::make_unique<RenderSystem>();
    m_PhysicsSystem = std::make_unique<PhysicsSystem>();
    m_PhysicsSystem->Initialize();

    m_ScriptSystem = std::make_unique<ScriptSystem>();

#ifdef BUILD_WITH_EDITOR
    m_ActiveScene = std::make_unique<Scene>("Default Scene");
#else
    m_PhysicsEnabled = true;
    m_ScriptsEnabled = true;
    m_AnimationEnabled = true;

    if (FileExists("datas/game.package")) {
        if (LoadFromPackage("datas/game.package", "Default_Scene")) {
            TraceLog(LOG_INFO, "✓ Game package loaded successfully - all assets embedded");

            if (m_ActiveScene) {
                AnimationSystem::ResetAnimators(m_ActiveScene->GetRegistry());
                CreatePhysicsBodies();
            }
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
    PROFILE_FUNCTION();

    {
        PROFILE_SCOPE("Scene::OnUpdate");
        if (m_ActiveScene) {
            m_ActiveScene->OnUpdate(deltaTime);
        }
    }

    {
        PROFILE_SCOPE("ScriptSystem::OnUpdate");
        if (m_ScriptsEnabled && m_ScriptSystem && m_ActiveScene) {
            m_ScriptSystem->OnUpdate(m_ActiveScene.get(), deltaTime);
        }
    }

    {
        PROFILE_SCOPE("AnimationSystem::Update");
        if (m_AnimationEnabled && m_ActiveScene) {
            AnimationSystem::Update(m_ActiveScene->GetRegistry(), deltaTime);
        }
    }

    {
        PROFILE_SCOPE("Physics");
        if (m_PhysicsEnabled && m_PhysicsSystem && m_ActiveScene) {
            m_PhysicsSystem->SetScene(m_ActiveScene.get());

            {
                PROFILE_SCOPE("PhysicsSystem::Update");
                m_PhysicsSystem->Update(deltaTime, m_ActiveScene->GetRegistry());
            }

            {
                PROFILE_SCOPE("PhysicsSystem::ProcessCollisionEvents");
                m_PhysicsSystem->ProcessCollisionEvents(m_ActiveScene->GetRegistry());
            }

            {
                PROFILE_SCOPE("ScriptSystem::OnFixedUpdate");
                if (m_ScriptsEnabled && m_ScriptSystem) {
                    m_ScriptSystem->OnFixedUpdate(m_ActiveScene.get(), deltaTime);
                }
            }
        }
    }
}

entt::entity Engine::FindPrimaryCamera() {
    if (!m_ActiveScene) {
        return entt::null;
    }

    if (m_PrimaryCameraCached) {
        entt::registry& registry = m_ActiveScene->GetRegistry();
        if (registry.valid(m_PrimaryCamera) && registry.all_of<Camera, Transform>(m_PrimaryCamera)) {
            const Camera& camera = registry.get<Camera>(m_PrimaryCamera);
            if (camera.isPrimary) {
                return m_PrimaryCamera;
            }
        }
        m_PrimaryCameraCached = false;
    }

    entt::registry& registry = m_ActiveScene->GetRegistry();
    m_PrimaryCamera = entt::null;

    auto view = registry.view<Camera, Transform>();
    for (auto entity : view) {
        const Camera& camera = view.get<Camera>(entity);
        if (camera.isPrimary) {
            m_PrimaryCamera = entity;
            m_PrimaryCameraCached = true;
            break;
        }
    }

    return m_PrimaryCamera;
}

void Engine::Render() {
    PROFILE_FUNCTION();

    {
        PROFILE_SCOPE("Scene::OnRender");
        if (m_ActiveScene) {
            m_ActiveScene->OnRender();
        }
    }

    {
        PROFILE_SCOPE("RenderSystem");
        if (m_RenderSystem && m_ActiveScene) {
#ifdef BUILD_WITH_EDITOR
            m_RenderSystem->Render(m_ActiveScene->GetRegistry());
#else
            entt::registry& registry = m_ActiveScene->GetRegistry();
            entt::entity primaryCamera = FindPrimaryCamera();

            if (primaryCamera != entt::null) {
                const Camera& cameraComp = registry.get<Camera>(primaryCamera);
                const Transform& transform = registry.get<Transform>(primaryCamera);

                Camera2D raylibCamera{};
                raylibCamera.target = transform.position;
                raylibCamera.offset = Vector2{static_cast<float>(GetScreenWidth()) / 2.0f, static_cast<float>(GetScreenHeight()) / 2.0f};
                raylibCamera.rotation = cameraComp.rotation;
                raylibCamera.zoom = cameraComp.zoom;

                m_RenderSystem->RenderWithCamera(registry, raylibCamera);
            } else {
                m_RenderSystem->Render(registry);
            }
#endif
        }
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

    m_PackageLoader->InitializeAssetRegistry();
    AssetRegistry::Instance().Initialize();
    AssetRegistry::Instance().RegisterExtractedAssets();

    m_ActiveScene = m_PackageLoader->LoadScene(sceneName, m_ScriptSystem.get());
    m_PrimaryCameraCached = false;

    if (m_ActiveScene) {
        entt::registry& registry = m_ActiveScene->GetRegistry();
        size_t entityCount = registry.storage<entt::entity>().size();
        size_t scriptCount = registry.view<Script>().size();
        size_t cameraCount = registry.view<Camera>().size();

        TraceLog(LOG_INFO, "Scene loaded: %zu entities, %zu scripts, %zu cameras", entityCount, scriptCount, cameraCount);

        return true;
    }

    return false;
}

} // namespace PiiXeL
