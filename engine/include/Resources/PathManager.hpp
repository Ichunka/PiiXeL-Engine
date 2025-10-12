#ifndef PIIXELENGINE_PATHMANAGER_HPP
#define PIIXELENGINE_PATHMANAGER_HPP

#include <string>

namespace PiiXeL {

class PathManager {
public:
    PathManager(const PathManager&) = delete;
    PathManager& operator=(const PathManager&) = delete;

    static PathManager& Instance();

    void Initialize();

    [[nodiscard]] std::string GetEngineAssetsPath(const std::string& relativePath = "") const;
    [[nodiscard]] std::string GetGameAssetsPath(const std::string& relativePath = "") const;

private:
    PathManager() = default;

    std::string m_EngineAssetsPath;
    std::string m_GameAssetsPath;
    bool m_Initialized{false};
};

} // namespace PiiXeL

#endif // PIIXELENGINE_PATHMANAGER_HPP
