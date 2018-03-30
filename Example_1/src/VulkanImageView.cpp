#include "VulkanImageView.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>


VulkanImageView::VulkanImageView(VulkanLogicalDevicePtr device, VulkanImagePtr image, VkImageAspectFlags aspectFlags):
    _device(device),
    _image(image),
    _aspectFlags(aspectFlags),
    _imageView(VK_NULL_HANDLE){
    
    // Описание вьюшки
    VkImageViewCreateInfo viewInfo = {};
    memset(&viewInfo, 0, sizeof(VkImageViewCreateInfo));
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _image->getImage(); // Изображение
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 2D
    viewInfo.format = _image->getBaseFormat();   // Формат вьюшки
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдельным компонентам??
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдельным компонентам??
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдельным компонентам??
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдельным компонентам??
    viewInfo.subresourceRange.aspectMask = _aspectFlags; // Использование вью текстуры
    viewInfo.subresourceRange.baseMipLevel = 0; // 0 мипмаплевел
    viewInfo.subresourceRange.levelCount = image->getBaseMipmapsCount();
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    // Создаем имедж вью
    if (vkCreateImageView(_device->getDevice(), &viewInfo, nullptr, &_imageView) != VK_SUCCESS) {
        printf("Failed to create texture image view!\n");
        fflush(stdout);
        throw std::runtime_error("Failed to create texture image view!");
    }
}


VulkanImageView::~VulkanImageView(){
    vkDestroyImageView(_device->getDevice(), _imageView, nullptr);
}

VkImageView VulkanImageView::getImageView() const{
    return _imageView;
}

VulkanLogicalDevicePtr VulkanImageView::getBaseDevice() const{
    return _device;
}

VulkanImagePtr VulkanImageView::getBaseImage() const{
    return _image;
}

VkImageAspectFlags VulkanImageView::getBaseAspectFlags() const{
    return _aspectFlags;
}

