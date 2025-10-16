#ifndef PIIXELENGINE_AUDIOASSET_HPP
#define PIIXELENGINE_AUDIOASSET_HPP

#include "Resources/Asset.hpp"

#include <raylib.h>
#include <vector>

namespace PiiXeL {

enum class AudioFormat {
    Unknown = 0,
    WAV = 1,
    OGG = 2,
    MP3 = 3,
    FLAC = 4
};

enum class AudioLoadType {
    DecompressOnLoad,
    CompressedInMemory,
    Streaming
};

class AudioAsset : public Asset {
public:
    AudioAsset() : Asset{UUID{}, AssetType::Audio, ""} {}
    explicit AudioAsset(UUID uuid, const std::string& name);
    ~AudioAsset() override;

    bool Load(const void* data, size_t size) override;
    void Unload() override;
    [[nodiscard]] size_t GetMemoryUsage() const override;

    [[nodiscard]] Sound GetSound() const { return m_Sound; }
    [[nodiscard]] uint32_t GetFrameCount() const { return m_Sound.frameCount; }
    [[nodiscard]] float GetLength() const;
    [[nodiscard]] uint32_t GetSampleRate() const;
    [[nodiscard]] uint32_t GetChannels() const;
    [[nodiscard]] AudioFormat GetFormat() const { return m_Format; }
    [[nodiscard]] AudioLoadType GetLoadType() const { return m_LoadType; }

    void SetFormat(AudioFormat format) { m_Format = format; }
    void SetLoadType(AudioLoadType loadType) { m_LoadType = loadType; }

    static std::vector<uint8_t> EncodeToMemory(const std::string& sourcePath);
    static AudioFormat DetectAudioFormat(const std::string& extension);

private:
    Sound m_Sound{};
    Wave m_Wave{};
    AudioFormat m_Format{AudioFormat::Unknown};
    AudioLoadType m_LoadType{AudioLoadType::DecompressOnLoad};
};

} // namespace PiiXeL

#endif
