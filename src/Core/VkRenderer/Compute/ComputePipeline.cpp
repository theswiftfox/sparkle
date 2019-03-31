#include "ComputePipeline.h"

#include "Application.h"
#include "FileReader.h"
#include "VulkanInitializers.h"

using namespace Sparkle;

void ComputePipeline::initialize(uint32_t queueIndex, uint32_t cmdBuffCount)
{
	auto device = App::getHandle().getRenderBackend()->getDevice();
	std::array<VkDescriptorPoolSize, 2> poolSizes = {
		vk::init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3),
		vk::init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
	};

	auto descPoolInfo = vk::init::descriptorPoolInfo(poolSizes.data(), static_cast<uint32_t>(poolSizes.size()), 2);
	VK_THROW_ON_ERROR(vkCreateDescriptorPool(device, &descPoolInfo, nullptr, &descPool), "DescriptorPool creation for Compute failed!");

	VkDeviceQueueCreateInfo queueInfo = {};
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.pNext = nullptr;
	queueInfo.queueFamilyIndex = queueIndex;
	queueInfo.queueCount = 1;
	vkGetDeviceQueue(device, queueIndex, 0, &queue);

	std::array<VkDescriptorSetLayoutBinding, 4> setLayoutBindings = {
		vk::init::setLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
		vk::init::setLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1),
		vk::init::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2),
		vk::init::setLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 3)
	};

	auto setLayoutInfo = vk::init::setLayoutInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
	VK_THROW_ON_ERROR(vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &descSetLayout), "DescriptorSetLayout creation for Compute failed!");

	auto pipeLayoutInfo = vk::init::pipelineLayoutInfo(&descSetLayout);

	VK_THROW_ON_ERROR(vkCreatePipelineLayout(device, &pipeLayoutInfo, nullptr, &pipelineLayout), "PipelineLayout creation for Compute failed!");

	auto allocInfo = vk::init::descriptorSetAllocateInfo(descPool, &descSetLayout, 1);
	VK_THROW_ON_ERROR(vkAllocateDescriptorSets(device, &allocInfo, &descSet), "DescriptorSet allocation for Compute failed!");

	uboMem = new vkExt::SharedMemory();
	App::getHandle().getRenderBackend()->createBuffer(sizeof(UBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uboBuff, uboMem);

	auto pipeCreateInfo = vk::init::computePipelineCreateInfo(pipelineLayout);
	// shader = Shaders::createShaderModule(Tools::FileReader::readFile("shaders/cull.comp.hlsl.spv"));
	shader = Shaders::createShaderModule(Tools::FileReader::readFile("shaders/cull.comp.spv"));
	pipeCreateInfo.stage = vk::init::shaderStageInfo(shader, VK_SHADER_STAGE_COMPUTE_BIT);
	VK_THROW_ON_ERROR(vkCreateComputePipelines(device, nullptr, 1, &pipeCreateInfo, nullptr, &pipeline), "Compute Pipeline creation failed!");

	VkCommandPoolCreateInfo cpi = {};
	cpi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cpi.pNext = nullptr;
	cpi.queueFamilyIndex = queueIndex;
	cpi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_THROW_ON_ERROR(vkCreateCommandPool(device, &cpi, nullptr, &cmdPool), "Command Pool creation for Compute failed!");

	auto cmdBuffInfo = vk::init::commandBufferInfo(cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	cmdBuffers.resize(cmdBuffCount);
	for (auto& cmdBuff : cmdBuffers) {
		VK_THROW_ON_ERROR(vkAllocateCommandBuffers(device, &cmdBuffInfo, &cmdBuff), "Command Buffer creation for Compute failed!");
	}

	auto fenceInfo = vk::init::fenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	fences.resize(cmdBuffCount);
	for (auto& fence : fences) {
		VK_THROW_ON_ERROR(vkCreateFence(device, &fenceInfo, nullptr, &fence), "Fence creation for Compute failed!");
	}

	auto semInfo = vk::init::semaphoreInfo();
	semaphores.resize(cmdBuffCount);
	for (auto& sem : semaphores) {
		VK_THROW_ON_ERROR(vkCreateSemaphore(device, &semInfo, nullptr, &sem), "Semaphore creation for Compute failed!");
	}
}

void ComputePipeline::cleanup()
{
	auto device = App::getHandle().getRenderBackend()->getDevice();

	for (auto& sem : semaphores) {
		vkDestroySemaphore(device, sem, nullptr);
	}
	for (auto& fence : fences) {
		vkDestroyFence(device, fence, nullptr);
	}
	vkDestroyCommandPool(device, cmdPool, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyShaderModule(device, shader, nullptr);
	//	vkFreeDescriptorSets(device, descPool, 1, &descSet);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descPool, nullptr);

	uboBuff.destroy(true);
	delete (uboMem);
}

void ComputePipeline::updateUBO(const ComputePipeline::UBO& ubo)
{
	if (!uboBuff.mapped())
		uboBuff.map();
	uboBuff.copyTo(&ubo, sizeof(UBO));
}