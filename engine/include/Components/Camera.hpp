#ifndef PIIXELENGINE_CAMERA_HPP
#define PIIXELENGINE_CAMERA_HPP

#include <raylib.h>

namespace PiiXeL {

struct Camera {
    bool isPrimary{true};
    Vector2 offset{0.0f, 0.0f};
    float zoom{1.0f};
    float rotation{0.0f};

    Camera() = default;

    [[nodiscard]] Camera2D ToRaylib(const Vector2& entityPos) const {
        return Camera2D{offset, entityPos, rotation, zoom};
    }
};

} // namespace PiiXeL

#endif // PIIXELENGINE_CAMERA_HPP
