#include "VulkanHelpers.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

// Подбираем тип памяти буффера вершин
uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    // Запрашиваем типы памяти физического устройства
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
    
    // Найдем тип памяти, который подходит для самого буфера
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    printf("Failed to find suitable memory type!\n");
    fflush(stdout);
    throw std::runtime_error("Failed to find suitable memory type!");
}


// Подбираем формат текстуры в зависимости от доступных на устройстве
VkFormat findSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        // Запрашиваем информацию для формата
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    
    printf("Failed to find supported format!\n");
    fflush(stdout);
    throw std::runtime_error("Failed to find supported format!");
}
