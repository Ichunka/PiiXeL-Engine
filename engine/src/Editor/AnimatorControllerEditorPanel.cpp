#ifdef BUILD_WITH_EDITOR

#include "Editor/AnimatorControllerEditorPanel.hpp"
#include "Animation/AnimationSerializer.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/Asset.hpp"
#include "Resources/TextureAsset.hpp"
#include "Animation/AnimationClip.hpp"
#include <imgui.h>
#include <raylib.h>
#include <cmath>

namespace PiiXeL {

constexpr float NODE_WIDTH = 180.0f;
constexpr float NODE_HEIGHT = 80.0f;
constexpr float NODE_ROUNDING = 4.0f;
constexpr float CONNECTOR_RADIUS = 8.0f;

void AnimatorControllerEditorPanel::Render() {
    if (!m_IsOpen) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2{1200, 800}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Animator Controller Editor", &m_IsOpen)) {
        ImGui::End();
        return;
    }

    if (!m_Controller) {
        ImGui::TextColored(ImVec4{1.0f, 0.5f, 0.5f, 1.0f}, "No controller loaded");
        ImGui::End();
        return;
    }

    RenderToolbar();
    ImGui::Separator();

    ImGui::BeginChild("MainContent", ImVec2{0, 0}, false);
    {
        ImGui::Columns(3, "AnimatorColumns", true);
        ImGui::SetColumnWidth(0, 250);
        ImGui::SetColumnWidth(2, 250);

        ImGui::BeginChild("ParametersPanel", ImVec2{0, 0}, true);
        RenderParametersPanel();
        ImGui::EndChild();

        ImGui::NextColumn();

        ImGui::BeginChild("StateGraphPanel", ImVec2{0, 0}, true);
        RenderStateGraph();
        ImGui::EndChild();

        ImGui::NextColumn();

        ImGui::BeginChild("InspectorPanel", ImVec2{0, 0}, true);
        RenderInspector();
        ImGui::EndChild();

        ImGui::Columns(1);
    }
    ImGui::EndChild();

    ImGui::End();
}

void AnimatorControllerEditorPanel::Open(const std::string& controllerPath) {
    m_CurrentPath = controllerPath;

    UUID existingUUID = AssetRegistry::Instance().GetUUIDFromPath(controllerPath);
    if (existingUUID.Get() != 0) {
        std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAsset(existingUUID);
        m_Controller = std::dynamic_pointer_cast<AnimatorController>(asset);
    } else {
        std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAssetFromPath(controllerPath);
        m_Controller = std::dynamic_pointer_cast<AnimatorController>(asset);
    }

    if (!m_Controller) {
        TraceLog(LOG_ERROR, "Failed to load AnimatorController: %s", controllerPath.c_str());
    }

    m_IsOpen = true;
    m_SelectedStateIndex = -1;
    m_SelectedTransitionIndex = -1;
    m_DraggingStateIndex = -1;
    m_CreatingTransitionFromIndex = -1;
    m_GraphScrollingX = 0.0f;
    m_GraphScrollingY = 0.0f;
}

void AnimatorControllerEditorPanel::Close() {
    m_IsOpen = false;
    m_Controller.reset();
    m_CurrentPath.clear();
}

void AnimatorControllerEditorPanel::RenderToolbar() {
    ImGui::Text("Controller: %s", m_Controller->GetName().c_str());
    ImGui::SameLine();

    if (ImGui::Button("Save")) {
        Save();
    }

    ImGui::SameLine();
    ImGui::Text("| States: %zu | Transitions: %zu | Parameters: %zu",
                m_Controller->GetStates().size(),
                m_Controller->GetTransitions().size(),
                m_Controller->GetParameters().size());
}

