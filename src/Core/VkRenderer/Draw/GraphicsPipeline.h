#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "Shader.h"
#include "VulkanExtension.h"

namespace Sparkle {
class DeferredDraw {
public:
	struct FrameBufferAtt {
		vkExt::Image image;
		VkImageView view;
		vkExt::SharedMemory* memory;
		VkFormat format;
	};
	struct MRTFrameBuffer {
		VkExtent2D extent;
		VkFramebuffer framebuffer;
		FrameBufferAtt position;
		FrameBufferAtt normal;
		FrameBufferAtt albedo;
		FrameBufferAtt pbrSpecular;
		FrameBufferAtt depth;
	};

	DeferredDraw(VkViewport targetViewport)
	    : viewport(targetViewport)
	{
		initPipelines();
	}

	void initPipelines();
	void updateMRTDescriptorSets() const;
	void updateDeferredDescriptorSets() const;
	void updateDescriptorSets() const;

	auto getDeferredFramebufferPtr() { return swapChainFramebuffers.data(); }
	auto getDeferredFramebufferPtrs() const { return swapChainFramebuffers; }
	auto getDeferredRenderPassPtr() const { return deferredRenderPass; }
	auto getDeferredPipelinePtr() const { return deferredPipeline; }
	auto getDeferredPipelineLayoutPtr() const { return deferredPipelineLayout; }
	auto getDeferredDescriptorSetPtr(size_t index)
	{
		return deferredDescriptorSets[index];
	}

	auto getMRTFramebufferPtr() { return offscreenFramebuffers.data(); }
	auto getMRTFramebufferPtrs() const { return offscreenFramebuffers; }
	auto getMRTRenderPassPtr() const { return mrtRenderPass; }
	auto getMRTPipelinePtr() const { return mrtPipeline; }
	auto getMRTPipelineLayoutPtr() const { return mrtPipelineLayout; }
	auto getMRTDescriptorSetPtr(size_t index)
	{
		if (mrtProgram->dynamicBufferDirty) {
			updateMRTDescriptorSets();
			mrtProgram->dynamicBufferDirty = false;
		}
		return mrtDescriptorSets[index];
	}

	auto getMRTShaderProgramPtr() const { return mrtProgram; }
	auto getDeferredShaderProgramPtr() const { return deferredProgram; }

	void cleanup();

private:
	VkDescriptorSetLayout mrtDescriptorSetLayout {};
	std::vector<VkDescriptorSet> mrtDescriptorSets {};
	//VkDescriptorPool mrtDescriptorPool {};

	VkDescriptorSetLayout deferredDescriptorSetLayout {};
	std::vector<VkDescriptorSet> deferredDescriptorSets {};
	//VkDescriptorPool deferredDescriptorPool {};

	//VkDescriptorSetLayout descriptorSetLayout {};
	//VkDescriptorSet descriptorSet {};
	VkDescriptorPool descriptorPool {};

	VkRenderPass deferredRenderPass {};
	VkPipelineLayout deferredPipelineLayout {};
	VkPipeline deferredPipeline {};

	VkRenderPass mrtRenderPass {};
	VkPipelineLayout mrtPipelineLayout {};
	VkPipeline mrtPipeline {};

	VkSampler colorSampler = nullptr;

	VkViewport viewport {};

	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<MRTFrameBuffer> offscreenFramebuffers;

	std::shared_ptr<Sparkle::Shaders::MRTShaderProgram> mrtProgram;
	std::shared_ptr<Sparkle::Shaders::DeferredShaderProgram> deferredProgram;

	void initAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAtt* attachment);
};
} // namespace Sparkle
