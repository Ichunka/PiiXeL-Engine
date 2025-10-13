#include "Resources/AssetRegistry.hpp"
#include "Resources/TextureAsset.hpp"
#include "Resources/AudioAsset.hpp"
#include "Animation/SpriteSheet.hpp"
#include "Animation/AnimationClip.hpp"
#include "Animation/AnimatorController.hpp"
#include <raylib.h>
#include <cinttypes>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace PiiXeL {

AssetRegistry::AssetRegistry() = default;

AssetRegistry::~AssetRegistry() {
    Shutdown();
}

void AssetRegistry::Initialize() {
    if (m_IsInitialized) {
        return;
    }

    m_Importer.LoadUUIDCache();
    m_IsInitialized = true;

    TraceLog(LOG_INFO, "AssetRegistry initialized");
}

void AssetRegistry::Shutdown() {
    if (!m_IsInitialized) {
        return;
    }

    UnloadAll();
    m_Importer.SaveUUIDCache();
    m_IsInitialized = false;

    TraceLog(LOG_INFO, "AssetRegistry shutdown");
}

std::shared_ptr<Asset> AssetRegistry::LoadAsset(UUID uuid) {
    auto it = m_Assets.find(uuid);
    if (it != m_Assets.end()) {
        if (it->second->IsLoaded()) {
            return it->second;
        }
    }

    auto pathIt = m_UUIDToPath.find(uuid);
    if (pathIt == m_UUIDToPath.end()) {
        TraceLog(LOG_WARNING, "Asset not found in registry: %" PRIu64, uuid.Get());
        return nullptr;
    }

    auto cacheIt = m_PackageDataCache.find(uuid);
    if (cacheIt != m_PackageDataCache.end()) {
        AssetMetadata metadata{};
        std::vector<uint8_t> data{};

        AssetPackage package{};
        if (package.LoadFromMemory(cacheIt->second.data(), cacheIt->second.size(), metadata, data)) {
            auto asset = CreateAsset(metadata.type, metadata.uuid, metadata.name);
            if (asset) {
                metadata.sourceFile = pathIt->second;
                asset->SetMetadata(metadata);
                if (asset->Load(data.data(), data.size())) {
                    m_Assets[metadata.uuid] = asset;
                    return asset;
                }
            }
        }
    }

    std::string packagePath = AssetPackage::GetPackagePath(pathIt->second);
    return LoadAssetFromPackage(packagePath, pathIt->second);
}

std::shared_ptr<Asset> AssetRegistry::LoadAssetFromPath(const std::string& path) {
    std::string normalizedPath = path;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

    auto it = m_PathToUUID.find(normalizedPath);
    if (it != m_PathToUUID.end()) {
        return LoadAsset(it->second);
    }

    auto result = m_Importer.ImportAsset(path);
    if (!result.success) {
        TraceLog(LOG_ERROR, "Failed to import asset: %s", path.c_str());
        return nullptr;
    }

    m_PathToUUID[normalizedPath] = result.uuid;
    m_UUIDToPath[result.uuid] = normalizedPath;

    return LoadAssetFromPackage(result.packagePath, normalizedPath);
}

std::shared_ptr<Asset> AssetRegistry::GetAsset(UUID uuid) {
    auto it = m_Assets.find(uuid);
    if (it != m_Assets.end()) {
        return it->second;
    }

    auto pathIt = m_UUIDToPath.find(uuid);
    if (pathIt != m_UUIDToPath.end()) {
        return LoadAsset(uuid);
    }

    return nullptr;
}

void AssetRegistry::UnloadAsset(UUID uuid) {
    auto it = m_Assets.find(uuid);
    if (it != m_Assets.end()) {
        it->second->Unload();
        m_Assets.erase(it);
    }
}

void AssetRegistry::UnloadAll() {
    for (auto& [uuid, asset] : m_Assets) {
        asset->Unload();
    }
    m_Assets.clear();
}

void AssetRegistry::ImportDirectory(const std::string& directory) {
    auto results = m_Importer.ImportDirectory(directory, true);

    for (const auto& result : results) {
        if (result.success) {
            AssetMetadata metadata{};
            std::vector<uint8_t> data{};

            AssetPackage package{};
            if (package.LoadFromFile(result.packagePath, metadata, data)) {
                std::string normalizedPath = metadata.sourceFile;
                std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
                m_PathToUUID[normalizedPath] = metadata.uuid;
                m_UUIDToPath[metadata.uuid] = normalizedPath;
            }
        }
    }

    TraceLog(LOG_INFO, "Imported directory: %s", directory.c_str());
}

