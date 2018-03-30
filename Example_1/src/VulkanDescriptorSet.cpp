#include "VulkanDescriptorSet.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "VulkanHelpers.h"

VulkanDescriptorSetUpdateConfig::VulkanDescriptorSetUpdateConfig():
    type(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
    binding(0){
}


VulkanDescriptorSet::VulkanDescriptorSet(VulkanLogicalDevicePtr logicalDevice, VulkanDescriptorSetLayoutPtr layout, VulkanDescriptorPoolPtr pool):
    _logicalDevice(logicalDevice),
    _layout(layout),
    _pool(pool){

    // Настройки аллокатора для дескрипторов ресурсов
    VkDescriptorSetLayout layouts[] = {_layout->getLayout()};
    VkDescriptorSetAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkDescriptorSetAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _pool->getPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;
    
    // Аллоцируем дескрипторы в пуле
    if (vkAllocateDescriptorSets(_logicalDevice->getDevice(), &allocInfo, &_set) != VK_SUCCESS) {
        printf("Failed to allocate descriptor set!\n");
        fflush(stdout);
        throw std::runtime_error("Failed to allocate descriptor set!");
    }
}

VulkanDescriptorSet::~VulkanDescriptorSet(){
}

VkDescriptorSet VulkanDescriptorSet::getSet() const{
    return _set;
}

VulkanLogicalDevicePtr VulkanDescriptorSet::getBaseDevice() const{
    return _logicalDevice;
}

VulkanDescriptorSetLayoutPtr VulkanDescriptorSet::getBaseLayout() const{
    return _layout;
}

VulkanDescriptorPoolPtr VulkanDescriptorSet::getBasePool() const{
    return _pool;
}

void VulkanDescriptorSet::updateDescriptorSet(const std::vector<VulkanDescriptorSetUpdateConfig>& configs){
    // Настройки дескрипторов
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.reserve(configs.size());
    
    for (const VulkanDescriptorSetUpdateConfig& config: configs) {
        VkWriteDescriptorSet writeSet = {};
        memset(&writeSet, 0, sizeof(VkWriteDescriptorSet));
        
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = _set;   // Набор дескрипторов из пула
        writeSet.dstBinding = config.binding;                 // Биндится на 0м значении в шейдере
        writeSet.dstArrayElement = 0;            // 0 элемент
        writeSet.descriptorType = config.type; // Тип - юниформ буффер
        writeSet.descriptorCount = 1;            // 1н дескриптор
        if (config.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            writeSet.pImageInfo = &config.imageInfo;
        }else if(config.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER){
            writeSet.pBufferInfo = &config.bufferInfo;
        }
        
        descriptorWrites.push_back(writeSet);
    }
    
    // Обновляем описание дескрипторов на устройстве
    vkUpdateDescriptorSets(_logicalDevice->getDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);    
}
