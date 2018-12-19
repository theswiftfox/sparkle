#include "Geometry.h"
#include "Application.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <unordered_map>

using namespace Engine;
using namespace Geometry;

Mesh::Mesh(MeshData data, std::shared_ptr<Material> material, glm::mat4 model) {
	this->model = model;
	this->material = material;
	meshFromVertsAndIndices(data.vertices, data.indices);
}

void Node::translate(glm::vec3 pos) {
	model = glm::translate(model, pos);
}


void Geometry::Mesh::meshFromVertsAndIndices(std::vector<Vertex> verts, std::vector<uint32_t> inds)
{
	vertices = verts;
	indices = inds;

	bufferOffset = App::getHandle().uploadMeshGPU(this);
}

glm::mat4 Geometry::Node::accumModel() {
	glm::mat4 m(1.0f);
	if (parent) {
		m = parent->accumModel();
	}
	if (drawable()) {
		m = m * model;
	}
	return m;
}

std::vector<std::shared_ptr<Node>> Engine::Geometry::Node::getDrawableSceneAsFlatVec()
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

void Engine::Geometry::Scene::cleanup() {
	root.reset();

	for (auto& tex : textureCache) {
		tex.reset();
	}

	textureCache.clear();
}

std::vector<std::shared_ptr<Texture>> Engine::Geometry::Scene::loadMaterialTextures(aiMaterial * mat, aiTextureType type, std::string typeName)
{
	std::vector<std::shared_ptr<Texture>> textures;
	
	for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
		aiString str;
		mat->GetTexture(type, i, &str);

		bool isLoaded = false;
		for (const auto& tex : textureCache) {
			if (std::strcmp(tex->path().c_str(), str.C_Str()) == 0) {
				textures.push_back(tex);
				isLoaded = true;
				break;
			}
		}

		if (!isLoaded) {
			auto tex = std::make_shared<Texture>(rootDirectory + str.C_Str());
			textureCache.push_back(tex);
			textures.push_back(tex);
		}
	}

	return textures;
}

void Engine::Geometry::Scene::processAINode(aiNode * node, const aiScene * scene, std::shared_ptr<Node> parentNode)
{
	auto parent = parentNode;
	if (node->mNumMeshes > 0) {
		auto mesh = createMesh(node, scene, parent);
		parent->addChild(mesh);
		parent = mesh;
	}

	for (size_t i = 0; i < node->mNumChildren; ++i) {
		processAINode(node->mChildren[i], scene, parent);
	}
}

std::shared_ptr<Node> Engine::Geometry::Scene::createMesh(aiNode * node, const aiScene * scene, std::shared_ptr<Node> parentNode)
{
	std::vector<std::shared_ptr<Node>> nodes;

	for (size_t i = 0; i < node->mNumMeshes; ++i) {
		auto aiMeshData = scene->mMeshes[node->mMeshes[i]];
		
		Mesh::MeshData data;

		for (size_t j = 0; j < aiMeshData->mNumVertices; ++j) {
			auto aiVtx = aiMeshData->mVertices[j];
			Vertex vtx;
			vtx.position = glm::vec3(aiVtx.x, aiVtx.y, aiVtx.z);
			auto norm = aiMeshData->mNormals[j];
			vtx.normal = glm::vec3(norm.x, norm.y, norm.z);

			if (aiMeshData->mTextureCoords[0]) {
				auto tc = aiMeshData->mTextureCoords[0][j];
				vtx.texCoord = glm::vec2(tc.x, tc.y);
			}

			data.vertices.push_back(vtx);
		}

		for (size_t j = 0; j < aiMeshData->mNumFaces; ++j) {
			auto face = aiMeshData->mFaces[j];
			for (size_t k = 0; k < face.mNumIndices; ++k) {
				data.indices.push_back(face.mIndices[k]);
			}
		}

		// material
		std::shared_ptr<Material> material = nullptr;
		if (aiMeshData->mMaterialIndex >= 0) {
			auto aiMat = scene->mMaterials[aiMeshData->mMaterialIndex];
			auto textures = loadMaterialTextures(aiMat, aiTextureType_DIFFUSE, TEX_TYPE_DIFFUSE);
			auto spec = loadMaterialTextures(aiMat, aiTextureType_SPECULAR, TEX_TYPE_SPECULAR);

			textures.insert(textures.end(), spec.begin(), spec.end());

			material = std::make_shared<Material>(textures);
		}

		auto nodeTransform = node->mTransformation;
		float tvals[16] = {
			nodeTransform.a1, nodeTransform.b1, nodeTransform.c1, nodeTransform.d1,
			nodeTransform.a2, nodeTransform.b2, nodeTransform.c2, nodeTransform.d2,
			nodeTransform.a3, nodeTransform.b3, nodeTransform.c3, nodeTransform.d3,
			nodeTransform.a4, nodeTransform.b4, nodeTransform.b4, nodeTransform.d4
		};

		glm::mat4 nodeModel = glm::make_mat4(tvals);

		auto mesh = std::make_shared<Mesh>(data, material, nodeModel);

		if (aiMeshData->mName.length > 0) {
			mesh->setName(std::string(aiMeshData->mName.C_Str()));
		}

		nodes.push_back(std::static_pointer_cast<Node, Mesh>(mesh));
	}

	if (nodes.size() > 0) {
		auto root = std::make_shared<Node>();
		root->setChildren(nodes);
		return root;
	}
	else {
		return nodes[0];
	}
}

void Engine::Geometry::Scene::loadFromFile(const std::string& fileName) {
	Assimp::Importer importer;

	auto aiScene = importer.ReadFile(fileName,
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices | 
		aiProcess_SortByPType
	);

	if (!aiScene) {
		throw std::runtime_error("Unable to load scene from " + fileName);
	}

	auto dirOffs = std::find(fileName.rbegin(), fileName.rend(), '/');
	if (dirOffs != fileName.rend()) {
		rootDirectory = fileName.substr(0, (fileName.rend() - dirOffs));

	}
	else {
		rootDirectory = "assets";
	}

	processAINode(aiScene->mRootNode, aiScene, root);
}