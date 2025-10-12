#include "Core/Application.hpp"
#include "Project/ProjectSettings.hpp"
#include "Reflection/ReflectionInit.hpp"
#include "Build/GamePackageLoader.hpp"
#include "Build/GamePackage.hpp"
#include <iostream>

int main() {
    try {
        PiiXeL::Reflection::InitializeReflection();

        PiiXeL::ProjectSettings& settings = PiiXeL::ProjectSettings::Instance();
        settings.Load("game.config.json");

        PiiXeL::GamePackageLoader packageLoader{};
        if (!packageLoader.LoadPackage("datas/game.package")) {
            std::cerr << "Failed to load game package" << std::endl;
            return 1;
        }

        const PiiXeL::GamePackage& package = packageLoader.GetPackage();
        const nlohmann::json& packageConfig = package.GetConfig();

        PiiXeL::ApplicationConfig config{};
        config.title = packageConfig.value("title", "PiiXeL Game");
        config.windowWidth = packageConfig.value("windowWidth", 1280);
        config.windowHeight = packageConfig.value("windowHeight", 720);
        config.targetFPS = packageConfig.value("targetFPS", 60);
        config.vsync = packageConfig.value("vsync", true);
        config.resizable = packageConfig.value("resizable", true);
        config.fullscreen = packageConfig.value("fullscreen", false);
        config.iconPath = packageConfig.value("icon", "");

        PiiXeL::Application app{config};
        app.Run();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
