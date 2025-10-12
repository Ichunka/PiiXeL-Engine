#include "Build/GamePackageBuilder.hpp"
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <raylib.h>

namespace PiiXeL {

#ifdef _WIN32
struct IconPixelData {
    std::vector<unsigned char> pixels;
    int width;
    int height;
};

extern bool ConvertIcoToRawPixels(const std::string& icoPath, IconPixelData& outData);

static bool ConvertIcoToPng(const std::string& icoPath, const std::string& pngPath) {
    IconPixelData pixelData{};

    if (!ConvertIcoToRawPixels(icoPath, pixelData)) {
        TraceLog(LOG_ERROR, "Failed to load .ico file: %s", icoPath.c_str());
        return false;
    }

    Image image{};
    image.data = pixelData.pixels.data();
    image.width = pixelData.width;
    image.height = pixelData.height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    bool success = ExportImage(image, pngPath.c_str());

    if (success) {
        TraceLog(LOG_INFO, "Successfully converted %s to %s", icoPath.c_str(), pngPath.c_str());
    } else {
        TraceLog(LOG_ERROR, "Failed to export PNG: %s", pngPath.c_str());
    }

    return success;
}
#endif

GamePackageBuilder::GamePackageBuilder() = default;
GamePackageBuilder::~GamePackageBuilder() = default;

bool GamePackageBuilder::BuildFromProject(const std::string& projectPath, const std::string& outputPath) {
    GamePackage package{};

    GamePackageHeader header{};
    header.version = 1;
    header.engineVersion = "0.1.0";
    package.SetHeader(header);

    std::string configPath = projectPath + "/game.config.json";
    if (!std::filesystem::exists(configPath)) {
        configPath = std::filesystem::current_path().string() + "/game.config.json";
    }

    std::filesystem::path basePath = std::filesystem::path(configPath).parent_path();

    nlohmann::json projectConfig{};
    std::ifstream configFile{configPath};
    if (configFile.is_open()) {
        try {
            configFile >> projectConfig;
            TraceLog(LOG_INFO, "Loaded project config: %s", configPath.c_str());
        } catch (const nlohmann::json::exception& e) {
            TraceLog(LOG_ERROR, "Failed to parse config: %s", e.what());
        }
        configFile.close();
    } else {
        TraceLog(LOG_WARNING, "Config file not found: %s", configPath.c_str());
    }

    nlohmann::json config{};
    config["title"] = projectConfig.value("projectName", "PiiXeL Game");
    config["windowWidth"] = projectConfig.value("window", nlohmann::json{}).value("width", 1280);
    config["windowHeight"] = projectConfig.value("window", nlohmann::json{}).value("height", 720);
    config["targetFPS"] = projectConfig.value("window", nlohmann::json{}).value("targetFPS", 60);
    config["vsync"] = projectConfig.value("window", nlohmann::json{}).value("vsync", true);
    config["mainScene"] = projectConfig.value("startScene", "content/scenes/Default_Scene.scene");

    std::string iconPath = projectConfig.value("window", nlohmann::json{}).value("icon", "");
    if (!iconPath.empty()) {
        std::filesystem::path fullIconPath = basePath / iconPath;
        if (std::filesystem::exists(fullIconPath)) {
            std::string extension = fullIconPath.extension().string();
            if (extension == ".ico") {
#ifdef _WIN32
                std::string pngPath = fullIconPath.string();
                pngPath = pngPath.substr(0, pngPath.find_last_of('.')) + ".png";

                if (ConvertIcoToPng(fullIconPath.string(), pngPath)) {
                    std::string pngRelativePath = iconPath.substr(0, iconPath.find_last_of('.')) + ".png";
                    config["icon"] = pngRelativePath;
                    TraceLog(LOG_INFO, "Converted icon from .ico to .png: %s", pngRelativePath.c_str());

                    AssetData iconAsset = LoadAssetFile(pngPath, "texture");
                    if (!iconAsset.data.empty()) {
                        iconAsset.path = pngRelativePath;
                        package.AddAsset(iconAsset);
                        TraceLog(LOG_INFO, "Added converted icon to package: %s", pngRelativePath.c_str());
                    }
                } else {
                    TraceLog(LOG_WARNING, "Failed to convert .ico to .png, icon will not be included");
                }
#else
                TraceLog(LOG_WARNING, "ICO to PNG conversion only supported on Windows, icon will not be included");
#endif
            } else {
                config["icon"] = iconPath;
            }
        } else {
            TraceLog(LOG_WARNING, "Icon file not found: %s", fullIconPath.string().c_str());
        }
    }

    package.SetConfig(config);

    if (projectConfig.contains("build") && projectConfig["build"].contains("scenes")) {
        for (const auto& scenePath : projectConfig["build"]["scenes"]) {
            std::string fullPath = (basePath / scenePath.get<std::string>()).string();
            if (std::filesystem::exists(fullPath)) {
                std::ifstream file{fullPath};
                if (file.is_open()) {
                    nlohmann::json sceneData{};
                    try {
                        file >> sceneData;
                        std::string sceneName = std::filesystem::path(fullPath).stem().string();
                        package.AddScene(sceneName, sceneData);
                        TraceLog(LOG_INFO, "Added scene: %s", sceneName.c_str());
                    } catch (const nlohmann::json::exception& e) {
                        TraceLog(LOG_ERROR, "Failed to parse scene: %s - %s", fullPath.c_str(), e.what());
                    }
                    file.close();
                }
            } else {
                TraceLog(LOG_WARNING, "Scene file not found: %s", fullPath.c_str());
            }
        }
    } else {
        std::string scenesPath = (basePath / "content/scenes").string();
        ScanScenes(scenesPath, package);
    }

    if (projectConfig.contains("build") && projectConfig["build"].contains("assets")) {
        std::string assetsMode = projectConfig["build"].value("assetsMode", "auto");

        if (assetsMode == "auto") {
            CollectAssetsFromScenes(package, basePath);
        } else if (assetsMode == "all") {
            std::string assetsPath = (basePath / "content/assets").string();
            ScanAssets(assetsPath, package);
        } else if (assetsMode == "manual") {
            for (const auto& assetPathStr : projectConfig["build"]["assets"]) {
                std::string assetPath = (basePath / assetPathStr.get<std::string>()).string();
                if (std::filesystem::exists(assetPath)) {
                    if (std::filesystem::is_directory(assetPath)) {
                        ScanAssets(assetPath, package);
                    } else {
                        std::string extension = std::filesystem::path(assetPath).extension().string();
                        std::string type{};

                        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp") {
                            type = "texture";
                        } else if (extension == ".wav" || extension == ".mp3" || extension == ".ogg") {
                            type = "audio";
                        }

                        if (!type.empty()) {
                            AssetData asset = LoadAssetFile(assetPath, type);
                            if (!asset.data.empty()) {
                                std::string pathInPackage = assetPathStr.get<std::string>();
                                for (char& c : pathInPackage) {
                                    if (c == '\\') c = '/';
                                }
                                asset.path = pathInPackage;
                                package.AddAsset(asset);
                                TraceLog(LOG_INFO, "Added asset: %s (%s)", asset.path.c_str(), type.c_str());
                            }
                        }
                    }
                } else {
                    TraceLog(LOG_WARNING, "Asset path not found: %s", assetPath.c_str());
                }
            }
        }
    } else {
        CollectAssetsFromScenes(package, basePath);
    }

