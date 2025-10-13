#ifndef PIIXELENGINE_BOXCOLLIDER2D_HPP
#define PIIXELENGINE_BOXCOLLIDER2D_HPP

#include <raylib.h>

namespace PiiXeL {

struct CircleCollider2D {
    float radius{0.5f};
    Vector2 offset{0.0f, 0.0f};
    bool isTrigger{false};
    void* box2dFixture{nullptr};
    CircleCollider2D() = default;
    CircleCollider2D(const float r) : radius{r} {}
    CircleCollider2D(const float r, const Vector2& o) : radius{r}, offset{o} {}
};

} // namespace PiiXeL

#endif // PIIXELENGINE_BOXCOLLIDER2D_HPP
