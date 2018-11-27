#include "Shader.h"
#include "Geometry.h"
#include "FileReader.h"
#include "Application.h"

#ifndef _WINDOWS
#define _aligned_malloc(size, alignment) aligned_alloc(alignment, size)
#define _aligned_free(obj) free(obj)
#endif

using namespace Engine::Shaders;

MainShaderProgram::MainShaderProgram(VkViewport targetViewport, const std::string& vtxShaderFile, const std::string& tescShaderFile, const std::string& teseShaderFile, const std::string& fragShaderFile, size_t objectInstances, ShaderStages enabledStages) {
	pUniformBufferMemory = new vkExt::SharedMemory();
	pDynamicBufferMemory = new vkExt::SharedMemory();
	pDynamicStatusMemory = new vkExt::SharedMemory();
	objectCount = objectInstances;
	viewport = targetViewport;
	withTesselation = false;
	if (enabledStages.vert) {
		vtxShaderCode = Tools::FileReader::readFile(vtxShaderFile);
	}
	if (enabledStages.tes) {
		tescShaderCode = Tools::FileReader::readFile(tescShaderFile);
		teseShaderCode = Tools::FileReader::readFile(teseShaderFile);
		withTesselation = true;
	}
	if (enabledStages.frag) {
		fragShaderCode = Tools::FileReader::readFile(fragShaderFile);
	}
	create(enabledStages);
}

MainShaderProgram::~MainShaderProgram() {
	MainShaderProgram::cleanup();
}

void MainShaderProgram::create(ShaderStages stages) {
	createDescriptorSetLayout();
	createDescriptorPool();
	createRenderPass();
	createGraphicsPipeline(stages);
	createFramebuffers();
	createUniformBuffer();
	createDescriptorSet();
}

void MainShaderProgram::updateUniformBufferObject(const UBO& ubo)
{
	pUniformBuffer.map();
	pUniformBuffer.copyTo(&ubo, sizeof(ubo));
	pUniformBuffer.unmap();
}

void MainShaderProgram::updateTesselationControlSettings(const TesselationControlSettings& sets) {
	pUniformBuffer.map(teseSettingsOffset);
	pUniformBuffer.copyTo(&sets, sizeof(sets));
	pUniformBuffer.unmap();
}

void MainShaderProgram::updateFragmentShaderSettings(const FragmentShaderSettings& sets) {
	pUniformBuffer.map(fragSettingsOffset);
	pUniformBuffer.copyTo(&sets, sizeof(sets));
	pUniformBuffer.unmap();
}

void MainShaderProgram::cleanup()
{
	ShaderProgram::cleanup();

	auto& app = Engine::App::getHandle();
	auto device = app.getDevice();

	pUniformBuffer.destroy(true);
	if (pUniformBufferMemory) {
		delete(pUniformBufferMemory);
	}
	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	if (pDynamicBuffer.buffer) {
		pDynamicBuffer.destroy(true);
	}
	if (pDynamicBufferMemory) {
		delete(pDynamicBufferMemory);
	}
	if (pDynamicStatusBuffer.buffer) {
		pDynamicStatusBuffer.destroy(true);
	}
	if (pDynamicStatusMemory) {
		delete(pDynamicStatusMemory);
	}

	vkDestroyPipeline(device, pGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pPipelineLayout, nullptr);

	vkDestroyRenderPass(device, pRenderPass, nullptr);
}

void MainShaderProgram::createRenderPass() {
	auto& app = Engine::App::getHandle();
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = app.getImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	const auto depthFormat = app.getDepthFormat();
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttRef = {};
	colorAttRef.attachment = 0;
	colorAttRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttRef = {};
	depthAttRef.attachment = 1;
	depthAttRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttRef;
	subpass.pDepthStencilAttachment = &depthAttRef;


	VkSubpassDependency dependency = {
		VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0
	};

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(app.getDevice(), &renderPassInfo, nullptr, &pRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("RenderPass creation failed!");
	}
}

