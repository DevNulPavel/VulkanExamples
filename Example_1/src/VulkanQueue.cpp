#include "VulkanQueue.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "CommonConstants.h"


VulkanQueue::VulkanQueue(VulkanLogicalDevicePtr device, uint32_t familyIndex, uint32_t queueIndex, VkQueue inQueue):
    queue(inQueue),
    _device(device),
    _familyIndex(familyIndex),
    _queueIndex(queueIndex){
}

VulkanQueue::~VulkanQueue(){
    // Wait
    vkQueueWaitIdle(queue);
}
