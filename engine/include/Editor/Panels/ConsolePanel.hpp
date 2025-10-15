#ifndef PIIXELENGINE_CONSOLEPANEL_HPP
#define PIIXELENGINE_CONSOLEPANEL_HPP

#ifdef BUILD_WITH_EDITOR

#include <vector>

#include "EditorPanel.hpp"

namespace PiiXeL {

struct ConsoleFilters {
    bool showEngine{true};
    bool showGame{true};
    bool showTrace{true};
    bool showDebug{true};
    bool showInfo{true};
    bool showWarning{true};
    bool showError{true};
    bool showCategoryEngine{true};
    bool showCategoryAsset{true};
    bool showCategoryEditor{true};
    bool showCategoryPhysics{true};
    bool showCategoryRender{true};
    bool showCategoryScene{true};
    bool showCategoryScript{true};
    bool showCategoryAnimation{true};
    bool showCategoryBuild{true};
    bool showCategoryGame{true};
    bool showCategoryUnknown{true};
};

class ConsolePanel : public EditorPanel {
public:
    ConsolePanel(ConsoleFilters* filters, bool* autoScroll, int* selectedTab, std::vector<int>* selectedLines,
                 int* lastClickedLine);

    void OnImGuiRender() override;
    const char* GetTitle() const override { return "Console"; }
    bool IsOpen() const override { return m_IsOpen; }
    void SetOpen(bool open) override { m_IsOpen = open; }

private:
    ConsoleFilters* m_Filters;
    bool* m_AutoScroll;
    int* m_SelectedTab;
    std::vector<int>* m_SelectedLines;
    int* m_LastClickedLine;

    bool m_IsOpen{true};
};

} // namespace PiiXeL

#endif

#endif
