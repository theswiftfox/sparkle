#pragma once

#include <memory>

#include "Geometry.h"
#include "GltfLoader.h"

namespace Sparkle {
namespace Import {
	class SceneLoader {
	public:
		void loadFromFile(std::string filePath);

		bool isLoaded();
		std::unique_ptr<Geometry::Scene> processScene();

	private:
		std::unique_ptr<glTFLoader> glTFImporter = nullptr;
	};
} // namespace Import
} // namespace Sparkle