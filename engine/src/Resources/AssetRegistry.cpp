#include "Resources/AssetRegistry.hpp"
#include "Resources/TextureAsset.hpp"
#include "Resources/AudioAsset.hpp"
#include <raylib.h>
#include <cinttypes>
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

    std::string packagePath = AssetPackage::GetPackagePath(pathIt->second);
    return LoadAssetFromPackage(packagePath);
}

std::shared_ptr<Asset> AssetRegistry::LoadAssetFromPath(const std::string& path) {
    auto it = m_PathToUUID.find(path);
    if (it != m_PathToUUID.end()) {
        return LoadAsset(it->second);
    }

    auto result = m_Importer.ImportAsset(path);
    if (!result.success) {
        TraceLog(LOG_ERROR, "Failed to import asset: %s", path.c_str());
        return nullptr;
    }

    m_PathToUUID[path] = result.uuid;
    m_UUIDToPath[result.uuid] = path;

    return LoadAssetFromPackage(result.packagePath);
}

std::shared_ptr<Asset> AssetRegistry::GetAsset(UUID uuid) {
    auto it = m_Assets.find(uuid);
    if (it != m_Assets.end()) {
        return it->second;
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
                m_PathToUUID[metadata.sourceFile] = metadata.uuid;
                m_UUIDToPath[metadata.uuid] = metadata.sourceFile;
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
    std::string cachePath = "datas/.asset_uuid_cache";
    if (!std::filesystem::exists(cachePath)) {
        TraceLog(LOG_WARNING, "UUID cache not found at: %s", cachePath.c_str());
        return;
    }

    std::ifstream cacheFile{cachePath, std::ios::binary};
    if (!cacheFile.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open UUID cache: %s", cachePath.c_str());
        return;
    }

    uint32_t count = 0;
    cacheFile.read(reinterpret_cast<char*>(&count), sizeof(count));

    size_t registeredCount = 0;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t pathLen = 0;
        cacheFile.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));

        std::string sourcePath(pathLen, '\0');
        cacheFile.read(&sourcePath[0], pathLen);

        uint64_t uuidValue = 0;
        cacheFile.read(reinterpret_cast<char*>(&uuidValue), sizeof(uuidValue));

        UUID uuid{uuidValue};
        m_UUIDToPath[uuid] = sourcePath;
        m_PathToUUID[sourcePath] = uuid;
        registeredCount++;
    }

    cacheFile.close();
    TraceLog(LOG_INFO, "Registered %zu assets from UUID cache", registeredCount);
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
            m_UUIDToPath[metadata.uuid] = metadata.sourceFile;
            m_PathToUUID[metadata.sourceFile] = metadata.uuid;
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
    auto it = m_PathToUUID.find(path);
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
        default:
            TraceLog(LOG_ERROR, "Unsupported asset type: %d", static_cast<int>(type));
            return nullptr;
    }
}

std::shared_ptr<Asset> AssetRegistry::LoadAssetFromPackage(const std::string& packagePath) {
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

    asset->SetMetadata(metadata);

    if (!asset->Load(data.data(), data.size())) {
        TraceLog(LOG_ERROR, "Failed to load asset data: %s", metadata.name.c_str());
        return nullptr;
    }

    m_Assets[metadata.uuid] = asset;
    m_UUIDToPath[metadata.uuid] = metadata.sourceFile;
    m_PathToUUID[metadata.sourceFile] = metadata.uuid;

    TraceLog(LOG_INFO, "Loaded asset: %s (UUID: %" PRIu64 ")", metadata.name.c_str(), metadata.uuid.Get());
    return asset;
}

} // namespace PiiXeL
