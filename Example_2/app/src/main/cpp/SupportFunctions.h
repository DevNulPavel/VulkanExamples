#ifndef SUPPORT_FUNCTIONS_H
#define SUPPORT_FUNCTIONS_H

#include <vector>
#include <string>
#include <android/asset_manager.h>
#include <android/log.h>
#include <vulkan_wrapper.h>


#define  LOG_TAG    "VulkanTest"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

// Читаем побайтово файлик
std::vector<unsigned char> readFile(AAssetManager* androidAssetManager, const std::string& filename);

// Подбираем тип памяти под свойства
uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

// Создаем изображение
void createImage(VkDevice vulkanLogicalDevice, VkPhysicalDevice physDevice,
                 uint32_t width, uint32_t height,
                 VkFormat format, VkImageTiling tiling,
                 VkImageLayout layout, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties,
                 VkImage& image, VkDeviceMemory& imageMemory);

// Создание вью для изображения
void createImageView(VkDevice vulkanLogicalDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView);



#endif