    if (!package.SaveToFile(outputPath)) {
        TraceLog(LOG_ERROR, "Failed to save game package");
        return false;
    }

    TraceLog(LOG_INFO, "Game package built successfully: %s", outputPath.c_str());
    return true;
}

void GamePackageBuilder::ScanScenes(const std::string& scenesPath, GamePackage& package) {
    if (!std::filesystem::exists(scenesPath)) {
        TraceLog(LOG_WARNING, "Scenes directory not found: %s", scenesPath.c_str());
        return;
    }

    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(scenesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".scene") {
            std::ifstream file{entry.path()};
            if (file.is_open()) {
                nlohmann::json sceneData{};
                try {
                    file >> sceneData;
                    std::string sceneName = entry.path().stem().string();
                    package.AddScene(sceneName, sceneData);
                    TraceLog(LOG_INFO, "Added scene: %s", sceneName.c_str());
                } catch (const nlohmann::json::exception& e) {
                    TraceLog(LOG_ERROR, "Failed to parse scene: %s - %s", entry.path().string().c_str(), e.what());
                }
                file.close();
            }
        }
    }
}

void GamePackageBuilder::ScanAssets(const std::string& assetsPath, GamePackage& package) {
    if (!std::filesystem::exists(assetsPath)) {
        TraceLog(LOG_WARNING, "Assets directory not found: %s", assetsPath.c_str());
        return;
    }

    for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(assetsPath)) {
        if (entry.is_regular_file()) {
            std::string extension = entry.path().extension().string();
            std::string type{};

            if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp") {
                type = "texture";
            } else if (extension == ".wav" || extension == ".mp3" || extension == ".ogg") {
                type = "audio";
            } else {
                continue;
            }

            std::string relativePath = std::filesystem::relative(entry.path(), assetsPath).string();
            for (char& c : relativePath) {
                if (c == '\\') c = '/';
            }

            AssetData asset = LoadAssetFile(entry.path().string(), type);
            if (!asset.data.empty()) {
                asset.path = "content/assets/" + relativePath;
                package.AddAsset(asset);
                TraceLog(LOG_INFO, "Added asset: %s (%s)", asset.path.c_str(), type.c_str());
            }
        }
    }
}

