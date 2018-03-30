#ifndef VULKAN_SAMPLER_H
#define VULKAN_SAMPLER_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"


struct VulkanSampler {
public:
    VulkanSampler(VulkanLogicalDevicePtr device, VkFilter minFiler, VkFilter magFilter, VkSamplerAddressMode mode);
    ~VulkanSampler();
    VkSampler getSampler() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    VkFilter getBaseMinFiler() const;
    VkFilter getBaseMagFilter() const;
    VkSamplerAddressMode getBaseMode() const;
    
private:
    VulkanLogicalDevicePtr _device;
    VkFilter _minFiler;
    VkFilter _magFilter;
    VkSamplerAddressMode _mode;
    VkSampler _sampler;
    
private:
};

typedef std::shared_ptr<VulkanSampler> VulkanSamplerPtr;

#endif
