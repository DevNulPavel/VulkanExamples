#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"

struct VulkanImage {
public:
    VulkanImage();
    VulkanImage(VkImage image, VkFormat format, VkExtent2D size);
    VulkanImage(VulkanLogicalDevicePtr device, VkImage image, VkFormat format, VkExtent2D size, bool needDestroy);
    ~VulkanImage();
    VkImage getImage() const;
    VkFormat getFormat() const;
    VkExtent2D getSize() const;
    
private:
    VulkanLogicalDevicePtr _device;
    VkImage _image;
    VkFormat _format;
    VkExtent2D _size;
    bool _needDestroy;
    
private:
};

typedef std::shared_ptr<VulkanImage> VulkanImagePtr;

#endif
