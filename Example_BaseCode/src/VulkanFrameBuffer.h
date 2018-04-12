#ifndef VULKAN_FRAME_BUFFER_H
#define VULKAN_FRAME_BUFFER_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanImageView.h"


class VulkanFrameBuffer {
public:
    VulkanFrameBuffer(VulkanLogicalDevicePtr device,
                      VulkanRenderPassPtr renderPass,
                      const std::vector<VulkanImageViewPtr>& imageViews,
                      VkExtent2D size);
    ~VulkanFrameBuffer();
    VkFramebuffer getBuffer() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    VulkanRenderPassPtr getBaseRenderPass() const;
    std::vector<VulkanImageViewPtr> getBaseImageViews() const;
    VkExtent2D getBaseSize() const;
    
private:
    VulkanLogicalDevicePtr _device;
    VulkanRenderPassPtr _renderPass;
    std::vector<VulkanImageViewPtr> _imageViews;
    VkExtent2D _size;
    VkFramebuffer _buffer;
    
private:
};

typedef std::shared_ptr<VulkanFrameBuffer> VulkanFrameBufferPtr;

#endif
