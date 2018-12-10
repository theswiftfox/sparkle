#include "RenderBackend.h"

using namespace Engine;

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);
static void checkVkResult(VkResult err) {
	if (err == VK_SUCCESS) return;
	if (err < 0) {
		throw std::runtime_error("VkResult: " + std::to_string(err));
	}
	else {
		std::cout << "VkResult: " << err << std::endl;
	}
}

void RenderBackend::setupVulkan() {
	createVulkanInstance();
	setupDebugCallback(); createSurface();
	selectPhysicalDevice();
	createVulkanDevice();
	createSwapChain();
	createImageViews();
	createCommandPool();
	createDepthResources();
	createDrawBuffer();
	createSamplerDescriptorSet();
	createPipeline();
	createCommandBuffers();
	createSyncObjects();
	setupGui();
}

void RenderBackend::drawFrame(double deltaT) {
	vkWaitForFences(pVulkanDevice, 1, &inFlightFences[frameCounter], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(pVulkanDevice, 1, &inFlightFences[frameCounter]);

	uint32_t imageIndex;
	auto result = vkAcquireNextImageKHR(pVulkanDevice, pSwapChain, std::numeric_limits<uint64_t>::max(), semImageAvailable[frameCounter], VK_NULL_HANDLE, &imageIndex);

#ifndef INTEL_GPU_BUILD
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
#else
	int width, height;
	glfwGetWindowSize(pWindow, &width, &height);
	const auto uWidth = static_cast<uint32_t>(width);
	const auto uHeight = static_cast<uint32_t>(height);
	if (uWidth != swapChainExtent.width && uHeight != swapChainExtent.height) {
		recreateSwapChain();
		return;
	}
#endif


	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Aquisation of SwapChain Image failed");
	}

	{
		auto& dst = swapChainImages[imageIndex];
		VkImageSubresourceRange range = {};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.levelCount = 1;
		range.layerCount = 1;

		auto cmdBuff = beginOneTimeCommand();
		transitionImageLayout(dst, swapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdBuff);

		vkCmdClearColorImage(cmdBuff, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cClearColor, 1, &range);
		transitionImageLayout(dst, swapChainImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, cmdBuff);

		vkEndCommandBuffer(cmdBuff);

		VkSubmitInfo submitInfo = {};

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { semImageAvailable[frameCounter] };

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuff;

		VkSemaphore signalSemaphores[] = { semImageAvailable[frameCounter] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		auto res = vkQueueSubmit(pGraphicsQueue, 1, &submitInfo, inFlightFences[frameCounter]);
		if (res != VK_SUCCESS) {
			throw std::runtime_error("Clear Screen failed! Error: " + std::to_string(res));
		}

		vkWaitForFences(pVulkanDevice, 1, &inFlightFences[frameCounter], VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(pVulkanDevice, 1, &inFlightFences[frameCounter]);

		vkFreeCommandBuffers(pVulkanDevice, pCommandPool, 1, &cmdBuff);
	}
	auto renderQueued = false;
	if (!geometry.empty()) {
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { semImageAvailable[frameCounter] };

		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		std::vector<VkCommandBuffer> cmdBuffers;

		cmdBuffers.push_back(commandBuffers[imageIndex]);

		submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
		submitInfo.pCommandBuffers = cmdBuffers.data();

		VkSemaphore signalSemaphores[] = { semRenderFinished[frameCounter] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		auto res = vkQueueSubmit(pGraphicsQueue, 1, &submitInfo, inFlightFences[frameCounter]);
		renderQueued = true;
		if (res != VK_SUCCESS) {
			throw std::runtime_error("Queue submission failed! FrameIndex: " + std::to_string(frameCounter) + " Error: " + std::to_string(res));
		}
	}

	// final pass draws GUI elements on top of the current framebuffer image
	drawGui(imageIndex, renderQueued);

	VkSemaphore guiDrawnSemaphore[] = { semGuiRendered[frameCounter] };
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = guiDrawnSemaphore;

	VkSwapchainKHR swapChains[] = { pSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(pPresentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Aquisation of SwapChain Image failed");
	}

	frameCounter = (frameCounter + 1) % MAX_FRAMES_IN_FLIGHT;
	vkDeviceWaitIdle(pVulkanDevice);
}


void RenderBackend::updateUniforms() {
	int width, height;
	glfwGetWindowSize(pWindow, &width, &height);

	Shaders::UBO ubo = {};

	ubo.view = pCamera->getView();
	ubo.projection = pCamera->getProjection();

	const auto shaderProg = pGraphicsPipeline->getShaderProgramPtr();
	shaderProg->updateUniformBufferObject(ubo);
	Shaders::TesselationControlSettings tese = {};
	shaderProg->updateTesselationControlSettings(tese);
	shaderProg->updateDynamicUniformBufferObject(geometry);

	shaderProg->updateFragmentShaderSettings(fragmentShaderSettings);
}

void RenderBackend::cleanupSwapChain() {
	vkFreeCommandBuffers(pVulkanDevice, pCommandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	pGraphicsPipeline->cleanup();
	pGraphicsPipeline.reset();

	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(pVulkanDevice, imageView, nullptr);
		auto i = std::find(deviceCreatedImageViews.begin(), deviceCreatedImageViews.end(), imageView);
		if (i != deviceCreatedImageViews.end()) {
			deviceCreatedImageViews.erase(i);
		}
	}

	vkDestroySwapchainKHR(pVulkanDevice, pSwapChain, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore(pVulkanDevice, semRenderFinished[i], nullptr);
		vkDestroySemaphore(pVulkanDevice, semImageAvailable[i], nullptr);
		vkDestroyFence(pVulkanDevice, inFlightFences[i], nullptr);
	}
}

void RenderBackend::cleanup() {
	cleanupSwapChain();

	pDrawBuffer.destroy(true); // cleanup vertex buffer and memory

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	for (auto& mesh : geometry) {
		mesh.reset();
	}
	geometry.clear();

	for (auto& imageView : deviceCreatedImageViews) {
		vkDestroyImageView(pVulkanDevice, imageView, nullptr);
	}

	for (auto& image : deviceCreatedImages) {
		vkDestroyImage(pVulkanDevice, image, nullptr);
	}

	vkDestroyCommandPool(pVulkanDevice, pCommandPool, nullptr);

	vkDestroyDevice(pVulkanDevice, nullptr);

	if (enableValidationLayers) {
		DestroyDebugReportCallbackEXT(pVulkanInstance, pCallback, nullptr);
	}

	vkDestroySurfaceKHR(pVulkanInstance, pSurface, nullptr);
	vkDestroyInstance(pVulkanInstance, nullptr);

	glfwDestroyWindow(pWindow);
	glfwTerminate();
}

void RenderBackend::createVulkanInstance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("Requested validation layers not present!");
	}

	VkApplicationInfo appInfo = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		nullptr,
		APP_NAME,
		VK_MAKE_VERSION(0, 1, 0),
		"Ghost Engine",
		VK_MAKE_VERSION(0, 0, 0),
		VK_API_VERSION_1_0
	};
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&createInfo, nullptr, &pVulkanInstance) != VK_SUCCESS) {
		throw std::runtime_error("Vulkan Instance creation failed!");
	}
}

