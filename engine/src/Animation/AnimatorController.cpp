#include "Animation/AnimatorController.hpp"
#include <algorithm>
#include <nlohmann/json.hpp>
#include <raylib.h>

namespace PiiXeL {

AnimatorController::AnimatorController(UUID uuid, const std::string& name)
    : Asset(uuid, AssetType::AnimatorController, name) {
}

bool AnimatorController::Load(const void* data, size_t size) {
    if (!data || size == 0) {
        TraceLog(LOG_ERROR, "Invalid data for AnimatorController");
        return false;
    }

    try {
        std::string jsonStr{reinterpret_cast<const char*>(data), size};
        nlohmann::json json = nlohmann::json::parse(jsonStr);

        if (json.contains("defaultState")) {
            m_DefaultState = json["defaultState"].get<std::string>();
        }

        if (json.contains("parameters") && json["parameters"].is_array()) {
            m_Parameters.clear();
            for (const auto& paramJson : json["parameters"]) {
                AnimatorParameter param{};
                param.name = paramJson.value("name", "");
                param.type = static_cast<AnimatorParameterType>(paramJson.value("type", 0));

                if (paramJson.contains("defaultValue")) {
                    switch (param.type) {
                        case AnimatorParameterType::Float:
                            param.defaultValue = paramJson["defaultValue"].get<float>();
                            break;
                        case AnimatorParameterType::Int:
                            param.defaultValue = paramJson["defaultValue"].get<int>();
                            break;
                        case AnimatorParameterType::Bool:
                        case AnimatorParameterType::Trigger:
                            param.defaultValue = paramJson["defaultValue"].get<bool>();
                            break;
                    }
                }

                m_Parameters.push_back(param);
            }
        }

        if (json.contains("states") && json["states"].is_array()) {
            m_States.clear();
            for (const auto& stateJson : json["states"]) {
                AnimatorState state{};
                state.name = stateJson.value("name", "");
                state.animationClipUUID = UUID{stateJson.value("animationClipUUID", 0ULL)};
                state.speed = stateJson.value("speed", 1.0f);

                if (stateJson.contains("editorPosition") && stateJson["editorPosition"].size() == 2) {
                    state.editorPosition.x = stateJson["editorPosition"][0].get<float>();
                    state.editorPosition.y = stateJson["editorPosition"][1].get<float>();
                }

                m_States.push_back(state);
            }
        }

        if (json.contains("transitions") && json["transitions"].is_array()) {
            m_Transitions.clear();
            for (const auto& transJson : json["transitions"]) {
                AnimatorTransition transition{};
                transition.fromState = transJson.value("fromState", "");
                transition.toState = transJson.value("toState", "");
                transition.exitTime = transJson.value("exitTime", 0.0f);
                transition.transitionDuration = transJson.value("transitionDuration", 0.0f);
                transition.hasExitTime = transJson.value("hasExitTime", false);

                if (transJson.contains("conditions") && transJson["conditions"].is_array()) {
                    for (const auto& condJson : transJson["conditions"]) {
                        TransitionCondition condition{};
                        condition.parameterName = condJson.value("parameterName", "");
                        condition.type = static_cast<TransitionConditionType>(condJson.value("type", 0));

                        if (condJson.contains("value")) {
                            if (condJson["value"].is_number_float()) {
                                condition.value = condJson["value"].get<float>();
                            } else if (condJson["value"].is_number_integer()) {
                                condition.value = condJson["value"].get<int>();
                            } else if (condJson["value"].is_boolean()) {
                                condition.value = condJson["value"].get<bool>();
                            }
                        }

                        transition.conditions.push_back(condition);
                    }
                }

                m_Transitions.push_back(transition);
            }
        }

        m_IsLoaded = true;
        return true;
    } catch (const nlohmann::json::exception& e) {
        TraceLog(LOG_ERROR, "Failed to parse AnimatorController JSON: %s", e.what());
        return false;
    }
}

void AnimatorController::Unload() {
    m_Parameters.clear();
    m_States.clear();
    m_Transitions.clear();
    m_DefaultState.clear();
    m_IsLoaded = false;
}

size_t AnimatorController::GetMemoryUsage() const {
    size_t total = sizeof(AnimatorController);

    total += m_Parameters.capacity() * sizeof(AnimatorParameter);
    for (const AnimatorParameter& param : m_Parameters) {
        total += param.name.capacity();
    }

    total += m_States.capacity() * sizeof(AnimatorState);
    for (const AnimatorState& state : m_States) {
        total += state.name.capacity();
    }

    total += m_Transitions.capacity() * sizeof(AnimatorTransition);
    for (const AnimatorTransition& transition : m_Transitions) {
        total += transition.fromState.capacity();
        total += transition.toState.capacity();
        total += transition.conditions.capacity() * sizeof(TransitionCondition);
        for (const TransitionCondition& condition : transition.conditions) {
            total += condition.parameterName.capacity();
        }
    }

    total += m_DefaultState.capacity();

    return total;
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
