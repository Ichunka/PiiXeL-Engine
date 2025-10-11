#include "Components/BoxCollider2D.hpp"
#include "Reflection/Reflection.hpp"

namespace PiiXeL {

BEGIN_REFLECT(BoxCollider2D)
    FIELD(size)
    FIELD(offset)
    FIELD(isTrigger)
END_REFLECT(BoxCollider2D)

namespace Reflection {
void __force_link_BoxCollider2D() {}
}

}
