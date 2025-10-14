#ifndef PIIXELENGINE_TYPEINFO_HPP
#define PIIXELENGINE_TYPEINFO_HPP

#include <any>
#include <functional>
#include <string>
#include <typeindex>
#include <vector>

namespace PiiXeL::Reflection {

enum class FieldType { Float, Int, Bool, String, Vector2, Color, Entity, EntityRef, AssetRef, Custom };

enum FieldFlags : uint32_t {
    None = 0,
    Public = 1 << 0,
    ReadOnly = 1 << 1,
    EntityPicker = 1 << 2,
    ColorPicker = 1 << 3,
    Range = 1 << 4,
    Serializable = 1 << 5,
    AssetPicker = 1 << 6
};

struct FieldMetadata {
    float rangeMin{0.0f};
    float rangeMax{100.0f};
    float dragSpeed{1.0f};
    std::string assetType{};
};

struct FieldInfo {
    std::string name;
    FieldType type;
    uint32_t flags{FieldFlags::Public | FieldFlags::Serializable};
    size_t offset{0};
    size_t size{0};
    FieldMetadata metadata;

    std::function<void*(void*)> getPtr;
    std::function<const void*(const void*)> getConstPtr;
    std::function<std::any(const void*)> getValue;
    std::function<void(void*, std::any)> setValue;

    std::type_index typeIndex{typeid(void)};
};

class TypeInfo {
public:
    TypeInfo() = default;
    TypeInfo(std::string name, std::type_index typeIdx, size_t size);

    void AddField(FieldInfo field);

    [[nodiscard]] const std::string& GetName() const { return m_Name; }
    [[nodiscard]] std::type_index GetTypeIndex() const { return m_TypeIndex; }
    [[nodiscard]] size_t GetSize() const { return m_Size; }
    [[nodiscard]] const std::vector<FieldInfo>& GetFields() const { return m_Fields; }
    [[nodiscard]] const FieldInfo* GetField(const std::string& name) const;

private:
    std::string m_Name;
    std::type_index m_TypeIndex{typeid(void)};
    size_t m_Size{0};
    std::vector<FieldInfo> m_Fields;
};

} // namespace PiiXeL::Reflection

#endif
