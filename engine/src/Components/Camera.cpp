#include "Components/Camera.hpp"
#include "Reflection/Reflection.hpp"

namespace PiiXeL {

BEGIN_REFLECT(Camera)
    FIELD(isPrimary)
    FIELD(offset)
    FIELD_RANGE(zoom, 0.1f, 10.0f, 0.1f)
    FIELD_RANGE(rotation, -360.0f, 360.0f, 1.0f)
END_REFLECT(Camera)

namespace Reflection {
void __force_link_Camera() {}
}

}