void AnimatorControllerEditorPanel::RenderParametersPanel() {
    ImGui::TextColored(ImVec4{0.4f, 0.8f, 0.6f, 1.0f}, "Parameters");
    ImGui::Separator();

    ImGui::InputText("Name", m_NewParameterName, sizeof(m_NewParameterName));

    const char* paramTypes[] = {"Float", "Int", "Bool", "Trigger"};
    ImGui::Combo("Type", &m_NewParameterType, paramTypes, 4);

    if (ImGui::Button("Add Parameter", ImVec2{-1, 0})) {
        AnimatorParameter param{};
        param.name = m_NewParameterName;
        param.type = static_cast<AnimatorParameterType>(m_NewParameterType);

        switch (param.type) {
            case AnimatorParameterType::Float:
                param.defaultValue = 0.0f;
                break;
            case AnimatorParameterType::Int:
                param.defaultValue = 0;
                break;
            case AnimatorParameterType::Bool:
            case AnimatorParameterType::Trigger:
                param.defaultValue = false;
                break;
        }

        m_Controller->AddParameter(param);
        snprintf(m_NewParameterName, sizeof(m_NewParameterName), "NewParameter%zu", m_Controller->GetParameters().size() + 1);
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "Current Parameters:");

    ImGui::BeginChild("ParametersList", ImVec2{0, 0}, false);
    const std::vector<AnimatorParameter>& params = m_Controller->GetParameters();
    int paramToDelete = -1;

    for (size_t i = 0; i < params.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));

        const char* typeStr = "Unknown";
        switch (params[i].type) {
            case AnimatorParameterType::Float: typeStr = "Float"; break;
            case AnimatorParameterType::Int: typeStr = "Int"; break;
            case AnimatorParameterType::Bool: typeStr = "Bool"; break;
            case AnimatorParameterType::Trigger: typeStr = "Trigger"; break;
        }

        ImGui::Text("%s [%s]", params[i].name.c_str(), typeStr);
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            paramToDelete = static_cast<int>(i);
        }

        ImGui::PopID();
    }

    if (paramToDelete >= 0 && paramToDelete < static_cast<int>(params.size())) {
        m_Controller->RemoveParameter(params[paramToDelete].name);
    }

    ImGui::EndChild();
}

void AnimatorControllerEditorPanel::RenderStateGraph() {
    ImGui::TextColored(ImVec4{0.6f, 0.8f, 1.0f, 1.0f}, "State Graph");
    ImGui::SameLine();
    ImGui::TextDisabled("(Right-click to create state)");
    ImGui::Separator();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    const ImVec2 canvasEnd = ImVec2{canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y};

    drawList->AddRectFilled(canvasPos, canvasEnd, IM_COL32(30, 30, 30, 255));
    drawList->AddRect(canvasPos, canvasEnd, IM_COL32(60, 60, 60, 255));

    ImGui::InvisibleButton("Canvas", canvasSize);
    const bool isHovered = ImGui::IsItemHovered();
    const ImVec2 mousePos = ImGui::GetMousePos();
    const ImVec2 mousePosInCanvas = ImVec2{mousePos.x - canvasPos.x - m_GraphScrollingX, mousePos.y - canvasPos.y - m_GraphScrollingY};

    if (isHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
        m_GraphScrollingX += delta.x;
        m_GraphScrollingY += delta.y;
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
    }

    if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        m_OpenContextMenu = true;
        m_ContextMenuPosX = mousePosInCanvas.x;
        m_ContextMenuPosY = mousePosInCanvas.y;
    }

    drawList->PushClipRect(canvasPos, canvasEnd, true);

    ImVec2 scrolling = ImVec2{m_GraphScrollingX, m_GraphScrollingY};
    RenderTransitions(canvasPos, scrolling);

    const std::vector<AnimatorState>& states = m_Controller->GetStates();
    for (size_t i = 0; i < states.size(); ++i) {
        RenderStateNode(i, canvasPos, scrolling);
    }

    if (m_CreatingTransitionFromIndex >= 0) {
        const AnimatorState& fromState = states[m_CreatingTransitionFromIndex];
        ImVec2 fromPos = ImVec2{
            fromState.editorPosition.x + NODE_WIDTH / 2.0f,
            fromState.editorPosition.y + NODE_HEIGHT / 2.0f
        };
        ImVec2 screenFromPos = ImVec2{
            canvasPos.x + m_GraphScrollingX + fromPos.x,
            canvasPos.y + m_GraphScrollingY + fromPos.y
        };
        drawList->AddLine(screenFromPos, mousePos, IM_COL32(100, 200, 100, 255), 2.0f);
    }

    drawList->PopClipRect();

    if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        int clickedState = GetStateIndexAtPosition(mousePosInCanvas, canvasPos, scrolling);

        if (m_CreatingTransitionFromIndex >= 0) {
            if (clickedState >= 0 && clickedState != m_CreatingTransitionFromIndex) {
                CreateTransition(m_CreatingTransitionFromIndex, clickedState);
            }
            m_CreatingTransitionFromIndex = -1;
        } else if (clickedState >= 0) {
            m_SelectedStateIndex = clickedState;
            m_SelectedTransitionIndex = -1;
            m_DraggingStateIndex = clickedState;
        } else {
            int clickedTransition = GetTransitionIndexAtPosition(mousePos, canvasPos, scrolling);
            if (clickedTransition >= 0) {
                m_SelectedTransitionIndex = clickedTransition;
                m_SelectedStateIndex = -1;
            } else {
                m_SelectedStateIndex = -1;
                m_SelectedTransitionIndex = -1;
            }
        }
    }

    if (m_DraggingStateIndex >= 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.0f)) {
        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 1.0f);
        AnimatorState& state = const_cast<AnimatorState&>(states[m_DraggingStateIndex]);
        state.editorPosition.x += delta.x;
        state.editorPosition.y += delta.y;
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_DraggingStateIndex = -1;
    }

    if (m_OpenContextMenu) {
        ImGui::OpenPopup("GraphContextMenu");
        m_OpenContextMenu = false;
    }

    if (ImGui::BeginPopup("GraphContextMenu")) {
        ImGui::InputText("State Name", m_NewStateName, sizeof(m_NewStateName));
        if (ImGui::MenuItem("Create State")) {
            CreateNewState(ImVec2{m_ContextMenuPosX, m_ContextMenuPosY});
        }
        ImGui::EndPopup();
    }
}

