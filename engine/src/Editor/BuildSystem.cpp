#include "Editor/BuildSystem.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace PiiXeL {

BuildSystem::BuildSystem() :
    m_IsBuilding(false), m_CancelRequested(false)
#ifdef _WIN32
    ,
    m_ProcessHandle(nullptr)
#else
    ,
    m_ProcessPid(-1)
#endif
{
    m_Progress.currentStep = BuildStep::Idle;
    m_Progress.percentage = 0.0f;
    m_Progress.isRunning = false;
}

BuildSystem::~BuildSystem() {
    Cancel();
}

void BuildSystem::UpdateProgress(BuildStep step, float percentage, const std::string& message) {
    m_Progress.currentStep = step;
    m_Progress.percentage = percentage;
    m_Progress.statusMessage = message;
    m_Progress.isRunning = (step != BuildStep::Idle && step != BuildStep::Completed && step != BuildStep::Failed);

    if (m_CurrentCallback) {
        m_CurrentCallback(m_Progress);
    }
}

std::string BuildSystem::GetProjectRoot() const {
    fs::path currentPath = fs::current_path();

    while (currentPath.has_parent_path()) {
        if (fs::exists(currentPath / "CMakeLists.txt") && fs::exists(currentPath / "engine")) {
            return currentPath.string();
        }
        currentPath = currentPath.parent_path();
    }

    return fs::current_path().string();
}

std::string BuildSystem::GetBuildDirectory() const {
    return GetProjectRoot() + "/build/game";
}

std::string BuildSystem::GetGameDirectory() const {
    return GetProjectRoot() + "/games/MyFirstGame";
}

