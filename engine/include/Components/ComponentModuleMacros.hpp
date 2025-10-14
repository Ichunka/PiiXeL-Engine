#ifndef PIIXELENGINE_COMPONENTMODULEMACROS_HPP
#define PIIXELENGINE_COMPONENTMODULEMACROS_HPP

#include "Components/ComponentModule.hpp"
#include "Components/ComponentModuleRegistry.hpp"
#include "Reflection/Reflection.hpp"

#ifdef BUILD_WITH_EDITOR
#include "Reflection/ImGuiRenderer.hpp"
#endif

#define BEGIN_COMPONENT_MODULE(TypeName) \
namespace { \
struct TypeName##_ModuleRegistrar { \
    TypeName##_ModuleRegistrar() { \
        using ReflectedType = TypeName; \
        auto module = std::make_shared<::PiiXeL::ComponentModule<ReflectedType>>(#TypeName); \
        ::PiiXeL::Reflection::TypeBuilder<ReflectedType> reflectionBuilder{#TypeName};

#define REFLECT_FIELDS() \
        {

#define END_REFLECT_MODULE() \
        reflectionBuilder.Register(); \
        }

#define AUTO_SERIALIZATION() \
        module->SetSerializer([](const ReflectedType& component) -> nlohmann::json { \
            return ::PiiXeL::Reflection::JsonSerializer::Serialize(component); \
        }); \
        module->SetDeserializer([](ReflectedType& component, const nlohmann::json& data) { \
            ::PiiXeL::Reflection::JsonSerializer::Deserialize(data, component); \
        });

#ifdef BUILD_WITH_EDITOR

#define EDITOR_UI() \
        module->SetEditorUI([](ReflectedType& component, entt::registry& registry, entt::entity entity, ::PiiXeL::CommandHistory& history, ::PiiXeL::ComponentModule<ReflectedType>::EntityPickerFunc entityPicker, ::PiiXeL::ComponentModule<ReflectedType>::AssetPickerFunc assetPicker)

#define EDITOR_UI_END() \
        );

#define EDITOR_CREATE_DEFAULT() \
        module->SetCreateDefault([](entt::registry& registry, entt::entity entity) -> ReflectedType

#define EDITOR_CREATE_DEFAULT_END() \
        );

#define EDITOR_DUPLICATE() \
        module->SetDuplicateFunc([](const ReflectedType& original) -> ReflectedType

#define EDITOR_DUPLICATE_END() \
        );

#define EDITOR_DISPLAY_ORDER(order) \
        module->SetDisplayOrder(order);

#define SKIP_REGISTRY_RENDER() \
        module->SetRenderInRegistry(false);

#else

#define EDITOR_UI() \
        if (false)

#define EDITOR_UI_END()

#define EDITOR_CREATE_DEFAULT() \
        if (false) return ReflectedType{}; \
        []() -> ReflectedType

#define EDITOR_CREATE_DEFAULT_END()

#define EDITOR_DUPLICATE() \
        if (false) return ReflectedType{}; \
        [](const ReflectedType&) -> ReflectedType

#define EDITOR_DUPLICATE_END()

#define EDITOR_DISPLAY_ORDER(order)

#define SKIP_REGISTRY_RENDER()

#endif

#define END_COMPONENT_MODULE(TypeName) \
        ::PiiXeL::ComponentModuleRegistry::Instance().RegisterModule(module); \
    } \
}; \
[[maybe_unused]] static TypeName##_ModuleRegistrar __module_registrar_##TypeName; \
} \
namespace Reflection { \
    void __force_link_##TypeName() {} \
}

#endif
