#include "Core/Logger.hpp"

namespace PiiXeL {

bool Logger::s_CategoryEnabled[11] = {
    true,  // ENGINE
    true,  // ASSET
    true,  // EDITOR
    true,  // PHYSICS
    true,  // RENDER
    true,  // SCENE
    true,  // SCRIPT
    true,  // ANIMATION
    true,  // BUILD
    true   // GAME
};

bool Logger::IsCategoryEnabled(LogCategory category) {
    return s_CategoryEnabled[static_cast<int>(category)];
}

void Logger::SetCategoryEnabled(LogCategory category, bool enabled) {
    s_CategoryEnabled[static_cast<int>(category)] = enabled;
}

const char* Logger::GetCategoryName(LogCategory category) {
    switch (category) {
        case LogCategory::ENGINE:    return "ENGINE";
        case LogCategory::ASSET:     return "ASSET";
        case LogCategory::EDITOR:    return "EDITOR";
        case LogCategory::PHYSICS:   return "PHYSICS";
        case LogCategory::RENDER:    return "RENDER";
        case LogCategory::SCENE:     return "SCENE";
        case LogCategory::SCRIPT:    return "SCRIPT";
        case LogCategory::ANIMATION: return "ANIMATION";
        case LogCategory::BUILD:     return "BUILD";
        case LogCategory::GAME:      return "GAME";
        default:                     return "UNKNOWN";
    }
}

} // namespace PiiXeL
