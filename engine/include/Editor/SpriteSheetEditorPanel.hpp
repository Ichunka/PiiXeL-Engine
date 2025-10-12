#ifndef PIIXELENGINE_SPRITESHEETEDITORPANEL_HPP
#define PIIXELENGINE_SPRITESHEETEDITORPANEL_HPP

#ifdef BUILD_WITH_EDITOR

#include "Animation/SpriteSheet.hpp"
#include "Components/UUID.hpp"
#include <string>
#include <memory>
#include <vector>
#include <set>

struct ImVec2;

namespace PiiXeL {

enum class SelectionMode {
    Grid,
    Manual
};

class SpriteSheetEditorPanel {
public:
    SpriteSheetEditorPanel() = default;
    ~SpriteSheetEditorPanel() = default;

    void Render();
    void Open(const std::string& spriteSheetPath);
    void Close();

    [[nodiscard]] bool IsOpen() const { return m_IsOpen; }

private:
    void RenderToolbar();
    void RenderTextureSelector();
    void RenderGridSettings();
    void RenderFrameGroups();
    void RenderFrameList();
    void RenderPreview();

    void UpdateFramesFromGrid();
    void UpdateFramesFromSelection();
    void CreateGroupFromSelection();
    void Save();

    int GetCellIndexFromMousePos(const ImVec2& mousePos, const ImVec2& imagePos, float cellWidth, float cellHeight);

    bool m_IsOpen{false};
    std::string m_CurrentPath;
    std::shared_ptr<SpriteSheet> m_SpriteSheet;

    UUID m_SelectedTextureUUID{0};
    int m_GridColumns{1};
    int m_GridRows{1};

    SelectionMode m_SelectionMode{SelectionMode::Grid};
    std::set<int> m_SelectedCells;

    float m_PreviewZoom{1.0f};
    Vector2 m_PreviewOffset{0.0f, 0.0f};
    int m_SelectedFrameIndex{-1};
    int m_SelectedGroupIndex{-1};

    bool m_ShowGrid{true};
    bool m_ShowPivots{true};

    char m_NewGroupName[64]{"NewGroup"};
    char m_EditGroupName[64]{""};
    bool m_IsEditingGroupName{false};
};

} // namespace PiiXeL

#endif

#endif
