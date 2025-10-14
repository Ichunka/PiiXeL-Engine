#include "Project/ProjectSettings.hpp"

#include "Systems/PhysicsSystem.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <raylib.h>

namespace PiiXeL {

ProjectSettings& ProjectSettings::Instance() {
    static ProjectSettings instance;
    return instance;
}

bool ProjectSettings::Load(const std::string& filepath) {
    if (!FileExists(filepath.c_str())) {
        TraceLog(LOG_WARNING, "Project config not found: %s - using defaults", filepath.c_str());
        return false;
    }

    std::ifstream file{filepath};
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open project config: %s", filepath.c_str());
        return false;
    }

    nlohmann::json json{};
    try {
        file >> json;
    }
    catch (const nlohmann::json::exception& e) {
        TraceLog(LOG_ERROR, "Failed to parse project config: %s", e.what());
        return false;
    }
    file.close();

    if (json.contains("projectName")) {
        projectName = json["projectName"].get<std::string>();
    }
    if (json.contains("version")) {
        version = json["version"].get<std::string>();
    }
    if (json.contains("company")) {
        company = json["company"].get<std::string>();
    }
    if (json.contains("startScene")) {
        startScene = json["startScene"].get<std::string>();
    }

    if (json.contains("window")) {
        const nlohmann::json& windowJson = json["window"];
        if (windowJson.contains("width"))
            window.width = windowJson["width"].get<int>();
        if (windowJson.contains("height"))
            window.height = windowJson["height"].get<int>();
        if (windowJson.contains("resizable"))
            window.resizable = windowJson["resizable"].get<bool>();
        if (windowJson.contains("vsync"))
            window.vsync = windowJson["vsync"].get<bool>();
        if (windowJson.contains("fullscreen"))
            window.fullscreen = windowJson["fullscreen"].get<bool>();
        if (windowJson.contains("targetFPS"))
            window.targetFPS = windowJson["targetFPS"].get<int>();
        if (windowJson.contains("icon"))
            window.icon = windowJson["icon"].get<std::string>();
    }

    if (json.contains("physics")) {
        const nlohmann::json& physicsJson = json["physics"];
        if (physicsJson.contains("gravity")) {
            const nlohmann::json& gravityJson = physicsJson["gravity"];
            if (gravityJson.is_array() && gravityJson.size() == 2) {
                physics.gravity.x = gravityJson[0].get<float>();
                physics.gravity.y = gravityJson[1].get<float>();
            }
        }
        if (physicsJson.contains("timeStep")) {
            physics.timeStep = physicsJson["timeStep"].get<float>();
        }
        if (physicsJson.contains("velocityIterations")) {
            physics.velocityIterations = physicsJson["velocityIterations"].get<int>();
        }
        if (physicsJson.contains("positionIterations")) {
            physics.positionIterations = physicsJson["positionIterations"].get<int>();
        }
    }

    if (json.contains("build")) {
        buildConfig = json["build"];
    }

    TraceLog(LOG_INFO, "Project settings loaded: %s", projectName.c_str());
    return true;
}

bool ProjectSettings::Save(const std::string& filepath) {
    nlohmann::json json{};

    json["projectName"] = projectName;
    json["version"] = version;
    json["company"] = company;
    json["startScene"] = startScene;

    json["window"]["width"] = window.width;
    json["window"]["height"] = window.height;
    json["window"]["resizable"] = window.resizable;
    json["window"]["vsync"] = window.vsync;
    json["window"]["fullscreen"] = window.fullscreen;
    json["window"]["targetFPS"] = window.targetFPS;
    if (!window.icon.empty()) {
        json["window"]["icon"] = window.icon;
    }

    json["physics"]["gravity"] = {physics.gravity.x, physics.gravity.y};
    json["physics"]["timeStep"] = physics.timeStep;
    json["physics"]["velocityIterations"] = physics.velocityIterations;
    json["physics"]["positionIterations"] = physics.positionIterations;

    if (!buildConfig.is_null()) {
        json["build"] = buildConfig;
    }

    std::ofstream file{filepath};
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to save project config: %s", filepath.c_str());
        return false;
    }

    file << json.dump(4);
    file.close();

    TraceLog(LOG_INFO, "Project settings saved: %s", filepath.c_str());
    return true;
}

void ProjectSettings::ApplyToPhysics(PhysicsSystem* physicsSystem) {
    if (physicsSystem) {
        physicsSystem->SetGravity(physics.gravity);
    }
}

} // namespace PiiXeL
