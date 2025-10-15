#include "Components/Sprite.hpp"

#include "Reflection/Reflection.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/TextureAsset.hpp"

#include <cinttypes>
#include <raylib.h>

namespace PiiXeL {

BEGIN_REFLECT(Sprite)
FIELD_ASSET(textureAssetUUID, "texture")
FIELD(tint)
FIELD(origin)
FIELD(layer)
END_REFLECT(Sprite)

Texture2D Sprite::GetTexture() const {
    if (textureAssetUUID.Get() == 0) {
        m_LastLoadedTextureUUID = UUID{0};
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

    Texture2D texture = texAsset->GetTexture();

    if (texture.id != 0) {
        Sprite* mutableThis = const_cast<Sprite*>(this);

        if ((m_LastLoadedTextureUUID != textureAssetUUID &&
             (mutableThis->sourceRect.width == 0.0f && mutableThis->sourceRect.height == 0.0f)) ||
            (m_LastLoadedTextureUUID == UUID{0} &&
             (mutableThis->sourceRect.width == 0.0f && mutableThis->sourceRect.height == 0.0f)))
        {
            mutableThis->sourceRect = {0.0f, 0.0f, static_cast<float>(texture.width),
                                       static_cast<float>(texture.height)};
            m_LastLoadedTextureUUID = textureAssetUUID;
        }
        else if (m_LastLoadedTextureUUID != textureAssetUUID) {
            m_LastLoadedTextureUUID = textureAssetUUID;
        }
    }

    return texture;
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
    m_LastLoadedTextureUUID = UUID{0};

    Texture2D tex = GetTexture();
    if (tex.id != 0) {
        TraceLog(LOG_INFO, "Sprite texture set: %" PRIu64 ", size: %dx%d, sourceRect: %.0fx%.0f", assetUUID.Get(),
                 tex.width, tex.height, sourceRect.width, sourceRect.height);
    }
    else {
        TraceLog(LOG_WARNING, "Sprite SetTexture: texture not loaded for UUID %" PRIu64, assetUUID.Get());
    }
}

namespace Reflection {
void __force_link_Sprite() {}
} // namespace Reflection

} // namespace PiiXeL
