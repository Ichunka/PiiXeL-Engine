#ifdef BUILD_WITH_EDITOR

#include "Editor/SpriteSheetEditorPanel.hpp"

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

void SpriteSheetEditorPanel::Render() {
    if (!m_IsOpen) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2{1000, 700}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Sprite Sheet Editor", &m_IsOpen)) {
        if (m_SpriteSheet) {
            RenderToolbar();
            ImGui::Separator();

            ImGui::BeginChild("LeftPanel", ImVec2{350, 0}, true);
            RenderTextureSelector();
            ImGui::Separator();
            RenderGridSettings();
            ImGui::Separator();
            RenderFrameGroups();
            ImGui::Separator();
            RenderFrameList();
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("PreviewPanel", ImVec2{0, 0}, true);
            RenderPreview();
            ImGui::EndChild();
        }
        else {
            ImGui::TextColored(ImVec4{1.0f, 0.3f, 0.3f, 1.0f}, "Failed to load sprite sheet");
        }
    }
    ImGui::End();
}

void SpriteSheetEditorPanel::Open(const std::string& spriteSheetPath) {
    m_CurrentPath = spriteSheetPath;
    m_IsOpen = true;

    m_SelectedFrameIndex = -1;
    m_SelectedGroupIndex = -1;
    m_SelectedCells.clear();
    m_PreviewZoom = 1.0f;
    m_PreviewOffset = Vector2{0.0f, 0.0f};
    m_IsEditingGroupName = false;

    UUID existingUUID = AssetRegistry::Instance().GetUUIDFromPath(spriteSheetPath);
    if (existingUUID.Get() != 0) {
        std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAsset(existingUUID);
        m_SpriteSheet = std::dynamic_pointer_cast<SpriteSheet>(asset);
    }
    else {
        std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAssetFromPath(spriteSheetPath);
        m_SpriteSheet = std::dynamic_pointer_cast<SpriteSheet>(asset);
    }

    if (m_SpriteSheet) {
        m_SelectedTextureUUID = m_SpriteSheet->GetTextureUUID();
        m_GridColumns = m_SpriteSheet->GetGridColumns();
        m_GridRows = m_SpriteSheet->GetGridRows();
        m_GridSpacingX = m_SpriteSheet->GetGridSpacingX();
        m_GridSpacingY = m_SpriteSheet->GetGridSpacingY();

        PX_LOG_INFO(EDITOR, "Loaded sprite sheet - Grid: %dx%d, Spacing: %dx%d", m_GridColumns, m_GridRows,
                    m_GridSpacingX, m_GridSpacingY);

        if (m_SelectedTextureUUID.Get() != 0) {
            AssetRegistry::Instance().LoadAsset(m_SelectedTextureUUID);
        }

        if (m_SpriteSheet->GetFrameGroupCount() > 0) {
            size_t expectedFrameCount = static_cast<size_t>(m_GridColumns * m_GridRows);
            if (m_SpriteSheet->GetFrameCount() != expectedFrameCount) {
                PX_LOG_WARNING(
                    EDITOR, "SpriteSheet has %zu frames but grid is %dx%d = %zu cells. Regenerating frames from grid.",
                    m_SpriteSheet->GetFrameCount(), m_GridColumns, m_GridRows, expectedFrameCount);
                UpdateFramesFromGrid();
            }
        }
    }
}

void SpriteSheetEditorPanel::Close() {
    m_IsOpen = false;
    m_SpriteSheet.reset();
    m_CurrentPath.clear();
    m_SelectedTextureUUID = UUID{0};
    m_SelectedFrameIndex = -1;
    m_SelectedGroupIndex = -1;
    m_SelectedCells.clear();
    m_PreviewZoom = 1.0f;
    m_PreviewOffset = Vector2{0.0f, 0.0f};
    m_IsEditingGroupName = false;
    m_GridSpacingX = 0;
    m_GridSpacingY = 0;
}

void SpriteSheetEditorPanel::RenderToolbar() {
    if (ImGui::Button("Save")) {
        Save();
    }

    ImGui::SameLine();
    ImGui::Text("| %s", m_SpriteSheet->GetName().c_str());
}

