#include "GltfLoader.h"
#include "Util.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>

using namespace Sparkle;

void Import::glTFLoader::loadFromFile(std::string filePath)
{
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

	return scene;
}

void Import::glTFLoader::loadMaterials()
{
}
void Import::glTFLoader::loadTextures()
{
}
void Import::glTFLoader::loadNode(std::shared_ptr<Geometry::Node> parent, const tinygltf::Node& node)
{
	glm::mat4 modelmat = glm::mat4(1.0f);
	auto nodeParent = parent;
	if (node.matrix.size() == 16) {
		modelmat = glm::make_mat4x4(node.matrix.data());
	} else {
		glm::vec3 pos = glm::vec3(0.0f);
		if (node.translation.size() == 3) {
			pos = glm::make_vec3(node.translation.data());
		}
		glm::mat3 rot = glm::mat3(1.0f);
		if (node.rotation.size() == 4) {
			glm::quat q = glm::make_quat(node.rotation.data());
			rot = glm::mat3(q);
		}
		glm::vec3 scale = glm::vec3(1.0f);
		if (node.scale.size() == 3) {
			scale = glm::make_vec3(node.scale.data());
		}
		modelmat = glm::mat4(rot);
		modelmat = glm::translate(modelmat, pos);
		modelmat = glm::scale(modelmat, scale);
	}
	if (node.mesh > -1) {
		const auto mesh = model.meshes[node.mesh];
		for (size_t i = 0; i < mesh.primitives.size(); ++i) {
			const auto& primitive = mesh.primitives[i];
			if (primitive.indices < 0) {
				continue;
			}
			const float* bufferPos;
			const float* bufferNorm;
			const float* bufferUV;
			const float* bufferTang;
			const float* bufferBiTang;

			assert(primitive.attributes.find("POSITION") != primitive.attributes.end()); // Position is required
			const auto& posAcc = model.accessors[primitive.attributes.find("POSITION")->second];
			const auto& posView = model.bufferViews[posAcc.bufferView];
			bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAcc.byteOffset + posView.byteOffset]));
		}
	} else {
		auto sparkleNode = std::make_shared<Geometry::Node>(modelmat, nodeParent);
		nodeParent->addChild(sparkleNode);
		nodeParent = sparkleNode;
	}
	if (node.children.size() > 0) {
		for (const auto& c : node.children) {
			loadNode(nodeParent, model.nodes[c]);
		}
	}
}