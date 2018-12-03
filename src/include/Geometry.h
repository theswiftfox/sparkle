#ifndef GEOMETRY_H
#define GEOMETRY_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Texture.h"

#include <array>
#include <memory>
#include <vector>
#include <string>

namespace Engine {
	class App;

	namespace Geometry {
		typedef struct PerMeshPushConstants {
			VkBool32 useTexture;
			VkBool32 isSelected;
		} PushConstants;

		typedef struct Vertex {
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec4 baseColor;
			glm::vec2 texCoord;

			static VkVertexInputBindingDescription getBindingDescription() {
				const VkVertexInputBindingDescription binding = {
					0,
					sizeof(Vertex),
					VK_VERTEX_INPUT_RATE_VERTEX
				};
				return binding;
			}

			static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
				const std::array<VkVertexInputAttributeDescription, 4> attributes = { {
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
						VK_FORMAT_R32G32B32A32_SFLOAT,
						offsetof(Vertex, baseColor)
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
				return position == other.position && normal == other.normal && baseColor == other.baseColor && texCoord == other.texCoord;
			}

		} Vertex;

		typedef struct ColoredVertex {
			glm::vec3 position;
			glm::vec4 color;

			static VkVertexInputBindingDescription getBindingDescription() {
				const VkVertexInputBindingDescription binding = {
					0,
					sizeof(ColoredVertex),
					VK_VERTEX_INPUT_RATE_VERTEX
				};
				return binding;
			}

			static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
				const std::array<VkVertexInputAttributeDescription, 2> attributes = { {
					{
						0,
						0,
						VK_FORMAT_R32G32B32_SFLOAT,
						offsetof(ColoredVertex, position)
					},
					{
						1,
						0,
						VK_FORMAT_R32G32B32_SFLOAT,
						offsetof(ColoredVertex, color)
					}
					} };
				return attributes;
			}

			bool operator==(const ColoredVertex& other) const {
				return position == other.position && color == other.color;
			}
		} CVertex;

		enum ColorType {
			TEX = 0, // texture
			MTL = 1, // materials
			VTC = 2  // vertex colors
		};

		class Mesh {
		public:
			struct BufferOffset {
				size_t vertexOffs;
				size_t indexOffs;
			};

			Mesh(std::vector<Vertex> verts, std::vector<uint32_t> indices);
			Mesh(std::string id, std::string fromObj, std::string textureOrMtlDir, ColorType colorType = ColorType::VTC);

			VkDescriptorSet getDescriptorSet() const { return pDescriptorSet; }

			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			/**
			 * \brief offset of the vertices and indices in the bound draw buffer
			 */
			BufferOffset bufferOffset;

			PushConstants getStatus() const {
				return status;
			}

			void select() {
				status.isSelected = VK_TRUE;
			}
			void unselect() {
				status.isSelected = VK_FALSE;
			}

			void translate(glm::vec3 pos);
			void setTranslation(glm::vec3 pos);
			void setScale(glm::vec3 scaleVec);
			void scale(glm::vec3 scale);
			void rotateTo(glm::vec3 targetPoint);
			void scaleToSize(float h);
			void setRotation(glm::vec3 r);

			/**
			 * \brief resets the model matrix of the mesh
			 */
			void restore() {
				model = glm::mat4(1.0f);
			}

			glm::vec3 getPosition();
			glm::vec3 getRotation();
			glm::vec3 getScale();

			 std::string name() const { return ID; }

			glm::mat4 modelMat() const {
				return model;
			}

			size_t size() const { return indices.size(); }
		private:
			std::string ID;

			float height = 1.0f;

			PushConstants status = { VK_FALSE, VK_FALSE };

			glm::mat4 model = glm::mat4(1.0f);

			VkDescriptorPool pDescriptorPool;
			VkDescriptorSetLayout pDescriptorSetLayout;
			VkDescriptorSet pDescriptorSet;

			std::unique_ptr<Engine::Texture> pSampler;

			void createDescriptorPool();
			void createDescriptorSetLayout();
			void createDescriptorSet();
			void meshFromVertsAndIndices(std::vector<Vertex> verts, std::vector<uint32_t> inds);
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

namespace std {
	template<> struct hash<Engine::Geometry::CVertex> {
		size_t operator()(Engine::Geometry::CVertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1);
		}
	};
}

#endif
