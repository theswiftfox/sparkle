#include "Geometry.h"
#include "Application.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <unordered_map>

#include <algorithm>

#ifdef __WIN32
	#include <filesystem>
	namespace fs = std::filesystem;
#elif __linux__
	#include <experimental/filesystem>
	namespace fs = std::experimental::filesystem;
#endif

using namespace Engine;
using namespace Geometry;

static bool isImageExtension(std::string ext) {
	if (ext.compare(".png") == 0) return true;
	if (ext.compare(".jpg") == 0) return true;
	if (ext.compare(".bmp") == 0) return true;
	return false;
}

Mesh::Mesh(MeshData data, std::shared_ptr<Material> material, std::shared_ptr<Node> parent, glm::mat4 model) {
	this->model = model;
	this->material = material;
	this->parent = parent;
	if (material) { // only upload drawable meshes to gpu!
		meshFromVertsAndIndices(data.vertices, data.indices);
	}
}

void Node::translate(glm::vec3 pos) {
	model = glm::translate(model, pos);
}


void Mesh::meshFromVertsAndIndices(std::vector<Vertex> verts, std::vector<uint32_t> inds)
{
	vertices = verts;
	indices = inds;

	bufferOffset = App::getHandle().uploadMeshGPU(this);
}

glm::mat4 Node::accumModel() {
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

const std::vector<std::shared_ptr<Node>> Scene::getRenderableScene() {
	if (cacheDirty) {
		drawableSceneCache = root->getDrawableSceneAsFlatVec();
		cacheDirty = false;
	}
	return drawableSceneCache;
}

void Scene::cleanup() {
	for (auto node : drawableSceneCache) {
		node->getMaterial()->cleanup();
	}
	root.reset();

	for (auto& tex : textureCache) {
		tex->cleanup();
	}

	textureCache.clear();
}

std::vector<std::shared_ptr<Texture>> Scene::loadMaterialTextures(aiMaterial * mat, aiTextureType type, size_t typeID)
{
	std::vector<std::shared_ptr<Texture>> textures;
	
	for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
		aiString str;
		mat->GetTexture(type, i, &str);
		auto strPtr = str.C_Str();
		if (*strPtr == '*') { // embedded texture!
			try {
				auto index = std::stoi(strPtr + 1);
				auto texData = scenePtr->mTextures[index];
				auto name = std::string(texData->mFilename.C_Str());

				bool isLoaded = false;
				for (const auto& tex : textureCache) {
					if (std::strcmp(tex->path().c_str(), name.c_str()) == 0) {
						textures.push_back(tex);
						isLoaded = true;
						break;
					}
				}
				if (!isLoaded) {
					auto tex = std::make_shared<Texture>(texData, typeID, name);
					textures.push_back(tex);
				}
			}
			catch (std::exception& ex) {
				// todo: log function
				std::cout << ex.what() << std::endl;
			}
		}
		else {
			std::string stdString;
			if (*strPtr == '/' || *strPtr == '\\') {
				stdString = std::string(strPtr + 1);
			}
			else {
				stdString = std::string(strPtr);
			}
			std::replace(stdString.begin(), stdString.end(), '\\', '/');
			//auto dirOffs = stdString.rfind('/');
			//dirOffs = dirOffs == std::string::npos ? stdString.rfind('\\') : dirOffs;
			//if (dirOffs != std::string::npos) {
			//	stdString = stdString.substr(dirOffs + 1);
			//}

			{
				std::lock_guard<std::mutex> lock(dirMutex);
				stdString = rootDirectory + stdString.c_str();
			}
			bool isLoaded = false;
			for (const auto& tex : textureCache) {
				if (std::strcmp(tex->path().c_str(), stdString.c_str()) == 0) {
					textures.push_back(tex);
					isLoaded = true;
					break;
				}
			}

			if (!isLoaded) {
				auto tex = std::make_shared<Texture>(stdString, typeID);
				textureCache.push_back(tex);
				textures.push_back(tex);
			}
		}
	}

	return textures;
}

