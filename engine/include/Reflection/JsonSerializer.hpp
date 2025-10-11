#ifndef PIIXELENGINE_JSONSERIALIZER_HPP
#define PIIXELENGINE_JSONSERIALIZER_HPP

#include "Reflection/TypeInfo.hpp"
#include "Reflection/TypeRegistry.hpp"
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <entt/entt.hpp>

namespace PiiXeL::Reflection {

class JsonSerializer {
public:
    template<typename T>
    static nlohmann::json Serialize(const T& object) {
        const TypeInfo* typeInfo = TypeRegistry::Instance().GetTypeInfo<T>();
        if (!typeInfo) {
            return nlohmann::json::object();
        }

        nlohmann::json j;
        for (const FieldInfo& field : typeInfo->GetFields()) {
            if (!(field.flags & FieldFlags::Serializable)) {
                continue;
            }

            const void* fieldPtr = field.getConstPtr(static_cast<const void*>(&object));
            j[field.name] = SerializeField(field, fieldPtr);
        }

        return j;
    }

    template<typename T>
    static void Deserialize(const nlohmann::json& j, T& object) {
        const TypeInfo* typeInfo = TypeRegistry::Instance().GetTypeInfo<T>();
        if (!typeInfo) {
            return;
        }

        for (const FieldInfo& field : typeInfo->GetFields()) {
            if (!(field.flags & FieldFlags::Serializable)) {
                continue;
            }

            if (!j.contains(field.name)) {
                continue;
            }

            void* fieldPtr = field.getPtr(static_cast<void*>(&object));
            DeserializeField(field, j[field.name], fieldPtr);
        }
    }

    static nlohmann::json SerializeField(const FieldInfo& field, const void* fieldPtr);
    static void DeserializeField(const FieldInfo& field, const nlohmann::json& j, void* fieldPtr);
};

}

#endif
