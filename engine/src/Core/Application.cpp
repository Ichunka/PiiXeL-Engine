#include "Core/Application.hpp"
#include "Core/Engine.hpp"
#include "Resources/AssetManager.hpp"
#include "Project/ProjectSettings.hpp"
#include "Debug/Profiler.hpp"
#include <raylib.h>

#ifdef BUILD_WITH_EDITOR
#include "Editor/EditorLayer.hpp"
#include <rlImGui.h>
#include <imgui.h>
#endif

namespace PiiXeL {

Application::Application(const ApplicationConfig& config)
    : m_Config{config}
    , m_Engine{nullptr}
#ifdef BUILD_WITH_EDITOR
    , m_EditorLayer{nullptr}
#endif
    , m_Running{false}
    , m_Initialized{false}
{
}

Application::~Application() {
    Cleanup();
}

void Application::Initialize() {
    if (m_Initialized) {
        return;
    }

    if (m_Config.resizable) {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    }
    if (m_Config.vsync) {
        SetConfigFlags(FLAG_VSYNC_HINT);
    }

    InitWindow(m_Config.windowWidth, m_Config.windowHeight, m_Config.title.c_str());
    SetTargetFPS(m_Config.targetFPS);

    if (m_Config.fullscreen) {
        ToggleFullscreen();
    }

#ifdef BUILD_WITH_EDITOR
    rlImGuiSetup(true);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

    m_Engine = std::make_unique<Engine>();
    m_Engine->Initialize();

#ifdef BUILD_WITH_EDITOR
    m_EditorLayer = std::make_unique<EditorLayer>(m_Engine.get());
#else
    ProjectSettings& settings = ProjectSettings::Instance();
    settings.ApplyToPhysics(m_Engine->GetPhysicsSystem());

    m_Engine->CreatePhysicsBodies();
    m_Engine->SetPhysicsEnabled(true);
    m_Engine->SetScriptsEnabled(true);
#endif

    m_Initialized = true;
    m_Running = true;
}

void Application::Run() {
    Initialize();

    float lastTime{static_cast<float>(GetTime())};

    while (m_Running && !WindowShouldClose()) {
        Profiler::Instance().BeginFrame();

        float currentTime{static_cast<float>(GetTime())};
        float deltaTime{currentTime - lastTime};
        lastTime = currentTime;

        Update(deltaTime);

        BeginDrawing();
        ClearBackground(Color{30, 30, 30, 255});

        Render();

        EndDrawing();

        Profiler::Instance().EndFrame();
    }

    Shutdown();
}

void Application::Update(float deltaTime) {
    if (IsKeyPressed(KEY_F11)) {
        ToggleFullscreen();
        if (IsWindowFullscreen()) {
            TraceLog(LOG_INFO, "Switched to FULLSCREEN mode (F11 to exit)");
        } else {
            TraceLog(LOG_INFO, "Switched to WINDOWED mode (F11 for fullscreen)");
        }
    }

    if (m_Engine) {
        m_Engine->Update(deltaTime);
    }

#ifdef BUILD_WITH_EDITOR
    if (m_EditorLayer) {
        m_EditorLayer->OnUpdate(deltaTime);
    }
#endif
}

void Application::Render() {
#ifdef BUILD_WITH_EDITOR
    rlImGuiBegin();

    if (m_EditorLayer) {
        m_EditorLayer->OnImGuiRender();
    }

    rlImGuiEnd();
#else
    if (m_Engine) {
        m_Engine->Render();
    }
#endif
}

void Application::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    m_Running = false;
    Cleanup();
}

void Application::Cleanup() {
    if (!m_Initialized) {
        return;
    }

#ifdef BUILD_WITH_EDITOR
    m_EditorLayer.reset();
#endif

    if (m_Engine) {
        m_Engine->Shutdown();
        m_Engine.reset();
    }

    AssetManager::Instance().Shutdown();

#ifdef BUILD_WITH_EDITOR
    rlImGuiShutdown();
    SetTraceLogCallback(nullptr);
#endif
    CloseWindow();

    m_Initialized = false;
}

} // namespace PiiXeL
