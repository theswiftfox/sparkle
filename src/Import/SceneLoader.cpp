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
	if (assimpImporter) {
		assimpImporter.reset();
	}
	auto path = fs::path(filePath);
	if (path.extension() == ".gltf") {
		glTFImporter = std::make_unique<Import::glTFLoader>();
		glTFImporter->loadFromFile(filePath);
	} else {
		assimpImporter = std::make_unique<Import::AssimpLoader>();
		assimpImporter->loadFromFile(filePath);
	}

}

std::unique_ptr<Geometry::Scene> Import::SceneLoader::processScene() {
	if (glTFImporter) {
		return glTFImporter->processGlTF();
	} else {
		return assimpImporter->processAssimp();
	}
}

bool Import::SceneLoader::isLoaded() {
	if (glTFImporter) {
		return glTFImporter->isLoaded();
	} else {
		return assimpImporter->isLoaded();
	}
}

Import::ImportProgress Import::SceneLoader::getProgress() const {
    if (glTFImporter) {
        return glTFImporter->progress;
    } else {
        ImportProgress progress;
        progress.state = ImportProgress::State::DONE;
        return progress;
    }
}