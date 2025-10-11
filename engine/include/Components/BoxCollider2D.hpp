#ifndef PIIXELENGINE_BOXCOLLIDER2D_HPP
#define PIIXELENGINE_BOXCOLLIDER2D_HPP

#include <raylib.h>

namespace PiiXeL {

struct BoxCollider2D {
    Vector2 size{1.0f, 1.0f};
    Vector2 offset{0.0f, 0.0f};
    bool isTrigger{false};
    void* box2dFixture{nullptr};

    BoxCollider2D() = default;
    BoxCollider2D(const Vector2& s) : size{s} {}
    BoxCollider2D(const Vector2& s, const Vector2& o) : size{s}, offset{o} {}
};

} // namespace PiiXeL

#endif // PIIXELENGINE_BOXCOLLIDER2D_HPP
