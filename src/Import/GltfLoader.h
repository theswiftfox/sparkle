#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include <future>
#include <mutex>

#include "Geometry.h"
#include <tinygltf/tiny_gltf.h>

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
		void loadNode(std::shared_ptr<Geometry::Node> parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model);
	};
} // namespace Utils
} // namespace Sparkle

#endif