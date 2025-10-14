#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorSceneManager.hpp"

#include "Core/Engine.hpp"
#include "Core/Logger.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneSerializer.hpp"

#include <entt/entt.hpp>

namespace PiiXeL {

EditorSceneManager::EditorSceneManager(Engine* engine) : m_Engine{engine}, m_CurrentScenePath{} {}

void EditorSceneManager::NewScene() {
    if (m_Engine && m_Engine->GetActiveScene())
    {
        Scene* scene = m_Engine->GetActiveScene();
        entt::registry& registry = scene->GetRegistry();
        registry.clear();

        scene->SetName("Untitled Scene");
        m_CurrentScenePath.clear();

        PX_LOG_INFO(EDITOR, "New scene created");
    }
}

void EditorSceneManager::SaveScene() {
    if (m_CurrentScenePath.empty())
    {
        SaveSceneAs();
        return;
    }

    if (m_Engine && m_Engine->GetActiveScene())
    {
        Scene* scene = m_Engine->GetActiveScene();
        SceneSerializer serializer{scene};
        serializer.Serialize(m_CurrentScenePath);
    }
}

void EditorSceneManager::SaveSceneAs() {
    if (!m_Engine || !m_Engine->GetActiveScene())
    { return; }

    Scene* scene = m_Engine->GetActiveScene();
    std::string filename = scene->GetName();

    for (char& c : filename)
    {
        if (c == ' ')
        { c = '_'; }
    }

    m_CurrentScenePath = "content/scenes/" + filename + ".scene";

    SceneSerializer serializer{scene};
    serializer.Serialize(m_CurrentScenePath);
}

void EditorSceneManager::LoadScene() {
    if (!m_Engine || !m_Engine->GetActiveScene())
    { return; }

    if (m_CurrentScenePath.empty())
    {
        PX_LOG_WARNING(EDITOR, "No scene path set. Save the scene first.");
        return;
    }

    Scene* scene = m_Engine->GetActiveScene();
    SceneSerializer serializer{scene};
    serializer.Deserialize(m_CurrentScenePath);
}

} // namespace PiiXeL

#endif
