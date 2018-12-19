#include "Texture.h"
#include "Application.h"

#include "FileReader.h"

using namespace Engine;

Texture::Texture(std::string filePath) : filePath(filePath) {
	auto& context = App::getHandle().getRenderBackend();
	auto tex = Tools::FileReader::loadImage(filePath);
	if (!tex.imageData) {
		throw std::runtime_error("Unable to load texture from: " + filePath);
	}
	width = tex.width;
	height = tex.height;
	channels = tex.channels;

	VkDeviceSize texSize = width * height * 4; // RGBA

	vkExt::Buffer staging;
	vkExt::SharedMemory* stagingMem = new vkExt::SharedMemory();

	context->createBuffer(texSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging, stagingMem);

	staging.map();
	staging.copyTo(tex.imageData, static_cast<size_t>(texSize));
	staging.unmap();

	tex.free();

	texMemory = new vkExt::SharedMemory();
	context->createImage2D(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texImage, texMemory);
	context->transitionImageLayout(texImage.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	staging.copyBufferToImage(context->getCommandPool(), context->getDefaultQueue(), texImage.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	context->transitionImageLayout(texImage.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	staging.destroy(true);
	delete(stagingMem);

	texImageView = context->createImageView2D(texImage.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	
	VkSamplerCreateInfo samplerInfo = {
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		nullptr,
		0,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		0.0f,
		VK_TRUE,
		16.0f,
		VK_FALSE,
		VK_COMPARE_OP_ALWAYS,
		0.0f,
		0.0f,
		VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		VK_FALSE
	};
	if (vkCreateSampler(context->getDevice(), &samplerInfo, nullptr, &texImageSampler) != VK_SUCCESS) {
		throw std::runtime_error("Sampler creation for texture failed!");
	}

}
Texture::~Texture() {
	auto& context = App::getHandle().getRenderBackend();
	vkDestroySampler(context->getDevice(), texImageSampler, nullptr);
}