void RenderBackend::selectPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(pVulkanInstance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("No GPU with Vulkan support found");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(pVulkanInstance, &deviceCount, devices.data());

	std::multimap<size_t, VkPhysicalDevice> ratedDevices;

	for (const auto& device : devices) {
		auto score = checkDeviceRequirements(device);
		ratedDevices.insert(std::make_pair(score, device));
	}

	if (ratedDevices.rbegin()->first > 0) {
		pPhysicalDevice = ratedDevices.rbegin()->second;
	}
	else {
		throw std::runtime_error("No GPU meeting the requirements found");
	}
}

void RenderBackend::createVulkanDevice() {
	const auto queueFamilyIndices = getQueueFamilies(pPhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentFamily };

	auto queuePriority = 1.0f;
	for (auto& queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pEnabledFeatures = &requiredFeatures;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

	if (enableValidationLayers) {
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		deviceCreateInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(pPhysicalDevice, &deviceCreateInfo, nullptr, &pVulkanDevice) != VK_SUCCESS) {
		throw std::runtime_error("Vulkan device setup failed");
	}

	vkGetDeviceQueue(pVulkanDevice, queueFamilyIndices.graphicsFamily, 0, &pGraphicsQueue);
	vkGetDeviceQueue(pVulkanDevice, queueFamilyIndices.presentFamily, 0, &pPresentQueue);
}

void RenderBackend::createSurface() {
	if (glfwCreateWindowSurface(pVulkanInstance, pWindow, nullptr, &pSurface) != VK_SUCCESS) {
		throw std::runtime_error("Window surface creation failed!");
	}
}

void RenderBackend::recreateSwapChain() {
	vkDeviceWaitIdle(pVulkanDevice); // do not destroy old swapchain while in use!

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createDepthResources();
	createPipeline();
	createCommandBuffers();

	createSyncObjects();
	setupGui();
}

void RenderBackend::createSwapChain(VkSwapchainKHR oldSwapChain) {
	const auto info = getSwapChainSupportInfo(pPhysicalDevice);

	surfaceFormat = getBestSwapSurfaceFormat(info.formats);
	presentMode = getBestPresentMode(info.presentModes);
	const auto extent = getSwapExtent(info.capabilities);

	auto imageCount = info.capabilities.minImageCount + 1; // +1 for tripple buffering
	if (info.capabilities.maxImageCount > 0 && imageCount > info.capabilities.maxImageCount) {
		imageCount = info.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = pSurface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	const auto indices = getQueueFamilies(pPhysicalDevice);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // bit less performance, but saves the ownership hassle for now
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = info.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // do not alphablend with other windows

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapChain;

	if (vkCreateSwapchainKHR(pVulkanDevice, &createInfo, nullptr, &pSwapChain) != VK_SUCCESS) {
		throw std::runtime_error("Swapchain creation failed!");
	}

	vkGetSwapchainImagesKHR(pVulkanDevice, pSwapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(pVulkanDevice, pSwapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void RenderBackend::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); ++i) {
		swapChainImageViews[i] = createImageView2D(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void RenderBackend::createPipeline() {
	VkViewport viewport = {
		0.0f,
		0.0f,
		static_cast<float>(swapChainExtent.width),
		static_cast<float>(swapChainExtent.height),
		0.0f,
		1.0f
	};
	std::cout << __FUNCTION__ << "-> main rendering pipeline" << std::endl;
	pGraphicsPipeline = std::make_unique<GraphicsPipeline>(viewport);
	pGraphicsPipeline->getShaderProgramPtr()->updateDynamicUniformBufferObject(geometry);
}

void RenderBackend::createCommandPool()
{
	auto queueFamilyIndices = getQueueFamilies(pPhysicalDevice);

	VkCommandPoolCreateInfo cmdPoolInfo = {
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		static_cast<uint32_t>(queueFamilyIndices.graphicsFamily)
	};

	if (vkCreateCommandPool(pVulkanDevice, &cmdPoolInfo, nullptr, &pCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("CommandPool creation failed!");
	}
}

void RenderBackend::createDepthResources() {
	auto depthFormat = getDepthFormat();

	const auto size = swapChainImages.size();
	const auto width = swapChainExtent.width;
	const auto height = swapChainExtent.height;

	depthImages.resize(size);
	depthImageViews.resize(size);
	depthImageMemory.resize(size);

	for (size_t i = 0; i < size; ++i) {
		depthImageMemory[i] = new vkExt::SharedMemory();
		createImage2D(width, height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImages[i], depthImageMemory[i]);
		depthImageViews[i] = createImageView2D(depthImages[i].image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		transitionImageLayout(depthImages[i].image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
}

void RenderBackend::createSamplerDescriptorSet() {
	VkDescriptorSetLayoutBinding vertSamplerBinding = {
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_VERTEX_BIT,
		nullptr
	};

	VkDescriptorSetLayoutBinding fragSamplerBinding = {
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1,
		&fragSamplerBinding
	};

	if (vkCreateDescriptorSetLayout(pVulkanDevice, &layoutInfo, nullptr, &pSamplerSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("DescriptorSetLayout creation failed!");
	}
}


// Setup Code for DearImGui
void RenderBackend::setupGui() {
	windowData = {};
	windowData.Surface = pSurface;

	const uint32_t queueFam = getQueueFamilies(pPhysicalDevice).graphicsFamily;

	windowData.SurfaceFormat = surfaceFormat;
	windowData.PresentMode = presentMode;
	ImGui_ImplVulkanH_CreateWindowDataCommandBuffers(pPhysicalDevice, pVulkanDevice, queueFam, &windowData, nullptr);
	windowData.Swapchain = pSwapChain;
	windowData.ClearEnable = false;
	windowData.Width = swapChainExtent.width;
	windowData.Height = swapChainExtent.height;
	windowData.BackBufferCount = static_cast<uint32_t>(swapChainImages.size());
	windowData.BackBuffer = swapChainImages.data();
	windowData.BackBufferView = swapChainImageViews.data();
	windowData.Framebuffer = new VkFramebuffer[swapChainImages.size()];

	ImGui_ImplVulkanH_CreateWindowDataSwapChainAndFramebuffer(pPhysicalDevice, pVulkanDevice, &windowData, nullptr, swapChainExtent.width, swapChainExtent.height);

	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
	poolInfo.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(poolSizes));
	poolInfo.pPoolSizes = poolSizes;

	checkVkResult(vkCreateDescriptorPool(pVulkanDevice, &poolInfo, nullptr, &pGuiDescriptorPool));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	imguiIo = ImGui::GetIO();

	ImGui_ImplGlfw_InitForVulkan(pWindow, true);

	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = pVulkanInstance;
	initInfo.PhysicalDevice = pPhysicalDevice;
	initInfo.Device = pVulkanDevice;
	initInfo.QueueFamily = queueFam;
	initInfo.Queue = pGraphicsQueue;
	initInfo.PipelineCache = VK_NULL_HANDLE;
	initInfo.DescriptorPool = pGuiDescriptorPool;
	initInfo.Allocator = VK_NULL_HANDLE;
	initInfo.CheckVkResultFn = checkVkResult;
	ImGui_ImplVulkan_Init(&initInfo, windowData.RenderPass);

	ImGui::StyleColorsDark();

	VkCommandPool commandPool = windowData.Frames[windowData.FrameIndex].CommandPool;
	VkCommandBuffer cmdBuff = windowData.Frames[windowData.FrameIndex].CommandBuffer;

	checkVkResult(vkResetCommandPool(pVulkanDevice, commandPool, 0));
	VkCommandBufferBeginInfo begin = {};
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	checkVkResult(vkBeginCommandBuffer(cmdBuff, &begin));

	ImGui_ImplVulkan_CreateFontsTexture(cmdBuff);

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmdBuff;

	checkVkResult(vkEndCommandBuffer(cmdBuff));
	checkVkResult(vkQueueSubmit(pGraphicsQueue, 1, &submit, nullptr));

	checkVkResult(vkDeviceWaitIdle(pVulkanDevice));
	ImGui_ImplVulkan_InvalidateFontUploadObjects();

}

// Load some default settings for the Application.
// TODO: todo: put in config ini or similar
void RenderBackend::setupApplicationData(std::string config) {

	fragmentShaderSettings.ambient = 0.3f;
	fragmentShaderSettings.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	fragmentShaderSettings.lightDirection = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	fragmentShaderSettings.shininess = 0.5f;

}

void RenderBackend::destroyCommandBuffers() {
	vkFreeCommandBuffers(pVulkanDevice, pCommandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
}

void RenderBackend::recreateDrawCommandBuffers() {
	vkWaitForFences(pVulkanDevice, MAX_FRAMES_IN_FLIGHT, inFlightFences.data(), VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkDeviceWaitIdle(pVulkanDevice);
	for (auto& cmdBuff : commandBuffers) {
		vkResetCommandBuffer(cmdBuff, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	}
	recordCommandBuffers();
}

void RenderBackend::recreateAllCommandBuffers() {
	recreateDrawCommandBuffers();
}

void RenderBackend::createCommandBuffers() {
	//commandBuffers.resize(pMainShaderProgram->getFramebufferPtrs().size());

	VkCommandBufferAllocateInfo allocInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		pCommandPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		(uint32_t)commandBuffers.size()
	};

	if (vkAllocateCommandBuffers(pVulkanDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("CommandBuffer allocation failed!");
	}

	recordCommandBuffers();
}

// Record Command Buffers for main geometry
void RenderBackend::recordCommandBuffers() {
	auto swapChainFramebuffersRef = pGraphicsPipeline->getFramebufferPtrs();

	for (size_t i = 0; i < commandBuffers.size(); ++i) {
		VkCommandBufferBeginInfo info = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
			nullptr
		};

		if (vkBeginCommandBuffer(commandBuffers[i], &info) != VK_SUCCESS) {
			throw std::runtime_error("Begin command buffer recording failed!");
		}

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = cClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo = {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			nullptr,
			pGraphicsPipeline->getRenderPassPtr(),
			swapChainFramebuffersRef[i],
			{
				{ 0, 0 },
				swapChainExtent
			},
			static_cast<uint32_t>(clearValues.size()),
			clearValues.data()
		};

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pGraphicsPipeline->getVkGraphicsPipelinePtr());
		if (pDrawBuffer.buffer) {
			VkBuffer vtxBuffers[] = { pDrawBuffer.buffer };
			vkCmdBindIndexBuffer(commandBuffers[i], pDrawBuffer.buffer, indexBufferOffset, VK_INDEX_TYPE_UINT32);

			const auto shaderProgram = pGraphicsPipeline->getShaderProgramPtr();

			for (auto j = 0; j < geometry.size(); ++j) {
				auto& mesh = geometry[j];

				VkDeviceSize offsets[] = { mesh->bufferOffset.vertexOffs };
				vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vtxBuffers, offsets);

				uint32_t dynamicOffsets[] = { j * shaderProgram->getDynamicAlignment(), j * shaderProgram->getDynamicStatusAlignment() };

				std::vector<VkDescriptorSet> sets;
				sets.push_back(pGraphicsPipeline->getDescriptorSetPtr());
				sets.push_back(mesh->getDescriptorSet());
				if (pCamera) {
					vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pGraphicsPipeline->getPipelineLayoutPtr(), 0, static_cast<uint32_t>(sets.size()), sets.data(), 2, dynamicOffsets);
					vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(mesh->size()), 1, static_cast<uint32_t>(mesh->bufferOffset.indexOffs), 0, 0);
				}
			}
		}
		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("End command buffer recording failed!");
		}
	}
}

void RenderBackend::createSyncObjects() {
	semImageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
	semRenderFinished.resize(MAX_FRAMES_IN_FLIGHT);
	semGuiRendered.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semInfo = {
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};
	VkFenceCreateInfo fenceInfo = {
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		nullptr,
		VK_FENCE_CREATE_SIGNALED_BIT
	};

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (vkCreateSemaphore(pVulkanDevice, &semInfo, nullptr, &semImageAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(pVulkanDevice, &semInfo, nullptr, &semRenderFinished[i]) != VK_SUCCESS ||
			vkCreateSemaphore(pVulkanDevice, &semInfo, nullptr, &semGuiRendered[i]) != VK_SUCCESS ||
			vkCreateFence(pVulkanDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Synchronization obejct creation failed!");
		}
	}
}

void RenderBackend::setupDebugCallback() {
	if (!enableValidationLayers) return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags =
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_DEBUG_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if (CreateDebugReportCallbackEXT(pVulkanInstance, &createInfo, nullptr, &pCallback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set debug report callback");
	}
}

void RenderBackend::createDrawBuffer() {
	const auto size = maxVertexSize + maxIndexSize;
	indexBufferOffset = maxVertexSize;
	lastVertexOffset = 0;
	lastIndexOffset = 0;

	ppDrawMemory = new vkExt::SharedMemory();
	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pDrawBuffer, ppDrawMemory);

}

void RenderBackend::increaseDrawBufferSize(VkDeviceSize newVertLimit, VkDeviceSize newIndexLimit) {
	const auto oldIndexOffset = indexBufferOffset;
	maxVertexSize = newVertLimit;
	maxIndexSize = newIndexLimit;
	indexBufferOffset = maxVertexSize;
	const auto size = maxVertexSize + maxIndexSize;

	vkExt::Buffer temp;
	auto* tempMem = new vkExt::SharedMemory();

	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, temp, tempMem);

	if (lastVertexOffset > 0) {
		temp.copyToBuffer(pCommandPool, pGraphicsQueue, pDrawBuffer, lastVertexOffset);
	}
	if (lastIndexOffset > 0) {
		temp.copyToBuffer(pCommandPool, pGraphicsQueue, pDrawBuffer, lastIndexOffset, oldIndexOffset, indexBufferOffset);
	}

	pDrawBuffer.destroy(true);
	const auto pOldMem = ppDrawMemory;
	ppDrawMemory = tempMem;
	delete (pOldMem);

	pDrawBuffer.buffer = temp.buffer;
	pDrawBuffer.device = pVulkanDevice;
	pDrawBuffer.usageFlags = temp.usageFlags;
	pDrawBuffer.memory = ppDrawMemory;
	pDrawBuffer.memoryOffset = 0;
	pDrawBuffer.setupDescriptor();
}


Geometry::Mesh::BufferOffset RenderBackend::storeMesh(const Geometry::Mesh* mesh) {
	const auto indexOffset = lastIndexOffset / sizeof(uint32_t);
	const auto vertexOffset = lastVertexOffset;

	const auto vertSize = mesh->vertices.size() * sizeof(Geometry::Vertex);
	const auto indSize = mesh->indices.size() * sizeof(uint32_t);
	const auto totalVertSize = vertSize + lastVertexOffset;
	const auto totalIndexSize = indSize + lastIndexOffset;
	if (totalVertSize + totalIndexSize > maxVertexSize + maxIndexSize) {
		increaseDrawBufferSize(2 * totalVertSize, 2 * totalIndexSize);
	}

	vkExt::Buffer stagingBuffer;
	auto* stagingMemory = new vkExt::SharedMemory();

	createBuffer(vertSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);
	// fill data
	stagingBuffer.map();
	stagingBuffer.copyTo(mesh->vertices.data(), (size_t)vertSize);
	stagingBuffer.unmap();

	pDrawBuffer.copyToBuffer(pCommandPool, pGraphicsQueue, stagingBuffer, vertSize, 0, lastVertexOffset);
	lastVertexOffset += vertSize;

	stagingBuffer.destroy(true);
	createBuffer(indSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);

	stagingBuffer.map();
	stagingBuffer.copyTo(mesh->indices.data(), (size_t)indSize);
	stagingBuffer.unmap();

	pDrawBuffer.copyToBuffer(pCommandPool, pGraphicsQueue, stagingBuffer, indSize, 0, indexBufferOffset + lastIndexOffset);
	lastIndexOffset += indSize;

	stagingBuffer.destroy(true);
	delete(stagingMemory);

	Geometry::Mesh::BufferOffset offset = { vertexOffset, indexOffset };
	return offset;
}

VkCommandBuffer RenderBackend::beginOneTimeCommand() const {
	VkCommandBufferAllocateInfo allocInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		pCommandPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		1
	};

	VkCommandBuffer buffer;
	if (vkAllocateCommandBuffers(pVulkanDevice, &allocInfo, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("BeginOneTimeCommand failed!");
	}

	VkCommandBufferBeginInfo begin = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		nullptr
	};

	vkBeginCommandBuffer(buffer, &begin);

	return buffer;
}

void RenderBackend::endOneTimeCommand(VkCommandBuffer buffer) const {
	vkEndCommandBuffer(buffer);

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &buffer;

	vkQueueSubmit(pGraphicsQueue, 1, &info, VK_NULL_HANDLE);
	vkQueueWaitIdle(pGraphicsQueue);

	vkFreeCommandBuffers(pVulkanDevice, pCommandPool, 1, &buffer);
}

void RenderBackend::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, vkExt::Buffer& buffer, vkExt::SharedMemory* bufferMemory) const {
	VkBuffer tmpBuffer;
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(pVulkanDevice, &bufferInfo, nullptr, &tmpBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Buffer creation failed!");
	}

	VkDeviceMemory deviceMem;
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(pVulkanDevice, tmpBuffer, &memReqs);

	VkMemoryAllocateInfo memInfo = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		memReqs.size,
		findMemoryType(memReqs.memoryTypeBits, properties)
	};

	if (vkAllocateMemory(pVulkanDevice, &memInfo, nullptr, &deviceMem) != VK_SUCCESS) {
		vkDestroyBuffer(pVulkanDevice, tmpBuffer, nullptr); // make sure to cleanup here!
		throw std::runtime_error("Buffer Memory allocation failed!");
	}

	bufferMemory->memory = deviceMem;
	bufferMemory->size = memReqs.size;
	bufferMemory->isAlive = true;

	buffer.buffer = tmpBuffer;
	buffer.device = pVulkanDevice;
	buffer.usageFlags = usage;
	buffer.memory = bufferMemory;
	buffer.memoryOffset = 0;
	buffer.setupDescriptor();

	buffer.bind();
}

void RenderBackend::allocateMemory(VkDeviceSize size, VkMemoryPropertyFlags properties, uint32_t memoryTypeFilterBits, vkExt::SharedMemory* memory) const {
	VkMemoryAllocateInfo allocInfo = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		size,
		findMemoryType(memoryTypeFilterBits, properties)
	};

	const auto result = vkAllocateMemory(pVulkanDevice, &allocInfo, nullptr, &memory->memory);
	if (result != VK_SUCCESS) {
		if (result == VK_ERROR_TOO_MANY_OBJECTS) {
			throw std::runtime_error("Too many memory allocations");
		}
		throw std::runtime_error("Memory allocation for image failed!");
	}
	memory->size = size;
	memory->isAlive = true;
}

void RenderBackend::createImage2D(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, vkExt::Image& image, vkExt::SharedMemory* imageMemory, VkDeviceSize memOffset, VkImageLayout initialLayout /*= VK_IMAGE_LAYOUT_UNDEFINED*/) {
	VkImage vkimage;

	VkImageFormatProperties props;
	vkGetPhysicalDeviceImageFormatProperties(pPhysicalDevice, format, VK_IMAGE_TYPE_2D, tiling, usage, 0, &props);

	VkImageCreateInfo imageInfo = {
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		nullptr,
		0,
		VK_IMAGE_TYPE_2D,
		format,
		{ width, height, 1 },
		1,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		tiling,
		usage,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		initialLayout
	};

	if (vkCreateImage(pVulkanDevice, &imageInfo, nullptr, &vkimage) != VK_SUCCESS) {
		throw std::runtime_error("Image creation failed!");
	}
	deviceCreatedImages.push_back(vkimage);

	image.device = pVulkanDevice;
	image.image = vkimage;
	image.width = width;
	image.height = height;
	image.memory = imageMemory;
	image.imageFormat = format;

	// skip memory allocation. 
	if (imageMemory == nullptr) return;

	if (imageMemory->isAlive) {
		image.bind(memOffset);
	}
	else {
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(pVulkanDevice, vkimage, &memReqs);

		VkMemoryAllocateInfo allocInfo = {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			memReqs.size,
			findMemoryType(memReqs.memoryTypeBits, properties)
		};

		if (vkAllocateMemory(pVulkanDevice, &allocInfo, nullptr, &imageMemory->memory) != VK_SUCCESS) {
			throw std::runtime_error("Memory allocation for image failed!");
		}
		imageMemory->size = memReqs.size;
		imageMemory->isAlive = true;
		image.bind();
	}
}

VkImageView RenderBackend::createImageView2D(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo createInfo = {
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		image,
		VK_IMAGE_VIEW_TYPE_2D,
		format,
		{
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		},
		{
			aspectFlags,
			0,
			1,
			0,
			1
		}
	};

	VkImageView view;
	if (vkCreateImageView(pVulkanDevice, &createInfo, nullptr, &view) != VK_SUCCESS) {
		throw std::runtime_error("ImageView creation failed!");
	}
	deviceCreatedImageViews.push_back(view);
	return view;
}

void RenderBackend::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuff /* = nullptr */) const {
	VkCommandBuffer cmdbuff = commandBuff ? commandBuff : beginOneTimeCommand();
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	barrier.subresourceRange.aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) ?
		formatHasStencilComponent(format) ?
		VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT
		: VK_IMAGE_ASPECT_COLOR_BIT;

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else {
		throw std::invalid_argument("Layout transition not supported");
	}

	vkCmdPipelineBarrier(
		cmdbuff,
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	if (!commandBuff) {
		endOneTimeCommand(cmdbuff);
	}
}

RequiredQueueFamilyIndices RenderBackend::getQueueFamilies(VkPhysicalDevice device) const {
	RequiredQueueFamilyIndices indices;

	uint32_t queueFamCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount == 0) continue;

		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = static_cast<int>(i);
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, static_cast<uint32_t>(i), pSurface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.allPresent()) {
			break;
		}
		++i;
	}

	return indices;
}

