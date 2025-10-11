#ifndef PIIXELENGINE_TRANSFORM_HPP
#define PIIXELENGINE_TRANSFORM_HPP

#include <raylib.h>

namespace PiiXeL {

struct Transform {
    Vector2 position{0.0f, 0.0f};
    float rotation{0.0f};
    Vector2 scale{1.0f, 1.0f};

    Transform() = default;
    Transform(const Vector2& pos, float rot = 0.0f, const Vector2& scl = {1.0f, 1.0f})
        : position{pos}, rotation{rot}, scale{scl} {}

    [[nodiscard]] Vector2 GetRight() const;
    [[nodiscard]] Vector2 GetUp() const;
    [[nodiscard]] Matrix GetMatrix() const;
};

} // namespace PiiXeL

#endif // PIIXELENGINE_TRANSFORM_HPP
