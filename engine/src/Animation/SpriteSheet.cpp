#include "Animation/SpriteSheet.hpp"

namespace PiiXeL {

SpriteSheet::SpriteSheet(UUID uuid, const std::string& name)
    : Asset(uuid, AssetType::SpriteSheet, name) {
}

bool SpriteSheet::Load(const void* data, size_t size) {
    (void)data;
    (void)size;
    m_IsLoaded = true;
    return true;
}

void SpriteSheet::Unload() {
    m_Frames.clear();
    m_TextureUUID = UUID{0};
    m_IsLoaded = false;
}

size_t SpriteSheet::GetMemoryUsage() const {
    return m_Frames.size() * sizeof(SpriteFrame);
}

void SpriteSheet::SetTexture(UUID textureUUID) {
    m_TextureUUID = textureUUID;
}

void SpriteSheet::AddFrame(const SpriteFrame& frame) {
    m_Frames.push_back(frame);
}

void SpriteSheet::SetFrames(const std::vector<SpriteFrame>& frames) {
    m_Frames = frames;
}

const SpriteFrame* SpriteSheet::GetFrame(size_t index) const {
    if (index >= m_Frames.size()) {
        return nullptr;
    }
    return &m_Frames[index];
}

} // namespace PiiXeL
