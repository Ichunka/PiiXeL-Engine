#include "Resources/AssetManager.hpp"

#include <filesystem>
#include <iostream>

namespace PiiXeL {

AssetManager::AssetManager() : m_Textures{}, m_DefaultTexture{}, m_IsShutdown{false} {
    CreateDefaultTexture();
}

AssetManager::~AssetManager() {
    Shutdown();
}

Texture2D AssetManager::LoadTexture(const std::string& path) {
    if (m_Textures.find(path) != m_Textures.end()) {
        return m_Textures[path];
    }

    Texture2D texture = ::LoadTexture(path.c_str());

    if (texture.id == 0) {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
    else {
        m_Textures[path] = texture;
    }

    return texture;
}

void AssetManager::UnloadTexture(const std::string& path) {
    if (m_Textures.find(path) != m_Textures.end()) {
        ::UnloadTexture(m_Textures[path]);
        m_Textures.erase(path);
    }
}

void AssetManager::UnloadAllTextures() {
    for (const std::pair<const std::string, Texture2D>& pair : m_Textures) {
        ::UnloadTexture(pair.second);
    }
    m_Textures.clear();
}

bool AssetManager::HasTexture(const std::string& path) const {
    return m_Textures.find(path) != m_Textures.end();
}

Texture2D AssetManager::GetTexture(const std::string& path) const {
    if (m_Textures.find(path) != m_Textures.end()) {
        return m_Textures.at(path);
    }
    return Texture2D{};
}

AssetManager& AssetManager::Instance() {
    static AssetManager instance;
    return instance;
}

Texture2D AssetManager::GetDefaultTexture() {
    CreateDefaultTexture();
    return m_DefaultTexture;
}

void AssetManager::CreateDefaultTexture() {
    if (m_DefaultTexture.id == 0) {
        Image whiteImage = GenImageColor(64, 64, WHITE);
        m_DefaultTexture = LoadTextureFromImage(whiteImage);
        UnloadImage(whiteImage);
    }
}

void AssetManager::Shutdown() {
    if (m_IsShutdown) {
        return;
    }

    UnloadAllTextures();

    if (m_DefaultTexture.id != 0) {
        ::UnloadTexture(m_DefaultTexture);
        m_DefaultTexture = Texture2D{};
    }

    m_IsShutdown = true;
}

std::vector<AssetInfo> AssetManager::ScanAssetsDirectory(const std::string& directory) {
    std::vector<AssetInfo> assets{};

    if (!std::filesystem::exists(directory)) {
        return assets;
    }

    for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string extension = entry.path().extension().string();
            std::string type{};

            if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" ||
                extension == ".tga")
            {
                type = "texture";
            }
            else if (extension == ".wav" || extension == ".mp3" || extension == ".ogg") {
                type = "audio";
            }
            else {
                continue;
            }

            AssetInfo info{};
            info.path = entry.path().string();
            info.filename = entry.path().filename().string();
            info.extension = extension;
            info.type = type;
            info.fileSize = std::filesystem::file_size(entry.path());

            if (type == "texture") {
                Image img = ::LoadImage(info.path.c_str());
                if (img.data != nullptr) {
                    info.width = img.width;
                    info.height = img.height;
                    UnloadImage(img);
                }
            }

            for (char& c : info.path) {
                if (c == '\\')
                    c = '/';
            }

            assets.push_back(info);
        }
    }

    return assets;
}

AssetInfo AssetManager::GetAssetInfo(const std::string& path) const {
    AssetInfo info{};

    if (!std::filesystem::exists(path)) {
        return info;
    }

    info.path = path;
    info.filename = std::filesystem::path(path).filename().string();
    info.extension = std::filesystem::path(path).extension().string();

    if (info.extension == ".png" || info.extension == ".jpg" || info.extension == ".jpeg" || info.extension == ".bmp") {
        info.type = "texture";
    }
    else if (info.extension == ".wav" || info.extension == ".mp3" || info.extension == ".ogg") {
        info.type = "audio";
    }

    info.fileSize = std::filesystem::file_size(path);

    if (info.type == "texture" && HasTexture(path)) {
        Texture2D tex = GetTexture(path);
        info.width = tex.width;
        info.height = tex.height;
    }

    return info;
}

} // namespace PiiXeL
