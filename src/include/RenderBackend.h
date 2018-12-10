/**
*	Copyright (c) 2018 Patrick Gantner
*
*   This work is licensed under the Creative Commons Attribution 4.0 International License.
*	To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/
*	or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
*
**/

#pragma once
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <VulkanExtension.h>

#include "Camera.h"
#include "Geometry.h"
#include "AppSettings.h"
#include "GraphicsPipeline.h"

namespace Engine {
	namespace Vulkan {
		struct RequiredQueueFamilyIndices {
			int graphicsFamily = -1;
			int presentFamily = -1;

			bool allPresent() const {
				return graphicsFamily >= 0 && presentFamily >= 0;
			}
		};

		struct SwapChainSupportInfo {
			VkSurfaceCapabilitiesKHR capabilities{};
			std::vector<VkSurfaceFormatKHR> formats{};
			std::vector<VkPresentModeKHR> presentModes{};
		};
	}

	constexpr auto INITIAL_VERTEX_COUNT = 1000;

	class RenderBackend {
	public:
		RenderBackend(GLFWwindow* windowPtr);

		void initialize(Settings config, bool withValidation = false);
		void drawFrame(double deltaT);
		void updateUniforms();

		void cleanup();

		Geometry::Mesh::BufferOffset storeMesh(const Geometry::Mesh* mesh);

		
		VkDevice getDevice() const { return pVulkanDevice; }
		VkPhysicalDevice getPhysicalDevice() const { return pPhysicalDevice; }
		VkCommandPool getCommandPool() const { return pCommandPool; }
		VkQueue getDefaultQueue() const { return pGraphicsQueue; }
		VkFormat getImageFormat() const { return swapChainImageFormat; }
		VkFormat getDepthFormat() const {
			return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		}
		VkExtent2D getSwapChainExtent() const { return swapChainExtent; }
		const std::vector<VkImageView>& getSwapChainImageViewsRef() const { return swapChainImageViews; }
		VkDescriptorSetLayout getSamplerDescriptorSetLayout() const { return pSamplerSetLayout; }

		/*
		* Vulkan Resource creation
		*/
		void allocateMemory(VkDeviceSize size, VkMemoryPropertyFlags properties, uint32_t memoryTypeFilterBits, vkExt::SharedMemory* memory) const;
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, vkExt::Buffer& buffer, vkExt::SharedMemory* bufferMemory) const;
		
		void createImage2D(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, vkExt::Image& image, vkExt::SharedMemory* imageMemory, VkDeviceSize memOffset = 0, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);
		VkImageView createImageView2D(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuff = nullptr) const;

		VkCommandBuffer beginOneTimeCommand() const;
		void endOneTimeCommand(VkCommandBuffer buffer) const;
	private:
		std::unique_ptr<Camera> pCamera;


		GLFWwindow* pWindow;

		VkDebugReportCallbackEXT pCallback;
		VkSurfaceKHR pSurface;

		VkSurfaceFormatKHR surfaceFormat;
		VkPresentModeKHR presentMode;

		VkInstance pVulkanInstance;
		VkPhysicalDevice pPhysicalDevice;
		VkDevice pVulkanDevice;
		VkSwapchainKHR pSwapChain;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		VkCommandPool pCommandPool;
		std::shared_ptr<GraphicsPipeline> pGraphicsPipeline;

		VkQueue pGraphicsQueue;
		VkQueue pPresentQueue;

		// Buffer and associated memory for Geometry used in the main draw pass
		vkExt::Buffer pDrawBuffer;
		vkExt::SharedMemory* ppDrawMemory;
		// some attributes used for the shared vertex and index buffer memory
		VkDeviceSize maxVertexSize = INITIAL_VERTEX_COUNT * sizeof(Geometry::Vertex);
		VkDeviceSize lastVertexOffset;
		VkDeviceSize maxIndexSize = INITIAL_VERTEX_COUNT * sizeof(uint32_t);
		VkDeviceSize indexBufferOffset;
		VkDeviceSize lastIndexOffset;

		// Synchronization objects
		std::vector<VkSemaphore> semImageAvailable;
		std::vector<VkSemaphore> semRenderFinished;
		std::vector<VkFence> inFlightFences;

		size_t frameCounter = 0;

		std::vector<VkCommandBuffer> commandBuffers;

		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;

		VkDescriptorSetLayout pSamplerSetLayout;

		VkPhysicalDeviceFeatures requiredFeatures = { };

		const std::vector<const char*> validationLayers = {
			"VK_LAYER_LUNARG_standard_validation"
		};

		const std::vector<const char*> requiredExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

#ifdef FORCE_NO_VALIDATION
		const bool enableValidationLayers = false;
#else
		bool enableValidationLayers = false;
#endif
		void setupVulkan();

		/*
		Vulkan setup
		*/
		void createVulkanInstance();
		void setupDebugCallback();
		void createSurface();
		void selectPhysicalDevice();
		void createVulkanDevice();
		void createSwapChain(VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE);
		void createImageViews();
		void createCommandPool();
		void createDepthResources();
		void createPipeline();
		void createSamplerDescriptorSet();
		void createDrawBuffer();
		void createCommandBuffers();
		void recordCommandBuffers();
		void createSyncObjects();
		void setupGui();
		void setupApplicationData(std::string config);
		void increaseDrawBufferSize(VkDeviceSize newVertLimit, VkDeviceSize newIndexLimit);
		void cleanupSwapChain();
		void recreateSwapChain();
		void destroyCommandBuffers();
		void recreateDrawCommandBuffers();
		void recreateAllCommandBuffers();

		Vulkan::RequiredQueueFamilyIndices getQueueFamilies(VkPhysicalDevice device) const;
		Vulkan::SwapChainSupportInfo getSwapChainSupportInfo(VkPhysicalDevice device) const;
		size_t checkDeviceRequirements(VkPhysicalDevice device) const;
		bool checkValidationLayerSupport() const;

		uint32_t findMemoryType(uint32_t filter, VkMemoryPropertyFlags properties) const;
		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

		static bool formatHasStencilComponent(VkFormat& format) {
			return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
		}

		static VkSurfaceFormatKHR getBestSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR getBestPresentMode(const std::vector<VkPresentModeKHR>& availableModes);

		VkExtent2D getSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

		std::vector<const char*> getRequiredExtensions() const;

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
	};
}