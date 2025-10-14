#ifndef PIIXELENGINE_COMMANDHISTORY_HPP
#define PIIXELENGINE_COMMANDHISTORY_HPP

#ifdef BUILD_WITH_EDITOR

#include <memory>
#include <stack>

#include "Command.hpp"

namespace PiiXeL {

class CommandHistory {
public:
    CommandHistory();
    ~CommandHistory();

    void ExecuteCommand(std::unique_ptr<Command> command);
    void AddCommand(std::unique_ptr<Command> command);
    void Undo();
    void Redo();

    [[nodiscard]] bool CanUndo() const;
    [[nodiscard]] bool CanRedo() const;

    void Clear();

private:
    std::stack<std::unique_ptr<Command>> m_UndoStack;
    std::stack<std::unique_ptr<Command>> m_RedoStack;
    const size_t m_MaxHistorySize{100};
};

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR

#endif // PIIXELENGINE_COMMANDHISTORY_HPP
