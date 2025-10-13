#ifndef PIIXELENGINE_ASSETMANAGER_HPP
#define PIIXELENGINE_ASSETMANAGER_HPP

#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "Components/UUID.hpp"

namespace PiiXeL {

struct AssetInfo {
    std::string path;
    std::string filename;
    std::string extension;
    std::string type;
    UUID uuid{0};
    size_t fileSize{0};
    int width{0};
    int height{0};
};

class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    Texture2D LoadTexture(const std::string& path);
    void UnloadTexture(const std::string& path);
    void UnloadAllTextures();
    void Shutdown();

    [[nodiscard]] bool HasTexture(const std::string& path) const;
    [[nodiscard]] Texture2D GetTexture(const std::string& path) const;
    [[nodiscard]] Texture2D GetDefaultTexture();

    std::vector<AssetInfo> ScanAssetsDirectory(const std::string& directory);
    [[nodiscard]] AssetInfo GetAssetInfo(const std::string& path) const;
    [[nodiscard]] const std::unordered_map<std::string, Texture2D>& GetAllTextures() const { return m_Textures; }

    static AssetManager& Instance();

private:
    void CreateDefaultTexture();

private:
    std::unordered_map<std::string, Texture2D> m_Textures;
    Texture2D m_DefaultTexture;
    bool m_IsShutdown{false};
};

} // namespace PiiXeL

#endif // PIIXELENGINE_ASSETMANAGER_HPP
