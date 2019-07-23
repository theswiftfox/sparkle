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

MRTShaderProgram::MRTShaderProgram(const std::vector<ShaderSource>& shaderSources, size_t bufferCount)
{
	uniformBuffers.resize(bufferCount);
	dynamicBufferMemory = new vkExt::SharedMemory();
	objectCount = 0;

	for (const auto& shader : shaderSources) {
		switch (shader.type) {
		case Vertex: {
			const auto vtxShaderCode = Tools::FileReader::readFile(shader.filePath);
			shaderModules.vtxModule = createShaderModule(vtxShaderCode);
			break;
		}
		case TessellationControl: {
			const auto tescShaderCode = Tools::FileReader::readFile(shader.filePath);
			shaderModules.tescModule = createShaderModule(tescShaderCode);
			break;
		}
		case TessellationEvaluation: {
			const auto teseShaderCode = Tools::FileReader::readFile(shader.filePath);
			shaderModules.teseModule = createShaderModule(teseShaderCode);
			break;
		}
		case Fragment: {
			const auto fragShaderCode = Tools::FileReader::readFile(shader.filePath);
			shaderModules.fragModule = createShaderModule(fragShaderCode);
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

void MRTShaderProgram::cleanup()
{
	shaderModules.cleanup();
	for (auto ub : uniformBuffers) {
		ub.destroy(true);
	}
	uniformBuffers.clear();
	
	for (auto ubM : uniformBufferMemory) {
		delete(ubM);
	}
	uniformBufferMemory.clear();

	if (dynamicBuffer.buffer) {
		dynamicBuffer.destroy(true);
	}

	if (dynamicBufferMemory) {
		delete(dynamicBufferMemory);
	}
}

void MRTShaderProgram::updateUniformBufferObject(const UniformBufferObject& ubo, size_t index)
{
	auto& ub = uniformBuffers[index];
	ub.map();
	ub.copyTo(&ubo, sizeof(ubo));
	ub.unmap();
}

void DeferredShaderProgram::updateFragmentShaderUniforms(const FragmentShaderUniforms& ubo, size_t index)
{
	auto& ub = uniformBuffers[index];
	ub.map();
	ub.copyTo(&ubo, sizeof(ubo));
	ub.unmap();
}

std::vector<VkPipelineShaderStageCreateInfo> MRTShaderProgram::getShaderStages() const
{
	return shaderModules.getShaderStages();
}

void MRTShaderProgram::createUniformBuffer()
{
	const auto& renderer = Sparkle::App::getHandle().getRenderBackend();

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(renderer->getPhysicalDevice(), &props);
	const auto alignment = props.limits.minUniformBufferOffsetAlignment;

	auto uboSize = sizeof(UniformBufferObject);
	if (alignment > 0) {
		uboSize = (uboSize + alignment - 1) & ~(alignment - 1);
	}
	const auto bufferSize = uboSize;
	for (auto& ub : uniformBuffers) {
		uniformBufferMemory.push_back(new vkExt::SharedMemory());
		renderer->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ub, *uniformBufferMemory.rbegin());
	}
}

void MRTShaderProgram::createDynamicBuffer(VkDeviceSize size)
{
	const auto& renderer = Sparkle::App::getHandle().getRenderBackend();

	if (dynamicBuffer.buffer) {
		dynamicBuffer.destroy(true);

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

	renderer->createBuffer(dynamicUboDataSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dynamicBuffer, dynamicBufferMemory);

	dynamicBuffer.map();

	dynamicBufferDirty = true;
	// getDescriptorInfos();
}

void MRTShaderProgram::updateDynamicUniformBufferObject(const std::vector<std::shared_ptr<Sparkle::Geometry::Node>>& meshes)
{
	if (!(dynamicBuffer.buffer) || meshes.size() * dUboAlignment > dynamicUboDataSize) {
		createDynamicBuffer(meshes.size());
	}
	for (auto i = 0u; i < meshes.size(); ++i) {
		const auto modelMat = meshes[i]->accumModel();
		const auto iUbo = (InstancedUniformBufferObject*)((uint64_t)dynamicUboData + i * (uint64_t)dUboAlignment);
		iUbo->model = glm::mat4(modelMat);
		iUbo->normal = glm::mat4(glm::transpose(glm::inverse(glm::mat3(modelMat))));
	}
	dynamicBuffer.copyTo(dynamicUboData, dynamicUboDataSize);
	dynamicBuffer.flush();
}

VkDescriptorBufferInfo MRTShaderProgram::getDescriptorInfos(size_t index) const
{
	const VkDescriptorBufferInfo uboModel = {
		uniformBuffers[index].buffer,
		0,
		sizeof(UniformBufferObject)
	};

	return uboModel;
}

/*
*	Deferred Shader Program
*/

DeferredShaderProgram::DeferredShaderProgram(const std::vector<ShaderSource>& shaderSources, size_t bufferCount)
{
	uniformBuffers.resize(bufferCount);

	for (const auto& shader : shaderSources) {
		switch (shader.type) {
		case Vertex: {
			const auto vtxShaderCode = Tools::FileReader::readFile(shader.filePath);
			shaderModules.vtxModule = createShaderModule(vtxShaderCode);
			break;
		}
		case TessellationControl: {
			const auto tescShaderCode = Tools::FileReader::readFile(shader.filePath);
			shaderModules.tescModule = createShaderModule(tescShaderCode);
			break;
		}
		case TessellationEvaluation: {
			const auto teseShaderCode = Tools::FileReader::readFile(shader.filePath);
			shaderModules.teseModule = createShaderModule(teseShaderCode);
			break;
		}
		case Fragment: {
			const auto fragShaderCode = Tools::FileReader::readFile(shader.filePath);
			shaderModules.fragModule = createShaderModule(fragShaderCode);
			break;
		}
		default:
			throw std::runtime_error("Unsupported Shader type for Graphics Shader! Did you mean to use a compute shader?");
			break;
		}
	}

	createUniformBuffer();
}

void DeferredShaderProgram::cleanup()
{
	shaderModules.cleanup();

	for (auto& ub : uniformBuffers) {
		if (ub.buffer) {
			ub.destroy(true);
		}
	}
	uniformBuffers.clear();

	for (auto& ubM : uniformBufferMemory) {
		delete(ubM);
	}
	uniformBufferMemory.clear();
}

std::vector<VkPipelineShaderStageCreateInfo> DeferredShaderProgram::getShaderStages() const
{
	return shaderModules.getShaderStages();
}

void DeferredShaderProgram::createUniformBuffer()
{
	const auto& renderer = Sparkle::App::getHandle().getRenderBackend();

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(renderer->getPhysicalDevice(), &props);
	const auto alignment = props.limits.minUniformBufferOffsetAlignment;

	auto uboSize = sizeof(FragmentShaderUniforms);
	if (alignment > 0) {
		uboSize = (uboSize + alignment - 1) & ~(alignment - 1);
	}
	const auto bufferSize = uboSize;
	for (auto& ub : uniformBuffers) {
		uniformBufferMemory.push_back(new vkExt::SharedMemory());
		renderer->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ub, *uniformBufferMemory.rbegin());
	}
}

VkDescriptorBufferInfo DeferredShaderProgram::getDescriptorInfos(size_t index) const
{
	const VkDescriptorBufferInfo fragUboModel = {
		uniformBuffers[index].buffer,
		0,
		sizeof(FragmentShaderUniforms)
	};
	return fragUboModel;
}

void ShaderProgramBase::cleanup()
{
	const auto device = Sparkle::App::getHandle().getRenderBackend()->getDevice();
	if (vtxModule) {
		vkDestroyShaderModule(device, vtxModule, nullptr);
	}
	if (tescModule) {
		vkDestroyShaderModule(device, tescModule, nullptr);
	}
	if (teseModule) {
		vkDestroyShaderModule(device, teseModule, nullptr);
	}
	if (fragModule) {
		vkDestroyShaderModule(device, fragModule, nullptr);
	}
}

std::vector<VkPipelineShaderStageCreateInfo> ShaderProgramBase::getShaderStages() const
{
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	if (vtxModule) {
		VkPipelineShaderStageCreateInfo stageInfo = {};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.module = vtxModule;
		stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		stageInfo.pName = "main";
		stages.push_back(stageInfo);
	}
	if (tescModule && teseModule) {
		{
			VkPipelineShaderStageCreateInfo stageInfo = {};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.module = tescModule;
			stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			stageInfo.pName = "main";
			stages.push_back(stageInfo);
		}
		{
			VkPipelineShaderStageCreateInfo stageInfo = {};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.module = teseModule;
			stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			stageInfo.pName = "main";
			stages.push_back(stageInfo);
		}
	}
	if (fragModule) {
		VkPipelineShaderStageCreateInfo stageInfo = {};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.module = fragModule;
		stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stageInfo.pName = "main";
		stages.push_back(stageInfo);
	}

	if (stages.empty()) {
		throw std::runtime_error("No shader stages enabled! Cannot create Pipeline without at least one stage");
	}

	return stages;
}

VkShaderModule Sparkle::Shaders::createShaderModule(const std::vector<char>& code)
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