bool BuildSystem::CopyFileTo(const std::string& src, const std::string& dst) {
    try {
        fs::create_directories(fs::path(dst).parent_path());
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool BuildSystem::CopyDirectoryTo(const std::string& src, const std::string& dst) {
    try {
        fs::create_directories(dst);
        for (const fs::directory_entry& entry : fs::recursive_directory_iterator(src)) {
            const fs::path relativePath = fs::relative(entry.path(), src);
            const fs::path destPath = fs::path(dst) / relativePath;

            if (entry.is_directory()) {
                fs::create_directories(destPath);
            }
            else {
                fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
            }
        }
        return true;
    }
    catch (...) {
        return false;
    }
}

void BuildSystem::RunBuildProcess(const std::vector<std::string>& commands, ProgressCallback callback) {
    m_IsBuilding = true;
    m_CancelRequested = false;
    m_CurrentCallback = callback;

    std::thread buildThread([this, commands]() {
        const std::string projectRoot = GetProjectRoot();
        const std::string buildDir = GetBuildDirectory();

        for (size_t i = 0; i < commands.size() && !m_CancelRequested; ++i) {
            const std::string& cmd = commands[i];
            const float progress = static_cast<float>(i) / static_cast<float>(commands.size());

            std::string stepMessage;
            BuildStep step = BuildStep::CompilingGame;

            if (cmd.find("cmake") != std::string::npos && cmd.find("-B") != std::string::npos) {
                step = BuildStep::ConfiguringCMake;
                stepMessage = "Configuring CMake...";
            }
            else if (cmd.find("cmake --build") != std::string::npos) {
                step = BuildStep::CompilingGame;
                stepMessage = "Compiling game executable...";
            }
            else if (cmd.find("build_package") != std::string::npos) {
                step = BuildStep::BuildingPackage;
                stepMessage = "Building game package...";
            }

            UpdateProgress(step, progress * 100.0f, stepMessage);

#ifdef _WIN32
            SECURITY_ATTRIBUTES sa{};
            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.bInheritHandle = TRUE;
            sa.lpSecurityDescriptor = nullptr;

            HANDLE hStdOutRead = nullptr;
            HANDLE hStdOutWrite = nullptr;

            if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
                UpdateProgress(BuildStep::Failed, 0.0f, "Failed to create pipe");
                m_IsBuilding = false;
                return;
            }

            SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

            STARTUPINFOA si{};
            PROCESS_INFORMATION pi{};
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
            si.wShowWindow = SW_HIDE;
            si.hStdOutput = hStdOutWrite;
            si.hStdError = hStdOutWrite;

            std::string cmdLine = "cmd.exe /C " + cmd + " 2>&1";

            if (!CreateProcessA(nullptr, const_cast<char*>(cmdLine.c_str()), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                                nullptr, projectRoot.c_str(), &si, &pi))
            {
                UpdateProgress(BuildStep::Failed, 0.0f, "Failed to start process");
                CloseHandle(hStdOutRead);
                CloseHandle(hStdOutWrite);
                m_IsBuilding = false;
                return;
            }

            CloseHandle(hStdOutWrite);

            m_ProcessHandle = pi.hProcess;

            std::string outputBuffer;
            char buffer[4096];
            DWORD bytesRead = 0;
            float lastProgress = progress * 100.0f;
            BuildStep lastStep = step;

            while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                outputBuffer += buffer;

                size_t pos = 0;
                while ((pos = outputBuffer.find('\n')) != std::string::npos) {
                    std::string line = outputBuffer.substr(0, pos);
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    if (!line.empty()) {
                        if (line.find("Configuring done") != std::string::npos ||
                            line.find("Generating done") != std::string::npos)
                        {
                            lastStep = BuildStep::ConfiguringCMake;
                            lastProgress = 20.0f;
                        }
                        else if (line.find("Building CXX") != std::string::npos ||
                                 line.find("Linking CXX") != std::string::npos)
                        {
                            lastStep = BuildStep::CompilingGame;
                            lastProgress = 50.0f;
                        }
                        else if (line.find("build_package") != std::string::npos &&
                                 line.find("Linking") != std::string::npos)
                        {
                            lastStep = BuildStep::BuildingPackage;
                            lastProgress = 70.0f;
                        }
                        else if (line.find("Packaging") != std::string::npos ||
                                 line.find("Adding") != std::string::npos)
                        {
                            lastStep = BuildStep::BuildingPackage;
                            lastProgress = 85.0f;
                        }

                        UpdateProgress(lastStep, lastProgress, line);
                    }
                    outputBuffer.erase(0, pos + 1);
                }
            }

            if (!outputBuffer.empty()) {
                UpdateProgress(step, progress * 100.0f, outputBuffer);
            }

            WaitForSingleObject(pi.hProcess, INFINITE);

            DWORD exitCode = 0;
            GetExitCodeProcess(pi.hProcess, &exitCode);

            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            CloseHandle(hStdOutRead);
            m_ProcessHandle = nullptr;

            if (exitCode != 0 && !m_CancelRequested) {
                UpdateProgress(BuildStep::Failed, 0.0f, "Build failed with exit code " + std::to_string(exitCode));
                m_IsBuilding = false;
                return;
            }
#else
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                UpdateProgress(BuildStep::Failed, 0.0f, "Failed to create pipe");
                m_IsBuilding = false;
                return;
            }

            const pid_t pid = fork();
            if (pid == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                dup2(pipefd[1], STDERR_FILENO);
                close(pipefd[1]);

                execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
                exit(1);
            }
            else if (pid > 0) {
                close(pipefd[1]);
                m_ProcessPid = pid;

                std::string outputBuffer;
                char buffer[4096];
                ssize_t bytesRead = 0;
                float lastProgress = progress * 100.0f;
                BuildStep lastStep = step;

                while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                    buffer[bytesRead] = '\0';
                    outputBuffer += buffer;

                    size_t pos = 0;
                    while ((pos = outputBuffer.find('\n')) != std::string::npos) {
                        std::string line = outputBuffer.substr(0, pos);
                        if (!line.empty() && line.back() == '\r') {
                            line.pop_back();
                        }
                        if (!line.empty()) {
                            if (line.find("Configuring done") != std::string::npos ||
                                line.find("Generating done") != std::string::npos)
                            {
                                lastStep = BuildStep::ConfiguringCMake;
                                lastProgress = 20.0f;
                            }
                            else if (line.find("Building CXX") != std::string::npos ||
                                     line.find("Linking CXX") != std::string::npos)
                            {
                                lastStep = BuildStep::CompilingGame;
                                lastProgress = 50.0f;
                            }
                            else if (line.find("build_package") != std::string::npos &&
                                     line.find("Linking") != std::string::npos)
                            {
                                lastStep = BuildStep::BuildingPackage;
                                lastProgress = 70.0f;
                            }
                            else if (line.find("Packaging") != std::string::npos ||
                                     line.find("Adding") != std::string::npos)
                            {
                                lastStep = BuildStep::BuildingPackage;
                                lastProgress = 85.0f;
                            }

                            UpdateProgress(lastStep, lastProgress, line);
                        }
                        outputBuffer.erase(0, pos + 1);
                    }
                }

                if (!outputBuffer.empty()) {
                    UpdateProgress(step, progress * 100.0f, outputBuffer);
                }

                close(pipefd[0]);

                int status = 0;
                waitpid(pid, &status, 0);
                m_ProcessPid = -1;

                if (WIFEXITED(status) && WEXITSTATUS(status) != 0 && !m_CancelRequested) {
                    UpdateProgress(BuildStep::Failed, 0.0f,
                                   "Build failed with exit code " + std::to_string(WEXITSTATUS(status)));
                    m_IsBuilding = false;
                    return;
                }
            }
            else {
                close(pipefd[0]);
                close(pipefd[1]);
                UpdateProgress(BuildStep::Failed, 0.0f, "Failed to fork process");
                m_IsBuilding = false;
                return;
            }
#endif
        }

        if (!m_CancelRequested) {
            UpdateProgress(BuildStep::Completed, 100.0f, "Build completed successfully!");
        }

        m_IsBuilding = false;
    });

    buildThread.detach();
}

