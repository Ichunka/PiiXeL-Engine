#include "Resources/TextureAsset.hpp"

#include "Core/Logger.hpp"

#include <cstring>

namespace PiiXeL {

TextureAsset::TextureAsset(UUID uuid, const std::string& name) : Asset{uuid, AssetType::Texture, name} {}

TextureAsset::~TextureAsset() {
    Unload();
}

bool TextureAsset::Load(const void* data, size_t size) {
    if (m_IsLoaded)
    { Unload(); }

    const char* fileExt = ".png";
    Image image = LoadImageFromMemory(fileExt, static_cast<const unsigned char*>(data), static_cast<int>(size));

    if (image.data == nullptr)
    {
        PX_LOG_ERROR(ASSET, "Failed to load texture from memory");
        return false;
    }

    m_Texture = LoadTextureFromImage(image);
    UnloadImage(image);

    if (m_Texture.id == 0)
    {
        PX_LOG_ERROR(ASSET, "Failed to create texture from image");
        return false;
    }

    SetTextureWrap(m_Texture, TEXTURE_WRAP_CLAMP);

    m_IsLoaded = true;
    PX_LOG_INFO(ASSET, "Texture asset loaded: %s (%dx%d)", m_Metadata.name.c_str(), m_Texture.width, m_Texture.height);
    return true;
}

void TextureAsset::Unload() {
    if (m_IsLoaded && m_Texture.id != 0)
    {
        UnloadTexture(m_Texture);
        m_Texture = Texture2D{};
        m_IsLoaded = false;
    }
}

size_t TextureAsset::GetMemoryUsage() const {
    if (!m_IsLoaded)
        return 0;

    int bytesPerPixel = 4;
    switch (m_Texture.format)
    {
        case PIXELFORMAT_UNCOMPRESSED_GRAYSCALE:
            bytesPerPixel = 1;
            break;
        case PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA:
            bytesPerPixel = 2;
            break;
        case PIXELFORMAT_UNCOMPRESSED_R5G6B5:
            bytesPerPixel = 2;
            break;
        case PIXELFORMAT_UNCOMPRESSED_R8G8B8:
            bytesPerPixel = 3;
            break;
        case PIXELFORMAT_UNCOMPRESSED_R8G8B8A8:
            bytesPerPixel = 4;
            break;
        default:
            bytesPerPixel = 4;
            break;
    }

    return m_Texture.width * m_Texture.height * bytesPerPixel;
}

std::vector<uint8_t> TextureAsset::EncodeToMemory(const std::string& sourcePath) {
    Image image = LoadImage(sourcePath.c_str());
    if (image.data == nullptr)
    {
        PX_LOG_ERROR(ASSET, "Failed to load image from: %s", sourcePath.c_str());
        return {};
    }

    int dataSize = 0;
    unsigned char* fileData = ExportImageToMemory(image, ".png", &dataSize);
    UnloadImage(image);

    if (fileData == nullptr || dataSize == 0)
    {
        PX_LOG_ERROR(ASSET, "Failed to encode image to memory");
        return {};
    }

    std::vector<uint8_t> result{fileData, fileData + dataSize};
    RL_FREE(fileData);

    return result;
}

} // namespace PiiXeL
