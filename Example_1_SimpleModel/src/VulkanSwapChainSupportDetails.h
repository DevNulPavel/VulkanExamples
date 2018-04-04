#ifndef VULKAN_SWAPCHAIN_DETAILS_H
#define VULKAN_SWAPCHAIN_DETAILS_H

#include <vector>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


// Структура с описанием возможностей свопчейна отрисовки
struct VulkanSwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

#endif
