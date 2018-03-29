#ifndef VULKAN_HELPERS_H
#define VULKAN_HELPERS_H

#include <vector>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


// Подбираем тип памяти буффера вершин
uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);


// Подбираем формат текстуры в зависимости от доступных на устройстве
VkFormat findSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);


#endif
