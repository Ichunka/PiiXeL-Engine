#include "Build/GamePackage.hpp"

#include "Core/Logger.hpp"

#include <fstream>
#include <raylib.h>

namespace PiiXeL {

GamePackage::GamePackage() : m_Header{}, m_Scenes{}, m_Assets{}, m_Config{}, m_AssetIndexMap{} {}

GamePackage::~GamePackage() = default;

bool GamePackage::SaveToFile(const std::string& filepath) {
    nlohmann::json packageJson{};

    packageJson["header"] = {{"version", m_Header.version},
                             {"sceneCount", m_Header.sceneCount},
                             {"assetCount", m_Header.assetCount},
                             {"engineVersion", m_Header.engineVersion}};

    packageJson["scenes"] = nlohmann::json::array();
    for (const std::pair<std::string, nlohmann::json>& scenePair : m_Scenes)
    {
        nlohmann::json sceneEntry{};
        sceneEntry["name"] = scenePair.first;
        sceneEntry["data"] = scenePair.second;
        packageJson["scenes"].push_back(sceneEntry);
    }

    packageJson["assets"] = nlohmann::json::array();
    for (const AssetData& asset : m_Assets)
    {
        nlohmann::json assetEntry{};
        assetEntry["path"] = asset.path;
        assetEntry["type"] = asset.type;
        assetEntry["size"] = asset.data.size();

        std::string base64Data{};
        base64Data.reserve(asset.data.size() * 4 / 3 + 4);

        const char* base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        size_t i = 0;
        for (; i + 2 < asset.data.size(); i += 3)
        {
            uint32_t val = (asset.data[i] << 16) | (asset.data[i + 1] << 8) | asset.data[i + 2];
            base64Data += base64Chars[(val >> 18) & 0x3F];
            base64Data += base64Chars[(val >> 12) & 0x3F];
            base64Data += base64Chars[(val >> 6) & 0x3F];
            base64Data += base64Chars[val & 0x3F];
        }

        if (i < asset.data.size())
        {
            uint32_t val = asset.data[i] << 16;
            if (i + 1 < asset.data.size())
            { val |= asset.data[i + 1] << 8; }
            base64Data += base64Chars[(val >> 18) & 0x3F];
            base64Data += base64Chars[(val >> 12) & 0x3F];
            base64Data += (i + 1 < asset.data.size()) ? base64Chars[(val >> 6) & 0x3F] : '=';
            base64Data += '=';
        }

        assetEntry["data"] = base64Data;
        packageJson["assets"].push_back(assetEntry);
    }

    packageJson["config"] = m_Config;

    std::ofstream file{filepath, std::ios::binary};
    if (!file.is_open())
    {
        PX_LOG_ERROR(BUILD, "Failed to create package file: %s", filepath.c_str());
        return false;
    }

    file << packageJson.dump();
    file.close();

    PX_LOG_INFO(BUILD, "Game package saved: %s", filepath.c_str());
    return true;
}

bool GamePackage::LoadFromFile(const std::string& filepath) {
    std::ifstream file{filepath, std::ios::binary};
    if (!file.is_open())
    {
        PX_LOG_ERROR(BUILD, "Failed to open package file: %s", filepath.c_str());
        return false;
    }

    nlohmann::json packageJson{};
    try
    { file >> packageJson; }
    catch (const nlohmann::json::exception& e)
    {
        PX_LOG_ERROR(BUILD, "Failed to parse package: %s", e.what());
        return false;
    }
    file.close();

    if (packageJson.contains("header"))
    {
        const nlohmann::json& header = packageJson["header"];
        m_Header.version = header.value("version", 1u);
        m_Header.sceneCount = header.value("sceneCount", 0u);
        m_Header.assetCount = header.value("assetCount", 0u);
        m_Header.engineVersion = header.value("engineVersion", "0.1.0");
    }

    if (packageJson.contains("scenes") && packageJson["scenes"].is_array())
    {
        for (const nlohmann::json& sceneEntry : packageJson["scenes"])
        {
            std::string name = sceneEntry.value("name", "Unnamed Scene");
            nlohmann::json sceneData = sceneEntry.value("data", nlohmann::json{});
            m_Scenes.push_back({name, sceneData});
        }
    }

    if (packageJson.contains("assets") && packageJson["assets"].is_array())
    {
        for (const nlohmann::json& assetEntry : packageJson["assets"])
        {
            AssetData asset{};
            asset.path = assetEntry.value("path", "");
            asset.type = assetEntry.value("type", "");

            std::string base64Data = assetEntry.value("data", "");
            if (!base64Data.empty())
            {
                const char* base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
                std::vector<int> decodeTable(256, -1);
                for (int i = 0; i < 64; ++i)
                { decodeTable[static_cast<unsigned char>(base64Chars[i])] = i; }

                asset.data.reserve(base64Data.size() * 3 / 4);
                for (size_t i = 0; i + 3 < base64Data.size(); i += 4)
                {
                    int val0 = decodeTable[static_cast<unsigned char>(base64Data[i])];
                    int val1 = decodeTable[static_cast<unsigned char>(base64Data[i + 1])];
                    int val2 = decodeTable[static_cast<unsigned char>(base64Data[i + 2])];
                    int val3 = decodeTable[static_cast<unsigned char>(base64Data[i + 3])];

                    if (val0 == -1 || val1 == -1)
                        break;

                    asset.data.push_back(static_cast<unsigned char>((val0 << 2) | (val1 >> 4)));
                    if (val2 != -1)
                    {
                        asset.data.push_back(static_cast<unsigned char>((val1 << 4) | (val2 >> 2)));
                        if (val3 != -1)
                        { asset.data.push_back(static_cast<unsigned char>((val2 << 6) | val3)); }
                    }
                }
            }

            m_AssetIndexMap[asset.path] = m_Assets.size();
            m_Assets.push_back(asset);
        }
    }

    if (packageJson.contains("config"))
    { m_Config = packageJson["config"]; }

    PX_LOG_INFO(BUILD, "Game package loaded: %s (%llu scenes, %llu assets)", filepath.c_str(),
                static_cast<unsigned long long>(m_Scenes.size()), static_cast<unsigned long long>(m_Assets.size()));
    return true;
}

void GamePackage::SetHeader(const GamePackageHeader& header) {
    m_Header = header;
}

void GamePackage::AddScene(const std::string& name, const nlohmann::json& sceneData) {
    m_Scenes.push_back({name, sceneData});
    m_Header.sceneCount = static_cast<uint32_t>(m_Scenes.size());
}

void GamePackage::AddAsset(const AssetData& asset) {
    m_AssetIndexMap[asset.path] = m_Assets.size();
    m_Assets.push_back(asset);
    m_Header.assetCount = static_cast<uint32_t>(m_Assets.size());
}

void GamePackage::SetConfig(const nlohmann::json& config) {
    m_Config = config;
}

const AssetData* GamePackage::GetAsset(const std::string& path) const {
    auto it = m_AssetIndexMap.find(path);
    if (it != m_AssetIndexMap.end())
    { return &m_Assets[it->second]; }
    return nullptr;
}

} // namespace PiiXeL
