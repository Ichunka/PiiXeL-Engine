#ifndef PIIXELENGINE_GAMEPACKAGEBUILDER_HPP
#define PIIXELENGINE_GAMEPACKAGEBUILDER_HPP

#include "GamePackage.hpp"
#include <string>

namespace PiiXeL {

class GamePackageBuilder {
public:
    GamePackageBuilder();
    ~GamePackageBuilder();

    bool BuildFromProject(const std::string& projectPath, const std::string& outputPath);

private:
    void ScanScenes(const std::string& scenesPath, GamePackage& package);
    void ScanAssets(const std::string& assetsPath, GamePackage& package);
    void CollectAssetsFromScenes(GamePackage& package, const std::filesystem::path& basePath);
    AssetData LoadAssetFile(const std::string& filepath, const std::string& type);

    std::vector<uint64_t> ExtractDependenciesFromPxa(const std::string& pxaPath);
    void CollectUUIDsFromComponent(const nlohmann::json& component, std::vector<uint64_t>& uuids);
};

} // namespace PiiXeL

#endif // PIIXELENGINE_GAMEPACKAGEBUILDER_HPP
