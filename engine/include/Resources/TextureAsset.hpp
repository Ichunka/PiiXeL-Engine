#ifndef PIIXELENGINE_TEXTUREASSET_HPP
#define PIIXELENGINE_TEXTUREASSET_HPP

#include "Resources/Asset.hpp"
#include <raylib.h>

namespace PiiXeL {

class TextureAsset : public Asset {
public:
    TextureAsset() : Asset{UUID{}, AssetType::Texture, ""} {}
    explicit TextureAsset(UUID uuid, const std::string& name);
    ~TextureAsset() override;

    bool Load(const void* data, size_t size) override;
    void Unload() override;
    [[nodiscard]] size_t GetMemoryUsage() const override;

    [[nodiscard]] Texture2D GetTexture() const { return m_Texture; }
    [[nodiscard]] int GetWidth() const { return m_Texture.width; }
    [[nodiscard]] int GetHeight() const { return m_Texture.height; }
    [[nodiscard]] int GetMipmaps() const { return m_Texture.mipmaps; }
    [[nodiscard]] int GetFormat() const { return m_Texture.format; }

    static std::vector<uint8_t> EncodeToMemory(const std::string& sourcePath);

private:
    Texture2D m_Texture{};
};

} // namespace PiiXeL

#endif
