#ifndef PIIXELENGINE_BUILDSYSTEM_HPP
#define PIIXELENGINE_BUILDSYSTEM_HPP

#include <string>
#include <functional>
#include <vector>
#include <memory>

namespace PiiXeL {

enum class BuildStep {
    Idle,
    ConfiguringCMake,
    CompilingGame,
    BuildingPackage,
    CopyingAssets,
    CopyingDependencies,
    Completed,
    Failed
};

struct BuildProgress {
    BuildStep currentStep;
    float percentage;
    std::string statusMessage;
    bool isRunning;
};

class BuildSystem {
public:
    BuildSystem();
    ~BuildSystem();

    using ProgressCallback = std::function<void(const BuildProgress&)>;

    void BuildGamePackage(ProgressCallback callback);
    void BuildGameExecutable(ProgressCallback callback);
    void ExportGame(const std::string& exportPath, ProgressCallback callback);

    [[nodiscard]] bool IsBuildRunning() const { return m_IsBuilding; }
    [[nodiscard]] const BuildProgress& GetCurrentProgress() const { return m_Progress; }
    [[nodiscard]] std::string GetProjectRoot() const;

    void Cancel();

private:
    void RunBuildProcess(const std::vector<std::string>& commands, ProgressCallback callback);
    void UpdateProgress(BuildStep step, float percentage, const std::string& message);
    std::string GetBuildDirectory() const;
    std::string GetGameDirectory() const;
    bool CopyFileTo(const std::string& src, const std::string& dst);
    bool CopyDirectoryTo(const std::string& src, const std::string& dst);

    BuildProgress m_Progress;
    bool m_IsBuilding;
    bool m_CancelRequested;
    ProgressCallback m_CurrentCallback;

#ifdef _WIN32
    void* m_ProcessHandle;
#else
    int m_ProcessPid;
#endif
};

}

#endif
