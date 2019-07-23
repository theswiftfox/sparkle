#include "GltfLoader.h"
#include "Util.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>
#include <Geometry.h>

using namespace Sparkle;
using namespace Geometry;

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

			Mesh::MeshData data = {};

			glm::vec3 minPos = {};
			glm::vec3 maxPos = {};

			const float* bufferPos;
			const float* bufferNorm;
			const float* bufferUV;
			const float* bufferTang;
			const float* bufferBiTang;

			assert(primitive.attributes.find("POSITION") != primitive.attributes.end()); // Position is required
			const auto & posAcc = model.accessors[primitive.attributes.find("POSITION")->second];
			const auto & posView = model.bufferViews[posAcc.bufferView];
			bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAcc.byteOffset + posView.byteOffset]));

			minPos = glm::vec3(posAcc.minValues[0], posAcc.minValues[1], posAcc.minValues[3]);
			maxPos = glm::vec3(posAcc.maxValues[0], posAcc.maxValues[1], posAcc.maxValues[3]);

			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
				const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
				const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
				bufferNorm = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
			}

			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
				const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
				const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
				bufferUV = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
			}
			if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
				const tinygltf::Accessor& tangAccessor = model.accessors[primitive.attributes.find("TANGENT")->second];
				const tinygltf::BufferView& tangView = model.bufferViews[tangAccessor.bufferView];
				bufferNorm = reinterpret_cast<const float*>(&(model.buffers[tangView.buffer].data[tangAccessor.byteOffset + tangView.byteOffset]));
			}
			if (primitive.attributes.find("BITANGENT") != primitive.attributes.end()) {
				const tinygltf::Accessor& biTangAccessor = model.accessors[primitive.attributes.find("BITANGENT")->second];
				const tinygltf::BufferView& biTangView = model.bufferViews[biTangAccessor.bufferView];
				bufferNorm = reinterpret_cast<const float*>(&(model.buffers[biTangView.buffer].data[biTangAccessor.byteOffset + biTangView.byteOffset]));
			}

			for (auto v = 0u; v < posAcc.count; ++v) {
				Vertex vtx;
				vtx.position = glm::make_vec3(&bufferPos[v * 3]);
				vtx.normal = glm::normalize(bufferNorm ? glm::make_vec3(&bufferNorm[v * 3]) : glm::vec3(0.0f));
				vtx.texCoord = bufferUV ? glm::make_vec2(&bufferUV[v * 2]) : glm::vec2(0.0f);
				vtx.tangent = bufferTang ? glm::make_vec3(&bufferTang[v * 3]) : glm::vec3(0.0f);
				vtx.bitangent = bufferBiTang ? glm::make_vec3(&bufferBiTang[v * 3]) : glm::vec3(0.0f);
				data.vertices.push_back(vtx);
			}

			if (primitive.indices > -1) {
				const auto& accessor = model.accessors[primitive.indices];
				const auto& view = model.bufferViews[accessor.bufferView];
				const auto& buffer = model.buffers[view.buffer];

				const void* dataPtr = &(buffer.data[accessor.byteOffset + view.byteOffset]);

				const uint32_t* ptr = nullptr;
				switch (accessor.componentType) {
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					ptr = static_cast<const uint32_t*>(dataPtr);
					for (auto idx = 0u; idx < accessor.count; ++idx) {
						data.indices.push_back(ptr[idx]);
					}
					break;
				default:
					LOGSTDOUT("Index component type " + std::to_string(accessor.componentType) + " not supported!");
					return;
					// todo: other types
				}
			}
			else {
				// todo?
			}
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