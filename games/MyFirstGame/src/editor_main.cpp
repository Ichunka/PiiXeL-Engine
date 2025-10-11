#include "Core/Application.hpp"
#include "Reflection/ReflectionInit.hpp"
#include "../include/ExampleScripts.hpp"
#include <iostream>

int main() {
    try {
        PiiXeL::Reflection::InitializeReflection();

        PiiXeL::ApplicationConfig config{};
        config.title = "PiiXeL Engine - Editor";
        config.windowWidth = 1280;
        config.windowHeight = 720;
        config.targetFPS = 0;
        config.vsync = false;
        config.resizable = true;
        config.fullscreen = false;

        PiiXeL::Application app{config};
        app.Run();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
