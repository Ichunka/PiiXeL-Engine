#ifndef PIIXELENGINE_AUDIOLISTENER_HPP
#define PIIXELENGINE_AUDIOLISTENER_HPP

#include <raylib.h>

namespace PiiXeL {

struct AudioListener {
    bool isActive{true};
    float volume{1.0f};
    bool pauseOnFocusLoss{true};

    Vector3 velocity{0.0f, 0.0f, 0.0f};

    AudioListener() = default;
};

} // namespace PiiXeL

#endif
