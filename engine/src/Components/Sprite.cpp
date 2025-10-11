#include "Components/Sprite.hpp"
#include "Resources/AssetManager.hpp"
#include "Reflection/Reflection.hpp"

namespace PiiXeL {

BEGIN_REFLECT(Sprite)
    FIELD(texturePath)
    FIELD(tint)
    FIELD(origin)
    FIELD(layer)
END_REFLECT(Sprite)

Sprite::Sprite() {
    texture = AssetManager::Instance().GetDefaultTexture();
    sourceRect = Rectangle{0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height)};
}

namespace Reflection {
void __force_link_Sprite() {}
}

} // namespace PiiXeL
