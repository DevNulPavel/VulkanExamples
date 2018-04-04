#include "VulkanFrameBuffer.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "Helpers.h"

VulkanFrameBuffer::VulkanFrameBuffer(VulkanLogicalDevicePtr device,
                                     VulkanRenderPassPtr renderPass,
                                     const std::vector<VulkanImageViewPtr>& imageViews,
                                     VkExtent2D size):
    _device(device),
    _renderPass(renderPass),
    _imageViews(imageViews),
    _size(size){
        
    // Список аттачментов
    std::vector<VkImageView> attachments;
    attachments.reserve(imageViews.size());
    for (const VulkanImageViewPtr& imageView: _imageViews) {
        attachments.push_back(imageView->getImageView());
    }
        
    // Информация для создания фрейб-буфферов
    VkFramebufferCreateInfo framebufferInfo = {};
    memset(&framebufferInfo, 0, sizeof(VkFramebufferCreateInfo));
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = _renderPass->getPass();  // Совместимый рендер-проход
    framebufferInfo.attachmentCount = attachments.size();   // Аттачменты
    framebufferInfo.pAttachments = attachments.data();      // Данные аттачментов
    framebufferInfo.width = _size.width;    // Размеры
    framebufferInfo.height = _size.height;  // Размеры
    framebufferInfo.layers = 1;
    
    if (vkCreateFramebuffer(_device->getDevice(), &framebufferInfo, nullptr, &_buffer) != VK_SUCCESS) {
        LOG("Failed to create framebuffer!");
        throw std::runtime_error("Failed to create framebuffer!");
    }
}

VulkanFrameBuffer::~VulkanFrameBuffer(){
    vkDestroyFramebuffer(_device->getDevice(), _buffer, nullptr);
}

VkFramebuffer VulkanFrameBuffer::getBuffer() const{
    return _buffer;
}

VulkanLogicalDevicePtr VulkanFrameBuffer::getBaseDevice() const{
    return _device;
}

VulkanRenderPassPtr VulkanFrameBuffer::getBaseRenderPass() const{
    return _renderPass;
}

std::vector<VulkanImageViewPtr> VulkanFrameBuffer::getBaseImageViews() const{
    return _imageViews;
}

VkExtent2D VulkanFrameBuffer::getBaseSize() const{
    return _size;
}

