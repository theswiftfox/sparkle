#pragma once

#include <future>
#include <mutex>

#include "Geometry.h"

#include <tiny_gltf.h>

namespace Sparkle {
namespace Import {
	class glTFLoader {
	public:
		void loadFromFile(std::string filePath);
		std::unique_ptr<Geometry::Scene> processGlTF();

	private:
		tinygltf::Model model;
		std::string rootDirectory;

		std::mutex dirMutex;
		std::future<void> levelLoadFuture;

		void loadMaterials();
		void loadTextures();
		void loadNode(std::shared_ptr<Geometry::Node> parent, const tinygltf::Node& node);
	};
} // namespace Import
} // namespace Sparkle