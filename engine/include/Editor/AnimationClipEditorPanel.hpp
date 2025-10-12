#ifndef PIIXELENGINE_ANIMATIONCLIPEDITORPANEL_HPP
#define PIIXELENGINE_ANIMATIONCLIPEDITORPANEL_HPP

#ifdef BUILD_WITH_EDITOR

#include "Animation/AnimationClip.hpp"
#include "Animation/SpriteSheet.hpp"
#include "Components/UUID.hpp"
#include <string>
#include <memory>
#include <vector>

struct ImVec2;

namespace PiiXeL {

class AnimationClipEditorPanel {
public:
    AnimationClipEditorPanel() = default;
    ~AnimationClipEditorPanel() = default;

    void Render();
    void Open(const std::string& animClipPath);
    void Close();

    [[nodiscard]] bool IsOpen() const { return m_IsOpen; }

private:
    void RenderToolbar();
    void RenderSettings();
    void RenderAvailableFrames();
    void RenderTimeline();
    void RenderPreview();

    void Save();
    void PlayPreview();
    void StopPreview();

    bool m_IsOpen{false};
    std::string m_CurrentPath;
    std::shared_ptr<AnimationClip> m_AnimationClip;
    std::shared_ptr<SpriteSheet> m_SpriteSheet;

    UUID m_SelectedSpriteSheetUUID{0};
    int m_SelectedFrameIndex{-1};
    int m_SelectedTimelineIndex{-1};

    bool m_IsPlaying{false};
    float m_PreviewTime{0.0f};
    size_t m_CurrentPreviewFrame{0};

    float m_PreviewZoom{2.0f};
};

} // namespace PiiXeL

#endif

#endif