void AnimatorControllerEditorPanel::RenderStateNode(size_t stateIndex, const ImVec2& canvasPos, const ImVec2& scrolling) {
    const AnimatorState& state = m_Controller->GetStates()[stateIndex];
    const bool isSelected = (static_cast<int>(stateIndex) == m_SelectedStateIndex);
    const bool isDefault = (state.name == m_Controller->GetDefaultState());

    ImVec2 nodePos = ImVec2{
        canvasPos.x + scrolling.x + state.editorPosition.x,
        canvasPos.y + scrolling.y + state.editorPosition.y
    };
    ImVec2 nodeEnd = ImVec2{nodePos.x + NODE_WIDTH, nodePos.y + NODE_HEIGHT};

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 nodeColor = isDefault ? IM_COL32(100, 150, 200, 255) : IM_COL32(60, 60, 80, 255);
    ImU32 borderColor = isSelected ? IM_COL32(255, 200, 100, 255) : IM_COL32(100, 100, 120, 255);

    drawList->AddRectFilled(nodePos, nodeEnd, nodeColor, NODE_ROUNDING);
    drawList->AddRect(nodePos, nodeEnd, borderColor, NODE_ROUNDING, 0, isSelected ? 3.0f : 2.0f);

    ImVec2 textPos = ImVec2{nodePos.x + 10, nodePos.y + 10};
    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), state.name.c_str());

    if (state.animationClipUUID.Get() != 0) {
        std::shared_ptr<Asset> clipAsset = AssetRegistry::Instance().GetAsset(state.animationClipUUID);
        std::shared_ptr<AnimationClip> clip = std::dynamic_pointer_cast<AnimationClip>(clipAsset);
        if (clip) {
            ImVec2 clipTextPos = ImVec2{nodePos.x + 10, nodePos.y + 30};
            drawList->AddText(clipTextPos, IM_COL32(180, 180, 180, 255), clip->GetName().c_str());
        }
    }

    ImVec2 speedTextPos = ImVec2{nodePos.x + 10, nodePos.y + 50};
    char speedText[32];
    snprintf(speedText, sizeof(speedText), "Speed: %.2fx", state.speed);
    drawList->AddText(speedTextPos, IM_COL32(150, 150, 150, 255), speedText);

    ImVec2 connectorPos = ImVec2{nodeEnd.x, nodePos.y + NODE_HEIGHT / 2.0f};
    drawList->AddCircleFilled(connectorPos, CONNECTOR_RADIUS, IM_COL32(100, 200, 100, 255));
}

