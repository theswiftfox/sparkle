#include "FileReader.h"

#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <string>


std::vector<char> Engine::Tools::FileReader::readFile(const std::string& fileName) {
	std::ifstream file(fileName, std::ios::ate | std::ios::binary); // read the file in binary back to front

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + fileName);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buff(fileSize);

	file.seekg(0);
	file.read(buff.data(), fileSize);

	file.close();

	return buff;
}

std::vector<std::string> Engine::Tools::FileReader::readFileLines(const std::string& fileName) {
	std::ifstream file(fileName); // read the file in binary back to front

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + fileName);
	}

	std::vector<std::string> lines;
	std::string line;
	while (getline(file, line)) {
		lines.push_back(line);
	}
	file.close();
	return lines;
}

void Engine::Tools::FileReader::ImageFile::free() {
	if (imageData) {
		stbi_image_free(imageData);
	}
}

Engine::Tools::FileReader::ImageFile Engine::Tools::FileReader::loadImage(std::string imagePath) {
	Engine::Tools::FileReader::ImageFile image;

	image.imageData = stbi_load(imagePath.c_str(), &image.width, &image.height, &image.channels, STBI_rgb_alpha);
	if (!image.imageData) {
		throw std::runtime_error("Unable to load texture from: " + imagePath);
	}
	return image;
}