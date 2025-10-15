#ifndef PIIXELENGINE_EDITORCOMMANDS_HPP
#define PIIXELENGINE_EDITORCOMMANDS_HPP

#ifdef BUILD_WITH_EDITOR

#include "Components/Sprite.hpp"
#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Scene/Scene.hpp"

#include <entt/entt.hpp>

#include <functional>

#include "Command.hpp"

namespace PiiXeL {

class ModifyTransformCommand : public Command {
public:
    ModifyTransformCommand(entt::registry* registry, entt::entity entity, const Transform& oldValue,
                           const Transform& newValue) :
        m_Registry{registry},
        m_Entity{entity}, m_OldValue{oldValue}, m_NewValue{newValue} {}

    void Execute() override {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<Transform>(m_Entity)) {
            m_Registry->get<Transform>(m_Entity) = m_NewValue;
        }
    }

    void Undo() override {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<Transform>(m_Entity)) {
            m_Registry->get<Transform>(m_Entity) = m_OldValue;
        }
    }

private:
    entt::registry* m_Registry;
    entt::entity m_Entity;
    Transform m_OldValue;
    Transform m_NewValue;
};

class ModifySpriteCommand : public Command {
public:
    ModifySpriteCommand(entt::registry* registry, entt::entity entity, const Sprite& oldValue, const Sprite& newValue) :
        m_Registry{registry}, m_Entity{entity}, m_OldValue{oldValue}, m_NewValue{newValue} {}

    void Execute() override {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<Sprite>(m_Entity)) {
            m_Registry->get<Sprite>(m_Entity) = m_NewValue;
        }
    }

    void Undo() override {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<Sprite>(m_Entity)) {
            m_Registry->get<Sprite>(m_Entity) = m_OldValue;
        }
    }

private:
    entt::registry* m_Registry;
    entt::entity m_Entity;
    Sprite m_OldValue;
    Sprite m_NewValue;
};

class CreateEntityCommand : public Command {
public:
    CreateEntityCommand(Scene* scene, const std::string& name) : m_Scene{scene}, m_Name{name}, m_Entity{entt::null} {}

    void Execute() override {
        if (m_Scene) {
            m_Entity = m_Scene->CreateEntity(m_Name);
        }
    }

    void Undo() override {
        if (m_Scene && m_Entity != entt::null) {
            m_Scene->DestroyEntity(m_Entity);
            m_Entity = entt::null;
        }
    }

    void Redo() override { Execute(); }

    [[nodiscard]] entt::entity GetEntity() const { return m_Entity; }

private:
    Scene* m_Scene;
    std::string m_Name;
    entt::entity m_Entity;
};

class DeleteEntityCommand : public Command {
public:
    DeleteEntityCommand(entt::registry* registry, entt::entity entity) : m_Registry{registry}, m_Entity{entity} {}

    void Execute() override {
        if (m_Registry && m_Registry->valid(m_Entity)) {
            m_Registry->destroy(m_Entity);
        }
    }

    void Undo() override {}

private:
    entt::registry* m_Registry;
    entt::entity m_Entity;
};

template <typename T>
class AddComponentCommand : public Command {
public:
    AddComponentCommand(entt::registry* registry, entt::entity entity, const T& component) :
        m_Registry{registry}, m_Entity{entity}, m_Component{component} {}

    void Execute() override {
        if (m_Registry && m_Registry->valid(m_Entity) && !m_Registry->all_of<T>(m_Entity)) {
            m_Registry->emplace<T>(m_Entity, m_Component);
        }
    }

    void Undo() override {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<T>(m_Entity)) {
            m_Registry->remove<T>(m_Entity);
        }
    }

private:
    entt::registry* m_Registry;
    entt::entity m_Entity;
    T m_Component;
};

template <typename T>
class RemoveComponentCommand : public Command {
public:
    RemoveComponentCommand(entt::registry* registry, entt::entity entity) : m_Registry{registry}, m_Entity{entity} {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<T>(m_Entity)) {
            m_SavedComponent = m_Registry->get<T>(m_Entity);
        }
    }

    void Execute() override {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<T>(m_Entity)) {
            m_Registry->remove<T>(m_Entity);
        }
    }

    void Undo() override {
        if (m_Registry && m_Registry->valid(m_Entity) && !m_Registry->all_of<T>(m_Entity)) {
            m_Registry->emplace<T>(m_Entity, m_SavedComponent);
        }
    }

private:
    entt::registry* m_Registry;
    entt::entity m_Entity;
    T m_SavedComponent;
};

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR

#endif // PIIXELENGINE_EDITORCOMMANDS_HPP
