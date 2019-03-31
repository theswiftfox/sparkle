#ifndef SCENE_LOADER_H
#define SCENE_LOADER_H

#include <memory>

#include "Geometry.h"
#include "AssimpLoader.h"
#include "GltfLoader.h"

namespace Sparkle {
namespace Import {
	class SceneLoader {
	public:
		void loadFromFile(std::string filePath);

		bool isLoaded();
		std::unique_ptr<Geometry::Scene> processScene();

	private:
		std::unique_ptr<AssimpLoader> assimpImporter = nullptr;
		std::unique_ptr<glTFLoader> glTFImporter = nullptr;
	};
}
}
#endif