SwapChainSupportInfo RenderBackend::getSwapChainSupportInfo(VkPhysicalDevice device) const {
	SwapChainSupportInfo info;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, pSurface, &info.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, pSurface, &formatCount, nullptr);

	if (formatCount > 0) {
		info.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, pSurface, &formatCount, info.formats.data());
	}

	uint32_t presentModesCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, pSurface, &presentModesCount, nullptr);

	if (presentModesCount > 0) {
		info.presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, pSurface, &presentModesCount, info.presentModes.data());
	}

	return info;
}

VkFormat RenderBackend::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const {
	for (auto& format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(pPhysicalDevice, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("No supported Format found!");
}

VkSurfaceFormatKHR RenderBackend::getBestSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) { // best case: we are free to choose
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& format : availableFormats) {
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { // try to find our prefered format
			return format;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR RenderBackend::getBestPresentMode(const std::vector<VkPresentModeKHR>& availableModes) {
	auto bestMode = VK_PRESENT_MODE_FIFO_KHR; // similar to vsync mode in opengl
	for (const auto& mode : availableModes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) { // tripple buffering
			return mode;
		}
		else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) { // images transferred to screen as they are rendered..may have tearing
			bestMode = mode;
		}
	}

	return bestMode;
}

VkExtent2D RenderBackend::getSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		auto width = WINDOW_WIDTH;
		auto height = WINDOW_HEIGHT;
		glfwGetWindowSize(pWindow, &width, &height);
		VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
		extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));

		return extent;
	}
}

