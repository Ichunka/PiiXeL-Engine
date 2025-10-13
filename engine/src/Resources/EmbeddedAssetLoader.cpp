#include "Resources/EmbeddedAssetLoader.hpp"
#include "EmbeddedAssetRegistry.generated.hpp"
#include "Core/Logger.hpp"
#include <raylib.h>

namespace PiiXeL {

std::optional<std::span<const std::byte>> EmbeddedAssetLoader::GetAssetData(std::string_view assetName) {
    const auto& registry = EmbeddedAssets::Registry;
    const auto it = registry.find(assetName);

    if (it != registry.end()) {
        return it->second;
    }

    return std::nullopt;
}

Texture2D EmbeddedAssetLoader::LoadTextureFromEmbedded(std::string_view assetName) {
    const auto data = GetAssetData(assetName);

    if (!data.has_value()) {
        PX_LOG_ERROR(ASSET, "Embedded asset not found: %.*s", static_cast<int>(assetName.size()), assetName.data());
        return Texture2D{};
    }

    const std::span<const std::byte>& span = data.value();
    const unsigned char* dataPtr = reinterpret_cast<const unsigned char*>(span.data());
    const int dataSize = static_cast<int>(span.size());

    const Image image = LoadImageFromMemory(".png", dataPtr, dataSize);
    if (image.data == nullptr) {
        PX_LOG_ERROR(ASSET, "Failed to load embedded image: %.*s", static_cast<int>(assetName.size()), assetName.data());
        return Texture2D{};
    }

    const Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);

    return texture;
}

Image EmbeddedAssetLoader::LoadImageFromEmbedded(std::string_view assetName) {
    const auto data = GetAssetData(assetName);

    if (!data.has_value()) {
        PX_LOG_ERROR(ASSET, "Embedded asset not found: %.*s", static_cast<int>(assetName.size()), assetName.data());
        return Image{};
    }

    const std::span<const std::byte>& span = data.value();
    const unsigned char* dataPtr = reinterpret_cast<const unsigned char*>(span.data());
    const int dataSize = static_cast<int>(span.size());

    const Image image = LoadImageFromMemory(".png", dataPtr, dataSize);
    if (image.data == nullptr) {
        PX_LOG_ERROR(ASSET, "Failed to load embedded image: %.*s", static_cast<int>(assetName.size()), assetName.data());
        return Image{};
    }

    return image;
}

bool EmbeddedAssetLoader::HasAsset(std::string_view assetName) {
    const auto& registry = EmbeddedAssets::Registry;
    return registry.find(assetName) != registry.end();
}

}
