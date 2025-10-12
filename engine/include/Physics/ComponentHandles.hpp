#ifndef PIIXELENGINE_COMPONENTHANDLES_HPP
#define PIIXELENGINE_COMPONENTHANDLES_HPP

namespace PiiXeL {

struct RigidBody2D;
class RigidBodyHandle;

template<typename Component>
struct ComponentHandle {
    using Type = void;
};

template<>
struct ComponentHandle<RigidBody2D> {
    using Type = RigidBodyHandle;
};

} // namespace PiiXeL

#endif
