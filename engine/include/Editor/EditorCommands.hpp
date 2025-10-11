#ifndef PIIXELENGINE_EDITORCOMMANDS_HPP
#define PIIXELENGINE_EDITORCOMMANDS_HPP

#ifdef BUILD_WITH_EDITOR

#include "Command.hpp"
#include "Components/Transform.hpp"
#include "Components/Sprite.hpp"
#include "Components/Camera.hpp"
#include "Components/RigidBody2D.hpp"
#include "Components/BoxCollider2D.hpp"
#include "Components/Tag.hpp"
#include <entt/entt.hpp>
#include <functional>

namespace PiiXeL {

class ModifyTransformCommand : public Command {
public:
    ModifyTransformCommand(entt::registry* registry, entt::entity entity, const Transform& oldValue, const Transform& newValue)
        : m_Registry{registry}
        , m_Entity{entity}
        , m_OldValue{oldValue}
        , m_NewValue{newValue}
    {}

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
    ModifySpriteCommand(entt::registry* registry, entt::entity entity, const Sprite& oldValue, const Sprite& newValue)
        : m_Registry{registry}
        , m_Entity{entity}
        , m_OldValue{oldValue}
        , m_NewValue{newValue}
    {}

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

class ModifyCameraCommand : public Command {
public:
    ModifyCameraCommand(entt::registry* registry, entt::entity entity, const Camera& oldValue, const Camera& newValue)
        : m_Registry{registry}
        , m_Entity{entity}
        , m_OldValue{oldValue}
        , m_NewValue{newValue}
    {}

    void Execute() override {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<Camera>(m_Entity)) {
            m_Registry->get<Camera>(m_Entity) = m_NewValue;
        }
    }

    void Undo() override {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<Camera>(m_Entity)) {
            m_Registry->get<Camera>(m_Entity) = m_OldValue;
        }
    }

private:
    entt::registry* m_Registry;
    entt::entity m_Entity;
    Camera m_OldValue;
    Camera m_NewValue;
};

class ModifyRigidBody2DCommand : public Command {
public:
    ModifyRigidBody2DCommand(entt::registry* registry, entt::entity entity, const RigidBody2D& oldValue, const RigidBody2D& newValue)
        : m_Registry{registry}
        , m_Entity{entity}
        , m_OldValue{oldValue}
        , m_NewValue{newValue}
    {}

    void Execute() override {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<RigidBody2D>(m_Entity)) {
            RigidBody2D& rb = m_Registry->get<RigidBody2D>(m_Entity);
            b2BodyId oldBodyId = rb.box2dBodyId;
            rb = m_NewValue;
            rb.box2dBodyId = oldBodyId;
        }
    }

    void Undo() override {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<RigidBody2D>(m_Entity)) {
            RigidBody2D& rb = m_Registry->get<RigidBody2D>(m_Entity);
            b2BodyId oldBodyId = rb.box2dBodyId;
            rb = m_OldValue;
            rb.box2dBodyId = oldBodyId;
        }
    }

private:
    entt::registry* m_Registry;
    entt::entity m_Entity;
    RigidBody2D m_OldValue;
    RigidBody2D m_NewValue;
};

class ModifyBoxCollider2DCommand : public Command {
public:
    ModifyBoxCollider2DCommand(entt::registry* registry, entt::entity entity, const BoxCollider2D& oldValue, const BoxCollider2D& newValue)
        : m_Registry{registry}
        , m_Entity{entity}
        , m_OldValue{oldValue}
        , m_NewValue{newValue}
    {}

    void Execute() override {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<BoxCollider2D>(m_Entity)) {
            m_Registry->get<BoxCollider2D>(m_Entity) = m_NewValue;
        }
    }

    void Undo() override {
        if (m_Registry && m_Registry->valid(m_Entity) && m_Registry->all_of<BoxCollider2D>(m_Entity)) {
            m_Registry->get<BoxCollider2D>(m_Entity) = m_OldValue;
        }
    }

private:
    entt::registry* m_Registry;
    entt::entity m_Entity;
    BoxCollider2D m_OldValue;
    BoxCollider2D m_NewValue;
};

