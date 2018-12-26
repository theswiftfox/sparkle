#pragma once

#include <map>
#include <vector>
#include <memory>

#include "Texture.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#define TEX_TYPE_DIFFUSE 0
#define TEX_TYPE_SPECULAR 1
#define TEX_TYPE_NORMAL 2

#define TEX_BINDING_OFFSET 0
#define BINDING_DIFFUSE 0
#define BINDING_SPECULAR 1
#define BINDING_NORMAL 2

namespace Engine {
	enum MaterialFeatureFlags {
		SPARKLE_MAT_NORMAL_MAP = 0x010,
		SPARKLE_MAT_PBR = 0x100
	};
	class Material {
	public:
		typedef uint32_t MaterialFeatures;
		struct MaterialUniforms {
			MaterialFeatures features;
			float specular;
			float roughness;
			float metallic;
		};

		Material(std::vector<std::shared_ptr<Texture>> textures, float specular);
		Material(std::vector<std::shared_ptr<Texture>> textures, float specular, float roughness, float metallic);


		MaterialUniforms getUniforms() const;

		VkDescriptorSet getDescriptorSet() const { return pDescriptorSet; }

		void updateDescriptorSets();

	private:
		std::map<size_t, std::shared_ptr<Texture>> textures;

		VkDescriptorPool pDescriptorPool;
		VkDescriptorSetLayout pDescriptorSetLayout;
		VkDescriptorSet pDescriptorSet;

		MaterialUniforms uniforms;

		void initTextureMaterial(std::vector<std::shared_ptr<Texture>> textures);
	};

}