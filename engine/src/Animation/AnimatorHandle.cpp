#include "Animation/AnimatorHandle.hpp"

#include "Components/Animator.hpp"
#include "Scene/Scene.hpp"

namespace PiiXeL {

AnimatorHandle::AnimatorHandle(Scene* scene, entt::entity entity, Animator* component) :
    m_Scene{scene}, m_Entity{entity}, m_Component{component} {}

bool AnimatorHandle::IsValid() const {
    if (!m_Scene || m_Entity == entt::null || !m_Component) {
        return false;
    }

    entt::registry& registry = m_Scene->GetRegistry();
    return registry.valid(m_Entity) && registry.all_of<Animator>(m_Entity);
}

void AnimatorHandle::Play() {
    if (IsValid()) {
        m_Component->isPlaying = true;
    }
}

void AnimatorHandle::Pause() {
    if (IsValid()) {
        m_Component->isPlaying = false;
    }
}

void AnimatorHandle::Stop() {
    if (IsValid()) {
        m_Component->isPlaying = false;
        m_Component->stateTime = 0.0f;
        m_Component->currentFrameIndex = 0;
        m_Component->frameTime = 0.0f;
    }
}

void AnimatorHandle::SetSpeed(float speed) {
    if (IsValid()) {
        m_Component->playbackSpeed = speed;
    }
}

float AnimatorHandle::GetSpeed() const {
    if (IsValid()) {
        return m_Component->playbackSpeed;
    }
    return 1.0f;
}

void AnimatorHandle::SetFloat(const std::string& name, float value) {
    if (IsValid()) {
        m_Component->parameters[name] = value;
    }
}

void AnimatorHandle::SetInt(const std::string& name, int value) {
    if (IsValid()) {
        m_Component->parameters[name] = value;
    }
}

void AnimatorHandle::SetBool(const std::string& name, bool value) {
    if (IsValid()) {
        m_Component->parameters[name] = value;
    }
}

void AnimatorHandle::SetTrigger(const std::string& name) {
    if (IsValid()) {
        m_Component->triggers[name] = true;
    }
}

float AnimatorHandle::GetFloat(const std::string& name) const {
    if (IsValid()) {
        auto it = m_Component->parameters.find(name);
        if (it != m_Component->parameters.end() && std::holds_alternative<float>(it->second)) {
            return std::get<float>(it->second);
        }
    }
    return 0.0f;
}

int AnimatorHandle::GetInt(const std::string& name) const {
    if (IsValid()) {
        auto it = m_Component->parameters.find(name);
        if (it != m_Component->parameters.end() && std::holds_alternative<int>(it->second)) {
            return std::get<int>(it->second);
        }
    }
    return 0;
}

bool AnimatorHandle::GetBool(const std::string& name) const {
    if (IsValid()) {
        auto it = m_Component->parameters.find(name);
        if (it != m_Component->parameters.end() && std::holds_alternative<bool>(it->second)) {
            return std::get<bool>(it->second);
        }
    }
    return false;
}

std::string AnimatorHandle::GetCurrentState() const {
    if (IsValid()) {
        return m_Component->currentState;
    }
    return "";
}

bool AnimatorHandle::IsPlaying() const {
    if (IsValid()) {
        return m_Component->isPlaying;
    }
    return false;
}

float AnimatorHandle::GetStateTime() const {
    if (IsValid()) {
        return m_Component->stateTime;
    }
    return 0.0f;
}

} // namespace PiiXeL
