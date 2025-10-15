#pragma once

#include "Resources/Asset.hpp"

#include <string>
#include <vector>

namespace PiiXeL {

struct AnimationFrame {
    size_t frameIndex{0};
    float duration{0.1f};
};

enum class AnimationWrapMode { Once, Loop, PingPong };

class AnimationClip : public Asset {
public:
    AnimationClip(UUID uuid, const std::string& name);
    ~AnimationClip() override = default;

    bool Load(const void* data, size_t size) override;
    void Unload() override;
    size_t GetMemoryUsage() const override;

    void SetSpriteSheet(UUID spriteSheetUUID);
    UUID GetSpriteSheetUUID() const { return m_SpriteSheetUUID; }

    void AddFrame(size_t frameIndex, float duration);
    void SetFrames(const std::vector<AnimationFrame>& frames);
    const std::vector<AnimationFrame>& GetFrames() const { return m_Frames; }

    void SetWrapMode(AnimationWrapMode mode) { m_WrapMode = mode; }
    AnimationWrapMode GetWrapMode() const { return m_WrapMode; }

    void SetFrameRate(float fps);
    float GetFrameRate() const { return m_FrameRate; }

    float GetTotalDuration() const;

private:
    UUID m_SpriteSheetUUID{0};
    std::vector<AnimationFrame> m_Frames;
    AnimationWrapMode m_WrapMode{AnimationWrapMode::Loop};
    float m_FrameRate{12.0f};
};

} // namespace PiiXeL
