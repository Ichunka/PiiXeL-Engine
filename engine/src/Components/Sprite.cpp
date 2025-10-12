#include "Components/Sprite.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/TextureAsset.hpp"
#include "Reflection/Reflection.hpp"
#include <raylib.h>
#include <cinttypes>

namespace PiiXeL {

BEGIN_REFLECT(Sprite)
    FIELD_ASSET(textureAssetUUID, "texture")
    FIELD(tint)
    FIELD(origin)
    FIELD(layer)
END_REFLECT(Sprite)

Texture2D Sprite::GetTexture() const {
    if (textureAssetUUID.Get() == 0) {
        return Texture2D{};
    }

    std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAsset(textureAssetUUID);
    if (!asset) {
        return Texture2D{};
    }

    TextureAsset* texAsset = dynamic_cast<TextureAsset*>(asset.get());
    if (!texAsset) {
        return Texture2D{};
    }

    return texAsset->GetTexture();
}

bool Sprite::IsValid() const {
    return textureAssetUUID.Get() != 0;
}

Vector2 Sprite::GetSize() const {
    if (sourceRect.width > 0 && sourceRect.height > 0) {
        return {sourceRect.width, sourceRect.height};
    }

    Texture2D tex = GetTexture();
    if (tex.id != 0) {
        return {static_cast<float>(tex.width), static_cast<float>(tex.height)};
    }

    return {64.0f, 64.0f};
}

void Sprite::SetTexture(UUID assetUUID) {
    textureAssetUUID = assetUUID;

    Texture2D tex = GetTexture();
    if (tex.id != 0) {
        if (sourceRect.width == 0.0f && sourceRect.height == 0.0f) {
            sourceRect = {0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)};
            TraceLog(LOG_INFO, "Sprite texture set: %" PRIu64 ", size: %dx%d, sourceRect: %.0fx%.0f",
                     assetUUID.Get(), tex.width, tex.height, sourceRect.width, sourceRect.height);
        }
    } else {
        TraceLog(LOG_WARNING, "Sprite SetTexture: texture not loaded for UUID %" PRIu64, assetUUID.Get());
    }
}

namespace Reflection {
void __force_link_Sprite() {}
}

} // namespace PiiXeL
