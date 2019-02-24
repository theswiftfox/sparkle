#include "Application.h"
#include "FileReader.h"
#include "RenderBackend.h"
#include <UI.h>

#include <iomanip>
#include <sstream>

using namespace Sparkle;

VkPipelineShaderStageCreateInfo Sparkle::GUI::loadUiShader(const std::string shaderName, VkShaderStageFlagBits stage)
{
    VkPipelineShaderStageCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = stage;
    info.pName = "main";

    auto shaderData = Sparkle::Tools::FileReader::readFile(shaderName);
    auto module = Shaders::createShaderModule(shaderData);

    info.module = module;
    return info;
}

GUI::GUI()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    renderBackend = App::getHandle().getRenderBackend();
    device = renderBackend->getDevice();
    assimpProgress = ProgressData();
}

//GUI::~GUI() {
//	vertexBuffer.destroy(true);
//	indexBuffer.destroy(true);
//	delete(vertexMemory);
//	delete(indexMemory);
//
//	fontTex.reset();
//	vkDestroyPipelineCache(device, pipelineCache, nullptr);
//	vkDestroyPipeline(device, pipeline, nullptr);
//	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
//	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
//	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
//}

void GUI::init(float width, float height, VkRenderPass renderPass)
{
    windowWidth = (float)width;
    windowHeight = (float)height;
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.DisplaySize = ImVec2(windowWidth, windowHeight);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    ImGui::StyleColorsDark();
    this->renderPass = renderPass;
    initResources();
}

void GUI::initResources()
{
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* fontData;
    int widthTx, heightTx;

    io.Fonts->GetTexDataAsRGBA32(&fontData, &widthTx, &heightTx);

    fontTex = std::make_shared<Texture>(fontData, widthTx, heightTx, 4, 0);

    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.maxSets = 2;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = poolSizes.data();

    VK_THROW_ON_ERROR(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool), "Error creating UI descriptor pool");

    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }
    };

    VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo = {};
    descSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    descSetLayoutCreateInfo.pBindings = bindings.data();

    VK_THROW_ON_ERROR(vkCreateDescriptorSetLayout(device, &descSetLayoutCreateInfo, nullptr, &descriptorSetLayout), "Error creating descriptor set for UI");

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VK_THROW_ON_ERROR(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet), "Error allocating descriptor set for UI");

    const auto fontDesc = fontTex->descriptor();
    std::array<VkWriteDescriptorSet, 1> writeSets = {
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSet, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &fontDesc }
    };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

    VkPipelineCacheCreateInfo pipeCacheCreateInfo = {};
    pipeCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VK_THROW_ON_ERROR(vkCreatePipelineCache(device, &pipeCacheCreateInfo, nullptr, &pipelineCache), "Error creating pipeline cache for UI");

    VkPushConstantRange pushConstRange = {};
    pushConstRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstRange.size = sizeof(PushConstants);
    pushConstRange.offset = 0;

    VkPipelineLayoutCreateInfo pipeLayoutInfo = {};
    pipeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLayoutInfo.setLayoutCount = 1;
    pipeLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipeLayoutInfo.pushConstantRangeCount = 1;
    pipeLayoutInfo.pPushConstantRanges = &pushConstRange;

    VK_THROW_ON_ERROR(vkCreatePipelineLayout(device, &pipeLayoutInfo, nullptr, &pipelineLayout), "Error creating pipeline layout in UI");

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
    rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationInfo.depthClampEnable = VK_FALSE;
    rasterizationInfo.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState blendAttachments = {};
    blendAttachments.blendEnable = VK_TRUE;
    blendAttachments.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachments.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachments.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachments.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachments.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachments.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachments.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blendInfo = {};
    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = &blendAttachments;

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable = VK_FALSE;
    depthStencilInfo.depthWriteEnable = VK_FALSE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilInfo.front = depthStencilInfo.back;
    depthStencilInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;

    VkPipelineViewportStateCreateInfo viewportInfo = {};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;

    VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {};

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pRasterizationState = &rasterizationInfo;
    pipelineInfo.pColorBlendState = &blendInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();

    std::vector<VkVertexInputBindingDescription> vtxAttrib = {
        { 0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX }
    };
    std::vector<VkVertexInputAttributeDescription> vtxDesc = {
        { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos) },
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv) },
        { 2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col) }
    };

    VkPipelineVertexInputStateCreateInfo vtxInputInfo {};
    vtxInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vtxInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vtxAttrib.size());
    vtxInputInfo.pVertexBindingDescriptions = vtxAttrib.data();
    vtxInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vtxDesc.size());
    vtxInputInfo.pVertexAttributeDescriptions = vtxDesc.data();

    pipelineInfo.pVertexInputState = &vtxInputInfo;

    shaderStages[0] = loadUiShader("shaders/ui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadUiShader("shaders/ui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    vtxModule = shaderStages[0].module;
    frgModule = shaderStages[1].module;

    VK_THROW_ON_ERROR(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline), "GUI Pipeline creation failed");
}

