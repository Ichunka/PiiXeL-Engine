#include "Core/Application.hpp"

#include "Build/GamePackage.hpp"
#include "Build/GamePackageLoader.hpp"
#include "Core/Engine.hpp"
#include "Core/Logger.hpp"
#include "Debug/Profiler.hpp"
#include "Project/ProjectSettings.hpp"
#include "Resources/AssetManager.hpp"
#include "Resources/PathManager.hpp"

#include <raylib.h>

#ifdef BUILD_WITH_EDITOR
#include "Editor/EditorLayer.hpp"
#include "Resources/AssetRegistry.hpp"

#include <imgui.h>
#include <rlImGui.h>
#endif

#ifndef BUILD_WITH_EDITOR
#include "Core/SplashScreen.hpp"
#endif

namespace PiiXeL {

Application::Application(const ApplicationConfig& config) :
    m_Config{config}, m_Engine{nullptr}
#ifdef BUILD_WITH_EDITOR
    ,
    m_EditorLayer{nullptr}
#endif
    ,
    m_Running{false}, m_Initialized{false} {
}

Application::~Application() {
    Cleanup();
}

void Application::Initialize() {
    if (m_Initialized) {
        return;
    }

    PathManager::Instance().Initialize();
    SetExitKey(0);

#ifndef BUILD_WITH_EDITOR
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

    if (!m_Config.iconPath.empty()) {
        Image iconImage{};

        if (m_Config.packageLoader) {
            const GamePackage& package = m_Config.packageLoader->GetPackage();
            const AssetData* iconAsset = package.GetAsset(m_Config.iconPath);

            if (iconAsset && iconAsset->type == "texture") {
                iconImage =
                    LoadImageFromMemory(".png", iconAsset->data.data(), static_cast<int>(iconAsset->data.size()));
                if (iconImage.data != nullptr) {
                    PX_LOG_INFO(ENGINE, "Window icon loaded from package: %s", m_Config.iconPath.c_str());
                }
                else {
                    PX_LOG_WARNING(ENGINE, "Failed to load window icon from package: %s", m_Config.iconPath.c_str());
                }
            }
            else {
                PX_LOG_WARNING(ENGINE, "Window icon not found in package: %s", m_Config.iconPath.c_str());
            }
        }
        else {
            if (FileExists(m_Config.iconPath.c_str())) {
                iconImage = LoadImage(m_Config.iconPath.c_str());
                if (iconImage.data != nullptr) {
                    PX_LOG_INFO(ENGINE, "Window icon loaded from disk: %s", m_Config.iconPath.c_str());
                }
                else {
                    PX_LOG_WARNING(ENGINE, "Failed to load window icon: %s", m_Config.iconPath.c_str());
                }
            }
            else {
                PX_LOG_WARNING(ENGINE, "Window icon file not found: %s", m_Config.iconPath.c_str());
            }
        }

        if (iconImage.data != nullptr) {
            if (iconImage.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
                ImageFormat(&iconImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            }
            SetWindowIcon(iconImage);
            UnloadImage(iconImage);
            PX_LOG_INFO(ENGINE, "Window icon set successfully");
        }
    }

    SplashScreen splashScreen{};
    splashScreen.ShowEmbedded("engine/ui/splashscreen", 3.0f);

    float lastTime{static_cast<float>(GetTime())};

    while (!splashScreen.IsFinished() && !WindowShouldClose()) {
        float currentTime{static_cast<float>(GetTime())};
        float deltaTime{currentTime - lastTime};
        lastTime = currentTime;

        splashScreen.Update(deltaTime);

        BeginDrawing();
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
    if (m_Config.resizable) {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    }
    if (m_Config.vsync) {
        SetConfigFlags(FLAG_VSYNC_HINT);
    }

    InitWindow(800, 450, "PiiXeL Engine - Loading Assets");
    SetTargetFPS(60);

    AssetRegistry::Instance().Initialize();

    AssetRegistry::Instance().ScanAllPxaFiles(".", [&](size_t current, size_t total, const std::string&) {
        float progress = total > 0 ? static_cast<float>(current) / static_cast<float>(total) : 0.0f;

        BeginDrawing();
        ClearBackground(Color{20, 20, 25, 255});

        const char* title = "PiiXeL Engine";
        int titleWidth = MeasureText(title, 48);
        DrawText(title, (800 - titleWidth) / 2, 150, 48, WHITE);

        int barWidth = 600;
        int barHeight = 30;
        int barX = 100;
        int barY = 225;

        DrawRectangle(barX, barY, barWidth, barHeight, Color{40, 40, 45, 255});
        DrawRectangle(barX, barY, static_cast<int>(barWidth * progress), barHeight, Color{100, 150, 255, 255});
        DrawRectangleLines(barX, barY, barWidth, barHeight, Color{80, 80, 85, 255});

        int percentage = static_cast<int>(progress * 100.0f);
        std::string percentText = std::to_string(percentage) + "%";
        int percentWidth = MeasureText(percentText.c_str(), 20);
        DrawText(percentText.c_str(), (800 - percentWidth) / 2, barY + barHeight + 20, 20, LIGHTGRAY);

        EndDrawing();
    });

    SetWindowSize(m_Config.windowWidth, m_Config.windowHeight);
    SetWindowTitle(m_Config.title.c_str());
    SetTargetFPS(m_Config.targetFPS);

    if (m_Config.fullscreen) {
        ToggleFullscreen();
    }

    if (!m_Config.iconPath.empty() && FileExists(m_Config.iconPath.c_str())) {
        Image iconImage = LoadImage(m_Config.iconPath.c_str());
        if (iconImage.data != nullptr) {
            if (iconImage.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
                ImageFormat(&iconImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            }
            SetWindowIcon(iconImage);
            UnloadImage(iconImage);
        }
    }

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
            PX_LOG_INFO(ENGINE, "Switched to FULLSCREEN mode (F11 to exit)");
        }
        else {
            PX_LOG_INFO(ENGINE, "Switched to WINDOWED mode (F11 for fullscreen)");
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
    AssetRegistry::Instance().Shutdown();
    rlImGuiShutdown();
    SetTraceLogCallback(nullptr);
#endif
    CloseWindow();

    m_Initialized = false;
}

} // namespace PiiXeL
