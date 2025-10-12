#include "Build/GamePackageBuilder.hpp"
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <cinttypes>
#include <raylib.h>

namespace PiiXeL {

#ifdef _WIN32
struct IconPixelData {
    std::vector<unsigned char> pixels;
    int width;
    int height;
};

extern bool ConvertIcoToRawPixels(const std::string& icoPath, IconPixelData& outData);

static AssetData ConvertIcoToPngInMemory(const std::string& icoPath) {
    AssetData asset{};
    asset.type = "texture";

    IconPixelData pixelData{};

    if (!ConvertIcoToRawPixels(icoPath, pixelData)) {
        TraceLog(LOG_ERROR, "Failed to load .ico file: %s", icoPath.c_str());
        return asset;
    }

    Image image{};
    image.data = pixelData.pixels.data();
    image.width = pixelData.width;
    image.height = pixelData.height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    int fileSize{};
    unsigned char* pngData = ExportImageToMemory(image, ".png", &fileSize);

    if (pngData && fileSize > 0) {
        asset.data.assign(pngData, pngData + fileSize);
        RL_FREE(pngData);
        TraceLog(LOG_INFO, "Successfully converted %s to PNG in memory (%d bytes)", icoPath.c_str(), fileSize);
    } else {
        TraceLog(LOG_ERROR, "Failed to export PNG to memory: %s", icoPath.c_str());
    }

    return asset;
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
                AssetData iconAsset = ConvertIcoToPngInMemory(fullIconPath.string());
                if (!iconAsset.data.empty()) {
                    std::string pngRelativePath = iconPath.substr(0, iconPath.find_last_of('.')) + ".png";
                    iconAsset.path = pngRelativePath;
                    config["icon"] = pngRelativePath;
                    package.AddAsset(iconAsset);
                    TraceLog(LOG_INFO, "Converted icon from .ico to .png and added to package: %s", pngRelativePath.c_str());
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

    std::string uuidCachePath = (basePath / "datas" / ".asset_uuid_cache").string();
    if (std::filesystem::exists(uuidCachePath)) {
        AssetData cacheAsset = LoadAssetFile(uuidCachePath, "data");
        if (!cacheAsset.data.empty()) {
            cacheAsset.path = "datas/.asset_uuid_cache";
            package.AddAsset(cacheAsset);
            TraceLog(LOG_INFO, "Added UUID cache to package");
        }
    }

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
    std::unordered_set<uint64_t> collectedUUIDs;
    std::unordered_map<uint64_t, std::string> uuidToPath;

    std::string cachePath = (basePath / "datas" / ".asset_uuid_cache").string();
    if (std::filesystem::exists(cachePath)) {
        std::ifstream cacheFile{cachePath, std::ios::binary};
        if (cacheFile.is_open()) {
            uint32_t count = 0;
            cacheFile.read(reinterpret_cast<char*>(&count), sizeof(count));

            for (uint32_t i = 0; i < count; ++i) {
                uint32_t pathLen = 0;
                cacheFile.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));

                std::string path(pathLen, '\0');
                cacheFile.read(&path[0], pathLen);

                uint64_t uuidValue = 0;
                cacheFile.read(reinterpret_cast<char*>(&uuidValue), sizeof(uuidValue));

                std::replace(path.begin(), path.end(), '\\', '/');

                uuidToPath[uuidValue] = path;
            }
            cacheFile.close();
            TraceLog(LOG_INFO, "Loaded UUID cache with %u entries", count);
        }
    } else {
        TraceLog(LOG_WARNING, "UUID cache not found: %s", cachePath.c_str());
    }

    std::unordered_set<uint64_t> missingUUIDs;
    for (const std::pair<std::string, nlohmann::json>& scenePair : package.GetScenes()) {
        const nlohmann::json& sceneData = scenePair.second;
        if (sceneData.contains("entities") && sceneData["entities"].is_array()) {
            for (const nlohmann::json& entity : sceneData["entities"]) {
                if (entity.contains("Sprite")) {
                    const nlohmann::json& sprite = entity["Sprite"];
                    if (sprite.contains("textureAssetUUID")) {
                        uint64_t uuidValue = sprite["textureAssetUUID"].get<uint64_t>();
                        if (uuidValue != 0 && uuidToPath.find(uuidValue) == uuidToPath.end()) {
                            missingUUIDs.insert(uuidValue);
                        }
                    }
                }
            }
        }
    }