void AnimatorControllerEditorPanel::RenderTransitions(const ImVec2& canvasPos, const ImVec2& scrolling) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const std::vector<AnimatorTransition>& transitions = m_Controller->GetTransitions();
    const std::vector<AnimatorState>& states = m_Controller->GetStates();

    for (size_t i = 0; i < transitions.size(); ++i) {
        const AnimatorTransition& trans = transitions[i];

        int fromIndex = -1;
        int toIndex = -1;

        for (size_t j = 0; j < states.size(); ++j) {
            if (states[j].name == trans.fromState) fromIndex = static_cast<int>(j);
            if (states[j].name == trans.toState) toIndex = static_cast<int>(j);
        }

        if (fromIndex < 0 || toIndex < 0) continue;

        const AnimatorState& fromState = states[fromIndex];
        const AnimatorState& toState = states[toIndex];

        ImVec2 fromPos = ImVec2{
            canvasPos.x + scrolling.x + fromState.editorPosition.x + NODE_WIDTH,
            canvasPos.y + scrolling.y + fromState.editorPosition.y + NODE_HEIGHT / 2.0f
        };
        ImVec2 toPos = ImVec2{
            canvasPos.x + scrolling.x + toState.editorPosition.x,
            canvasPos.y + scrolling.y + toState.editorPosition.y + NODE_HEIGHT / 2.0f
        };

        const bool isSelected = (static_cast<int>(i) == m_SelectedTransitionIndex);
        ImU32 lineColor = isSelected ? IM_COL32(255, 200, 100, 255) : IM_COL32(150, 150, 150, 255);
        float lineThickness = isSelected ? 3.0f : 2.0f;

        drawList->AddLine(fromPos, toPos, lineColor, lineThickness);

        ImVec2 direction = ImVec2{toPos.x - fromPos.x, toPos.y - fromPos.y};
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction.x /= length;
            direction.y /= length;

            ImVec2 arrowTip = ImVec2{toPos.x - direction.x * 15.0f, toPos.y - direction.y * 15.0f};
            ImVec2 perpendicular = ImVec2{-direction.y, direction.x};
            ImVec2 arrowLeft = ImVec2{arrowTip.x - direction.x * 10.0f + perpendicular.x * 8.0f, arrowTip.y - direction.y * 10.0f + perpendicular.y * 8.0f};
            ImVec2 arrowRight = ImVec2{arrowTip.x - direction.x * 10.0f - perpendicular.x * 8.0f, arrowTip.y - direction.y * 10.0f - perpendicular.y * 8.0f};

            drawList->AddTriangleFilled(arrowTip, arrowLeft, arrowRight, lineColor);
        }
    }
}

