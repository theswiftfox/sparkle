#include "Geometry.h"
#include "Application.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <glm/gtx/euler_angles.hpp>

#include <string>
#include <unordered_map>

using namespace Engine;
using namespace Geometry;

Mesh::Mesh(std::string id, std::string fromObj, std::string textureOrMtlDir, ColorType colorType /* = ColorType::VTC */) : ID(id) {
	tinyobj::attrib_t attribute;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	model = glm::mat4(1.0f);

	if (colorType == ColorType::MTL) {
		if (!tinyobj::LoadObj(&attribute, &shapes, &materials, &err, fromObj.c_str(), textureOrMtlDir.c_str())) {
			throw std::runtime_error(err);
		}
	}
	else {
		if (!tinyobj::LoadObj(&attribute, &shapes, &materials, &err, fromObj.c_str())) {
			throw std::runtime_error(err);
		}
		if (colorType == ColorType::TEX) {
			pSampler = std::make_unique<Engine::Texture>(textureOrMtlDir);
			status.useTexture = VK_TRUE;
		}
	}

	float minZ = 0.0f;
	float maxZ = 0.0f;

	std::unordered_map<Vertex, uint32_t> uniqueVertices;
	bool calcNormals = false;

	for (const auto& shape : shapes) {
		size_t face = 0;
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};

			vertex.position = glm::vec3(
				attribute.vertices[3 * index.vertex_index + 0],
				attribute.vertices[3 * index.vertex_index + 1],
				attribute.vertices[3 * index.vertex_index + 2]
			);
			if (vertex.position.z < minZ) {
				minZ = vertex.position.z;
			}
			if (vertex.position.z > maxZ) {
				maxZ = vertex.position.z;
			}
			if (!attribute.normals.empty()) {
				vertex.normal = glm::vec3(
					attribute.normals[3 * index.normal_index + 0],
					attribute.normals[3 * index.normal_index + 1],
					attribute.normals[3 * index.normal_index + 2]
				);
			}			
			else {
				calcNormals = calcNormals ? calcNormals : true;
				vertex.normal = glm::vec3(0.0f);
			}
			
			const auto idx = shape.mesh.material_ids[face / 3];

			switch (colorType) {
			case ColorType::MTL:
				if (idx != -1) {
					const auto mtl = materials[idx];
					vertex.baseColor = glm::vec4(mtl.diffuse[0], mtl.diffuse[1], mtl.diffuse[2], mtl.dissolve);
				}
				else {
					std::cout << "Warn: couldn't load material for obj " << fromObj << std::endl;
					vertex.baseColor = glm::vec4(0.0);
				}
				vertex.texCoord = glm::vec2(0.0f);
				break;
			case ColorType::TEX:
				vertex.baseColor = glm::vec4(0.0f);
				if (attribute.texcoords.empty()) {
					throw std::runtime_error("Object loaded with ColorType::TEX but no obj contains no texture coordinates!");
				}
				vertex.texCoord = glm::vec2(
					attribute.texcoords[2 * index.texcoord_index + 0],
					1.0f - attribute.texcoords[2 * index.texcoord_index + 1]
				);
				break;
			case ColorType::VTC:
			default:
				vertex.baseColor = glm::vec4(
					attribute.colors[3 * index.vertex_index + 0],
					attribute.colors[3 * index.vertex_index + 1],
					attribute.colors[3 * index.vertex_index + 2],
					1.0f
				);
				vertex.texCoord = glm::vec2(0.0f);
				break;
			}

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);

			++face;
		}
	}

	height = maxZ - minZ;

	if (calcNormals) {
		glm::vec3 v0, v1, v2;
		for (auto i = 0; i < indices.size(); ++i) {
			switch (i % 3) {
			case 0:
				v0 = vertices[indices[i]].position;
				break;
			case 1:
				v1 = vertices[indices[i]].position;
				break;
			case 2:
				v2 = vertices[indices[i]].position;
				glm::vec3 n = glm::normalize(glm::cross((v1 - v0), (v2 - v0)));
				vertices[indices[i - 2]].normal = n;
				vertices[indices[i - 1]].normal = n;
				vertices[indices[i]].normal = n;
				break;
			}
		}
	}

	meshFromVertsAndIndices(vertices, indices);
}

Mesh::Mesh(std::vector<Vertex> verts, std::vector<uint32_t> indices) {
	meshFromVertsAndIndices(verts, indices);
}

