#ifndef PIIXELENGINE_GAMEPACKAGELOADER_HPP
#define PIIXELENGINE_GAMEPACKAGELOADER_HPP

#include "GamePackage.hpp"
#include <raylib.h>
#include <unordered_map>
#include <memory>

namespace PiiXeL {

class Scene;
class ScriptSystem;

class GamePackageLoader {
public:
    GamePackageLoader();
    ~GamePackageLoader();

    bool LoadPackage(const std::string& filepath);
    std::unique_ptr<Scene> LoadScene(const std::string& sceneName, ScriptSystem* scriptSystem = nullptr);
    Texture2D LoadTexture(const std::string& assetPath);
    void UnloadAllTextures();
    void InitializeAssetRegistry();

    [[nodiscard]] const GamePackage& GetPackage() const { return m_Package; }
    [[nodiscard]] bool IsLoaded() const { return m_IsLoaded; }

private:
    GamePackage m_Package;
    bool m_IsLoaded{false};
    std::unordered_map<std::string, Texture2D> m_LoadedTextures;
};

} // namespace PiiXeL

#endif // PIIXELENGINE_GAMEPACKAGELOADER_HPP
