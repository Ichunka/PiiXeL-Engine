#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <algorithm>

struct UUIDEntry {
    std::string sourcePath;
    uint64_t uuid;
};

int main(int argc, char** argv) {
    std::string projectPath = ".";
    if (argc > 1) {
        projectPath = argv[1];
    }

    std::filesystem::path contentPath = std::filesystem::path(projectPath) / "content";
    if (!std::filesystem::exists(contentPath)) {
        std::cerr << "Content directory not found: " << contentPath << std::endl;
        return 1;
    }

    std::vector<UUIDEntry> entries;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(contentPath)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".pxa") {
            continue;
        }

        std::ifstream pxaFile{entry.path(), std::ios::binary};
        if (!pxaFile.is_open()) continue;

        uint32_t magic = 0;
        pxaFile.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != 0x41585850) {
            pxaFile.close();
            continue;
        }

        uint32_t version = 0;
        pxaFile.read(reinterpret_cast<char*>(&version), sizeof(version));

        uint16_t assetType = 0;
        pxaFile.read(reinterpret_cast<char*>(&assetType), sizeof(assetType));

        uint16_t reserved = 0;
        pxaFile.read(reinterpret_cast<char*>(&reserved), sizeof(reserved));

        uint64_t uuidValue = 0;
        pxaFile.read(reinterpret_cast<char*>(&uuidValue), sizeof(uuidValue));

        uint64_t metadataSize = 0;
        pxaFile.read(reinterpret_cast<char*>(&metadataSize), sizeof(metadataSize));

        pxaFile.seekg(sizeof(uint64_t) * 2, std::ios::cur);

        std::string metadataStr(static_cast<size_t>(metadataSize), '\0');
        pxaFile.read(&metadataStr[0], static_cast<std::streamsize>(metadataSize));

        size_t pos1 = metadataStr.find('|');
        size_t pos2 = metadataStr.find('|', pos1 + 1);

        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            std::string sourcePath = metadataStr.substr(pos1 + 1, pos2 - pos1 - 1);
            std::replace(sourcePath.begin(), sourcePath.end(), '\\', '/');

            UUIDEntry uuidEntry;
            uuidEntry.sourcePath = sourcePath;
            uuidEntry.uuid = uuidValue;
            entries.push_back(uuidEntry);

            std::cout << "Found: " << sourcePath << " -> " << uuidValue << std::endl;
        }

        pxaFile.close();
    }

    std::filesystem::path cachePath = std::filesystem::path(projectPath) / "datas" / ".asset_uuid_cache";
    std::filesystem::create_directories(cachePath.parent_path());

    std::ifstream existingCache{cachePath, std::ios::binary};
    if (existingCache.is_open()) {
        uint32_t count = 0;
        existingCache.read(reinterpret_cast<char*>(&count), sizeof(count));

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t pathLen = 0;
            existingCache.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));

            std::string path(pathLen, '\0');
            existingCache.read(&path[0], pathLen);

            uint64_t uuidValue = 0;
            existingCache.read(reinterpret_cast<char*>(&uuidValue), sizeof(uuidValue));

            std::replace(path.begin(), path.end(), '\\', '/');

            bool isAsset = false;
            for (const auto& entry : entries) {
                if (entry.sourcePath == path) {
                    isAsset = true;
                    break;
                }
            }

            if (!isAsset && path.find(".scene") != std::string::npos) {
                UUIDEntry sceneEntry;
                sceneEntry.sourcePath = path;
                sceneEntry.uuid = uuidValue;
                entries.push_back(sceneEntry);
                std::cout << "Preserved scene: " << path << " -> " << uuidValue << std::endl;
            }
        }
        existingCache.close();
    }

    std::ofstream cacheFile{cachePath, std::ios::binary};
    if (!cacheFile.is_open()) {
        std::cerr << "Failed to write UUID cache" << std::endl;
        return 1;
    }

    uint32_t count = static_cast<uint32_t>(entries.size());
    cacheFile.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& entry : entries) {
        uint32_t pathLen = static_cast<uint32_t>(entry.sourcePath.size());
        cacheFile.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
        cacheFile.write(entry.sourcePath.c_str(), pathLen);

        cacheFile.write(reinterpret_cast<const char*>(&entry.uuid), sizeof(entry.uuid));
    }

    cacheFile.close();

    std::cout << "\nRebuilt UUID cache with " << count << " entries at: " << cachePath << std::endl;
    return 0;
}
