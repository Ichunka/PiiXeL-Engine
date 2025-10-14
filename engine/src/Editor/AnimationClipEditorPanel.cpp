#ifdef BUILD_WITH_EDITOR

#include "Editor/AnimationClipEditorPanel.hpp"

#include "Animation/AnimationSerializer.hpp"
#include "Core/Logger.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/TextureAsset.hpp"

#include <cinttypes>
#include <cstring>
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

namespace PiiXeL {

void AnimationClipEditorPanel::Render() {
    if (!m_IsOpen) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2{1200, 800}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Animation Clip Editor", &m_IsOpen)) {
        if (m_AnimationClip) {
            RenderToolbar();
            ImGui::Separator();

            ImGui::BeginChild("LeftPanel", ImVec2{350, 0}, true);
            RenderSettings();
            ImGui::Separator();
            RenderAvailableFrames();
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("RightPanel", ImVec2{0, 0}, true);
            ImGui::BeginChild("TimelinePanel", ImVec2{0, -250}, true);
            RenderTimeline();
            ImGui::EndChild();

            ImGui::BeginChild("PreviewPanel", ImVec2{0, 0}, true);
            RenderPreview();
            ImGui::EndChild();
            ImGui::EndChild();
        }
        else {
            ImGui::TextColored(ImVec4{1.0f, 0.3f, 0.3f, 1.0f}, "Failed to load animation clip");
        }
    }
    ImGui::End();
}

void AnimationClipEditorPanel::Open(const std::string& animClipPath) {
    m_CurrentPath = animClipPath;
    m_IsOpen = true;

    m_SelectedFrameIndex = -1;
    m_SelectedTimelineIndex = -1;
    m_IsPlaying = false;
    m_PreviewTime = 0.0f;
    m_CurrentPreviewFrame = 0;
    m_PreviewZoom = 2.0f;

    UUID existingUUID = AssetRegistry::Instance().GetUUIDFromPath(animClipPath);
    if (existingUUID.Get() != 0) {
        std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAsset(existingUUID);
        m_AnimationClip = std::dynamic_pointer_cast<AnimationClip>(asset);
    }
    else {
        std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAssetFromPath(animClipPath);
        m_AnimationClip = std::dynamic_pointer_cast<AnimationClip>(asset);
    }

    if (m_AnimationClip) {
        m_SelectedSpriteSheetUUID = m_AnimationClip->GetSpriteSheetUUID();
        PX_LOG_INFO(EDITOR, "AnimationClip opened, SpriteSheetUUID: %" PRIu64, m_SelectedSpriteSheetUUID.Get());

        if (m_SelectedSpriteSheetUUID.Get() != 0) {
            std::shared_ptr<Asset> spriteSheetAsset = AssetRegistry::Instance().LoadAsset(m_SelectedSpriteSheetUUID);
            m_SpriteSheet = std::dynamic_pointer_cast<SpriteSheet>(spriteSheetAsset);

            if (m_SpriteSheet) {
                PX_LOG_INFO(EDITOR, "SpriteSheet loaded: %s", m_SpriteSheet->GetName().c_str());
                UUID textureUUID = m_SpriteSheet->GetTextureUUID();
                if (textureUUID.Get() != 0) {
                    AssetRegistry::Instance().LoadAsset(textureUUID);
                }
            }
            else {
                PX_LOG_WARNING(EDITOR, "Failed to load SpriteSheet with UUID: %" PRIu64,
                               m_SelectedSpriteSheetUUID.Get());
            }
        }
    }
}

void AnimationClipEditorPanel::Close() {
    m_IsOpen = false;
    m_AnimationClip.reset();
    m_SpriteSheet.reset();
    m_CurrentPath.clear();
    m_SelectedSpriteSheetUUID = UUID{0};
    m_SelectedFrameIndex = -1;
    m_SelectedTimelineIndex = -1;
    m_IsPlaying = false;
    m_PreviewTime = 0.0f;
    m_CurrentPreviewFrame = 0;
}

