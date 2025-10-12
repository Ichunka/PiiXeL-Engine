#ifndef PIIXELENGINE_AUDIOASSET_HPP
#define PIIXELENGINE_AUDIOASSET_HPP

#include "Resources/Asset.hpp"
#include <raylib.h>

namespace PiiXeL {

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

    static std::vector<uint8_t> EncodeToMemory(const std::string& sourcePath);

private:
    Sound m_Sound{};
    Wave m_Wave{};
};

} // namespace PiiXeL

#endif
