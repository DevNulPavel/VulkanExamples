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
    wait();
}

void VulkanQueue::submitBuffer(VulkanCommandBufferPtr buffer){
    VkCommandBuffer commandBuffer = buffer->getBuffer();
    
    // Структура с описанием отправки в буффер
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    // Отправляем задание на отрисовку в буффер отрисовки
    vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE);
}

void VulkanQueue::wait(){
    // TODO: Ожидание передачи комманды в очередь на GPU???
    // Как альтернативу - можно использовать Fence
    vkQueueWaitIdle(_queue);
    
    /*std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();
     vkQueueWaitIdle(vulkanGraphicsQueue);
     std::chrono::high_resolution_clock::time_point time2 = std::chrono::high_resolution_clock::now();
     std::chrono::high_resolution_clock::duration elapsed = time2 - time1;
     int64_t elapsedMicroSec = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
     printf("Wait duration (vkQueueWaitIdle(vulkanGraphicsQueue)): %lldmicroSec\n", elapsedMicroSec);
     fflush(stdout);*/
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
