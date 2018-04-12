#ifndef VULKAN_SAMPLER_H
#define VULKAN_SAMPLER_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"
#include "VulkanResource.h"


class VulkanSampler: public VulkanResource {
public:
    VulkanSampler(VulkanLogicalDevicePtr device,
                  VkFilter minFiler, VkFilter magFilter,
                  VkSamplerAddressMode mode,
                  uint32_t maxMipLevels, uint32_t minMipLevel = 0, float mipLevelBias = 0.0f);
    ~VulkanSampler();
    VkSampler getSampler() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    VkFilter getBaseMinFiler() const;
    VkFilter getBaseMagFilter() const;
    VkSamplerAddressMode getBaseMode() const;
    uint32_t getBaseMaxMipLevel() const;
    
private:
    VulkanLogicalDevicePtr _device;
    VkFilter _minFiler;
    VkFilter _magFilter;
    VkSamplerAddressMode _mode;
    uint32_t _maxMipLevels;
    VkSampler _sampler;
    
private:
};

typedef std::shared_ptr<VulkanSampler> VulkanSamplerPtr;

#endif
