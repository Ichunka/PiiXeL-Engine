#include "Animation/AnimationClip.hpp"

namespace PiiXeL {

AnimationClip::AnimationClip(UUID uuid, const std::string& name)
    : Asset(uuid, AssetType::AnimationClip, name) {
}

bool AnimationClip::Load(const void* data, size_t size) {
    (void)data;
    (void)size;
    m_IsLoaded = true;
    return true;
}

void AnimationClip::Unload() {
    m_Frames.clear();
    m_SpriteSheetUUID = UUID{0};
    m_IsLoaded = false;
}

size_t AnimationClip::GetMemoryUsage() const {
    return m_Frames.size() * sizeof(AnimationFrame);
}

void AnimationClip::SetSpriteSheet(UUID spriteSheetUUID) {
    m_SpriteSheetUUID = spriteSheetUUID;
}

void AnimationClip::AddFrame(size_t frameIndex, float duration) {
    m_Frames.push_back({frameIndex, duration});
}

void AnimationClip::SetFrames(const std::vector<AnimationFrame>& frames) {
    m_Frames = frames;
}

void AnimationClip::SetFrameRate(float fps) {
    m_FrameRate = fps;
    float frameDuration = 1.0f / fps;
    for (AnimationFrame& frame : m_Frames) {
        frame.duration = frameDuration;
    }
}

float AnimationClip::GetTotalDuration() const {
    float total = 0.0f;
    for (const AnimationFrame& frame : m_Frames) {
        total += frame.duration;
    }
    return total;
}

} // namespace PiiXeL
