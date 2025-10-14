#ifndef PIIXELENGINE_COMPONENTMODULE_HPP
#define PIIXELENGINE_COMPONENTMODULE_HPP

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <string>
#include <typeindex>

namespace PiiXeL {

#ifdef BUILD_WITH_EDITOR
class CommandHistory;
#endif

class IComponentModule {
public:
    virtual ~IComponentModule() = default;

    virtual const char* GetName() const = 0;
    virtual std::type_index GetTypeIndex() const = 0;

    virtual nlohmann::json Serialize(entt::registry& registry, entt::entity entity) const = 0;
    virtual void Deserialize(entt::registry& registry, entt::entity entity, const nlohmann::json& data) = 0;

    virtual bool HasComponent(entt::registry& registry, entt::entity entity) const = 0;
    virtual void RemoveComponent(entt::registry& registry, entt::entity entity) = 0;

#ifdef BUILD_WITH_EDITOR
    using EntityPickerFunc = std::function<bool(const char*, entt::entity*)>;
    using AssetPickerFunc = std::function<bool(const char*, class UUID*, const std::string&)>;

    virtual void RenderInspectorUI(entt::registry& registry, entt::entity entity, CommandHistory& history,
                                   EntityPickerFunc entityPicker, AssetPickerFunc assetPicker) = 0;
    virtual void AddComponentToEntity(entt::registry& registry, entt::entity entity, CommandHistory& history) = 0;
    virtual void DuplicateComponent(entt::registry& registry, entt::entity srcEntity, entt::entity dstEntity) = 0;
    virtual int GetDisplayOrder() const = 0;
    virtual bool IsRenderedByRegistry() const = 0;
#endif
};

template <typename T>
class ComponentModule : public IComponentModule {
public:
    using SerializeFunc = std::function<nlohmann::json(const T&)>;
    using DeserializeFunc = std::function<void(T&, const nlohmann::json&)>;

#ifdef BUILD_WITH_EDITOR
    using EntityPickerFunc = std::function<bool(const char*, entt::entity*)>;
    using AssetPickerFunc = std::function<bool(const char*, class UUID*, const std::string&)>;
    using EditorUIFunc =
        std::function<void(T&, entt::registry&, entt::entity, CommandHistory&, EntityPickerFunc, AssetPickerFunc)>;
    using CreateDefaultFunc = std::function<T(entt::registry&, entt::entity)>;
    using DuplicateFunc = std::function<T(const T&)>;
#endif

    explicit ComponentModule(const char* name) :
        m_Name{name}, m_TypeIndex{std::type_index(typeid(T))}
#ifdef BUILD_WITH_EDITOR
        ,
        m_DisplayOrder{100}, m_RenderInRegistry{true}
#endif
    {
    }

    const char* GetName() const override { return m_Name; }

    std::type_index GetTypeIndex() const override { return m_TypeIndex; }

    void SetSerializer(SerializeFunc func) { m_Serializer = func; }

    void SetDeserializer(DeserializeFunc func) { m_Deserializer = func; }

#ifdef BUILD_WITH_EDITOR
    void SetEditorUI(EditorUIFunc func) { m_EditorUI = func; }

    void SetCreateDefault(CreateDefaultFunc func) { m_CreateDefault = func; }

    void SetDuplicateFunc(DuplicateFunc func) { m_DuplicateFunc = func; }

    void SetDisplayOrder(int order) { m_DisplayOrder = order; }

    int GetDisplayOrder() const override { return m_DisplayOrder; }

    void SetRenderInRegistry(bool render) { m_RenderInRegistry = render; }

    bool IsRenderedByRegistry() const override { return m_RenderInRegistry; }
#endif

    nlohmann::json Serialize(entt::registry& registry, entt::entity entity) const override {
        if (!registry.all_of<T>(entity)) {
            return nlohmann::json{};
        }

        const T& component = registry.get<T>(entity);
        if (m_Serializer) {
            return m_Serializer(component);
        }
        return nlohmann::json{};
    }

    void Deserialize(entt::registry& registry, entt::entity entity, const nlohmann::json& data) override {
        T component{};
        if (m_Deserializer) {
            m_Deserializer(component, data);
        }
        registry.emplace<T>(entity, component);
    }

    bool HasComponent(entt::registry& registry, entt::entity entity) const override {
        return registry.all_of<T>(entity);
    }

    void RemoveComponent(entt::registry& registry, entt::entity entity) override {
        if (registry.all_of<T>(entity)) {
            registry.remove<T>(entity);
        }
    }

#ifdef BUILD_WITH_EDITOR
    void RenderInspectorUI(entt::registry& registry, entt::entity entity, CommandHistory& history,
                           EntityPickerFunc entityPicker, AssetPickerFunc assetPicker) override {
        if (!registry.all_of<T>(entity)) {
            return;
        }

        T& component = registry.get<T>(entity);
        if (m_EditorUI) {
            m_EditorUI(component, registry, entity, history, entityPicker, assetPicker);
        }
    }

    void AddComponentToEntity(entt::registry& registry, entt::entity entity,
                              [[maybe_unused]] CommandHistory& history) override {
        if (registry.all_of<T>(entity)) {
            return;
        }

        T component{};
        if (m_CreateDefault) {
            component = m_CreateDefault(registry, entity);
        }

        registry.emplace<T>(entity, component);
    }

    void DuplicateComponent(entt::registry& registry, entt::entity srcEntity, entt::entity dstEntity) override {
        if (!registry.all_of<T>(srcEntity)) {
            return;
        }

        const T& original = registry.get<T>(srcEntity);
        T copy;

        if (m_DuplicateFunc) {
            copy = m_DuplicateFunc(original);
        }
        else {
            copy = original;
        }

        registry.emplace<T>(dstEntity, copy);
    }
#endif

private:
    const char* m_Name;
    std::type_index m_TypeIndex;

    SerializeFunc m_Serializer;
    DeserializeFunc m_Deserializer;

#ifdef BUILD_WITH_EDITOR
    EditorUIFunc m_EditorUI;
    CreateDefaultFunc m_CreateDefault;
    DuplicateFunc m_DuplicateFunc;
    int m_DisplayOrder;
    bool m_RenderInRegistry;
#endif
};

} // namespace PiiXeL

#endif
