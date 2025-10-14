#include "Reflection/ReflectionInit.hpp"

namespace PiiXeL::Reflection {

void __force_link_Tag();
void __force_link_Transform();
void __force_link_Camera();
void __force_link_Sprite();
void __force_link_RigidBody2D();
void __force_link_BoxCollider2D();
void __force_link_CircleCollider2D();
void __force_link_Animator();

void InitializeReflection() {
    __force_link_Tag();
    __force_link_Transform();
    __force_link_Camera();
    __force_link_Sprite();
    __force_link_RigidBody2D();
    __force_link_BoxCollider2D();
    __force_link_CircleCollider2D();
    __force_link_Animator();
}

}
