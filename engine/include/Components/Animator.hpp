#pragma once

#include "Components/UUID.hpp"
#include <string>
#include <unordered_map>
#include <variant>

namespace PiiXeL {

struct Animator {
    UUID controllerUUID{0};

    std::string currentState;
    float stateTime{0.0f};
    size_t currentFrameIndex{0};
    float frameTime{0.0f};

    bool isPlaying{true};
    float playbackSpeed{1.0f};

    std::unordered_map<std::string, std::variant<float, int, bool>> parameters;
    std::unordered_map<std::string, bool> triggers;

    std::string transitionToState;
    float transitionTime{0.0f};
    float transitionDuration{0.0f};
    bool isTransitioning{false};
};

} // namespace PiiXeL
