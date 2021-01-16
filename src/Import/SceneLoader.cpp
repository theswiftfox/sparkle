#include "SceneLoader.h"

#include "Util.h"

#include <filesystem>

using namespace Sparkle;

void Import::SceneLoader::loadFromFile(std::string filePath)
{
	if (glTFImporter) {
		glTFImporter.reset();
	}
	auto path = std::filesystem::path(filePath);
	if (path.extension() == ".gltf1") {
		glTFImporter = std::make_unique<Import::glTFLoader>();
		glTFImporter->loadFromFile(filePath);
	} else {
		throw std::runtime_error("unsupported model file");
	}
}

std::unique_ptr<Geometry::Scene> Import::SceneLoader::processScene()
{
	if (glTFImporter) {
		// todo
		return nullptr;
	} else {
		return nullptr;
	}
}

bool Import::SceneLoader::isLoaded()
{
	if (glTFImporter) {
		return false;
		// return glTFImporter->isLoaded();
	} else {
		return false;
	}
}