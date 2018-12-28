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

#include <future>

#include "UI.h"
#include "Camera.h"
#include "Geometry.h"
#include "AppSettings.h"
#include "GraphicsPipeline.h"

#define MAX_FRAMES_IN_FLIGHT 2
#define DEVICE_NOT_SUITABLE 0

#define INTEL_GPU_BUILD // TODO: change to dynamic check if intel gpu..

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
		RenderBackend(GLFWwindow* windowPtr, std::string name, std::shared_ptr<Camera> camera, std::shared_ptr<Geometry::Scene> scene);

		void initialize(std::shared_ptr<Settings> settings, bool withValidation = false);
		void draw(double deltaT);
		void updateUniforms();
		void updateUiData(GUI::FrameData uiData);

		void cleanup();
		void reloadShaders();

		Geometry::Mesh::BufferOffset uploadMeshGPU(const Geometry::Mesh* mesh);

		std::shared_ptr<GUI> getUiHandle() const { return pUi; }
		void updateDrawCommand();
		
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
		const std::vector<VkImageView>& getDepthImageViewsRef() const { return depthImageViews; }
		const VkDescriptorSetLayout& getMaterialDescriptorSetLayout() const { return pMaterialDescriptorSetLayout; }
		const size_t getMaterialTextureLimit() const { return materialTextureLimit; }

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
		std::string appName;

		std::shared_ptr<Camera> pCamera = nullptr;
		std::shared_ptr<Geometry::Scene> pScene = nullptr;
		std::shared_ptr<GUI> pUi = nullptr;

		Shaders::FragmentShaderUniforms fragmentUBO;

		std::future<void> levelLoadFuture;

		GLFWwindow* pWindow;
		int viewportWidth, viewportHeight;

		VkDebugReportCallbackEXT pCallback;
		VkSurfaceKHR pSurface;

		VkSurfaceFormatKHR surfaceFormat;
		VkPresentModeKHR presentMode;

		VkInstance pVulkanInstance;
		VkPhysicalDevice pPhysicalDevice;
		VkDevice pVulkanDevice;
		VkSwapchainKHR pSwapChain;
		VkFormat swapChainImageFormat;
		VkFormat depthFormat;
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
		std::vector<VkSemaphore> semUiFinished;
		std::vector<VkFence> inFlightFences;

		size_t frameCounter = 0;

		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkCommandBuffer> uiCommandBuffers;

		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;

		std::vector<vkExt::Image> depthImages;
		std::vector<VkImageView> depthImageViews;
		std::vector<vkExt::SharedMemory*> depthImageMemory;

		VkDescriptorSetLayout pMaterialDescriptorSetLayout;
		size_t materialTextureLimit = 5; // diffuse, spec, normal, roughness, metallic 

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
		VkClearColorValue cClearColor = { 0.2f, 0.2f, 0.2f, 1.0f };
		VkClearDepthStencilValue cClearDepth = { 1.0f, 0 };

		std::vector<VkImage> deviceCreatedImages;
		std::vector<VkImageView> deviceCreatedImageViews;

		bool updateGeometry = true;

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
		void createMaterialDescriptorSetLayout();
		void createPipeline();
		void createDrawBuffer();
		void createCommandBuffers();
		void recordDrawCmdBuffers();
		void createSyncObjects();
		void setupGui();
		void setupLights();
		void increaseDrawBufferSize(VkDeviceSize newVertLimit, VkDeviceSize newIndexLimit);
		void cleanupSwapChain();
		void recreateSwapChain();
		void destroyCommandBuffers();
		void recreateDrawCmdBuffers();
		void recreateAllCmdBuffers();

		void initLight(Lights::Light* light, glm::vec3 pos, glm::vec3 col, float radius, Lights::LType type = Lights::LTypeFlags::SPARKLE_LIGHT_TYPE_POINT);

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