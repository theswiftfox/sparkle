#ifndef APP_H
#define APP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>

#include <atomic>
#include <vector>

#include "AppSettings.h"
#include "VulkanExtension.h"
#include "Geometry.h"
#include "Texture.h"
#include "Camera.h"
#include "GraphicsPipeline.h"

constexpr auto WINDOW_WIDTH = 1024;
constexpr auto WINDOW_HEIGHT = 768;
constexpr auto DEVICE_NOT_SUITABLE = 0;

#define APP_NAME "Sparkle Engine"
#define MAX_FRAMES_IN_FLIGHT 2
#define INTEL_GPU_BUILD // TODO: change to dynamic check if intel gpu..

constexpr auto INITIAL_VERTEX_COUNT = 1000;

namespace Engine {
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

	class App {
	public:
		static App& getHandle() {
			static App handle;
			return handle;
		}

		~App() {
			if (ppDrawMemory) {
				delete(ppDrawMemory);
			}
		}

		void run(std::string config, bool withValidation = false) {
#ifndef FORCE_NO_VALIDATION
			enableValidationLayers = withValidation;
#endif
			initialize(config);
			renderLoop();
			cleanup();
		}

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
		VkDescriptorSetLayout getSamplerDescriptorSetLayout() const { return pSamplerSetLayout; }

		Geometry::Mesh::BufferOffset storeMesh(const Geometry::Mesh* mesh);

		void allocateMemory(VkDeviceSize size, VkMemoryPropertyFlags properties, uint32_t memoryTypeFilterBits, vkExt::SharedMemory* memory) const;

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, vkExt::Buffer& buffer, vkExt::SharedMemory* bufferMemory) const;
		void createImage2D(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, vkExt::Image& image, vkExt::SharedMemory* imageMemory, VkDeviceSize memOffset = 0, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED) const;
		VkImageView createImageView2D(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuff = nullptr) const;

		VkCommandBuffer beginOneTimeCommand() const;
		void endOneTimeCommand(VkCommandBuffer buffer) const;

	private:
		/*
		Members
		*/
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
		std::vector<VkSemaphore> semGuiRendered;
		std::vector<VkFence> inFlightFences;

		size_t frameCounter = 0;

		// Command Buffers for: render pass
		// these are recorded once (for a swapchain) and reused every frame. 
		std::vector<VkCommandBuffer> commandBuffers;

		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;

		std::vector<vkExt::Image> depthImages;
		std::vector<VkImageView> depthImageViews;
		std::vector<vkExt::SharedMemory*> depthImageMemory;

		// GUI data, renderpass and descriptor pool
		ImGui_ImplVulkanH_WindowData windowData;
		VkRenderPass pGuiRenderPass;
		VkDescriptorPool pGuiDescriptorPool;

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

		std::shared_ptr<Settings> settings;
		// Different settings for the application and Rendering. Can be changed via GUI during runtime.
		Shaders::FragmentShaderSettings fragmentShaderSettings = {};

		std::vector<std::shared_ptr<Geometry::Mesh>> geometry;
		std::unique_ptr<Camera> pCamera;

		/*** GUI Interaction ***/
		// Bind ImGuiIO
		ImGuiIO imguiIo;

		// Status message string
		std::string message = "";

		// FPS string
		bool showFPS = true;
		const std::string fps = "FPS: ";
		size_t lastFPS = 0;

		// Different booleans to hold state information of the UI
		bool imguiHandlesMouse = false;
		bool imguiHandlesKeyboard = false;
		bool acceptingInput = false;
		bool shouldRecreateSwapchain = false;

		VkClearColorValue cClearColor = { 0.2f, 0.2f, 0.2f, 1.0f };
		VkClearDepthStencilValue cClearDepth = { 1.0f, 0 };

		App() {
			requiredFeatures.tessellationShader = VK_FALSE;
			requiredFeatures.samplerAnisotropy = VK_TRUE;
			settings = loadFromDefault();
		}
		App(const App& app) = delete;

		/* 
		Function prototypes
		*/
		void initialize(std::string config) {
			if (!config.empty()) {
				settings = std::make_unique<Settings>(config);
			} else {
				settings = loadFromDefault();
			}
			pCamera = std::make_unique<Camera>(settings);
			createWindow();
			setupVulkan();
			setupApplicationData(config);
		}

		std::unique_ptr<Settings> loadFromDefault() const {
			return std::make_unique<Settings>("assets/settings.ini");
		}

		void createWindow();
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
		void createShaders();
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

		RequiredQueueFamilyIndices getQueueFamilies(VkPhysicalDevice device ) const;
		SwapChainSupportInfo getSwapChainSupportInfo(VkPhysicalDevice device) const;
		size_t checkDeviceRequirements(VkPhysicalDevice device) const;
		bool checkValidationLayerSupport() const;

		uint32_t findMemoryType(uint32_t filter, VkMemoryPropertyFlags properties) const;
		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

		static bool formatHasStencilComponent(VkFormat& format) {
			return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
		}

		// swapchain
		static VkSurfaceFormatKHR getBestSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR getBestPresentMode(const std::vector<VkPresentModeKHR>& availableModes);

		VkExtent2D getSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

		std::vector<const char*> getRequiredExtensions() const;

		void renderLoop();
		void drawGui(size_t imageIndex, bool renderQueued);
		void drawFrame(double deltaT);
		void updateUniforms();
		void cleanup();

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);

		void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

		// Friend classes
		friend class Texture;
	};
}

#endif
