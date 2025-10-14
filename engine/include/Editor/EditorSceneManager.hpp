#pragma once

#ifdef BUILD_WITH_EDITOR

#include <string>

namespace PiiXeL {

class Engine;

class EditorSceneManager {
public:
    explicit EditorSceneManager(Engine* engine);

    void NewScene();
    void SaveScene();
    void SaveSceneAs();
    void LoadScene();

    [[nodiscard]] const std::string& GetCurrentScenePath() const { return m_CurrentScenePath; }
    void SetCurrentScenePath(const std::string& path) { m_CurrentScenePath = path; }

private:
    Engine* m_Engine;
    std::string m_CurrentScenePath;
};

}

#endif
