#pragma once

#ifdef BUILD_WITH_EDITOR

#include <imgui.h>

namespace PiiXeL {

class EditorPanelManager {
public:
    EditorPanelManager();

    void SetupDockingLayout();
    void BeginDockspace();
    void EndDockspace();

    [[nodiscard]] bool IsDockingLayoutInitialized() const { return m_DockingLayoutInitialized; }
    void SetDockingLayoutInitialized(bool initialized) { m_DockingLayoutInitialized = initialized; }

private:
    bool m_DockingLayoutInitialized{false};
};

} // namespace PiiXeL

#endif
