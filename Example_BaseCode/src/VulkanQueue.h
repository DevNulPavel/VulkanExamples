#ifndef VULKAN_QUEUE_H
#define VULKAN_QUEUE_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanQueuesFamiliesIndexes.h"
#include "VulkanSwapChainSupportDetails.h"
#include "VulkanLogicalDevice.h"
#include "VulkanCommandBuffer.h"

class VulkanQueue {
    friend VulkanLogicalDevice;
public:
    ~VulkanQueue();
    void submitBuffer(VulkanCommandBufferPtr buffer);
    void wait();
    VkQueue getQueue() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    uint32_t getFamilyIndex() const;
    uint32_t getQueueIndex() const;
    
protected:
    VulkanQueue(VulkanLogicalDevicePtr device, uint32_t familyIndex, uint32_t queueIndex, VkQueue queue);
    
private:
    VulkanLogicalDevicePtr _device;
    uint32_t _familyIndex;
    uint32_t _queueIndex;
    VkQueue _queue;

private:
};

typedef std::shared_ptr<VulkanQueue> VulkanQueuePtr;

#endif
