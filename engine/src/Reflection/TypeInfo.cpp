#include "Reflection/TypeInfo.hpp"

namespace PiiXeL::Reflection {

TypeInfo::TypeInfo(std::string name, std::type_index typeIdx, size_t size) :
    m_Name{std::move(name)}, m_TypeIndex{typeIdx}, m_Size{size} {}

void TypeInfo::AddField(FieldInfo field) {
    m_Fields.push_back(std::move(field));
}

const FieldInfo* TypeInfo::GetField(const std::string& name) const {
    for (const FieldInfo& field : m_Fields) {
        if (field.name == name) {
            return &field;
        }
    }
    return nullptr;
}

} // namespace PiiXeL::Reflection
