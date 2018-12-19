#include "Shader.h"
#include "Geometry.h"
#include "FileReader.h"
#include "Application.h"

#ifndef _WINDOWS
#define _aligned_malloc(size, alignment) aligned_alloc(alignment, size)
#define _aligned_free(obj) free(obj)
#endif

using namespace Engine::Shaders;

ShaderProgram::ShaderProgram(const std::string& vtxShaderFile, const std::string& tescShaderFile, const std::string& teseShaderFile, const std::string& fragShaderFile, size_t objectInstances) {
	pUniformBufferMemory = new vkExt::SharedMemory();
	pDynamicBufferMemory = new vkExt::SharedMemory();
	objectCount = objectInstances;
	if (!vtxShaderFile.empty()) {
		const auto vtxShaderCode = Tools::FileReader::readFile(vtxShaderFile);
		vertexShader = createShaderModule(vtxShaderCode);
	}
	if (!tescShaderFile.empty() && !teseShaderFile.empty()) {
		const auto tescShaderCode = Tools::FileReader::readFile(tescShaderFile);
		const auto teseShaderCode = Tools::FileReader::readFile(teseShaderFile);
		tessControlShader = createShaderModule(tescShaderCode);
		tessEvalShader = createShaderModule(teseShaderCode);
	}
	if (!fragShaderFile.empty()) {
		const auto fragShaderCode = Tools::FileReader::readFile(fragShaderFile);
		fragmentShader = createShaderModule(fragShaderCode);
	}

	createUniformBuffer();
	createDynamicBuffer(0);
}

void ShaderProgram::cleanup() const {
	const auto device = Engine::App::getHandle().getRenderBackend()->getDevice();
	if (vertexShader)
	{
		vkDestroyShaderModule(device, vertexShader, nullptr);
	}
	if (tessControlShader)
	{
		vkDestroyShaderModule(device, tessControlShader, nullptr);
	}
	if (tessEvalShader)
	{
		vkDestroyShaderModule(device, tessEvalShader, nullptr);
	}
	if (fragmentShader)
	{
		vkDestroyShaderModule(device, fragmentShader, nullptr);
	}

	pUniformBuffer.destroy(true);
	if (pUniformBufferMemory) {
		delete(pUniformBufferMemory);
	}

	if (pDynamicBuffer.buffer) {
		pDynamicBuffer.destroy(true);
	}
	if (pDynamicBufferMemory) {
		delete(pDynamicBufferMemory);
	}
}


void ShaderProgram::updateUniformBufferObject(const UBO& ubo)
{
	pUniformBuffer.map();
	pUniformBuffer.copyTo(&ubo, sizeof(ubo));
	pUniformBuffer.unmap();
}

void ShaderProgram::updateFragmentShaderSettings(const FragmentShaderSettings& sets) {
	pUniformBuffer.map(fragSettingsOffset);
	pUniformBuffer.copyTo(&sets, sizeof(sets));
	pUniformBuffer.unmap();
}

std::vector<VkPipelineShaderStageCreateInfo> ShaderProgram::getShaderStages() const
{
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	if (vertexShader)
	{
		VkPipelineShaderStageCreateInfo stageInfo = {};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.module = vertexShader;
		stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		stageInfo.pName = "main";
		stages.push_back(stageInfo);
	}
	if (tessControlShader && tessEvalShader)
	{
		{
			VkPipelineShaderStageCreateInfo stageInfo = {};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.module = tessControlShader;
			stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			stageInfo.pName = "main";
			stages.push_back(stageInfo);
		}
		{
			VkPipelineShaderStageCreateInfo stageInfo = {};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.module = tessEvalShader;
			stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			stageInfo.pName = "main";
			stages.push_back(stageInfo);
		}
	}
	if (fragmentShader)
	{
		VkPipelineShaderStageCreateInfo stageInfo = {};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.module = fragmentShader;
		stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stageInfo.pName = "main";
		stages.push_back(stageInfo);
	}

	if (stages.empty()) {
		throw std::runtime_error("No shader stages enabled! Cannot create Pipeline without at least one stage");
	}

	return stages;
}


void ShaderProgram::createUniformBuffer()
{
	const auto& renderer = Engine::App::getHandle().getRenderBackend();

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(renderer->getPhysicalDevice(), &props);
	const auto alignment = props.limits.minUniformBufferOffsetAlignment;

	const auto uboSize = sizeof(UBO);
	if (alignment > 0) {
		fragSettingsOffset = (uboSize + alignment - 1) & ~(alignment - 1);
	}
	else {
		fragSettingsOffset = uboSize;
	}


	const auto fragSize = sizeof(FragmentShaderSettings);

	const auto bufferSize = fragSettingsOffset + fragSize;
	renderer->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pUniformBuffer, pUniformBufferMemory);
}

void ShaderProgram::createDynamicBuffer(VkDeviceSize size) {
	const auto& renderer = Engine::App::getHandle().getRenderBackend();

	if (pDynamicBuffer.buffer) {
		pDynamicBuffer.destroy(true);

		_aligned_free(dynamicUboData.model);
	}

	objectCount = size;

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(renderer->getPhysicalDevice(), &props);
	const auto alignment = static_cast<uint32_t>(props.limits.minUniformBufferOffsetAlignment);

	dynamicAlignment = static_cast<uint32_t>(sizeof(glm::mat4));
	if (alignment > 0) {
		dynamicAlignment = (dynamicAlignment + alignment - 1) & ~(alignment - 1);
	}
	// if buffer is initialized with empty geometry (objectCount of 0), use 1 alignment as initial size.
	dynamicUboDataSize = objectCount > 0 ? objectCount * dynamicAlignment : dynamicAlignment;
	dynamicUboData.model = (glm::mat4*)_aligned_malloc(dynamicUboDataSize, dynamicAlignment);

	renderer->createBuffer(dynamicUboDataSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pDynamicBuffer, pDynamicBufferMemory);
	
	pDynamicBuffer.map();

	dynamicBufferDirty = true;
	// getDescriptorInfos();
}

void ShaderProgram::updateDynamicUniformBufferObject(const std::vector<std::shared_ptr<Engine::Geometry::Node>>& meshes) {
	if (!(pDynamicBuffer.buffer) || meshes.size() * dynamicAlignment > dynamicUboDataSize) {
		createDynamicBuffer(meshes.size());
	}
	for (auto i = 0; i < meshes.size(); ++i) {
		const auto model = (glm::mat4*)((uint64_t)dynamicUboData.model + i * dynamicAlignment);
		*model = glm::mat4(meshes[i]->accumModel());
	}
	pDynamicBuffer.copyTo(dynamicUboData.model, dynamicUboDataSize);
	pDynamicBuffer.flush();
}


std::array<VkDescriptorBufferInfo, 2> ShaderProgram::getDescriptorInfos() const
{
	const VkDescriptorBufferInfo uboModel = {
		pUniformBuffer.buffer,
		0,
		sizeof(UBO)
	};

	const VkDescriptorBufferInfo fragSettingsModel = {
		pUniformBuffer.buffer,
		fragSettingsOffset,
		sizeof(FragmentShaderSettings)
	};

	const std::array<VkDescriptorBufferInfo, 2> descSets = { uboModel, fragSettingsModel };
	return descSets;
}


VkShaderModule ShaderProgram::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(Engine::App::getHandle().getRenderBackend()->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Shader Module creation failed!");
	}
	return shaderModule;
}
