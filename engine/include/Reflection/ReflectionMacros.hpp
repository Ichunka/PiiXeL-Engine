#ifndef PIIXELENGINE_REFLECTIONMACROS_HPP
#define PIIXELENGINE_REFLECTIONMACROS_HPP

#include "Reflection/TypeInfo.hpp"
#include "Reflection/TypeRegistry.hpp"
#include <raylib.h>
#include <entt/entt.hpp>

namespace PiiXeL {
    class EntityRef;
    class UUID;
}

namespace PiiXeL::Reflection {

template<typename T>
struct TypeDeducer {
    static FieldType GetFieldType() {
        if constexpr (std::is_same_v<T, float>) return FieldType::Float;
        else if constexpr (std::is_same_v<T, int>) return FieldType::Int;
        else if constexpr (std::is_same_v<T, bool>) return FieldType::Bool;
        else if constexpr (std::is_same_v<T, std::string>) return FieldType::String;
        else if constexpr (std::is_same_v<T, Vector2>) return FieldType::Vector2;
        else if constexpr (std::is_same_v<T, Color>) return FieldType::Color;
        else if constexpr (std::is_same_v<T, entt::entity>) return FieldType::Entity;
        else if constexpr (std::is_same_v<T, EntityRef>) return FieldType::EntityRef;
        else if constexpr (std::is_same_v<T, UUID>) return FieldType::AssetRef;
        else return FieldType::Custom;
    }
};

template<typename ClassType, typename FieldType>
inline FieldInfo MakeFieldInfo(const std::string& name, FieldType ClassType::*memberPtr, uint32_t flags, FieldMetadata metadata = {}) {
    FieldInfo info;
    info.name = name;
    info.type = TypeDeducer<FieldType>::GetFieldType();
    info.flags = flags;
    info.offset = 0;
    info.size = sizeof(FieldType);
    info.metadata = metadata;
    info.typeIndex = std::type_index(typeid(FieldType));

    info.getPtr = [memberPtr](void* obj) -> void* {
        return &(static_cast<ClassType*>(obj)->*memberPtr);
    };

    info.getConstPtr = [memberPtr](const void* obj) -> const void* {
        return &(static_cast<const ClassType*>(obj)->*memberPtr);
    };

    info.getValue = [memberPtr](const void* obj) -> std::any {
        return static_cast<const ClassType*>(obj)->*memberPtr;
    };

    info.setValue = [memberPtr](void* obj, std::any value) {
        static_cast<ClassType*>(obj)->*memberPtr = std::any_cast<FieldType>(value);
    };

    return info;
}

template<typename T>
class TypeBuilder {
public:
    explicit TypeBuilder(const std::string& typeName)
        : m_TypeInfo{typeName, std::type_index(typeid(T)), sizeof(T)}
    {}

    template<typename FieldType>
    TypeBuilder& Field(const std::string& name, FieldType T::*memberPtr, uint32_t flags = FieldFlags::Public | FieldFlags::Serializable, FieldMetadata metadata = {}) {
        m_TypeInfo.AddField(MakeFieldInfo<T, FieldType>(name, memberPtr, flags, metadata));
        return *this;
    }

    void Register() {
        TypeRegistry::Instance().RegisterType(std::move(m_TypeInfo));
    }

private:
    TypeInfo m_TypeInfo;
};

}

#define BEGIN_REFLECT(TypeName) \
namespace { \
struct TypeName##_ReflectionRegistrar { \
TypeName##_ReflectionRegistrar() { \
using ReflectedType = TypeName; \
::PiiXeL::Reflection::TypeBuilder<ReflectedType> builder{#TypeName}; \
(void)builder;

#define FIELD(FieldName) \
builder.Field(#FieldName, &ReflectedType::FieldName);

#define FIELD_FLAGS(FieldName, Flags) \
builder.Field(#FieldName, &ReflectedType::FieldName, Flags);

#define FIELD_RANGE(FieldName, Min, Max, Speed) \
builder.Field(#FieldName, &ReflectedType::FieldName, \
::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable | ::PiiXeL::Reflection::FieldFlags::Range, \
::PiiXeL::Reflection::FieldMetadata{Min, Max, Speed});

#define FIELD_ENTITY(FieldName) \
builder.Field(#FieldName, &ReflectedType::FieldName, \
::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable | ::PiiXeL::Reflection::FieldFlags::EntityPicker);

#define FIELD_ASSET(FieldName, AssetType) \
builder.Field(#FieldName, &ReflectedType::FieldName, \
::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable | ::PiiXeL::Reflection::FieldFlags::AssetPicker, \
::PiiXeL::Reflection::FieldMetadata{0.0f, 0.0f, 0.0f, AssetType});

#define END_REFLECT(TypeName) \
builder.Register(); \
} \
}; \
[[maybe_unused]] static TypeName##_ReflectionRegistrar __type_registrar_##TypeName; \
}

#endif