void AnimatorControllerEditorPanel::RenderInspector() {
    ImGui::TextColored(ImVec4{0.8f, 0.6f, 0.8f, 1.0f}, "Inspector");
    ImGui::Separator();

    if (m_SelectedStateIndex >= 0) {
        const std::vector<AnimatorState>& states = m_Controller->GetStates();
        if (m_SelectedStateIndex < static_cast<int>(states.size())) {
            AnimatorState& state = const_cast<AnimatorState&>(states[m_SelectedStateIndex]);

            ImGui::Text("State: %s", state.name.c_str());
            ImGui::Separator();

            char nameBuffer[64];
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
            strncpy(nameBuffer, state.name.c_str(), sizeof(nameBuffer) - 1);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
            if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
                state.name = nameBuffer;
            }

            ImGui::DragFloat("Speed", &state.speed, 0.1f, 0.0f, 10.0f);

            if (ImGui::Button("Set as Default State", ImVec2{-1, 0})) {
                m_Controller->SetDefaultState(state.name);
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "Animation Clip:");

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIM_ANIM")) {
                    UUID assetUUID = *static_cast<const UUID*>(payload->Data);
                    std::shared_ptr<Asset> clipAsset = AssetRegistry::Instance().GetAsset(assetUUID);
                    std::shared_ptr<AnimationClip> clip = std::dynamic_pointer_cast<AnimationClip>(clipAsset);
                    if (clip) {
                        state.animationClipUUID = assetUUID;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (state.animationClipUUID.Get() != 0) {
                std::shared_ptr<Asset> clipAsset = AssetRegistry::Instance().GetAsset(state.animationClipUUID);
                std::shared_ptr<AnimationClip> clip = std::dynamic_pointer_cast<AnimationClip>(clipAsset);
                if (clip) {
                    ImGui::Text("  %s", clip->GetName().c_str());
                    if (ImGui::Button("Clear##ClipClear")) {
                        state.animationClipUUID = UUID{0};
                    }
                } else {
                    ImGui::TextColored(ImVec4{1.0f, 0.5f, 0.5f, 1.0f}, "  [Missing]");
                }
            } else {
                ImGui::TextDisabled("  Drag AnimationClip here");
            }

            ImGui::Separator();
            if (ImGui::Button("Create Transition", ImVec2{-1, 0})) {
                m_CreatingTransitionFromIndex = m_SelectedStateIndex;
            }

            ImGui::Separator();
            if (ImGui::Button("Delete State", ImVec2{-1, 0})) {
                DeleteState(m_SelectedStateIndex);
                m_SelectedStateIndex = -1;
            }
        }
    } else if (m_SelectedTransitionIndex >= 0) {
        const std::vector<AnimatorTransition>& transitions = m_Controller->GetTransitions();
        if (m_SelectedTransitionIndex < static_cast<int>(transitions.size())) {
            AnimatorTransition& trans = const_cast<AnimatorTransition&>(transitions[m_SelectedTransitionIndex]);

            ImGui::Text("Transition: %s -> %s", trans.fromState.c_str(), trans.toState.c_str());
            ImGui::Separator();

            ImGui::Checkbox("Has Exit Time", &trans.hasExitTime);
            if (trans.hasExitTime) {
                ImGui::DragFloat("Exit Time", &trans.exitTime, 0.01f, 0.0f, 1.0f);
            }
            ImGui::DragFloat("Duration", &trans.transitionDuration, 0.01f, 0.0f, 1.0f);

            ImGui::Separator();
            ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "Conditions:");

            std::vector<TransitionCondition>& conditions = trans.conditions;
            int conditionToDelete = -1;

            for (size_t i = 0; i < conditions.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));
                TransitionCondition& cond = conditions[i];

                ImGui::Text("%s", cond.parameterName.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) {
                    conditionToDelete = static_cast<int>(i);
                }

                const char* condTypes[] = {"Equals", "NotEquals", "Greater", "Less"};
                int condType = static_cast<int>(cond.type);
                if (ImGui::Combo("Type", &condType, condTypes, 4)) {
                    cond.type = static_cast<TransitionConditionType>(condType);
                }

                ImGui::PopID();
            }

            if (conditionToDelete >= 0) {
                conditions.erase(conditions.begin() + conditionToDelete);
            }

            ImGui::Separator();
            if (ImGui::Button("Add Condition", ImVec2{-1, 0})) {
                if (!m_Controller->GetParameters().empty()) {
                    TransitionCondition newCond{};
                    newCond.parameterName = m_Controller->GetParameters()[0].name;
                    newCond.type = TransitionConditionType::Equals;
                    newCond.value = false;
                    conditions.push_back(newCond);
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Delete Transition", ImVec2{-1, 0})) {
                DeleteTransition(m_SelectedTransitionIndex);
                m_SelectedTransitionIndex = -1;
            }
        }
    } else {
        ImGui::TextDisabled("Select a state or transition to edit");
    }
}

void AnimatorControllerEditorPanel::CreateNewState(const ImVec2& position) {
    AnimatorState newState{};
    newState.name = m_NewStateName;
    newState.editorPosition = Vector2{position.x, position.y};
    newState.speed = 1.0f;
    newState.animationClipUUID = UUID{0};

    m_Controller->AddState(newState);
    snprintf(m_NewStateName, sizeof(m_NewStateName), "NewState%zu", m_Controller->GetStates().size() + 1);

    if (m_Controller->GetStates().size() == 1) {
        m_Controller->SetDefaultState(newState.name);
    }
}

void AnimatorControllerEditorPanel::DeleteState(size_t stateIndex) {
    const std::vector<AnimatorState>& states = m_Controller->GetStates();
    if (stateIndex >= states.size()) return;

    const std::string stateName = states[stateIndex].name;

    const std::vector<AnimatorTransition>& transitions = m_Controller->GetTransitions();
    for (int i = static_cast<int>(transitions.size()) - 1; i >= 0; --i) {
        if (transitions[i].fromState == stateName || transitions[i].toState == stateName) {
            m_Controller->RemoveTransition(transitions[i].fromState, transitions[i].toState);
        }
    }

    m_Controller->RemoveState(stateName);
}

void AnimatorControllerEditorPanel::CreateTransition(size_t fromIndex, size_t toIndex) {
    const std::vector<AnimatorState>& states = m_Controller->GetStates();
    if (fromIndex >= states.size() || toIndex >= states.size()) return;

    AnimatorTransition trans{};
    trans.fromState = states[fromIndex].name;
    trans.toState = states[toIndex].name;
    trans.exitTime = 0.75f;
    trans.transitionDuration = 0.25f;
    trans.hasExitTime = false;

    m_Controller->AddTransition(trans);
}

