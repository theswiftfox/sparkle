#ifndef SHADER_H
#define SHADER_H

#include <vulkan/vulkan.h>

#include "VulkanExtension.h"
#include "Geometry.h"

#include <string>
#include <vector>

namespace Engine {
	namespace Shaders {

		class App;

		typedef struct UniformBufferObject {
			glm::mat4 view;
			glm::mat4 projection;
		} UBO;

		typedef struct InstancedUniformBufferObject {
			glm::mat4* model;
		} InstancedUBO;

		typedef struct TesselationControlSettings {

		} TesselationControlSettings;

		typedef struct FragmentShaderSettings {
			glm::vec4 lightDirection;
			glm::vec4 lightColor;
			float ambient;
			float diffuse;
			float shininess;
		} FragmentShaderSettings;

		class ShaderProgram {
		protected:
			static VkShaderModule createShaderModule(const std::vector<char>& code);
		
		public:
			ShaderProgram(const std::string& vtxShaderFile, const std::string& tescShaderFile, const std::string& teseShaderFile, const std::string& fragShaderFile, size_t objectInstances);

			void cleanup() const;

			void updateUniformBufferObject(const UBO& ubo);
			void updateDynamicUniformBufferObject(const std::vector<std::shared_ptr<Geometry::Node>>& meshes);
			void updateTesselationControlSettings(const TesselationControlSettings& sets);
			void updateFragmentShaderSettings(const FragmentShaderSettings& sets);

			auto getDynamicAlignment() const { return dynamicAlignment; }

			std::vector<VkPipelineShaderStageCreateInfo> getShaderStages() const;
			auto tesselationEnabled() const
			{
				return (tessControlShader != nullptr && tessEvalShader != nullptr);
			}

			std::array<VkDescriptorBufferInfo, 3> getDescriptorInfos() const;

			vkExt::Buffer pUniformBuffer;
			vkExt::Buffer pDynamicBuffer;

		private:
			VkShaderModule vertexShader = nullptr;
			VkShaderModule tessControlShader = nullptr;
			VkShaderModule tessEvalShader = nullptr;
			VkShaderModule fragmentShader = nullptr;

			vkExt::SharedMemory* pUniformBufferMemory;
			vkExt::SharedMemory* pDynamicBufferMemory;
			vkExt::SharedMemory* pDynamicVertexTypeMemory;

			size_t teseSettingsOffset{};
			size_t fragSettingsOffset{};

			size_t objectCount;
			uint32_t dynamicAlignment{};

			InstancedUBO dynamicUboData{};
			VkDeviceSize dynamicUboDataSize{};

			void createUniformBuffer();
			void createDynamicBuffer(VkDeviceSize size);

		};
	}
}
#endif // SHADER_H
