#ifndef PIIXELENGINE_PROFILERPANEL_HPP
#define PIIXELENGINE_PROFILERPANEL_HPP

#ifdef BUILD_WITH_EDITOR

#include "EditorPanel.hpp"
#include "Debug/Profiler.hpp"

namespace PiiXeL {

class ProfilerPanel : public EditorPanel {
public:
    ProfilerPanel(
        bool* paused,
        FrameSnapshot* pausedSnapshot,
        int* selectedFrame,
        FrameSnapshot* selectedFrameSnapshot,
        float* flameGraphZoom,
        float* flameGraphScroll
    );

    void OnImGuiRender() override;
    const char* GetTitle() const override { return "Profiler"; }
    bool IsOpen() const override { return m_IsOpen; }
    void SetOpen(bool open) override { m_IsOpen = open; }

private:
    bool* m_Paused;
    FrameSnapshot* m_PausedSnapshot;
    int* m_SelectedFrame;
    FrameSnapshot* m_SelectedFrameSnapshot;
    float* m_FlameGraphZoom;
    float* m_FlameGraphScroll;

    bool m_IsOpen{true};
};

}

#endif

#endif
