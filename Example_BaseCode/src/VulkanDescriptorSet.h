#ifndef VULKAN_DESCRIPTOR_SET_H
#define VULKAN_DESCRIPTOR_SET_H

#include <memory>
#include <set>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorPool.h"
#include "VulkanSampler.h"
#include "VulkanImageView.h"
#include "VulkanBuffer.h"
#include "VulkanResource.h"


struct VulkanDescriptorSetImageInfo{
    VulkanSamplerPtr sampler;
    VulkanImageViewPtr imageView;
    VkImageLayout imageLayout;
    
    VulkanDescriptorSetImageInfo();
};

struct VulkanDescriptorSetBufferInfo{
    VulkanBufferPtr buffer;
    VkDeviceSize offset;
    VkDeviceSize range;
    
    VulkanDescriptorSetBufferInfo();
};

struct VulkanDescriptorSetUpdateConfig {
    VkDescriptorType type;
    uint32_t binding;
    VulkanDescriptorSetImageInfo imageInfo;
    VulkanDescriptorSetBufferInfo bufferInfo;
    
    VulkanDescriptorSetUpdateConfig();
};

class VulkanDescriptorSet {
public:
    VulkanDescriptorSet(VulkanLogicalDevicePtr logicalDevice, VulkanDescriptorSetLayoutPtr layout, VulkanDescriptorPoolPtr pool);
    ~VulkanDescriptorSet();
    void updateDescriptorSet(const std::vector<VulkanDescriptorSetUpdateConfig>& configs);
    VkDescriptorSet getSet() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    VulkanDescriptorSetLayoutPtr getBaseLayout() const;
    VulkanDescriptorPoolPtr getBasePool() const;
    
private:
    VulkanLogicalDevicePtr _logicalDevice;
    VulkanDescriptorSetLayoutPtr _layout;
    VulkanDescriptorPoolPtr _pool;
    std::set<VulkanResourcePtr> _usedObjects;
    VkDescriptorSet _set;
    
private:
};

typedef std::shared_ptr<VulkanDescriptorSet> VulkanDescriptorSetPtr;

#endif
