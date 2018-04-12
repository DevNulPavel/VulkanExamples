#ifndef VULKAN_BUFFER_H
#define VULKAN_BUFFER_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"
#include "VulkanResource.h"


class VulkanBuffer: public VulkanResource {
public:
    VulkanBuffer(VulkanLogicalDevicePtr logicalDevice, VkMemoryPropertyFlags properties, VkBufferUsageFlags usage, size_t dataSize);
    ~VulkanBuffer();
    void uploadDataToBuffer(unsigned char* data, size_t dataSize, size_t offset = 0);
    char* map(size_t dataSize, size_t offset = 0);
    void unmap();
    VkBuffer getBuffer() const;
    VkDeviceMemory getMemory() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    VkMemoryPropertyFlags getBaseProperties() const;
    VkBufferUsageFlags getBaseUsage() const;
    size_t getBaseSize() const;
    
private:
    VulkanLogicalDevicePtr _logicalDevice;
    VkMemoryPropertyFlags _properties;
    VkBufferUsageFlags _usage;
    size_t _dataSize;
    VkBuffer _buffer;
    VkDeviceMemory _bufferMemory;
    
private:
};

typedef std::shared_ptr<VulkanBuffer> VulkanBufferPtr;

#endif
