#include "Core/Logger.hpp"

#include <cstdarg>
#include <cstdio>

#ifdef BUILD_WITH_EDITOR
#include "Editor/ConsoleLogger.hpp"

#include <raylib.h>
#endif

namespace PiiXeL {

bool Logger::s_CategoryEnabled[12] = {true, true, true, true, true, true, true, true, true, true, true};

bool Logger::IsCategoryEnabled(LogCategory category) {
    return s_CategoryEnabled[static_cast<int>(category)];
}

void Logger::SetCategoryEnabled(LogCategory category, bool enabled) {
    s_CategoryEnabled[static_cast<int>(category)] = enabled;
}

const char* Logger::GetCategoryName(LogCategory category) {
    switch (category)
    {
        case LogCategory::ENGINE:
            return "ENGINE";
        case LogCategory::ASSET:
            return "ASSET";
        case LogCategory::EDITOR:
            return "EDITOR";
        case LogCategory::PHYSICS:
            return "PHYSICS";
        case LogCategory::RENDER:
            return "RENDER";
        case LogCategory::SCENE:
            return "SCENE";
        case LogCategory::SCRIPT:
            return "SCRIPT";
        case LogCategory::ANIMATION:
            return "ANIMATION";
        case LogCategory::BUILD:
            return "BUILD";
        case LogCategory::GAME:
            return "GAME";
        case LogCategory::UNKNOWN:
            return "UNKNOWN";
        default:
            return "UNKNOWN";
    }
}

void Logger::LogMessage(LogLevel level, LogCategory category, const char* format, ...) {
    const char* levelStr{};
    FILE* output{stdout};

    switch (level)
    {
        case LogLevel::Trace:
            levelStr = "TRACE";
            break;
        case LogLevel::Debug:
            levelStr = "DEBUG";
            break;
        case LogLevel::Info:
            levelStr = "INFO";
            break;
        case LogLevel::Warning:
            levelStr = "WARNING";
            output = stderr;
            break;
        case LogLevel::Error:
            levelStr = "ERROR";
            output = stderr;
            break;
    }

    const char* categoryStr{GetCategoryName(category)};

    char buffer[4096];
    std::va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    std::fprintf(output, "%s: [%s] %s\n", levelStr, categoryStr, buffer);
    std::fflush(output);

#ifdef BUILD_WITH_EDITOR
    ConsoleLogger::Instance().AddLog(buffer, level, LogSource::Runtime, category, static_cast<float>(GetTime()));
#endif
}

} // namespace PiiXeL
