#ifndef VULKAN_RENDER_PASS_H
#define VULKAN_RENDER_PASS_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"

struct VulkanRenderPassConfig{
    VkFormat format;
    VkAttachmentLoadOp loadOp;
    VkAttachmentStoreOp storeOp;
    VkImageLayout initLayout;
    VkImageLayout finalLayout;
    VkImageLayout refLayout;
    
    VulkanRenderPassConfig();
};

struct VulkanRenderPass {
public:
    VulkanRenderPass(VulkanLogicalDevicePtr device,
                     VulkanRenderPassConfig imageConfig,
                     VulkanRenderPassConfig depthConfig);
    VulkanRenderPass(VulkanLogicalDevicePtr device,
                     VulkanRenderPassConfig imageConfig);
    ~VulkanRenderPass();
    VulkanRenderPassConfig getBaseImageConfig() const;
    VulkanRenderPassConfig getBaseDepthConfig() const;
    
private:
    VulkanLogicalDevicePtr _device;
    VulkanRenderPassConfig  _imageConfig;
    VulkanRenderPassConfig  _depthConfig;
    VkRenderPass _renderPass;
    
private:
    // Создание рендер-прохода
    void createRenderPass();
};

typedef std::shared_ptr<VulkanRenderPass> VulkanRenderPassPtr;

#endif
