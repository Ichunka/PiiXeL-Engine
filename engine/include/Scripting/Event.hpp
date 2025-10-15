#ifndef PIIXELENGINE_EVENT_HPP
#define PIIXELENGINE_EVENT_HPP

#include <entt/entt.hpp>

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace PiiXeL {

template <typename... Args>
class Event {
public:
    using Callback = std::function<void(Args...)>;
    using CallbackId = size_t;

    CallbackId Subscribe(Callback callback) {
        CallbackId id = m_NextId++;
        m_Callbacks[id] = std::move(callback);
        return id;
    }

    void Unsubscribe(CallbackId id) { m_Callbacks.erase(id); }

    void Invoke(Args... args) {
        for (auto& [id, callback] : m_Callbacks) {
            callback(args...);
        }
    }

    void Clear() { m_Callbacks.clear(); }

private:
    std::unordered_map<CallbackId, Callback> m_Callbacks;
    CallbackId m_NextId{0};
};

struct CollisionEvent {
    entt::entity self;
    entt::entity other;
};

struct TriggerEvent {
    entt::entity self;
    entt::entity other;
};

class EventDispatcher {
public:
    static EventDispatcher& Instance() {
        static EventDispatcher instance;
        return instance;
    }

    template <typename EventType>
    size_t Subscribe(std::function<void(const EventType&)> callback) {
        auto& event = GetEvent<EventType>();
        return event.Subscribe(callback);
    }

    template <typename EventType>
    void Unsubscribe(size_t id) {
        auto& event = GetEvent<EventType>();
        event.Unsubscribe(id);
    }

    template <typename EventType>
    void Dispatch(const EventType& eventData) {
        auto& event = GetEvent<EventType>();
        event.Invoke(eventData);
    }

    template <typename EventType>
    void Clear() {
        auto& event = GetEvent<EventType>();
        event.Clear();
    }

private:
    EventDispatcher() = default;

    template <typename EventType>
    Event<const EventType&>& GetEvent() {
        std::type_index typeIndex = std::type_index(typeid(EventType));

        auto it = m_Events.find(typeIndex);
        if (it == m_Events.end()) {
            auto event = std::make_unique<Event<const EventType&>>();
            Event<const EventType&>* eventPtr = event.get();
            m_Events[typeIndex] = std::move(event);
            return *eventPtr;
        }

        return *static_cast<Event<const EventType&>*>(it->second.get());
    }

    std::unordered_map<std::type_index, std::unique_ptr<void, void (*)(void*)>> m_Events;
};

} // namespace PiiXeL

#endif // PIIXELENGINE_EVENT_HPP
