#pragma once

#include "Resources/Asset.hpp"
#include "Components/UUID.hpp"
#include <raylib.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>

namespace PiiXeL {

enum class AnimatorParameterType {
    Float,
    Int,
    Bool,
    Trigger
};

struct AnimatorParameter {
    std::string name;
    AnimatorParameterType type{AnimatorParameterType::Float};
    std::variant<float, int, bool> defaultValue{0.0f};
};

enum class TransitionConditionType {
    Greater,
    Less,
    Equals,
    NotEquals
};

struct TransitionCondition {
    std::string parameterName;
    TransitionConditionType type{TransitionConditionType::Equals};
    std::variant<float, int, bool> value{0.0f};
};

struct AnimatorTransition {
    std::string fromState;
    std::string toState;
    std::vector<TransitionCondition> conditions;
    float exitTime{0.0f};
    float transitionDuration{0.0f};
    bool hasExitTime{false};
};

struct AnimatorState {
    std::string name;
    UUID animationClipUUID{0};
    float speed{1.0f};
    Vector2 editorPosition{0.0f, 0.0f};
};

class AnimatorController : public Asset {
public:
    AnimatorController(UUID uuid, const std::string& name);
    ~AnimatorController() override = default;

    bool Load(const void* data, size_t size) override;
    void Unload() override;
    size_t GetMemoryUsage() const override;

    void AddParameter(const AnimatorParameter& parameter);
    void RemoveParameter(const std::string& name);
    const std::vector<AnimatorParameter>& GetParameters() const { return m_Parameters; }

    void AddState(const AnimatorState& state);
    void RemoveState(const std::string& name);
    const std::vector<AnimatorState>& GetStates() const { return m_States; }
    const AnimatorState* GetState(const std::string& name) const;

    void AddTransition(const AnimatorTransition& transition);
    void RemoveTransition(const std::string& fromState, const std::string& toState);
    const std::vector<AnimatorTransition>& GetTransitions() const { return m_Transitions; }
    std::vector<AnimatorTransition> GetTransitionsFromState(const std::string& stateName) const;

    void SetDefaultState(const std::string& stateName) { m_DefaultState = stateName; }
    const std::string& GetDefaultState() const { return m_DefaultState; }

private:
    std::vector<AnimatorParameter> m_Parameters;
    std::vector<AnimatorState> m_States;
    std::vector<AnimatorTransition> m_Transitions;
    std::string m_DefaultState;
};

} // namespace PiiXeL