void Scene::processAINode(aiNode * node, const aiScene * scene, std::shared_ptr<Node> parentNode)
{
	//auto aiMat = glm::make_mat4(&node->mTransformation.a1);
	//auto modelMat = glm::transpose(aiMat);
	//auto parent = std::make_shared<Node>(modelMat, parentNode);
	auto parent = parentNode;
	if (node->mNumMeshes > 0) {
		createMesh(node, scene, parent);
	}	

	for (size_t i = 0; i < node->mNumChildren; ++i) {
		processAINode(node->mChildren[i], scene, parent);
	}
	for (size_t i = 0; i < scene->mNumLights; ++i) {
		auto aiLight = scene->mLights[i];
		auto diffCol = aiLight->mColorDiffuse;
		auto specCol = aiLight->mColorSpecular;

		aiVector3D vec(0.0f);

		switch (aiLight->mType) {
		case aiLightSource_DIRECTIONAL:
			vec = aiLight->mDirection;

			break;
		case aiLightSource_POINT:
			vec = aiLight->mPosition;
			
			break;
		default:
			std::cout << "Light with invalid type. Skipping\r\n";
		}
	}
}

void Scene::createMesh(aiNode * node, const aiScene * scene, std::shared_ptr<Node> parentNode)
{
	auto meshRoot = parentNode;
	if (node->mNumMeshes > 1) {
		meshRoot = std::make_shared<Node>(glm::mat4(1.0f), parentNode);
	}

	for (size_t i = 0; i < node->mNumMeshes; ++i) {
		auto aiMeshData = scene->mMeshes[node->mMeshes[i]];
		
		Mesh::MeshData data;

		for (size_t j = 0; j < aiMeshData->mNumVertices; ++j) {
			auto aiVtx = aiMeshData->mVertices[j];
			Vertex vtx;
			vtx.position = glm::vec3(aiVtx.x, aiVtx.y, aiVtx.z);
			auto norm = aiMeshData->mNormals[j];
			vtx.normal = glm::vec3(norm.x, norm.y, norm.z);
			if (aiMeshData->mTangents) {
				auto tang = aiMeshData->mTangents[j];
				vtx.tangent = glm::vec3(tang.x, tang.y, tang.z);
				auto btang = aiMeshData->mBitangents[j];
				vtx.bitangent = glm::vec3(btang.x, btang.y, btang.z);
			}
			else {
				vtx.tangent = glm::vec3(0.0f);
				vtx.bitangent = vtx.tangent;
			}

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
			auto norm = loadMaterialTextures(aiMat, aiTextureType_HEIGHT, TEX_TYPE_NORMAL);
			auto rough = loadMaterialTextures(aiMat, aiTextureType_SHININESS, TEX_TYPE_ROUGHNESS);
			auto metal = loadMaterialTextures(aiMat, aiTextureType_AMBIENT, TEX_TYPE_METALLIC);

			if (textures.empty()) { // workaround for current blender export error - should be fixed when using own level format!
				auto matName = std::string(aiMat->GetName().C_Str());
				matName = matName.substr(0, matName.find_first_of('.'));
				if (textureFiles.find(matName) != textureFiles.end()) {
					auto texFile = rootDirectory + textureFiles[matName];
					bool isLoaded = false;
					for (const auto& tex : textureCache) {
						if (tex->path().compare(texFile) == 0) {
							textures.push_back(tex);
							isLoaded = true;
							break;
						}
					}

					if (!isLoaded) {
						auto tex = std::make_shared<Texture>(texFile, TEX_TYPE_DIFFUSE);
						textureCache.push_back(tex);
						textures.push_back(tex);
					}
					textures.push_back(textureCache[1]); // load default spec texture
				}
			}
			if (textures.size() == 0) {
				// load default textures
				textures.insert(textures.end(), textureCache.begin(), textureCache.begin() + 1);
			}
			if (spec.size() == 0) {
				// load default spec tex
				spec.insert(spec.end(), textureCache.begin() + 1, textureCache.begin() + 2);
			}
			textures.insert(textures.end(), spec.begin(), spec.end());
			textures.insert(textures.end(), norm.begin(), norm.end());
			textures.insert(textures.end(), rough.begin(), rough.end());
			textures.insert(textures.end(), metal.begin(), metal.end());

			float specular;
			if (aiGetMaterialFloat(aiMat, AI_MATKEY_REFLECTIVITY, &specular) != AI_SUCCESS) {
				specular = 0.5f;
			}

			material = std::make_shared<Material>(textures, specular);

		}
		
		auto nodeTransform = node->mTransformation;
		float tvals[16] = {
			nodeTransform.a1, nodeTransform.b1, nodeTransform.c1, nodeTransform.d1,
			nodeTransform.a2, nodeTransform.b2, nodeTransform.c2, nodeTransform.d2,
			nodeTransform.a3, nodeTransform.b3, nodeTransform.c3, nodeTransform.d3,
			nodeTransform.a4, nodeTransform.b4, nodeTransform.c4, nodeTransform.d4
		};

		glm::mat4 nodeModel = glm::make_mat4(tvals);

		auto mesh = std::make_shared<Mesh>(data, material, meshRoot, nodeModel);
		meshRoot->addChild(mesh);

		if (aiMeshData->mName.length > 0) {
			mesh->setName(std::string(aiMeshData->mName.C_Str()));
		}
	}
}

