#include "FileReader.h"

#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"

#include <gli/load_dds.hpp>

#include <string>

#ifdef _WIN32
#include <filesystem>
namespace fs = std::filesystem;
#elif __linux__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

std::vector<char> Sparkle::Tools::FileReader::readFile(const std::string& fileName)
{
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

std::vector<std::string> Sparkle::Tools::FileReader::readFileLines(const std::string& fileName)
{
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

void Sparkle::Tools::FileReader::ImageFile::free()
{
    if (imageFileType == SPARKLE_IMAGE_OTHER && imageData) {
        stbi_image_free(imageData);
    }
}

Sparkle::Tools::FileReader::ImageFile Sparkle::Tools::FileReader::loadImage(std::string imagePath)
{
    Sparkle::Tools::FileReader::ImageFile image;
    image.mipCount = -1;
    image.size = -1;
    image.imageFileType = SPARKLE_IMAGE_OTHER;

    fs::path path(imagePath);
    if (path.extension().compare(".dds") == 0) { // use gli
        image.tex = gli::texture2d(gli::load_dds(imagePath));
        if (image.tex.empty()) {
            throw std::runtime_error("DDS: Unable to load texture from: " + imagePath);
        }
        image.imageData = reinterpret_cast<unsigned char*>(image.tex.data());
        auto e = image.tex[0].extent();
        image.width = e.x;
        image.height = e.y;
        image.size = image.tex.size();
        for (size_t i = 0; i < image.tex.levels(); ++i) {
            ImageMipLevel mip;
            mip.width = image.tex[i].extent().x;
            mip.height = image.tex[i].extent().y;
            mip.size = image.tex[i].size();
            image.mipLevels.push_back(mip);
        }
        image.mipCount = image.tex.levels();
        image.imageFileType = SPARKLE_IMAGE_DDS;
    } else { // try to load with stbi TODO: mip levels?
        image.imageData = stbi_load(imagePath.c_str(), &image.width, &image.height, &image.channels, STBI_rgb_alpha);
    }
    if (!image.imageData) {
        throw std::runtime_error("STBI: Unable to load texture from: " + imagePath);
    }
    return image;
}