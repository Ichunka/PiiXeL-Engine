#include "Build/GamePackageBuilder.hpp"
#include "Core/Logger.hpp"
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
        PX_LOG_ERROR(BUILD, "Failed to load .ico file: %s", icoPath.c_str());
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
        PX_LOG_INFO(BUILD, "Successfully converted %s to PNG in memory (%d bytes)", icoPath.c_str(), fileSize);
    } else {
        PX_LOG_ERROR(BUILD, "Failed to export PNG to memory: %s", icoPath.c_str());
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
            PX_LOG_INFO(BUILD, "Loaded project config: %s", configPath.c_str());
        } catch (const nlohmann::json::exception& e) {
            PX_LOG_ERROR(BUILD, "Failed to parse config: %s", e.what());
        }
        configFile.close();
    } else {
        PX_LOG_WARNING(BUILD, "Config file not found: %s", configPath.c_str());
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
                    PX_LOG_INFO(BUILD, "Converted icon from .ico to .png and added to package: %s", pngRelativePath.c_str());
                } else {
                    PX_LOG_WARNING(BUILD, "Failed to convert .ico to .png, icon will not be included");
                }
#else
                PX_LOG_WARNING(BUILD, "ICO to PNG conversion only supported on Windows, icon will not be included");
#endif
            } else {
                config["icon"] = iconPath;
            }
        } else {
            PX_LOG_WARNING(BUILD, "Icon file not found: %s", fullIconPath.string().c_str());
        }
    }

    package.SetConfig(config);

    std::string uuidCachePath = (basePath / "datas" / ".asset_uuid_cache").string();
    if (std::filesystem::exists(uuidCachePath)) {
        AssetData cacheAsset = LoadAssetFile(uuidCachePath, "data");
        if (!cacheAsset.data.empty()) {
            cacheAsset.path = "datas/.asset_uuid_cache";
            package.AddAsset(cacheAsset);
            PX_LOG_INFO(BUILD, "Added UUID cache to package");
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
                        PX_LOG_INFO(BUILD, "Added scene: %s", sceneName.c_str());
                    } catch (const nlohmann::json::exception& e) {
                        PX_LOG_ERROR(BUILD, "Failed to parse scene: %s - %s", fullPath.c_str(), e.what());
                    }
                    file.close();
                }
            } else {
                PX_LOG_WARNING(BUILD, "Scene file not found: %s", fullPath.c_str());
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
                                PX_LOG_INFO(BUILD, "Added asset: %s (%s)", asset.path.c_str(), type.c_str());
                            }
                        }
                    }
                } else {
                    PX_LOG_WARNING(BUILD, "Asset path not found: %s", assetPath.c_str());
                }
            }
        }
    } else {
        CollectAssetsFromScenes(package, basePath);
    }

    if (!package.SaveToFile(outputPath)) {
        PX_LOG_ERROR(BUILD, "Failed to save game package");
        return false;
    }

    PX_LOG_INFO(BUILD, "Game package built successfully: %s", outputPath.c_str());
    return true;
}

void GamePackageBuilder::ScanScenes(const std::string& scenesPath, GamePackage& package) {
    if (!std::filesystem::exists(scenesPath)) {
        PX_LOG_WARNING(BUILD, "Scenes directory not found: %s", scenesPath.c_str());
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
                    PX_LOG_INFO(BUILD, "Added scene: %s", sceneName.c_str());
                } catch (const nlohmann::json::exception& e) {
                    PX_LOG_ERROR(BUILD, "Failed to parse scene: %s - %s", entry.path().string().c_str(), e.what());
                }
                file.close();
            }
        }
    }
}

void GamePackageBuilder::ScanAssets(const std::string& assetsPath, GamePackage& package) {
    if (!std::filesystem::exists(assetsPath)) {
        PX_LOG_WARNING(BUILD, "Assets directory not found: %s", assetsPath.c_str());
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
                PX_LOG_INFO(BUILD, "Added asset: %s (%s)", asset.path.c_str(), type.c_str());
            }
        }
    }
}

void GamePackageBuilder::CollectUUIDsFromComponent(const nlohmann::json& component, std::vector<uint64_t>& uuids) {
    for (auto it = component.begin(); it != component.end(); ++it) {
        if (it.value().is_number_unsigned()) {
            std::string key = it.key();
            if (key.find("UUID") != std::string::npos || key.find("uuid") != std::string::npos) {
                uint64_t uuidValue = it.value().get<uint64_t>();
                if (uuidValue != 0) {
                    uuids.push_back(uuidValue);
                }
            }
        }
    }
}

