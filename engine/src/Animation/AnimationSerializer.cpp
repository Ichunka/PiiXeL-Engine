#include "Animation/AnimationSerializer.hpp"
#include <fstream>
#include <raylib.h>

namespace PiiXeL {

bool AnimationSerializer::SerializeSpriteSheet(const SpriteSheet& spriteSheet, const std::string& filepath) {
    nlohmann::json json = SpriteSheetToJson(spriteSheet);

    std::ofstream file{filepath};
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open file for writing: %s", filepath.c_str());
        return false;
    }

    file << json.dump(4);
    file.close();

    TraceLog(LOG_INFO, "SpriteSheet saved to: %s", filepath.c_str());
    return true;
}

bool AnimationSerializer::DeserializeSpriteSheet(SpriteSheet& spriteSheet, const std::string& filepath) {
    std::ifstream file{filepath};
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open file for reading: %s", filepath.c_str());
        return false;
    }

    nlohmann::json json{};
    try {
        file >> json;
    } catch (const nlohmann::json::exception& e) {
        TraceLog(LOG_ERROR, "Failed to parse JSON: %s", e.what());
        return false;
    }
    file.close();

    JsonToSpriteSheet(json, spriteSheet);
    return true;
}

bool AnimationSerializer::SerializeAnimationClip(const AnimationClip& clip, const std::string& filepath) {
    nlohmann::json json = AnimationClipToJson(clip);

    std::ofstream file{filepath};
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open file for writing: %s", filepath.c_str());
        return false;
    }

    file << json.dump(4);
    file.close();

    TraceLog(LOG_INFO, "AnimationClip saved to: %s", filepath.c_str());
    return true;
}

bool AnimationSerializer::DeserializeAnimationClip(AnimationClip& clip, const std::string& filepath) {
    std::ifstream file{filepath};
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open file for reading: %s", filepath.c_str());
        return false;
    }

    nlohmann::json json{};
    try {
        file >> json;
    } catch (const nlohmann::json::exception& e) {
        TraceLog(LOG_ERROR, "Failed to parse JSON: %s", e.what());
        return false;
    }
    file.close();

    JsonToAnimationClip(json, clip);
    return true;
}

bool AnimationSerializer::SerializeAnimatorController(const AnimatorController& controller, const std::string& filepath) {
    nlohmann::json json = AnimatorControllerToJson(controller);

    std::ofstream file{filepath};
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open file for writing: %s", filepath.c_str());
        return false;
    }

    file << json.dump(4);
    file.close();

    TraceLog(LOG_INFO, "AnimatorController saved to: %s", filepath.c_str());
    return true;
}

bool AnimationSerializer::DeserializeAnimatorController(AnimatorController& controller, const std::string& filepath) {
    std::ifstream file{filepath};
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open file for reading: %s", filepath.c_str());
        return false;
    }

    nlohmann::json json{};
    try {
        file >> json;
    } catch (const nlohmann::json::exception& e) {
        TraceLog(LOG_ERROR, "Failed to parse JSON: %s", e.what());
        return false;
    }
    file.close();

    JsonToAnimatorController(json, controller);
    return true;
}

nlohmann::json AnimationSerializer::SpriteSheetToJson(const SpriteSheet& spriteSheet) {
    nlohmann::json json{};

    json["name"] = spriteSheet.GetName();
    json["uuid"] = spriteSheet.GetUUID().Get();
    json["textureUUID"] = spriteSheet.GetTextureUUID().Get();
    json["gridColumns"] = spriteSheet.GetGridColumns();
    json["gridRows"] = spriteSheet.GetGridRows();

    json["frames"] = nlohmann::json::array();
    for (const SpriteFrame& frame : spriteSheet.GetFrames()) {
        nlohmann::json frameJson{};
        frameJson["name"] = frame.name;
        frameJson["sourceRect"] = {frame.sourceRect.x, frame.sourceRect.y, frame.sourceRect.width, frame.sourceRect.height};
        frameJson["pivot"] = {frame.pivot.x, frame.pivot.y};
        json["frames"].push_back(frameJson);
    }

    return json;
}

void AnimationSerializer::JsonToSpriteSheet(const nlohmann::json& json, SpriteSheet& spriteSheet) {
    if (json.contains("textureUUID")) {
        spriteSheet.SetTexture(UUID{json["textureUUID"].get<uint64_t>()});
    }

    if (json.contains("gridColumns") && json.contains("gridRows")) {
        spriteSheet.SetGridSize(json["gridColumns"].get<int>(), json["gridRows"].get<int>());
    }

    if (json.contains("frames") && json["frames"].is_array()) {
        std::vector<SpriteFrame> frames;
        for (const auto& frameJson : json["frames"]) {
            SpriteFrame frame{};
            frame.name = frameJson.value("name", "");

            if (frameJson.contains("sourceRect") && frameJson["sourceRect"].size() == 4) {
                frame.sourceRect.x = frameJson["sourceRect"][0].get<float>();
                frame.sourceRect.y = frameJson["sourceRect"][1].get<float>();
                frame.sourceRect.width = frameJson["sourceRect"][2].get<float>();
                frame.sourceRect.height = frameJson["sourceRect"][3].get<float>();
            }

            if (frameJson.contains("pivot") && frameJson["pivot"].size() == 2) {
                frame.pivot.x = frameJson["pivot"][0].get<float>();
                frame.pivot.y = frameJson["pivot"][1].get<float>();
            }

            frames.push_back(frame);
        }
        spriteSheet.SetFrames(frames);
    }
}

