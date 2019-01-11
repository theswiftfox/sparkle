#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "Shader.h"

namespace Engine
{
	class GraphicsPipeline
	{
	public:
		struct FrameBufferAtt {
			VkImage image;
			VkImageView view;
			VkFormat format;
			VkDeviceMemory memory;
		};
		struct FrameBuffer {
			int32_t width;
			int32_t height;
			VkFramebuffer framebuffer;
			FrameBufferAtt position;
			FrameBufferAtt normal;
			FrameBufferAtt pbrValues;
			FrameBufferAtt depth;
			VkRenderPass renderPass;
		};

		GraphicsPipeline(VkViewport targetViewport) : viewport(targetViewport) {
			initPipeline();
		}

		void initPipeline();
		void updateDescriptorSets() const;
		
		auto getFramebufferPtr() { return swapChainFramebuffers.data(); }
		auto getFramebufferPtrs() const { return swapChainFramebuffers; }
		auto getRenderPassPtr() const { return pRenderPass; }
		auto getVkGraphicsPipelinePtr() const { return pGraphicsPipeline; }
		auto getPipelineLayoutPtr() const { return pPipelineLayout; }
		auto getDescriptorSetPtr() { 
			if (shader->dynamicBufferDirty) {
				updateDescriptorSets();
				shader->dynamicBufferDirty = false;
			}
			return pDescriptorSet;
		}

		auto getShaderProgramPtr() const { return shader; }

		void cleanup();

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
