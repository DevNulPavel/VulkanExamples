#include "VulkanImage.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>


VulkanImage::VulkanImage():
    _image(VK_NULL_HANDLE){
}

VulkanImage::VulkanImage(VkImage image, VkFormat format, VkExtent2D size):
    _image(image),
    _format(format),
    _size(size),
    _needDestroy(false){
    // Удаляется извне
}

VulkanImage::VulkanImage(VulkanLogicalDevicePtr device, VkImage image, VkFormat format, VkExtent2D size, bool needDestroy):
    _device(device),
    _image(image),
    _format(format),
    _size(size),
    _needDestroy(needDestroy){
}

VulkanImage::~VulkanImage(){
    if (_needDestroy && _device && _image) {
        vkDestroyImage(_device->getDevice(), _image, nullptr);
    }
}

VkImage VulkanImage::getImage() const{
    return _image;
}

VkFormat VulkanImage::getFormat() const{
    return _format;
}

VkExtent2D VulkanImage::getSize() const{
    return _size;
}