nlohmann::json AnimationSerializer::AnimationClipToJson(const AnimationClip& clip) {
    nlohmann::json json{};

    json["name"] = clip.GetName();
    json["uuid"] = clip.GetUUID().Get();
    json["spriteSheetUUID"] = clip.GetSpriteSheetUUID().Get();
    json["frameRate"] = clip.GetFrameRate();
    json["wrapMode"] = static_cast<int>(clip.GetWrapMode());

    json["frames"] = nlohmann::json::array();
    for (const AnimationFrame& frame : clip.GetFrames()) {
        nlohmann::json frameJson{};
        frameJson["frameIndex"] = frame.frameIndex;
        frameJson["duration"] = frame.duration;
        json["frames"].push_back(frameJson);
    }

    return json;
}

void AnimationSerializer::JsonToAnimationClip(const nlohmann::json& json, AnimationClip& clip) {
    if (json.contains("spriteSheetUUID")) {
        clip.SetSpriteSheet(UUID{json["spriteSheetUUID"].get<uint64_t>()});
    }

    if (json.contains("frameRate")) {
        clip.SetFrameRate(json["frameRate"].get<float>());
    }

    if (json.contains("wrapMode")) {
        clip.SetWrapMode(static_cast<AnimationWrapMode>(json["wrapMode"].get<int>()));
    }

    if (json.contains("frames") && json["frames"].is_array()) {
        std::vector<AnimationFrame> frames;
        for (const auto& frameJson : json["frames"]) {
            AnimationFrame frame{};
            frame.frameIndex = frameJson.value("frameIndex", 0);
            frame.duration = frameJson.value("duration", 0.1f);
            frames.push_back(frame);
        }
        clip.SetFrames(frames);
    }
}

nlohmann::json AnimationSerializer::AnimatorControllerToJson(const AnimatorController& controller) {
    nlohmann::json json{};

    json["name"] = controller.GetName();
    json["uuid"] = controller.GetUUID().Get();
    json["defaultState"] = controller.GetDefaultState();

    json["parameters"] = nlohmann::json::array();
    for (const AnimatorParameter& param : controller.GetParameters()) {
        nlohmann::json paramJson{};
        paramJson["name"] = param.name;
        paramJson["type"] = static_cast<int>(param.type);

        if (std::holds_alternative<float>(param.defaultValue)) {
            paramJson["defaultValue"] = std::get<float>(param.defaultValue);
        } else if (std::holds_alternative<int>(param.defaultValue)) {
            paramJson["defaultValue"] = std::get<int>(param.defaultValue);
        } else if (std::holds_alternative<bool>(param.defaultValue)) {
            paramJson["defaultValue"] = std::get<bool>(param.defaultValue);
        }

        json["parameters"].push_back(paramJson);
    }

    json["states"] = nlohmann::json::array();
    for (const AnimatorState& state : controller.GetStates()) {
        nlohmann::json stateJson{};
        stateJson["name"] = state.name;
        stateJson["animationClipUUID"] = state.animationClipUUID.Get();
        stateJson["speed"] = state.speed;
        stateJson["editorPosition"] = {state.editorPosition.x, state.editorPosition.y};
        json["states"].push_back(stateJson);
    }

    json["transitions"] = nlohmann::json::array();
    for (const AnimatorTransition& transition : controller.GetTransitions()) {
        nlohmann::json transJson{};
        transJson["fromState"] = transition.fromState;
        transJson["toState"] = transition.toState;
        transJson["exitTime"] = transition.exitTime;
        transJson["transitionDuration"] = transition.transitionDuration;
        transJson["hasExitTime"] = transition.hasExitTime;

        transJson["conditions"] = nlohmann::json::array();
        for (const TransitionCondition& condition : transition.conditions) {
            nlohmann::json condJson{};
            condJson["parameterName"] = condition.parameterName;
            condJson["type"] = static_cast<int>(condition.type);

            if (std::holds_alternative<float>(condition.value)) {
                condJson["value"] = std::get<float>(condition.value);
            } else if (std::holds_alternative<int>(condition.value)) {
                condJson["value"] = std::get<int>(condition.value);
            } else if (std::holds_alternative<bool>(condition.value)) {
                condJson["value"] = std::get<bool>(condition.value);
            }

            transJson["conditions"].push_back(condJson);
        }

        json["transitions"].push_back(transJson);
    }

    return json;
}

void AnimationSerializer::JsonToAnimatorController(const nlohmann::json& json, AnimatorController& controller) {
    if (json.contains("defaultState")) {
        controller.SetDefaultState(json["defaultState"].get<std::string>());
    }

    if (json.contains("parameters") && json["parameters"].is_array()) {
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

            controller.AddParameter(param);
        }
    }

    if (json.contains("states") && json["states"].is_array()) {
        for (const auto& stateJson : json["states"]) {
            AnimatorState state{};
            state.name = stateJson.value("name", "");
            state.animationClipUUID = UUID{stateJson.value("animationClipUUID", 0ULL)};
            state.speed = stateJson.value("speed", 1.0f);

            if (stateJson.contains("editorPosition") && stateJson["editorPosition"].size() == 2) {
                state.editorPosition.x = stateJson["editorPosition"][0].get<float>();
                state.editorPosition.y = stateJson["editorPosition"][1].get<float>();
            }

            controller.AddState(state);
        }
    }

    if (json.contains("transitions") && json["transitions"].is_array()) {
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

            controller.AddTransition(transition);
        }
    }
}

} // namespace PiiXeL
