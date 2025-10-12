#include "Core/Application.hpp"
#include "Core/Engine.hpp"
#include "Core/SplashScreen.hpp"
#include "Resources/AssetManager.hpp"
#include "Resources/PathManager.hpp"
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

    if (!m_Config.iconPath.empty() && FileExists(m_Config.iconPath.c_str())) {
        Image iconImage = LoadImage(m_Config.iconPath.c_str());
        SetWindowIcon(iconImage);
        UnloadImage(iconImage);
        TraceLog(LOG_INFO, "Window icon set: %s", m_Config.iconPath.c_str());
    }

    PathManager::Instance().Initialize();

#ifndef BUILD_WITH_EDITOR
    SplashScreen splashScreen{};
    splashScreen.ShowEmbedded("engine/ui/splashscreen", 3.0f);

    float lastTime{static_cast<float>(GetTime())};

    while (!splashScreen.IsFinished() && !WindowShouldClose()) {
        float currentTime{static_cast<float>(GetTime())};
        float deltaTime{currentTime - lastTime};
        lastTime = currentTime;

        splashScreen.Update(deltaTime);

        BeginDrawing();
        ClearBackground(Color{0, 0, 0, 255});
        splashScreen.Render();
        EndDrawing();

        if (!m_Initialized) {
            m_Engine = std::make_unique<Engine>();
            m_Engine->Initialize();

            ProjectSettings& settings = ProjectSettings::Instance();
            settings.ApplyToPhysics(m_Engine->GetPhysicsSystem());

            m_Engine->CreatePhysicsBodies();
            m_Engine->SetPhysicsEnabled(true);
            m_Engine->SetScriptsEnabled(true);

            m_Initialized = true;
            splashScreen.MarkLoadingComplete();
        }
    }
#else
    rlImGuiSetup(true);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    m_Engine = std::make_unique<Engine>();
    m_Engine->Initialize();

    m_EditorLayer = std::make_unique<EditorLayer>(m_Engine.get());

    m_Initialized = true;
#endif

    m_Running = true;
}

void Application::Run() {
    Initialize();

    float lastTime{static_cast<float>(GetTime())};

    while (m_Running && !WindowShouldClose()) {
#ifdef BUILD_WITH_EDITOR
        Profiler::Instance().BeginFrame();
#endif

        float currentTime{static_cast<float>(GetTime())};
        float deltaTime{currentTime - lastTime};
        lastTime = currentTime;

#ifdef BUILD_WITH_EDITOR
        {
            PROFILE_SCOPE("Update");
            Update(deltaTime);
        }
#else
        Update(deltaTime);
#endif

#ifdef BUILD_WITH_EDITOR
        {
            PROFILE_SCOPE("BeginDrawing");
            BeginDrawing();
            ClearBackground(Color{30, 30, 30, 255});
        }

        {
            PROFILE_SCOPE("Render");
            Render();
        }

        {
            PROFILE_SCOPE("EndDrawing + GPU Wait");
            EndDrawing();
        }
#else
        BeginDrawing();
        ClearBackground(Color{30, 30, 30, 255});
        Render();
        EndDrawing();
#endif

#ifdef BUILD_WITH_EDITOR
        Profiler::Instance().EndFrame();
#endif
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
    {
        PROFILE_SCOPE("rlImGuiBegin");
        rlImGuiBegin();
    }

    if (m_EditorLayer) {
        m_EditorLayer->OnImGuiRender();
    }

    {
        PROFILE_SCOPE("rlImGuiEnd");
        rlImGuiEnd();
    }
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
