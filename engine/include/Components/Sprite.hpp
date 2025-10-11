#ifndef PIIXELENGINE_SPRITE_HPP
#define PIIXELENGINE_SPRITE_HPP

#include <raylib.h>
#include <string>

namespace PiiXeL {

struct Sprite {
    Texture2D texture{};
    std::string texturePath{};
    Color tint{WHITE};
    Rectangle sourceRect{0.0f, 0.0f, 0.0f, 0.0f};
    Vector2 origin{0.5f, 0.5f};
    int layer{0};

    Sprite();
    explicit Sprite(const Texture2D& tex)
        : texture{tex}, sourceRect{0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)} {}

    [[nodiscard]] bool IsValid() const { return texture.id != 0; }
    [[nodiscard]] Vector2 GetSize() const { return {sourceRect.width, sourceRect.height}; }
};

} // namespace PiiXeL

#endif // PIIXELENGINE_SPRITE_HPP
