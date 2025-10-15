#pragma once

#ifdef BUILD_WITH_EDITOR

#include "EditorPanel.hpp"

namespace PiiXeL {

class Engine;

class ProjectSettingsPanel : public EditorPanel {
public:
    explicit ProjectSettingsPanel(Engine* engine);

    void OnImGuiRender() override;
    const char* GetTitle() const override { return "Project Settings"; }
    bool IsOpen() const override { return m_IsOpen; }
    void SetOpen(bool open) override { m_IsOpen = open; }

private:
    Engine* m_Engine;
    bool m_IsOpen{false};
};

} // namespace PiiXeL

#endif
