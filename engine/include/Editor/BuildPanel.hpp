#ifndef PIIXELENGINE_BUILDPANEL_HPP
#define PIIXELENGINE_BUILDPANEL_HPP

#include "Editor/BuildSystem.hpp"
#include <string>
#include <vector>
#include <memory>

namespace PiiXeL {

class BuildPanel {
public:
    BuildPanel();
    ~BuildPanel();

    void Render();
    void Update();

private:
    void RenderBuildButtons();
    void RenderProgressBar();
    void RenderBuildLog();
    void OnBuildProgress(const BuildProgress& progress);
    void AddLogMessage(const std::string& message);

    std::unique_ptr<BuildSystem> m_BuildSystem;
    std::vector<std::string> m_BuildLog;
    std::string m_ExportPath;
    bool m_ShowExportDialog;
    char m_ExportPathBuffer[512];
};

}

#endif
