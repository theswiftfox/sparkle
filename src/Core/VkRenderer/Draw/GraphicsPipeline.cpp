#include "GraphicsPipeline.h"
#include "Application.h"
#include "Geometry.h"
#include "Material.h"

#include "VulkanInitializers.h"
#include <VulkanExtension.h>
#include <vulkan/vulkan.h>

using namespace Sparkle;

void DeferredDraw::initPipelines()
{
	auto renderer = App::getHandle().getRenderBackend();
	const auto& device = renderer->getDevice();
	const auto& imageViewsRef = renderer->getSwapChainImageViewsRef();
	const auto& depthViewsRef = renderer->getDepthImageViewsRef();
	const auto& extent = renderer->getSwapChainExtent();
	const auto depthFormat = renderer->getDepthFormat();

	// MRT framebuffers
	{
		std::vector<VkAttachmentReference> colorReferences;
		colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		colorReferences.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		colorReferences.push_back({ 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

		VkAttachmentReference depthReference = { 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments = colorReferences.data();
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
		subpass.pDepthStencilAttachment = &depthReference;

		// subpass dependencies for attachment layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// init attachments
		offscreenFramebuffers.resize(imageViewsRef.size());
		for (auto& framebuffer : offscreenFramebuffers) {
			framebuffer.extent = extent;
			initAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &framebuffer.position);
			initAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &framebuffer.normal);
			initAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &framebuffer.albedo);
			initAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &framebuffer.pbrSpecular);
			initAttachment(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &framebuffer.depth);
		}

		std::array<VkAttachmentDescription, 5> attDescs = {};
		for (auto j = 0u; j < 5; ++j) {
			attDescs[j].samples = VK_SAMPLE_COUNT_1_BIT;
			attDescs[j].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attDescs[j].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attDescs[j].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attDescs[j].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			if (j == 4) { // depth att
				attDescs[j].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			} else {
				attDescs[j].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
		}

		attDescs[0].format = offscreenFramebuffers[0].position.format;
		attDescs[1].format = offscreenFramebuffers[0].normal.format;
		attDescs[2].format = offscreenFramebuffers[0].albedo.format;
		attDescs[3].format = offscreenFramebuffers[0].pbrSpecular.format;
		attDescs[4].format = offscreenFramebuffers[0].depth.format;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attDescs.size());
		renderPassInfo.pAttachments = attDescs.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = dependencies.data();

		VK_THROW_ON_ERROR(vkCreateRenderPass(device, &renderPassInfo, nullptr, &mrtRenderPass), "Unable to create RenderPass for MRT!");

		for (auto& fb : offscreenFramebuffers) {
			std::array<VkImageView, 5> attachments = {
				fb.position.view,
				fb.normal.view,
				fb.albedo.view,
				fb.pbrSpecular.view,
				fb.depth.view
			};

			VkFramebufferCreateInfo fbi = {};
			fbi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbi.pNext = nullptr;
			fbi.renderPass = mrtRenderPass;
			fbi.pAttachments = attachments.data();
			fbi.attachmentCount = static_cast<uint32_t>(attachments.size());
			fbi.width = fb.extent.width;
			fbi.height = fb.extent.height;
			fbi.layers = 1;

			VK_THROW_ON_ERROR(vkCreateFramebuffer(device, &fbi, nullptr, &fb.framebuffer), "Framebuffer creation failed for MRT");
		}
		auto samplerInfo = vk::init::samplerCreateInfo();
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = samplerInfo.addressModeU;
		samplerInfo.addressModeW = samplerInfo.addressModeU;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_THROW_ON_ERROR(vkCreateSampler(device, &samplerInfo, nullptr, &colorSampler), "Sampler creation failed for ColorSampler");
	}

	// Deferred Pipeline
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = renderer->getImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

		VK_THROW_ON_ERROR(vkCreateRenderPass(device, &renderPassInfo, nullptr, &deferredRenderPass), "RenderPass creation failed!");

		// create shader modules
		Shaders::ShaderSource vtx = { Shaders::ShaderType::Vertex, "shaders/MRT.vert.hlsl.spv" };
		Shaders::ShaderSource frag = { Shaders::ShaderType::Fragment, "shaders/MRT.frag.hlsl.spv" };
		std::vector<Shaders::ShaderSource> shaders = { vtx, frag };
		mrtProgram = std::make_unique<Shaders::MRTShaderProgram>(shaders, imageViewsRef.size());

		// deferred shaders
		Shaders::ShaderSource dVtx = { Shaders::ShaderType::Vertex, "shaders/deferred.vert.hlsl.spv" };
		Shaders::ShaderSource dFrg = { Shaders::ShaderType::Fragment, "shaders/deferred.frag.hlsl.spv" };
		std::vector<Shaders::ShaderSource> dShaders = { dVtx, dFrg };
		deferredProgram = std::make_unique<Shaders::DeferredShaderProgram>(dShaders, imageViewsRef.size());

		auto mrtStages = mrtProgram->getShaderStages();
		auto defStages = deferredProgram->getShaderStages();

		auto bindingDescription = Geometry::Vertex::getBindingDescriptions();
		auto attributeDescriptions = Geometry::Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vtxInputInfo = {};
		vtxInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vtxInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
		vtxInputInfo.pVertexBindingDescriptions = bindingDescription.data();
		vtxInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vtxInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo assemblyInfo = {};
		assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assemblyInfo.primitiveRestartEnable = VK_FALSE;

		VkPipelineTessellationStateCreateInfo tessellationInfo = {};
		tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		tessellationInfo.patchControlPoints = 3;

		VkRect2D scissor = {
			{ 0, 0 },
			renderer->getSwapChainExtent()
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

		std::vector<VkDescriptorSetLayoutBinding> mrtBindings;
		std::vector<VkDescriptorSetLayoutBinding> deferredBindings;

		const VkDescriptorSetLayoutBinding uboBinding = {
			0,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_VERTEX_BIT,
			nullptr
		};
		mrtBindings.push_back(uboBinding);

		const VkDescriptorSetLayoutBinding modelBinding = {
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			1,
			VK_SHADER_STAGE_VERTEX_BIT,
			nullptr
		};
		mrtBindings.push_back(modelBinding);

		const VkDescriptorSetLayoutBinding fragSettingsBinding = {
			0,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			nullptr
		};
		deferredBindings.push_back(fragSettingsBinding);

		const VkDescriptorSetLayoutBinding positionTexBinding = {
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			nullptr
		};
		deferredBindings.push_back(positionTexBinding);
		const VkDescriptorSetLayoutBinding normalTexBinding = {
			2,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			nullptr
		};
		deferredBindings.push_back(normalTexBinding);
		const VkDescriptorSetLayoutBinding albedoTexBinding = {
			3,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			nullptr
		};
		deferredBindings.push_back(albedoTexBinding);
		const VkDescriptorSetLayoutBinding pbrTexBinding = {
			4,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			nullptr
		};
		deferredBindings.push_back(pbrTexBinding);

		VkDescriptorSetLayoutCreateInfo layoutInfo = {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(mrtBindings.size()),
			mrtBindings.data()
		};

		VK_THROW_ON_ERROR(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &mrtDescriptorSetLayout), "DescriptorSetLayout creation failed!");

		layoutInfo.pBindings = deferredBindings.data();
		layoutInfo.bindingCount = static_cast<uint32_t>(deferredBindings.size());
		VK_THROW_ON_ERROR(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &deferredDescriptorSetLayout), "DescriptorSetLayout creation failed!");

		auto bufferSetCount = static_cast<uint32_t>(imageViewsRef.size());
		std::array<VkDescriptorPoolSize, 3> sizes = {};
		sizes[0] = {
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			2u * bufferSetCount
		};
		sizes[1] = {
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			1u * bufferSetCount
		};
		sizes[2] = {
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			4u * bufferSetCount
		};

		VkDescriptorPoolCreateInfo info = {
			VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			nullptr,
			0,
			2u * bufferSetCount,
			static_cast<uint32_t>(sizes.size()),
			sizes.data()
		};

		VK_THROW_ON_ERROR(vkCreateDescriptorPool(device, &info, nullptr, &descriptorPool), "DescriptorPool creation failed!");

		VkDescriptorSetLayout layouts[] = { mrtDescriptorSetLayout };
		VkDescriptorSetAllocateInfo allocInfo = {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			nullptr,
			descriptorPool,
			1,
			layouts
		};

		mrtDescriptorSets.resize(imageViewsRef.size());
		for (auto& mrtDescSet : mrtDescriptorSets) {
			VK_THROW_ON_ERROR(vkAllocateDescriptorSets(device, &allocInfo, &mrtDescSet), "MRT DescriptorSet allocation failed!");
		}

		layouts[0] = deferredDescriptorSetLayout;
		deferredDescriptorSets.resize(imageViewsRef.size());
		for (auto& descSet : deferredDescriptorSets) {
			VK_THROW_ON_ERROR(vkAllocateDescriptorSets(device, &allocInfo, &descSet), "Deferred DescriptorSet allocation failed!");
		}
		updateDescriptorSets();

		VkDescriptorSetLayout descLayouts[] = { mrtDescriptorSetLayout, renderer->getMaterialDescriptorSetLayout() };

		std::array<VkPushConstantRange, 1> pushConstantRanges = {
			{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Material::MaterialUniforms) }
		};

		// MRT Layout
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 2;
		pipelineLayoutInfo.pSetLayouts = descLayouts;
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
		;

		VK_THROW_ON_ERROR(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mrtPipelineLayout), "PipelineLayout creation failed!");

		// Deferred Layout

		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &deferredDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		VK_THROW_ON_ERROR(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &deferredPipelineLayout), "PipelineLayout creation failed!");

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachments = {};
		for (auto i = 0u; i < 4; ++i) {
			blendAttachments[i].colorWriteMask = 0xF;
			blendAttachments[i].blendEnable = VK_FALSE;
		}

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &blendAttachments[0];
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		// Deferred Pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = static_cast<uint32_t>(defStages.size());
		pipelineInfo.pStages = defStages.data();
		pipelineInfo.pVertexInputState = &vtxInputInfo;
		pipelineInfo.pInputAssemblyState = &assemblyInfo;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pTessellationState = &tessellationInfo;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = deferredPipelineLayout;
		pipelineInfo.renderPass = deferredRenderPass;
		pipelineInfo.subpass = 0;

		VK_THROW_ON_ERROR(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &deferredPipeline), "Pipeline creation failed!");

		colorBlending.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
		colorBlending.pAttachments = blendAttachments.data();

		// MRT Pipeline
		pipelineInfo.stageCount = static_cast<uint32_t>(mrtStages.size());
		pipelineInfo.pStages = mrtStages.data();
		pipelineInfo.renderPass = mrtRenderPass;
		pipelineInfo.layout = mrtPipelineLayout;

		VK_THROW_ON_ERROR(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &mrtPipeline), "Pipeline creation failed!");

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
				deferredRenderPass,
				static_cast<uint32_t>(attachments.size()),
				fbAttachments.data(),
				extent.width,
				extent.height,
				1
			};

			VK_THROW_ON_ERROR(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]), "ARShader: Framebuffer creation failed!");
		}
	}
}