void Mesh::createDescriptorPool() {
	VkDescriptorPoolSize size = {
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1
	};

	VkDescriptorPoolCreateInfo info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		0,
		1,
		1,
		&size
	};

	if (vkCreateDescriptorPool(App::getHandle().getRenderBackend()->getDevice(), &info, nullptr, &pDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Mesh:: DescriptorPool creation failed!");
	}
}

void Mesh::createDescriptorSetLayout() {
	pDescriptorSetLayout = App::getHandle().getRenderBackend()->getSamplerDescriptorSetLayout();
}

void Mesh::createDescriptorSet() {
	auto device = App::getHandle().getRenderBackend()->getDevice();
	VkDescriptorSetLayout layouts[] = { pDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		pDescriptorPool,
		1,
		layouts
	};

	if (vkAllocateDescriptorSets(device, &allocInfo, &pDescriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("DescriptorSet allocation failed!");
	}

	if (pSampler) {
		auto descriptor = pSampler->descriptor();
		VkWriteDescriptorSet sampler = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			pDescriptorSet,
			0,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&descriptor,
			nullptr,
			nullptr
		};

		vkUpdateDescriptorSets(device, 1, &sampler, 0, nullptr);
	}
}

void Mesh::translate(glm::vec3 pos) {
	model = glm::translate(model, pos);
}

// set translation while keeping rotation
void Mesh::setTranslation(glm::vec3 pos) {
	model[3] = glm::vec4(pos, 1.0f);
}

glm::vec3 Mesh::getPosition() {
	return glm::vec3(model[3]);
}
glm::vec3 Mesh::getRotation() {
	auto yaw = 0.0f;
	auto pitch = 0.0f;
	auto roll = 0.0f;

	pitch = asinf(-model[1][2]);
	if (cosf(pitch) > 0.0001f) {
		yaw = atan2f(model[0][2], model[2][2]);
		roll = atan2f(model[1][0], model[1][1]);
	}
	else {
		yaw = 0.0f;
		roll = atan2f(-model[0][1], model[0][0]);
	}

	return glm::vec3(pitch, -yaw, -roll);
}
glm::vec3 Mesh::getScale() {
	const auto scaleX = glm::length(glm::vec3(model[0]));
	const auto scaleY = glm::length(glm::vec3(model[1]));
	const auto scaleZ = glm::length(glm::vec3(model[2]));
	return glm::vec3(scaleX, scaleY, scaleZ);
}

void Mesh::setScale(glm::vec3 scaleVec) {
	if (scaleVec.x < 0.00001f || scaleVec.y < 0.00001f || scaleVec.z < 0.00001f) {
		return;
	}
	const auto scale = getScale();
	const auto inverseScale = 1.0f / scale;
	
	model = glm::scale(model, inverseScale);
	model = glm::scale(model, scaleVec);
}

void Mesh::rotateTo(glm::vec3 targetPoint) {
	const glm::vec3 pos = glm::vec3(model[3]);
	model = glm::inverse(glm::lookAt(targetPoint, pos, glm::vec3(1.0f, 0.0f, 0.0f)));
}

void Mesh::scale(glm::vec3 scale) {
	model = glm::scale(model, scale);
}

void Mesh::scaleToSize(float h) {
	const float scaleFac = h / height;
	scale(glm::vec3(scaleFac, 1.0f, 1.0f));
}

void Mesh::setRotation(glm::vec3 r) {
	const auto scale = getScale();
	const auto pos = getPosition();

	model = glm::eulerAngleYXZ(r.y, r.x, r.z);
	model[3] = glm::vec4(pos, 1.0f);
	model = glm::scale(model, scale);
}

void Geometry::Mesh::meshFromVertsAndIndices(std::vector<Vertex> verts, std::vector<uint32_t> inds)
{
	vertices = verts;
	indices = inds;

	createDescriptorPool();
	createDescriptorSetLayout();
	createDescriptorSet();

	bufferOffset = App::getHandle().storeMesh(this);
}

void Engine::Geometry::Scene::storeMesh(std::shared_ptr<Mesh> m)
{
	meshes.push_back(m);
}

void Engine::Geometry::Scene::cleanup() {
	for (auto& mesh : meshes) {
		mesh.reset();
	}
	meshes.clear();
}
