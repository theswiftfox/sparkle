#pragma once

#include <map>
#include <memory>
#include <vector>

#include "Texture.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#define TEX_BINDING_OFFSET 0
#define BINDING_DIFFUSE 0
#define BINDING_SPECULAR 1
#define BINDING_NORMAL 2
#define BINDING_ROUGHNESS 3
#define BINDING_METALLIC 4

namespace Sparkle {
enum MaterialFeatureFlags {
    SPARKLE_MAT_NORMAL_MAP = 0x010,
    SPARKLE_MAT_PBR = 0x100,
    SPARKLE_MAT_METALLIC_ROUGHNESS = 0x001
};
class Material {
public:
    typedef uint32_t MaterialFeatures;
    struct MaterialUniforms {
        MaterialFeatures features;
    };

    Material(std::vector<std::shared_ptr<Texture>> textures, bool postponeInit = false);
    Material(std::vector<std::shared_ptr<Texture>> textures, float roughness, float metallic);

    MaterialUniforms getUniforms() const;

    VkDescriptorSet getDescriptorSet() const { return pDescriptorSet; }

    void init();

    void updateDescriptorSets();

    void cleanup();

private:
    std::map<size_t, std::shared_ptr<Texture>> textures;

    VkDescriptorPool pDescriptorPool;
    VkDescriptorSetLayout pDescriptorSetLayout;
    VkDescriptorSet pDescriptorSet;

    MaterialUniforms uniforms;

    bool initialized = false;

    void initTextureMaterial(std::vector<std::shared_ptr<Texture>> textures, bool postponeInit = false);
};

}