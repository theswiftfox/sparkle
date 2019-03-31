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

#define SPARKLE_SHADER_LIMIT_LIGHTS 1000

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

	struct ShaderProgramBase {
		VkShaderModule vtxModule = nullptr;	 // vertex shader
		VkShaderModule tescModule = nullptr; // tessellation control shader
		VkShaderModule teseModule = nullptr; // tessellation evaluation shader
		VkShaderModule geomModule = nullptr; // geometry shader
		VkShaderModule fragModule = nullptr; // fragment shader

		std::vector<VkPipelineShaderStageCreateInfo> getShaderStages() const;
		void cleanup();
	};

	class MRTShaderProgram {
	public:
		struct UniformBufferObject {
			glm::mat4 view;
			glm::mat4 projection;
		};

		struct InstancedUniformBufferObject {
			glm::mat4 model;
			glm::mat4 normal;
		};

		MRTShaderProgram(const std::vector<Shaders::ShaderSource>& shaderSources, size_t bufferCount);

		void cleanup();

		void updateUniformBufferObject(const UniformBufferObject& ubo, size_t index);
		void updateDynamicUniformBufferObject(const std::vector<std::shared_ptr<Geometry::Node>>& meshes);

		auto getDynamicAlignment() const { return dUboAlignment; }

		std::vector<VkPipelineShaderStageCreateInfo> getShaderStages() const;

		VkDescriptorBufferInfo getDescriptorInfos(size_t index) const;

		std::vector<vkExt::Buffer> uniformBuffers;
		vkExt::Buffer dynamicBuffer;

		bool dynamicBufferDirty = true;

	private:
		ShaderProgramBase shaderModules;

		std::vector<vkExt::SharedMemory*> uniformBufferMemory;
		vkExt::SharedMemory* dynamicBufferMemory;

		size_t objectCount;
		uint32_t dUboAlignment {};

		InstancedUniformBufferObject* dynamicUboData;
		VkDeviceSize dynamicUboDataSize {};

		void createUniformBuffer();
		void createDynamicBuffer(VkDeviceSize size);
	};

	class DeferredShaderProgram {
	public:
		struct FragmentShaderUniforms {
			glm::vec4 cameraPos;
			uint32_t numLights;
			float exposure = 1.0f;
			float gamma = 2.2f;
			float _pad;
			Lights::Light lights[SPARKLE_SHADER_LIMIT_LIGHTS];
		};

		DeferredShaderProgram(const std::vector<Shaders::ShaderSource>& shaderSources, size_t bufferCount);
		void cleanup();

		void updateFragmentShaderUniforms(const FragmentShaderUniforms& ubo, size_t index);

		std::vector<VkPipelineShaderStageCreateInfo> getShaderStages() const;
		VkDescriptorBufferInfo getDescriptorInfos(size_t index) const;

		std::vector<vkExt::Buffer> uniformBuffers;

	private:
		ShaderProgramBase shaderModules;

		std::vector<vkExt::SharedMemory*> uniformBufferMemory;

		void createUniformBuffer();
	};
} // namespace Shaders
} // namespace Sparkle
#endif // SHADER_H
