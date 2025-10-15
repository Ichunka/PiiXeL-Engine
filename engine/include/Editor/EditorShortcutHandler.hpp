#pragma once

#ifdef BUILD_WITH_EDITOR

namespace PiiXeL {

class Engine;
class EditorCommandSystem;
class EditorSceneManager;
class EditorSelectionManager;
class EditorStateManager;

class EditorShortcutHandler {
public:
    static void HandleShortcuts(Engine* engine, EditorCommandSystem* commandSystem, EditorSceneManager* sceneManager,
                                EditorSelectionManager* selectionManager, EditorStateManager* stateManager);
};

} // namespace PiiXeL

#endif
