#ifdef BUILD_WITH_EDITOR

#include "Editor/AnimatorControllerEditorPanel.hpp"
#include "Core/Logger.hpp"
#include "Animation/AnimationSerializer.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/Asset.hpp"
#include "Resources/AssetManager.hpp"
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
        static float leftPanelWidth = 250.0f;
        const float minPanelWidth = 150.0f;
        const float splitterWidth = 4.0f;

        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        leftPanelWidth = std::max(minPanelWidth, std::min(leftPanelWidth, contentSize.x - minPanelWidth - splitterWidth));

        ImGui::BeginChild("ParametersPanel", ImVec2{leftPanelWidth, 0}, true);
        RenderParametersPanel();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::Button("##splitter1", ImVec2{splitterWidth, -1});
        if (ImGui::IsItemActive()) {
            leftPanelWidth += ImGui::GetIO().MouseDelta.x;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        ImGui::SameLine();

        ImGui::BeginChild("StateGraphPanel", ImVec2{0, 0}, true);
        RenderStateGraph();
        ImGui::EndChild();
    }
    ImGui::EndChild();

    ImGui::End();
}

void AnimatorControllerEditorPanel::Open(const std::string& controllerPath) {
    if (m_IsOpen && m_CurrentPath == controllerPath) {
        return;
    }

    m_CurrentPath = controllerPath;

    PX_LOG_INFO(EDITOR, "Opening AnimatorController: %s", controllerPath.c_str());

    UUID existingUUID = AssetRegistry::Instance().GetUUIDFromPath(controllerPath);
    if (existingUUID.Get() != 0) {
        PX_LOG_INFO(EDITOR, "Loading from AssetRegistry with UUID: %llu", existingUUID.Get());
        std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAsset(existingUUID);
        m_Controller = std::dynamic_pointer_cast<AnimatorController>(asset);
    } else {
        PX_LOG_INFO(EDITOR, "Loading from path (no UUID in registry)");
        std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAssetFromPath(controllerPath);
        m_Controller = std::dynamic_pointer_cast<AnimatorController>(asset);
    }

    if (!m_Controller) {
        PX_LOG_ERROR(EDITOR, "Failed to load AnimatorController: %s", controllerPath.c_str());
    } else {
        PX_LOG_INFO(EDITOR, "AnimatorController loaded successfully");
        const std::vector<AnimatorState>& states = m_Controller->GetStates();
        for (size_t i = 0; i < states.size(); ++i) {
            PX_LOG_INFO(EDITOR, "Loaded State %zu: %s - AnimClip UUID: %llu", i, states[i].name.c_str(), states[i].animationClipUUID.Get());
        }
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

    ImVec2 scrolling = ImVec2{m_GraphScrollingX, m_GraphScrollingY};

    int rightClickedStateIndex = -1;
    if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        rightClickedStateIndex = GetStateIndexAtPosition(mousePosInCanvas, canvasPos, scrolling);
        if (rightClickedStateIndex >= 0) {
            m_SelectedStateIndex = rightClickedStateIndex;
            m_SelectedTransitionIndex = -1;
            if (m_OnSelectionChanged) {
                m_OnSelectionChanged();
            }
            ImGui::OpenPopup("NodeContextMenu");
        } else {
            m_OpenContextMenu = true;
            m_ContextMenuPosX = mousePosInCanvas.x;
            m_ContextMenuPosY = mousePosInCanvas.y;
        }
    }

    drawList->PushClipRect(canvasPos, canvasEnd, true);
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
            if (m_OnSelectionChanged) {
                m_OnSelectionChanged();
            }
        } else {
            int clickedTransition = GetTransitionIndexAtPosition(mousePos, canvasPos, scrolling);
            if (clickedTransition >= 0) {
                m_SelectedTransitionIndex = clickedTransition;
                m_SelectedStateIndex = -1;
                if (m_OnSelectionChanged) {
                    m_OnSelectionChanged();
                }
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

    if (ImGui::BeginPopup("NodeContextMenu")) {
        if (m_SelectedStateIndex >= 0 && m_SelectedStateIndex < static_cast<int>(states.size())) {
            const AnimatorState& state = states[m_SelectedStateIndex];
            const bool isDefault = (state.name == m_Controller->GetDefaultState());

            ImGui::Text("State: %s", state.name.c_str());
            ImGui::Separator();

            if (ImGui::MenuItem("Create Transition")) {
                m_CreatingTransitionFromIndex = m_SelectedStateIndex;
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Set as Default", nullptr, false, !isDefault)) {
                m_Controller->SetDefaultState(state.name);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Delete State")) {
                DeleteState(m_SelectedStateIndex);
                m_SelectedStateIndex = -1;
                ImGui::CloseCurrentPopup();
            }
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

    ImU32 headerColor = isDefault ? IM_COL32(70, 140, 220, 255) : IM_COL32(50, 110, 180, 255);
    ImU32 bodyColor = isDefault ? IM_COL32(45, 80, 120, 255) : IM_COL32(40, 65, 100, 255);
    ImU32 borderColor = isSelected ? IM_COL32(100, 180, 255, 255) : IM_COL32(70, 130, 200, 255);

    ImVec2 headerEnd = ImVec2{nodeEnd.x, nodePos.y + 28};
    drawList->AddRectFilled(nodePos, headerEnd, headerColor, NODE_ROUNDING, ImDrawFlags_RoundCornersTop);
    drawList->AddRectFilled(ImVec2{nodePos.x, nodePos.y + 28}, nodeEnd, bodyColor, NODE_ROUNDING, ImDrawFlags_RoundCornersBottom);
    drawList->AddRect(nodePos, nodeEnd, borderColor, NODE_ROUNDING, 0, isSelected ? 3.0f : 2.0f);

    ImVec2 textPos = ImVec2{nodePos.x + 10, nodePos.y + 10};
    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), state.name.c_str());

    if (isDefault) {
        ImVec2 defaultTagPos = ImVec2{nodePos.x + NODE_WIDTH - 65, nodePos.y + 8};
        drawList->AddRectFilled(defaultTagPos, ImVec2{defaultTagPos.x + 55, defaultTagPos.y + 14}, IM_COL32(100, 200, 100, 200), 2.0f);
        ImVec2 defaultTextPos = ImVec2{defaultTagPos.x + 5, defaultTagPos.y + 1};
        drawList->AddText(defaultTextPos, IM_COL32(255, 255, 255, 255), "DEFAULT");
    }

    if (state.animationClipUUID.Get() != 0) {
        std::string clipPath = AssetRegistry::Instance().GetPathFromUUID(state.animationClipUUID);
        if (!clipPath.empty()) {
            std::filesystem::path fsPath{clipPath};
            std::string displayName = fsPath.stem().string();
            ImVec2 clipTextPos = ImVec2{nodePos.x + 10, nodePos.y + 30};
            drawList->AddText(clipTextPos, IM_COL32(180, 180, 180, 255), displayName.c_str());
        }
    }

    ImVec2 speedTextPos = ImVec2{nodePos.x + 10, nodePos.y + 50};
    char speedText[32];
    snprintf(speedText, sizeof(speedText), "Speed: %.2fx", state.speed);
    drawList->AddText(speedTextPos, IM_COL32(150, 150, 150, 255), speedText);
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
        ImU32 lineColor = isSelected ? IM_COL32(255, 200, 100, 255) : IM_COL32(200, 200, 200, 255);
        ImU32 arrowOutlineColor = isSelected ? IM_COL32(255, 150, 0, 255) : IM_COL32(80, 80, 80, 255);
        float lineThickness = isSelected ? 3.5f : 2.5f;

        drawList->AddLine(fromPos, toPos, lineColor, lineThickness);

        ImVec2 direction = ImVec2{toPos.x - fromPos.x, toPos.y - fromPos.y};
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction.x /= length;
            direction.y /= length;

            ImVec2 arrowTip = ImVec2{toPos.x - direction.x * 20.0f, toPos.y - direction.y * 20.0f};
            ImVec2 perpendicular = ImVec2{-direction.y, direction.x};
            ImVec2 arrowLeft = ImVec2{arrowTip.x - direction.x * 12.0f + perpendicular.x * 10.0f, arrowTip.y - direction.y * 12.0f + perpendicular.y * 10.0f};
            ImVec2 arrowRight = ImVec2{arrowTip.x - direction.x * 12.0f - perpendicular.x * 10.0f, arrowTip.y - direction.y * 12.0f - perpendicular.y * 10.0f};

            drawList->AddTriangle(arrowTip, arrowLeft, arrowRight, arrowOutlineColor, 2.0f);
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

            const bool isDefault = (state.name == m_Controller->GetDefaultState());
            if (!isDefault) {
                if (ImGui::Button("Set as Default State", ImVec2{-1, 0})) {
                    m_Controller->SetDefaultState(state.name);
                }
            } else {
                ImGui::BeginDisabled();
                ImGui::Button("Already Default State", ImVec2{-1, 0});
                ImGui::EndDisabled();
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "Animation Clip:");

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.15f, 0.15f, 0.15f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4{0.4f, 0.4f, 0.4f, 1.0f});
            ImGui::BeginChild("AnimClipDropZone", ImVec2{-1, 50}, true);

            ImVec2 childSize = ImGui::GetContentRegionAvail();
            ImGui::InvisibleButton("DropZoneButton", childSize);

            ImVec2 textPos = ImGui::GetItemRectMin();
            textPos.y += 15;

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            if (state.animationClipUUID.Get() != 0) {
                std::string clipPath = AssetRegistry::Instance().GetPathFromUUID(state.animationClipUUID);
                if (!clipPath.empty()) {
                    std::filesystem::path fsPath{clipPath};
                    std::string displayName = fsPath.stem().string();
                    textPos.x += 10;
                    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), displayName.c_str());
                } else {
                    textPos.x += 10;
                    drawList->AddText(textPos, IM_COL32(255, 128, 128, 255), "[Missing Clip]");
                }
            } else {
                textPos.x += 10;
                drawList->AddText(textPos, IM_COL32(128, 128, 128, 255), "Drop AnimationClip here");
            }

            if (ImGui::BeginDragDropTarget()) {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_UUID");
                if (payload) {
                    UUID droppedUUID = *static_cast<const UUID*>(payload->Data);
                    std::string assetPath = AssetRegistry::Instance().GetPathFromUUID(droppedUUID);
                    if (!assetPath.empty() && assetPath.find(".animclip") != std::string::npos) {
                        state.animationClipUUID = droppedUUID;
                        PX_LOG_INFO(EDITOR, "Animation clip UUID assigned: %llu", state.animationClipUUID.Get());
                        Save();
                    }
                }

                if (!payload) {
                    payload = ImGui::AcceptDragDropPayload("ASSET_ANIM");
                    if (payload) {
                        AssetInfo* assetInfo = *static_cast<AssetInfo**>(payload->Data);
                        if (assetInfo && assetInfo->type == "animclip") {
                            PX_LOG_INFO(EDITOR, "Received drop - UUID: %llu, filename: %s", assetInfo->uuid.Get(), assetInfo->filename.c_str());

                            if (assetInfo->uuid.Get() != 0) {
                                state.animationClipUUID = assetInfo->uuid;
                                PX_LOG_INFO(EDITOR, "Animation clip UUID assigned: %llu", state.animationClipUUID.Get());
                                Save();
                            } else {
                                PX_LOG_WARNING(EDITOR, "Asset UUID is 0, trying to load from path: %s", assetInfo->path.c_str());
                                std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAssetFromPath(assetInfo->path);
                                if (asset) {
                                    state.animationClipUUID = asset->GetUUID();
                                    PX_LOG_INFO(EDITOR, "Loaded and assigned UUID: %llu", state.animationClipUUID.Get());
                                    Save();
                                }
                            }
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::EndChild();
            ImGui::PopStyleColor(2);

            if (state.animationClipUUID.Get() != 0) {
                if (ImGui::Button("Clear##ClipClear", ImVec2{-1, 0})) {
                    state.animationClipUUID = UUID{0};
                    Save();
                }
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
            const std::vector<AnimatorParameter>& params = m_Controller->GetParameters();
            int conditionToDelete = -1;

            for (size_t i = 0; i < conditions.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));
                TransitionCondition& cond = conditions[i];

                int currentParamIndex = 0;
                std::vector<const char*> paramNames;
                for (size_t j = 0; j < params.size(); ++j) {
                    paramNames.push_back(params[j].name.c_str());
                    if (params[j].name == cond.parameterName) {
                        currentParamIndex = static_cast<int>(j);
                    }
                }

                if (!params.empty()) {
                    if (ImGui::Combo("Parameter", &currentParamIndex, paramNames.data(), static_cast<int>(paramNames.size()))) {
                        cond.parameterName = params[currentParamIndex].name;
                    }

                    const AnimatorParameter& selectedParam = params[currentParamIndex];

                    if (selectedParam.type == AnimatorParameterType::Trigger) {
                        ImGui::TextDisabled("Trigger (fires when set)");
                    } else if (selectedParam.type == AnimatorParameterType::Bool) {
                        const char* boolCondTypes[] = {"True", "False"};
                        int boolCondType = std::get<bool>(cond.value) ? 0 : 1;
                        if (ImGui::Combo("Condition", &boolCondType, boolCondTypes, 2)) {
                            cond.value = (boolCondType == 0);
                            cond.type = (boolCondType == 0) ? TransitionConditionType::Equals : TransitionConditionType::NotEquals;
                        }
                    } else if (selectedParam.type == AnimatorParameterType::Float) {
                        const char* floatCondTypes[] = {"Greater", "Less", "Equals"};
                        int floatCondType = (cond.type == TransitionConditionType::Greater) ? 0 :
                                           (cond.type == TransitionConditionType::Less) ? 1 : 2;
                        if (ImGui::Combo("Condition", &floatCondType, floatCondTypes, 3)) {
                            cond.type = (floatCondType == 0) ? TransitionConditionType::Greater :
                                       (floatCondType == 1) ? TransitionConditionType::Less : TransitionConditionType::Equals;
                        }
                        float floatValue = std::holds_alternative<float>(cond.value) ? std::get<float>(cond.value) : 0.0f;
                        if (ImGui::DragFloat("Value", &floatValue, 0.1f)) {
                            cond.value = floatValue;
                        }
                    } else if (selectedParam.type == AnimatorParameterType::Int) {
                        const char* intCondTypes[] = {"Greater", "Less", "Equals", "NotEquals"};
                        int intCondType = static_cast<int>(cond.type);
                        if (ImGui::Combo("Condition", &intCondType, intCondTypes, 4)) {
                            cond.type = static_cast<TransitionConditionType>(intCondType);
                        }
                        int intValue = std::holds_alternative<int>(cond.value) ? std::get<int>(cond.value) : 0;
                        if (ImGui::DragInt("Value", &intValue)) {
                            cond.value = intValue;
                        }
                    }
                } else {
                    ImGui::TextColored(ImVec4{1.0f, 0.5f, 0.5f, 1.0f}, "No parameters available");
                }

                if (ImGui::SmallButton("Delete")) {
                    conditionToDelete = static_cast<int>(i);
                }

                ImGui::Separator();
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
        PX_LOG_ERROR(EDITOR, "Cannot save: no path or controller");
        return;
    }

    PX_LOG_INFO(EDITOR, "Saving AnimatorController to: %s", m_CurrentPath.c_str());

    const std::vector<AnimatorState>& states = m_Controller->GetStates();
    for (size_t i = 0; i < states.size(); ++i) {
        PX_LOG_INFO(EDITOR, "State %zu: %s - AnimClip UUID: %llu", i, states[i].name.c_str(), states[i].animationClipUUID.Get());
    }

    if (AnimationSerializer::SerializeAnimatorController(*m_Controller, m_CurrentPath)) {
        PX_LOG_INFO(EDITOR, "AnimatorController saved successfully to: %s", m_CurrentPath.c_str());
        AssetRegistry::Instance().ReimportAsset(m_CurrentPath);
    } else {
        PX_LOG_ERROR(EDITOR, "Failed to save AnimatorController to: %s", m_CurrentPath.c_str());
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
