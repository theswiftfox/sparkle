/*
*   SparkleTypes.h
*   
*   Custom Types used in Sparkle
*   
*   Copyright (C) 2019 by Patrick Gantner
*
*   This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

namespace Sparkle {
namespace Vulkan {
	struct RequiredQueueFamilyIndices {
		int graphicsFamily = -1;
		int presentFamily = -1;
		int computeFamily = -1;

		bool allPresent() const
		{
			return graphicsFamily >= 0 && presentFamily >= 0 && computeFamily >= 0;
		}
	};

	struct SwapChainSupportInfo {
		VkSurfaceCapabilitiesKHR capabilities {};
		std::vector<VkSurfaceFormatKHR> formats {};
		std::vector<VkPresentModeKHR> presentModes {};
	};
} // namespace Vulkan
} // namespace Sparkle