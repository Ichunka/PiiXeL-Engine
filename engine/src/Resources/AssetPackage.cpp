#include "Resources/AssetPackage.hpp"
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <raylib.h>

namespace PiiXeL {

static std::string NormalizePath(const std::string& path) {
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

bool AssetPackage::SaveToFile(const std::string& path, const AssetMetadata& metadata,
                               const void* data, size_t dataSize) {
    std::ofstream file{path, std::ios::binary};
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open file for writing: %s", path.c_str());
        return false;
    }

    Header header{};
    header.assetType = static_cast<uint16_t>(metadata.type);
    header.uuid = metadata.uuid.Get();
    header.importTimestamp = metadata.importTimestamp;
    header.sourceTimestamp = metadata.sourceTimestamp;
    header.dataSize = dataSize;

    std::string normalizedSourceFile = NormalizePath(metadata.sourceFile);
    std::string metadataStr = metadata.name + "|" + normalizedSourceFile + "|" +
                              std::to_string(metadata.version);
    header.metadataSize = metadataStr.size();

    if (!WriteHeader(file, header)) return false;

    file.write(metadataStr.c_str(), metadataStr.size());
    if (!file.good()) return false;

    if (!WriteData(file, data, dataSize)) return false;

    file.close();
    TraceLog(LOG_INFO, "Asset package saved: %s", path.c_str());
    return true;
}

bool AssetPackage::LoadFromFile(const std::string& path, AssetMetadata& outMetadata,
                                 std::vector<uint8_t>& outData) {
    std::ifstream file{path, std::ios::binary};
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open file for reading: %s", path.c_str());
        return false;
    }

    Header header{};
    if (!ReadHeader(file, header)) return false;

    if (header.magic != MAGIC_NUMBER) {
        TraceLog(LOG_ERROR, "Invalid asset package magic number: %s", path.c_str());
        return false;
    }

    if (header.version > VERSION) {
        TraceLog(LOG_ERROR, "Asset package version too new: %s", path.c_str());
        return false;
    }

    outMetadata.type = static_cast<AssetType>(header.assetType);
    outMetadata.uuid = UUID{header.uuid};
    outMetadata.importTimestamp = header.importTimestamp;
    outMetadata.sourceTimestamp = header.sourceTimestamp;

    if (!ReadMetadata(file, outMetadata, header.metadataSize)) return false;
    if (!ReadData(file, outData, header.dataSize)) return false;

    file.close();
    TraceLog(LOG_INFO, "Asset package loaded: %s", path.c_str());
    return true;
}

bool AssetPackage::LoadFromMemory(const uint8_t* data, size_t dataSize, AssetMetadata& outMetadata,
                                    std::vector<uint8_t>& outData) {
    if (!data || dataSize < sizeof(Header)) {
        return false;
    }

    size_t offset = 0;

    Header header{};
    std::memcpy(&header, data + offset, sizeof(Header));
    offset += sizeof(Header);

    if (header.magic != MAGIC_NUMBER) {
        return false;
    }

    if (header.version > VERSION) {
        return false;
    }

    outMetadata.type = static_cast<AssetType>(header.assetType);
    outMetadata.uuid = UUID{header.uuid};
    outMetadata.importTimestamp = header.importTimestamp;
    outMetadata.sourceTimestamp = header.sourceTimestamp;

    if (offset + header.metadataSize > dataSize) {
        return false;
    }

    std::string metadataStr(reinterpret_cast<const char*>(data + offset), header.metadataSize);
    offset += header.metadataSize;

    size_t pos1 = metadataStr.find('|');
    size_t pos2 = metadataStr.find('|', pos1 + 1);

    if (pos1 == std::string::npos || pos2 == std::string::npos) {
        return false;
    }

    outMetadata.name = metadataStr.substr(0, pos1);
    outMetadata.sourceFile = NormalizePath(metadataStr.substr(pos1 + 1, pos2 - pos1 - 1));
    outMetadata.version = std::stoul(metadataStr.substr(pos2 + 1));

    if (offset + header.dataSize > dataSize) {
        return false;
    }

    outData.resize(header.dataSize);
    std::memcpy(outData.data(), data + offset, header.dataSize);

    return true;
}

