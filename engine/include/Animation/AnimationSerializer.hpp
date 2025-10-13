#pragma once

#include "Animation/SpriteSheet.hpp"
#include "Animation/AnimationClip.hpp"
#include "Animation/AnimatorController.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace PiiXeL {

class AnimationSerializer {
public:
    static bool SerializeSpriteSheet(const SpriteSheet& spriteSheet, const std::string& filepath);
    static bool DeserializeSpriteSheet(SpriteSheet& spriteSheet, const std::string& filepath);

    static bool SerializeAnimationClip(const AnimationClip& clip, const std::string& filepath);
    static bool DeserializeAnimationClip(AnimationClip& clip, const std::string& filepath);

    static bool SerializeAnimatorController(const AnimatorController& controller, const std::string& filepath);
    static bool DeserializeAnimatorController(AnimatorController& controller, const std::string& filepath);

    static nlohmann::json SpriteSheetToJson(const SpriteSheet& spriteSheet);
    static void JsonToSpriteSheet(const nlohmann::json& json, SpriteSheet& spriteSheet);

    static nlohmann::json AnimationClipToJson(const AnimationClip& clip);
    static void JsonToAnimationClip(const nlohmann::json& json, AnimationClip& clip);

    static nlohmann::json AnimatorControllerToJson(const AnimatorController& controller);
    static void JsonToAnimatorController(const nlohmann::json& json, AnimatorController& controller);
};

} // namespace PiiXeL
