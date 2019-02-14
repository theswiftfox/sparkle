#ifndef TEXTURE_H
#define TEXTURE_H

#include "VulkanExtension.h"

#include <assimp/texture.h>
#include <string>

namespace Engine {
class Texture {
public:
    Texture(std::string filePath, size_t typeID);
    Texture(const aiTexture* tex, size_t typeID, std::string id = "");
    Texture(void* data, int width, int height, int channels, size_t typeID, std::string id = "");
    void cleanup();

    VkDescriptorImageInfo descriptor() const
    {
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

    void initFromData(void* data, int width, int height, int channles, VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM, int mipLevels = -1);
    void initFromData(void* data, VkDeviceSize size, VkFormat imageFormat, int m);
};
}

#endif // TEXTURE_H
