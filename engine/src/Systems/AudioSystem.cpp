#include "Systems/AudioSystem.hpp"

#include "Components/AudioListener.hpp"
#include "Components/AudioSource.hpp"
#include "Components/Transform.hpp"
#include "Core/Logger.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/AudioAsset.hpp"
#include "Scene/Scene.hpp"

#include <raymath.h>

#include <cmath>

namespace PiiXeL {

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem() {
    Shutdown();
}

void AudioSystem::Initialize() {
    if (m_IsInitialized) {
        return;
    }

    InitAudioDevice();

    if (!IsAudioDeviceReady()) {
        PX_LOG_ERROR(ENGINE, "Failed to initialize audio device");
        return;
    }

    SetMasterVolume(m_MasterVolume);
    m_IsInitialized = true;
    PX_LOG_INFO(ENGINE, "Audio system initialized");
}

void AudioSystem::Shutdown() {
    if (!m_IsInitialized) {
        return;
    }

    CloseAudioDevice();
    m_IsInitialized = false;
    PX_LOG_INFO(ENGINE, "Audio system shutdown");
}

void AudioSystem::Update(float deltaTime, entt::registry& registry) {
    (void)deltaTime;

    if (!m_IsInitialized) {
        return;
    }

    UpdateListener(registry);
    UpdateSources(registry);
    UpdateSpatialAudio(registry);
}

void AudioSystem::UpdateListener(entt::registry& registry) {
    auto view = registry.view<AudioListener>();

    m_ListenerEntity = entt::null;

    for (auto entity : view) {
        AudioListener& listener = view.get<AudioListener>(entity);
        if (listener.isActive) {
            m_ListenerEntity = entity;
            break;
        }
    }
}

void AudioSystem::UpdateSources(entt::registry& registry) {
    auto view = registry.view<AudioSource>();

    for (auto entity : view) {
        AudioSource& source = view.get<AudioSource>(entity);

        if (source.audioClip.Get() == 0) {
            continue;
        }

        std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAsset(source.audioClip);
        if (!asset) {
            continue;
        }

        AudioAsset* audioAsset = dynamic_cast<AudioAsset*>(asset.get());
        if (!audioAsset || !audioAsset->IsLoaded()) {
            continue;
        }

        Sound sound = audioAsset->GetSound();
        if (sound.frameCount == 0) {
            continue;
        }

        if (source.playOnAwake && !source.hasPlayedOnAwake) {
            float targetVolume = source.mute ? 0.0f : (source.volume * m_MasterVolume);
            SetSoundVolume(sound, targetVolume);
            SetSoundPitch(sound, source.pitch);
            source.lastAppliedVolume = targetVolume;
            source.lastAppliedPitch = source.pitch;
            PlaySound(sound);
            source.state = AudioSourceState::Playing;
            source.hasPlayedOnAwake = true;
            PX_LOG_INFO(ENGINE, "Playing sound on awake");
        }

        if (source.state == AudioSourceState::Playing) {
            if (!IsSoundPlaying(sound)) {
                if (source.loop) {
                    float targetVolume = source.mute ? 0.0f : (source.volume * m_MasterVolume);
                    SetSoundVolume(sound, targetVolume);
                    SetSoundPitch(sound, source.pitch);
                    source.lastAppliedVolume = targetVolume;
                    source.lastAppliedPitch = source.pitch;
                    PlaySound(sound);
                }
                else {
                    source.state = AudioSourceState::Stopped;
                }
            }
        }
    }
}

void AudioSystem::UpdateSpatialAudio(entt::registry& registry) {
    if (m_ListenerEntity == entt::null) {
        return;
    }

    Transform* listenerTransform = registry.try_get<Transform>(m_ListenerEntity);
    if (!listenerTransform) {
        return;
    }

    Vector2 listenerPos = listenerTransform->position;

    auto view = registry.view<AudioSource, Transform>();

    for (auto entity : view) {
        auto [source, transform] = view.get<AudioSource, Transform>(entity);

        if (!source.spatialize || source.spatialBlend < 0.01f) {
            continue;
        }

        if (source.audioClip.Get() == 0 || source.state != AudioSourceState::Playing) {
            continue;
        }

        std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAsset(source.audioClip);
        if (!asset) {
            continue;
        }

        AudioAsset* audioAsset = dynamic_cast<AudioAsset*>(asset.get());
        if (!audioAsset || !audioAsset->IsLoaded()) {
            continue;
        }

        Sound sound = audioAsset->GetSound();

        Vector2 sourcePos = transform.position;
        float distance = Vector2Distance(sourcePos, listenerPos);

        float volumeAttenuation = 1.0f;
        if (distance > source.minDistance) {
            if (distance >= source.maxDistance) {
                volumeAttenuation = 0.0f;
            }
            else {
                volumeAttenuation = 1.0f - ((distance - source.minDistance) / (source.maxDistance - source.minDistance));
            }
        }

        volumeAttenuation = volumeAttenuation * source.spatialBlend + (1.0f - source.spatialBlend);

        float finalVolume = source.volume * volumeAttenuation * m_MasterVolume;
        if (source.mute) {
            finalVolume = 0.0f;
        }

        float pan = 0.0f;
        if (source.maxDistance > 0.0f) {
            float deltaX = listenerPos.x - sourcePos.x;
            pan = deltaX / source.maxDistance;
            pan = Clamp(pan, -1.0f, 1.0f);
        }
        float targetPan = 0.5f + pan * 0.5f;

        constexpr float volumeEpsilon = 0.01f;
        constexpr float panEpsilon = 0.02f;

        if (fabs(finalVolume - source.lastAppliedVolume) > volumeEpsilon) {
            SetSoundVolume(sound, finalVolume);
            source.lastAppliedVolume = finalVolume;
        }

        if (fabs(targetPan - source.lastAppliedPan) > panEpsilon) {
            SetSoundPan(sound, targetPan);
            source.lastAppliedPan = targetPan;
        }
    }
}

void AudioSystem::PlaySource(entt::registry& registry, entt::entity entity) {
    AudioSource* source = registry.try_get<AudioSource>(entity);
    if (!source || source->audioClip.Get() == 0) {
        return;
    }

    std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAsset(source->audioClip);
    if (!asset) {
        return;
    }

    AudioAsset* audioAsset = dynamic_cast<AudioAsset*>(asset.get());
    if (!audioAsset || !audioAsset->IsLoaded()) {
        return;
    }

    Sound sound = audioAsset->GetSound();
    PlaySound(sound);
    source->state = AudioSourceState::Playing;
    PX_LOG_INFO(ENGINE, "Playing audio source");
}

void AudioSystem::PauseSource(entt::registry& registry, entt::entity entity) {
    AudioSource* source = registry.try_get<AudioSource>(entity);
    if (!source || source->audioClip.Get() == 0) {
        return;
    }

    std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAsset(source->audioClip);
    if (!asset) {
        return;
    }

    AudioAsset* audioAsset = dynamic_cast<AudioAsset*>(asset.get());
    if (!audioAsset || !audioAsset->IsLoaded()) {
        return;
    }

    Sound sound = audioAsset->GetSound();
    PauseSound(sound);
    source->state = AudioSourceState::Paused;
}

void AudioSystem::StopSource(entt::registry& registry, entt::entity entity) {
    AudioSource* source = registry.try_get<AudioSource>(entity);
    if (!source || source->audioClip.Get() == 0) {
        return;
    }

    std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAsset(source->audioClip);
    if (!asset) {
        return;
    }

    AudioAsset* audioAsset = dynamic_cast<AudioAsset*>(asset.get());
    if (!audioAsset || !audioAsset->IsLoaded()) {
        return;
    }

    Sound sound = audioAsset->GetSound();
    StopSound(sound);
    source->state = AudioSourceState::Stopped;
}

void AudioSystem::StopAllSources(entt::registry& registry) {
    auto view = registry.view<AudioSource>();
    for (auto entity : view) {
        StopSource(registry, entity);
    }
}

void AudioSystem::SetMasterVolume(float volume) {
    m_MasterVolume = Clamp(volume, 0.0f, 1.0f);
    if (m_IsInitialized) {
        ::SetMasterVolume(m_MasterVolume);
    }
}

void AudioSystem::ResetAudioSources(entt::registry& registry) {
    auto view = registry.view<AudioSource>();
    for (auto entity : view) {
        AudioSource& source = view.get<AudioSource>(entity);
        source.state = AudioSourceState::Stopped;
        source.hasPlayedOnAwake = false;
        source.lastAppliedVolume = -1.0f;
        source.lastAppliedPitch = -1.0f;
        source.lastAppliedPan = 0.5f;
    }
}

} // namespace PiiXeL
