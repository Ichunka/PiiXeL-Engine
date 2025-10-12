#include "Resources/AssetImporter.hpp"
#include "Resources/TextureAsset.hpp"
#include "Resources/AudioAsset.hpp"
#include <raylib.h>
#include <fstream>
#include <chrono>
#include <algorithm>

namespace PiiXeL {

AssetImporter::ImportResult AssetImporter::ImportAsset(const std::string& sourcePath, bool forceReimport) {
    ImportResult result{};

    if (!std::filesystem::exists(sourcePath)) {
        result.errorMessage = "Source file does not exist: " + sourcePath;
        TraceLog(LOG_ERROR, "%s", result.errorMessage.c_str());
        return result;
    }

    if (!forceReimport && AssetPackage::PackageExists(sourcePath)) {
        if (!AssetPackage::NeedsReimport(sourcePath)) {
            TraceLog(LOG_INFO, "Asset is up to date: %s", sourcePath.c_str());
            result.success = true;
            result.packagePath = AssetPackage::GetPackagePath(sourcePath);
            return result;
        }
    }

    AssetType type = DetectAssetType(sourcePath);
    if (type == AssetType::Unknown) {
        result.errorMessage = "Unknown asset type: " + sourcePath;
        TraceLog(LOG_WARNING, "%s", result.errorMessage.c_str());
        return result;
    }

    UUID uuid = GetOrCreateUUID(sourcePath);

    switch (type) {
        case AssetType::Texture:
            result = ImportTexture(sourcePath, uuid);
            break;
        case AssetType::Audio:
            result = ImportAudio(sourcePath, uuid);
            break;
        default:
            result.errorMessage = "Unsupported asset type";
            break;
    }

    if (result.success) {
        SaveUUIDCache();
    }

    return result;
}

std::vector<AssetImporter::ImportResult> AssetImporter::ImportDirectory(const std::string& directory,
                                                                         bool recursive) {
    std::vector<ImportResult> results;

    if (!std::filesystem::exists(directory)) {
        TraceLog(LOG_ERROR, "Directory does not exist: %s", directory.c_str());
        return results;
    }

    LoadUUIDCache();

    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator{directory}) {
            if (!entry.is_regular_file()) continue;

            std::string path = entry.path().string();
            std::string ext = entry.path().extension().string();

            if (ext == ".pxa") continue;

            AssetType type = DetectAssetType(path);
            if (type != AssetType::Unknown) {
                results.push_back(ImportAsset(path));
            }
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator{directory}) {
            if (!entry.is_regular_file()) continue;

            std::string path = entry.path().string();
            std::string ext = entry.path().extension().string();

            if (ext == ".pxa") continue;

            AssetType type = DetectAssetType(path);
            if (type != AssetType::Unknown) {
                results.push_back(ImportAsset(path));
            }
        }
    }

    SaveUUIDCache();

    int successCount = static_cast<int>(std::count_if(results.begin(), results.end(),
                                                       [](const ImportResult& r) { return r.success; }));

    TraceLog(LOG_INFO, "Imported %d/%d assets from: %s", successCount,
             static_cast<int>(results.size()), directory.c_str());

    return results;
}

AssetType AssetImporter::DetectAssetType(const std::string& path) const {
    std::filesystem::path fsPath{path};
    std::string ext = fsPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
        ext == ".tga" || ext == ".gif") {
        return AssetType::Texture;
    }

    if (ext == ".wav" || ext == ".ogg" || ext == ".mp3" || ext == ".flac") {
        return AssetType::Audio;
    }

    if (ext == ".scene") {
        return AssetType::Scene;
    }

    return AssetType::Unknown;
}

std::string AssetImporter::GetUUIDCacheFilePath() {
    return "datas/.asset_uuid_cache";
}

void AssetImporter::SaveUUIDCache() {
    std::string cachePath = GetUUIDCacheFilePath();
    std::filesystem::create_directories(std::filesystem::path{cachePath}.parent_path());

    std::ofstream file{cachePath, std::ios::binary};
    if (!file.is_open()) {
        TraceLog(LOG_WARNING, "Failed to save UUID cache");
        return;
    }

    uint32_t count = static_cast<uint32_t>(m_UUIDCache.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& [path, uuid] : m_UUIDCache) {
        uint32_t pathLen = static_cast<uint32_t>(path.size());
        file.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
        file.write(path.c_str(), pathLen);

        uint64_t uuidValue = uuid.Get();
        file.write(reinterpret_cast<const char*>(&uuidValue), sizeof(uuidValue));
    }

    file.close();
}

