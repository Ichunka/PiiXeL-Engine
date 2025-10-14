#ifndef PIIXELENGINE_EDITORPANEL_HPP
#define PIIXELENGINE_EDITORPANEL_HPP

#ifdef BUILD_WITH_EDITOR

namespace PiiXeL {

class EditorPanel {
public:
    virtual ~EditorPanel() = default;

    virtual void OnImGuiRender() = 0;
    virtual const char* GetTitle() const = 0;
    virtual bool IsOpen() const = 0;
    virtual void SetOpen(bool open) = 0;
};

} // namespace PiiXeL

#endif

#endif
