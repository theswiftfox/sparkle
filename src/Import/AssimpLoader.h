#ifndef ASSIMP_LOADER_H
#define ASSIMP_LOADER_H

#include <vector>
#include <future>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "Geometry.h"

namespace Sparkle {
namespace Import {
	class AssimpLoader {
	public:
		void loadFromFile(const std::string& fileName);
		std::unique_ptr<Geometry::Scene> processAssimp();
		bool isLoaded() const { return loaded; }
	private:
		std::atomic_bool loaded = false;

		std::future<void> levelLoadFuture;
		std::vector<std::shared_ptr<Texture>> textureCache;
		std::map<std::string, std::string> textureFiles;
		std::vector<std::shared_ptr<Material>> materialCache;

		std::vector<std::pair<std::string, int>> texFileNames = { { "_albedo", TEX_TYPE_DIFFUSE }, { "_metallic", TEX_TYPE_METALLIC }, { "_normal", TEX_TYPE_NORMAL }, { "_roughness", TEX_TYPE_ROUGHNESS } };

		std::mutex sceneMutex;
		Assimp::Importer importer;
		const aiScene* scenePtr;
		std::mutex dirMutex;
		std::string rootDirectory;

		std::vector<std::shared_ptr<Texture>> loadMaterialTextures(aiMaterial* mat, aiTextureType type, size_t typeID);
		void processAINode(aiNode* node, const aiScene* scene, std::shared_ptr<Geometry::Node> parentNode);
		void createMesh(aiNode* node, const aiScene* scene, std::shared_ptr<Geometry::Node> parentNode);
	
	};
}
}

#endif