#ifndef PIIXELENGINE_SPRITE_HPP
#define PIIXELENGINE_SPRITE_HPP

#include <raylib.h>
#include "Components/UUID.hpp"

namespace PiiXeL {

struct Sprite {
    UUID textureAssetUUID{0};
    Color tint{WHITE};
    Rectangle sourceRect{0.0f, 0.0f, 0.0f, 0.0f};
    Vector2 origin{0.5f, 0.5f};
    int layer{0};

    Sprite() = default;

    [[nodiscard]] Texture2D GetTexture() const;
    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Vector2 GetSize() const;

    void SetTexture(UUID assetUUID);

private:
    mutable UUID m_LastLoadedTextureUUID{0};
};

} // namespace PiiXeL

#endif // PIIXELENGINE_SPRITE_HPP
