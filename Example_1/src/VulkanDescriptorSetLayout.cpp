#include "VulkanDescriptorSetLayout.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>


VulkanDescriptorSetConfig::VulkanDescriptorSetConfig():
    binding(0),
    desriptorsCount(0),
    desriptorType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
    descriptorStageFlags(VK_SHADER_STAGE_ALL){
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanLogicalDevicePtr device, const std::vector<VulkanDescriptorSetConfig>& configs):
    _device(device),
    _config(configs){
    
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(configs.size());
    
    // Создаем описание биндингов
    for (const VulkanDescriptorSetConfig& config: configs) {
        VkDescriptorSetLayoutBinding binding = {};
        memset(&binding, 0, sizeof(VkDescriptorSetLayoutBinding));
        binding.binding = config.binding;
        binding.descriptorCount = config.desriptorsCount;
        binding.descriptorType = config.desriptorType;
        binding.stageFlags = config.descriptorStageFlags;
        binding.pImmutableSamplers = nullptr; // Optional
        
        bindings.push_back(binding);
    }

    // Биндинги
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    memset(&layoutInfo, 0, sizeof(VkDescriptorSetLayoutCreateInfo));
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    // Создаем лаяут для шейдера
    if (vkCreateDescriptorSetLayout(_device->getDevice(), &layoutInfo, nullptr, &_layout) != VK_SUCCESS) {
        printf("Failed to create descriptor set layout!");
        fflush(stdout);
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout(){
    vkDestroyDescriptorSetLayout(_device->getDevice(), _layout, nullptr);
}

VkDescriptorSetLayout VulkanDescriptorSetLayout::getLayout() const{
    return _layout;
}

VulkanLogicalDevicePtr VulkanDescriptorSetLayout::getBaseDevice() const{
    return _device;
}

std::vector<VulkanDescriptorSetConfig> VulkanDescriptorSetLayout::getConfig() const{
    return _config;
}

