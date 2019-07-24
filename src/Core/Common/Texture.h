#ifndef TEXTURE_H
#define TEXTURE_H

#include "VulkanExtension.h"

#include <assimp/texture.h>
#include <string>

#define TEX_TYPE_NONE 0x0
#define TEX_TYPE_DIFFUSE 0x1
#define TEX_TYPE_SPECULAR 0x2
#define TEX_TYPE_NORMAL 0x3
#define TEX_TYPE_ROUGHNESS 0x4
#define TEX_TYPE_METALLIC 0x5
#define TEX_TYPE_METALLIC_ROUGHNESS 0x6
#define TEX_TYPE_PLACEHOLDER 0x7

namespace Sparkle {
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

    void changeType(size_t newType) { typeID = newType; }

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