void AnimatorControllerEditorPanel::DeleteTransition(size_t transitionIndex) {
    const std::vector<AnimatorTransition>& transitions = m_Controller->GetTransitions();
    if (transitionIndex >= transitions.size()) return;

    const AnimatorTransition& trans = transitions[transitionIndex];
    m_Controller->RemoveTransition(trans.fromState, trans.toState);
}

void AnimatorControllerEditorPanel::Save() {
    if (m_CurrentPath.empty() || !m_Controller) {
        TraceLog(LOG_ERROR, "Cannot save: no path or controller");
        return;
    }

    if (AnimationSerializer::SerializeAnimatorController(*m_Controller, m_CurrentPath)) {
        TraceLog(LOG_INFO, "AnimatorController saved successfully");
    }
}

ImVec2 AnimatorControllerEditorPanel::GetStateNodePosition(size_t stateIndex) const {
    const std::vector<AnimatorState>& states = m_Controller->GetStates();
    if (stateIndex >= states.size()) return ImVec2{0, 0};
    return ImVec2{states[stateIndex].editorPosition.x, states[stateIndex].editorPosition.y};
}

int AnimatorControllerEditorPanel::GetStateIndexAtPosition(const ImVec2& pos, const ImVec2& canvasPos, const ImVec2& scrolling) {
    (void)canvasPos;
    (void)scrolling;
    const std::vector<AnimatorState>& states = m_Controller->GetStates();

    for (int i = static_cast<int>(states.size()) - 1; i >= 0; --i) {
        const AnimatorState& state = states[i];
        ImVec2 nodePos = ImVec2{state.editorPosition.x, state.editorPosition.y};
        ImVec2 nodeEnd = ImVec2{nodePos.x + NODE_WIDTH, nodePos.y + NODE_HEIGHT};

        if (pos.x >= nodePos.x && pos.x <= nodeEnd.x && pos.y >= nodePos.y && pos.y <= nodeEnd.y) {
            return i;
        }
    }

    return -1;
}

int AnimatorControllerEditorPanel::GetTransitionIndexAtPosition(const ImVec2& mousePos, const ImVec2& canvasPos, const ImVec2& scrolling) {
    const std::vector<AnimatorTransition>& transitions = m_Controller->GetTransitions();
    const std::vector<AnimatorState>& states = m_Controller->GetStates();

    for (int i = static_cast<int>(transitions.size()) - 1; i >= 0; --i) {
        const AnimatorTransition& trans = transitions[i];

        int fromIndex = -1;
        int toIndex = -1;

        for (size_t j = 0; j < states.size(); ++j) {
            if (states[j].name == trans.fromState) fromIndex = static_cast<int>(j);
            if (states[j].name == trans.toState) toIndex = static_cast<int>(j);
        }

        if (fromIndex < 0 || toIndex < 0) continue;

        const AnimatorState& fromState = states[fromIndex];
        const AnimatorState& toState = states[toIndex];

        ImVec2 fromPos = ImVec2{
            canvasPos.x + scrolling.x + fromState.editorPosition.x + NODE_WIDTH,
            canvasPos.y + scrolling.y + fromState.editorPosition.y + NODE_HEIGHT / 2.0f
        };
        ImVec2 toPos = ImVec2{
            canvasPos.x + scrolling.x + toState.editorPosition.x,
            canvasPos.y + scrolling.y + toState.editorPosition.y + NODE_HEIGHT / 2.0f
        };

        ImVec2 dir = ImVec2{toPos.x - fromPos.x, toPos.y - fromPos.y};
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len == 0) continue;
        dir.x /= len;
        dir.y /= len;

        ImVec2 perpendicular = ImVec2{-dir.y, dir.x};

        ImVec2 toMouse = ImVec2{mousePos.x - fromPos.x, mousePos.y - fromPos.y};
        float alongLine = toMouse.x * dir.x + toMouse.y * dir.y;
        float perpDist = std::abs(toMouse.x * perpendicular.x + toMouse.y * perpendicular.y);

        if (alongLine >= 0 && alongLine <= len && perpDist < 10.0f) {
            return i;
        }
    }

    return -1;
}

} // namespace PiiXeL

#endif