void SpriteSheetEditorPanel::RenderTextureSelector() {
    ImGui::Text("Texture");

    if (m_SelectedTextureUUID.Get() != 0) {
        std::shared_ptr<Asset> texAsset = AssetRegistry::Instance().GetAsset(m_SelectedTextureUUID);
        if (texAsset) {
            ImGui::Text("  %s", texAsset->GetName().c_str());
        }
        else {
            ImGui::TextColored(ImVec4{1.0f, 0.5f, 0.5f, 1.0f}, "  [Not Found]");
        }
    }
    else {
        ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "  None");
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_UUID")) {
            UUID droppedUUID = *static_cast<const UUID*>(payload->Data);
            std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAsset(droppedUUID);
            if (asset && asset->GetMetadata().type == AssetType::Texture) {
                m_SelectedTextureUUID = droppedUUID;
                m_SpriteSheet->SetTexture(m_SelectedTextureUUID);
                UpdateFramesFromGrid();
            }
        }
        else if (const ImGuiPayload* assetPayload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE")) {
            struct AssetInfo {
                std::string path;
                std::string filename;
                std::string extension;
                std::string type;
                int width;
                int height;
                size_t fileSize;
            };

            AssetInfo** draggedAssetPtr = static_cast<AssetInfo**>(const_cast<void*>(assetPayload->Data));
            if (draggedAssetPtr && *draggedAssetPtr) {
                AssetInfo* draggedAsset = *draggedAssetPtr;
                UUID textureUUID = AssetRegistry::Instance().GetUUIDFromPath(draggedAsset->path);
                if (textureUUID.Get() == 0) {
                    std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAssetFromPath(draggedAsset->path);
                    if (asset) {
                        textureUUID = asset->GetUUID();
                    }
                }

                if (textureUUID.Get() != 0) {
                    m_SelectedTextureUUID = textureUUID;
                    m_SpriteSheet->SetTexture(m_SelectedTextureUUID);
                    UpdateFramesFromGrid();
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear##texture")) {
        m_SelectedTextureUUID = UUID{0};
        m_SpriteSheet->SetTexture(UUID{0});
    }

    if (m_SelectedTextureUUID.Get() != 0) {
        std::shared_ptr<Asset> texAsset = AssetRegistry::Instance().GetAsset(m_SelectedTextureUUID);
        if (texAsset) {
            TextureAsset* tex = dynamic_cast<TextureAsset*>(texAsset.get());
            if (tex) {
                ImGui::Text("Size: %dx%d", tex->GetWidth(), tex->GetHeight());
            }
        }
    }
}

void SpriteSheetEditorPanel::RenderGridSettings() {
    ImGui::Text("Grid Slicing");

    int prevColumns = m_GridColumns;
    int prevRows = m_GridRows;
    int prevSpacingX = m_GridSpacingX;
    int prevSpacingY = m_GridSpacingY;

    ImGui::SliderInt("Columns", &m_GridColumns, 1, 32);
    ImGui::SliderInt("Rows", &m_GridRows, 1, 32);
    ImGui::SliderInt("Spacing X", &m_GridSpacingX, 0, 100);
    ImGui::SliderInt("Spacing Y", &m_GridSpacingY, 0, 100);

    if (m_GridColumns != prevColumns || m_GridRows != prevRows || m_GridSpacingX != prevSpacingX ||
        m_GridSpacingY != prevSpacingY)
    {
        m_SpriteSheet->SetGridSize(m_GridColumns, m_GridRows);
        m_SpriteSheet->SetGridSpacing(m_GridSpacingX, m_GridSpacingY);
        if (m_SelectionMode == SelectionMode::Grid) {
            UpdateFramesFromGrid();
        }
    }

    ImGui::Separator();
    ImGui::Text("Selection Mode");

    if (ImGui::RadioButton("Grid (All)", m_SelectionMode == SelectionMode::Grid)) {
        m_SelectionMode = SelectionMode::Grid;
        UpdateFramesFromGrid();
        m_SelectedGroupIndex = -1;
        m_IsEditingGroupName = false;
    }

    if (ImGui::RadioButton("Manual", m_SelectionMode == SelectionMode::Manual)) {
        m_SelectionMode = SelectionMode::Manual;
        m_SelectedCells.clear();
    }

    if (m_SelectionMode == SelectionMode::Manual) {
        ImGui::Text("Selected: %zu / %d", m_SelectedCells.size(), m_GridColumns * m_GridRows);

        if (ImGui::Button("Select All")) {
            m_SelectedCells.clear();
            for (int i = 0; i < m_GridColumns * m_GridRows; ++i) {
                m_SelectedCells.insert(i);
            }
            if (m_SelectedGroupIndex >= 0 &&
                m_SelectedGroupIndex < static_cast<int>(m_SpriteSheet->GetFrameGroupCount()))
            {
                FrameGroup* group = m_SpriteSheet->GetFrameGroup(static_cast<size_t>(m_SelectedGroupIndex));
                if (group) {
                    group->frameIndices.clear();
                    for (int cell : m_SelectedCells) {
                        group->frameIndices.push_back(static_cast<size_t>(cell));
                    }
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            m_SelectedCells.clear();
            if (m_SelectedGroupIndex >= 0 &&
                m_SelectedGroupIndex < static_cast<int>(m_SpriteSheet->GetFrameGroupCount()))
            {
                FrameGroup* group = m_SpriteSheet->GetFrameGroup(static_cast<size_t>(m_SelectedGroupIndex));
                if (group) {
                    group->frameIndices.clear();
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Invert")) {
            std::set<int> newSelection;
            for (int i = 0; i < m_GridColumns * m_GridRows; ++i) {
                if (m_SelectedCells.find(i) == m_SelectedCells.end()) {
                    newSelection.insert(i);
                }
            }
            m_SelectedCells = newSelection;
            if (m_SelectedGroupIndex >= 0 &&
                m_SelectedGroupIndex < static_cast<int>(m_SpriteSheet->GetFrameGroupCount()))
            {
                FrameGroup* group = m_SpriteSheet->GetFrameGroup(static_cast<size_t>(m_SelectedGroupIndex));
                if (group) {
                    group->frameIndices.clear();
                    for (int cell : m_SelectedCells) {
                        group->frameIndices.push_back(static_cast<size_t>(cell));
                    }
                }
            }
        }

        if (m_SpriteSheet->GetFrameGroupCount() > 0) {
            ImGui::BeginDisabled();
            ImGui::Button("Apply Selection");
            ImGui::EndDisabled();
            ImGui::TextColored(ImVec4{1.0f, 0.5f, 0.5f, 1.0f}, "  Cannot use with groups!");
            ImGui::TextColored(ImVec4{0.6f, 0.6f, 0.6f, 1.0f}, "  Delete all groups first");
        }
        else {
            if (ImGui::Button("Apply Selection")) {
                UpdateFramesFromSelection();
            }
        }
    }
    else {
        if (ImGui::Button("Auto-Generate All Frames")) {
            UpdateFramesFromGrid();
        }
    }

    ImGui::Separator();
    ImGui::Checkbox("Show Grid", &m_ShowGrid);
    ImGui::Checkbox("Show Pivots", &m_ShowPivots);
}

void SpriteSheetEditorPanel::RenderFrameList() {
    ImGui::Text("Frames (%zu)", m_SpriteSheet->GetFrames().size());

    ImGui::BeginChild("FrameListScroll", ImVec2{0, 0}, false);

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

        if (isSelected) {
            ImGui::PopStyleColor();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
}

void SpriteSheetEditorPanel::RenderPreview() {
    if (m_SelectedTextureUUID.Get() == 0) {
        ImVec2 regionSize = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize("No texture selected\nDrag a texture here");
        ImGui::SetCursorPosX((regionSize.x - textSize.x) * 0.5f);
        ImGui::SetCursorPosY((regionSize.y - textSize.y) * 0.5f);
        ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "No texture selected\nDrag a texture here");

        ImGui::SetCursorPos(ImVec2{0, 0});
        ImGui::InvisibleButton("dropzone", regionSize);

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* dropPayload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE")) {
                struct AssetInfo {
                    std::string path;
                    std::string filename;
                    std::string extension;
                    std::string type;
                    int width;
                    int height;
                    size_t fileSize;
                };

                AssetInfo** draggedAssetPtr = static_cast<AssetInfo**>(const_cast<void*>(dropPayload->Data));
                if (draggedAssetPtr && *draggedAssetPtr) {
                    AssetInfo* draggedAsset = *draggedAssetPtr;
                    UUID textureUUID = AssetRegistry::Instance().GetUUIDFromPath(draggedAsset->path);
                    if (textureUUID.Get() == 0) {
                        std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAssetFromPath(draggedAsset->path);
                        if (asset) {
                            textureUUID = asset->GetUUID();
                        }
                    }

                    if (textureUUID.Get() != 0) {
                        m_SelectedTextureUUID = textureUUID;
                        m_SpriteSheet->SetTexture(m_SelectedTextureUUID);
                        UpdateFramesFromGrid();
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        return;
    }

    std::shared_ptr<Asset> texAsset = AssetRegistry::Instance().GetAsset(m_SelectedTextureUUID);
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

    ImGui::Text("Zoom: %.1fx", m_PreviewZoom);
    if (ImGui::Button("-")) {
        m_PreviewZoom = fmaxf(0.1f, m_PreviewZoom - 0.1f);
    }
    ImGui::SameLine();
    if (ImGui::Button("+")) {
        m_PreviewZoom = fminf(5.0f, m_PreviewZoom + 0.1f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        m_PreviewZoom = 1.0f;
        m_PreviewOffset = Vector2{0.0f, 0.0f};
    }

    ImGui::Separator();

    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float displayWidth = static_cast<float>(texture.width) * m_PreviewZoom;
    float displayHeight = static_cast<float>(texture.height) * m_PreviewZoom;

    ImVec2 imagePos{canvasPos.x + m_PreviewOffset.x, canvasPos.y + m_PreviewOffset.y};

    ImGui::SetCursorScreenPos(imagePos);
    rlImGuiImageRect(&texture, static_cast<int>(displayWidth), static_cast<int>(displayHeight),
                     Rectangle{0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)});

    ImGui::SetCursorScreenPos(canvasPos);

    if (m_ShowGrid) {
        float spacingX = static_cast<float>(m_GridSpacingX) * m_PreviewZoom;
        float spacingY = static_cast<float>(m_GridSpacingY) * m_PreviewZoom;
        float cellWidth = displayWidth / static_cast<float>(m_GridColumns);
        float cellHeight = displayHeight / static_cast<float>(m_GridRows);
        float frameWidth = cellWidth - spacingX;
        float frameHeight = cellHeight - spacingY;
        float halfSpacingX = spacingX * 0.5f;
        float halfSpacingY = spacingY * 0.5f;

        for (int row = 0; row < m_GridRows; ++row) {
            for (int col = 0; col < m_GridColumns; ++col) {
                float x = imagePos.x + col * cellWidth + halfSpacingX;
                float y = imagePos.y + row * cellHeight + halfSpacingY;

                drawList->AddRect(ImVec2{x, y}, ImVec2{x + frameWidth, y + frameHeight}, IM_COL32(255, 255, 0, 200),
                                  0.0f, 0, 1.0f);
            }
        }
    }

    if (m_SelectionMode == SelectionMode::Manual) {
        float spacingX = static_cast<float>(m_GridSpacingX) * m_PreviewZoom;
        float spacingY = static_cast<float>(m_GridSpacingY) * m_PreviewZoom;
        float cellWidth = displayWidth / static_cast<float>(m_GridColumns);
        float cellHeight = displayHeight / static_cast<float>(m_GridRows);
        float frameWidth = cellWidth - spacingX;
        float frameHeight = cellHeight - spacingY;
        float halfSpacingX = spacingX * 0.5f;
        float halfSpacingY = spacingY * 0.5f;

        for (int cellIndex : m_SelectedCells) {
            int row = cellIndex / m_GridColumns;
            int col = cellIndex % m_GridColumns;

            float x = imagePos.x + col * cellWidth + halfSpacingX;
            float y = imagePos.y + row * cellHeight + halfSpacingY;

            drawList->AddRectFilled(ImVec2{x, y}, ImVec2{x + frameWidth, y + frameHeight}, IM_COL32(0, 255, 255, 80));

            drawList->AddRect(ImVec2{x, y}, ImVec2{x + frameWidth, y + frameHeight}, IM_COL32(0, 255, 255, 255), 0.0f,
                              0, 2.0f);
        }
    }

    if (m_SelectedFrameIndex >= 0 && m_SelectedFrameIndex < static_cast<int>(m_SpriteSheet->GetFrames().size())) {
        const SpriteFrame* frame = m_SpriteSheet->GetFrame(static_cast<size_t>(m_SelectedFrameIndex));
        if (frame) {
            float frameX = imagePos.x + frame->sourceRect.x * m_PreviewZoom;
            float frameY = imagePos.y + frame->sourceRect.y * m_PreviewZoom;
            float frameW = frame->sourceRect.width * m_PreviewZoom;
            float frameH = frame->sourceRect.height * m_PreviewZoom;

            drawList->AddRect(ImVec2{frameX, frameY}, ImVec2{frameX + frameW, frameY + frameH},
                              IM_COL32(0, 255, 0, 255), 0.0f, 0, 2.0f);

            if (m_ShowPivots) {
                float pivotX = frameX + frame->pivot.x * frameW;
                float pivotY = frameY + frame->pivot.y * frameH;

                drawList->AddCircleFilled(ImVec2{pivotX, pivotY}, 4.0f, IM_COL32(255, 0, 0, 255));
                drawList->AddLine(ImVec2{pivotX - 8, pivotY}, ImVec2{pivotX + 8, pivotY}, IM_COL32(255, 0, 0, 255),
                                  2.0f);
                drawList->AddLine(ImVec2{pivotX, pivotY - 8}, ImVec2{pivotX, pivotY + 8}, IM_COL32(255, 0, 0, 255),
                                  2.0f);
            }
        }
    }

    ImGui::InvisibleButton("canvas", canvasSize);
    if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        m_PreviewOffset.x += delta.x;
        m_PreviewOffset.y += delta.y;
    }

    if (m_SelectionMode == SelectionMode::Manual && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        float spacingX = static_cast<float>(m_GridSpacingX) * m_PreviewZoom;
        float spacingY = static_cast<float>(m_GridSpacingY) * m_PreviewZoom;
        float cellWidth = displayWidth / static_cast<float>(m_GridColumns);
        float cellHeight = displayHeight / static_cast<float>(m_GridRows);
        float frameWidth = cellWidth - spacingX;
        float frameHeight = cellHeight - spacingY;
        float halfSpacingX = spacingX * 0.5f;
        float halfSpacingY = spacingY * 0.5f;
        ImVec2 adjustedImagePos{imagePos.x + halfSpacingX, imagePos.y + halfSpacingY};
        int cellIndex = GetCellIndexFromMousePos(mousePos, adjustedImagePos, frameWidth, frameHeight,
                                                 cellWidth - frameWidth, cellHeight - frameHeight);

        if (cellIndex >= 0) {
            if (m_SelectedCells.find(cellIndex) != m_SelectedCells.end()) {
                m_SelectedCells.erase(cellIndex);
            }
            else {
                m_SelectedCells.insert(cellIndex);
            }

            if (m_SelectedGroupIndex >= 0 &&
                m_SelectedGroupIndex < static_cast<int>(m_SpriteSheet->GetFrameGroupCount()))
            {
                FrameGroup* group = m_SpriteSheet->GetFrameGroup(static_cast<size_t>(m_SelectedGroupIndex));
                if (group) {
                    group->frameIndices.clear();
                    for (int cell : m_SelectedCells) {
                        group->frameIndices.push_back(static_cast<size_t>(cell));
                    }
                }
            }
        }
    }
}

void SpriteSheetEditorPanel::UpdateFramesFromGrid() {
    if (m_SelectedTextureUUID.Get() == 0) {
        return;
    }

    std::shared_ptr<Asset> texAsset = AssetRegistry::Instance().GetAsset(m_SelectedTextureUUID);
    if (!texAsset) {
        return;
    }

    TextureAsset* tex = dynamic_cast<TextureAsset*>(texAsset.get());
    if (!tex) {
        return;
    }

    float texWidth = static_cast<float>(tex->GetWidth());
    float texHeight = static_cast<float>(tex->GetHeight());
    float cellWidth = texWidth / static_cast<float>(m_GridColumns);
    float cellHeight = texHeight / static_cast<float>(m_GridRows);
    float frameWidth = cellWidth - static_cast<float>(m_GridSpacingX);
    float frameHeight = cellHeight - static_cast<float>(m_GridSpacingY);
    float halfSpacingX = static_cast<float>(m_GridSpacingX) * 0.5f;
    float halfSpacingY = static_cast<float>(m_GridSpacingY) * 0.5f;

    std::vector<SpriteFrame> frames;
    for (int row = 0; row < m_GridRows; ++row) {
        for (int col = 0; col < m_GridColumns; ++col) {
            SpriteFrame frame{};
            char frameName[64];
            snprintf(frameName, sizeof(frameName), "frame_%d_%d", row, col);
            frame.name = frameName;
            frame.sourceRect =
                Rectangle{col * cellWidth + halfSpacingX, row * cellHeight + halfSpacingY, frameWidth, frameHeight};
            frame.pivot = Vector2{0.5f, 0.5f};
            frames.push_back(frame);
        }
    }

    m_SpriteSheet->SetFrames(frames);
}

void SpriteSheetEditorPanel::UpdateFramesFromSelection() {
    if (m_SelectedTextureUUID.Get() == 0) {
        return;
    }

    std::shared_ptr<Asset> texAsset = AssetRegistry::Instance().GetAsset(m_SelectedTextureUUID);
    if (!texAsset) {
        return;
    }

    TextureAsset* tex = dynamic_cast<TextureAsset*>(texAsset.get());
    if (!tex) {
        return;
    }

    float texWidth = static_cast<float>(tex->GetWidth());
    float texHeight = static_cast<float>(tex->GetHeight());
    float cellWidth = texWidth / static_cast<float>(m_GridColumns);
    float cellHeight = texHeight / static_cast<float>(m_GridRows);
    float frameWidth = cellWidth - static_cast<float>(m_GridSpacingX);
    float frameHeight = cellHeight - static_cast<float>(m_GridSpacingY);
    float halfSpacingX = static_cast<float>(m_GridSpacingX) * 0.5f;
    float halfSpacingY = static_cast<float>(m_GridSpacingY) * 0.5f;

    std::vector<SpriteFrame> frames;
    for (int cellIndex : m_SelectedCells) {
        int row = cellIndex / m_GridColumns;
        int col = cellIndex % m_GridColumns;

        SpriteFrame frame{};
        char frameName[64];
        snprintf(frameName, sizeof(frameName), "frame_%d_%d", row, col);
        frame.name = frameName;
        frame.sourceRect =
            Rectangle{col * cellWidth + halfSpacingX, row * cellHeight + halfSpacingY, frameWidth, frameHeight};
        frame.pivot = Vector2{0.5f, 0.5f};
        frames.push_back(frame);
    }

    m_SpriteSheet->SetFrames(frames);
}

void SpriteSheetEditorPanel::RenderFrameGroups() {
    ImGui::TextColored(ImVec4{0.8f, 0.6f, 0.4f, 1.0f}, "Frame Groups");
    ImGui::Text("Groups: %zu", m_SpriteSheet->GetFrameGroupCount());

    if (m_SelectionMode == SelectionMode::Manual && !m_SelectedCells.empty()) {
        ImGui::InputText("New Group", m_NewGroupName, sizeof(m_NewGroupName));
        if (ImGui::Button("Create Group from Selection", ImVec2{-1, 0})) {
            CreateGroupFromSelection();
        }
        ImGui::Separator();
    }

    ImGui::BeginChild("FrameGroupsScroll", ImVec2{0, m_SelectedGroupIndex >= 0 ? 100.0f : 150.0f}, false);

    int groupToDelete = -1;
    size_t groupCount = m_SpriteSheet->GetFrameGroupCount();

    for (size_t i = 0; i < groupCount; ++i) {
        ImGui::PushID(static_cast<int>(i));

        const FrameGroup* group = m_SpriteSheet->GetFrameGroup(i);
        if (!group) {
            ImGui::PopID();
            continue;
        }

        bool isSelected = (static_cast<int>(i) == m_SelectedGroupIndex);
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{0.8f, 0.6f, 0.4f, 1.0f});
        }

        if (isSelected && m_IsEditingGroupName) {
            ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##edit", m_EditGroupName, sizeof(m_EditGroupName),
                                 ImGuiInputTextFlags_EnterReturnsTrue))
            {
                FrameGroup* editGroup = m_SpriteSheet->GetFrameGroup(i);
                if (editGroup) {
                    editGroup->name = m_EditGroupName;
                }
                m_IsEditingGroupName = false;
            }
            if (!ImGui::IsItemActive() && !ImGui::IsItemFocused()) {
                m_IsEditingGroupName = false;
            }
        }
        else {
            char label[128];
            snprintf(label, sizeof(label), "%s (%zu frames)", group->name.c_str(), group->frameIndices.size());

            if (ImGui::Selectable(label, isSelected)) {
                m_SelectedGroupIndex = static_cast<int>(i);

                const FrameGroup* currentGroup = m_SpriteSheet->GetFrameGroup(i);
                if (currentGroup) {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
                    strncpy(m_EditGroupName, currentGroup->name.c_str(), sizeof(m_EditGroupName) - 1);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
                    m_EditGroupName[sizeof(m_EditGroupName) - 1] = '\0';

                    m_SelectedCells.clear();
                    for (size_t cellIdx : currentGroup->frameIndices) {
                        m_SelectedCells.insert(static_cast<int>(cellIdx));
                    }

                    if (m_SelectionMode != SelectionMode::Manual) {
                        m_SelectionMode = SelectionMode::Manual;
                    }
                }
            }

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Rename")) {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
                    strncpy(m_EditGroupName, group->name.c_str(), sizeof(m_EditGroupName) - 1);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
                    m_EditGroupName[sizeof(m_EditGroupName) - 1] = '\0';
                    m_IsEditingGroupName = true;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Delete")) {
                    groupToDelete = static_cast<int>(i);
                }
                ImGui::EndPopup();
            }

            if (isSelected && ImGui::IsKeyPressed(ImGuiKey_F2)) {
                m_IsEditingGroupName = true;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
                strncpy(m_EditGroupName, group->name.c_str(), sizeof(m_EditGroupName) - 1);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
                m_EditGroupName[sizeof(m_EditGroupName) - 1] = '\0';
            }
        }

        if (isSelected) {
            ImGui::PopStyleColor();
        }

        if (!m_IsEditingGroupName || !isSelected) {
            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                groupToDelete = static_cast<int>(i);
            }
        }

        ImGui::PopID();
    }

    if (groupToDelete >= 0) {
        m_SpriteSheet->RemoveFrameGroup(static_cast<size_t>(groupToDelete));
        if (m_SelectedGroupIndex == groupToDelete) {
            m_SelectedGroupIndex = -1;
            m_IsEditingGroupName = false;
        }
        else if (m_SelectedGroupIndex > groupToDelete) {
            m_SelectedGroupIndex--;
        }
    }

    ImGui::EndChild();

    if (m_SelectedGroupIndex >= 0 && m_SelectedGroupIndex < static_cast<int>(m_SpriteSheet->GetFrameGroupCount())) {
        const FrameGroup* selectedGroup = m_SpriteSheet->GetFrameGroup(static_cast<size_t>(m_SelectedGroupIndex));
        if (selectedGroup) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "Selected Group:");
            ImGui::Text("  %s (%zu cells)", selectedGroup->name.c_str(), selectedGroup->frameIndices.size());
        }
    }
}

