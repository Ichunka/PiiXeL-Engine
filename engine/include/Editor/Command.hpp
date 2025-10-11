#ifndef PIIXELENGINE_COMMAND_HPP
#define PIIXELENGINE_COMMAND_HPP

#ifdef BUILD_WITH_EDITOR

namespace PiiXeL {

class Command {
public:
    virtual ~Command() = default;

    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual void Redo() { Execute(); }
};

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR

#endif // PIIXELENGINE_COMMAND_HPP
