#define TINYGLTF_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

#include "GltfLoader.h"
#include "Util.h"

using namespace Sparkle;

void Import::glTFLoader::loadFromFile(std::string filePath) {
	levelLoadFuture = std::async(std::launch::async, [this, filePath]() {
		tinygltf::TinyGLTF loader;
		std::string err, warn;
		auto ret = loader.LoadASCIIFromFile(&model, &err, &warn, filePath);

		if (!warn.empty()) {
			LOGSTDOUT(warn)
		}
		if (!err.empty()) {
			LOGSTDOUT(err);
		}
		if (!ret) {
			throw std::runtime_error("Unable to read glTF file!");
		}
		auto dirOffs = filePath.rfind('/');
		dirOffs = dirOffs == std::string::npos ? filePath.rfind('\\') : dirOffs;
		{
			std::lock_guard<std::mutex> lock(dirMutex);
			if (dirOffs != std::string::npos) {
				rootDirectory = filePath.substr(0, dirOffs) + "/";
			} else {
				rootDirectory = "assets/";
			}
		}
	});
}

std::unique_ptr<Geometry::Scene> Import::glTFLoader::processGlTF()
{
	levelLoadFuture.get();
	auto scene = std::make_unique<Geometry::Scene>();

}