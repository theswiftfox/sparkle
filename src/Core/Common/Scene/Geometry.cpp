#include "Geometry.h"
#include "Application.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace Sparkle;
using namespace Geometry;

Mesh::Mesh(MeshData data, std::shared_ptr<Material> material, std::shared_ptr<Node> parent, glm::mat4 model)
{
	this->model = model;
	this->material = material;
	this->parent = parent;
	if (material) { // only upload drawable meshes to gpu!
		meshFromVertsAndIndices(data.vertices, data.indices);
	}
}

void Node::translate(glm::vec3 pos)
{
	model = glm::translate(model, pos);
}

void Mesh::meshFromVertsAndIndices(std::vector<Vertex> verts, std::vector<uint32_t> inds)
{
	vertices = verts;
	indices = inds;

	bufferOffset = App::getHandle().uploadMeshGPU(this);
}

glm::mat4 Node::accumModel()
{
	glm::mat4 m(1.0f);
	if (parent) {
		m = parent->accumModel();
	}
	return m * model;
}

std::vector<std::shared_ptr<Node>> Node::getDrawableSceneAsFlatVec()
{
	std::vector<std::shared_ptr<Node>> nodes;
	for (const auto& c : children) {
		if (c->drawable()) {
			nodes.push_back(c);
		}
		auto childNodes = c->getDrawableSceneAsFlatVec();
		nodes.insert(nodes.end(), childNodes.begin(), childNodes.end());
	}

	return nodes;
}

const std::vector<std::shared_ptr<Node>> Scene::getRenderableScene()
{
	if (cacheDirty) {
		drawableSceneCache = root->getDrawableSceneAsFlatVec();
		cacheDirty = false;
	}
	return drawableSceneCache;
}

size_t Scene::objectCount()
{
	return drawableSceneCache.size();
}

void Scene::cleanup()
{
	root.reset();
	for (auto mat : materialCache) {
		mat->cleanup();
	}
	materialCache.clear();

	for (auto& tex : textureCache) {
		tex->cleanup();
	}
	textureCache.clear();
}
