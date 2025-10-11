#ifndef PIIXELENGINE_PROJECTSETTINGS_HPP
#define PIIXELENGINE_PROJECTSETTINGS_HPP

#include <string>
#include <raylib.h>
#include <nlohmann/json.hpp>

namespace PiiXeL {

struct WindowSettings {
    int width{1280};
    int height{720};
    bool resizable{true};
    bool vsync{true};
    bool fullscreen{false};
    int targetFPS{60};
};

struct PhysicsSettings {
    Vector2 gravity{0.0f, 9.8f};
    float timeStep{0.016666f};
    int velocityIterations{8};
    int positionIterations{3};
};

struct ProjectSettings {
    std::string projectName{"My Game"};
    std::string startScene{"Default_Scene"};
    std::string version{"1.0.0"};
    std::string company{"Indie Studio"};

    WindowSettings window;
    PhysicsSettings physics;
    nlohmann::json buildConfig;

    static ProjectSettings& Instance();

    bool Load(const std::string& filepath = "game.config.json");
    bool Save(const std::string& filepath = "game.config.json");

    void ApplyToPhysics(class PhysicsSystem* physicsSystem);

private:
    ProjectSettings() = default;
};

} // namespace PiiXeL

#endif // PIIXELENGINE_PROJECTSETTINGS_HPP