void MainShaderProgram::createGraphicsPipeline(ShaderStages enabledStages) {
	auto& app = Engine::App::getHandle();
	const auto device = app.getDevice();

	std::vector<VkPipelineShaderStageCreateInfo> stages;
	VkShaderModule vtxShaderModule;
	VkShaderModule tescShaderModule;
	VkShaderModule teseShaderModule;
	VkShaderModule fragShaderModule;

	if (enabledStages.vert) {
		vtxShaderModule = createShaderModule(vtxShaderCode);

		VkPipelineShaderStageCreateInfo vtxStageInfo = {};
		vtxStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vtxStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vtxStageInfo.module = vtxShaderModule;
		vtxStageInfo.pName = "main";

		stages.push_back(vtxStageInfo);
	}
	if (enabledStages.tes) {
		tescShaderModule = createShaderModule(tescShaderCode);
		teseShaderModule = createShaderModule(teseShaderCode);

		VkPipelineShaderStageCreateInfo tescStageInfo = {};
		tescStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		tescStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		tescStageInfo.module = tescShaderModule;
		tescStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo teseStageInfo = {};
		teseStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		teseStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		teseStageInfo.module = teseShaderModule;
		teseStageInfo.pName = "main";

		stages.push_back(tescStageInfo);
		stages.push_back(teseStageInfo);
	}
	if (enabledStages.frag) {
		fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStageInfo.module = fragShaderModule;
		fragStageInfo.pName = "main";

		stages.push_back(fragStageInfo);
	}

	if (stages.empty()) {
		throw std::runtime_error("No shader stages enabled! Cannot create Pipeline without at least one stage");
	}

	auto bindingDescription = Geometry::Vertex::getBindingDescription();
	auto attributeDescriptions = Geometry::Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vtxInputInfo = {};
	vtxInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vtxInputInfo.vertexBindingDescriptionCount = 1;
	vtxInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vtxInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vtxInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo assemblyInfo = {};
	const auto topology = enabledStages.tes == VK_TRUE ? VK_PRIMITIVE_TOPOLOGY_PATCH_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assemblyInfo.topology = topology;
	assemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineTessellationStateCreateInfo tessellationInfo = {};
	tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessellationInfo.patchControlPoints = 3;

	VkRect2D scissor = {
		{ 0, 0 },
		app.getSwapChainExtent()
	};

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &blendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkDescriptorSetLayout descLayouts[] = { pDescriptorSetLayout, app.getSamplerDescriptorSetLayout() };
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts = descLayouts;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("PipelineLayout creation failed!");
	}

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
	pipelineInfo.pStages = stages.data();
	pipelineInfo.pVertexInputState = &vtxInputInfo;
	pipelineInfo.pInputAssemblyState = &assemblyInfo;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pTessellationState = &tessellationInfo;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = pPipelineLayout;
	pipelineInfo.renderPass = pRenderPass;
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(
		device, nullptr, 1, &pipelineInfo, nullptr, &pGraphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Pipeline creation failed!");
	}

	if (enabledStages.vert) {
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
	}
	if (enabledStages.tes) {
		vkDestroyShaderModule(device, teseShaderModule, nullptr);
		vkDestroyShaderModule(device, tescShaderModule, nullptr);
	}
	if (enabledStages.frag) {
		vkDestroyShaderModule(device, vtxShaderModule, nullptr);
	}
}