void GamePackageBuilder::CollectAssetsFromScenes(GamePackage& package, const std::filesystem::path& basePath) {
    std::unordered_set<std::string> collectedAssets;

    for (const std::pair<std::string, nlohmann::json>& scenePair : package.GetScenes()) {
        const nlohmann::json& sceneData = scenePair.second;

        if (sceneData.contains("entities") && sceneData["entities"].is_array()) {
            for (const nlohmann::json& entity : sceneData["entities"]) {
                if (entity.contains("Sprite")) {
                    const nlohmann::json& sprite = entity["Sprite"];
                    if (sprite.contains("texturePath")) {
                        std::string texturePath = sprite["texturePath"].get<std::string>();
                        if (!texturePath.empty() && collectedAssets.find(texturePath) == collectedAssets.end()) {
                            std::string fullPath = (basePath / texturePath).string();
                            if (std::filesystem::exists(fullPath)) {
                                AssetData asset = LoadAssetFile(fullPath, "texture");
                                if (!asset.data.empty()) {
                                    std::string pathInPackage = texturePath;
                                    for (char& c : pathInPackage) {
                                        if (c == '\\') c = '/';
                                    }
                                    asset.path = pathInPackage;
                                    package.AddAsset(asset);
                                    collectedAssets.insert(texturePath);
                                    TraceLog(LOG_INFO, "Auto-collected asset: %s (texture)", asset.path.c_str());
                                }
                            } else {
                                TraceLog(LOG_WARNING, "Asset referenced in scene not found: %s", fullPath.c_str());
                            }
                        }
                    }
                }
            }
        }
    }

    TraceLog(LOG_INFO, "Collected %zu unique assets from scenes", collectedAssets.size());
}

AssetData GamePackageBuilder::LoadAssetFile(const std::string& filepath, const std::string& type) {
    AssetData asset{};
    asset.type = type;

    std::ifstream file{filepath, std::ios::binary | std::ios::ate};
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open asset file: %s", filepath.c_str());
        return asset;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    asset.data.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(asset.data.data()), size)) {
        TraceLog(LOG_ERROR, "Failed to read asset file: %s", filepath.c_str());
        asset.data.clear();
    }

    file.close();
    return asset;
}

} // namespace PiiXeL