void BuildSystem::BuildGamePackage(ProgressCallback callback) {
    if (m_IsBuilding) {
        return;
    }

    const std::string projectRoot = GetProjectRoot();
    const std::string buildDir = GetBuildDirectory();
    const std::string gameDir = GetGameDirectory();

    std::vector<std::string> commands;

#ifdef _WIN32
    const std::string vcvars =
        "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat\"";
    const std::string cmakeCache = buildDir + "\\CMakeCache.txt";
    const std::string tempBatch = buildDir + "\\build_script.bat";

    std::ofstream batchFile(tempBatch);
    batchFile << "@echo off\n";
    batchFile << "call " << vcvars << " >nul 2>&1\n";
    batchFile << "if exist \"" << cmakeCache << "\" del \"" << cmakeCache << "\"\n";
    batchFile << "cmake --preset game -S \"" << projectRoot << "\" -B \"" << buildDir << "\"\n";
    batchFile << "if errorlevel 1 exit /b 1\n";
    batchFile << "cmake --build \"" << buildDir << "\" --target build_package --config Release\n";
    batchFile << "if errorlevel 1 exit /b 1\n";
    batchFile << "cd /d \"" << gameDir << "\"\n";
    batchFile << "\"" << buildDir << "\\build_package.exe\"\n";
    batchFile.close();

    commands.push_back(tempBatch);
#else
    const std::string cmakeCache = buildDir + "/CMakeCache.txt";
    commands.push_back("rm -f \"" + cmakeCache + "\"");
    commands.push_back("cmake -S \"" + projectRoot + "\" -B \"" + buildDir +
                       "\" -DCMAKE_BUILD_TYPE=Release -DBUILD_EDITOR=OFF");
    commands.push_back("cmake --build \"" + buildDir + "\" --target build_package --config Release");
    commands.push_back("cd \"" + gameDir + "\" && \"" + buildDir + "/build_package\"");
#endif

    RunBuildProcess(commands, callback);
}

void BuildSystem::BuildGameExecutable(ProgressCallback callback) {
    if (m_IsBuilding) {
        return;
    }

    const std::string projectRoot = GetProjectRoot();
    const std::string buildDir = GetBuildDirectory();

    std::vector<std::string> commands;

#ifdef _WIN32
    const std::string vcvars =
        "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat\"";
    const std::string cmakeCache = buildDir + "\\CMakeCache.txt";
    const std::string tempBatch = buildDir + "\\build_script.bat";

    std::ofstream batchFile(tempBatch);
    batchFile << "@echo off\n";
    batchFile << "call " << vcvars << " >nul 2>&1\n";
    batchFile << "if exist \"" << cmakeCache << "\" del \"" << cmakeCache << "\"\n";
    batchFile << "cmake --preset game -S \"" << projectRoot << "\" -B \"" << buildDir << "\"\n";
    batchFile << "if errorlevel 1 exit /b 1\n";
    batchFile << "cmake --build \"" << buildDir << "\" --target game --config Release\n";
    batchFile.close();

    commands.push_back(tempBatch);
#else
    const std::string cmakeCache = buildDir + "/CMakeCache.txt";
    commands.push_back("rm -f \"" + cmakeCache + "\"");
    commands.push_back("cmake -S \"" + projectRoot + "\" -B \"" + buildDir +
                       "\" -DCMAKE_BUILD_TYPE=Release -DBUILD_EDITOR=OFF");
    commands.push_back("cmake --build \"" + buildDir + "\" --target game --config Release");
#endif

    RunBuildProcess(commands, callback);
}

