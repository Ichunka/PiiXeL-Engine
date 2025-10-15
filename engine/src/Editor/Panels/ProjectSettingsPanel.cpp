#ifdef BUILD_WITH_EDITOR

#include "Editor/Panels/ProjectSettingsPanel.hpp"

#include "Core/Engine.hpp"
#include "Debug/Profiler.hpp"
#include "Project/ProjectSettings.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <imgui.h>

namespace PiiXeL {

ProjectSettingsPanel::ProjectSettingsPanel(Engine* engine) : m_Engine{engine}, m_IsOpen{false} {}

void ProjectSettingsPanel::OnImGuiRender() {
    PROFILE_FUNCTION();
    if (!m_IsOpen) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2{600, 500}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Project Settings", &m_IsOpen)) {
        ProjectSettings& settings = ProjectSettings::Instance();

        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("General")) {
                ImGui::SeparatorText("Project Info");

                char projectName[256];
                std::memcpy(projectName, settings.projectName.c_str(),
                            std::min(settings.projectName.size(), sizeof(projectName) - 1));
                projectName[std::min(settings.projectName.size(), sizeof(projectName) - 1)] = '\0';

                if (ImGui::InputText("Project Name", projectName, sizeof(projectName))) {
                    settings.projectName = std::string(projectName);
                }

                char version[64];
                std::memcpy(version, settings.version.c_str(), std::min(settings.version.size(), sizeof(version) - 1));
                version[std::min(settings.version.size(), sizeof(version) - 1)] = '\0';

                if (ImGui::InputText("Version", version, sizeof(version))) {
                    settings.version = std::string(version);
                }

                char company[256];
                std::memcpy(company, settings.company.c_str(), std::min(settings.company.size(), sizeof(company) - 1));
                company[std::min(settings.company.size(), sizeof(company) - 1)] = '\0';

                if (ImGui::InputText("Company", company, sizeof(company))) {
                    settings.company = std::string(company);
                }

                char startScene[256];
                std::memcpy(startScene, settings.startScene.c_str(),
                            std::min(settings.startScene.size(), sizeof(startScene) - 1));
                startScene[std::min(settings.startScene.size(), sizeof(startScene) - 1)] = '\0';

                if (ImGui::InputText("Start Scene", startScene, sizeof(startScene))) {
                    settings.startScene = std::string(startScene);
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Window")) {
                ImGui::SeparatorText("Window Settings");

                ImGui::DragInt("Width", &settings.window.width, 1.0f, 640, 3840);
                ImGui::DragInt("Height", &settings.window.height, 1.0f, 480, 2160);
                ImGui::DragInt("Target FPS", &settings.window.targetFPS, 1.0f, 30, 240);

                ImGui::Checkbox("Resizable", &settings.window.resizable);
                ImGui::Checkbox("VSync", &settings.window.vsync);
                ImGui::Checkbox("Fullscreen by Default", &settings.window.fullscreen);

                ImGui::SeparatorText("Window Icon");
                char iconPath[512];
                std::memcpy(iconPath, settings.window.icon.c_str(),
                            std::min(settings.window.icon.size(), sizeof(iconPath) - 1));
                iconPath[std::min(settings.window.icon.size(), sizeof(iconPath) - 1)] = '\0';
                if (ImGui::InputText("Icon Path", iconPath, sizeof(iconPath))) {
                    settings.window.icon = std::string(iconPath);
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Path to icon file (relative to game directory)\n\n"
                                      "Recommended: Use .ico format for both window and exe icon\n\n"
                                      "How it works:\n"
                                      "1. Set path to .ico file (e.g., content/assets/icon.ico)\n"
                                      "2. Engine tries to load .ico for window icon\n"
                                      "3. If .ico fails, automatically uses .png fallback\n"
                                      "4. Windows exe icon is embedded via CMake\n\n"
                                      "Fallback: If .ico doesn't work for window icon,\n"
                                      "create icon.png in same directory\n\n"
                                      "Any pixel format works (RGBA, RGB, Grayscale)\n"
                                      "- Engine auto-converts to required format");
                }

                ImGui::Spacing();
                ImGui::TextDisabled("Note: These settings apply to the built game.");
                ImGui::TextDisabled("Use F11 to toggle fullscreen at runtime.");

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Physics")) {
                ImGui::SeparatorText("Physics Settings");

                ImGui::DragFloat2("Gravity", &settings.physics.gravity.x, 0.1f, -100.0f, 100.0f);
                ImGui::DragFloat("Time Step", &settings.physics.timeStep, 0.001f, 0.001f, 0.1f, "%.4f");
                ImGui::DragInt("Velocity Iterations", &settings.physics.velocityIterations, 1.0f, 1, 20);
                ImGui::DragInt("Position Iterations", &settings.physics.positionIterations, 1.0f, 1, 20);

                ImGui::Spacing();
                if (ImGui::Button("Apply to Current Physics")) {
                    if (m_Engine && m_Engine->GetPhysicsSystem()) {
                        settings.ApplyToPhysics(m_Engine->GetPhysicsSystem());
                    }
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Build")) {
                ImGui::SeparatorText("Build Settings");

                ImGui::Text("Assets Mode:");
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Auto: Only include assets used in scenes");
                    ImGui::Text("All: Include all assets from content/assets");
                    ImGui::Text("Manual: Manually specify assets to include");
                    ImGui::EndTooltip();
                }

                const char* assetsModes[] = {"auto", "all", "manual"};
                int currentMode = 0;
                std::string currentModeStr = "auto";

                if (settings.buildConfig.contains("assetsMode")) {
                    currentModeStr = settings.buildConfig["assetsMode"].get<std::string>();
                    if (currentModeStr == "all")
                        currentMode = 1;
                    else if (currentModeStr == "manual")
                        currentMode = 2;
                }

                if (ImGui::Combo("##AssetsMode", &currentMode, assetsModes, 3)) {
                    settings.buildConfig["assetsMode"] = assetsModes[currentMode];
                }

                ImGui::Spacing();
                ImGui::SeparatorText("Scenes to Export");

                if (!settings.buildConfig.contains("scenes") || !settings.buildConfig["scenes"].is_array()) {
                    settings.buildConfig["scenes"] = nlohmann::json::array();
                }

                nlohmann::json& scenes = settings.buildConfig["scenes"];

                for (size_t i = 0; i < scenes.size(); ++i) {
                    ImGui::PushID(static_cast<int>(i));

                    std::string scenePath = scenes[i].get<std::string>();
                    char scenePathBuf[512];
                    std::memcpy(scenePathBuf, scenePath.c_str(), std::min(scenePath.size(), sizeof(scenePathBuf) - 1));
                    scenePathBuf[std::min(scenePath.size(), sizeof(scenePathBuf) - 1)] = '\0';

                    ImGui::SetNextItemWidth(-60.0f);
                    if (ImGui::InputText("##scene", scenePathBuf, sizeof(scenePathBuf))) {
                        scenes[i] = std::string(scenePathBuf);
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Remove")) {
                        scenes.erase(scenes.begin() + static_cast<int>(i));
                        --i;
                    }

                    ImGui::PopID();
                }

                if (ImGui::Button("Add Scene")) {
                    scenes.push_back("content/scenes/NewScene.scene");
                }

                if (currentMode == 2) {
                    ImGui::Spacing();
                    ImGui::SeparatorText("Assets to Export (Manual Mode)");

                    if (!settings.buildConfig.contains("assets") || !settings.buildConfig["assets"].is_array()) {
                        settings.buildConfig["assets"] = nlohmann::json::array();
                    }

                    nlohmann::json& assets = settings.buildConfig["assets"];

                    for (size_t i = 0; i < assets.size(); ++i) {
                        ImGui::PushID(1000 + static_cast<int>(i));

                        std::string assetPath = assets[i].get<std::string>();
                        char assetPathBuf[512];
                        std::memcpy(assetPathBuf, assetPath.c_str(),
                                    std::min(assetPath.size(), sizeof(assetPathBuf) - 1));
                        assetPathBuf[std::min(assetPath.size(), sizeof(assetPathBuf) - 1)] = '\0';

                        ImGui::SetNextItemWidth(-60.0f);
                        if (ImGui::InputText("##asset", assetPathBuf, sizeof(assetPathBuf))) {
                            assets[i] = std::string(assetPathBuf);
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("Remove")) {
                            assets.erase(assets.begin() + static_cast<int>(i));
                            --i;
                        }

                        ImGui::PopID();
                    }

                    if (ImGui::Button("Add Asset")) {
                        assets.push_back("content/assets/");
                    }
                }
                else {
                    ImGui::Spacing();
                    if (currentMode == 0) {
                        ImGui::TextDisabled("Assets will be automatically detected from scenes");
                    }
                    else {
                        ImGui::TextDisabled("All assets from content/assets will be included");
                    }
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save Settings")) {
            settings.Save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Settings")) {
            settings.Load();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            m_IsOpen = false;
        }
    }
    ImGui::End();
}

} // namespace PiiXeL

#endif
