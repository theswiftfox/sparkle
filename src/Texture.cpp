#include "Texture.h"
#include "Application.h"

#include "FileReader.h"
#include "stb_image.h"

using namespace Engine;

Texture::Texture(std::string filePath, size_t typeID)
    : filePath(filePath)
    , typeID(typeID)
{
    auto tex = Tools::FileReader::loadImage(filePath);
    if (!tex.imageData) {
        throw std::runtime_error("Unable to load texture from: " + filePath);
    }
    if (tex.imageFileType == Tools::FileReader::ImageType::SPARKLE_IMAGE_DDS) {
        width = tex.width;
        height = tex.height;
        channels = 4; /* tex.channels; */
        initFromData(tex.imageData, static_cast<VkDeviceSize>(tex.size), VK_FORMAT_BC2_UNORM_BLOCK, -1);
    } else {
        initFromData(tex.imageData, tex.width, tex.height, 4 /*channels*/, VK_FORMAT_R8G8B8A8_UNORM);
    }

    tex.free();
}

Engine::Texture::Texture(const aiTexture* tex, size_t typeID, std::string id)
    : filePath(id)
    , typeID(typeID)
{
    const auto len = tex->mHeight == 0 ? tex->mWidth : tex->mWidth * tex->mHeight;
    int w, h, c;
    auto stbiTex = stbi_load_from_memory(reinterpret_cast<unsigned char*>(tex->pcData), len, &w, &h, &c, 4); // force 4 components
    initFromData(stbiTex, w, h, 4 /*c*/);
}

Texture::Texture(void* data, int width, int height, int channels, size_t typeID, std::string id)
    : filePath(id)
    , typeID(typeID)
{
    initFromData(data, width, height, channels);
}

void Texture::initFromData(void* data, int w, int h, int c, VkFormat imageFormat, int m)
{
    width = w;
    height = h;
    channels = c;

    VkDeviceSize texSize = width * height * channels; // RGBA
    if (texSize == 0)
        return;

    initFromData(data, texSize, imageFormat, m);
}

void Texture::initFromData(void* data, VkDeviceSize s, VkFormat format, int m)
{
    auto context = App::getHandle().getRenderBackend();

    vkExt::Buffer staging;
    vkExt::SharedMemory* stagingMem = new vkExt::SharedMemory();

    context->createBuffer(s, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging, stagingMem);

    staging.map();
    staging.copyTo(data, static_cast<size_t>(s));
    staging.unmap();

    texMemory = new vkExt::SharedMemory();
    context->createImage2D(width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texImage, texMemory);
    context->transitionImageLayout(texImage.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    if (m <= -1) {
        staging.copyBufferToImage(context->getCommandPool(), context->getDefaultQueue(), texImage.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    } else {
        std::vector<VkBufferImageCopy> copyRegions;
        for (uint32_t i = 0; i < static_cast<uint32_t>(m); ++i) {
            VkBufferImageCopy region {};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = i;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            // region.imageExtent.width =
            // FIXME!
        }
    }
    context->transitionImageLayout(texImage.image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    staging.destroy(true);
    delete (stagingMem);

    texImageView = context->createImageView2D(texImage.image, format, VK_IMAGE_ASPECT_COLOR_BIT);

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
void Texture::cleanup()
{
    auto context = App::getHandle().getRenderBackend();
    vkDestroySampler(context->getDevice(), texImageSampler, nullptr);
    texMemory->free(context->getDevice());
}