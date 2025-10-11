#ifndef PIIXELENGINE_PROFILER_HPP
#define PIIXELENGINE_PROFILER_HPP

#include <string>
#include <chrono>
#include <vector>
#include <unordered_map>

namespace PiiXeL {

struct ProfileResult {
    std::string name;
    double duration;
    size_t callCount;
};

class Profiler {
public:
    static Profiler& Instance();

    void BeginFrame();
    void EndFrame();

    void BeginScope(const std::string& name);
    void EndScope(const std::string& name);

    const std::vector<ProfileResult>& GetResults() const { return m_Results; }
    double GetFrameTime() const { return m_FrameTime; }
    double GetFPS() const { return m_FPS; }

    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

private:
    Profiler() = default;

    struct ScopeData {
        std::chrono::high_resolution_clock::time_point startTime;
        double totalDuration;
        size_t callCount;
    };

    bool m_Enabled{false};
    std::unordered_map<std::string, ScopeData> m_Scopes;
    std::vector<ProfileResult> m_Results;
    std::chrono::high_resolution_clock::time_point m_FrameStart;
    double m_FrameTime{0.0};
    double m_FPS{0.0};
};

class ProfileScope {
public:
    explicit ProfileScope(const std::string& name) : m_Name(name) {
        if (Profiler::Instance().IsEnabled()) {
            Profiler::Instance().BeginScope(m_Name);
        }
    }

    ~ProfileScope() {
        if (Profiler::Instance().IsEnabled()) {
            Profiler::Instance().EndScope(m_Name);
        }
    }

private:
    std::string m_Name;
};

#define PROFILE_SCOPE(name) PiiXeL::ProfileScope profileScope##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)

} // namespace PiiXeL

#endif
