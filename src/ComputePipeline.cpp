#include "ComputePipeline.h"

#include "Application.h"
#include "VulkanInitializers.h"

using namespace Sparkle;

void ComputePipeline::initialize(uint32_t queueIndex)
{
    auto device = App::getHandle().getRenderBackend()->getDevice();
    std::array<VkDescriptorPoolSize, 2> poolSizes = {
        sparkle::vk::init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3),
        sparkle::vk::init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
    };

    auto descPoolInfo = sparkle::vk::init::descriptorPoolInfo(poolSizes.data(), static_cast<uint32_t>(poolSizes.size()), 2);
    VK_THROW_ON_ERROR(vkCreateDescriptorPool(device, &descPoolInfo, nullptr, &descPool), "DescriptorPool creation for Compute failed!");

    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.pNext = nullptr;
    queueInfo.queueFamilyIndex = queueIndex;
    queueInfo.queueCount = 1;
    vkGetDeviceQueue(device, queueIndex, 0, &queue);

    std::array<VkDescriptorSetLayoutBinding, 4> setLayoutBindings = {
        sparkle::vk::init::setLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
        sparkle::vk::init::setLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1),
        sparkle::vk::init::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2),
        sparkle::vk::init::setLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 3)
    };

    auto setLayoutInfo = sparkle::vk::init::setLayoutInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
    VK_THROW_ON_ERROR(vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &descSetLayout), "DescriptorSetLayout creation for Compute failed!");

    auto pipeLayoutInfo = sparkle::vk::init::pipelineLayoutInfo(&descSetLayout);
    VK_THROW_ON_ERROR(vkCreatePipelineLayout(device, &pipeLayoutInfo, nullptr, &pipelineLayout), "PipelineLayout creation for Compute failed!");

    auto allocInfo = sparkle::vk::init::descriptorSetAllocateInfo(descPool, &descSetLayout, 1);
    VK_THROW_ON_ERROR(vkAllocateDescriptorSets(device, &allocInfo, &descSet), "DescriptorSet allocation for Compute failed!");
}

void ComputePipeline::cleanup()
{
    auto device = App::getHandle().getRenderBackend()->getDevice();

    vkFreeDescriptorSets(device, descPool, 1, &descSet);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descPool, nullptr);
}