void SpriteSheetEditorPanel::CreateGroupFromSelection() {
    if (m_SelectedCells.empty()) {
        return;
    }

    if (m_SpriteSheet->GetFrameCount() != static_cast<size_t>(m_GridColumns * m_GridRows)) {
        UpdateFramesFromGrid();
    }

    FrameGroup group{};
    group.name = m_NewGroupName;

    for (int cellIndex : m_SelectedCells) {
        group.frameIndices.push_back(static_cast<size_t>(cellIndex));
    }

    m_SpriteSheet->AddFrameGroup(group);

    snprintf(m_NewGroupName, sizeof(m_NewGroupName), "NewGroup%zu", m_SpriteSheet->GetFrameGroupCount() + 1);
}

int SpriteSheetEditorPanel::GetCellIndexFromMousePos(const ImVec2& mousePos, const ImVec2& imagePos, float cellWidth,
                                                     float cellHeight, float spacingX, float spacingY) {
    float relX = mousePos.x - imagePos.x;
    float relY = mousePos.y - imagePos.y;

    int col = static_cast<int>(relX / (cellWidth + spacingX));
    int row = static_cast<int>(relY / (cellHeight + spacingY));

    if (col < 0 || col >= m_GridColumns || row < 0 || row >= m_GridRows) {
        return -1;
    }

    float cellStartX = col * (cellWidth + spacingX);
    float cellStartY = row * (cellHeight + spacingY);

    if (relX >= cellStartX && relX < cellStartX + cellWidth && relY >= cellStartY && relY < cellStartY + cellHeight) {
        return row * m_GridColumns + col;
    }

    return -1;
}

