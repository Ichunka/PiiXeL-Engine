#ifndef PIIXELENGINE_COMPONENTHANDLES_HPP
#define PIIXELENGINE_COMPONENTHANDLES_HPP

namespace PiiXeL {

struct RigidBody2D;
class RigidBodyHandle;

struct Animator;
class AnimatorHandle;

template <typename Component>
struct ComponentHandle {
    using Type = void;
};

template <>
struct ComponentHandle<RigidBody2D> {
    using Type = RigidBodyHandle;
};

template <>
struct ComponentHandle<Animator> {
    using Type = AnimatorHandle;
};

} // namespace PiiXeL

#endif
