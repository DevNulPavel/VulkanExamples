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
                     const VkRenderPassCreateInfo& customPassInfo);
    VulkanRenderPass(VulkanLogicalDevicePtr device,
                     const VulkanRenderPassConfig& imageConfig,
                     const VulkanRenderPassConfig& depthConfig);
    VulkanRenderPass(VulkanLogicalDevicePtr device,
                     const VulkanRenderPassConfig& imageConfig);
    ~VulkanRenderPass();
    VkRenderPass getPass() const;
    VulkanRenderPassConfig getBaseImageConfig() const;
    VulkanRenderPassConfig getBaseDepthConfig() const;
    bool isCustom() const;
    
private:
    VulkanLogicalDevicePtr _device;
    VulkanRenderPassConfig  _imageConfig;
    VulkanRenderPassConfig  _depthConfig;
    bool _isCustom;
    VkRenderPass _renderPass;
    
private:
};

typedef std::shared_ptr<VulkanRenderPass> VulkanRenderPassPtr;

#endif
