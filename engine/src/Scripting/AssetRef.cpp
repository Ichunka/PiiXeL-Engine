#include "Scripting/AssetRef.hpp"
#include "Resources/AssetManager.hpp"

namespace PiiXeL {

Texture2D AssetRef<Texture2D>::Get() {
    if (!m_IsLoaded && !m_Path.empty()) {
        m_Texture = AssetManager::Instance().LoadTexture(m_Path);
        m_IsLoaded = true;
    }
    return m_Texture;
}

} // namespace PiiXeL
