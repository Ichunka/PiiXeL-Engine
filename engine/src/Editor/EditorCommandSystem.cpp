#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorCommandSystem.hpp"

#include "Components/Transform.hpp"
#include "Editor/EditorCommands.hpp"

namespace PiiXeL {

EditorCommandSystem::EditorCommandSystem() : m_CommandHistory{}, m_CachedTransform{}, m_IsModifyingTransform{false} {}

EditorCommandSystem::~EditorCommandSystem() = default;

void EditorCommandSystem::ExecuteCommand(std::unique_ptr<Command> command) {
    m_CommandHistory.ExecuteCommand(std::move(command));
}

void EditorCommandSystem::AddCommand(std::unique_ptr<Command> command) {
    m_CommandHistory.AddCommand(std::move(command));
}

void EditorCommandSystem::Undo() {
    m_CommandHistory.Undo();
}

void EditorCommandSystem::Redo() {
    m_CommandHistory.Redo();
}

bool EditorCommandSystem::CanUndo() const {
    return m_CommandHistory.CanUndo();
}

bool EditorCommandSystem::CanRedo() const {
    return m_CommandHistory.CanRedo();
}

void EditorCommandSystem::Clear() {
    m_CommandHistory.Clear();
}

void EditorCommandSystem::BeginTransformEdit(entt::registry* registry, entt::entity entity) {
    if (!registry || !registry->valid(entity) || !registry->all_of<Transform>(entity)) {
        return;
    }

    if (!m_IsModifyingTransform) {
        m_CachedTransform = registry->get<Transform>(entity);
        m_IsModifyingTransform = true;
    }
}

void EditorCommandSystem::EndTransformEdit(entt::registry* registry, entt::entity entity) {
    if (!registry || !registry->valid(entity) || !registry->all_of<Transform>(entity)) {
        return;
    }

    if (m_IsModifyingTransform) {
        const Transform& currentTransform = registry->get<Transform>(entity);

        if (m_CachedTransform.position.x != currentTransform.position.x ||
            m_CachedTransform.position.y != currentTransform.position.y ||
            m_CachedTransform.rotation != currentTransform.rotation ||
            m_CachedTransform.scale.x != currentTransform.scale.x ||
            m_CachedTransform.scale.y != currentTransform.scale.y)
        {
            AddCommand(std::make_unique<ModifyTransformCommand>(registry, entity, m_CachedTransform, currentTransform));
        }

        m_IsModifyingTransform = false;
    }
}

void EditorCommandSystem::UpdateCachedTransform(const Transform& transform) {
    if (!m_IsModifyingTransform) {
        m_CachedTransform = transform;
    }
}

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR
