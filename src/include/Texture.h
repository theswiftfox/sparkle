#ifndef TEXTURE_H
#define TEXTURE_H

#include "VulkanExtension.h"

#include <string>

namespace Engine {
	class Texture {
	public:
		Texture(std::string filePath);
		~Texture();

		VkDescriptorImageInfo descriptor() const {
			const VkDescriptorImageInfo info = {
				texImageSampler,
				texImageView,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
			return info;
		}
		const auto path() const { return filePath; }

	private:
		std::string filePath;

		int width, height, channels;
		
		vkExt::SharedMemory* texMemory;
		vkExt::Image texImage;
		VkImageView texImageView;
		VkSampler texImageSampler;
	};
}

#endif // TEXTURE_H
