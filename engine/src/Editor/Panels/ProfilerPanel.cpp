#ifdef BUILD_WITH_EDITOR

#include "Editor/Panels/ProfilerPanel.hpp"

#include "Debug/Profiler.hpp"

#include <algorithm>
#include <imgui.h>
#include <raylib.h>

namespace PiiXeL {

ProfilerPanel::ProfilerPanel(bool* paused, FrameSnapshot* pausedSnapshot, int* selectedFrame,
                             FrameSnapshot* selectedFrameSnapshot, float* flameGraphZoom, float* flameGraphScroll) :
    m_Paused{paused},
    m_PausedSnapshot{pausedSnapshot}, m_SelectedFrame{selectedFrame}, m_SelectedFrameSnapshot{selectedFrameSnapshot},
    m_FlameGraphZoom{flameGraphZoom}, m_FlameGraphScroll{flameGraphScroll} {}

void ProfilerPanel::OnImGuiRender() {
    PROFILE_FUNCTION();
    ImGui::Begin("Profiler");

    Profiler& profiler = Profiler::Instance();

    bool enabled = profiler.IsEnabled();
    if (ImGui::Checkbox("Enable Profiler", &enabled))
    { profiler.SetEnabled(enabled); }

    if (!profiler.IsEnabled())
    {
        ImGui::End();
        return;
    }

    ImGui::SameLine();
    bool recording = profiler.IsRecording();
    if (ImGui::Checkbox("Record", &recording))
    { profiler.SetRecording(recording); }

    ImGui::SameLine();
    if (ImGui::Button(*m_Paused ? "Resume" : "Pause"))
    {
        *m_Paused = !*m_Paused;
        if (*m_Paused)
        {
            m_PausedSnapshot->results = profiler.GetResults();
            m_PausedSnapshot->frameTime = profiler.GetFrameTime();
            m_PausedSnapshot->fps = profiler.GetFPS();
        }
    }

    const FrameSnapshot* displaySnapshot = nullptr;
    if (*m_SelectedFrame >= 0)
    { displaySnapshot = m_SelectedFrameSnapshot; }
    else if (*m_Paused)
    { displaySnapshot = m_PausedSnapshot; }

    ImGui::SameLine();
    if (ImGui::Button("Copy Frame"))
    {
        if (displaySnapshot)
        {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(3);
            ss << "Frame Time: " << displaySnapshot->frameTime << " ms\n";
            ss << "FPS: " << std::setprecision(1) << displaySnapshot->fps << "\n\n";
            ss << std::setprecision(3);
            ss << "Scope Timings:\n";
            ss << "----------------------------------------\n";

            for (const ProfileResult& result : displaySnapshot->results)
            {
                float percentage = static_cast<float>(result.duration / displaySnapshot->frameTime) * 100.0f;
                ss << std::string(result.depth * 2, ' ');
                ss << result.name << ": " << result.duration << " ms (" << std::setprecision(1) << percentage << "%) ["
                   << result.callCount << " calls]\n";
                ss << std::setprecision(3);
            }

            SetClipboardText(ss.str().c_str());
        }
        else
        { profiler.CopyFrameToClipboard(); }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear History"))
    {
        profiler.ClearHistory();
        *m_SelectedFrame = -1;
    }

    ImGui::Separator();

    if (displaySnapshot)
    {
        if (*m_SelectedFrame >= 0)
        { ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Viewing Frame: %d", *m_SelectedFrame); }
        else
        { ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "PAUSED"); }
    }

    if (displaySnapshot)
    {
        ImGui::Text("Frame Time: %.3f ms", displaySnapshot->frameTime);
        ImGui::Text("FPS: %.1f", displaySnapshot->fps);
    }
    else
    {
        ImGui::Text("Frame Time: %.3f ms", profiler.GetFrameTime());
        ImGui::Text("FPS: %.1f", profiler.GetFPS());
    }

    if (profiler.IsRecording())
    { ImGui::Text("Recorded: %zu frames", profiler.GetFrameHistory().size()); }

    ImGui::Separator();

    const std::vector<ProfileResult>* currentResults = nullptr;
    double currentFrameTime = 0.0;

    if (displaySnapshot)
    {
        currentResults = &displaySnapshot->results;
        currentFrameTime = displaySnapshot->frameTime;
    }
    else
    {
        currentResults = &profiler.GetResults();
        currentFrameTime = profiler.GetFrameTime();
    }

    if (ImGui::BeginTabBar("ProfilerTabs"))
    {
        if (ImGui::BeginTabItem("Current Frame"))
        {
            if (ImGui::BeginTable("ProfilerTable", 3,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Time (ms)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableHeadersRow();

                for (const ProfileResult& result : *currentResults)
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Indent(result.depth * 15.0f);
                    ImGui::Text("%s", result.name.c_str());
                    ImGui::Unindent(result.depth * 15.0f);

                    ImGui::TableSetColumnIndex(1);
                    float percentage = static_cast<float>(result.duration / currentFrameTime) * 100.0f;
                    ImGui::Text("%.3f (%.1f%%)", result.duration, percentage);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%zu", result.callCount);
                }

                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Flame Graph"))
        {
            if (*m_SelectedFrame >= 0)
            { ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Viewing Frame: %d", *m_SelectedFrame); }
            else if (*m_Paused)
            { ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "PAUSED"); }

            ImGui::Text("Frame Time: %.3f ms", currentFrameTime);
            ImGui::SameLine();
            ImGui::Text("Zoom: %.1fx", *m_FlameGraphZoom);
            ImGui::SameLine();
            if (ImGui::Button("Reset Zoom"))
            {
                *m_FlameGraphZoom = 1.0f;
                *m_FlameGraphScroll = 0.0f;
            }
            ImGui::SameLine();
            ImGui::Text("Use mouse wheel to zoom, middle-click drag to pan");

            ImVec2 canvasPos = ImGui::GetCursorScreenPos();
            ImVec2 canvasSize = ImGui::GetContentRegionAvail();
            canvasSize.y = 450.0f;

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                                    IM_COL32(20, 20, 20, 255));

            float barHeight = 25.0f;
            float frameTime = static_cast<float>(currentFrameTime);

            ImVec2 mousePos = ImGui::GetMousePos();
            bool isHovering = mousePos.x >= canvasPos.x && mousePos.x <= canvasPos.x + canvasSize.x &&
                              mousePos.y >= canvasPos.y && mousePos.y <= canvasPos.y + canvasSize.y;

            if (isHovering)
            {
                float mouseWheel = ImGui::GetIO().MouseWheel;
                if (mouseWheel != 0.0f)
                {
                    float oldZoom = *m_FlameGraphZoom;
                    *m_FlameGraphZoom *= (1.0f + mouseWheel * 0.1f);
                    *m_FlameGraphZoom = std::max(1.0f, std::min(*m_FlameGraphZoom, 500.0f));

                    float mouseRelativeX = (mousePos.x - canvasPos.x) / canvasSize.x;
                    float worldPosAtMouse = (mouseRelativeX + *m_FlameGraphScroll) / oldZoom;
                    *m_FlameGraphScroll = worldPosAtMouse * *m_FlameGraphZoom - mouseRelativeX;
                }

                if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
                {
                    ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
                    *m_FlameGraphScroll -= delta.x / canvasSize.x;
                    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
                }
            }

            *m_FlameGraphScroll = std::max(0.0f, std::min(*m_FlameGraphScroll, *m_FlameGraphZoom - 1.0f));

            drawList->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);

            const ProfileResult* hoveredResult = nullptr;

            for (const ProfileResult& result : *currentResults)
            {
                float normalizedStart = static_cast<float>(result.startTime / frameTime);
                float normalizedDuration = static_cast<float>(result.duration / frameTime);

                float xStart = canvasPos.x + (normalizedStart * *m_FlameGraphZoom - *m_FlameGraphScroll) * canvasSize.x;
                float width = normalizedDuration * *m_FlameGraphZoom * canvasSize.x;
                float yStart = canvasPos.y + result.depth * barHeight;

                if (xStart + width < canvasPos.x || xStart > canvasPos.x + canvasSize.x)
                    continue;
                if (width < 0.5f)
                    continue;

                bool isHovered = isHovering && mousePos.x >= xStart && mousePos.x <= xStart + width &&
                                 mousePos.y >= yStart && mousePos.y <= yStart + barHeight - 2;

                if (isHovered)
                { hoveredResult = &result; }

                ImU32 color = IM_COL32(100 + (result.depth * 30) % 155, 150 + (result.depth * 40) % 105,
                                       200 - (result.depth * 20) % 100, isHovered ? 255 : 200);

                drawList->AddRectFilled(ImVec2(xStart, yStart), ImVec2(xStart + width, yStart + barHeight - 2), color);

                drawList->AddRect(ImVec2(xStart, yStart), ImVec2(xStart + width, yStart + barHeight - 2),
                                  isHovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 255, 255, 100), 0.0f, 0,
                                  isHovered ? 2.0f : 1.0f);

                if (width > 30.0f)
                {
                    drawList->AddText(ImVec2(xStart + 2, yStart + 4), IM_COL32(255, 255, 255, 255),
                                      result.name.c_str());
                }
            }

            drawList->PopClipRect();

            ImGui::Dummy(canvasSize);

            if (hoveredResult)
            {
                ImGui::BeginTooltip();
                ImGui::Text("Scope: %s", hoveredResult->name.c_str());
                ImGui::Text("Duration: %.3f ms", hoveredResult->duration);
                ImGui::Text("Percentage: %.1f%%", static_cast<float>(hoveredResult->duration / frameTime) * 100.0f);
                ImGui::Text("Calls: %zu", hoveredResult->callCount);
                ImGui::Text("Depth: %d", hoveredResult->depth);
                ImGui::EndTooltip();
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("History"))
        {
            const std::deque<FrameSnapshot>& history = profiler.GetFrameHistory();

            if (history.empty())
            { ImGui::Text("No recorded frames. Enable 'Record' to capture frames."); }
            else
            {
                ImGui::Text("Click on a frame to inspect it. Right-click to deselect.");

                ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                ImVec2 canvasSize = ImGui::GetContentRegionAvail();
                canvasSize.y = 250.0f;

                ImDrawList* drawList = ImGui::GetWindowDrawList();

                drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                                        IM_COL32(20, 20, 20, 255));

                float maxFrameTime = 16.67f;
                for (const auto& snapshot : history)
                {
                    if (snapshot.frameTime > maxFrameTime)
                    { maxFrameTime = static_cast<float>(snapshot.frameTime); }
                }

                float barWidth = canvasSize.x / static_cast<float>(history.size());

                ImVec2 mousePos = ImGui::GetMousePos();
                bool isHovering = mousePos.x >= canvasPos.x && mousePos.x <= canvasPos.x + canvasSize.x &&
                                  mousePos.y >= canvasPos.y && mousePos.y <= canvasPos.y + canvasSize.y;

                int hoveredFrame = -1;

                for (size_t i = 0; i < history.size(); ++i)
                {
                    const FrameSnapshot& snapshot = history[i];
                    float x = canvasPos.x + i * barWidth;
                    float height = static_cast<float>(snapshot.frameTime / maxFrameTime) * canvasSize.y;
                    float y = canvasPos.y + canvasSize.y - height;

                    bool isBarHovered = isHovering && mousePos.x >= x && mousePos.x <= x + barWidth && mousePos.y >= y;

                    if (isBarHovered)
                    { hoveredFrame = static_cast<int>(i); }

                    ImU32 color;
                    if (static_cast<int>(i) == *m_SelectedFrame)
                    { color = IM_COL32(255, 255, 0, 255); }
                    else if (isBarHovered)
                    {
                        color =
                            snapshot.frameTime > 16.67f ? IM_COL32(255, 150, 150, 255) : IM_COL32(150, 255, 150, 255);
                    }
                    else
                    {
                        color =
                            snapshot.frameTime > 16.67f ? IM_COL32(255, 100, 100, 255) : IM_COL32(100, 255, 100, 255);
                    }

                    drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + barWidth - 1, canvasPos.y + canvasSize.y), color);
                }

                drawList->AddLine(
                    ImVec2(canvasPos.x, canvasPos.y + canvasSize.y - (16.67f / maxFrameTime) * canvasSize.y),
                    ImVec2(canvasPos.x + canvasSize.x,
                           canvasPos.y + canvasSize.y - (16.67f / maxFrameTime) * canvasSize.y),
                    IM_COL32(255, 255, 0, 200), 2.0f);

                ImGui::Dummy(canvasSize);

                if (hoveredFrame >= 0 && hoveredFrame < static_cast<int>(history.size()))
                {
                    const FrameSnapshot& snapshot = history[hoveredFrame];
                    ImGui::BeginTooltip();
                    ImGui::Text("Frame: %d", hoveredFrame);
                    ImGui::Text("Time: %.3f ms", snapshot.frameTime);
                    ImGui::Text("FPS: %.1f", snapshot.fps);
                    ImGui::EndTooltip();

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        *m_SelectedFrame = hoveredFrame;
                        *m_SelectedFrameSnapshot = snapshot;
                    }
                }

                if (isHovering && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                { *m_SelectedFrame = -1; }

                ImGui::Text("Yellow line: 60 FPS target (16.67ms)");
                if (*m_SelectedFrame >= 0)
                { ImGui::Text("Selected Frame: %d", *m_SelectedFrame); }
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace PiiXeL

#endif
