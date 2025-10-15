#ifndef PIIXELENGINE_EDITORCOMMANDSYSTEM_HPP
#define PIIXELENGINE_EDITORCOMMANDSYSTEM_HPP

#ifdef BUILD_WITH_EDITOR

#include "CommandHistory.hpp"
#include "Components/Transform.hpp"

#include <entt/entt.hpp>
#include <memory>

namespace PiiXeL {

class Command;

class EditorCommandSystem {
public:
    EditorCommandSystem();
    ~EditorCommandSystem();

    void ExecuteCommand(std::unique_ptr<Command> command);
    void AddCommand(std::unique_ptr<Command> command);
    void Undo();
    void Redo();

    [[nodiscard]] bool CanUndo() const;
    [[nodiscard]] bool CanRedo() const;

    void Clear();

    void BeginTransformEdit(entt::registry* registry, entt::entity entity);
    void EndTransformEdit(entt::registry* registry, entt::entity entity);
    [[nodiscard]] bool IsModifyingTransform() const { return m_IsModifyingTransform; }
    void UpdateCachedTransform(const Transform& transform);
    [[nodiscard]] const Transform& GetCachedTransform() const { return m_CachedTransform; }

private:
    CommandHistory m_CommandHistory;
    Transform m_CachedTransform;
    bool m_IsModifyingTransform{false};
};

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR

#endif // PIIXELENGINE_EDITORCOMMANDSYSTEM_HPP
