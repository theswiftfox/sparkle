#ifndef SHADER_H
#define SHADER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "VulkanExtension.h"
#include "Geometry.h"

#include <string>
#include <vector>

namespace Engine {
	namespace Shaders {

		class App;

		typedef struct UniformBufferObject {
			glm::mat4 view;
			glm::mat4 projection;
		} UBO;

		typedef struct InstancedUniformBufferObject {
			glm::mat4* model;
		} InstancedUBO;

		typedef struct VertexType {
			VkBool32* value;
		} VertexType;

		typedef struct TesselationControlSettings {
			VkBool32 enableDistortion;
		} TesselationControlSettings;

		typedef struct FragmentShaderSettings {
			glm::vec4 lightDirection;
			glm::vec4 lightColor;
			float ambient;
			float shininess;
			VkBool32 greyscale;
		} FragmentShaderSettings;

		typedef struct ShaderStages {
			VkBool32 vert;
			VkBool32 tes;
			VkBool32 geo;
			VkBool32 frag;
		} ShaderStages;

		static const ShaderStages ALL_STAGES = { VK_TRUE, VK_TRUE, VK_TRUE, VK_TRUE };
		static const ShaderStages VERTEX_FRAGMENT = { VK_TRUE, VK_FALSE, VK_FALSE, VK_TRUE };

		class ShaderProgram {
		public:
			virtual ~ShaderProgram() = default;
			virtual void cleanup();

		protected:
			VkDescriptorSetLayout pDescriptorSetLayout{};
			VkDescriptorSet pDescriptorSet{};
			VkDescriptorPool pDescriptorPool{};

			VkRenderPass pRenderPass{};
			VkPipelineLayout pPipelineLayout{};
			VkPipeline pGraphicsPipeline{};

			VkViewport viewport{};

			std::vector<VkFramebuffer> swapChainFramebuffers;

			virtual void createDescriptorSetLayout() = 0;
			virtual void createDescriptorPool() = 0;

			virtual void createRenderPass() = 0;
			virtual void createGraphicsPipeline(ShaderStages enabledStages) = 0;
			virtual void createFramebuffers() = 0;
			static VkShaderModule createShaderModule(const std::vector<char>& code);
		};

		class MainShaderProgram : public ShaderProgram {
		public:
			MainShaderProgram(VkViewport targetViewport, const std::string& vtxShaderFile, const std::string& tescShaderFile, const std::string& teseShaderFile, const std::string& fragShaderFile, size_t objectInstances, ShaderStages enabledStages = VERTEX_FRAGMENT);
			virtual ~MainShaderProgram();

			void cleanup() override;

			void updateUniformBufferObject(const UBO& ubo);
			void updateDynamicUniformBufferObject(const std::vector<std::shared_ptr<Geometry::Mesh>>& meshes);
			void updateTesselationControlSettings(const TesselationControlSettings& sets);
			void updateFragmentShaderSettings(const FragmentShaderSettings& sets);

			auto getFramebufferPtr() { return swapChainFramebuffers.data(); }
			auto getDynamicAlignment() const { return dynamicAlignment; }
			auto getDynamicStatusAlignment() const { return statusAlignment; }
			auto getFramebufferPtrs() const { return swapChainFramebuffers; }
			auto getRenderPassPtr() const { return pRenderPass; }
			auto getGraphicsPipelinePtr() const { return pGraphicsPipeline; }
			auto getPipelineLayoutPtr() const { return pPipelineLayout; }
			auto getDescriptorSetPtr() const { return pDescriptorSet; }

		private:
			std::vector<char> vtxShaderCode, tescShaderCode, teseShaderCode, fragShaderCode;

			vkExt::Buffer pUniformBuffer;
			vkExt::SharedMemory* pUniformBufferMemory;

			vkExt::Buffer pDynamicBuffer;
			vkExt::SharedMemory* pDynamicBufferMemory;

			vkExt::Buffer pDynamicStatusBuffer;
			vkExt::SharedMemory* pDynamicStatusMemory;

			size_t teseSettingsOffset{};
			size_t fragSettingsOffset{};

			size_t objectCount;
			uint32_t dynamicAlignment{};
			uint32_t statusAlignment{};

			InstancedUBO dynamicUboData{};
			VkDeviceSize dynamicUboDataSize{};
			VertexType dynamicStatusData{};
			VkDeviceSize dynamicStatusSize{};

			bool withTesselation;

			void create(ShaderStages stages);

			void createDescriptorSetLayout() override;
			void createDescriptorPool() override;
			void createDescriptorSet();
			void updateDescriptorSet();

			void createRenderPass() override;
			void createGraphicsPipeline(ShaderStages enabledStages) override;
			void createFramebuffers() override;
			void createUniformBuffer();
			void createDynamicBuffer(VkDeviceSize size);

		};
	}
}
#endif // SHADER_H