void Scene::loadFromFile(const std::string& fileName) {
	levelLoadFuture = std::async(std::launch::async, [this, fileName]() {
		auto uiHandle = App::getHandle().getRenderBackend()->getUiHandle();
		{
			std::lock_guard<std::mutex> lock(sceneMutex);
			importer.SetProgressHandler(uiHandle.get());
			scenePtr = importer.ReadFile(fileName,
				// aiProcess_GenNormals |
				// aiProcess_CalcTangentSpace | 
				aiProcess_JoinIdenticalVertices |
				aiProcess_Triangulate |
				// aiProcess_FlipUVs |
				aiProcess_FindDegenerates |
				aiProcess_SortByPType |
				// aiProcess_PreTransformVertices |
				0
			);
		}
		auto err = std::string(importer.GetErrorString());
		if (!err.empty()) {
			std::cout << __FUNCTION__ << ": ImportError: " << err << std::endl;
		}
		if (!scenePtr) {
			// todo error logging in gui!
			std::cout << "Unable to load scene from " + fileName << std::endl;
			return;
		}

		auto dirOffs = fileName.rfind('/');
		dirOffs = dirOffs == std::string::npos ? fileName.rfind('\\') : dirOffs;
		{
			std::lock_guard<std::mutex> lock(dirMutex);
			if (dirOffs != std::string::npos) {
				rootDirectory = fileName.substr(0, dirOffs) + "/";
			}
			else {
				rootDirectory = "assets/";
			}
			for (const auto& file : fs::directory_iterator(rootDirectory)) {
				#ifdef __WIN32
				if (file.is_directory()) continue;
				#elif __linux__
				if (fs::is_directory(file)) continue;
				#endif
				auto ext = file.path().extension();
				if (isImageExtension(ext.string())) {
					textureFiles[file.path().stem().string()] = (file.path().filename().string());
				}
			}
		}
		loaded = true;
		return;
	});
}

void Scene::processAssimp() {
	levelLoadFuture.get();
	{
		std::lock_guard<std::mutex> lock(sceneMutex);
		textureCache.push_back(std::make_shared<Texture>("assets/materials/default/diff.png", TEX_TYPE_DIFFUSE));
		textureCache.push_back(std::make_shared<Texture>("assets/materials/default/spec.png", TEX_TYPE_SPECULAR));
		processAINode(scenePtr->mRootNode, scenePtr, root);
		cacheDirty = true;
		importer.SetProgressHandler(nullptr); // important! Importer's destructor calls delete on the progress handler pointer!
		importer.FreeScene();
		App::getHandle().getRenderBackend()->getUiHandle()->Update();
	}
}