#include "VulkanQueue.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "CommonConstants.h"


VulkanQueue::VulkanQueue(VulkanLogicalDevicePtr device, uint32_t familyIndex, uint32_t queueIndex, VkQueue inQueue):
    _device(device),
    _familyIndex(familyIndex),
    _queueIndex(queueIndex),
    _queue(inQueue){
}

VulkanQueue::~VulkanQueue(){
    // Wait
    vkQueueWaitIdle(_queue);
}

VkQueue VulkanQueue::getQueue() const{
    return _queue;
}

VulkanLogicalDevicePtr VulkanQueue::getBaseDevice() const{
    return _device;
}

uint32_t VulkanQueue::getFamilyIndex() const {
    return _familyIndex;
}

uint32_t VulkanQueue::getQueueIndex() const {
    return _queueIndex;
}
