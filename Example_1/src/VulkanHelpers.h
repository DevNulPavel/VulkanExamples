#ifndef VULKAN_HELPERS_H
#define VULKAN_HELPERS_H

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Подбираем тип памяти буффера вершин
uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

#endif
