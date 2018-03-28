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
