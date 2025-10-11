#ifndef PIIXELENGINE_ASSETREF_HPP
#define PIIXELENGINE_ASSETREF_HPP

#include "Components/UUID.hpp"
#include <string>
#include <raylib.h>

namespace PiiXeL {

template<typename T>
class AssetRef {
public:
    AssetRef() = default;
    explicit AssetRef(const std::string& path) : m_Path{path} {}
    explicit AssetRef(UUID uuid) : m_UUID{uuid} {}

    void SetPath(const std::string& path) {
        m_Path = path;
        m_UUID = UUID(0);
        m_IsLoaded = false;
    }

    void SetUUID(UUID uuid) { m_UUID = uuid; }
    [[nodiscard]] UUID GetUUID() const { return m_UUID; }
    [[nodiscard]] const std::string& GetPath() const { return m_Path; }
    [[nodiscard]] bool IsValid() const { return !m_Path.empty() || m_UUID.Get() != 0; }

    T* Get();
    const T* Get() const;

    T* operator->() { return Get(); }
    const T* operator->() const { return Get(); }

    operator bool() const { return IsValid(); }

private:
    std::string m_Path;
    UUID m_UUID{0};
    mutable T* m_CachedAsset{nullptr};
    mutable bool m_IsLoaded{false};
};

template<>
class AssetRef<Texture2D> {
public:
    AssetRef() = default;
    explicit AssetRef(const std::string& path) : m_Path{path} {}
    explicit AssetRef(UUID uuid) : m_UUID{uuid} {}

    void SetPath(const std::string& path) {
        m_Path = path;
        m_UUID = UUID(0);
        m_Texture = Texture2D{};
        m_IsLoaded = false;
    }

    void SetUUID(UUID uuid) { m_UUID = uuid; }
    [[nodiscard]] UUID GetUUID() const { return m_UUID; }
    [[nodiscard]] const std::string& GetPath() const { return m_Path; }
    [[nodiscard]] bool IsValid() const { return m_Texture.id != 0; }

    Texture2D Get();
    const Texture2D& GetConst() const { return m_Texture; }

    operator bool() const { return IsValid(); }

private:
    std::string m_Path;
    UUID m_UUID{0};
    mutable Texture2D m_Texture{};
    mutable bool m_IsLoaded{false};
};

} // namespace PiiXeL

#endif // PIIXELENGINE_ASSETREF_HPP