void SpriteSheetEditorPanel::Save() {
    if (!m_SpriteSheet) {
        PX_LOG_ERROR(EDITOR, "Cannot save: no sprite sheet");
        return;
    }

    if (m_CurrentPath.empty()) {
        PX_LOG_ERROR(EDITOR, "Cannot save: no path set (m_CurrentPath is empty)");
        return;
    }

    m_SpriteSheet->SetGridSize(m_GridColumns, m_GridRows);
    m_SpriteSheet->SetGridSpacing(m_GridSpacingX, m_GridSpacingY);

    PX_LOG_INFO(EDITOR, "Attempting to save sprite sheet to: %s", m_CurrentPath.c_str());
    PX_LOG_INFO(EDITOR, "Before save - Frames: %zu, Groups: %zu, Spacing: %dx%d", m_SpriteSheet->GetFrameCount(),
                m_SpriteSheet->GetFrameGroupCount(), m_GridSpacingX, m_GridSpacingY);

    if (AnimationSerializer::SerializeSpriteSheet(*m_SpriteSheet, m_CurrentPath)) {
        PX_LOG_INFO(EDITOR, "Saved sprite sheet successfully: %s", m_CurrentPath.c_str());

        UUID spriteSheetUUID = m_SpriteSheet->GetUUID();
        AssetRegistry::Instance().ReimportAsset(m_CurrentPath);

        std::shared_ptr<Asset> reloadedAsset = AssetRegistry::Instance().LoadAsset(spriteSheetUUID);
        m_SpriteSheet = std::dynamic_pointer_cast<SpriteSheet>(reloadedAsset);

        if (m_SpriteSheet) {
            m_SelectedTextureUUID = m_SpriteSheet->GetTextureUUID();
            m_GridColumns = m_SpriteSheet->GetGridColumns();
            m_GridRows = m_SpriteSheet->GetGridRows();
            m_GridSpacingX = m_SpriteSheet->GetGridSpacingX();
            m_GridSpacingY = m_SpriteSheet->GetGridSpacingY();

            PX_LOG_INFO(EDITOR, "After reload - Frames: %zu, Groups: %zu, Grid: %dx%d, Spacing: %dx%d",
                        m_SpriteSheet->GetFrameCount(), m_SpriteSheet->GetFrameGroupCount(), m_GridColumns, m_GridRows,
                        m_GridSpacingX, m_GridSpacingY);

            if (m_SpriteSheet->GetFrameGroupCount() > 0) {
                size_t expectedFrameCount = static_cast<size_t>(m_GridColumns * m_GridRows);
                if (m_SpriteSheet->GetFrameCount() != expectedFrameCount) {
                    PX_LOG_WARNING(
                        EDITOR,
                        "After reload: SpriteSheet has %zu frames but grid is %dx%d = %zu cells. Regenerating frames.",
                        m_SpriteSheet->GetFrameCount(), m_GridColumns, m_GridRows, expectedFrameCount);
                    UpdateFramesFromGrid();
                }
            }
        }
        else {
            PX_LOG_ERROR(EDITOR, "Failed to reload sprite sheet after save");
        }
    }
    else {
        PX_LOG_ERROR(EDITOR, "Failed to save sprite sheet: %s", m_CurrentPath.c_str());
    }
}

} // namespace PiiXeL

#endif
