#pragma once

#ifdef BUILD_WITH_EDITOR

namespace PiiXeL {

class Engine;

class EditorAnimatorPreviewSystem {
public:
    static void UpdateAnimatorPreviewInEditMode(Engine* engine);
};

} // namespace PiiXeL

#endif