void AnimationClipEditorPanel::RenderToolbar() {
    if (ImGui::Button("Save")) {
        Save();
    }

    ImGui::SameLine();
    ImGui::Text("| %s", m_AnimationClip->GetName().c_str());

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Text("| Duration: %.2fs", m_AnimationClip->GetTotalDuration());
}

void AnimationClipEditorPanel::RenderSettings() {
    ImGui::TextColored(ImVec4{0.4f, 0.8f, 0.6f, 1.0f}, "STEP 1: Select SpriteSheet");
    ImGui::Separator();

    if (m_SelectedSpriteSheetUUID.Get() == 0) {
        ImGui::Text("SpriteSheet:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "None");

        ImVec2 regionSize = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

        ImGui::BeginChild("DropZone", ImVec2{regionSize.x, 80}, true, ImGuiWindowFlags_NoScrollbar);
        ImVec2 dropSize = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize("Drop SpriteSheet here");
        ImGui::SetCursorPosX((dropSize.x - textSize.x) * 0.5f);
        ImGui::SetCursorPosY((dropSize.y - textSize.y) * 0.5f);
        ImGui::TextColored(ImVec4{0.6f, 0.6f, 0.6f, 1.0f}, "Drop SpriteSheet here");

        ImGui::SetCursorPos(ImVec2{0, 0});
        ImGui::InvisibleButton("##dropzone", dropSize);

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_ANIM")) {
                struct AssetInfo {
                    std::string path;
                    std::string filename;
                    std::string extension;
                    std::string type;
                    int width;
                    int height;
                    size_t fileSize;
                };

                AssetInfo** draggedAssetPtr = static_cast<AssetInfo**>(const_cast<void*>(payload->Data));
                if (draggedAssetPtr && *draggedAssetPtr) {
                    AssetInfo* draggedAsset = *draggedAssetPtr;
                    if (draggedAsset->type == "spritesheet") {
                        UUID spriteSheetUUID = AssetRegistry::Instance().GetUUIDFromPath(draggedAsset->path);
                        if (spriteSheetUUID.Get() == 0) {
                            std::shared_ptr<Asset> asset =
                                AssetRegistry::Instance().LoadAssetFromPath(draggedAsset->path);
                            if (asset) {
                                spriteSheetUUID = asset->GetUUID();
                            }
                        }

                        if (spriteSheetUUID.Get() != 0) {
                            m_SelectedSpriteSheetUUID = spriteSheetUUID;
                            m_AnimationClip->SetSpriteSheet(m_SelectedSpriteSheetUUID);
                            std::shared_ptr<Asset> spriteSheetAsset =
                                AssetRegistry::Instance().LoadAsset(m_SelectedSpriteSheetUUID);
                            m_SpriteSheet = std::dynamic_pointer_cast<SpriteSheet>(spriteSheetAsset);

                            if (m_SpriteSheet) {
                                UUID textureUUID = m_SpriteSheet->GetTextureUUID();
                                if (textureUUID.Get() != 0) {
                                    AssetRegistry::Instance().LoadAsset(textureUUID);
                                }
                            }

                            Save();
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::EndChild();
    }
    else {
        ImGui::Text("SpriteSheet:");

        if (m_SpriteSheet) {
            ImGui::Text("  %s", m_SpriteSheet->GetName().c_str());
            ImGui::TextColored(ImVec4{0.3f, 1.0f, 0.3f, 1.0f}, "  Frames available: %zu",
                               m_SpriteSheet->GetFrameCount());
        }
        else if (m_SelectedSpriteSheetUUID.Get() != 0) {
            ImGui::TextColored(ImVec4{1.0f, 0.5f, 0.5f, 1.0f}, "  [Not Loaded]");
        }

        if (ImGui::Button("Clear SpriteSheet")) {
            m_SelectedSpriteSheetUUID = UUID{0};
            m_AnimationClip->SetSpriteSheet(UUID{0});
            m_SpriteSheet.reset();
            Save();
        }
    }

    ImGui::Separator();

    float frameRate = m_AnimationClip->GetFrameRate();
    if (ImGui::SliderFloat("Frame Rate (FPS)", &frameRate, 1.0f, 60.0f)) {
        m_AnimationClip->SetFrameRate(frameRate);
    }

    ImGui::Separator();
    ImGui::Text("Wrap Mode");

    AnimationWrapMode currentMode = m_AnimationClip->GetWrapMode();
    if (ImGui::RadioButton("Once", currentMode == AnimationWrapMode::Once)) {
        m_AnimationClip->SetWrapMode(AnimationWrapMode::Once);
    }
    if (ImGui::RadioButton("Loop", currentMode == AnimationWrapMode::Loop)) {
        m_AnimationClip->SetWrapMode(AnimationWrapMode::Loop);
    }
    if (ImGui::RadioButton("Ping Pong", currentMode == AnimationWrapMode::PingPong)) {
        m_AnimationClip->SetWrapMode(AnimationWrapMode::PingPong);
    }
}

void AnimationClipEditorPanel::RenderAvailableFrames() {
    ImGui::TextColored(ImVec4{0.4f, 0.8f, 0.6f, 1.0f}, "STEP 2: Build Timeline");
    ImGui::Separator();

    if (!m_SpriteSheet) {
        ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "No SpriteSheet selected");
        ImGui::TextWrapped("Select a SpriteSheet first (Step 1)");
        return;
    }

    ImGui::Text("Available Frames: %zu", m_SpriteSheet->GetFrameCount());
    ImGui::Text("Available Groups: %zu", m_SpriteSheet->GetFrameGroupCount());
    ImGui::TextWrapped("Drag frames/groups to timeline");
    ImGui::Separator();

    ImGui::BeginChild("AvailableFramesScroll", ImVec2{0, 0}, false);

    if (m_SpriteSheet->GetFrameGroupCount() > 0 &&
        ImGui::CollapsingHeader("Frame Groups", ImGuiTreeNodeFlags_DefaultOpen)) {
        const std::vector<FrameGroup>& groups = m_SpriteSheet->GetFrameGroups();
        for (size_t i = 0; i < groups.size(); ++i) {
            ImGui::PushID(static_cast<int>(i) + 10000);

            char label[128];
            snprintf(label, sizeof(label), "[GROUP] %s (%zu frames)", groups[i].name.c_str(),
                     groups[i].frameIndices.size());

            ImGui::Button(label, ImVec2{-1, 0});

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                size_t groupIdx = i;
                ImGui::SetDragDropPayload("ANIM_GROUP", &groupIdx, sizeof(size_t));
                ImGui::Text("Group: %s", groups[i].name.c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::PopID();
        }
        ImGui::Separator();
    }

    if (ImGui::CollapsingHeader("Individual Frames", ImGuiTreeNodeFlags_DefaultOpen)) {
        const std::vector<SpriteFrame>& frames = m_SpriteSheet->GetFrames();
        for (size_t i = 0; i < frames.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));

            bool isSelected = (static_cast<int>(i) == m_SelectedFrameIndex);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{0.3f, 0.5f, 0.8f, 1.0f});
            }

            char label[64];
            snprintf(label, sizeof(label), "Frame %zu", i);
            if (ImGui::Selectable(label, isSelected)) {
                m_SelectedFrameIndex = static_cast<int>(i);
            }

            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("ANIM_FRAME", &i, sizeof(size_t));
                ImGui::Text("Frame %zu", i);
                ImGui::EndDragDropSource();
            }

            if (isSelected) {
                ImGui::PopStyleColor();
            }

            ImGui::PopID();
        }
    }

    ImGui::EndChild();

    if (ImGui::Button("Add Selected Frame")) {
        if (m_SelectedFrameIndex >= 0) {
            m_AnimationClip->AddFrame(static_cast<size_t>(m_SelectedFrameIndex),
                                      1.0f / m_AnimationClip->GetFrameRate());
        }
    }
}

