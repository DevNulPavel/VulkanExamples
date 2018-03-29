#ifndef VULKAN_FRAME_BUFFER_H
#define VULKAN_FRAME_BUFFER_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanImageView.h"


struct VulkanFrameBuffer {
public:
    VulkanFrameBuffer(VulkanLogicalDevicePtr device,
                      VulkanRenderPassPtr renderPass,
                      const std::vector<VulkanImageViewPtr>& imageViews,
                      uint32_t width, uint32_t height);
    ~VulkanFrameBuffer();
    VkFramebuffer getBuffer() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    VulkanRenderPassPtr getBaseRenderPass() const;
    std::vector<VulkanImageViewPtr> getBaseImageViews() const;
    uint32_t getBaseWidth() const;
    uint32_t getBaseHeight() const;
    
private:
    VulkanLogicalDevicePtr _device;
    VulkanRenderPassPtr _renderPass;
    std::vector<VulkanImageViewPtr> _imageViews;
    uint32_t _width;
    uint32_t _height;
    VkFramebuffer _buffer;
    
private:
};

typedef std::shared_ptr<VulkanFrameBuffer> VulkanFrameBufferPtr;

#endif
