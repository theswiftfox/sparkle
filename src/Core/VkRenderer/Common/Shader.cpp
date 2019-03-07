#include "Shader.h"
#include "Application.h"
#include "FileReader.h"
#include "Geometry.h"

void* _alignedAlloc(size_t size, size_t alignment)
{
	void* data = nullptr;
#if defined(_MSC_VER)
	data = _aligned_malloc(size, alignment);
#else
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

void _alignedFree(void* data)
{
#if defined(_MSC_VER)
	_aligned_free(data);
#else
	free(data);
#endif
}

using namespace Sparkle::Shaders;

ShaderProgram::ShaderProgram(const std::vector<ShaderSource>& shaderSources)
{
	pUniformBufferMemory = new vkExt::SharedMemory();
	pDynamicBufferMemory = new vkExt::SharedMemory();
	objectCount = 0;

	for (const auto& shader : shaderSources) {
		switch (shader.type) {
		case Vertex: {
			const auto vtxShaderCode = Tools::FileReader::readFile(shader.filePath);
			vertexShader = createShaderModule(vtxShaderCode);
			break;
		}
		case TessellationControl: {
			const auto tescShaderCode = Tools::FileReader::readFile(shader.filePath);
			tessControlShader = createShaderModule(tescShaderCode);
			break;
		}
		case TessellationEvaluation: {
			const auto teseShaderCode = Tools::FileReader::readFile(shader.filePath);
			tessEvalShader = createShaderModule(teseShaderCode);
			break;
		}
		case Fragment: {
			const auto fragShaderCode = Tools::FileReader::readFile(shader.filePath);
			fragmentShader = createShaderModule(fragShaderCode);
			break;
		}
		default:
			throw std::runtime_error("Unsupported Shader type for Graphics Shader! Did you mean to use a compute shader?");
			break;
		}
	}

	createUniformBuffer();
	createDynamicBuffer(0);
}

void ShaderProgram::cleanup() const
{
	const auto device = Sparkle::App::getHandle().getRenderBackend()->getDevice();
	if (vertexShader) {
		vkDestroyShaderModule(device, vertexShader, nullptr);
	}
	if (tessControlShader) {
		vkDestroyShaderModule(device, tessControlShader, nullptr);
	}
	if (tessEvalShader) {
		vkDestroyShaderModule(device, tessEvalShader, nullptr);
	}
	if (fragmentShader) {
		vkDestroyShaderModule(device, fragmentShader, nullptr);
	}

	pUniformBuffer.destroy(true);
	if (pUniformBufferMemory) {
		delete (pUniformBufferMemory);
	}

	if (pDynamicBuffer.buffer) {
		pDynamicBuffer.destroy(true);
	}
	if (pDynamicBufferMemory) {
		delete (pDynamicBufferMemory);
	}
}

void ShaderProgram::updateUniformBufferObject(const UniformBufferObject& ubo)
{
	pUniformBuffer.map();
	pUniformBuffer.copyTo(&ubo, sizeof(ubo));
	pUniformBuffer.unmap();
}

void ShaderProgram::updateFragmentShaderUniforms(const FragmentShaderUniforms& sets)
{
	pUniformBuffer.map(fragSettingsOffset);
	pUniformBuffer.copyTo(&sets, sizeof(sets));
	pUniformBuffer.unmap();
}

std::vector<VkPipelineShaderStageCreateInfo> ShaderProgram::getShaderStages() const
{
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	if (vertexShader) {
		VkPipelineShaderStageCreateInfo stageInfo = {};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.module = vertexShader;
		stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		stageInfo.pName = "main";
		stages.push_back(stageInfo);
	}
	if (tessControlShader && tessEvalShader) {
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
	if (fragmentShader) {
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
	const auto& renderer = Sparkle::App::getHandle().getRenderBackend();

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(renderer->getPhysicalDevice(), &props);
	const auto alignment = props.limits.minUniformBufferOffsetAlignment;

	const auto uboSize = sizeof(UniformBufferObject);
	if (alignment > 0) {
		fragSettingsOffset = (uboSize + alignment - 1) & ~(alignment - 1);
	} else {
		fragSettingsOffset = uboSize;
	}

	const auto fragSize = sizeof(FragmentShaderUniforms);

	const auto bufferSize = fragSettingsOffset + fragSize;
	renderer->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pUniformBuffer, pUniformBufferMemory);
}

void ShaderProgram::createDynamicBuffer(VkDeviceSize size)
{
	const auto& renderer = Sparkle::App::getHandle().getRenderBackend();

	if (pDynamicBuffer.buffer) {
		pDynamicBuffer.destroy(true);

		_alignedFree(dynamicUboData);
	}

	objectCount = size;

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(renderer->getPhysicalDevice(), &props);
	const auto alignment = static_cast<uint32_t>(props.limits.minUniformBufferOffsetAlignment);

	dUboAlignment = static_cast<uint32_t>(sizeof(InstancedUniformBufferObject));
	if (alignment > 0) {
		dUboAlignment = (dUboAlignment + alignment - 1) & ~(alignment - 1);
	}
	// if buffer is initialized with empty geometry (objectCount of 0), use 1 alignment as initial size.
	dynamicUboDataSize = objectCount > 0 ? objectCount * dUboAlignment : dUboAlignment;
	dynamicUboData = (InstancedUniformBufferObject*)_alignedAlloc(dynamicUboDataSize, dUboAlignment);

	renderer->createBuffer(dynamicUboDataSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pDynamicBuffer, pDynamicBufferMemory);

	pDynamicBuffer.map();

	dynamicBufferDirty = true;
	// getDescriptorInfos();
}

void ShaderProgram::updateDynamicUniformBufferObject(const std::vector<std::shared_ptr<Sparkle::Geometry::Node>>& meshes)
{
	if (!(pDynamicBuffer.buffer) || meshes.size() * dUboAlignment > dynamicUboDataSize) {
		createDynamicBuffer(meshes.size());
	}
	for (auto i = 0; i < meshes.size(); ++i) {
		const auto modelMat = meshes[i]->accumModel();
		const auto iUbo = (InstancedUniformBufferObject*)((uint64_t)dynamicUboData + i * dUboAlignment);
		iUbo->model = glm::mat4(modelMat);
		iUbo->normal = glm::transpose(glm::inverse(modelMat));
	}
	pDynamicBuffer.copyTo(dynamicUboData, dynamicUboDataSize);
	pDynamicBuffer.flush();
}

std::array<VkDescriptorBufferInfo, 2> ShaderProgram::getDescriptorInfos() const
{
	const VkDescriptorBufferInfo uboModel = {
		pUniformBuffer.buffer,
		0,
		sizeof(UniformBufferObject)
	};

	const VkDescriptorBufferInfo fragSettingsModel = {
		pUniformBuffer.buffer,
		fragSettingsOffset,
		sizeof(FragmentShaderUniforms)
	};

	const std::array<VkDescriptorBufferInfo, 2> descSets = { uboModel, fragSettingsModel };
	return descSets;
}

inline VkShaderModule Sparkle::Shaders::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(Sparkle::App::getHandle().getRenderBackend()->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Shader Module creation failed!");
	}
	return shaderModule;
}