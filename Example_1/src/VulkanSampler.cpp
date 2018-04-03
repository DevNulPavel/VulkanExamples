#include "VulkanSampler.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "Helpers.h"


VulkanSampler::VulkanSampler(VulkanLogicalDevicePtr device, VkFilter minFiler, VkFilter magFilter, VkSamplerAddressMode mode, uint32_t maxMipLevels):
    _device(device),
    _minFiler(minFiler),
    _magFilter(magFilter),
    _mode(mode),
    _maxMipLevels(maxMipLevels){
        
    // Описание семплирования для текстуры
    VkSamplerCreateInfo samplerInfo = {};
    memset(&samplerInfo, 0, sizeof(VkSamplerCreateInfo));
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.minFilter = _minFiler;   // Линейное
    samplerInfo.magFilter = _magFilter;   // Линейное
    samplerInfo.addressModeU = _mode;   // Ограничение по границе
    samplerInfo.addressModeV = _mode;   // Ограничение по границе
    samplerInfo.addressModeW = _mode;   // Ограничение по границе
    samplerInfo.anisotropyEnable = VK_FALSE;    // Анизотропная фильтрация
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(_maxMipLevels);
    
    // Создаем семплер
    if (vkCreateSampler(_device->getDevice(), &samplerInfo, nullptr, &_sampler) != VK_SUCCESS) {
        LOG("Failed to create texture sampler!\n");
        throw std::runtime_error("Failed to create texture sampler!");
    }
}

VulkanSampler::~VulkanSampler(){
    vkDestroySampler(_device->getDevice(), _sampler, nullptr);
}

VkSampler VulkanSampler::getSampler() const{
    return _sampler;
}

VulkanLogicalDevicePtr VulkanSampler::getBaseDevice() const{
    return _device;
}

VkFilter VulkanSampler::getBaseMinFiler() const{
    return _minFiler;
}

VkFilter VulkanSampler::getBaseMagFilter() const{
    return _magFilter;
}

VkSamplerAddressMode VulkanSampler::getBaseMode() const{
    return _mode;
}
