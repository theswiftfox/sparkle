#pragma once

#include <stdint.h>

namespace sparkle
{
    struct Resolution
    {
        uint32_t width;
        uint32_t height;
    };

    enum WindowMode
    {
        WINDOWED = 0,
        BORDERLESS = 1,
        FULLSCREEN = 2
    };

    struct Config
    {
        Resolution resolution = {1024, 768};
        WindowMode windowMode = WindowMode::WINDOWED;
        float renderDistance = 1000.0f;
        float fov = 75.0f;

        bool load();
        bool store();
    };
} // namespace sparkle