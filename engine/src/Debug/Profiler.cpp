#include "Debug/Profiler.hpp"

namespace PiiXeL {

Profiler& Profiler::Instance() {
    static Profiler instance;
    return instance;
}

void Profiler::BeginFrame() {
    if (!m_Enabled) return;

    m_FrameStart = std::chrono::high_resolution_clock::now();

    for (auto& [name, data] : m_Scopes) {
        data.totalDuration = 0.0;
        data.callCount = 0;
    }
}

void Profiler::EndFrame() {
    if (!m_Enabled) return;

    auto frameEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> frameDuration = frameEnd - m_FrameStart;
    m_FrameTime = frameDuration.count();
    m_FPS = 1000.0 / m_FrameTime;

    m_Results.clear();
    for (const auto& [name, data] : m_Scopes) {
        if (data.callCount > 0) {
            m_Results.push_back({name, data.totalDuration, data.callCount});
        }
    }
}

void Profiler::BeginScope(const std::string& name) {
    if (!m_Enabled) return;

    auto& scope = m_Scopes[name];
    scope.startTime = std::chrono::high_resolution_clock::now();
}

void Profiler::EndScope(const std::string& name) {
    if (!m_Enabled) return;

    auto endTime = std::chrono::high_resolution_clock::now();
    auto& scope = m_Scopes[name];

    std::chrono::duration<double, std::milli> duration = endTime - scope.startTime;
    scope.totalDuration += duration.count();
    scope.callCount++;
}

} // namespace PiiXeL