void AnimationClipEditorPanel::RenderTimeline() {
    ImGui::Text("Animation Timeline (%zu frames)", m_AnimationClip->GetFrames().size());

    if (ImGui::Button("Clear All")) {
        std::vector<AnimationFrame> emptyFrames;
        m_AnimationClip->SetFrames(emptyFrames);
        m_SelectedTimelineIndex = -1;
    }

    ImGui::Separator();

    ImGui::BeginChild("TimelineScroll", ImVec2{0, 0}, false);

    std::vector<AnimationFrame> frames = m_AnimationClip->GetFrames();
    bool framesChanged = false;
    int frameToDelete = -1;

    for (size_t i = 0; i < frames.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));

        bool isSelected = (static_cast<int>(i) == m_SelectedTimelineIndex);
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{0.3f, 0.5f, 0.8f, 1.0f});
        }

        char label[64];
        snprintf(label, sizeof(label), "[%zu] Frame %zu", i, frames[i].frameIndex);
        if (ImGui::Selectable(label, isSelected)) {
            m_SelectedTimelineIndex = static_cast<int>(i);
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* dropPayload = ImGui::AcceptDragDropPayload("ANIM_FRAME")) {
                size_t droppedFrameIndex = *static_cast<const size_t*>(dropPayload->Data);
                AnimationFrame newFrame{};
                newFrame.frameIndex = droppedFrameIndex;
                newFrame.duration = 1.0f / m_AnimationClip->GetFrameRate();
                frames.insert(frames.begin() + i, newFrame);
                framesChanged = true;
            }
            else if (const ImGuiPayload* groupPayload = ImGui::AcceptDragDropPayload("ANIM_GROUP")) {
                size_t groupIndex = *static_cast<const size_t*>(groupPayload->Data);
                const FrameGroup* group = m_SpriteSheet->GetFrameGroup(groupIndex);
                if (group) {
                    for (size_t frameIdx : group->frameIndices) {
                        AnimationFrame newFrame{};
                        newFrame.frameIndex = frameIdx;
                        newFrame.duration = 1.0f / m_AnimationClip->GetFrameRate();
                        frames.insert(frames.begin() + i, newFrame);
                        i++;
                    }
                    framesChanged = true;
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (isSelected) {
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
        float duration = frames[i].duration;
        ImGui::SetNextItemWidth(100);
        if (ImGui::DragFloat("##duration", &duration, 0.01f, 0.01f, 10.0f, "%.2fs")) {
            frames[i].duration = duration;
            framesChanged = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("X")) {
            frameToDelete = static_cast<int>(i);
        }

        ImGui::PopID();
    }

    if (frameToDelete >= 0) {
        frames.erase(frames.begin() + frameToDelete);
        framesChanged = true;
        if (m_SelectedTimelineIndex == frameToDelete) {
            m_SelectedTimelineIndex = -1;
        }
    }

    if (frames.empty()) {
        ImVec2 regionSize = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize("Empty Timeline");
        ImGui::SetCursorPosX((regionSize.x - textSize.x) * 0.5f);
        ImGui::SetCursorPosY((regionSize.y - textSize.y) * 0.5f - 20);
        ImGui::TextColored(ImVec4{1.0f, 0.8f, 0.2f, 1.0f}, "Empty Timeline");

        ImGui::SetCursorPosX(10);
        ImGui::SetCursorPosY((regionSize.y) * 0.5f);
        ImGui::TextWrapped("Drag frames from 'Available Frames' here\nor use 'Add Selected Frame' button");

        ImGui::SetCursorPos(ImVec2{0, 0});
        ImGui::InvisibleButton("##empty_timeline", regionSize);

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* dropPayload = ImGui::AcceptDragDropPayload("ANIM_FRAME")) {
                size_t droppedFrameIndex = *static_cast<const size_t*>(dropPayload->Data);
                AnimationFrame newFrame{};
                newFrame.frameIndex = droppedFrameIndex;
                newFrame.duration = 1.0f / m_AnimationClip->GetFrameRate();
                frames.push_back(newFrame);
                framesChanged = true;
            }
            else if (const ImGuiPayload* groupPayload = ImGui::AcceptDragDropPayload("ANIM_GROUP")) {
                size_t groupIndex = *static_cast<const size_t*>(groupPayload->Data);
                const FrameGroup* group = m_SpriteSheet->GetFrameGroup(groupIndex);
                if (group) {
                    for (size_t frameIdx : group->frameIndices) {
                        AnimationFrame newFrame{};
                        newFrame.frameIndex = frameIdx;
                        newFrame.duration = 1.0f / m_AnimationClip->GetFrameRate();
                        frames.push_back(newFrame);
                    }
                    framesChanged = true;
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    if (framesChanged) {
        m_AnimationClip->SetFrames(frames);
    }

    ImGui::EndChild();
}

void AnimationClipEditorPanel::RenderPreview() {
    ImGui::TextColored(ImVec4{0.4f, 0.8f, 0.6f, 1.0f}, "STEP 3: Preview Animation");
    ImGui::Separator();

    if (ImGui::Button(m_IsPlaying ? "Stop" : "Play")) {
        if (m_IsPlaying) {
            StopPreview();
        }
        else {
            PlayPreview();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        m_PreviewTime = 0.0f;
        m_CurrentPreviewFrame = 0;
    }

    ImGui::SameLine();
    ImGui::Text("Frame: %zu / %zu", m_CurrentPreviewFrame + 1, m_AnimationClip->GetFrames().size());

    ImGui::Separator();

    if (m_IsPlaying) {
        float deltaTime = ImGui::GetIO().DeltaTime;
        m_PreviewTime += deltaTime;

        const std::vector<AnimationFrame>& frames = m_AnimationClip->GetFrames();
        if (!frames.empty()) {
            float totalDuration = m_AnimationClip->GetTotalDuration();

            if (m_AnimationClip->GetWrapMode() == AnimationWrapMode::Once) {
                if (m_PreviewTime >= totalDuration) {
                    m_PreviewTime = totalDuration;
                    m_IsPlaying = false;
                }
            }
            else if (m_AnimationClip->GetWrapMode() == AnimationWrapMode::Loop) {
                if (m_PreviewTime >= totalDuration) {
                    m_PreviewTime = fmodf(m_PreviewTime, totalDuration);
                }
            }

            float accumulatedTime = 0.0f;
            for (size_t i = 0; i < frames.size(); ++i) {
                accumulatedTime += frames[i].duration;
                if (m_PreviewTime < accumulatedTime) {
                    m_CurrentPreviewFrame = i;
                    break;
                }
            }
        }
    }

    if (!m_SpriteSheet) {
        ImVec2 regionSize = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize("No SpriteSheet");
        ImGui::SetCursorPosX((regionSize.x - textSize.x) * 0.5f);
        ImGui::SetCursorPosY((regionSize.y - textSize.y) * 0.5f - 10);
        ImGui::TextColored(ImVec4{1.0f, 0.8f, 0.2f, 1.0f}, "No SpriteSheet");
        ImGui::SetCursorPosX(10);
        ImGui::SetCursorPosY((regionSize.y - textSize.y) * 0.5f + 10);
        ImGui::TextWrapped("Drag a SHEET from Content Browser (Step 1)");
        return;
    }

    const std::vector<AnimationFrame>& frames = m_AnimationClip->GetFrames();
    if (frames.empty()) {
        ImVec2 regionSize = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize("No frames in timeline");
        ImGui::SetCursorPosX((regionSize.x - textSize.x) * 0.5f);
        ImGui::SetCursorPosY((regionSize.y - textSize.y) * 0.5f - 10);
        ImGui::TextColored(ImVec4{1.0f, 0.8f, 0.2f, 1.0f}, "No frames in timeline");
        ImGui::SetCursorPosX(10);
        ImGui::SetCursorPosY((regionSize.y - textSize.y) * 0.5f + 10);
        ImGui::TextWrapped("Add frames to timeline (Step 2)");
        return;
    }

    if (m_CurrentPreviewFrame >= frames.size()) {
        m_CurrentPreviewFrame = 0;
    }

    size_t spriteFrameIndex = frames[m_CurrentPreviewFrame].frameIndex;
    if (spriteFrameIndex >= m_SpriteSheet->GetFrameCount()) {
        ImGui::TextColored(ImVec4{1.0f, 0.3f, 0.3f, 1.0f}, "Invalid frame index");
        return;
    }

    const SpriteFrame* spriteFrame = m_SpriteSheet->GetFrame(spriteFrameIndex);
    if (!spriteFrame) {
        return;
    }

    UUID textureUUID = m_SpriteSheet->GetTextureUUID();
    if (textureUUID.Get() == 0) {
        ImGui::Text("No texture in SpriteSheet");
        return;
    }

    std::shared_ptr<Asset> texAsset = AssetRegistry::Instance().GetAsset(textureUUID);
    if (!texAsset) {
        ImGui::Text("Texture not loaded");
        return;
    }

    TextureAsset* tex = dynamic_cast<TextureAsset*>(texAsset.get());
    if (!tex) {
        ImGui::Text("Invalid texture asset");
        return;
    }

    Texture2D texture = tex->GetTexture();
    if (texture.id == 0) {
        ImGui::Text("Texture not ready");
        return;
    }

    ImVec2 availSize = ImGui::GetContentRegionAvail();
    float displayWidth = spriteFrame->sourceRect.width * m_PreviewZoom;
    float displayHeight = spriteFrame->sourceRect.height * m_PreviewZoom;

    ImVec2 cursorPos = ImGui::GetCursorPos();
    cursorPos.x += (availSize.x - displayWidth) * 0.5f;
    cursorPos.y += (availSize.y - displayHeight) * 0.5f;
    ImGui::SetCursorPos(cursorPos);

    rlImGuiImageRect(&texture, static_cast<int>(displayWidth), static_cast<int>(displayHeight),
                     spriteFrame->sourceRect);
}

void AnimationClipEditorPanel::PlayPreview() {
    m_IsPlaying = true;
}

void AnimationClipEditorPanel::StopPreview() {
    m_IsPlaying = false;
}

void AnimationClipEditorPanel::Save() {
    if (!m_AnimationClip || m_CurrentPath.empty()) {
        PX_LOG_ERROR(EDITOR, "Cannot save: no animation clip or path");
        return;
    }

    if (AnimationSerializer::SerializeAnimationClip(*m_AnimationClip, m_CurrentPath)) {
        PX_LOG_INFO(EDITOR, "Saved animation clip: %s", m_CurrentPath.c_str());

        UUID animClipUUID = m_AnimationClip->GetUUID();
        AssetRegistry::Instance().ReimportAsset(m_CurrentPath);

        std::shared_ptr<Asset> reloadedAsset = AssetRegistry::Instance().LoadAsset(animClipUUID);
        m_AnimationClip = std::dynamic_pointer_cast<AnimationClip>(reloadedAsset);

        if (m_AnimationClip) {
            m_SelectedSpriteSheetUUID = m_AnimationClip->GetSpriteSheetUUID();
            if (m_SelectedSpriteSheetUUID.Get() != 0 && !m_SpriteSheet) {
                std::shared_ptr<Asset> spriteSheetAsset =
                    AssetRegistry::Instance().LoadAsset(m_SelectedSpriteSheetUUID);
                m_SpriteSheet = std::dynamic_pointer_cast<SpriteSheet>(spriteSheetAsset);
            }
        }
    }
    else {
        PX_LOG_ERROR(EDITOR, "Failed to save animation clip: %s", m_CurrentPath.c_str());
    }
}

} // namespace PiiXeL

#endif
