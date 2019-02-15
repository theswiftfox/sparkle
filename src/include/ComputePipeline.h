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

    VkQueue queue;
    VkCommandPool cmdPool;
    VkCommandBuffer cmdBuff;
    VkFence fence;
    VkSemaphore sem;
    VkDescriptorPool descPool;
    VkDescriptorSetLayout descSetLayout;
    VkDescriptorSet descSet;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    void initialize(uint32_t queueIndex);
    void cleanup();
};
}

#endif