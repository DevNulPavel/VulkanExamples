#ifndef VULKAN_HELPERS_H
#define VULKAN_HELPERS_H

#include <vector>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanQueue.h"
#include "VulkanImage.h"


// Подбираем тип памяти буффера вершин
uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);


// Подбираем формат текстуры в зависимости от доступных на устройстве
VkFormat findSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

// Есть ли поддержка трафарета в формате
bool hasStencilComponent(VkFormat format);

// Перевод изображения из одного лаяута в другой (из одного способа использования в другой)
void transitionImageLayout(VulkanCommandBufferPtr commandBuffer,
                           VulkanImagePtr image,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout,
                           uint32_t startMipmapLevel,
                           uint32_t levelsCount,
                           VkImageAspectFlags aspectFlags,
                           VkPipelineStageFlagBits srcStage,
                           VkPipelineStageFlagBits dstStage,
                           VkAccessFlags srcAccessBarrier,
                           VkAccessFlags dstAccessBarrier);

// Закидываем в очередь операцию копирования текстуры
void copyImage(VulkanCommandBufferPtr commandBuffer, VulkanImagePtr srcImage, VulkanImagePtr dstImage);

// Создаем мипмапы для картинок
void generateMipmapsForImage(VulkanCommandBufferPtr commandBuffer, VulkanImagePtr image);

// Запуск коммандного буффера на получение комманд
VulkanCommandBufferPtr beginSingleTimeCommands(VulkanLogicalDevicePtr device, VulkanCommandPoolPtr pool);

// Завершение коммандного буффера + отправка в очередь
void endAndQueueSingleTimeCommands(VulkanCommandBufferPtr commandBuffer, VulkanQueuePtr queue);

// Создание текстуры из изображения на диске
VulkanImagePtr createTextureImage(VulkanLogicalDevicePtr device, VulkanQueuePtr queue, VulkanCommandPoolPtr pool, const std::string& path);

#endif
