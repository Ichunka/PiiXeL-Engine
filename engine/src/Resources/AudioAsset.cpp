#include "Resources/AudioAsset.hpp"

#include "Core/Logger.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace PiiXeL {

AudioAsset::AudioAsset(UUID uuid, const std::string& name) : Asset{uuid, AssetType::Audio, name} {
    m_Format = DetectAudioFormat(m_Metadata.sourceExtension);
}

AudioAsset::~AudioAsset() {
    Unload();
}

bool AudioAsset::Load(const void* data, size_t size) {
    if (m_IsLoaded) {
        Unload();
    }

    if (m_Format == AudioFormat::Unknown) {
        m_Format = DetectAudioFormat(m_Metadata.sourceExtension);
    }

    const char* fileExt = ".wav";
    switch (m_Format) {
    case AudioFormat::WAV:
        fileExt = ".wav";
        break;
    case AudioFormat::OGG:
        fileExt = ".ogg";
        break;
    case AudioFormat::MP3:
        fileExt = ".mp3";
        break;
    case AudioFormat::FLAC:
        fileExt = ".flac";
        break;
    default:
        PX_LOG_ERROR(ASSET, "Unsupported audio format for asset: %s", m_Metadata.name.c_str());
        return false;
    }

    m_Wave = LoadWaveFromMemory(fileExt, static_cast<const unsigned char*>(data), static_cast<int>(size));

    if (m_Wave.data == nullptr) {
        PX_LOG_ERROR(ASSET, "Failed to load audio from memory: %s", m_Metadata.name.c_str());
        return false;
    }

    m_Sound = LoadSoundFromWave(m_Wave);

    if (m_Sound.frameCount == 0) {
        PX_LOG_ERROR(ASSET, "Failed to create sound from wave: %s", m_Metadata.name.c_str());
        UnloadWave(m_Wave);
        return false;
    }

    m_IsLoaded = true;
    PX_LOG_INFO(ASSET, "Audio asset loaded: %s (%u frames, %.2fs, %uHz, %u channels)", m_Metadata.name.c_str(),
                m_Sound.frameCount, GetLength(), GetSampleRate(), GetChannels());
    return true;
}

void AudioAsset::Unload() {
    if (m_IsLoaded) {
        if (m_Sound.frameCount > 0) {
            UnloadSound(m_Sound);
            m_Sound = Sound{};
        }
        if (m_Wave.data != nullptr) {
            UnloadWave(m_Wave);
            m_Wave = Wave{};
        }
        m_IsLoaded = false;
    }
}

size_t AudioAsset::GetMemoryUsage() const {
    if (!m_IsLoaded)
        return 0;
    return m_Wave.frameCount * m_Wave.channels * (m_Wave.sampleSize / 8);
}

float AudioAsset::GetLength() const {
    if (!m_IsLoaded || m_Wave.sampleRate == 0)
        return 0.0f;
    return static_cast<float>(m_Wave.frameCount) / static_cast<float>(m_Wave.sampleRate);
}

uint32_t AudioAsset::GetSampleRate() const {
    if (!m_IsLoaded)
        return 0;
    return m_Wave.sampleRate;
}

uint32_t AudioAsset::GetChannels() const {
    if (!m_IsLoaded)
        return 0;
    return m_Wave.channels;
}

AudioFormat AudioAsset::DetectAudioFormat(const std::string& extension) {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (ext == ".wav")
        return AudioFormat::WAV;
    if (ext == ".ogg")
        return AudioFormat::OGG;
    if (ext == ".mp3")
        return AudioFormat::MP3;
    if (ext == ".flac")
        return AudioFormat::FLAC;

    return AudioFormat::Unknown;
}

std::vector<uint8_t> AudioAsset::EncodeToMemory(const std::string& sourcePath) {
    std::ifstream file{sourcePath, std::ios::binary | std::ios::ate};
    if (!file.is_open()) {
        PX_LOG_ERROR(ASSET, "Failed to open audio file: %s", sourcePath.c_str());
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        PX_LOG_ERROR(ASSET, "Failed to read audio file: %s", sourcePath.c_str());
        return {};
    }

    return buffer;
}

} // namespace PiiXeL
