#include "GltfLoader.h"
#include "SceneLoader.h"
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
		progress.state = ImportProgress::State::READING;
		progress.percent = 0.1f;
		auto ret = loader.LoadASCIIFromFile(&model, &err, &warn, filePath);
		progress.percent = 0.5f;
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
        loadTextures();
        loadMaterials();
        loaded = true;
	});
}

std::unique_ptr<Geometry::Scene> Import::glTFLoader::processGlTF()
{
	levelLoadFuture.get();
    for (auto& mat : materialCache) {
        mat->init();
    }
	progress.state = ImportProgress::State::MESH;
	auto scene = std::make_unique<Geometry::Scene>();
	const auto& glTFscene = model.defaultScene > -1 ? model.scenes[model.defaultScene] : model.scenes[0];
    auto p = 1u;
    auto mul = 1.0f / (float)glTFscene.nodes.size();
    for (auto& node : glTFscene.nodes) {
        progress.percent = p * mul;
        loadNode(scene->getRootNodePtr(), model.nodes[node]);
        ++p;
    }
    progress.state = ImportProgress::State::DONE;
    progress.percent = 1.0f;
	return scene;
}

void Import::glTFLoader::loadMaterials()
{
    progress.state = ImportProgress::State::MATERIALS;
    auto p = 1u;
    auto mul = 1.0f / (float)model.materials.size();
    for (auto& material : model.materials) {
        progress.percent = p * mul;
        std::vector<std::shared_ptr<Texture>> textures;
        textures.push_back(placeholder);
        if (material.values.find("baseColorTexture") != material.values.end()) {
            auto idx = material.values["baseColorTexture"].TextureIndex();
            if (idx < textureCache.size()) {
                if (textureCache[idx]->type() == TEX_TYPE_NONE) {
                    textureCache[idx]->changeType(TEX_TYPE_DIFFUSE);
                }
                textures.push_back(textureCache[idx]);
            }
        }
        if (material.values.find("metallicRoughnessTexture") != material.values.end()) {
            auto idx = material.values["metallicRoughnessTexture"].TextureIndex();
            if (idx < textureCache.size()) {
                if (textureCache[idx]->type() == TEX_TYPE_NONE) {
                    textureCache[idx]->changeType(TEX_TYPE_METALLIC_ROUGHNESS);
                }
                textures.push_back(textureCache[idx]);
            }
        }
        if (material.values.find("normalTexture") != material.values.end()) {
            auto idx = material.values["normalTexture"].TextureIndex();
            if (idx < textureCache.size()) {
                if (textureCache[idx]->type() == TEX_TYPE_NONE) {
                    textureCache[idx]->changeType(TEX_TYPE_NORMAL);
                }
                textures.push_back(textureCache[idx]);
            }
        }
        if (material.extensions.find("KHR_materials_pbrSpecularGlossiness") != material.extensions.end()) {
			auto ext = material.extensions.find("KHR_materials_pbrSpecularGlossiness");
			if (ext->second.Has("specularGlossinessTexture")) {
                auto idx = ext->second.Get("specularGlossinessTexture").Get("index").Get<int>();
                if (idx < textureCache.size()) {
                    if (textureCache[idx]->type() == TEX_TYPE_NONE) {
                        textureCache[idx]->changeType(TEX_TYPE_SPECULAR);
                    }
                    textures.push_back(textureCache[idx]);
                }
			}
			if (ext->second.Has("diffuseTexture")) {
                auto idx = ext->second.Get("diffuseTexture").Get("index").Get<int>();
                if (idx < textureCache.size()) {
                    if (textureCache[idx]->type() == TEX_TYPE_NONE) {
                        textureCache[idx]->changeType(TEX_TYPE_DIFFUSE);
                    }
                    textures.push_back(textureCache[idx]);
                }
			}
		}
        materialCache.push_back(std::make_shared<Material>(textures, true));
        ++p;
    }
}
void Import::glTFLoader::loadTextures()
{
    // TODO: samplers
    progress.state = ImportProgress::State::TEXTURES;
    auto p = 1u;
    auto mul = 1.0f / (float)model.textures.size();
    for (auto& tex : model.textures) {
        progress.percent = p * mul;
        auto img = model.images[tex.source];
        if (img.uri.length() > 0) {
            textureCache.push_back(std::make_shared<Texture>(rootDirectory + img.uri, TEX_TYPE_NONE));
        } else {
            unsigned char* imgBuff = nullptr;
            bool tempBuffer = false;
            if (img.component == 3) { // convert to rgba
                auto size = img.width * img.height * 4;
                imgBuff = new unsigned char[size];
                auto ptrImgBuff = imgBuff;
                auto ptrImgData = img.image.data();
                for (auto i = 0; i < img.width * img.height; ++i) {
                    for (auto j = 0u; j < 3; ++j) {
                        ptrImgBuff[j] = ptrImgData[j];
                    }
                    ptrImgBuff[3] = 0;

                    ptrImgBuff += 4;
                    ptrImgData += 3;
                }
                tempBuffer = true;
            } else {
                imgBuff = img.image.data();
            }
            textureCache.push_back(std::make_shared<Texture>(imgBuff, img.width, img.height, 4, TEX_TYPE_NONE));
            if (tempBuffer) {
                delete[] imgBuff;
            }
        }
        ++p;
    }
    placeholder = std::make_shared<Texture>("assets/materials/default/diff.png", TEX_TYPE_PLACEHOLDER);
}
void Import::glTFLoader::loadNode(std::shared_ptr<Geometry::Node> parent, const tinygltf::Node& node)
{
	glm::mat4 modelMat = glm::mat4(1.0f);
	auto nodeParent = parent;
	if (node.matrix.size() == 16) {
		modelMat = glm::make_mat4x4(node.matrix.data());
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
		modelMat = glm::mat4(rot);
		modelMat = glm::translate(modelMat, pos);
		modelMat = glm::scale(modelMat, scale);
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

			const float* bufferPos = nullptr;
            const float* bufferNorm = nullptr;
            const float* bufferUV = nullptr;
            const float* bufferTang = nullptr;
            const float* bufferBiTang = nullptr;

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
				bufferTang = reinterpret_cast<const float*>(&(model.buffers[tangView.buffer].data[tangAccessor.byteOffset + tangView.byteOffset]));
			}
			if (primitive.attributes.find("BITANGENT") != primitive.attributes.end()) {
				const tinygltf::Accessor& biTangAccessor = model.accessors[primitive.attributes.find("BITANGENT")->second];
				const tinygltf::BufferView& biTangView = model.bufferViews[biTangAccessor.bufferView];
				bufferBiTang = reinterpret_cast<const float*>(&(model.buffers[biTangView.buffer].data[biTangAccessor.byteOffset + biTangView.byteOffset]));
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

				switch (accessor.componentType) {
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
					const uint32_t* ptr = static_cast<const uint32_t*>(dataPtr);
					for (auto idx = 0u; idx < accessor.count; ++idx) {
						data.indices.push_back(ptr[idx]);
					}
					break;
				}
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
					const uint16_t* ptr = static_cast<const uint16_t*>(dataPtr);
					for (auto idx = 0u; idx < accessor.count; ++idx) {
						data.indices.push_back(ptr[idx]);
					}
					break;
				}
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
					const uint8_t* ptr = static_cast<const uint8_t*>(dataPtr);
					for (auto idx = 0u; idx < accessor.count; ++idx) {
						data.indices.push_back(ptr[idx]);
					}
					break;
				}
				default:
					LOGSTDOUT("Index component type " + std::to_string(accessor.componentType) + " not supported!");
					return;
				}
			}
			else {
				// expect to have an index per vertex?
				for (auto i = 0u; i < data.vertices.size(); ++i) {
				    data.indices.push_back(i);
				}
			}

			// material:
			std::shared_ptr<Material> material = nullptr;
			if (primitive.material > -1) {
                if (primitive.material < materialCache.size()) {
                    material = materialCache[primitive.material];
                } else {
                    LOGSTDOUT("Material referenced by primitive not found. Is this glTF file valid?");
                    // TODO: assign default material
                }
			} else {

			}

			auto sparkleMesh = std::make_shared<Geometry::Mesh>(data, material, nodeParent, modelMat);
		}
	} else {
		auto sparkleNode = std::make_shared<Geometry::Node>(modelMat, nodeParent);
		nodeParent->addChild(sparkleNode);
		nodeParent = sparkleNode;
	}
	if (node.children.size() > 0) {
		for (const auto& c : node.children) {
			loadNode(nodeParent, model.nodes[c]);
		}
	}
}