#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include "VulkanExtension.h"

#include "Shader.h"

namespace Sparkle {
struct ComputePipeline {
	struct UBO {
		glm::mat4 view;
		glm::mat4 proj;
	} ubo;

	struct MeshData {
		glm::mat4 model;
		uint32_t firstIndex;
		uint32_t indexCount;
	};

	VkQueue queue;
	VkCommandPool cmdPool;
	std::vector<VkCommandBuffer> cmdBuffers;
	VkFence fence;
	VkSemaphore sem;
	VkDescriptorPool descPool;
	VkDescriptorSetLayout descSetLayout;
	VkDescriptorSet descSet;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	vkExt::Buffer uboBuff;
	vkExt::SharedMemory* uboMem;

	VkShaderModule shader;

	void initialize(uint32_t queueIndex, uint32_t cmdBuffCount);
	void cleanup();
};
} // namespace Sparkle

#endif