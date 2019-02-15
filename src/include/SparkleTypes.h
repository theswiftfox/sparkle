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
}

namespace Geometry {
    typedef struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            const VkVertexInputBindingDescription binding = {
                0,
                sizeof(Vertex),
                VK_VERTEX_INPUT_RATE_VERTEX
            };
            return binding;
        }

        static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions()
        {
            const std::array<VkVertexInputAttributeDescription, 5> attributes = {
                { { 0,
                      0,
                      VK_FORMAT_R32G32B32_SFLOAT,
                      offsetof(Vertex, position) },
                    { 1,
                        0,
                        VK_FORMAT_R32G32B32_SFLOAT,
                        offsetof(Vertex, normal) },
                    { 2,
                        0,
                        VK_FORMAT_R32G32B32_SFLOAT,
                        offsetof(Vertex, tangent) },
                    { 3,
                        0,
                        VK_FORMAT_R32G32B32_SFLOAT,
                        offsetof(Vertex, bitangent) },
                    { 4,
                        0,
                        VK_FORMAT_R32G32_SFLOAT,
                        offsetof(Vertex, texCoord) } }
            };
            return attributes;
        }

    } Vertex;
}
}