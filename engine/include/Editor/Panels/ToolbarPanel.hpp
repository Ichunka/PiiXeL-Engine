#pragma once

#ifdef BUILD_WITH_EDITOR

#include "Editor/Panels/EditorPanel.hpp"

#include <functional>

namespace PiiXeL {

class Engine;
class EditorGizmoSystem;
class EditorStateManager;
enum class GizmoMode;

class ToolbarPanel : public EditorPanel {
public:
    ToolbarPanel(Engine* engine, EditorGizmoSystem* gizmoSystem, EditorStateManager* stateManager);

    void OnImGuiRender() override;
    const char* GetTitle() const override { return "Toolbar"; }
    bool IsOpen() const override { return true; }
    void SetOpen(bool) override {}

    void SetOnPlayCallback(std::function<void()> callback) { m_OnPlayCallback = callback; }
    void SetOnStopCallback(std::function<void()> callback) { m_OnStopCallback = callback; }

private:
    Engine* m_Engine;
    EditorGizmoSystem* m_GizmoSystem;
    EditorStateManager* m_StateManager;
    std::function<void()> m_OnPlayCallback;
    std::function<void()> m_OnStopCallback;
};

} // namespace PiiXeL

#endif
