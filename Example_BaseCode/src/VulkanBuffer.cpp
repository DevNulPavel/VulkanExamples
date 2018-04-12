#include "VulkanBuffer.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "VulkanHelpers.h"
#include "Helpers.h"


VulkanBuffer::VulkanBuffer(VulkanLogicalDevicePtr logicalDevice, VkMemoryPropertyFlags properties, VkBufferUsageFlags usage, size_t dataSize):
    _logicalDevice(logicalDevice),
    _properties(properties),
    _usage(usage),
    _dataSize(dataSize){
    
    // VK_SHARING_MODE_EXCLUSIVE: изображение принадлежит одному семейству в один момент времени и должно быть явно передано другому семейству. Данный вариант обеспечивает наилучшую производительность.
    // VK_SHARING_MODE_CONCURRENT: изображение может быть использовано несколькими семействами без явной передачи.
    
    // Описание формата буффера
    VkBufferCreateInfo bufferInfo = {};
    memset(&bufferInfo, 0, sizeof(VkBufferCreateInfo));
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;     // Размер буффера
    bufferInfo.usage = usage;   // Использование данного буффера
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
    // Непосредственно создание буффера
    if (vkCreateBuffer(_logicalDevice->getDevice(), &bufferInfo, nullptr, &_buffer) != VK_SUCCESS) {
        LOG("Failed to create buffer!\n");
        throw std::runtime_error("Failed to create buffer!");
    }
        
    // Запрашиваем данные о необходимой памяти
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_logicalDevice->getDevice(), _buffer, &memRequirements);
    
    // Настройки аллокации буффера
    uint32_t memoryTypeIndex = findMemoryType(_logicalDevice->getBasePhysicalDevice()->getDevice(),
                                              memRequirements.memoryTypeBits,
                                              _properties);
    
    VkMemoryAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkMemoryAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    
    // Выделяем память для буффера
    if (vkAllocateMemory(_logicalDevice->getDevice(), &allocInfo, nullptr, &_bufferMemory) != VK_SUCCESS) {
        LOG("Failed to allocate vertex buffer memory!");
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }
   
    // Подцепляем память к буфферу
    // Последний параметр – смещение в области памяти. Т.к. эта память выделяется специально для буфера вершин, смещение просто 0.
    // Если же оно не будет равно нулю, то значение должно быть кратным memRequirements.alignment.
    vkBindBufferMemory(_logicalDevice->getDevice(), _buffer, _bufferMemory, 0);
}

VulkanBuffer::~VulkanBuffer(){
    vkDestroyBuffer(_logicalDevice->getDevice(), _buffer, nullptr);
    vkFreeMemory(_logicalDevice->getDevice(), _bufferMemory, nullptr);
}

void VulkanBuffer::uploadDataToBuffer(unsigned char* inputData, size_t dataSize, size_t offset){
    // Маппим видео-память в адресное пространство оперативной памяти
    void* data = nullptr;
    vkMapMemory(_logicalDevice->getDevice(), _bufferMemory, static_cast<VkDeviceSize>(offset), static_cast<VkDeviceSize>(dataSize), 0, &data);
    
    // Копируем вершины в память
    memcpy(data, inputData, dataSize);
    
    // Размапим
    vkUnmapMemory(_logicalDevice->getDevice(), _bufferMemory);
}

char* VulkanBuffer::map(size_t dataSize, size_t offset){
    // Маппим видео-память в адресное пространство оперативной памяти
    char* data = nullptr;
    vkMapMemory(_logicalDevice->getDevice(), _bufferMemory, static_cast<VkDeviceSize>(offset), static_cast<VkDeviceSize>(dataSize), 0, (void**)&data);
    return data;
}

void VulkanBuffer::unmap(){
    // Размапим
    vkUnmapMemory(_logicalDevice->getDevice(), _bufferMemory);
}

VkBuffer VulkanBuffer::getBuffer() const{
    return _buffer;
}

VulkanLogicalDevicePtr VulkanBuffer::getBaseDevice() const{
    return _logicalDevice;
}

VkMemoryPropertyFlags VulkanBuffer::getBaseProperties() const{
    return _properties;
}

VkBufferUsageFlags VulkanBuffer::getBaseUsage() const{
    return _usage;
}

size_t VulkanBuffer::getBaseSize() const{
    return _dataSize;
}
