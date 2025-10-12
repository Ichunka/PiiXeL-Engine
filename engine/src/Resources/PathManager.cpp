#include "Resources/PathManager.hpp"
#include <filesystem>
#include <raylib.h>

namespace PiiXeL {

PathManager& PathManager::Instance() {
    static PathManager instance;
    return instance;
}

void PathManager::Initialize() {
    if (m_Initialized) {
        return;
    }

    std::filesystem::path executablePath{GetApplicationDirectory()};

    std::filesystem::path engineAssetsPath = executablePath / ".." / ".." / "engine" / "assets";
    engineAssetsPath = std::filesystem::weakly_canonical(engineAssetsPath);

    m_EngineAssetsPath = engineAssetsPath.string();

    m_GameAssetsPath = GetWorkingDirectory();

    m_Initialized = true;

    TraceLog(LOG_INFO, "PathManager initialized");
    TraceLog(LOG_INFO, ("Engine assets path: " + m_EngineAssetsPath).c_str());
    TraceLog(LOG_INFO, ("Game assets path: " + m_GameAssetsPath).c_str());
}

std::string PathManager::GetEngineAssetsPath(const std::string& relativePath) const {
    if (relativePath.empty()) {
        return m_EngineAssetsPath;
    }

    std::filesystem::path fullPath = std::filesystem::path{m_EngineAssetsPath} / relativePath;
    return fullPath.string();
}

std::string PathManager::GetGameAssetsPath(const std::string& relativePath) const {
    if (relativePath.empty()) {
        return m_GameAssetsPath;
    }

    std::filesystem::path fullPath = std::filesystem::path{m_GameAssetsPath} / relativePath;
    return fullPath.string();
}

} // namespace PiiXeL
