#ifndef TEXTURE_H
#define TEXTURE_H

#include "VulkanExtension.h"

#include <string>

namespace Engine {
	class Texture {
	public:
		Texture(std::string filePath, size_t typeID);
		Texture(void* data, int width, int height, int channels, size_t typeID);
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

		const auto type() const { return typeID; }

	private:
		std::string filePath;

		size_t typeID;

		int width, height, channels;
		
		vkExt::SharedMemory* texMemory;
		vkExt::Image texImage;
		VkImageView texImageView;
		VkSampler texImageSampler;

		void initFromData(void* data, int width, int height, int channles);
	};
}

#endif // TEXTURE_H
