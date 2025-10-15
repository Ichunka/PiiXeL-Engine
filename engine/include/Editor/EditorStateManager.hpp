#pragma once

#ifdef BUILD_WITH_EDITOR

#include <string>

namespace PiiXeL {

class Engine;
class EditorSceneManager;
class EditorSelectionManager;
class EditorCommandSystem;

enum class EditorState {
    Edit,
    Play
};

class EditorStateManager {
public:
    EditorStateManager();

    void OnPlayButtonPressed(
        Engine* engine,
        EditorSceneManager* sceneManager,
        EditorSelectionManager* selectionManager,
        EditorCommandSystem* commandSystem
    );

    void OnStopButtonPressed(
        Engine* engine,
        EditorSelectionManager* selectionManager,
        EditorCommandSystem* commandSystem
    );

    [[nodiscard]] EditorState GetState() const { return m_EditorState; }
    [[nodiscard]] bool IsPlayMode() const { return m_EditorState == EditorState::Play; }
    [[nodiscard]] bool IsEditMode() const { return m_EditorState == EditorState::Edit; }

private:
    EditorState m_EditorState{EditorState::Edit};
    std::string m_PlayModeSnapshot{};
};

}

#endif
