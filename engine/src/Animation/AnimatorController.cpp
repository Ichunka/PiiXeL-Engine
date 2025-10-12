#include "Animation/AnimatorController.hpp"
#include <algorithm>

namespace PiiXeL {

AnimatorController::AnimatorController(UUID uuid, const std::string& name)
    : Asset(uuid, AssetType::AnimatorController, name) {
}

bool AnimatorController::Load(const void* data, size_t size) {
    (void)data;
    (void)size;
    m_IsLoaded = true;
    return true;
}

void AnimatorController::Unload() {
    m_Parameters.clear();
    m_States.clear();
    m_Transitions.clear();
    m_DefaultState.clear();
    m_IsLoaded = false;
}

size_t AnimatorController::GetMemoryUsage() const {
    return m_Parameters.size() * sizeof(AnimatorParameter) +
           m_States.size() * sizeof(AnimatorState) +
           m_Transitions.size() * sizeof(AnimatorTransition);
}

void AnimatorController::AddParameter(const AnimatorParameter& parameter) {
    m_Parameters.push_back(parameter);
}

void AnimatorController::RemoveParameter(const std::string& name) {
    m_Parameters.erase(
        std::remove_if(m_Parameters.begin(), m_Parameters.end(),
            [&name](const AnimatorParameter& param) { return param.name == name; }),
        m_Parameters.end()
    );
}

void AnimatorController::AddState(const AnimatorState& state) {
    m_States.push_back(state);
    if (m_DefaultState.empty()) {
        m_DefaultState = state.name;
    }
}

void AnimatorController::RemoveState(const std::string& name) {
    m_States.erase(
        std::remove_if(m_States.begin(), m_States.end(),
            [&name](const AnimatorState& state) { return state.name == name; }),
        m_States.end()
    );

    if (m_DefaultState == name && !m_States.empty()) {
        m_DefaultState = m_States[0].name;
    }
}

const AnimatorState* AnimatorController::GetState(const std::string& name) const {
    for (const AnimatorState& state : m_States) {
        if (state.name == name) {
            return &state;
        }
    }
    return nullptr;
}

void AnimatorController::AddTransition(const AnimatorTransition& transition) {
    m_Transitions.push_back(transition);
}

void AnimatorController::RemoveTransition(const std::string& fromState, const std::string& toState) {
    m_Transitions.erase(
        std::remove_if(m_Transitions.begin(), m_Transitions.end(),
            [&](const AnimatorTransition& trans) {
                return trans.fromState == fromState && trans.toState == toState;
            }),
        m_Transitions.end()
    );
}

std::vector<AnimatorTransition> AnimatorController::GetTransitionsFromState(const std::string& stateName) const {
    std::vector<AnimatorTransition> result;
    for (const AnimatorTransition& transition : m_Transitions) {
        if (transition.fromState == stateName) {
            result.push_back(transition);
        }
    }
    return result;
}

} // namespace PiiXeL