void AssetImporter::LoadUUIDCache() {
    std::string cachePath = GetUUIDCacheFilePath();

    if (!std::filesystem::exists(cachePath)) {
        return;
    }

    std::ifstream file{cachePath, std::ios::binary};
    if (!file.is_open()) {
        return;
    }

    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    m_UUIDCache.clear();
    m_UUIDCache.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t pathLen = 0;
        file.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));

        std::vector<char> pathBuffer(pathLen + 1, '\0');
        file.read(pathBuffer.data(), pathLen);
        std::string path{pathBuffer.data()};

        uint64_t uuidValue = 0;
        file.read(reinterpret_cast<char*>(&uuidValue), sizeof(uuidValue));

        m_UUIDCache[path] = UUID{uuidValue};
    }

    file.close();
    TraceLog(LOG_INFO, "Loaded UUID cache: %u entries", count);
}

AssetImporter::ImportResult AssetImporter::ImportTexture(const std::string& sourcePath, UUID uuid) {
    ImportResult result{};
    result.uuid = uuid;

    std::vector<uint8_t> data = TextureAsset::EncodeToMemory(sourcePath);
    if (data.empty()) {
        result.errorMessage = "Failed to encode texture";
        return result;
    }

    AssetMetadata metadata{};
    metadata.uuid = uuid;
    metadata.type = AssetType::Texture;
    metadata.name = std::filesystem::path{sourcePath}.stem().string();
    metadata.sourceFile = sourcePath;
    metadata.importTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    auto fileTime = GetFileLastWriteTime(sourcePath);
    metadata.sourceTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        fileTime.time_since_epoch()).count();

    std::string packagePath = AssetPackage::GetPackagePath(sourcePath);
    AssetPackage package{};

    if (!package.SaveToFile(packagePath, metadata, data.data(), data.size())) {
        result.errorMessage = "Failed to save asset package";
        return result;
    }

    result.success = true;
    result.packagePath = packagePath;
    TraceLog(LOG_INFO, "Imported texture: %s -> %s", sourcePath.c_str(), packagePath.c_str());

    return result;
}

AssetImporter::ImportResult AssetImporter::ImportAudio(const std::string& sourcePath, UUID uuid) {
    ImportResult result{};
    result.uuid = uuid;

    std::vector<uint8_t> data = AudioAsset::EncodeToMemory(sourcePath);
    if (data.empty()) {
        result.errorMessage = "Failed to encode audio";
        return result;
    }

    AssetMetadata metadata{};
    metadata.uuid = uuid;
    metadata.type = AssetType::Audio;
    metadata.name = std::filesystem::path{sourcePath}.stem().string();
    metadata.sourceFile = sourcePath;
    metadata.importTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    auto fileTime = GetFileLastWriteTime(sourcePath);
    metadata.sourceTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        fileTime.time_since_epoch()).count();

    std::string packagePath = AssetPackage::GetPackagePath(sourcePath);
    AssetPackage package{};

    if (!package.SaveToFile(packagePath, metadata, data.data(), data.size())) {
        result.errorMessage = "Failed to save asset package";
        return result;
    }

    result.success = true;
    result.packagePath = packagePath;
    TraceLog(LOG_INFO, "Imported audio: %s -> %s", sourcePath.c_str(), packagePath.c_str());

    return result;
}

UUID AssetImporter::GetOrCreateUUID(const std::string& sourcePath) {
    auto it = m_UUIDCache.find(sourcePath);
    if (it != m_UUIDCache.end()) {
        return it->second;
    }

    UUID newUUID{};
    m_UUIDCache[sourcePath] = newUUID;
    return newUUID;
}

std::chrono::system_clock::time_point AssetImporter::GetFileLastWriteTime(const std::string& path) {
    try {
        auto ftime = std::filesystem::last_write_time(path);
        return std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
    } catch (...) {
        return std::chrono::system_clock::now();
    }
}

} // namespace PiiXeL
