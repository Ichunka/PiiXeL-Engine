#pragma once

#include <cstddef>
#include <optional>
#include <raylib.h>
#include <span>
#include <string_view>

namespace PiiXeL {

class EmbeddedAssetLoader {
public:
    static std::optional<std::span<const std::byte>> GetAssetData(std::string_view assetName);

    static Texture2D LoadTextureFromEmbedded(std::string_view assetName);

    static Image LoadImageFromEmbedded(std::string_view assetName);

    static bool HasAsset(std::string_view assetName);

private:
    EmbeddedAssetLoader() = default;
};

} // namespace PiiXeL
