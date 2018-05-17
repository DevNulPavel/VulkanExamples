#include "VulkanDescriptorSet.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "VulkanHelpers.h"
#include "Helpers.h"

VulkanDescriptorSetImageInfo::VulkanDescriptorSetImageInfo():
    imageLayout(VK_IMAGE_LAYOUT_UNDEFINED){
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

VulkanDescriptorSetBufferInfo::VulkanDescriptorSetBufferInfo():
    offset(0),
    range(0){
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

VulkanDescriptorSetUpdateConfig::VulkanDescriptorSetUpdateConfig():
    type(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
    binding(0){
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

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
        LOG("Failed to allocate descriptor set!\n");
        throw std::runtime_error("Failed to allocate descriptor set!");
    }
}

VulkanDescriptorSet::~VulkanDescriptorSet(){
	vkFreeDescriptorSets(_logicalDevice->getDevice(), _pool->getPool(), 1, &_set);
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
    // Освобождаем старые задействованные объекты
    _usedObjects.clear();
    
    // Настройки дескрипторов
    std::vector<VkDescriptorImageInfo> imageInfos(configs.size());   // Специально, чтобы не происходила реаллокация данных при добавлении в вектор
    std::vector<VkDescriptorBufferInfo> bufferInfos(configs.size()); // Специально, чтобы не происходила реаллокация данных при добавлении в вектор
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.reserve(configs.size());
    
    for (uint32_t i = 0; i < configs.size(); i++) {
        const VulkanDescriptorSetUpdateConfig& config = configs[i];
        VkWriteDescriptorSet writeSet = {};
        memset(&writeSet, 0, sizeof(VkWriteDescriptorSet));
        
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = _set;   // Набор дескрипторов из пула
        writeSet.dstBinding = config.binding;                 // Биндится на 0м значении в шейдере
        writeSet.dstArrayElement = 0;            // 0 элемент
        writeSet.descriptorType = config.type; // Тип - юниформ буффер
        writeSet.descriptorCount = 1;            // 1н дескриптор
        switch (config.type) {
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:{
                VkDescriptorImageInfo imageInfo = {};
                memset(&imageInfo, 0, sizeof(VkDescriptorImageInfo));
                
                // Sampler
                if (config.imageInfo.sampler) {
                    _usedObjects.insert(config.imageInfo.sampler);
                    imageInfo.sampler = config.imageInfo.sampler->getSampler();
                }
                // Image
                if (config.imageInfo.imageView) {
                    _usedObjects.insert(config.imageInfo.imageView);
                    imageInfo.imageView = config.imageInfo.imageView->getImageView();
                    imageInfo.imageLayout = config.imageInfo.imageLayout;
                }
                
                // Сохраняем структуру, чтобы не уничтожилась
                imageInfos[i] = imageInfo;
                // Указатель на сохраненый объект
                writeSet.pImageInfo = &(imageInfos[i]);
            }break;
                
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:{
                VkDescriptorBufferInfo bufferInfo = {};
                memset(&bufferInfo, 0, sizeof(VkDescriptorBufferInfo));
                
                // Buffer
                if (config.bufferInfo.buffer) {
                    _usedObjects.insert(config.bufferInfo.buffer);
                    bufferInfo.buffer = config.bufferInfo.buffer->getBuffer();
                    bufferInfo.offset = config.bufferInfo.offset;
                    bufferInfo.range = config.bufferInfo.range;
                }
                
                // Сохраняем структуру, чтобы не уничтожилась
                bufferInfos[i] = bufferInfo;
                // Указатель на сохраненый объект
                writeSet.pBufferInfo = &(bufferInfos[i]);
            }break;
                
            default:
                LOG("Invalid descriptor set type\n");
                throw std::runtime_error("Invalid descriptor set type");
                break;
        }
        
        descriptorWrites.push_back(writeSet);
    }
    
    // Обновляем описание дескрипторов на устройстве
    vkUpdateDescriptorSets(_logicalDevice->getDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);    
}