void DeferredDraw::updateDescriptorSets() const
{
	updateMRTDescriptorSets();
	updateDeferredDescriptorSets();
}

void DeferredDraw::updateMRTDescriptorSets() const
{
	size_t i = 0;

	for (auto& descSet : mrtDescriptorSets) {
		const auto uboModel = mrtProgram->getDescriptorInfos(i);

		std::vector<VkWriteDescriptorSet> write;

		const VkWriteDescriptorSet ubo = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descSet,
			0,
			0,
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			nullptr,
			&uboModel,
			nullptr
		};
		write.push_back(ubo);

		if (mrtProgram->dynamicBuffer.buffer) {
			const VkWriteDescriptorSet dyn = {
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				descSet,
				1,
				0,
				1,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
				nullptr,
				&mrtProgram->dynamicBuffer.descriptor,
				nullptr
			};
			write.push_back(dyn);
		}

		vkUpdateDescriptorSets(App::getHandle().getRenderBackend()->getDevice(), static_cast<uint32_t>(write.size()), write.data(), 0, nullptr);
		++i;
	}
}
void DeferredDraw::updateDeferredDescriptorSets() const
{
	size_t i = 0;
	for (auto& descSet : deferredDescriptorSets) {
		std::vector<VkWriteDescriptorSet> write;

		auto fragModel = deferredProgram->getDescriptorInfos(i);
		const VkWriteDescriptorSet frag = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descSet,
			0,
			0,
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			nullptr,
			&fragModel,
			nullptr
		};
		write.push_back(frag);

		VkDescriptorImageInfo posInfo = {};
		posInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		posInfo.imageView = offscreenFramebuffers[i].position.view;
		posInfo.sampler = colorSampler;

		const VkWriteDescriptorSet pos = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descSet,
			1,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&posInfo,
			nullptr,
			nullptr
		};
		write.push_back(pos);

		VkDescriptorImageInfo normalInfo = {};
		normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalInfo.imageView = offscreenFramebuffers[i].normal.view;
		normalInfo.sampler = colorSampler;

		const VkWriteDescriptorSet normal = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descSet,
			2,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&normalInfo,
			nullptr,
			nullptr
		};
		write.push_back(normal);

		VkDescriptorImageInfo albInfo = {};
		albInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albInfo.imageView = offscreenFramebuffers[i].albedo.view;
		albInfo.sampler = colorSampler;

		const VkWriteDescriptorSet alb = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descSet,
			3,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&albInfo,
			nullptr,
			nullptr
		};
		write.push_back(alb);
		VkDescriptorImageInfo pbrInfo = {};
		pbrInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		pbrInfo.imageView = offscreenFramebuffers[i].pbrSpecular.view;
		pbrInfo.sampler = colorSampler;

		const VkWriteDescriptorSet pbr = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descSet,
			4,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&pbrInfo,
			nullptr,
			nullptr
		};
		write.push_back(pbr);

		vkUpdateDescriptorSets(App::getHandle().getRenderBackend()->getDevice(), static_cast<uint32_t>(write.size()), write.data(), 0, nullptr);
		++i;
	}
}

