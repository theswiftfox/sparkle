#include "Material.h"

#include "Application.h"


Engine::Material::Material(std::vector<std::shared_ptr<Texture>> textures, float specular) {
	uniforms.features = 0x0;
	uniforms.specular = specular;
	uniforms.roughness = 0.0f;
	uniforms.metallic = 0.0f;
	initTextureMaterial(textures);
}

Engine::Material::Material(std::vector<std::shared_ptr<Texture>> textures, float specular, float roughness, float metallic) {
	uniforms.roughness = roughness;
	uniforms.metallic = metallic;
	uniforms.specular = specular;
	uniforms.features = SPARKLE_MAT_PBR;
	initTextureMaterial(textures);
}

Engine::Material::MaterialUniforms Engine::Material::getUniforms() const {
	return uniforms;
}


void Engine::Material::initTextureMaterial(std::vector<std::shared_ptr<Texture>> textures) {
	for (const auto& tex : textures) {
		this->textures[tex->type()] = tex; // todo: maybe use a vec to allow multiple of the same type
	}

	auto device = App::getHandle().getRenderBackend()->getDevice();
	auto texLimit = static_cast<uint32_t>(App::getHandle().getRenderBackend()->getMaterialTextureLimit());

	VkDescriptorPoolSize size = {
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		texLimit
	};

	VkDescriptorPoolCreateInfo info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		0,
		texLimit,
		1,
		&size
	};

	if (vkCreateDescriptorPool(device, &info, nullptr, &pDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Mesh:: DescriptorPool creation failed!");
	}

	pDescriptorSetLayout = App::getHandle().getRenderBackend()->getMaterialDescriptorSetLayout();

	VkDescriptorSetLayout layouts[] = { pDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		pDescriptorPool,
		1,
		layouts
	};

	VK_THROW_ON_ERROR(vkAllocateDescriptorSets(device, &allocInfo, &pDescriptorSet), "DescriptorSet allocation failed!");

	updateDescriptorSets();
}

void Engine::Material::updateDescriptorSets()
{
	std::vector<VkWriteDescriptorSet> writes;

	if (textures.find(TEX_TYPE_DIFFUSE) != textures.end()) {
		auto descriptor = textures[TEX_TYPE_DIFFUSE]->descriptor();
		VkWriteDescriptorSet write = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			pDescriptorSet,
			TEX_BINDING_OFFSET + BINDING_DIFFUSE,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&descriptor,
			nullptr,
			nullptr
		};
		writes.push_back(write);
	}
	if (textures.find(TEX_TYPE_SPECULAR) != textures.end()) {
		auto descriptor = textures[TEX_TYPE_SPECULAR]->descriptor();
		VkWriteDescriptorSet write = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			pDescriptorSet,
			TEX_BINDING_OFFSET + BINDING_SPECULAR,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&descriptor,
			nullptr,
			nullptr
		};
		writes.push_back(write);
	}
	if (textures.find(TEX_TYPE_NORMAL) != textures.end()) {
		auto descriptor = textures[TEX_TYPE_NORMAL]->descriptor();
		VkWriteDescriptorSet write = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			pDescriptorSet,
			TEX_BINDING_OFFSET + BINDING_NORMAL,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&descriptor,
			nullptr,
			nullptr
		};
		writes.push_back(write);
		uniforms.features |= SPARKLE_MAT_NORMAL_MAP;
	}

	if (writes.size() > 0) {
		if (writes.size() > App::getHandle().getRenderBackend()->getMaterialTextureLimit()) {
			throw std::runtime_error("Trying to write more samplers than supported by the pipeline");
		}
		vkUpdateDescriptorSets(App::getHandle().getRenderBackend()->getDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}
}