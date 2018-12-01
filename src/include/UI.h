#ifndef UI_H
#define UI_H

#include <imgui/imgui.h>

#include <Application.h>
#include <VulkanExtension.h>

namespace Engine {
	class GUI {
	private:
		VkSampler sampler;
		vkExt::Buffer vertexBuffer;
		vkExt::Buffer indexBuffer;
		int32_t vtxCount = 0;
		int32_t idxCount = 0;

		vkExt::SharedMemory* fontMemory = nullptr;
		vkExt::Image fontImage;
		VkImageView fontImageView = VK_NULL_HANDLE;

		VkPipelineCache pipelineCache;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;

		VkDescriptorPool descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;

		VkDevice device = VK_NULL_HANDLE;		
	public:
		struct PushConstants {
			glm::vec2 scale;
			glm::vec2 translate;
		} pushConstants;

		GUI() {
			device = App::getHandle().getDevice();
			ImGui::CreateContext();
		}

		~GUI();

		void init(float width, float height);
		void initResources();
	};
}

#endif