void DeferredDraw::cleanup()
{
	const auto& renderer = App::getHandle().getRenderBackend();
	const auto device = renderer->getDevice();

	//	vkFreeDescriptorSets(device, pDescriptorPool, 1, &pDescriptorSet);
	vkDestroyDescriptorSetLayout(device, mrtDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, deferredDescriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);

	vkDestroyPipeline(device, mrtPipeline, nullptr);
	vkDestroyPipeline(device, deferredPipeline, nullptr);
	vkDestroyPipelineLayout(device, mrtPipelineLayout, nullptr);
	vkDestroyPipelineLayout(device, deferredPipelineLayout, nullptr);
	vkDestroyRenderPass(device, mrtRenderPass, nullptr);
	vkDestroyRenderPass(device, deferredRenderPass, nullptr);

	for (auto& framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
	for (auto& fb : offscreenFramebuffers) {
		vkDestroyFramebuffer(device, fb.framebuffer, nullptr);
		fb.position.memory->free(device);
		delete (fb.position.memory);
		fb.normal.memory->free(device);
		delete (fb.normal.memory);
		fb.albedo.memory->free(device);
		delete (fb.albedo.memory);
		fb.pbrSpecular.memory->free(device);
		delete (fb.pbrSpecular.memory);
		fb.depth.memory->free(device);
		delete (fb.depth.memory);
		/*vkDestroyImageView(device, fb.position.view, nullptr);
		vkDestroyImage(device, fb.position.image.image, nullptr);
		vkDestroyImageView(device, fb.normal.view, nullptr);
		vkDestroyImage(device, fb.normal.image.image, nullptr);
		vkDestroyImageView(device, fb.pbrValues.view, nullptr);
		vkDestroyImage(device, fb.pbrValues.image.image, nullptr);
		vkDestroyImageView(device, fb.depth.view, nullptr);
		vkDestroyImage(device, fb.depth.image.image, nullptr);*/
	}

	vkDestroySampler(device, colorSampler, nullptr);

	mrtProgram->cleanup();
	deferredProgram->cleanup();
	mrtProgram.reset();
	deferredProgram.reset();
}

void DeferredDraw::initAttachment(VkFormat format, VkImageUsageFlagBits usage, DeferredDraw::FrameBufferAtt* attachment)
{
	attachment->format = format;
	VkImageAspectFlags aspectMask = 0x0;

	if ((usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if ((usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		if (format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D32_SFLOAT) {
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		} else {
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}

	assert(aspectMask > 0x0);

	const auto backend = App::getHandle().getRenderBackend();
	const auto extent = backend->getSwapChainExtent();

	attachment->memory = new vkExt::SharedMemory();
	backend->createImage2D(extent.width, extent.height, format, VK_IMAGE_TILING_OPTIMAL, usage | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachment->image, attachment->memory);
	attachment->view = backend->createImageView2D(attachment->image.image, format, aspectMask);
}