bool AssetPackage::LoadMetadataOnly(const std::string& path, AssetMetadata& outMetadata) {
    std::ifstream file{path, std::ios::binary};
    if (!file.is_open()) {
        return false;
    }

    Header header{};
    if (!ReadHeader(file, header)) return false;

    if (header.magic != MAGIC_NUMBER || header.version > VERSION) {
        return false;
    }

    outMetadata.type = static_cast<AssetType>(header.assetType);
    outMetadata.uuid = UUID{header.uuid};
    outMetadata.importTimestamp = header.importTimestamp;
    outMetadata.sourceTimestamp = header.sourceTimestamp;

    if (!ReadMetadata(file, outMetadata, header.metadataSize)) return false;

    file.close();
    return true;
}

std::string AssetPackage::GetPackagePath(const std::string& sourcePath) {
    std::filesystem::path path{sourcePath};
    path.replace_extension(".pxa");
    return path.string();
}

bool AssetPackage::PackageExists(const std::string& sourcePath) {
    std::string packagePath = GetPackagePath(sourcePath);
    return std::filesystem::exists(packagePath);
}

bool AssetPackage::NeedsReimport(const std::string& sourcePath) {
    std::string packagePath = GetPackagePath(sourcePath);

    if (!std::filesystem::exists(packagePath)) {
        return true;
    }

    uint64_t sourceTimestamp = GetFileTimestamp(sourcePath);
    uint64_t packageTimestamp = GetFileTimestamp(packagePath);

    return sourceTimestamp > packageTimestamp;
}

bool AssetPackage::WriteHeader(std::ofstream& stream, const Header& header) {
    stream.write(reinterpret_cast<const char*>(&header), sizeof(Header));
    return stream.good();
}

bool AssetPackage::ReadHeader(std::ifstream& stream, Header& header) {
    stream.read(reinterpret_cast<char*>(&header), sizeof(Header));
    return stream.good();
}

bool AssetPackage::WriteMetadata(std::ofstream& stream, const AssetMetadata& metadata) {
    std::string metadataStr = metadata.name + "|" + metadata.sourceFile + "|" +
                              std::to_string(metadata.version);
    stream.write(metadataStr.c_str(), metadataStr.size());
    return stream.good();
}

bool AssetPackage::ReadMetadata(std::ifstream& stream, AssetMetadata& metadata, size_t metadataSize) {
    std::vector<char> buffer(metadataSize + 1, '\0');
    stream.read(buffer.data(), metadataSize);
    if (!stream.good()) return false;

    std::string metadataStr{buffer.data()};
    size_t pos1 = metadataStr.find('|');
    size_t pos2 = metadataStr.find('|', pos1 + 1);

    if (pos1 == std::string::npos || pos2 == std::string::npos) return false;

    metadata.name = metadataStr.substr(0, pos1);
    metadata.sourceFile = NormalizePath(metadataStr.substr(pos1 + 1, pos2 - pos1 - 1));
    metadata.version = std::stoul(metadataStr.substr(pos2 + 1));

    return true;
}

bool AssetPackage::WriteData(std::ofstream& stream, const void* data, size_t size) {
    stream.write(static_cast<const char*>(data), size);
    return stream.good();
}

bool AssetPackage::ReadData(std::ifstream& stream, std::vector<uint8_t>& data, size_t size) {
    data.resize(size);
    stream.read(reinterpret_cast<char*>(data.data()), size);
    return stream.good();
}

uint64_t AssetPackage::GetFileTimestamp(const std::string& path) {
    try {
        auto ftime = std::filesystem::last_write_time(path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        return std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count();
    } catch (...) {
        return 0;
    }
}

} // namespace PiiXeL
