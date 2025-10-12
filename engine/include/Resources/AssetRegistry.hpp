#ifndef PIIXELENGINE_ASSETREGISTRY_HPP
#define PIIXELENGINE_ASSETREGISTRY_HPP

#include "Resources/Asset.hpp"
#include "Resources/AssetPackage.hpp"
#include "Resources/AssetImporter.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace PiiXeL {

class AssetRegistry {
public:
    AssetRegistry();
    ~AssetRegistry();

    AssetRegistry(const AssetRegistry&) = delete;
    AssetRegistry& operator=(const AssetRegistry&) = delete;

    void Initialize();
    void Shutdown();

    std::shared_ptr<Asset> LoadAsset(UUID uuid);
    std::shared_ptr<Asset> LoadAssetFromPath(const std::string& path);
    std::shared_ptr<Asset> GetAsset(UUID uuid);

    void UnloadAsset(UUID uuid);
    void UnloadAll();

    void ImportDirectory(const std::string& directory);
    void ReimportAsset(const std::string& sourcePath);
    void RegisterExtractedAssets();

    [[nodiscard]] bool IsAssetLoaded(UUID uuid) const;
    [[nodiscard]] UUID GetUUIDFromPath(const std::string& path) const;
    [[nodiscard]] std::string GetPathFromUUID(UUID uuid) const;

    [[nodiscard]] std::vector<AssetMetadata> GetAllAssetMetadata() const;
    [[nodiscard]] const std::unordered_map<UUID, std::shared_ptr<Asset>>& GetAllAssets() const;
    [[nodiscard]] size_t GetTotalMemoryUsage() const;

    static AssetRegistry& Instance();

private:
    std::shared_ptr<Asset> CreateAsset(AssetType type, UUID uuid, const std::string& name);
    std::shared_ptr<Asset> LoadAssetFromPackage(const std::string& packagePath);

    std::unordered_map<UUID, std::shared_ptr<Asset>> m_Assets;
    std::unordered_map<UUID, std::string> m_UUIDToPath;
    std::unordered_map<std::string, UUID> m_PathToUUID;

    AssetImporter m_Importer;
    bool m_IsInitialized{false};
};

} // namespace PiiXeL

#endif
