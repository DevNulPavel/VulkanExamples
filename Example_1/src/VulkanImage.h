#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanPhysicalDevice.h"
#include "VulkanLogicalDevice.h"

struct VulkanImage {
public:
    VulkanImage();
    VulkanImage(VkImage image, VkFormat format, VkExtent2D size);
    VulkanImage(VulkanLogicalDevicePtr device,
                VkImage image,
                VkFormat format,
                VkExtent2D size,
                bool needDestroy);
    VulkanImage(VulkanLogicalDevicePtr logicDevice,
                uint32_t width, uint32_t height,
                VkFormat format,
                VkImageTiling tiling,
                VkImageLayout layout,
                VkImageUsageFlags usage,
                VkMemoryPropertyFlags properties,
                uint32_t mipmapsCount);
    ~VulkanImage();
    VkImage getImage() const;
    VkDeviceMemory getImageMemory() const;
    VkFormat getFormat() const;
    VkExtent2D getSize() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    
private:
    VulkanLogicalDevicePtr _logicalDevice;
    VkImage _image;
    VkDeviceMemory _imageMemory;
    VkFormat _format;
    VkExtent2D _size;
    bool _needDestroy;
    
private:
    void createImage(uint32_t width, uint32_t height,
                     VkFormat format,
                     VkImageTiling tiling,
                     VkImageLayout layout,
                     VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     uint32_t mipmapsCount);
};

typedef std::shared_ptr<VulkanImage> VulkanImagePtr;

#endif
