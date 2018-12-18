#include "Material.h"

#include "Application.h"

Engine::Material::Material(std::vector<std::shared_ptr<Texture>> textures) : textures(textures) {
	auto device = App::getHandle().getRenderBackend()->getDevice();

	VkDescriptorPoolSize size = {
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		static_cast<uint32_t>(textures.size())
	};

	VkDescriptorPoolCreateInfo info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		0,
		1,
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

	if (vkAllocateDescriptorSets(device, &allocInfo, &pDescriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("DescriptorSet allocation failed!");
	}

	updateDescriptorSets();
}

void Engine::Material::updateDescriptorSets()
{
	std::vector<VkWriteDescriptorSet> writes;
	size_t texBinding = 0;
	for (const auto& tex : textures) {
		auto descriptor = tex->descriptor();
		VkWriteDescriptorSet sampler = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			pDescriptorSet,
			texBinding,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&descriptor,
			nullptr,
			nullptr
		};

		writes.push_back(sampler);

		++texBinding;
	}
	
	if (writes.size() > 0) {
		if (writes.size() > App::getHandle().getRenderBackend()->getMaterialTextureLimit()) {
			throw std::runtime_error("Trying to write more samplers than supported by the pipeline");
		}
		vkUpdateDescriptorSets(App::getHandle().getRenderBackend()->getDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}
}