void AssetRegistry::ReimportAsset(const std::string& sourcePath) {
    auto result = m_Importer.ImportAsset(sourcePath, true);

    if (result.success) {
        auto it = m_Assets.find(result.uuid);
        if (it != m_Assets.end()) {
            it->second->Unload();
            m_Assets.erase(it);
        }

        TraceLog(LOG_INFO, "Reimported asset: %s", sourcePath.c_str());
    }
}

void AssetRegistry::RegisterExtractedAssets() {
    TraceLog(LOG_INFO, "Scanning for .pxa assets in content/...");

    std::vector<std::filesystem::path> pxaFiles;
    try {
        std::filesystem::path contentPath = "content";
        if (std::filesystem::exists(contentPath)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(contentPath)) {
                if (entry.is_regular_file() && entry.path().extension() == ".pxa") {
                    pxaFiles.push_back(entry.path());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        TraceLog(LOG_ERROR, "Failed to scan content directory: %s", e.what());
    }

    size_t registeredCount = 0;
    for (const auto& pxaPath : pxaFiles) {
        AssetMetadata metadata{};
        AssetPackage package{};

        if (package.LoadMetadataOnly(pxaPath.string(), metadata)) {
            std::string normalizedPath = metadata.sourceFile;
            std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

            m_UUIDToPath[metadata.uuid] = normalizedPath;
            m_PathToUUID[normalizedPath] = metadata.uuid;
            registeredCount++;
        }
    }

    TraceLog(LOG_INFO, "Registered %zu assets from .pxa files", registeredCount);
}

void AssetRegistry::ScanAllPxaFiles(const std::string& rootPath, ProgressCallback callback) {
    namespace fs = std::filesystem;

    std::vector<fs::path> pxaFiles{};

    try {
        for (const auto& entry : fs::recursive_directory_iterator(rootPath, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file() && entry.path().extension() == ".pxa") {
                pxaFiles.push_back(entry.path());
            }
        }
    } catch (const fs::filesystem_error& e) {
        TraceLog(LOG_ERROR, "Filesystem error during scan: %s", e.what());
        return;
    }

    size_t total = pxaFiles.size();
    size_t current = 0;

    for (const auto& pxaPath : pxaFiles) {
        AssetMetadata metadata{};

        AssetPackage package{};
        if (package.LoadMetadataOnly(pxaPath.string(), metadata)) {
            std::string normalizedPath = metadata.sourceFile;
            std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
            m_UUIDToPath[metadata.uuid] = normalizedPath;
            m_PathToUUID[normalizedPath] = metadata.uuid;
        }

        current++;
        if (callback) {
            callback(current, total, pxaPath.string());
        }
    }

    TraceLog(LOG_INFO, "Scanned %zu .pxa files from %s", total, rootPath.c_str());
}

bool AssetRegistry::IsAssetLoaded(UUID uuid) const {
    auto it = m_Assets.find(uuid);
    return it != m_Assets.end() && it->second->IsLoaded();
}

UUID AssetRegistry::GetUUIDFromPath(const std::string& path) const {
    std::string normalizedPath = path;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

    auto it = m_PathToUUID.find(normalizedPath);
    if (it != m_PathToUUID.end()) {
        return it->second;
    }
    return UUID{0};
}

std::string AssetRegistry::GetPathFromUUID(UUID uuid) const {
    auto it = m_UUIDToPath.find(uuid);
    if (it != m_UUIDToPath.end()) {
        return it->second;
    }
    return "";
}

std::vector<AssetMetadata> AssetRegistry::GetAllAssetMetadata() const {
    std::vector<AssetMetadata> metadata{};
    metadata.reserve(m_Assets.size());

    for (const auto& [uuid, asset] : m_Assets) {
        metadata.push_back(asset->GetMetadata());
    }

    return metadata;
}

const std::unordered_map<UUID, std::shared_ptr<Asset>>& AssetRegistry::GetAllAssets() const {
    return m_Assets;
}

const std::unordered_map<UUID, std::string>& AssetRegistry::GetAllKnownAssetPaths() const {
    return m_UUIDToPath;
}

size_t AssetRegistry::GetTotalMemoryUsage() const {
    size_t total = 0;
    for (const auto& [uuid, asset] : m_Assets) {
        total += asset->GetMemoryUsage();
    }
    return total;
}

AssetRegistry& AssetRegistry::Instance() {
    static AssetRegistry instance{};
    return instance;
}

std::shared_ptr<Asset> AssetRegistry::CreateAsset(AssetType type, UUID uuid, const std::string& name) {
    switch (type) {
        case AssetType::Texture:
            return std::make_shared<TextureAsset>(uuid, name);
        case AssetType::Audio:
            return std::make_shared<AudioAsset>(uuid, name);
        case AssetType::SpriteSheet:
            return std::make_shared<SpriteSheet>(uuid, name);
        case AssetType::AnimationClip:
            return std::make_shared<AnimationClip>(uuid, name);
        case AssetType::AnimatorController:
            return std::make_shared<AnimatorController>(uuid, name);
        default:
            TraceLog(LOG_ERROR, "Unsupported asset type: %d", static_cast<int>(type));
            return nullptr;
    }
}

std::shared_ptr<Asset> AssetRegistry::LoadAssetFromPackage(const std::string& packagePath, const std::string& sourcePath) {
    AssetMetadata metadata{};
    std::vector<uint8_t> data{};

    AssetPackage package{};
    if (!package.LoadFromFile(packagePath, metadata, data)) {
        TraceLog(LOG_ERROR, "Failed to load package: %s", packagePath.c_str());
        return nullptr;
    }

    auto asset = CreateAsset(metadata.type, metadata.uuid, metadata.name);
    if (!asset) {
        return nullptr;
    }

    metadata.sourceFile = sourcePath;
    asset->SetMetadata(metadata);

    if (!asset->Load(data.data(), data.size())) {
        TraceLog(LOG_ERROR, "Failed to load asset data: %s", metadata.name.c_str());
        return nullptr;
    }

    std::string normalizedPath = sourcePath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

    m_Assets[metadata.uuid] = asset;
    m_UUIDToPath[metadata.uuid] = normalizedPath;
    m_PathToUUID[normalizedPath] = metadata.uuid;

    TraceLog(LOG_INFO, "Loaded asset: %s (UUID: %" PRIu64 ")", metadata.name.c_str(), metadata.uuid.Get());
    return asset;
}

void AssetRegistry::LoadUUIDCacheFromMemory(const uint8_t* data, size_t dataSize) {
    if (!data || dataSize < sizeof(uint32_t)) {
        return;
    }

    size_t offset = 0;
    uint32_t count = 0;
    std::memcpy(&count, data + offset, sizeof(count));
    offset += sizeof(count);

    size_t registeredCount = 0;
    for (uint32_t i = 0; i < count; ++i) {
        if (offset + sizeof(uint32_t) > dataSize) break;

        uint32_t pathLen = 0;
        std::memcpy(&pathLen, data + offset, sizeof(pathLen));
        offset += sizeof(pathLen);

        if (offset + pathLen > dataSize) break;

        std::string sourcePath(reinterpret_cast<const char*>(data + offset), pathLen);
        offset += pathLen;

        if (offset + sizeof(uint64_t) > dataSize) break;

        uint64_t uuidValue = 0;
        std::memcpy(&uuidValue, data + offset, sizeof(uuidValue));
        offset += sizeof(uuidValue);

        std::replace(sourcePath.begin(), sourcePath.end(), '\\', '/');

        UUID uuid{uuidValue};
        m_UUIDToPath[uuid] = sourcePath;
        m_PathToUUID[sourcePath] = uuid;
        registeredCount++;
    }

    TraceLog(LOG_INFO, "Loaded UUID cache from memory: %zu entries", registeredCount);
}

void AssetRegistry::RegisterAssetFromMemory(UUID uuid, const std::string& sourcePath,
                                              const std::vector<uint8_t>& packageData) {
    std::string normalizedPath = sourcePath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

    m_PackageDataCache[uuid] = packageData;
    m_UUIDToPath[uuid] = normalizedPath;
    m_PathToUUID[normalizedPath] = uuid;
}

} // namespace PiiXeL