void GUI::updateBuffers(const std::vector<VkFence>& fences)
{
    auto device = App::getHandle().getRenderBackend()->getDevice();
    auto drawData = ImGui::GetDrawData();

    if ((drawData->TotalVtxCount == 0) || (drawData->TotalIdxCount == 0)) {
        return;
    }

    bool vtxResize = vtxCount < static_cast<uint64_t>(drawData->TotalVtxCount);
    if ((vertexBuffer.buffer == VK_NULL_HANDLE) || vtxResize) {
        if (vtxResize && vertexBuffer.buffer != VK_NULL_HANDLE) {
            vkWaitForFences(renderBackend->getDevice(), static_cast<uint32_t>(fences.size()), fences.data(), VK_TRUE, uint64_t(5e+8)); // wait for .5s
            vkResetFences(renderBackend->getDevice(), static_cast<uint32_t>(fences.size()), fences.data());
        }
        if (vertexBuffer.buffer) {
            vertexBuffer.destroy(true);
        }
        if (!vertexMemory) {
            vertexMemory = new vkExt::SharedMemory();
        } else if (vertexMemory->memory) {
            vertexMemory->free(device);
        }
        VkDeviceSize vtxBuffSize = std::min<VkDeviceSize>(sizeof(ImDrawVert) * drawData->TotalVtxCount * 10, minVtxBufferSize);
        vtxCount = vtxBuffSize / sizeof(ImDrawVert);
        renderBackend->createBuffer(vtxBuffSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBuffer, vertexMemory);
        vertexBuffer.map();
    }
    bool idxResize = idxCount < static_cast<uint64_t>(drawData->TotalIdxCount);
    if ((indexBuffer.buffer == VK_NULL_HANDLE) || idxResize) {
        if (idxResize && !vtxResize && indexBuffer.buffer != VK_NULL_HANDLE) {
            vkWaitForFences(renderBackend->getDevice(), static_cast<uint32_t>(fences.size()), fences.data(), VK_TRUE, std::numeric_limits<uint64_t>::max());
            vkResetFences(renderBackend->getDevice(), static_cast<uint32_t>(fences.size()), fences.data());
        }
        if (indexBuffer.buffer) {
            indexBuffer.destroy(true);
        }
        if (!indexMemory) {
            indexMemory = new vkExt::SharedMemory();
        } else if (indexMemory->memory) {
            indexMemory->free(device);
        }
        VkDeviceSize idxBuffSize = std::min<VkDeviceSize>(sizeof(ImDrawIdx) * drawData->TotalIdxCount * 10, minIdxBufferSize);
        idxCount = idxBuffSize / sizeof(ImDrawIdx);
        renderBackend->createBuffer(idxBuffSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, indexBuffer, indexMemory);
        indexBuffer.map();
    }

    VkDeviceSize vtxOffs = 0;
    VkDeviceSize idxOffs = 0;
    for (size_t i = 0; i < drawData->CmdListsCount; ++i) {
        const auto cmdList = drawData->CmdLists[i];
        vertexBuffer.copyTo(cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert), vtxOffs);
        indexBuffer.copyTo(cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx), idxOffs);
        vtxOffs += (cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        idxOffs += (cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
    }

    vertexBuffer.flush();
    indexBuffer.flush();
}

