#ifndef GEOMETRY_H
#define GEOMETRY_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>
#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "Material.h"
#include "SparkleTypes.h"
#include "Texture.h"

namespace Sparkle {
class App;

namespace Geometry {
	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
		glm::vec2 texCoord;

		static std::array<VkVertexInputBindingDescription, 1> getBindingDescriptions()
		{
			const std::array<VkVertexInputBindingDescription, 1> bindings = { {
				{ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX },
			} };
			return bindings;
		}

		static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions()
		{
			const std::array<VkVertexInputAttributeDescription, 5> attributes = {
				{ { 0,
				      0,
				      VK_FORMAT_R32G32B32_SFLOAT,
				      offsetof(Vertex, position) },
				    { 1,
				        0,
				        VK_FORMAT_R32G32B32_SFLOAT,
				        offsetof(Vertex, normal) },
				    { 2,
				        0,
				        VK_FORMAT_R32G32B32_SFLOAT,
				        offsetof(Vertex, tangent) },
				    { 3,
				        0,
				        VK_FORMAT_R32G32B32_SFLOAT,
				        offsetof(Vertex, bitangent) },
				    { 4,
				        0,
				        VK_FORMAT_R32G32_SFLOAT,
				        offsetof(Vertex, texCoord) } }
			};
			return attributes;
		}
	};

	class Node {
	public:
		Node(glm::mat4 modelMat = glm::mat4(1.0f), std::shared_ptr<Node> parentNode = nullptr)
		    : model(modelMat)
		    , parent(parentNode)
		    , ID("")
		{
		}
		virtual bool drawable() { return material != nullptr; }

		glm::mat4 accumModel();

		std::string name() const { return ID; }

		virtual std::shared_ptr<Material> getMaterial() const { return material; }

		std::vector<std::shared_ptr<Node>> getDrawableSceneAsFlatVec();

		void addChild(std::shared_ptr<Node>& child) { children.push_back(child); }
		void setChildren(std::vector<std::shared_ptr<Node>>& nodes)
		{
			children = nodes;
		}

		void setParent(std::shared_ptr<Node>& par) { parent = par; }

		void translate(glm::vec3 direction);
		void rotate(glm::mat3 rotation);
		void scaleUniform(float scaleFactor);
		void scale(glm::vec3 scaleVec);


	protected:
		std::vector<std::shared_ptr<Node>> children;

		std::shared_ptr<Node> parent = nullptr;

		std::shared_ptr<Material> material = nullptr;

		std::string ID;

		glm::mat4 model;

		glm::mat4 cachedModel;
		bool dirty = true;
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
			BoundingSphere boundingSphere;
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

		void reset()
		{
			model = initialModel;
		}

		void setName(std::string name)
		{
			ID = name;
		}

		glm::mat4 modelMat() const
		{
			return model;
		}

		auto getBounds() const { return boundingSphere; }

		size_t size() const { return indices.size(); }

	private:
		glm::mat4 initialModel;

		BoundingSphere boundingSphere;

		void meshFromVertsAndIndices(std::vector<Vertex> verts, std::vector<uint32_t> inds);
	};

	class Scene {
	public:
		Scene()
		{
			root = std::make_shared<Node>(/*model*/);
		}

		std::shared_ptr<Node> getRootNodePtr()
		{
			return root;
		}

		const std::vector<std::shared_ptr<Node>> getRenderableScene();
		size_t objectCount();

		void cleanup();
		void setDirty() { cacheDirty = true; }

		std::vector<std::shared_ptr<Texture>> textureCache;
		std::vector<std::shared_ptr<Material>> materialCache;

	private:
		std::shared_ptr<Node> root;

		std::vector<std::shared_ptr<Node>> drawableSceneCache;
		bool cacheDirty = false;

	};
} // namespace Geometry
} // namespace Sparkle

#endif
