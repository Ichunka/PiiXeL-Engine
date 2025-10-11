#ifndef PIIXELENGINE_GAMEPACKAGE_HPP
#define PIIXELENGINE_GAMEPACKAGE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace PiiXeL {

struct AssetData {
    std::string path;
    std::string type;
    std::vector<unsigned char> data;
};

struct GamePackageHeader {
    uint32_t version{1};
    uint32_t sceneCount{0};
    uint32_t assetCount{0};
    std::string engineVersion{"0.1.0"};
};

class GamePackage {
public:
    GamePackage();
    ~GamePackage();

    bool SaveToFile(const std::string& filepath);
    bool LoadFromFile(const std::string& filepath);

    void SetHeader(const GamePackageHeader& header);
    void AddScene(const std::string& name, const nlohmann::json& sceneData);
    void AddAsset(const AssetData& asset);
    void SetConfig(const nlohmann::json& config);

    [[nodiscard]] const GamePackageHeader& GetHeader() const { return m_Header; }
    [[nodiscard]] const std::vector<std::pair<std::string, nlohmann::json>>& GetScenes() const { return m_Scenes; }
    [[nodiscard]] const std::vector<AssetData>& GetAssets() const { return m_Assets; }
    [[nodiscard]] const nlohmann::json& GetConfig() const { return m_Config; }
    [[nodiscard]] const AssetData* GetAsset(const std::string& path) const;

private:
    GamePackageHeader m_Header;
    std::vector<std::pair<std::string, nlohmann::json>> m_Scenes;
    std::vector<AssetData> m_Assets;
    nlohmann::json m_Config;
    std::unordered_map<std::string, size_t> m_AssetIndexMap;
};

} // namespace PiiXeL

#endif // PIIXELENGINE_GAMEPACKAGE_HPP