size_t RenderBackend::checkDeviceRequirements(VkPhysicalDevice device) const {
	size_t score = 0;

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);

	VkPhysicalDeviceFeatures feats;
	vkGetPhysicalDeviceFeatures(device, &feats);

	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, deviceExtensions.data());

	if (requiredFeatures.tessellationShader > feats.tessellationShader) {
		return DEVICE_NOT_SUITABLE;
	}

	VkSurfaceCapabilitiesKHR capabilities;
	const auto res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, pSurface, &capabilities);
	if (res != VK_SUCCESS || !(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
		return DEVICE_NOT_SUITABLE;
	}

	std::set<std::string> required(requiredExtensions.begin(), requiredExtensions.end());
	for (const auto& ext : deviceExtensions) {
		required.erase(ext.extensionName);
	}

	if (!required.empty()) {
		return DEVICE_NOT_SUITABLE;
	}

	auto queueFamilyIndices = getQueueFamilies(device);
	auto swapChainInfo = getSwapChainSupportInfo(device);


	if (!(queueFamilyIndices.allPresent() && !swapChainInfo.formats.empty() && !swapChainInfo.presentModes.empty())) {
		return DEVICE_NOT_SUITABLE;
	}

	if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	score += props.limits.maxImageDimension2D;

	return score;
}

