#ifndef UI_H
#define UI_H

#include <imgui/imgui.h>

#include <memory>
#include <vector>
#include <VulkanExtension.h>

#include <glm/glm.hpp>

#include "Texture.h"
#include <assimp/ProgressHandler.hpp>

namespace Engine {
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

			ProgressData(bool isLoading = false, float value = 0.0f, int curr = 0, int max = 0) : 
				isLoading(isLoading), value(value), currentStep(curr), maxSteps(max) { }
		};

	private:
		static const uint32_t minIdxBufferSize = 1048576; // 1MB = 524288 indices
		static const uint32_t minVtxBufferSize = 1048576; // 1MB = 52428  vertices

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

		static VkPipelineShaderStageCreateInfo loadUiShader(const std::string shaderName, VkShaderStageFlagBits stage);
		void initResources();

		ProgressData assimpProgress;

	public:
		struct PushConstants {
			glm::vec2 scale;
			glm::vec2 translate;
		} pushConstants;

		GUI();

		~GUI();

		bool Update(float percentage = -1.0);
		void UpdatePostProcess(int currentStep /*= 0*/, int numberOfSteps /*= 0*/);

		void updateBuffers(const std::vector<VkFence>& fences);
		void updateFrame(const FrameData frameData);

		void init(float width, float height, VkRenderPass renderPass);
		void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer);
	};
}

#endif