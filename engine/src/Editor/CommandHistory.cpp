#ifdef BUILD_WITH_EDITOR

#include "Editor/CommandHistory.hpp"

namespace PiiXeL {

CommandHistory::CommandHistory() {}

CommandHistory::~CommandHistory() {
    Clear();
}

void CommandHistory::ExecuteCommand(std::unique_ptr<Command> command) {
    command->Execute();
    AddCommand(std::move(command));
}

void CommandHistory::AddCommand(std::unique_ptr<Command> command) {
    m_UndoStack.push(std::move(command));

    while (!m_RedoStack.empty()) {
        m_RedoStack.pop();
    }

    while (m_UndoStack.size() > m_MaxHistorySize) {
        std::stack<std::unique_ptr<Command>> temp;
        while (m_UndoStack.size() > 1) {
            temp.push(std::move(const_cast<std::unique_ptr<Command>&>(m_UndoStack.top())));
            m_UndoStack.pop();
        }
        m_UndoStack.pop();

        while (!temp.empty()) {
            m_UndoStack.push(std::move(const_cast<std::unique_ptr<Command>&>(temp.top())));
            temp.pop();
        }
    }
}

void CommandHistory::Undo() {
    if (!CanUndo()) {
        return;
    }

    std::unique_ptr<Command> command = std::move(const_cast<std::unique_ptr<Command>&>(m_UndoStack.top()));
    m_UndoStack.pop();

    command->Undo();
    m_RedoStack.push(std::move(command));
}

void CommandHistory::Redo() {
    if (!CanRedo()) {
        return;
    }

    std::unique_ptr<Command> command = std::move(const_cast<std::unique_ptr<Command>&>(m_RedoStack.top()));
    m_RedoStack.pop();

    command->Redo();
    m_UndoStack.push(std::move(command));
}

bool CommandHistory::CanUndo() const {
    return !m_UndoStack.empty();
}

bool CommandHistory::CanRedo() const {
    return !m_RedoStack.empty();
}

void CommandHistory::Clear() {
    while (!m_UndoStack.empty()) {
        m_UndoStack.pop();
    }
    while (!m_RedoStack.empty()) {
        m_RedoStack.pop();
    }
}

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR
