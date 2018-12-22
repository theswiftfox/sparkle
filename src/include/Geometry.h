#ifndef GEOMETRY_H
#define GEOMETRY_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Texture.h"
#include "Material.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <array>
#include <memory>
#include <vector>
#include <string>
#include <future>
#include <atomic>
#include <mutex>

namespace Engine {
	class App;

	namespace Geometry {
		typedef struct Vertex {
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec3 tangent;
			glm::vec3 bitangent;
			glm::vec2 texCoord;

			static VkVertexInputBindingDescription getBindingDescription() {
				const VkVertexInputBindingDescription binding = {
					0,
					sizeof(Vertex),
					VK_VERTEX_INPUT_RATE_VERTEX
				};
				return binding;
			}

			static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
				const std::array<VkVertexInputAttributeDescription, 5> attributes = { {
					{
						0,
						0,
						VK_FORMAT_R32G32B32_SFLOAT,
						offsetof(Vertex, position)
					},
					{
						1,
						0,
						VK_FORMAT_R32G32B32_SFLOAT,
						offsetof(Vertex, normal)
					},
					{
						2,
						0,
						VK_FORMAT_R32G32B32_SFLOAT,
						offsetof(Vertex, tangent)
					},
					{
						3,
						0,
						VK_FORMAT_R32G32B32_SFLOAT,
						offsetof(Vertex, bitangent)
					},
					{
						4,
						0,
						VK_FORMAT_R32G32_SFLOAT,
						offsetof(Vertex, texCoord)
					}
				} };
				return attributes;
			}


		} Vertex;

		class Node {
		public:
			Node(glm::mat4 modelMat = glm::mat4(1.0f), std::shared_ptr<Node> parentNode = nullptr) : model(modelMat), parent(parentNode), ID("") { }
			virtual bool drawable() { return material != nullptr; }

			glm::mat4 accumModel();

			std::string name() const { return ID; }

			virtual std::shared_ptr<Material> getMaterial() const { return material; }

			std::vector<std::shared_ptr<Node>> getDrawableSceneAsFlatVec();

			void addChild(std::shared_ptr<Node> child) { children.push_back(child); }
			void setChildren(std::vector<std::shared_ptr<Node>> nodes) {
				children = nodes;
			}

			void translate(glm::vec3 direction);
			void rotate(glm::mat3 rotation);
			void scaleUniform(float scaleFactor);
			void scale(glm::vec3 scaleVec);

		protected:
			std::shared_ptr<Node> parent = nullptr;
			std::vector<std::shared_ptr<Node>> children;

			std::shared_ptr<Material> material = nullptr;

			std::string ID;

			glm::mat4 model;
		};

		class Mesh : public Node {
		public:
			struct BufferOffset {
				size_t vertexOffs;
				size_t indexOffs;
			};

			struct MeshData {
				std::vector<Vertex> vertices;
				std::vector<uint32_t> indices;
			};

			Mesh(MeshData data, std::shared_ptr<Material> material, std::shared_ptr<Node> parent = nullptr, glm::mat4 model = glm::mat4(1.0f));

			bool drawable() { return Node::drawable(); }

			std::shared_ptr<Material> getMaterial() { return Node::getMaterial(); }

			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			/**
			 * offset of the vertices and indices in the bound draw buffer
			 */
			BufferOffset bufferOffset;

			void reset() {
				model = initialModel;
			}

			void setName(std::string name) {
				ID = name;
			}

			glm::mat4 modelMat() const {
				return model;
			}
			
			size_t size() const { return indices.size(); }
		private:
			glm::mat4 initialModel;

			void meshFromVertsAndIndices(std::vector<Vertex> verts, std::vector<uint32_t> inds);
		};

		class Scene {
		public:
			Scene() {
				root = std::make_shared<Node>();
			}

			// TODO: buffer this and only update on geometry changes
			const auto getRenderableScene() { return root->getDrawableSceneAsFlatVec(); }

			void loadFromFile(const std::string& fileName);
			void processAssimp();
			bool isLoaded() const { return loaded; }

			void cleanup();
		private:
			std::atomic_bool loaded = false;

			std::future<void> levelLoadFuture;

			std::shared_ptr<Node> root;
			std::vector<std::shared_ptr<Texture>> textureCache;

			std::mutex sceneMutex;
			Assimp::Importer importer;
			const aiScene* scenePtr;
			std::mutex dirMutex;
			std::string rootDirectory;

			std::vector<std::shared_ptr<Texture>> loadMaterialTextures(aiMaterial* mat, aiTextureType type, size_t typeID);
			void processAINode(aiNode* node, const aiScene* scene, std::shared_ptr<Node> parentNode);
			void createMesh(aiNode* node, const aiScene* scene, std::shared_ptr<Node> parentNode);
		};
	}
}


#endif
