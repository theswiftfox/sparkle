#ifndef UI_H
#define UI_H

#include <imgui/imgui.h>

#include <VulkanExtension.h>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "Texture.h"
#include <assimp/ProgressHandler.hpp>

namespace Sparkle {
class RenderBackend;

class GUI : public Assimp::ProgressHandler {
public:
    struct FrameData {
        size_t fps;
    };
    struct ProgressData {
        bool isLoading;
        float value;
        int currentStep;
        int maxSteps;

        ProgressData(bool isLoading = false, float value = 0.0f, int curr = 0, int max = 0)
            : isLoading(isLoading)
            , value(value)
            , currentStep(curr)
            , maxSteps(max)
        {
        }
    };
    struct PushConstants {
        glm::vec2 scale;
        glm::vec2 translate;
    } pushConstants;

    GUI();

    float getExposure() const { return exposure; }
    float getGamma() const { return gamma; }

    void toggleOptions()
    {
        showOptions = !showOptions;
    }

    //	~GUI();

    bool Update(float percentage = -1.0);
    void UpdatePostProcess(int currentStep /*= 0*/, int numberOfSteps /*= 0*/);

    void updateBuffers(const std::vector<VkFence>& fences);
    void updateFrame(const FrameData frameData);

    void init(float width, float height, VkRenderPass renderPass);
    void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer);

    void cleanup()
    {
        fontTex->cleanup();
        fontTex.reset();
        vertexBuffer.destroy(true);
        delete (vertexMemory);
        indexBuffer.destroy(true);
        delete (indexMemory);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyPipelineCache(device, pipelineCache, nullptr);

        vkDestroyShaderModule(device, vtxModule, nullptr);
        vkDestroyShaderModule(device, frgModule, nullptr);
    }

private:
    static const uint32_t minIdxBufferSize = 1048576; // 1MB = 524288 indices
    static const uint32_t minVtxBufferSize = 1048576; // 1MB = 52428  vertices

    VkShaderModule vtxModule;
    VkShaderModule frgModule;

    std::shared_ptr<RenderBackend> renderBackend;

    std::shared_ptr<Texture> fontTex;
    vkExt::Buffer vertexBuffer;
    vkExt::SharedMemory* vertexMemory = nullptr;
    vkExt::Buffer indexBuffer;
    vkExt::SharedMemory* indexMemory = nullptr;
    uint64_t vtxCount = 0;
    uint64_t idxCount = 0;

    VkRenderPass renderPass;
    VkPipelineCache pipelineCache;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;

    VkDevice device = VK_NULL_HANDLE;

    float windowWidth, windowHeight;

    float gamma = 2.2f;
    float exposure = 1.0f;

    bool showOptions = false;

    static VkPipelineShaderStageCreateInfo loadUiShader(const std::string shaderName, VkShaderStageFlagBits stage);
    void initResources();

    ProgressData assimpProgress;
};
}

#endif