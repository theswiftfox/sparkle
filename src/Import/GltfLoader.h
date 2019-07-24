#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include <future>
#include <mutex>

#include "Geometry.h"

#include <tinygltf/tiny_gltf.h>

namespace Sparkle {
namespace Import {
	constexpr const char* StatusStr[] = { "assets", "textures", "materials", "meshes", "" };
	struct ImportProgress {
		enum State { READING = 0,
			TEXTURES = 1,
			MATERIALS = 2,
			MESH = 3,
			DONE = 4 } state;
		float percent;
	};
	class glTFLoader {
	public:
		void loadFromFile(std::string filePath);
		std::unique_ptr<Geometry::Scene> processGlTF();

		bool isLoaded() { return loaded; }
		ImportProgress progress;

	private:
		tinygltf::Model model;
		std::string rootDirectory;

		std::shared_ptr<Texture> placeholder;
		std::vector<std::shared_ptr<Texture>> textureCache;
		std::vector<std::shared_ptr<Material>> materialCache;

		std::mutex dirMutex;
		std::future<void> levelLoadFuture;
		std::atomic_bool loaded = ATOMIC_VAR_INIT(false);

		void loadMaterials();
		void loadTextures();
		void loadNode(std::shared_ptr<Geometry::Node> parent, const tinygltf::Node& node);
	};
} // namespace Import
} // namespace Sparkle

#endif