class CreateEntityCommand : public Command {
public:
    CreateEntityCommand(entt::registry* registry, const std::string& name)
        : m_Registry{registry}
        , m_Name{name}
        , m_Entity{entt::null}
    {}

    void Execute() override {
        if (m_Registry) {
            m_Entity = m_Registry->create();
            m_Registry->emplace<Tag>(m_Entity, m_Name);
            m_Registry->emplace<Transform>(m_Entity);
        }
    }

    void Undo() override {
        if (m_Registry && m_Registry->valid(m_Entity)) {
            m_Registry->destroy(m_Entity);
            m_Entity = entt::null;
        }
    }

    void Redo() override {
        Execute();
    }

    [[nodiscard]] entt::entity GetEntity() const { return m_Entity; }

private:
    entt::registry* m_Registry;
    std::string m_Name;
    entt::entity m_Entity;
};

class DeleteEntityCommand : public Command {
public:
    DeleteEntityCommand(entt::registry* registry, entt::entity entity)
        : m_Registry{registry}
        , m_Entity{entity}
    {
        if (m_Registry && m_Registry->valid(m_Entity)) {
            if (m_Registry->all_of<Tag>(m_Entity)) {
                m_SavedTag = m_Registry->get<Tag>(m_Entity);
            }
            if (m_Registry->all_of<Transform>(m_Entity)) {
                m_SavedTransform = m_Registry->get<Transform>(m_Entity);
                m_HasTransform = true;
            }
            if (m_Registry->all_of<Sprite>(m_Entity)) {
                m_SavedSprite = m_Registry->get<Sprite>(m_Entity);
                m_HasSprite = true;
            }
            if (m_Registry->all_of<Camera>(m_Entity)) {
                m_SavedCamera = m_Registry->get<Camera>(m_Entity);
                m_HasCamera = true;
            }
            if (m_Registry->all_of<RigidBody2D>(m_Entity)) {
                m_SavedRigidBody = m_Registry->get<RigidBody2D>(m_Entity);
                m_HasRigidBody = true;
            }
            if (m_Registry->all_of<BoxCollider2D>(m_Entity)) {
                m_SavedBoxCollider = m_Registry->get<BoxCollider2D>(m_Entity);
                m_HasBoxCollider = true;
            }
        }
    }

    void Execute() override {
        if (m_Registry && m_Registry->valid(m_Entity)) {
            m_Registry->destroy(m_Entity);
        }
    }

    void Undo() override {
        if (m_Registry) {
            entt::entity newEntity = m_Registry->create(m_Entity);
            m_Entity = newEntity;

            m_Registry->emplace<Tag>(m_Entity, m_SavedTag);
            if (m_HasTransform) {
                m_Registry->emplace<Transform>(m_Entity, m_SavedTransform);
            }
            if (m_HasSprite) {
                m_Registry->emplace<Sprite>(m_Entity, m_SavedSprite);
            }
            if (m_HasCamera) {
                m_Registry->emplace<Camera>(m_Entity, m_SavedCamera);
            }
            if (m_HasRigidBody) {
                m_Registry->emplace<RigidBody2D>(m_Entity, m_SavedRigidBody);
            }
            if (m_HasBoxCollider) {
                m_Registry->emplace<BoxCollider2D>(m_Entity, m_SavedBoxCollider);
            }
        }
    }

private:
    entt::registry* m_Registry;
    entt::entity m_Entity;
    Tag m_SavedTag;
    Transform m_SavedTransform;
    Sprite m_SavedSprite;
    Camera m_SavedCamera;
    RigidBody2D m_SavedRigidBody;
    BoxCollider2D m_SavedBoxCollider;
    bool m_HasTransform{false};
    bool m_HasSprite{false};
    bool m_HasCamera{false};
    bool m_HasRigidBody{false};
    bool m_HasBoxCollider{false};
};

template<typename T>
class AddComponentCommand : public Command {
public:
    AddComponentCommand(entt::registry* registry, entt::entity entity, const T& component)
        : m_Registry{registry}
        , m_Entity{entity}
        , m_Component{component}
    {}

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

template<typename T>
class RemoveComponentCommand : public Command {
public:
    RemoveComponentCommand(entt::registry* registry, entt::entity entity)
        : m_Registry{registry}
        , m_Entity{entity}
    {
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
