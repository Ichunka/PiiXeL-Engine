#ifdef BUILD_WITH_EDITOR

#include "Debug/Profiler.hpp"

#include <algorithm>
#include <iomanip>
#include <raylib.h>
#include <sstream>

namespace PiiXeL {

Profiler& Profiler::Instance() {
    static Profiler instance;
    return instance;
}

void Profiler::BeginFrame() {
    if (!m_Enabled)
        return;

    m_FrameStart = std::chrono::high_resolution_clock::now();
    m_CurrentDepth = 0;

    for (auto& [name, data] : m_Scopes)
    {
        data.totalDuration = 0.0;
        data.callCount = 0;
        data.firstStartTime = 0.0;
        data.depth = 0;
    }
}

void Profiler::EndFrame() {
    if (!m_Enabled)
        return;

    auto frameEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> frameDuration = frameEnd - m_FrameStart;
    m_FrameTime = frameDuration.count();
    m_FPS = 1000.0 / m_FrameTime;

    m_Results.clear();
    for (const auto& [name, data] : m_Scopes)
    {
        if (data.callCount > 0)
        { m_Results.push_back({name, data.totalDuration, data.callCount, data.firstStartTime, data.depth}); }
    }

    std::sort(m_Results.begin(), m_Results.end(),
              [](const ProfileResult& a, const ProfileResult& b) { return a.startTime < b.startTime; });

    if (m_Recording)
    {
        FrameSnapshot snapshot;
        snapshot.results = m_Results;
        snapshot.frameTime = m_FrameTime;
        snapshot.fps = m_FPS;

        m_FrameHistory.push_back(snapshot);

        if (m_FrameHistory.size() > MAX_HISTORY)
        { m_FrameHistory.pop_front(); }
    }
}

void Profiler::BeginScope(const std::string& name) {
    if (!m_Enabled)
        return;

    auto now = std::chrono::high_resolution_clock::now();
    auto& scope = m_Scopes[name];
    scope.startTime = now;

    if (scope.callCount == 0)
    {
        std::chrono::duration<double, std::milli> elapsed = now - m_FrameStart;
        scope.firstStartTime = elapsed.count();
        scope.depth = m_CurrentDepth;
    }

    m_CurrentDepth++;
}

void Profiler::EndScope(const std::string& name) {
    if (!m_Enabled)
        return;

    m_CurrentDepth--;

    auto endTime = std::chrono::high_resolution_clock::now();
    auto& scope = m_Scopes[name];

    std::chrono::duration<double, std::milli> duration = endTime - scope.startTime;
    scope.totalDuration += duration.count();
    scope.callCount++;
}

std::string Profiler::GetCurrentFrameAsText() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);

    ss << "Frame Time: " << m_FrameTime << " ms\n";
    ss << "FPS: " << std::setprecision(1) << m_FPS << "\n\n";
    ss << std::setprecision(3);

    ss << "Scope Timings:\n";
    ss << "----------------------------------------\n";

    for (const ProfileResult& result : m_Results)
    {
        float percentage = static_cast<float>(result.duration / m_FrameTime) * 100.0f;
        ss << std::string(result.depth * 2, ' ');
        ss << result.name << ": " << result.duration << " ms (" << std::setprecision(1) << percentage << "%) ["
           << result.callCount << " calls]\n";
        ss << std::setprecision(3);
    }

    return ss.str();
}

void Profiler::CopyFrameToClipboard() const {
    std::string text = GetCurrentFrameAsText();
    SetClipboardText(text.c_str());
}

} // namespace PiiXeL

#endif
