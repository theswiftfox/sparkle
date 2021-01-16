#pragma once

#include <stdint.h>
#include <string>

namespace Sparkle {
struct Resolution {
	uint32_t width;
	uint32_t height;
};

enum WindowMode {
	WINDOWED = 0,
	BORDERLESS = 1,
	FULLSCREEN = 2
};

struct Config {
	Resolution resolution = { 1024, 768 };
	WindowMode windowMode = WindowMode::WINDOWED;
	float renderDistance = 1000.0f;
	float fov = 75.0f;
	std::string levelPath = "demo_cube";
	bool validationLayers = false;
	float brightness = 1.0f;

	bool load();
	bool load(std::string path);
	bool store();
	bool store(std::string path);
};
} // namespace Sparkle