std::vector<uint64_t> GamePackageBuilder::ExtractDependenciesFromPxa(const std::string& pxaPath) {
    std::vector<uint64_t> dependencies;

    struct PxaHeader {
        uint32_t magic;
        uint32_t version;
        uint16_t assetType;
        uint16_t reserved;
        uint64_t uuid;
        uint64_t metadataSize;
        uint64_t dataSize;
        uint64_t importTimestamp;
        uint64_t sourceTimestamp;
    };

    std::ifstream pxaFile{pxaPath, std::ios::binary};
    if (!pxaFile.is_open()) {
        return dependencies;
    }

    PxaHeader header{};
    pxaFile.read(reinterpret_cast<char*>(&header), sizeof(PxaHeader));

    if (!pxaFile || header.magic != 0x41585850) {
        pxaFile.close();
        return dependencies;
    }

    pxaFile.seekg(static_cast<std::streamoff>(header.metadataSize), std::ios::cur);

    std::string dataStr(static_cast<size_t>(header.dataSize), '\0');
    pxaFile.read(&dataStr[0], static_cast<std::streamsize>(header.dataSize));

    pxaFile.close();

    try {
        nlohmann::json assetData = nlohmann::json::parse(dataStr);

        if (assetData.contains("states") && assetData["states"].is_array()) {
            for (const auto& state : assetData["states"]) {
                if (state.contains("animationClipUUID")) {
                    uint64_t uuid = state["animationClipUUID"].get<uint64_t>();
                    if (uuid != 0) {
                        dependencies.push_back(uuid);
                    }
                }
            }
        }

        if (assetData.contains("spriteSheetUUID")) {
            uint64_t uuid = assetData["spriteSheetUUID"].get<uint64_t>();
            if (uuid != 0) {
                dependencies.push_back(uuid);
            }
        }

        if (assetData.contains("textureUUID")) {
            uint64_t uuid = assetData["textureUUID"].get<uint64_t>();
            if (uuid != 0) {
                dependencies.push_back(uuid);
            }
        }
    } catch (const nlohmann::json::exception&) {
    }

    return dependencies;
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
            PX_LOG_INFO(BUILD, "Loaded UUID cache with %u entries", count);
        }
    } else {
        PX_LOG_WARNING(BUILD, "UUID cache not found: %s", cachePath.c_str());
    }

    std::vector<uint64_t> uuidsToProcess;
    for (const std::pair<std::string, nlohmann::json>& scenePair : package.GetScenes()) {
        const nlohmann::json& sceneData = scenePair.second;
        if (sceneData.contains("entities") && sceneData["entities"].is_array()) {
            for (const nlohmann::json& entity : sceneData["entities"]) {
                for (auto it = entity.begin(); it != entity.end(); ++it) {
                    if (it.value().is_object()) {
                        CollectUUIDsFromComponent(it.value(), uuidsToProcess);
                    }
                }
            }
        }
    }

    PX_LOG_INFO(BUILD, "Scanning ALL .pxa files to build complete UUID registry...");

    struct PxaHeader {
        uint32_t magic;
        uint32_t version;
        uint16_t assetType;
        uint16_t reserved;
        uint64_t uuid;
        uint64_t metadataSize;
        uint64_t dataSize;
        uint64_t importTimestamp;
        uint64_t sourceTimestamp;
    };

    std::string contentPath = (basePath / "content").string();
    if (std::filesystem::exists(contentPath)) {
        try {
            for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(contentPath, std::filesystem::directory_options::skip_permission_denied)) {
                if (!entry.is_regular_file() || entry.path().extension() != ".pxa") {
                    continue;
                }

                std::ifstream pxaFile{entry.path(), std::ios::binary};
                if (!pxaFile.is_open()) {
                    continue;
                }

                PxaHeader header{};
                pxaFile.read(reinterpret_cast<char*>(&header), sizeof(PxaHeader));

                if (!pxaFile || header.magic != 0x41585850) {
                    pxaFile.close();
                    continue;
                }

                if (header.metadataSize > 10000) {
                    pxaFile.close();
                    PX_LOG_WARNING(BUILD, "Invalid metadata size: %s", entry.path().string().c_str());
                    continue;
                }

                std::string metadataStr(static_cast<size_t>(header.metadataSize), '\0');
                pxaFile.read(&metadataStr[0], static_cast<std::streamsize>(header.metadataSize));

                if (!pxaFile) {
                    pxaFile.close();
                    continue;
                }

                size_t pos1 = metadataStr.find('|');
                size_t pos2 = metadataStr.find('|', pos1 + 1);

                if (pos1 != std::string::npos && pos2 != std::string::npos) {
                    std::string extension = metadataStr.substr(pos1 + 1, pos2 - pos1 - 1);

                    std::filesystem::path pxaPath = entry.path();
                    std::string sourcePath = pxaPath.parent_path().string();
                    if (!sourcePath.empty()) sourcePath += "/";
                    sourcePath += pxaPath.stem().string() + extension;
                    std::replace(sourcePath.begin(), sourcePath.end(), '\\', '/');

                    uuidToPath[header.uuid] = sourcePath;
                    PX_LOG_INFO(BUILD, "Registered UUID %" PRIu64 " -> %s", header.uuid, sourcePath.c_str());
                }

                pxaFile.close();
            }
        } catch (const std::filesystem::filesystem_error& e) {
            PX_LOG_ERROR(BUILD, "Filesystem error: %s", e.what());
        }
    }

    PX_LOG_INFO(BUILD, "UUID registry contains %llu entries", static_cast<unsigned long long>(uuidToPath.size()));

    std::vector<uint64_t> processingQueue = uuidsToProcess;
    size_t queueIndex = 0;

    while (queueIndex < processingQueue.size()) {
        uint64_t currentUUID = processingQueue[queueIndex++];

        if (collectedUUIDs.find(currentUUID) != collectedUUIDs.end()) {
            continue;
        }

        auto it = uuidToPath.find(currentUUID);
        if (it == uuidToPath.end()) {
            PX_LOG_WARNING(BUILD, "Asset not found in registry: %" PRIu64, currentUUID);
            continue;
        }

        std::string sourcePath = it->second;
        std::string pxaPath = sourcePath.substr(0, sourcePath.find_last_of('.')) + ".pxa";
        std::string fullPath = (basePath / pxaPath).string();

        if (!std::filesystem::exists(fullPath)) {
            PX_LOG_WARNING(BUILD, "Asset file not found: %s (UUID: %" PRIu64 ")", fullPath.c_str(), currentUUID);
            continue;
        }

        AssetData asset = LoadAssetFile(fullPath, "asset");
        if (asset.data.empty()) {
            continue;
        }

        std::string pathInPackage = pxaPath;
        for (char& c : pathInPackage) {
            if (c == '\\') c = '/';
        }
        asset.path = pathInPackage;
        package.AddAsset(asset);
        collectedUUIDs.insert(currentUUID);
        PX_LOG_INFO(BUILD, "Auto-collected asset: %s (UUID: %" PRIu64 ")", asset.path.c_str(), currentUUID);

        std::vector<uint64_t> dependencies = ExtractDependenciesFromPxa(fullPath);
        for (uint64_t depUUID : dependencies) {
            if (collectedUUIDs.find(depUUID) == collectedUUIDs.end()) {
                bool alreadyQueued = false;
                for (size_t i = queueIndex; i < processingQueue.size(); ++i) {
                    if (processingQueue[i] == depUUID) {
                        alreadyQueued = true;
                        break;
                    }
                }
                if (!alreadyQueued) {
                    processingQueue.push_back(depUUID);
                    PX_LOG_INFO(BUILD, "Found dependency: %" PRIu64, depUUID);
                }
            }
        }
    }

    PX_LOG_INFO(BUILD, "Collected %zu unique assets from scenes (recursive)", collectedUUIDs.size());
}

AssetData GamePackageBuilder::LoadAssetFile(const std::string& filepath, const std::string& type) {
    AssetData asset{};
    asset.type = type;

    std::ifstream file{filepath, std::ios::binary | std::ios::ate};
    if (!file.is_open()) {
        PX_LOG_ERROR(BUILD, "Failed to open asset file: %s", filepath.c_str());
        return asset;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    asset.data.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(asset.data.data()), size)) {
        PX_LOG_ERROR(BUILD, "Failed to read asset file: %s", filepath.c_str());
        asset.data.clear();
    }

    file.close();
    return asset;
}

} // namespace PiiXeL
