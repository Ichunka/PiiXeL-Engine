#ifndef PIIXELENGINE_AUDIOSOURCE_HPP
#define PIIXELENGINE_AUDIOSOURCE_HPP

#include "Components/UUID.hpp"

#include <raylib.h>

namespace PiiXeL {

enum class AudioSourceState {
    Stopped,
    Playing,
    Paused
};

struct AudioSource {
    UUID audioClip{0};

    AudioSourceState state{AudioSourceState::Stopped};

    bool playOnAwake{false};
    bool loop{false};
    bool mute{false};
    bool spatialize{true};

    float volume{1.0f};
    float pitch{1.0f};
    float spatialBlend{1.0f};

    float minDistance{1.0f};
    float maxDistance{500.0f};

    float priority{128.0f};

    float playbackPosition{0.0f};

    void* backendSourceID{nullptr};

    bool hasPlayedOnAwake{false};

    float lastAppliedVolume{-1.0f};
    float lastAppliedPitch{-1.0f};
    float lastAppliedPan{0.5f};

    AudioSource() = default;
};

} // namespace PiiXeL

#endif
