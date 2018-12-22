#include "include/AppSettings.h"

#include "SimpleIni.h"

using namespace Engine;

bool Engine::Settings::load() {
	CSimpleIni ini;
	ini.SetUnicode();
	const auto error = ini.LoadFile(filePath.c_str());
	if (error < 0) return false;

	// try to read values for settings
	// Width
	const auto cWidth = ini.GetValue("Display", "Width");
	if (cWidth) {
		try {
			width = std::stoi(cWidth);
		}
		catch (std::exception &ex) {}
	}
	// Height
	const auto cHeight = ini.GetValue("Display", "Height");
	if (cHeight) {
		try {
			height = std::stoi(cHeight);
		}
		catch (std::exception &ex) {}
	}
	// RefreshRate
	const auto cRr = ini.GetValue("Display", "RefreshRate");
	if (cRr) {
		try {
			refreshRate = std::stoi(cRr);
		}
		catch (std::exception &ex) {}
	}
	// Fullscreen
	const auto cFs = ini.GetValue("Display", "Fullscreen");
	if (cFs != nullptr) {
		isFullscreen = cFs[0] == '1';
	}

	// Brightness
	const auto cBr = ini.GetValue("Display", "Brightness");
	if (cBr) {
		try {
			brightness = std::stof(cBr);
		}
		catch (std::exception &ex) {}
	}

	// FOV
	const auto cFov = ini.GetValue("Camera", "FOV");
	if (cFov) {
		try {
			fov = std::stof(cFov);
		}
		catch (std::exception &ex) {}
	}

	// RenderDistance
	const auto cRd = ini.GetValue("Camera", "RenderDistance");
	if (cRd) {
		try {
			renderDistance = std::stof(cRd);
		}
		catch (std::exception &ex) {}
	}

	// validation layer
	const auto cVal = ini.GetValue("Engine", "Validation");
	if (cVal) {
		validation = cVal[0] == '1' || std::string(cVal) == "True";
	}

	// level path
	const auto lvl = ini.GetValue("Scene", "Level");
	if (lvl) {
		try {
			levelPath = std::string(lvl);
		}
		catch (std::exception &ex) {}
	}

	return true;
}

bool Settings::store() {
	bool isOk = true;

	// TODO

	return isOk;
}

int Settings::getRefreshRate() const {
	return refreshRate;
}

float Settings::getFov() const {
	return fov;
}

float Settings::getRenderDistance() const {
	return renderDistance;
}

float Settings::getBrightness() const {
	return brightness;
}

bool Settings::getFullscreen() const {
	return isFullscreen;
}

std::pair<int, int> Settings::getResolution() const {
	return { width, height };
}

std::string Settings::getLevelPath() const {
	return levelPath;
}

bool Settings::withValidationLayer() const {
	return validation;
}