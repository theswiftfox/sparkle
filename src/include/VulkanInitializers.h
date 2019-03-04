/*
*   VulkanInitializers.h
*   
*   Initializers for Vulkan structures
*   
*   Copyright (C) 2019 by Patrick Gantner
*
*   This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vulkan/vulkan.h>

namespace Sparkle {
namespace vk {
	namespace init {
		inline VkCommandBufferAllocateInfo commandBufferInfo(VkCommandPool pool, VkCommandBufferLevel level, uint32_t count)
		{
			VkCommandBufferAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.pNext = nullptr;
			info.commandPool = pool;
			info.level = level;
			info.commandBufferCount = count;
			return info;
		}

		inline VkDescriptorSetLayoutBinding setLayoutBinding(VkDescriptorType type, VkShaderStageFlagBits stageFlagBits, uint32_t binding, uint32_t count = 1)
		{
			VkDescriptorSetLayoutBinding bindingInfo = {};
			bindingInfo.descriptorType = type;
			bindingInfo.stageFlags = stageFlagBits;
			bindingInfo.binding = binding;
			bindingInfo.descriptorCount = count;
			return bindingInfo;
		}

		inline VkDescriptorSetLayoutCreateInfo setLayoutInfo(const VkDescriptorSetLayoutBinding* pBindings, uint32_t count)
		{
			VkDescriptorSetLayoutCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;
			info.pBindings = pBindings;
			info.bindingCount = count;
			return info;
		}

		inline VkPipelineLayoutCreateInfo pipelineLayoutInfo(const VkDescriptorSetLayout* pSetLayouts, uint32_t count = 1)
		{
			VkPipelineLayoutCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;
			info.pSetLayouts = pSetLayouts;
			info.setLayoutCount = count;
			return info;
		}

		inline VkComputePipelineCreateInfo computePipelineCreateInfo(VkPipelineLayout layout, VkPipelineCreateFlags flags = 0)
		{
			VkComputePipelineCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			info.pNext = nullptr;
			info.layout = layout;
			info.flags = flags;
			return info;
		}

		inline VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(VkDescriptorPool pool, const VkDescriptorSetLayout* pSetLayouts, uint32_t count)
		{
			VkDescriptorSetAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			info.pNext = nullptr;
			info.pSetLayouts = pSetLayouts;
			info.descriptorSetCount = count;
			info.descriptorPool = pool;
			return info;
		}

		inline VkDescriptorPoolSize descriptorPoolSize(VkDescriptorType type, uint32_t size)
		{
			VkDescriptorPoolSize psize = {};
			psize.descriptorCount = size;
			psize.type = type;
			return psize;
		}

		inline VkDescriptorPoolCreateInfo descriptorPoolInfo(const VkDescriptorPoolSize* pDescriptorPoolSizes, uint32_t sizesCount, uint32_t maxSets)
		{
			VkDescriptorPoolCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			info.pNext = nullptr;
			info.pPoolSizes = pDescriptorPoolSizes;
			info.poolSizeCount = sizesCount;
			info.maxSets = maxSets;
			return info;
		}

		inline VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet targetSet, VkDescriptorType type, uint32_t binding, const VkDescriptorBufferInfo* bufferInfo, uint32_t count = 1)
		{
			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = targetSet;
			write.descriptorType = type;
			write.dstBinding = binding;
			write.pBufferInfo = bufferInfo;
			write.descriptorCount = count;
			return write;
		}

		inline VkCommandBufferBeginInfo commandBufferBeginInfo()
		{
			VkCommandBufferBeginInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.pNext = nullptr;
			info.pInheritanceInfo = nullptr;
			return info;
		}

		inline VkBufferMemoryBarrier bufferMemoryBarrier()
		{
			VkBufferMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.pNext = nullptr;
			return barrier;
		}

		inline VkPipelineShaderStageCreateInfo shaderStageInfo(VkShaderModule module, VkShaderStageFlagBits stageFlags)
		{
			VkPipelineShaderStageCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.pNext = nullptr;
			info.module = module;
			info.stage = stageFlags;
			info.pName = "main";
			return info;
		}

		inline VkFenceCreateInfo fenceInfo(VkFenceCreateFlagBits flags)
		{
			VkFenceCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = flags;
			return info;
		}
		inline VkSemaphoreCreateInfo semaphoreInfo()
		{
			VkSemaphoreCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			info.pNext = nullptr;
			return info;
		}
	} // namespace init
} // namespace vk
} // namespace Sparkle