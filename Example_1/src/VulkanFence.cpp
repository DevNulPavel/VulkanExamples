#include "VulkanFence.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "CommonConstants.h"
#include "Helpers.h"


VulkanFence::VulkanFence(VulkanLogicalDevicePtr device, bool signaled):
    _device(device),
    _fence(VK_NULL_HANDLE){
        
    // Создаем преграды для проверки завершения комманд отрисовки
    VkFenceCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkFenceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0; // Изначально создаем вытавленным
    VkResult fenceCreateStatus = vkCreateFence(_device->getDevice(), &createInfo, nullptr, &_fence);
    
    if (fenceCreateStatus != VK_SUCCESS) {
        LOG("Failed to create fence!");
        throw std::runtime_error("Failed to create fence!");
    }
}

VulkanFence::~VulkanFence(){
    vkDestroyFence(_device->getDevice(), _fence, nullptr);
}

VkFence VulkanFence::getFence() const{
    return _fence;
}

VulkanLogicalDevicePtr VulkanFence::getBaseDevice() const{
    return _device;
}

void VulkanFence::waitAndReset(){
    // Синхронизация с ожиданием на CPU завершения очереди выполнения комманд
    VkResult fenceStatus = vkGetFenceStatus(_device->getDevice(), _fence);
    if (fenceStatus != VK_SUCCESS) {
        LOG("Fence not set!\n");
    }
    VkResult waitStatus = vkWaitForFences(_device->getDevice(), 1, &_fence, VK_TRUE, std::numeric_limits<uint64_t>::max()-1);
    if (waitStatus == VK_SUCCESS) {
        /*VkResult resetFenceStatus = */vkResetFences(_device->getDevice(), 1, &_fence);
        //LOG("Fence waited + reset!\n");
    }
}
