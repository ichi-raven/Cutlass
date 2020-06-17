#pragma once
#include <vulkan/vulkan.hpp>

namespace Cutlass
{
    struct ImageObject
    {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
    };
}