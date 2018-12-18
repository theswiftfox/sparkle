#pragma once

#include <vector>
#include <memory>

#include "Texture.h"

#include <vulkan/vulkan.h>

#define TEX_TYPE_DIFFUSE "diffuseTex"
#define TEX_TYPE_SPECULAR "specularTex"

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
		std::vector<std::shared_ptr<Texture>> textures;

		VkDescriptorPool pDescriptorPool;
		VkDescriptorSetLayout pDescriptorSetLayout;
		VkDescriptorSet pDescriptorSet;
	};

}