#pragma once

#ifdef BUILD_WITH_EDITOR

#include "Editor/Panels/EditorPanel.hpp"

#include <functional>

namespace PiiXeL {

class Engine;
class EditorCommandSystem;
class EditorSceneManager;
class ProjectSettingsPanel;

class MenuBarPanel : public EditorPanel {
public:
    MenuBarPanel(Engine* engine, EditorCommandSystem* commandSystem, EditorSceneManager* sceneManager,
                 ProjectSettingsPanel* projectSettingsPanel);

    void OnImGuiRender() override;
    const char* GetTitle() const override { return "MenuBar"; }
    bool IsOpen() const override { return true; }
    void SetOpen(bool) override {}

private:
    Engine* m_Engine;
    EditorCommandSystem* m_CommandSystem;
    EditorSceneManager* m_SceneManager;
    ProjectSettingsPanel* m_ProjectSettingsPanel;
};

} // namespace PiiXeL

#endif
