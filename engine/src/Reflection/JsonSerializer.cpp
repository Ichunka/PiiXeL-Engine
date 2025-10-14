#include "Reflection/JsonSerializer.hpp"

#include "Scripting/EntityRef.hpp"

namespace PiiXeL::Reflection {

nlohmann::json JsonSerializer::SerializeField(const FieldInfo& field, const void* fieldPtr) {
    switch (field.type)
    {
        case FieldType::Float:
            return *static_cast<const float*>(fieldPtr);

        case FieldType::Int:
            return *static_cast<const int*>(fieldPtr);

        case FieldType::Bool:
            return *static_cast<const bool*>(fieldPtr);

        case FieldType::String:
            return *static_cast<const std::string*>(fieldPtr);

        case FieldType::Vector2: {
            const Vector2& v = *static_cast<const Vector2*>(fieldPtr);
            return nlohmann::json{{"x", v.x}, {"y", v.y}};
        }

        case FieldType::Color: {
            const Color& c = *static_cast<const Color*>(fieldPtr);
            return nlohmann::json{{"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}};
        }

        case FieldType::Entity: {
            entt::entity entity = *static_cast<const entt::entity*>(fieldPtr);
            return static_cast<uint32_t>(entity);
        }

        case FieldType::EntityRef: {
            const EntityRef& ref = *static_cast<const EntityRef*>(fieldPtr);
            return ref.GetUUID().Get();
        }

        case FieldType::Custom:
        default:
            return nlohmann::json::object();
    }
}

void JsonSerializer::DeserializeField(const FieldInfo& field, const nlohmann::json& j, void* fieldPtr) {
    switch (field.type)
    {
        case FieldType::Float:
            *static_cast<float*>(fieldPtr) = j.get<float>();
            break;

        case FieldType::Int:
            *static_cast<int*>(fieldPtr) = j.get<int>();
            break;

        case FieldType::Bool:
            *static_cast<bool*>(fieldPtr) = j.get<bool>();
            break;

        case FieldType::String:
            *static_cast<std::string*>(fieldPtr) = j.get<std::string>();
            break;

        case FieldType::Vector2: {
            Vector2& v = *static_cast<Vector2*>(fieldPtr);
            if (j.is_array() && j.size() == 2)
            {
                v.x = j[0].get<float>();
                v.y = j[1].get<float>();
            }
            else if (j.is_object())
            {
                v.x = j.value("x", 0.0f);
                v.y = j.value("y", 0.0f);
            }
            break;
        }

        case FieldType::Color: {
            Color& c = *static_cast<Color*>(fieldPtr);
            c.r = j.value("r", static_cast<unsigned char>(255));
            c.g = j.value("g", static_cast<unsigned char>(255));
            c.b = j.value("b", static_cast<unsigned char>(255));
            c.a = j.value("a", static_cast<unsigned char>(255));
            break;
        }

        case FieldType::Entity: {
            uint32_t entityId = j.get<uint32_t>();
            *static_cast<entt::entity*>(fieldPtr) = static_cast<entt::entity>(entityId);
            break;
        }

        case FieldType::EntityRef: {
            uint64_t uuidValue = j.get<uint64_t>();
            EntityRef& ref = *static_cast<EntityRef*>(fieldPtr);
            ref.SetUUID(UUID(uuidValue));
            break;
        }

        case FieldType::Custom:
        default:
            break;
    }
}

} // namespace PiiXeL::Reflection
