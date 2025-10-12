#ifndef PIIXELENGINE_ASSETPACKAGE_HPP
#define PIIXELENGINE_ASSETPACKAGE_HPP

#include "Resources/Asset.hpp"
#include <vector>
#include <memory>
#include <fstream>

namespace PiiXeL {

class AssetPackage {
public:
    static constexpr uint32_t MAGIC_NUMBER = 0x41585850;
    static constexpr uint32_t VERSION = 1;

    struct Header {
        uint32_t magic{MAGIC_NUMBER};
        uint32_t version{VERSION};
        uint16_t assetType{0};
        uint16_t reserved{0};
        uint64_t uuid{0};
        uint64_t metadataSize{0};
        uint64_t dataSize{0};
        uint64_t importTimestamp{0};
        uint64_t sourceTimestamp{0};
    };

    AssetPackage() = default;
    ~AssetPackage() = default;

    bool SaveToFile(const std::string& path, const AssetMetadata& metadata,
                    const void* data, size_t dataSize);

    bool LoadFromFile(const std::string& path, AssetMetadata& outMetadata,
                      std::vector<uint8_t>& outData);

    [[nodiscard]] static std::string GetPackagePath(const std::string& sourcePath);
    [[nodiscard]] static bool PackageExists(const std::string& sourcePath);
    [[nodiscard]] static bool NeedsReimport(const std::string& sourcePath);

private:
    bool WriteHeader(std::ofstream& stream, const Header& header);
    bool ReadHeader(std::ifstream& stream, Header& header);
    bool WriteMetadata(std::ofstream& stream, const AssetMetadata& metadata);
    bool ReadMetadata(std::ifstream& stream, AssetMetadata& metadata, size_t metadataSize);
    bool WriteData(std::ofstream& stream, const void* data, size_t size);
    bool ReadData(std::ifstream& stream, std::vector<uint8_t>& data, size_t size);

    static uint64_t GetFileTimestamp(const std::string& path);
};

} // namespace PiiXeL

#endif
