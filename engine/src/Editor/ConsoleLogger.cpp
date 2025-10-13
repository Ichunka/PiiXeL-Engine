#ifdef BUILD_WITH_EDITOR

#include "Editor/ConsoleLogger.hpp"
#include <raylib.h>
#include <cstdio>
#include <cstdarg>

namespace PiiXeL {

ConsoleLogger& ConsoleLogger::Instance() {
    static ConsoleLogger instance;
    return instance;
}

void ConsoleLogger::AddLog(const std::string& message, LogLevel level, LogSource source, LogCategory category, float timestamp) {
    std::lock_guard<std::mutex> lock{m_Mutex};

    m_Logs.emplace_back(message, level, source, category, timestamp);

    if (m_Logs.size() > 10000) {
        m_Logs.erase(m_Logs.begin(), m_Logs.begin() + 1000);
    }
}

void ConsoleLogger::Clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Logs.clear();
}

LogLevel ConsoleLogger::ConvertRaylibLogLevel(int raylibLevel) {
    switch (raylibLevel) {
        case LOG_TRACE:   return LogLevel::Trace;
        case LOG_DEBUG:   return LogLevel::Debug;
        case LOG_INFO:    return LogLevel::Info;
        case LOG_WARNING: return LogLevel::Warning;
        case LOG_ERROR:   return LogLevel::Error;
        default:          return LogLevel::Info;
    }
}

ImVec4 ConsoleLogger::GetCategoryColor(LogCategory category) {
    switch (category) {
        case LogCategory::ENGINE:    return ImVec4{0.4f, 0.7f, 1.0f, 1.0f};
        case LogCategory::ASSET:     return ImVec4{1.0f, 0.7f, 0.4f, 1.0f};
        case LogCategory::EDITOR:    return ImVec4{0.8f, 0.4f, 1.0f, 1.0f};
        case LogCategory::PHYSICS:   return ImVec4{0.4f, 1.0f, 0.6f, 1.0f};
        case LogCategory::RENDER:    return ImVec4{1.0f, 0.4f, 0.6f, 1.0f};
        case LogCategory::SCENE:     return ImVec4{0.6f, 1.0f, 1.0f, 1.0f};
        case LogCategory::SCRIPT:    return ImVec4{1.0f, 1.0f, 0.4f, 1.0f};
        case LogCategory::ANIMATION: return ImVec4{1.0f, 0.6f, 1.0f, 1.0f};
        case LogCategory::BUILD:     return ImVec4{0.7f, 0.7f, 0.4f, 1.0f};
        case LogCategory::GAME:      return ImVec4{0.4f, 1.0f, 0.4f, 1.0f};
        case LogCategory::UNKNOWN:   return ImVec4{0.6f, 0.6f, 0.6f, 1.0f};
        default:                     return ImVec4{0.6f, 0.6f, 0.6f, 1.0f};
    }
}

void ConsoleLogger::RaylibLogCallback(int logLevel, const char* text, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);

    std::string message{buffer};
    LogLevel level{ConvertRaylibLogLevel(logLevel)};

    LogSource source{LogSource::Engine};
    LogCategory category{LogCategory::ENGINE};

    float timestamp{0.0f};
    if (IsWindowReady()) {
        timestamp = static_cast<float>(GetTime());
    }

    ConsoleLogger::Instance().AddLog(message, level, source, category, timestamp);

    #ifdef _DEBUG
    printf("[%s] %s\n", source == LogSource::Engine ? "ENGINE" : "GAME", buffer);
    #endif
}

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR
