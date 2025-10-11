#include "Core/Application.hpp"
#include "Project/ProjectSettings.hpp"
#include "Reflection/ReflectionInit.hpp"
#include <iostream>

int main() {
    try {
        PiiXeL::Reflection::InitializeReflection();

        // Load project settings from game.config.json
        PiiXeL::ProjectSettings& settings = PiiXeL::ProjectSettings::Instance();
        settings.Load("game.config.json");

        // Apply settings to application config
        PiiXeL::ApplicationConfig config{};
        config.title = settings.projectName;
        config.windowWidth = settings.window.width;
        config.windowHeight = settings.window.height;
        config.targetFPS = settings.window.targetFPS;
        config.vsync = settings.window.vsync;
        config.resizable = settings.window.resizable;
        config.fullscreen = settings.window.fullscreen;

        PiiXeL::Application app{config};
        app.Run();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
