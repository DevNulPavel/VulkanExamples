#ifndef SWAPCHAIN_IMAGE_RESOURCE_H
#define SWAPCHAIN_IMAGE_RESOURCE_H

#include "CommonIncludes.h"
#include "CommonDefines.h"

struct SwapchainImageResources {
    VkImage image;
    VkCommandBuffer cmd;
    VkCommandBuffer graphics_to_present_cmd;
    VkImageView view;
    VkBuffer uniform_buffer;
    VkDeviceMemory uniform_memory;
    VkFramebuffer framebuffer;
    VkDescriptorSet descriptor_set;
};

#endif
