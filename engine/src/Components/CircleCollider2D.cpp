#include "Components/CircleCollider2D.hpp"
#include "Reflection/Reflection.hpp"

namespace PiiXeL {

BEGIN_REFLECT(CircleCollider2D)
    FIELD(radius)
    FIELD(offset)
    FIELD(isTrigger)
END_REFLECT(CircleCollider2D)

namespace Reflection {
void __force_link_BoxCollider2D() {}
}

}
