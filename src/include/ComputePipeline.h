#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include "VulkanExtension.h"

namespace Engine
{
	class ComputePipeline {



	private:
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