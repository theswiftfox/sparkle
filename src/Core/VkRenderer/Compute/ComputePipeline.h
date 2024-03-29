#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include "VulkanExtension.h"
#include "SparkleTypes.h"

#include "Shader.h"

namespace Sparkle {
struct ComputePipeline {
	struct UBO {
		glm::vec4 frustumPlanes[6];
		glm::vec3 cameraPos;
		uint32_t meshCount;
	} ubo;

	struct MeshData {
		glm::mat4 model;
		Geometry::BoundingSphere boundingSphere;
		uint32_t firstIndex;
		uint32_t indexCount;
		glm::vec2 pad;
	};

	VkQueue queue;
	VkCommandPool cmdPool;
	std::vector<VkCommandBuffer> cmdBuffers;
	std::vector<VkFence> fences;
	std::vector<VkSemaphore> semaphores;
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

	void updateUBO(const UBO& ubo);
};
} // namespace Sparkle

#endif