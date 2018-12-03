#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "Shader.h"

namespace Engine
{
	class GraphicsPipeline
	{
	public:
		void createPipeline();

		auto getFramebufferPtr() { return swapChainFramebuffers.data(); }
		auto getFramebufferPtrs() const { return swapChainFramebuffers; }
		auto getRenderPassPtr() const { return pRenderPass; }
		auto getGraphicsPipelinePtr() const { return pGraphicsPipeline; }
		auto getPipelineLayoutPtr() const { return pPipelineLayout; }
		auto getDescriptorSetPtr() const { return pDescriptorSet; }

	private:
		VkDescriptorSetLayout pDescriptorSetLayout{};
		VkDescriptorSet pDescriptorSet{};
		VkDescriptorPool pDescriptorPool{};

		VkRenderPass pRenderPass{};
		VkPipelineLayout pPipelineLayout{};
		VkPipeline pGraphicsPipeline{};

		VkViewport viewport{};

		std::vector<VkFramebuffer> swapChainFramebuffers;

		std::shared_ptr<Engine::Shaders::ShaderProgram> shader;
	};
}
