#include "SceneLoader.h"

#include "Util.h"

#ifdef _WIN32
#include <filesystem>
namespace fs = std::filesystem;
#elif __linux__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

using namespace Sparkle;

void Import::SceneLoader::loadFromFile(std::string filePath)
{
	if (glTFImporter) {
		glTFImporter.reset();
	}
	auto path = fs::path(filePath);
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
		return nullptr;
	} else {
		return assimpImporter->processAssimp();
	}
}

bool Import::SceneLoader::isLoaded()
{
	if (glTFImporter) {
		return glTFImporter->isLoaded();
	} else {
		return false;
	}
}