#include "Animation/AnimationClip.hpp"

#include "Core/Logger.hpp"

#include <nlohmann/json.hpp>

#include <raylib.h>

namespace PiiXeL {

AnimationClip::AnimationClip(UUID uuid, const std::string& name) : Asset(uuid, AssetType::AnimationClip, name) {}

bool AnimationClip::Load(const void* data, size_t size) {
    if (!data || size == 0)
    {
        PX_LOG_ERROR(ANIMATION, "Invalid data for AnimationClip");
        return false;
    }

    try
    {
        std::string jsonStr{reinterpret_cast<const char*>(data), size};
        nlohmann::json json = nlohmann::json::parse(jsonStr);

        if (json.contains("spriteSheetUUID"))
        { m_SpriteSheetUUID = UUID{json["spriteSheetUUID"].get<uint64_t>()}; }

        if (json.contains("frameRate"))
        { m_FrameRate = json["frameRate"].get<float>(); }

        if (json.contains("wrapMode"))
        { m_WrapMode = static_cast<AnimationWrapMode>(json["wrapMode"].get<int>()); }

        if (json.contains("frames") && json["frames"].is_array())
        {
            m_Frames.clear();
            for (const auto& frameJson : json["frames"])
            {
                AnimationFrame frame{};
                frame.frameIndex = frameJson.value("frameIndex", 0);
                frame.duration = frameJson.value("duration", 0.1f);
                m_Frames.push_back(frame);
            }
        }

        m_IsLoaded = true;
        return true;
    }
    catch (const nlohmann::json::exception& e)
    {
        PX_LOG_ERROR(ANIMATION, "Failed to parse AnimationClip JSON: %s", e.what());
        return false;
    }
}

void AnimationClip::Unload() {
    m_Frames.clear();
    m_SpriteSheetUUID = UUID{0};
    m_IsLoaded = false;
}

size_t AnimationClip::GetMemoryUsage() const {
    size_t total = sizeof(AnimationClip);
    total += m_Frames.capacity() * sizeof(AnimationFrame);
    return total;
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
    for (AnimationFrame& frame : m_Frames)
    { frame.duration = frameDuration; }
}

float AnimationClip::GetTotalDuration() const {
    float total = 0.0f;
    for (const AnimationFrame& frame : m_Frames)
    { total += frame.duration; }
    return total;
}

} // namespace PiiXeL
