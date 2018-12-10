#include "GraphicsPipeline.h"
#include "Application.h"
#include "Geometry.h"

#include <vulkan/vulkan.h>
#include <VulkanExtension.h>

using namespace Engine;

void GraphicsPipeline::initPipeline()
{
	auto& app = App::getHandle();
	const auto& device = app.getDevice();

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

	VkAttachmentReference colorAttRef;
	colorAttRef.attachment = 0;
	colorAttRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttRef;
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

	VK_THROW_ON_ERROR(vkCreateRenderPass(app.getDevice(), &renderPassInfo, nullptr, &pRenderPass), "RenderPass creation failed!");

	// create shader modules
	shader = std::make_unique<Shaders::ShaderProgram>("shaders/shader.vert.spv", "", "", "shaders/shader.frag.spv", 0);

	std::vector<VkPipelineShaderStageCreateInfo> stages = shader->getShaderStages();

	auto bindingDescription = Geometry::Vertex::getBindingDescription();
	auto attributeDescriptions = Geometry::Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vtxInputInfo = {};
	vtxInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vtxInputInfo.vertexBindingDescriptionCount = 1;
	vtxInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vtxInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vtxInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo assemblyInfo = {};
	const auto topology = shader->tesselationEnabled() ? VK_PRIMITIVE_TOPOLOGY_PATCH_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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

	std::vector <VkDescriptorSetLayoutBinding> bindings;

	const VkDescriptorSetLayoutBinding uboBinding = {
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		1,
		VK_SHADER_STAGE_VERTEX_BIT,
		nullptr
	};
	bindings.push_back(uboBinding);

	const VkDescriptorSetLayoutBinding modelBinding = {
		1,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		1,
		VK_SHADER_STAGE_VERTEX_BIT,
		nullptr
	};
	bindings.push_back(modelBinding);

	if (shader->tesselationEnabled()) {
		const VkDescriptorSetLayoutBinding teseSettingsBinding = {
			4,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
			nullptr
		};
		bindings.push_back(teseSettingsBinding);
	}

	const VkDescriptorSetLayoutBinding fragSettingsBinding = {
		2,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	};
	bindings.push_back(fragSettingsBinding);

	const VkDescriptorSetLayoutBinding colorTypeBinding = {
		3,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	};
	bindings.push_back(colorTypeBinding);

	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		static_cast<uint32_t>(bindings.size()),
		bindings.data()
	};

	VK_THROW_ON_ERROR(vkCreateDescriptorSetLayout(Engine::App::getHandle().getDevice(), &layoutInfo, nullptr, &pDescriptorSetLayout), "DescriptorSetLayout creation failed!");

	std::array<VkDescriptorPoolSize, 2> sizes = {};
	sizes[0] = {
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		3
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

	VK_THROW_ON_ERROR(vkCreateDescriptorPool(Engine::App::getHandle().getDevice(), &info, nullptr, &pDescriptorPool), "DescriptorPool creation failed!");

	VkDescriptorSetLayout layouts[] = { pDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		pDescriptorPool,
		1,
		layouts
	};

	VK_THROW_ON_ERROR(vkAllocateDescriptorSets(app.getDevice(), &allocInfo, &pDescriptorSet), "DescriptorSet allocation failed!");

	updateDescriptorSets();

	VkDescriptorSetLayout descLayouts[] = { pDescriptorSetLayout, app.getSamplerDescriptorSetLayout() };
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts = descLayouts;

	VK_THROW_ON_ERROR(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pPipelineLayout), "PipelineLayout creation failed!");

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

	VK_THROW_ON_ERROR(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &pGraphicsPipeline), "Pipeline creation failed!");

	const auto& imageViewsRef = app.getSwapChainImageViewsRef();
	const auto& depthViewsRef = app.getDepthImageViewsRef();
	const auto& extent = app.getSwapChainExtent();

	swapChainFramebuffers.resize(imageViewsRef.size());
	for (size_t i = 0; i < swapChainFramebuffers.size(); ++i) {
		std::array<VkImageView, 2> fbAttachments = {
			imageViewsRef[i],
			depthViewsRef[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr,
			0,
			pRenderPass,
			static_cast<uint32_t>(attachments.size()),
			fbAttachments.data(),
			extent.width,
			extent.height,
			1
		};

		VK_THROW_ON_ERROR(vkCreateFramebuffer(app.getDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]), "ARShader: Framebuffer creation failed!");
	}
}

void GraphicsPipeline::updateDescriptorSets() const
{
	auto[uboModel, tessModel, fragModel] = shader->getDescriptorInfos();

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

	if (shader->pDynamicBuffer.buffer) {
		const VkWriteDescriptorSet dyn = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			pDescriptorSet,
			1,
			0,
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			nullptr,
			&shader->pDynamicBuffer.descriptor,
			nullptr
		};
		write.push_back(dyn);
	}

	if (shader->pDynamicVertexTypeBuffer.buffer) {
		const VkWriteDescriptorSet dyn = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			pDescriptorSet,
			3,
			0,
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			nullptr,
			&shader->pDynamicVertexTypeBuffer.descriptor,
			nullptr
		};
		write.push_back(dyn);
	}

	if (shader->tesselationEnabled()) {
		const VkWriteDescriptorSet tese = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			pDescriptorSet,
			4,
			0,
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			nullptr,
			&tessModel,
			nullptr
		};
		write.push_back(tese);
	}

	const VkWriteDescriptorSet frag = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		nullptr,
		pDescriptorSet,
		2,
		0,
		1,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		nullptr,
		&fragModel,
		nullptr
	};
	write.push_back(frag);

	vkUpdateDescriptorSets(App::getHandle().getDevice(), static_cast<uint32_t>(write.size()), write.data(), 0, nullptr);
}

void GraphicsPipeline::cleanup() {
	const auto& app = App::getHandle();
	const auto device = app.getDevice();

//	vkFreeDescriptorSets(device, pDescriptorPool, 1, &pDescriptorSet);
	vkDestroyDescriptorSetLayout(device, pDescriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, pDescriptorPool, nullptr);

	vkDestroyPipeline(device, pGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pPipelineLayout, nullptr);
	vkDestroyRenderPass(device, pRenderPass, nullptr);

	for (auto& framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	shader->cleanup();
	shader.reset();
}
