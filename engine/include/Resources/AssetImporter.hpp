#ifndef PIIXELENGINE_ASSETIMPORTER_HPP
#define PIIXELENGINE_ASSETIMPORTER_HPP

#include "Resources/Asset.hpp"
#include "Resources/AssetPackage.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

namespace PiiXeL {

class AssetImporter {
public:
    struct ImportResult {
        bool success{false};
        UUID uuid{};
        std::string packagePath;
        std::string errorMessage;
    };

    AssetImporter() = default;
    ~AssetImporter() = default;

    ImportResult ImportAsset(const std::string& sourcePath, bool forceReimport = false);
    std::vector<ImportResult> ImportDirectory(const std::string& directory, bool recursive = true);

    [[nodiscard]] AssetType DetectAssetType(const std::string& path) const;
    [[nodiscard]] static std::string GetUUIDCacheFilePath();

    void SaveUUIDCache();
    void LoadUUIDCache();
    void ForceUUID(const std::string& sourcePath, UUID uuid);

private:
    ImportResult ImportTexture(const std::string& sourcePath, UUID uuid);
    ImportResult ImportAudio(const std::string& sourcePath, UUID uuid);
    ImportResult ImportSpriteSheet(const std::string& sourcePath, UUID uuid);
    ImportResult ImportAnimationClip(const std::string& sourcePath, UUID uuid);
    ImportResult ImportAnimatorController(const std::string& sourcePath, UUID uuid);

    UUID GetOrCreateUUID(const std::string& sourcePath);
    std::chrono::system_clock::time_point GetFileLastWriteTime(const std::string& path);

    std::unordered_map<std::string, UUID> m_UUIDCache;
};

} // namespace PiiXeL

#endif
