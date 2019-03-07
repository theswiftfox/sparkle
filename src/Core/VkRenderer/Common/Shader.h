#ifndef SHADER_H
#define SHADER_H

#include <vulkan/vulkan.h>

#include "Geometry.h"
#include "VulkanExtension.h"
#include <string>
#include <vector>

#include "Lights.h"

namespace Sparkle {
class App;
namespace Shaders {

#define SPARKLE_SHADER_LIMIT_LIGHTS 9

	enum ShaderType {
		Vertex,
		TessellationControl,
		TessellationEvaluation,
		Geometry,
		Fragment,
		Compute
	};

	struct ShaderSource {
		ShaderType type;
		std::string filePath;
	};

	VkShaderModule createShaderModule(const std::vector<char>& code);

	class ShaderProgram {
	public:
		struct UniformBufferObject {
			glm::mat4 view;
			glm::mat4 projection;
		};

		struct InstancedUniformBufferObject {
			glm::mat4 model;
			glm::mat4 normal;
		};

		struct FragmentShaderUniforms {
			glm::vec4 cameraPos;
			uint32_t numLights;
			float exposure = 1.0f;
			float gamma = 2.2f;
			float _pad;
			Lights::Light lights[SPARKLE_SHADER_LIMIT_LIGHTS];
		};

		ShaderProgram(const std::vector<Shaders::ShaderSource>& shaderSources);

		void cleanup() const;

		void updateUniformBufferObject(const UniformBufferObject& ubo);
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

		size_t fragSettingsOffset {};

		size_t objectCount;
		uint32_t dUboAlignment {};

		InstancedUniformBufferObject* dynamicUboData;
		VkDeviceSize dynamicUboDataSize {};

		void createUniformBuffer();
		void createDynamicBuffer(VkDeviceSize size);
	};
} // namespace Shaders
} // namespace Sparkle
#endif // SHADER_H
