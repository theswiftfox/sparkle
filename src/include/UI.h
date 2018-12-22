#ifndef UI_H
#define UI_H

#include <imgui/imgui.h>

#include <memory>
#include <vector>
#include <VulkanExtension.h>

#include <glm/glm.hpp>

#include "Texture.h"

namespace Engine {
	class RenderBackend;

	class GUI {
	public:
		struct FrameData {
			size_t fps;
		};

	private:
		std::shared_ptr<RenderBackend> renderBackend;

		std::shared_ptr<Texture> fontTex;
		vkExt::Buffer vertexBuffer;
		vkExt::SharedMemory* vertexMemory = nullptr;
		vkExt::Buffer indexBuffer;
		vkExt::SharedMemory* indexMemory = nullptr;
		int32_t vtxCount = 0;
		int32_t idxCount = 0;

		VkRenderPass renderPass;
		VkPipelineCache pipelineCache;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;

		VkDescriptorPool descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;

		VkDevice device = VK_NULL_HANDLE;

		float windowWidth, windowHeight;

		static VkPipelineShaderStageCreateInfo loadUiShader(const std::string shaderName, VkShaderStageFlagBits stage);
		void initResources();

	public:
		struct PushConstants {
			glm::vec2 scale;
			glm::vec2 translate;
		} pushConstants;

		GUI();

		~GUI();

		void updateBuffers(const std::vector<VkFence>& fences);
		void updateFrame(const FrameData frameData);

		void init(float width, float height, VkRenderPass renderPass);
		void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer);
	};
}

#endif