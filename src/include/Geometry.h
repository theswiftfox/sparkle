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

#include <array>
#include <memory>
#include <vector>
#include <string>

namespace Engine {
	class App;

	namespace Geometry {
		typedef struct Vertex {
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec2 texCoord;

			static VkVertexInputBindingDescription getBindingDescription() {
				const VkVertexInputBindingDescription binding = {
					0,
					sizeof(Vertex),
					VK_VERTEX_INPUT_RATE_VERTEX
				};
				return binding;
			}

			static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
				const std::array<VkVertexInputAttributeDescription, 3> attributes = { {
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
						3,
						0,
						VK_FORMAT_R32G32_SFLOAT,
						offsetof(Vertex, texCoord)
	}
				} };
				return attributes;
			}

			bool operator==(const Vertex& other) const {
				return position == other.position && normal == other.normal && texCoord == other.texCoord;
			}

		} Vertex;

		class Node {
		public:
			virtual bool drawable() { return material != nullptr; }

			glm::mat4 accumModel();

			std::string name() const { return ID; }

			virtual std::shared_ptr<Material> getMaterial() const { return material; }

			std::vector<std::shared_ptr<Node>> getDrawableSceneAsFlatVec();

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

			glm::mat4 model = glm::mat4(1.0f);
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

			Mesh(MeshData data, std::shared_ptr<Material> material, glm::mat4 model = glm::mat4(1.0f));

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

			const auto getRenderableScene() { return root->getDrawableSceneAsFlatVec(); }

			void loadFromFile(const std::string& fileName);

			void cleanup();
		private:
			std::shared_ptr<Node> root;
			std::vector<std::shared_ptr<Texture>> textureCache;

			std::string rootDirectory;

			std::vector<std::shared_ptr<Texture>> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
			void processAINode(aiNode* node, const aiScene* scene, std::shared_ptr<Node> parentNode);
			std::shared_ptr<Node> createMesh(aiNode* node, const aiScene* scene, std::shared_ptr<Node> parentNode);
		};
	}
}

namespace std {
	template<> struct hash<Engine::Geometry::Vertex> {
		size_t operator()(Engine::Geometry::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}


#endif