    if (!missingUUIDs.empty()) {
        TraceLog(LOG_INFO, "Scanning for .pxa files to resolve %zu missing UUIDs...", missingUUIDs.size());

        std::string contentPath = (basePath / "content").string();
        if (std::filesystem::exists(contentPath)) {
            for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(contentPath)) {
                if (entry.is_regular_file() && entry.path().extension() == ".pxa") {
                    TraceLog(LOG_INFO, "Checking .pxa file: %s", entry.path().string().c_str());
                    std::ifstream pxaFile{entry.path(), std::ios::binary};
                    if (!pxaFile.is_open()) {
                        TraceLog(LOG_WARNING, "Failed to open .pxa file: %s", entry.path().string().c_str());
                        continue;
                    }

                    uint32_t magic = 0;
                    pxaFile.read(reinterpret_cast<char*>(&magic), sizeof(magic));
                    if (magic != 0x41585850) {
                        TraceLog(LOG_WARNING, "Invalid magic number in .pxa file: %s (got 0x%08X)", entry.path().string().c_str(), magic);
                        pxaFile.close();
                        continue;
                    }

                    uint32_t version = 0;
                    pxaFile.read(reinterpret_cast<char*>(&version), sizeof(version));

                    uint16_t assetType = 0;
                    pxaFile.read(reinterpret_cast<char*>(&assetType), sizeof(assetType));

                    uint16_t reserved = 0;
                    pxaFile.read(reinterpret_cast<char*>(&reserved), sizeof(reserved));

                    uint64_t uuidValue = 0;
                    pxaFile.read(reinterpret_cast<char*>(&uuidValue), sizeof(uuidValue));

                    TraceLog(LOG_INFO, "Read UUID %" PRIu64 " from .pxa file", uuidValue);

                    uint64_t metadataSize = 0;
                    pxaFile.read(reinterpret_cast<char*>(&metadataSize), sizeof(metadataSize));

                    if (missingUUIDs.find(uuidValue) != missingUUIDs.end()) {
                        TraceLog(LOG_INFO, "UUID %" PRIu64 " matches a missing UUID, reading metadata...", uuidValue);
                        pxaFile.seekg(sizeof(uint64_t) * 2, std::ios::cur);

                        std::string metadataStr(static_cast<size_t>(metadataSize), '\0');
                        pxaFile.read(&metadataStr[0], static_cast<std::streamsize>(metadataSize));

                        size_t pos1 = metadataStr.find('|');
                        size_t pos2 = metadataStr.find('|', pos1 + 1);

                        if (pos1 != std::string::npos && pos2 != std::string::npos) {
                            std::string extension = metadataStr.substr(pos1 + 1, pos2 - pos1 - 1);

                            std::filesystem::path pxaPath = entry.path();
                            std::string sourcePath = pxaPath.parent_path().string();
                            if (!sourcePath.empty()) sourcePath += "/";
                            sourcePath += pxaPath.stem().string() + extension;
                            std::replace(sourcePath.begin(), sourcePath.end(), '\\', '/');

                            uuidToPath[uuidValue] = sourcePath;
                            TraceLog(LOG_INFO, "Resolved UUID %" PRIu64 " -> %s from .pxa scan", uuidValue, sourcePath.c_str());
                        }
                    }

                    pxaFile.close();
                }
            }
        }
    }

    for (const std::pair<std::string, nlohmann::json>& scenePair : package.GetScenes()) {
        const nlohmann::json& sceneData = scenePair.second;

        if (sceneData.contains("entities") && sceneData["entities"].is_array()) {
            for (const nlohmann::json& entity : sceneData["entities"]) {
                if (entity.contains("Sprite")) {
                    const nlohmann::json& sprite = entity["Sprite"];
                    if (sprite.contains("textureAssetUUID")) {
                        uint64_t uuidValue = sprite["textureAssetUUID"].get<uint64_t>();

                        if (uuidValue != 0 && collectedUUIDs.find(uuidValue) == collectedUUIDs.end()) {
                            auto it = uuidToPath.find(uuidValue);
                            if (it != uuidToPath.end()) {
                                std::string sourcePath = it->second;
                                std::string pxaPath = sourcePath.substr(0, sourcePath.find_last_of('.')) + ".pxa";
                                std::string fullPath = (basePath / pxaPath).string();

                                if (std::filesystem::exists(fullPath)) {
                                    AssetData asset = LoadAssetFile(fullPath, "texture");
                                    if (!asset.data.empty()) {
                                        std::string pathInPackage = pxaPath;
                                        for (char& c : pathInPackage) {
                                            if (c == '\\') c = '/';
                                        }
                                        asset.path = pathInPackage;
                                        package.AddAsset(asset);
                                        collectedUUIDs.insert(uuidValue);
                                        TraceLog(LOG_INFO, "Auto-collected asset: %s (UUID: %" PRIu64 ")", asset.path.c_str(), uuidValue);
                                    }
                                } else {
                                    TraceLog(LOG_WARNING, "Asset file not found: %s (UUID: %" PRIu64 ")", fullPath.c_str(), uuidValue);
                                }
                            } else {
                                TraceLog(LOG_WARNING, "Asset not found in registry: %" PRIu64, uuidValue);
                            }
                        }
                    }
                }
            }
        }
    }

    TraceLog(LOG_INFO, "Collected %zu unique assets from scenes", collectedUUIDs.size());
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
