#ifndef PIIXELENGINE_PROFILER_HPP
#define PIIXELENGINE_PROFILER_HPP

#ifdef BUILD_WITH_EDITOR

#include <chrono>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace PiiXeL {

struct ProfileResult {
    std::string name;
    double duration;
    size_t callCount;
    double startTime;
    int depth;
};

struct FrameSnapshot {
    std::vector<ProfileResult> results;
    double frameTime;
    double fps;
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

    void SetRecording(bool recording) { m_Recording = recording; }
    bool IsRecording() const { return m_Recording; }

    const std::deque<FrameSnapshot>& GetFrameHistory() const { return m_FrameHistory; }
    void ClearHistory() { m_FrameHistory.clear(); }

    std::string GetCurrentFrameAsText() const;
    void CopyFrameToClipboard() const;

private:
    Profiler() = default;

    struct ScopeData {
        std::chrono::high_resolution_clock::time_point startTime;
        double totalDuration;
        size_t callCount;
        double firstStartTime;
        int depth;
    };

    bool m_Enabled{false};
    bool m_Recording{false};
    std::unordered_map<std::string, ScopeData> m_Scopes;
    std::vector<ProfileResult> m_Results;
    std::chrono::high_resolution_clock::time_point m_FrameStart;
    double m_FrameTime{0.0};
    double m_FPS{0.0};
    int m_CurrentDepth{0};
    std::deque<FrameSnapshot> m_FrameHistory;
    static constexpr size_t MAX_HISTORY = 300;
};

class ProfileScope {
public:
    explicit ProfileScope(const std::string& name) : m_Name(name) {
        if (Profiler::Instance().IsEnabled())
        { Profiler::Instance().BeginScope(m_Name); }
    }

    ~ProfileScope() {
        if (Profiler::Instance().IsEnabled())
        { Profiler::Instance().EndScope(m_Name); }
    }

private:
    std::string m_Name;
};

#define PROFILE_SCOPE_CONCAT(a, b) a##b
#define PROFILE_SCOPE_EXPAND(name, line) PROFILE_SCOPE_CONCAT(profileScope, line)
#define PROFILE_SCOPE_IMPL(name, line) PiiXeL::ProfileScope PROFILE_SCOPE_EXPAND(name, line)(name)
#define PROFILE_SCOPE(name) PROFILE_SCOPE_IMPL(name, __LINE__)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)

} // namespace PiiXeL

#else

#define PROFILE_SCOPE(name) ((void)0)
#define PROFILE_FUNCTION() ((void)0)

#endif

#endif
