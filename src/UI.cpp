#include <UI.h>

using namespace Engine;

GUI::~GUI() {
	ImGui::DestroyContext();

	vertexBuffer.destroy();
	indexBuffer.destroy();

	fontImage.destroy();
	vkDestroyImageView(device, fontImageView, nullptr);
	fontMemory->free(device);
	vkDestroySampler(device, sampler, nullptr);
	vkDestroyPipelineCache(device, pipelineCache, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

void GUI::init(float width, float height) {
	ImGuiStyle& style = ImGui::GetStyle();
	// set style?

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(width, height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

void GUI::initResources() {
	ImGuiIO& io = ImGui::GetIO();

	unsigned char* fontData;
	int widthTx, heightTx;

	io.Fonts->GetTexDataAsRGBA32(&fontData, &widthTx, &heightTx);
	VkDeviceSize fontSize = widthTx * heightTx * 4 * sizeof(char);

	auto& app = App::getHandle();

	app.createImage2D(widthTx, heightTx, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, fontImage, fontMemory);
	fontImageView = app.createImageView2D(fontImage.image, fontImage.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

	vkExt::Buffer staging;
	vkExt::SharedMemory* stagingMem;
	app.createBuffer(fontSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging, stagingMem);

	staging.map();
	staging.copyTo(fontData, fontSize);
	staging.unmap();

	auto cmdCopy = app.beginOneTimeCommand();
	app.transitionImageLayout(fontImage.image, fontImage.imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdCopy);
	
	VkBufferImageCopy copyRegion = {};
	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageExtent = { fontImage.width, fontImage.height, 1 };

	vkCmdCopyBufferToImage(cmdCopy, staging.buffer, fontImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
	app.transitionImageLayout(fontImage.image, fontImage.imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	app.endOneTimeCommand(cmdCopy);
	staging.destroy(true);

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.pNext = nullptr;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	VK_THROW_ON_ERROR(vkCreateSampler(device, &samplerInfo, nullptr, &sampler), "Sampler creation for UI failed!");

	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.maxSets = 2;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = poolSizes.data();

	VK_THROW_ON_ERROR(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool), "Error creating UI descriptor pool");
	
	std::array<VkDescriptorSetLayoutBinding, 1> bindings = {
		{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &sampler }
	};

	VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo = {};
	descSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetLayoutCreateInfo.bindingCount = bindings.size();
	descSetLayoutCreateInfo.pBindings = bindings.data();

	VK_THROW_ON_ERROR(vkCreateDescriptorSetLayout(device, &descSetLayoutCreateInfo, nullptr, &descriptorSetLayout), "Error creating descriptor set for UI");

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	VK_THROW_ON_ERROR(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet), "Error allocating descriptor set for UI");

	std::array<VkWriteDescriptorSet, 1> writeSets = {
		{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSet, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  }
	};

	VkPipelineCacheCreateInfo pipeCacheCreateInfo = {};
	pipeCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	
	VK_THROW_ON_ERROR(vkCreatePipelineCache(device, &pipeCacheCreateInfo, nullptr, &pipelineCache), "Error creating pipeline cache for UI");


}