void MainShaderProgram::createFramebuffers()
{
	auto& app = Engine::App::getHandle();

	auto imageViewsRef = app.getSwapChainImageViewsRef();
	auto depthViewsRef = app.getDepthImageViewsRef();
	const auto extent = app.getSwapChainExtent();

	swapChainFramebuffers.resize(imageViewsRef.size());
	for (size_t i = 0; i < swapChainFramebuffers.size(); ++i) {
		std::array<VkImageView, 2> attachments = {
			imageViewsRef[i],
			depthViewsRef[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr,
			0,
			pRenderPass,
			static_cast<uint32_t>(attachments.size()),
			attachments.data(),
			extent.width,
			extent.height,
			1
		};

		if (vkCreateFramebuffer(app.getDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("ARShader: Framebuffer creation failed!");
		}
	}
}

void MainShaderProgram::createUniformBuffer()
{
	auto& app = Engine::App::getHandle();

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(app.getPhysicalDevice(), &props);
	const auto alignment = props.limits.minUniformBufferOffsetAlignment;

	const auto uboSize = sizeof(UBO);
	if (alignment > 0) {
		teseSettingsOffset = (uboSize + alignment - 1) & ~(alignment - 1);
	}
	else {
		teseSettingsOffset = uboSize;
	}

	const auto teseSize = sizeof(TesselationControlSettings);
	if (alignment > 0) {
		fragSettingsOffset = (teseSettingsOffset + teseSize + alignment - 1) & ~(alignment - 1);
	}
	else {
		fragSettingsOffset = teseSettingsOffset + teseSize;
	}

	const auto fragSize = sizeof(FragmentShaderSettings);

	const auto bufferSize = fragSettingsOffset + fragSize;
	app.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pUniformBuffer, pUniformBufferMemory);
}

void MainShaderProgram::createDynamicBuffer(VkDeviceSize size) {
	auto& app = Engine::App::getHandle();

	if (pDynamicBuffer.buffer) {
		pDynamicBuffer.destroy(true);

		_aligned_free(dynamicUboData.model);
	}
	if (pDynamicStatusBuffer.buffer) {
		pDynamicStatusBuffer.destroy(true);

		_aligned_free(dynamicStatusData.value);
	}

	objectCount = size;

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(app.getPhysicalDevice(), &props);
	const auto alignment = static_cast<uint32_t>(props.limits.minUniformBufferOffsetAlignment);

	dynamicAlignment = static_cast<uint32_t>(sizeof(glm::mat4));
	if (alignment > 0) {
		dynamicAlignment = (dynamicAlignment + alignment - 1) & ~(alignment - 1);
	}
	// if buffer is initialized with empty geometry (objectCount of 0), use 1 alignment as initial size.
	dynamicUboDataSize = objectCount > 0 ? objectCount * dynamicAlignment : dynamicAlignment;
	dynamicUboData.model = (glm::mat4*)_aligned_malloc(dynamicUboDataSize, dynamicAlignment);

	app.createBuffer(dynamicUboDataSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pDynamicBuffer, pDynamicBufferMemory);
	
	pDynamicBuffer.map();

	statusAlignment = static_cast<uint32_t>(sizeof(VkBool32));
	if (alignment > 0) {
		statusAlignment = (statusAlignment + alignment - 1) & ~(alignment - 1);
	}
	if (statusAlignment < (2 * static_cast<uint32_t>(sizeof(VkBool32)))) {
		// todo
		throw std::runtime_error("alignment too small for status dUBO");
	}
	dynamicStatusSize = objectCount > 0 ? objectCount * statusAlignment : statusAlignment;
	dynamicStatusData.value = (VkBool32*)_aligned_malloc(dynamicStatusSize, statusAlignment);

	app.createBuffer(dynamicStatusSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pDynamicStatusBuffer, pDynamicStatusMemory);

	pDynamicStatusBuffer.map();

	updateDescriptorSet();
}

void MainShaderProgram::updateDynamicUniformBufferObject(const std::vector<std::shared_ptr<Engine::Geometry::Mesh>>& meshes) {
	if (!(pDynamicBuffer.buffer && pDynamicStatusBuffer.buffer) || meshes.size() * dynamicAlignment > dynamicUboDataSize) {
		createDynamicBuffer(meshes.size());
	}
	for (auto i = 0; i < meshes.size(); ++i) {
		const auto model = (glm::mat4*)((uint64_t)dynamicUboData.model + i * dynamicAlignment);
		const auto useTex = (VkBool32*)((uint64_t)dynamicStatusData.value + i * statusAlignment);
		const auto selected = (VkBool32*)((uint64_t)dynamicStatusData.value + i * statusAlignment + sizeof(VkBool32));
		*model = glm::mat4(meshes[i]->modelMat());
		*useTex = meshes[i]->getStatus().useTexture;
		*selected = meshes[i]->getStatus().isSelected;
	}
	pDynamicBuffer.copyTo(dynamicUboData.model, dynamicUboDataSize);
	pDynamicBuffer.flush();
	pDynamicStatusBuffer.copyTo(dynamicStatusData.value, dynamicStatusSize);
	pDynamicStatusBuffer.flush();
}

void MainShaderProgram::createDescriptorSetLayout() {
	const VkDescriptorSetLayoutBinding uboBinding = {
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		1,
		VK_SHADER_STAGE_VERTEX_BIT,
		nullptr
	};
	const VkDescriptorSetLayoutBinding modelBinding = {
		1,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		1,
		VK_SHADER_STAGE_VERTEX_BIT,
		nullptr
	};

	const VkShaderStageFlags targetStage = withTesselation ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : VK_SHADER_STAGE_VERTEX_BIT;

	const VkDescriptorSetLayoutBinding distortionParamsBinding = {
		2,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		1,
		targetStage,
		nullptr
	};
	const VkDescriptorSetLayoutBinding teseSettingsBinding = {
		3,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		1,
		targetStage,
		nullptr
	};

	const VkDescriptorSetLayoutBinding fragSettingsBinding = {
		4,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	};

	const VkDescriptorSetLayoutBinding statusBinding = {
		5,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	};

	std::array<VkDescriptorSetLayoutBinding, 6> layoutBindings = { uboBinding, modelBinding, distortionParamsBinding, teseSettingsBinding, fragSettingsBinding, statusBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		static_cast<uint32_t>(layoutBindings.size()),
		layoutBindings.data()
	};

	if (vkCreateDescriptorSetLayout(Engine::App::getHandle().getDevice(), &layoutInfo, nullptr, &pDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("DescriptorSetLayout creation failed!");
	}
}

void MainShaderProgram::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> sizes = {};
	sizes[0] = {
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		4
	};
	sizes[1] = {
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		2
	};

	VkDescriptorPoolCreateInfo info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		0,
		1,
		static_cast<uint32_t>(sizes.size()),
		sizes.data()
	};

	if (vkCreateDescriptorPool(Engine::App::getHandle().getDevice(), &info, nullptr, &pDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("DescriptorPool creation failed!");
	}
}

void MainShaderProgram::createDescriptorSet() {
	auto& app = Engine::App::getHandle();
	VkDescriptorSetLayout layouts[] = { pDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		pDescriptorPool,
		1,
		layouts
	};

	if (vkAllocateDescriptorSets(app.getDevice(), &allocInfo, &pDescriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("DescriptorSet allocation failed!");
	}
}

void MainShaderProgram::updateDescriptorSet() {
	VkDescriptorBufferInfo uboModel = {
		pUniformBuffer.buffer,
		0,
		sizeof(UBO)
	};

	VkDescriptorBufferInfo teseSettingsModel = {
		pUniformBuffer.buffer,
		teseSettingsOffset,
		sizeof(TesselationControlSettings)
	};

	VkDescriptorBufferInfo fragSettingsModel = {
		pUniformBuffer.buffer,
		fragSettingsOffset,
		sizeof(FragmentShaderSettings)
	};

	std::vector<VkWriteDescriptorSet> write;

	const VkWriteDescriptorSet ubo = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		nullptr,
		pDescriptorSet,
		0,
		0,
		1,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		nullptr,
		&uboModel,
		nullptr
	};
	write.push_back(ubo);

	if (pDynamicBuffer.buffer) {
		const VkWriteDescriptorSet dyn = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			pDescriptorSet,
			1,
			0,
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			nullptr,
			&pDynamicBuffer.descriptor,
			nullptr
		};
		write.push_back(dyn);
	}

	if (pDynamicStatusBuffer.buffer) {
		const VkWriteDescriptorSet dyn = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			pDescriptorSet,
			5,
			0,
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			nullptr,
			&pDynamicStatusBuffer.descriptor,
			nullptr
		};
		write.push_back(dyn);
	}

	const VkWriteDescriptorSet tese = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		nullptr,
		pDescriptorSet,
		3,
		0,
		1,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		nullptr,
		&teseSettingsModel,
		nullptr
	};
	write.push_back(tese);

	const VkWriteDescriptorSet frag = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		nullptr,
		pDescriptorSet,
		4,
		0,
		1,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		nullptr,
		&fragSettingsModel,
		nullptr
	};
	write.push_back(frag);

	vkUpdateDescriptorSets(Engine::App::getHandle().getDevice(), static_cast<uint32_t>(write.size()), write.data(), 0, nullptr);
}

void ShaderProgram::cleanup() {
	vkDestroyDescriptorPool(Engine::App::getHandle().getDevice(), pDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(Engine::App::getHandle().getDevice(), pDescriptorSetLayout, nullptr);
}

VkShaderModule ShaderProgram::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(Engine::App::getHandle().getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Shader Module creation failed!");
	}
	return shaderModule;
}
