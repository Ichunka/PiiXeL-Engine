#include "Reflection/TypeRegistry.hpp"

namespace PiiXeL::Reflection {

TypeRegistry& TypeRegistry::Instance() {
    static TypeRegistry instance;
    return instance;
}

void TypeRegistry::RegisterType(TypeInfo typeInfo) {
    std::type_index typeIndex = typeInfo.GetTypeIndex();
    m_Types[typeIndex] = std::move(typeInfo);
}

const TypeInfo* TypeRegistry::GetTypeInfo(std::type_index typeIndex) const {
    auto it = m_Types.find(typeIndex);
    if (it != m_Types.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace PiiXeL::Reflection
