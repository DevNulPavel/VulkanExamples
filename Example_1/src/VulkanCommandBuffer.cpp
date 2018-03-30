#include "VulkanCommandBuffer.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>


VulkanCommandBuffer::VulkanCommandBuffer(VulkanLogicalDevicePtr logicalDevice, VulkanCommandPoolPtr pool):
    _logicalDevice(logicalDevice),
    _pool(pool){

    // Параметр level определяет, будет ли выделенный буфер команд первичным или вторичным буфером команд:
    // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Может быть передан очереди для исполнения, но не может быть вызван из других буферов команд.
    // VK_COMMAND_BUFFER_LEVEL_SECONDARY: не может быть передан непосредственно, но может быть вызван из первичных буферов команд.
    VkCommandBufferAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkCommandBufferAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // Первичный буффер, которыый будет исполняться сразу
    allocInfo.commandPool = _pool->getPool();      // Пул комманд
    allocInfo.commandBufferCount = 1;
    
    // Аллоцируем коммандный буффер для задач, который будут закидываться в очередь
    vkAllocateCommandBuffers(_logicalDevice->getDevice(), &allocInfo, &_commandBuffer);
}

VulkanCommandBuffer::~VulkanCommandBuffer(){
    vkFreeCommandBuffers(_logicalDevice->getDevice(), _pool->getPool(), 1, &_commandBuffer);
}

VkCommandBuffer VulkanCommandBuffer::getBuffer() const{
    return _commandBuffer;
}

VulkanCommandPoolPtr VulkanCommandBuffer::getBasePool() const{
    return _pool;
}

VulkanLogicalDevicePtr VulkanCommandBuffer::getBaseDevice() const{
    return _logicalDevice;
}

void VulkanCommandBuffer::begin(VkCommandBufferUsageFlags usageFlags) {
    // Параметр flags определяет, как использовать буфер команд. Возможны следующие значения:
    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: Буфер команд будет перезаписан сразу после первого выполнения.
    // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: Это вторичный буфер команд, который будет в единственном render pass.
    // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: Буфер команд может быть представлен еще раз, если он так же уже находится в ожидании исполнения.
    
    // Настройки запуска коммандного буффера
    VkCommandBufferBeginInfo beginInfo = {};
    memset(&beginInfo, 0, sizeof(VkCommandBufferBeginInfo));
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = usageFlags; // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    
    // Запускаем буффер комманд
    vkBeginCommandBuffer(_commandBuffer, &beginInfo);
}

void VulkanCommandBuffer::begin(VkCommandBufferUsageFlags usageFlags, const VkCommandBufferInheritanceInfo& inheritance){
    // Параметр flags определяет, как использовать буфер команд. Возможны следующие значения:
    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: Буфер команд будет перезаписан сразу после первого выполнения.
    // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: Это вторичный буфер команд, который будет в единственном render pass.
    // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: Буфер команд может быть представлен еще раз, если он так же уже находится в ожидании исполнения.
    
    // Настройки запуска коммандного буффера
    VkCommandBufferBeginInfo beginInfo = {};
    memset(&beginInfo, 0, sizeof(VkCommandBufferBeginInfo));
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = usageFlags; // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    beginInfo.pInheritanceInfo = &inheritance;
    
    // Запускаем буффер комманд
    vkBeginCommandBuffer(_commandBuffer, &beginInfo);
}

void VulkanCommandBuffer::end() {
    // Заканчиваем прием комманд
    vkEndCommandBuffer(_commandBuffer);
}

void VulkanCommandBuffer::reset(VkCommandBufferResetFlags flags) {
    // Заканчиваем прием комманд
    vkResetCommandBuffer(_commandBuffer, flags);
}