void BuildSystem::ExportGame(const std::string& exportPath, ProgressCallback callback) {
    if (m_IsBuilding) {
        return;
    }

    m_IsBuilding = true;
    m_CancelRequested = false;
    m_CurrentCallback = callback;

    std::thread exportThread([this, exportPath]() {
        const std::string buildDir = GetBuildDirectory();
        const std::string gameDir = GetGameDirectory();

        UpdateProgress(BuildStep::ConfiguringCMake, 0.0f, "Configuring CMake...");

        const std::string projectRoot = GetProjectRoot();
        std::vector<std::string> buildCommands;
#ifdef _WIN32
        const std::string vcvars =
            "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat\"";
        const std::string cmakeCache = buildDir + "\\CMakeCache.txt";
        const std::string tempBatch = buildDir + "\\build_script.bat";

        std::ofstream batchFile(tempBatch);
        batchFile << "@echo off\n";
        batchFile << "call " << vcvars << " >nul 2>&1\n";
        batchFile << "if exist \"" << cmakeCache << "\" del \"" << cmakeCache << "\"\n";
        batchFile << "cmake --preset game -S \"" << projectRoot << "\" -B \"" << buildDir << "\"\n";
        batchFile << "if errorlevel 1 exit /b 1\n";
        batchFile << "cmake --build \"" << buildDir << "\" --target game --config Release\n";
        batchFile << "if errorlevel 1 exit /b 1\n";
        batchFile << "cmake --build \"" << buildDir << "\" --target build_package --config Release\n";
        batchFile << "if errorlevel 1 exit /b 1\n";
        batchFile << "cd /d \"" << gameDir << "\"\n";
        batchFile << "\"" << buildDir << "\\build_package.exe\"\n";
        batchFile.close();

        buildCommands.push_back(tempBatch);
#else
        const std::string cmakeCache = buildDir + "/CMakeCache.txt";
        buildCommands.push_back("rm -f \"" + cmakeCache + "\"");
        buildCommands.push_back("cmake -S \"" + projectRoot + "\" -B \"" + buildDir +
                                "\" -DCMAKE_BUILD_TYPE=Release -DBUILD_EDITOR=OFF");
        buildCommands.push_back("cmake --build \"" + buildDir + "\" --target game --config Release");
        buildCommands.push_back("cmake --build \"" + buildDir + "\" --target build_package --config Release");
        buildCommands.push_back("cd \"" + gameDir + "\" && \"" + buildDir + "/build_package\"");
#endif

        RunBuildProcess(buildCommands, m_CurrentCallback);

        while (m_IsBuilding && !m_CancelRequested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (m_CancelRequested || m_Progress.currentStep == BuildStep::Failed) {
            m_IsBuilding = false;
            return;
        }

        UpdateProgress(BuildStep::CopyingAssets, 70.0f, "Copying game files...");

        fs::create_directories(exportPath);

#ifdef _WIN32
        const std::string exeName = "game.exe";
#else
        const std::string exeName = "game";
#endif
        const std::string exePath = buildDir + "/games/MyFirstGame/" + exeName;

        if (!CopyFileTo(exePath, exportPath + "/" + exeName)) {
            UpdateProgress(BuildStep::Failed, 0.0f, "Failed to copy game executable");
            m_IsBuilding = false;
            return;
        }

        UpdateProgress(BuildStep::CopyingAssets, 80.0f, "Copying game package...");

        const std::string packagePath = gameDir + "/datas/game.package";
        const std::string exportDataDir = exportPath + "/datas";
        fs::create_directories(exportDataDir);

        if (fs::exists(packagePath)) {
            if (!CopyFileTo(packagePath, exportDataDir + "/game.package")) {
                UpdateProgress(BuildStep::Failed, 0.0f, "Failed to copy game package");
                m_IsBuilding = false;
                return;
            }
        }

        UpdateProgress(BuildStep::CopyingDependencies, 90.0f, "Copying dependencies...");

#ifdef _WIN32
        std::vector<std::string> windowsDlls = {"raylib.dll", "box2d.dll"};

        for (const std::string& dll : windowsDlls) {
            const std::string dllPath = buildDir + "/" + dll;
            if (fs::exists(dllPath)) {
                CopyFileTo(dllPath, exportPath + "/" + dll);
            }
        }
#endif

        UpdateProgress(BuildStep::Completed, 100.0f, "Export completed successfully!");
        m_IsBuilding = false;
    });

    exportThread.detach();
}

void BuildSystem::Cancel() {
    if (!m_IsBuilding) {
        return;
    }

    m_CancelRequested = true;

#ifdef _WIN32
    if (m_ProcessHandle != nullptr) {
        TerminateProcess(m_ProcessHandle, 1);
        CloseHandle(m_ProcessHandle);
        m_ProcessHandle = nullptr;
    }
#else
    if (m_ProcessPid > 0) {
        kill(m_ProcessPid, SIGTERM);
        m_ProcessPid = -1;
    }
#endif

    UpdateProgress(BuildStep::Failed, 0.0f, "Build cancelled");
    m_IsBuilding = false;
}

} // namespace PiiXeL
