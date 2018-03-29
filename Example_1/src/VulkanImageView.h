#ifndef VULKAN_IMAGE_VIEW_H
#define VULKAN_IMAGE_VIEW_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"
#include "VulkanImage.h"


struct VulkanImageView {
public:
    VulkanImageView(VulkanLogicalDevicePtr device, VulkanImagePtr image, VkImageAspectFlags aspectFlags);
    ~VulkanImageView();
    VkImageView getImageView() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    VulkanImagePtr getBaseImage() const;
    VkImageAspectFlags getBaseAspectFlags() const;
    
private:
    VulkanLogicalDevicePtr _device;
    VulkanImagePtr _image;
    VkImageAspectFlags _aspectFlags;
    VkImageView _imageView;
    
private:
};

typedef std::shared_ptr<VulkanImageView> VulkanImageViewPtr;

#endif