bool RenderBackend::checkValidationLayerSupport() const {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const auto* layer : validationLayers) {
		auto found = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layer, layerProperties.layerName) == 0) {
				found = true;
				break;
			}
		}

		if (!found) {
			return false;
		}
	}

	return true;
}

std::vector<const char*> RenderBackend::getRequiredExtensions() const {
	uint32_t glfwExtCnt = 0;
	const auto glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCnt);

	std::vector<const char*> exts(glfwExts, glfwExts + glfwExtCnt);

	if (enableValidationLayers) {
		exts.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return exts;
}

uint32_t RenderBackend::findMemoryType(uint32_t filter, VkMemoryPropertyFlags properties) const {
	VkPhysicalDeviceMemoryProperties physMem;
	vkGetPhysicalDeviceMemoryProperties(pPhysicalDevice, &physMem);

	for (uint32_t i = 0; i < physMem.memoryTypeCount; ++i) {
		if ((filter & (1 << i)) && (physMem.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("Could not find suitable memory type!");
}


VKAPI_ATTR VkBool32 VKAPI_CALL RenderBackend::debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{

	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}

void RenderBackend::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	// ignore mouse events if:
	if (ImGui::IsAnyItemHovered() || ImGui::IsAnyWindowHovered() || imguiHandlesMouse || !acceptingInput) return;
	if (action == GLFW_RELEASE) {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			//acceptingInput = false;
			message.clear();
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			acceptingInput = false;
			message.clear();
		}
	}
}

VkResult CreateDebugReportCallbackEXT(
	VkInstance instance,
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugReportCallbackEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}
void DestroyDebugReportCallbackEXT(
	VkInstance instance,
	VkDebugReportCallbackEXT callback,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func) {
		func(instance, callback, pAllocator);
	}
