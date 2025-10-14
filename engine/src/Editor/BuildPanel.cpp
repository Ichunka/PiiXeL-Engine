#include "Editor/BuildPanel.hpp"

#include <cstring>
#include <ctime>
#include <imgui.h>
#include <iomanip>
#include <sstream>

namespace PiiXeL {

BuildPanel::BuildPanel() : m_BuildSystem(std::make_unique<BuildSystem>()), m_ShowExportDialog(false) {
    const std::string defaultExportPath = m_BuildSystem->GetProjectRoot() + "/export";
    std::memset(m_ExportPathBuffer, 0, sizeof(m_ExportPathBuffer));
#ifdef _MSC_VER
    strcpy_s(m_ExportPathBuffer, sizeof(m_ExportPathBuffer), defaultExportPath.c_str());
#else
    std::strncpy(m_ExportPathBuffer, defaultExportPath.c_str(), sizeof(m_ExportPathBuffer) - 1);
#endif
}

BuildPanel::~BuildPanel() = default;

void BuildPanel::Update() {}

std::string GetCurrentTimeString() {
    const std::time_t now = std::time(nullptr);
    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%H:%M:%S");
    return oss.str();
}

void BuildPanel::AddLogMessage(const std::string& message) {
    const std::string timestamp = "[" + GetCurrentTimeString() + "] ";
    m_BuildLog.push_back(timestamp + message);

    if (m_BuildLog.size() > 100)
    { m_BuildLog.erase(m_BuildLog.begin()); }
}

void BuildPanel::OnBuildProgress(const BuildProgress& progress) {
    if (!progress.statusMessage.empty())
    { AddLogMessage(progress.statusMessage); }
}

void BuildPanel::Render() {
    if (!ImGui::Begin("Build & Export", nullptr, ImGuiWindowFlags_None))
    {
        ImGui::End();
        return;
    }

    RenderBuildButtons();
    ImGui::Separator();
    RenderProgressBar();
    ImGui::Separator();
    RenderBuildLog();

    if (m_ShowExportDialog)
    {
        ImGui::OpenPopup("Export Game");
        m_ShowExportDialog = false;
    }

    if (ImGui::BeginPopupModal("Export Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Export Path:");
        ImGui::InputText("##exportpath", m_ExportPathBuffer, sizeof(m_ExportPathBuffer));

        ImGui::Spacing();

        if (ImGui::Button("Export", ImVec2(120, 0)))
        {
            m_ExportPath = std::string(m_ExportPathBuffer);
            AddLogMessage("Starting export to: " + m_ExportPath);

            m_BuildSystem->ExportGame(m_ExportPath,
                                      [this](const BuildProgress& progress) { OnBuildProgress(progress); });

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        { ImGui::CloseCurrentPopup(); }

        ImGui::EndPopup();
    }

    ImGui::End();
}

void BuildPanel::RenderBuildButtons() {
    ImGui::Text("Build Actions:");
    ImGui::Spacing();

    const bool isBuilding = m_BuildSystem->IsBuildRunning();

    ImGui::BeginDisabled(isBuilding);

    if (ImGui::Button("Build Game Package", ImVec2(200, 0)))
    {
        AddLogMessage("Building game package...");
        m_BuildSystem->BuildGamePackage([this](const BuildProgress& progress) { OnBuildProgress(progress); });
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    { ImGui::SetTooltip("Creates game.package with all assets and scenes"); }

    if (ImGui::Button("Build Game Executable", ImVec2(200, 0)))
    {
        AddLogMessage("Building game executable...");
        m_BuildSystem->BuildGameExecutable([this](const BuildProgress& progress) { OnBuildProgress(progress); });
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    { ImGui::SetTooltip("Compiles the standalone game (no editor)"); }

    if (ImGui::Button("Export Game (Full)", ImVec2(200, 0)))
    { m_ShowExportDialog = true; }

    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    { ImGui::SetTooltip("Builds and exports game with all dependencies to a folder"); }

    ImGui::EndDisabled();

    if (isBuilding)
    {
        ImGui::SameLine();
        if (ImGui::Button("Cancel Build", ImVec2(120, 0)))
        {
            m_BuildSystem->Cancel();
            AddLogMessage("Build cancelled by user");
        }
    }
}

const char* GetBuildStepName(BuildStep step) {
    switch (step)
    {
        case BuildStep::Idle:
            return "Idle";
        case BuildStep::ConfiguringCMake:
            return "Configuring CMake";
        case BuildStep::CompilingGame:
            return "Compiling Game";
        case BuildStep::BuildingPackage:
            return "Building Package";
        case BuildStep::CopyingAssets:
            return "Copying Assets";
        case BuildStep::CopyingDependencies:
            return "Copying Dependencies";
        case BuildStep::Completed:
            return "Completed";
        case BuildStep::Failed:
            return "Failed";
        default:
            return "Unknown";
    }
}

void BuildPanel::RenderProgressBar() {
    const BuildProgress& progress = m_BuildSystem->GetCurrentProgress();

    ImGui::Text("Build Status:");
    ImGui::Spacing();

    ImVec4 progressColor{0.2f, 0.6f, 1.0f, 1.0f};

    if (progress.currentStep == BuildStep::Completed)
    { progressColor = ImVec4{0.2f, 0.8f, 0.2f, 1.0f}; }
    else if (progress.currentStep == BuildStep::Failed)
    { progressColor = ImVec4{0.8f, 0.2f, 0.2f, 1.0f}; }

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, progressColor);

    const float progressFraction = progress.percentage / 100.0f;
    char progressText[64];
    std::snprintf(progressText, sizeof(progressText), "%.1f%%", progress.percentage);

    ImGui::ProgressBar(progressFraction, ImVec2(-1, 30), progressText);

    ImGui::PopStyleColor();

    if (progress.isRunning)
    {
        ImGui::Text("Current Step: %s", GetBuildStepName(progress.currentStep));

        if (!progress.statusMessage.empty())
        { ImGui::TextWrapped("%s", progress.statusMessage.c_str()); }

        ImGui::Spacing();

        const char* spinner[] = {"|", "/", "-", "\\"};
        static int spinnerIndex = 0;
        spinnerIndex = (spinnerIndex + 1) % 4;
        ImGui::Text("Working %s", spinner[spinnerIndex]);
    }
    else if (progress.currentStep == BuildStep::Completed)
    { ImGui::TextColored(ImVec4{0.2f, 1.0f, 0.2f, 1.0f}, "Build completed successfully!"); }
    else if (progress.currentStep == BuildStep::Failed)
    {
        ImGui::TextColored(ImVec4{1.0f, 0.2f, 0.2f, 1.0f}, "Build failed!");
        if (!progress.statusMessage.empty())
        { ImGui::TextWrapped("%s", progress.statusMessage.c_str()); }
    }
    else if (progress.currentStep == BuildStep::Idle)
    { ImGui::TextDisabled("No build running. Use the buttons above to start a build."); }
}

void BuildPanel::RenderBuildLog() {
    ImGui::Text("Build Log:");
    ImGui::Spacing();

    if (ImGui::BeginChild("BuildLogScroll", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        for (const std::string& logMessage : m_BuildLog)
        { ImGui::TextUnformatted(logMessage.c_str()); }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        { ImGui::SetScrollHereY(1.0f); }
    }
    ImGui::EndChild();

    if (ImGui::Button("Clear Log"))
    { m_BuildLog.clear(); }

    ImGui::SameLine();

    if (ImGui::Button("Copy Logs"))
    {
        std::string fullLog;
        for (const std::string& logMessage : m_BuildLog)
        { fullLog += logMessage + "\n"; }
        ImGui::SetClipboardText(fullLog.c_str());
    }
}

} // namespace PiiXeL
