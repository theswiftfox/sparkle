#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include "VulkanExtension.h"

#include "Shader.h"

namespace Engine
{
	struct ComputePipeline {
		VkQueue queue;
		VkCommandPool pool;
		VkCommandBuffer cmdBuff;
		VkFence fence;
		VkSemaphore sem;
		VkDescriptorSetLayout descSetLayout;
		VkDescriptorSet descSet;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;
	};
}

#endif