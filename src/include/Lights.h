#pragma once

#include <glm/glm.hpp>

namespace Sparkle {
namespace Lights {
    enum LTypeFlags {
        SPARKLE_LIGHT_TYPE_UNDEFINED = 0x0,
        SPARKLE_LIGHT_TYPE_DIRECTIONAL = 0x1,
        SPARKLE_LIGHT_TYPE_POINT = 0x2
    };
    typedef uint32_t LType;
    struct Light {
        glm::vec4 mVec;
        glm::vec4 mColor;
        float mRadius;
        glm::vec3 _pad;
    };
}
}