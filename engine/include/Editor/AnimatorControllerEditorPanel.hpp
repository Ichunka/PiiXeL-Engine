#ifndef PIIXELENGINE_ANIMATORCONTROLLEREDITORPANEL_HPP
#define PIIXELENGINE_ANIMATORCONTROLLEREDITORPANEL_HPP

#ifdef BUILD_WITH_EDITOR

#include "Animation/AnimatorController.hpp"
#include "Components/UUID.hpp"
#include <string>
#include <memory>
#include <vector>

struct ImVec2;

namespace PiiXeL {

class AnimatorControllerEditorPanel {
public:
    AnimatorControllerEditorPanel() = default;
    ~AnimatorControllerEditorPanel() = default;

    void Render();
    void RenderInspector();
    void RenderToolbar();
    void RenderParametersPanel();
    void RenderStateGraph();
    void Open(const std::string& controllerPath);
    void Close();

    [[nodiscard]] bool IsOpen() const { return m_IsOpen; }
    [[nodiscard]] bool HasSelection() const { return m_SelectedStateIndex >= 0 || m_SelectedTransitionIndex >= 0; }
    [[nodiscard]] std::shared_ptr<AnimatorController> GetController() const { return m_Controller; }

private:

    void RenderStateNode(size_t stateIndex, const ImVec2& canvasPos, const ImVec2& scrolling);
    void RenderTransitions(const ImVec2& canvasPos, const ImVec2& scrolling);

    void CreateNewState(const ImVec2& position);
    void DeleteState(size_t stateIndex);
    void CreateTransition(size_t fromIndex, size_t toIndex);
    void DeleteTransition(size_t transitionIndex);

    void Save();

    ImVec2 GetStateNodePosition(size_t stateIndex) const;
    int GetStateIndexAtPosition(const ImVec2& pos, const ImVec2& canvasPos, const ImVec2& scrolling);
    int GetTransitionIndexAtPosition(const ImVec2& mousePos, const ImVec2& canvasPos, const ImVec2& scrolling);

    bool m_IsOpen{false};
    std::string m_CurrentPath;
    std::shared_ptr<AnimatorController> m_Controller;

    int m_SelectedStateIndex{-1};
    int m_SelectedTransitionIndex{-1};
    int m_DraggingStateIndex{-1};
    int m_CreatingTransitionFromIndex{-1};

    float m_GraphScrollingX{0.0f};
    float m_GraphScrollingY{0.0f};
    float m_ContextMenuPosX{0.0f};
    float m_ContextMenuPosY{0.0f};
    bool m_OpenContextMenu{false};

    char m_NewParameterName[64]{"NewParameter"};
    int m_NewParameterType{0};

    char m_NewStateName[64]{"NewState"};
};

} // namespace PiiXeL

#endif

#endif
