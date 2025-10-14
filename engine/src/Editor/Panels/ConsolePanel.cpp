#ifdef BUILD_WITH_EDITOR

#include "Editor/Panels/ConsolePanel.hpp"
#include "Core/Logger.hpp"
#include "Editor/ConsoleLogger.hpp"
#include "Debug/Profiler.hpp"
#include <imgui.h>
#include <algorithm>

namespace PiiXeL {

ConsolePanel::ConsolePanel(
    ConsoleFilters* filters,
    bool* autoScroll,
    int* selectedTab,
    std::vector<int>* selectedLines,
    int* lastClickedLine
)
    : m_Filters{filters}
    , m_AutoScroll{autoScroll}
    , m_SelectedTab{selectedTab}
    , m_SelectedLines{selectedLines}
    , m_LastClickedLine{lastClickedLine}
{}

void ConsolePanel::OnImGuiRender() {
    PROFILE_FUNCTION();
    if (!ImGui::Begin("Console")) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear")) {
        ConsoleLogger::Instance().Clear();
        m_SelectedLines->clear();
        *m_LastClickedLine = -1;
    }

    ImGui::SameLine();
    if (ImGui::Button("Copy Selected")) {
        if (!m_SelectedLines->empty()) {
            std::string clipboardText;
            const auto& logs = ConsoleLogger::Instance().GetLogs();

            for (int selectedIdx : *m_SelectedLines) {
                if (selectedIdx >= 0 && selectedIdx < static_cast<int>(logs.size())) {
                    const auto& log = logs[selectedIdx];

                    const char* sourceStr = (log.source == LogSource::Engine) ? "[ENGINE]" : "[GAME]  ";
                    const char* categoryStr = Logger::GetCategoryName(log.category);
                    const char* levelStr;
                    switch (log.level) {
                        case LogLevel::Trace:   levelStr = "[TRACE]"; break;
                        case LogLevel::Debug:   levelStr = "[DEBUG]"; break;
                        case LogLevel::Info:    levelStr = "[INFO] "; break;
                        case LogLevel::Warning: levelStr = "[WARN] "; break;
                        case LogLevel::Error:   levelStr = "[ERROR]"; break;
                        default:                levelStr = "[?????]"; break;
                    }

                    clipboardText += sourceStr;
                    clipboardText += " ";
                    clipboardText += "[";
                    clipboardText += categoryStr;
                    clipboardText += "] ";
                    clipboardText += levelStr;
                    clipboardText += " ";
                    clipboardText += log.message;
                    clipboardText += "\n";
                }
            }

            ImGui::SetClipboardText(clipboardText.c_str());
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Copy All Filtered")) {
        std::string clipboardText;
        const auto& logs = ConsoleLogger::Instance().GetLogs();

        for (size_t i = 0; i < logs.size(); ++i) {
            const auto& log = logs[i];

            bool showByLevel = false;
            switch (log.level) {
                case LogLevel::Trace:   showByLevel = m_Filters->showTrace; break;
                case LogLevel::Debug:   showByLevel = m_Filters->showDebug; break;
                case LogLevel::Info:    showByLevel = m_Filters->showInfo; break;
                case LogLevel::Warning: showByLevel = m_Filters->showWarning; break;
                case LogLevel::Error:   showByLevel = m_Filters->showError; break;
            }

            if (!showByLevel) continue;

            bool showBySource = false;
            if (*m_SelectedTab == 0) {
                showBySource = true;
            } else if (*m_SelectedTab == 1 && log.source == LogSource::Engine) {
                showBySource = true;
            } else if (*m_SelectedTab == 2 && log.source == LogSource::Game) {
                showBySource = true;
            }

            if (!showBySource) continue;

            bool showByCategory = false;
            switch (log.category) {
                case LogCategory::ENGINE:    showByCategory = m_Filters->showCategoryEngine; break;
                case LogCategory::ASSET:     showByCategory = m_Filters->showCategoryAsset; break;
                case LogCategory::EDITOR:    showByCategory = m_Filters->showCategoryEditor; break;
                case LogCategory::PHYSICS:   showByCategory = m_Filters->showCategoryPhysics; break;
                case LogCategory::RENDER:    showByCategory = m_Filters->showCategoryRender; break;
                case LogCategory::SCENE:     showByCategory = m_Filters->showCategoryScene; break;
                case LogCategory::SCRIPT:    showByCategory = m_Filters->showCategoryScript; break;
                case LogCategory::ANIMATION: showByCategory = m_Filters->showCategoryAnimation; break;
                case LogCategory::BUILD:     showByCategory = m_Filters->showCategoryBuild; break;
                case LogCategory::GAME:      showByCategory = m_Filters->showCategoryGame; break;
                case LogCategory::UNKNOWN:   showByCategory = m_Filters->showCategoryUnknown; break;
            }

            if (!showByCategory) continue;

            const char* sourceStr = (log.source == LogSource::Engine) ? "[ENGINE]" : "[GAME]  ";
            const char* categoryStr = Logger::GetCategoryName(log.category);
            const char* levelStr;
            switch (log.level) {
                case LogLevel::Trace:   levelStr = "[TRACE]"; break;
                case LogLevel::Debug:   levelStr = "[DEBUG]"; break;
                case LogLevel::Info:    levelStr = "[INFO] "; break;
                case LogLevel::Warning: levelStr = "[WARN] "; break;
                case LogLevel::Error:   levelStr = "[ERROR]"; break;
                default:                levelStr = "[?????]"; break;
            }

            clipboardText += sourceStr;
            clipboardText += " ";
            clipboardText += "[";
            clipboardText += categoryStr;
            clipboardText += "] ";
            clipboardText += levelStr;
            clipboardText += " ";
            clipboardText += log.message;
            clipboardText += "\n";
        }

        if (!clipboardText.empty()) {
            ImGui::SetClipboardText(clipboardText.c_str());
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &*m_AutoScroll);

    ImGui::Separator();

    ImGui::Text("Level Filters:");
    ImGui::SameLine();
    ImGui::Checkbox("Trace", &m_Filters->showTrace);
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &m_Filters->showDebug);
    ImGui::SameLine();
    ImGui::Checkbox("Info", &m_Filters->showInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Warning", &m_Filters->showWarning);
    ImGui::SameLine();
    ImGui::Checkbox("Error", &m_Filters->showError);

    ImGui::Text("Category Filters:");
    ImGui::SameLine();
    if (ImGui::SmallButton("All##Categories")) {
        m_Filters->showCategoryEngine = true;
        m_Filters->showCategoryAsset = true;
        m_Filters->showCategoryEditor = true;
        m_Filters->showCategoryPhysics = true;
        m_Filters->showCategoryRender = true;
        m_Filters->showCategoryScene = true;
        m_Filters->showCategoryScript = true;
        m_Filters->showCategoryAnimation = true;
        m_Filters->showCategoryBuild = true;
        m_Filters->showCategoryGame = true;
        m_Filters->showCategoryUnknown = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("None##Categories")) {
        m_Filters->showCategoryEngine = false;
        m_Filters->showCategoryAsset = false;
        m_Filters->showCategoryEditor = false;
        m_Filters->showCategoryPhysics = false;
        m_Filters->showCategoryRender = false;
        m_Filters->showCategoryScene = false;
        m_Filters->showCategoryScript = false;
        m_Filters->showCategoryAnimation = false;
        m_Filters->showCategoryBuild = false;
        m_Filters->showCategoryGame = false;
        m_Filters->showCategoryUnknown = false;
    }
    ImGui::SameLine();
    ImGui::Checkbox("ENGINE", &m_Filters->showCategoryEngine);
    ImGui::SameLine();
    ImGui::Checkbox("ASSET", &m_Filters->showCategoryAsset);
    ImGui::SameLine();
    ImGui::Checkbox("EDITOR", &m_Filters->showCategoryEditor);
    ImGui::SameLine();
    ImGui::Checkbox("PHYSICS", &m_Filters->showCategoryPhysics);
    ImGui::SameLine();
    ImGui::Checkbox("RENDER", &m_Filters->showCategoryRender);
    ImGui::SameLine();
    ImGui::Checkbox("SCENE", &m_Filters->showCategoryScene);
    ImGui::SameLine();
    ImGui::Checkbox("SCRIPT", &m_Filters->showCategoryScript);
    ImGui::SameLine();
    ImGui::Checkbox("ANIMATION", &m_Filters->showCategoryAnimation);
    ImGui::SameLine();
    ImGui::Checkbox("BUILD", &m_Filters->showCategoryBuild);
    ImGui::SameLine();
    ImGui::Checkbox("GAME", &m_Filters->showCategoryGame);
    ImGui::SameLine();
    ImGui::Checkbox("UNKNOWN", &m_Filters->showCategoryUnknown);

    ImGui::Separator();

    if (ImGui::BeginTabBar("ConsoleSourceTabs")) {
        if (ImGui::BeginTabItem("All")) {
            *m_SelectedTab = 0;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Engine")) {
            *m_SelectedTab = 1;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Game")) {
            *m_SelectedTab = 2;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2{0, 0}, false, ImGuiWindowFlags_HorizontalScrollbar);

    const auto& logs = ConsoleLogger::Instance().GetLogs();

    ImGuiIO& io = ImGui::GetIO();
    int visibleLineIndex = 0;

    for (size_t i = 0; i < logs.size(); ++i) {
        const auto& log = logs[i];

        bool showByLevel = false;
        switch (log.level) {
            case LogLevel::Trace:   showByLevel = m_Filters->showTrace; break;
            case LogLevel::Debug:   showByLevel = m_Filters->showDebug; break;
            case LogLevel::Info:    showByLevel = m_Filters->showInfo; break;
            case LogLevel::Warning: showByLevel = m_Filters->showWarning; break;
            case LogLevel::Error:   showByLevel = m_Filters->showError; break;
        }

        if (!showByLevel) {
            continue;
        }

        bool showBySource = false;
        if (*m_SelectedTab == 0) {
            showBySource = true;
        } else if (*m_SelectedTab == 1 && log.source == LogSource::Engine) {
            showBySource = true;
        } else if (*m_SelectedTab == 2 && log.source == LogSource::Game) {
            showBySource = true;
        }

        if (!showBySource) {
            continue;
        }

        bool showByCategory = false;
        switch (log.category) {
            case LogCategory::ENGINE:    showByCategory = m_Filters->showCategoryEngine; break;
            case LogCategory::ASSET:     showByCategory = m_Filters->showCategoryAsset; break;
            case LogCategory::EDITOR:    showByCategory = m_Filters->showCategoryEditor; break;
            case LogCategory::PHYSICS:   showByCategory = m_Filters->showCategoryPhysics; break;
            case LogCategory::RENDER:    showByCategory = m_Filters->showCategoryRender; break;
            case LogCategory::SCENE:     showByCategory = m_Filters->showCategoryScene; break;
            case LogCategory::SCRIPT:    showByCategory = m_Filters->showCategoryScript; break;
            case LogCategory::ANIMATION: showByCategory = m_Filters->showCategoryAnimation; break;
            case LogCategory::BUILD:     showByCategory = m_Filters->showCategoryBuild; break;
            case LogCategory::GAME:      showByCategory = m_Filters->showCategoryGame; break;
            case LogCategory::UNKNOWN:   showByCategory = m_Filters->showCategoryUnknown; break;
        }

        if (!showByCategory) {
            continue;
        }

        bool isSelected = std::find(m_SelectedLines->begin(), m_SelectedLines->end(), static_cast<int>(i)) != m_SelectedLines->end();

        ImVec4 bgColor;
        if (isSelected) {
            bgColor = ImVec4{0.3f, 0.5f, 0.8f, 0.5f}; // Blue highlight for selection
        } else if (visibleLineIndex % 2 == 0) {
            bgColor = ImVec4{0.15f, 0.15f, 0.15f, 0.3f}; // lightly darker
        } else {
            bgColor = ImVec4{0.12f, 0.12f, 0.12f, 0.3f}; // Default dark
        }

        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImVec2 lineSize = ImVec2{ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight()};
        ImGui::GetWindowDrawList()->AddRectFilled(cursorPos, ImVec2{cursorPos.x + lineSize.x, cursorPos.y + lineSize.y}, ImGui::ColorConvertFloat4ToU32(bgColor));

        ImGui::PushID(static_cast<int>(i));
        ImGui::Selectable("##line", isSelected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns, ImVec2{0, 0});

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            if (io.KeyCtrl) {
                auto it = std::find(m_SelectedLines->begin(), m_SelectedLines->end(), static_cast<int>(i));
                if (it != m_SelectedLines->end()) {
                    m_SelectedLines->erase(it);
                } else {
                    m_SelectedLines->push_back(static_cast<int>(i));
                }
            } else if (io.KeyShift && *m_LastClickedLine != -1) {
                m_SelectedLines->clear();
                int start = std::min(*m_LastClickedLine, static_cast<int>(i));
                int end = std::max(*m_LastClickedLine, static_cast<int>(i));
                for (int idx = start; idx <= end; ++idx) {
                    m_SelectedLines->push_back(idx);
                }
            } else {
                m_SelectedLines->clear();
                m_SelectedLines->push_back(static_cast<int>(i));
            }
            *m_LastClickedLine = static_cast<int>(i);
        }

        ImGui::SameLine(0, 0);

        ImVec4 textColor;
        const char* levelStr;
        switch (log.level) {
            case LogLevel::Trace:
                textColor = ImVec4{0.6f, 0.6f, 0.6f, 1.0f};
                levelStr = "[TRACE]";
                break;
            case LogLevel::Debug:
                textColor = ImVec4{0.7f, 0.7f, 0.9f, 1.0f};
                levelStr = "[DEBUG]";
                break;
            case LogLevel::Info:
                textColor = ImVec4{0.9f, 0.9f, 0.9f, 1.0f};
                levelStr = "[INFO] ";
                break;
            case LogLevel::Warning:
                textColor = ImVec4{1.0f, 0.8f, 0.2f, 1.0f};
                levelStr = "[WARN] ";
                break;
            case LogLevel::Error:
                textColor = ImVec4{1.0f, 0.3f, 0.3f, 1.0f};
                levelStr = "[ERROR]";
                break;
            default:
                textColor = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
                levelStr = "[?????]";
                break;
        }

        ImVec4 sourceColor = (log.source == LogSource::Engine)
            ? ImVec4{0.4f, 0.7f, 1.0f, 1.0f}
            : ImVec4{0.4f, 1.0f, 0.4f, 1.0f};

        const char* sourceStr = (log.source == LogSource::Engine) ? "[ENGINE]" : "[GAME]  ";

        ImGui::PushStyleColor(ImGuiCol_Text, sourceColor);
        ImGui::Text("%s", sourceStr);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImVec4 categoryColor = ConsoleLogger::GetCategoryColor(log.category);
        const char* categoryStr = Logger::GetCategoryName(log.category);
        ImGui::PushStyleColor(ImGuiCol_Text, categoryColor);
        ImGui::Text("[%s]", categoryStr);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::Text("%s", levelStr);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::TextWrapped("%s", log.message.c_str());
        ImGui::PopStyleColor();

        ImGui::PopID();

        visibleLineIndex++;
    }

    if (*m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    ImGui::End();
}



}

#endif
