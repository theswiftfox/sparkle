#ifndef SHADER_H
#define SHADER_H

#include <vulkan/vulkan.h>

#include "VulkanExtension.h"
#include "Geometry.h"
#include <string>
#include <vector>

#include "Lights.h"

namespace Engine {
	namespace Shaders {

#define SPARKLE_SHADER_LIMIT_LIGHTS 9

		class App;

		typedef struct UniformBufferObject {
			glm::mat4 view;
			glm::mat4 projection;
		} UBO;

		typedef struct InstancedUniformBufferObject {
			glm::mat4* model;
		} InstancedUBO;

		typedef struct FragmentShaderUniforms {
			glm::vec4 cameraPos;
			uint32_t numLights;
			float exposure = 1.0f;
			float gamma = 2.2f;
			float _pad;
			Lights::Light lights[SPARKLE_SHADER_LIMIT_LIGHTS];
		} FragmentShaderUniforms;

		class ShaderProgram {		
		public:
			ShaderProgram(const std::string& vtxShaderFile, const std::string& tescShaderFile, const std::string& teseShaderFile, const std::string& fragShaderFile, size_t objectInstances);

			static VkShaderModule createShaderModule(const std::vector<char>& code);

			void cleanup() const;

			void updateUniformBufferObject(const UBO& ubo);
			void updateDynamicUniformBufferObject(const std::vector<std::shared_ptr<Geometry::Node>>& meshes);
			void updateFragmentShaderUniforms(const FragmentShaderUniforms& sets);

			auto getDynamicAlignment() const { return dUboAlignment; }

			std::vector<VkPipelineShaderStageCreateInfo> getShaderStages() const;
			auto tesselationEnabled() const
			{
				return (tessControlShader != nullptr && tessEvalShader != nullptr);
			}

			std::array<VkDescriptorBufferInfo, 2> getDescriptorInfos() const;

			vkExt::Buffer pUniformBuffer;
			vkExt::Buffer pDynamicBuffer;

			bool dynamicBufferDirty = true;

		private:
			VkShaderModule vertexShader = nullptr;
			VkShaderModule tessControlShader = nullptr;
			VkShaderModule tessEvalShader = nullptr;
			VkShaderModule fragmentShader = nullptr;

			vkExt::SharedMemory* pUniformBufferMemory;
			vkExt::SharedMemory* pDynamicBufferMemory;

			size_t fragSettingsOffset{};

			size_t objectCount;
			uint32_t dUboAlignment{};

			InstancedUBO dynamicUboData{};
			VkDeviceSize dynamicUboDataSize{};

			void createUniformBuffer();
			void createDynamicBuffer(VkDeviceSize size);

		};
	}
}
#endif // SHADER_H
