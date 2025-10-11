#ifndef PIIXELENGINE_ASSETREF_HPP
#define PIIXELENGINE_ASSETREF_HPP

#include <string>
#include <raylib.h>

namespace PiiXeL {

template<typename T>
class AssetRef {
public:
    AssetRef() = default;
    explicit AssetRef(const std::string& path) : m_Path{path} {}

    void SetPath(const std::string& path) {
        m_Path = path;
        m_IsLoaded = false;
    }

    [[nodiscard]] const std::string& GetPath() const { return m_Path; }
    [[nodiscard]] bool IsValid() const { return !m_Path.empty(); }

    T* Get();
    const T* Get() const;

    T* operator->() { return Get(); }
    const T* operator->() const { return Get(); }

    operator bool() const { return IsValid(); }

private:
    std::string m_Path;
    mutable T* m_CachedAsset{nullptr};
    mutable bool m_IsLoaded{false};
};

template<>
class AssetRef<Texture2D> {
public:
    AssetRef() = default;
    explicit AssetRef(const std::string& path) : m_Path{path} {}

    void SetPath(const std::string& path) {
        m_Path = path;
        m_Texture = Texture2D{};
        m_IsLoaded = false;
    }

    [[nodiscard]] const std::string& GetPath() const { return m_Path; }
    [[nodiscard]] bool IsValid() const { return m_Texture.id != 0; }

    Texture2D Get();
    const Texture2D& GetConst() const { return m_Texture; }

    operator bool() const { return IsValid(); }

private:
    std::string m_Path;
    mutable Texture2D m_Texture{};
    mutable bool m_IsLoaded{false};
};

} // namespace PiiXeL

#endif // PIIXELENGINE_ASSETREF_HPP