void GUI::updateFrame(GUI::FrameData frameData)
{
    ImGui::SetNextWindowPos(ImVec2(windowWidth - 100, 10));
    int flags = ImGuiWindowFlags_NoTitleBar;
    flags |= ImGuiWindowFlags_AlwaysAutoResize;
    flags |= ImGuiWindowFlags_NoMove;
    ImGui::Begin("FPS", nullptr, flags);
    auto text = "FPS: " + std::to_string(frameData.fps);
    ImGui::TextUnformatted(text.c_str());
    ImGui::End();
    // Todo: ImGui windows etc

    if (assimpProgress.isLoading) {
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        int flags = ImGuiWindowFlags_NoTitleBar;
        flags |= ImGuiWindowFlags_AlwaysAutoResize;
        flags |= ImGuiWindowFlags_NoMove;
        ImGui::Begin("Loading", nullptr, flags);
        if (assimpProgress.value > 0.000001f) {
            //auto text = "Processing assets: Step " + std::to_string(assimpProgress.currentStep) + "/" + std::to_string(assimpProgress.maxSteps);
            auto percentage = assimpProgress.value * 100;
            std::stringstream ss;
            ss << "Loading assets: " << std::fixed << std::setprecision(1) << percentage << "%";
            //	ImGui::TextUnformatted(text.c_str());
            ImGui::TextUnformatted(ss.str().c_str());
        } else {
            ImGui::TextUnformatted("Loading level file..");
        }
        ImGui::End();
    }

    if (showOptions) {
        ImGui::SetNextWindowPos(ImVec2(10, windowHeight - 100));
        int flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
        ImGui::Begin("Settings", &showOptions, flags);
        ImGui::SliderFloat("Exposure", &exposure, 0.1f, 10.0f, "%.1f");
        ImGui::SliderFloat("Gamma", &gamma, 0.5f, 4.0f, "%.1f");
        ImGui::End();
    }

    //ImGui::ShowDemoWindow();

    ImGui::Render();
}

void GUI::drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer)
{
    auto& io = ImGui::GetIO();

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.clearValueCount = 0;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = renderBackend->getSwapChainExtent();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    auto drawData = ImGui::GetDrawData();

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = drawData->DisplaySize.x;
    viewport.height = drawData->DisplaySize.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    pushConstants.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    pushConstants.translate = glm::vec2(-1.0f - drawData->DisplayPos.x * pushConstants.scale.x, -1.0f - drawData->DisplayPos.y * pushConstants.scale.y);
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

    int32_t vtxOffs = 0;
    uint32_t idxOffs = 0;
    ImVec2 displayPos = drawData->DisplayPos;

    if (drawData->CmdListsCount > 0) {
        VkDeviceSize offsets[1] = {
            0
        };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

        for (int32_t i = 0; i < drawData->CmdListsCount; ++i) {
            const auto cmdList = drawData->CmdLists[i];
            for (int32_t j = 0; j < cmdList->CmdBuffer.Size; ++j) {
                const auto drawCmd = &cmdList->CmdBuffer[j];
                VkRect2D scissor {};
                scissor.offset = {
                    std::max(static_cast<int32_t>(drawCmd->ClipRect.x - displayPos.x), 0), // x
                    std::max(static_cast<int32_t>(drawCmd->ClipRect.y - displayPos.y), 0) // y
                };
                scissor.extent = {
                    static_cast<uint32_t>(drawCmd->ClipRect.z - drawCmd->ClipRect.x), // width
                    static_cast<uint32_t>(drawCmd->ClipRect.w - drawCmd->ClipRect.y + 1) // height
                };

                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
                vkCmdDrawIndexed(commandBuffer, drawCmd->ElemCount, 1, idxOffs, vtxOffs, 0);
                idxOffs += drawCmd->ElemCount;
            }
            vtxOffs += cmdList->VtxBuffer.Size;
        }
    }
    vkCmdEndRenderPass(commandBuffer);
}

bool GUI::Update(float percentage /*=-1*/)
{
    assimpProgress.isLoading = percentage > -1.0f;
    assimpProgress.value = percentage;

    return false;
}

void GUI::UpdatePostProcess(int currentStep /*= 0*/, int numberOfSteps /*= 0*/)
{
    assimpProgress.currentStep = currentStep;
    assimpProgress.maxSteps = numberOfSteps;
    ProgressHandler::UpdatePostProcess(currentStep, numberOfSteps);
}