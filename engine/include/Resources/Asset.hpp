#ifndef PIIXELENGINE_ASSET_HPP
#define PIIXELENGINE_ASSET_HPP

#include "Components/UUID.hpp"
#include <string>
#include <cstdint>
#include <chrono>

namespace PiiXeL {

enum class AssetType : uint16_t {
    Unknown = 0,
    Texture = 1,
    Audio = 2,
    Scene = 3,
    Prefab = 4,
    Material = 5,
    Shader = 6,
    Font = 7
};

struct AssetMetadata {
    UUID uuid;
    AssetType type{AssetType::Unknown};
    std::string name;
    std::string sourceFile;
    uint64_t importTimestamp{0};
    uint64_t sourceTimestamp{0};
    uint32_t version{1};

    bool NeedsReimport(uint64_t currentSourceTimestamp) const {
        return currentSourceTimestamp > sourceTimestamp;
    }
};

class Asset {
public:
    Asset() = default;
    explicit Asset(UUID uuid, AssetType type, const std::string& name);
    virtual ~Asset() = default;

    [[nodiscard]] UUID GetUUID() const { return m_Metadata.uuid; }
    [[nodiscard]] AssetType GetType() const { return m_Metadata.type; }
    [[nodiscard]] const std::string& GetName() const { return m_Metadata.name; }
    [[nodiscard]] const AssetMetadata& GetMetadata() const { return m_Metadata; }

    void SetMetadata(const AssetMetadata& metadata) { m_Metadata = metadata; }

    [[nodiscard]] bool IsLoaded() const { return m_IsLoaded; }
    [[nodiscard]] bool IsValid() const { return m_Metadata.uuid.Get() != 0; }

    virtual bool Load(const void* data, size_t size) = 0;
    virtual void Unload() = 0;
    virtual size_t GetMemoryUsage() const = 0;

protected:
    AssetMetadata m_Metadata;
    bool m_IsLoaded{false};
};

} // namespace PiiXeL

#endif
