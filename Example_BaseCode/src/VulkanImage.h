#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanPhysicalDevice.h"
#include "VulkanLogicalDevice.h"
#include "VulkanResource.h"

class VulkanImage: public VulkanResource {
public:
    VulkanImage();
    VulkanImage(VkImage image, VkFormat format, VkExtent2D size);
    VulkanImage(VulkanLogicalDevicePtr device,
                VkImage image,
                VkFormat format,
                VkExtent2D size,
                bool needDestroy);
    VulkanImage(VulkanLogicalDevicePtr logicDevice,
                VkExtent2D size,
                VkFormat format,
                VkImageTiling tiling,
                VkImageLayout layout,
                VkImageUsageFlags usage,
                VkMemoryPropertyFlags properties,
                uint32_t mipmapsCount = 1,
                VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
    ~VulkanImage();
    VkImage getImage() const;
    VkDeviceMemory getImageMemory() const;
    VkSubresourceLayout getSubresourceLayout(VkImageAspectFlags aspect, uint32_t mipLevel) const; // Получаем лаяут картинки
    void uploadDataToImage(VkImageAspectFlags aspect, uint32_t mipLevel, unsigned char* imageSourceData, size_t dataSize); // Загружаем данные в картинку
    VulkanLogicalDevicePtr getBaseDevice() const;
    VkFormat getBaseFormat() const;
    VkExtent2D getBaseSize() const;
    VkImageTiling getBaseTiling() const;
    VkImageLayout getBaseLayout() const;
    VkImageUsageFlags getBaseUsage() const;
    VkMemoryPropertyFlags getBaseProperties() const;
    uint32_t getBaseMipmapsCount() const;
    VkSampleCountFlagBits getBaseSampleCount() const;
    void setNewLayout(VkImageLayout layout);
    
private:
    VulkanLogicalDevicePtr _logicalDevice;
    VkImage _image;
    VkDeviceMemory _imageMemory;
    VkFormat _format;
    VkExtent2D _size;
    VkImageTiling _tiling;
    VkImageLayout _layout;
    VkImageUsageFlags _usage;
    VkMemoryPropertyFlags _properties;
    uint32_t _mipmapsCount;
    VkSampleCountFlagBits _sampleCount;
    bool _needDestroy;
    
private:
    void createImage(uint32_t width, uint32_t height,
                     VkFormat format,
                     VkImageTiling tiling,
                     VkImageLayout layout,
                     VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     uint32_t mipmapsCount,
                     VkSampleCountFlagBits sampleCount);
};

typedef std::shared_ptr<VulkanImage> VulkanImagePtr;

#endif
