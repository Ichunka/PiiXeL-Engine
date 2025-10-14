#ifndef PIIXELENGINE_TYPEREGISTRY_HPP
#define PIIXELENGINE_TYPEREGISTRY_HPP

#include "Reflection/TypeInfo.hpp"

#include <memory>
#include <unordered_map>

namespace PiiXeL::Reflection {

class TypeRegistry {
public:
    static TypeRegistry& Instance();

    void RegisterType(TypeInfo typeInfo);

    [[nodiscard]] const TypeInfo* GetTypeInfo(std::type_index typeIndex) const;

    template <typename T>
    [[nodiscard]] const TypeInfo* GetTypeInfo() const {
        return GetTypeInfo(std::type_index(typeid(T)));
    }

    [[nodiscard]] const std::unordered_map<std::type_index, TypeInfo>& GetAllTypes() const { return m_Types; }

    [[nodiscard]] size_t GetTypeCount() const { return m_Types.size(); }

private:
    TypeRegistry() = default;
    std::unordered_map<std::type_index, TypeInfo> m_Types;
};

} // namespace PiiXeL::Reflection

#endif
