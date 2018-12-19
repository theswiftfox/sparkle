#pragma once

#include <map>
#include <vector>
#include <memory>

#include "Texture.h"

#include <vulkan/vulkan.h>

#define TEX_TYPE_DIFFUSE 0
#define TEX_TYPE_SPECULAR 1
#define TEX_TYPE_NORMAL 2

#define TEX_BINDING_OFFSET 0
#define BINDING_DIFFUSE 0
#define BINDING_SPECULAR 1
#define BINDING_NORMAL 2

namespace Engine {
	class Material {
	public:
		Material(std::vector<std::shared_ptr<Texture>> textures);

		struct MaterialUniforms {
			float diffuse;
			float specular;
		};

		MaterialUniforms uniforms;

		VkDescriptorSet getDescriptorSet() const { return pDescriptorSet; }

		void updateDescriptorSets();

	private:
		std::map<size_t, std::shared_ptr<Texture>> textures;

		VkDescriptorPool pDescriptorPool;
		VkDescriptorSetLayout pDescriptorSetLayout;
		VkDescriptorSet pDescriptorSet;
	};

}