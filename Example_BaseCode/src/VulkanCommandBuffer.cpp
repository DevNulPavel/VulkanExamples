#include "VulkanCommandBuffer.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "Helpers.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

VulkanRenderPassBeginInfo::VulkanRenderPassBeginInfo():
    renderArea({{0, 0}, {0, 0}}){
}

///////////////////////////////////////////////////////////////////////////////////////////////////

VulkanImageBarrierInfo::VulkanImageBarrierInfo():
    oldLayout(VK_IMAGE_LAYOUT_UNDEFINED),
    newLayout(VK_IMAGE_LAYOUT_UNDEFINED),
    startMipmapLevel(0),
    levelsCount(0),
    aspectFlags(VK_IMAGE_ASPECT_COLOR_BIT),
    srcAccessBarrier(0),
    dstAccessBarrier(0){
}

///////////////////////////////////////////////////////////////////////////////////////////////////

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
    // Очищаем задействованные объекты
    _usedObjects.clear();
    
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
    // Очищаем задействованные объекты
    _usedObjects.clear();
    
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
	if (vkBeginCommandBuffer(_commandBuffer, &beginInfo) != VK_SUCCESS) {
		LOG("Failed to begin command buffer!\n");
		throw std::runtime_error("Failed to begin command buffer!");
	}
}

void VulkanCommandBuffer::end() {
    // Заканчиваем прием комманд
	if (vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS) {
		LOG("Failed to record command buffer!\n");
		throw std::runtime_error("Failed to record command buffer!");
	}
}

void VulkanCommandBuffer::reset(VkCommandBufferResetFlags flags) {
    // Очищаем задействованные объекты
    _usedObjects.clear();
    
    // Заканчиваем прием комманд
	if (vkResetCommandBuffer(_commandBuffer, flags) != VK_SUCCESS) {
		LOG("Failed to reset command buffer!\n");
		throw std::runtime_error("Failed to reset command buffer!");
	}
}

void VulkanCommandBuffer::cmdBeginRenderPass(const VulkanRenderPassBeginInfo& beginInfo , VkSubpassContents contentsType){
    VkRenderPassBeginInfo renderPassInfo = {};
    memset(&renderPassInfo, 0, sizeof(VkRenderPassBeginInfo));
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    // Render pass object
    if (beginInfo.renderPass) {
        renderPassInfo.renderPass = beginInfo.renderPass->getPass();
        _usedObjects.insert(beginInfo.renderPass);
    }
    // Framebuffer object
    if (beginInfo.framebuffer) {
        renderPassInfo.framebuffer = beginInfo.framebuffer->getBuffer();
        _usedObjects.insert(beginInfo.framebuffer);
    }
    renderPassInfo.renderArea = beginInfo.renderArea;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(beginInfo.clearValues.size());
    renderPassInfo.pClearValues = beginInfo.clearValues.data();
    
    // Запуск рендер-прохода
    // VK_SUBPASS_CONTENTS_INLINE: Команды render pass будут включены в первичный буфер команд и вторичные буферы команд не будут задействованы.
    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: Команды render pass будут выполняться из вторичных буферов.
    vkCmdBeginRenderPass(_commandBuffer, &renderPassInfo, contentsType);
}

void VulkanCommandBuffer::cmdEndRenderPass(){
    vkCmdEndRenderPass(_commandBuffer);
}

void VulkanCommandBuffer::cmdSetViewport(const VkRect2D& inViewport){
    // Динамически изменяемый параметр в пайплайне
    VkViewport viewport = {};
    memset(&viewport, 0, sizeof(VkViewport));
    viewport.x = static_cast<float>(inViewport.offset.x);
    viewport.y = static_cast<float>(inViewport.offset.y);
    viewport.width = static_cast<float>(inViewport.extent.width);
    viewport.height = static_cast<float>(inViewport.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
}

void VulkanCommandBuffer::cmdSetScissor(const VkRect2D& scissor){
    vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
}

void VulkanCommandBuffer::cmdBindPipeline(const VulkanPipelinePtr& pipeline){
    _usedObjects.insert(pipeline);
    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());
}

void VulkanCommandBuffer::cmdBindVertexBuffer(const VulkanBufferPtr& buffer, VkDeviceSize offset){
    _usedObjects.insert(buffer);
    VkBuffer vertexBuffers[] = {buffer->getBuffer()};
    VkDeviceSize offsets[] = {offset};
    vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets);
}

void VulkanCommandBuffer::cmdBindVertexBuffers(const std::vector<VulkanBufferPtr>& buffers, const std::vector<VkDeviceSize>& offsets){
    _usedObjects.insert(buffers.begin(), buffers.end());
    
    std::vector<VkBuffer> vkBuffers;
    vkBuffers.reserve(buffers.size());
    for (const VulkanBufferPtr& buf: buffers) {
        vkBuffers.push_back(buf->getBuffer());
    }
    
    vkCmdBindVertexBuffers(_commandBuffer, 0, static_cast<uint32_t>(vkBuffers.size()), vkBuffers.data(), offsets.data());
}

void VulkanCommandBuffer::cmdBindIndexBuffer(const VulkanBufferPtr& buffer, VkIndexType type, VkDeviceSize offset){
    _usedObjects.insert(buffer);
    vkCmdBindIndexBuffer(_commandBuffer, buffer->getBuffer(), offset, type);
}

void VulkanCommandBuffer::cmdBindDescriptorSet(const VkPipelineLayout& pipelineLayout,
                                               const VulkanDescriptorSetPtr& set,
                                               uint32_t offset){
    _usedObjects.insert(set);
    
    VkDescriptorSet vkSet = set->getSet();
    if (offset == 0) {
        vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &vkSet, 0, nullptr);
    }else{
        uint32_t offsets[] = {offset};
        vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &vkSet, 1, offsets);
    }
}

void VulkanCommandBuffer::cmdBindDescriptorSets(const VkPipelineLayout& pipelineLayout,
                                                const std::vector<VulkanDescriptorSetPtr>& sets){
    _usedObjects.insert(sets.begin(), sets.end());
    
    std::vector<VkDescriptorSet> vkSets;
    vkSets.reserve(sets.size());
    for (const VulkanDescriptorSetPtr& set: sets) {
        vkSets.push_back(set->getSet());
    }
    
    vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
                            vkSets.size(), vkSets.data(),
                            0, nullptr);
}

void VulkanCommandBuffer::cmdBindDescriptorSets(const VkPipelineLayout& pipelineLayout,
                                                const std::vector<VulkanDescriptorSetPtr>& sets,
                                                const std::vector<uint32_t>& offsets){
    _usedObjects.insert(sets.begin(), sets.end());
    
    std::vector<VkDescriptorSet> vkSets;
    vkSets.reserve(sets.size());
    for (const VulkanDescriptorSetPtr& set: sets) {
        vkSets.push_back(set->getSet());
    }

    vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
                            vkSets.size(), vkSets.data(),
                            offsets.size(), offsets.data());
}

void VulkanCommandBuffer::cmdPushConstants(const VkPipelineLayout& pipelineLayout, VkShaderStageFlags stage, const void* data, uint32_t size, uint32_t offset){
    vkCmdPushConstants(_commandBuffer,
                       pipelineLayout,
                       stage,
                       offset,
                       size,
                       data);
}

void VulkanCommandBuffer::cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance){
     vkCmdDraw(_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance){
    vkCmdDrawIndexed(_commandBuffer, indexCount, instanceCount, 0, 0, 0);
}

void VulkanCommandBuffer::cmdCopyImage(const VulkanImagePtr& srcImage, const VulkanImagePtr& dstImage, VkImageAspectFlags aspectMask, uint32_t mipLevel){
    _usedObjects.insert(srcImage);
    _usedObjects.insert(dstImage);
    
    // Описание ресурса
    VkImageSubresourceLayers subResource = {};
    memset(&subResource, 0, sizeof(VkImageSubresourceLayers));
    subResource.aspectMask = aspectMask; // Текстура с цветом
    subResource.layerCount = 1; // Всего 1н слой
    subResource.baseArrayLayer = 0; // 0й слой
    subResource.mipLevel = mipLevel;   // Уровень мипмаппинга
    
    // Регион копирования текстуры
    VkImageCopy region = {};
    memset(&region, 0, sizeof(VkImageCopy));
    region.srcSubresource = subResource;
    region.dstSubresource = subResource;
    region.srcOffset = {0, 0, 0};
    region.dstOffset = {0, 0, 0};
    region.extent.width = srcImage->getBaseSize().width;
    region.extent.height = srcImage->getBaseSize().height;
    region.extent.depth = 1;
    
    // Создаем задачу на копирование данных
    vkCmdCopyImage(_commandBuffer,
                   srcImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dstImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &region);
}

void VulkanCommandBuffer::cmdBlitImage(const VkImageBlit& imageBlit, const VulkanImagePtr& srcImage, const VulkanImagePtr& dstImage){
    _usedObjects.insert(srcImage);
    _usedObjects.insert(dstImage);
    
    vkCmdBlitImage(_commandBuffer,
                   srcImage->getImage(),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dstImage->getImage(),
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1,
                   &imageBlit,
                   VK_FILTER_LINEAR);
}

void VulkanCommandBuffer::cmdCopyBuffer(const VkBufferCopy& copyRegion, const VulkanBufferPtr& srcBuffer, const VulkanBufferPtr& dstBuffer){
    _usedObjects.insert(srcBuffer);
    _usedObjects.insert(dstBuffer);
    
    // Ставим в очередь копирование буффера
    vkCmdCopyBuffer(_commandBuffer, srcBuffer->getBuffer(), dstBuffer->getBuffer(), 1, &copyRegion);
}

void VulkanCommandBuffer::cmdCopyAllBuffer(const VulkanBufferPtr& srcBuffer, const VulkanBufferPtr& dstBuffer){
    _usedObjects.insert(srcBuffer);
    _usedObjects.insert(dstBuffer);
    
    // Ставим в очередь копирование буффера
    VkBufferCopy copyRegion = {};
    memset(&copyRegion, 0, sizeof(VkBufferCopy));
    copyRegion.size = static_cast<VkDeviceSize>(srcBuffer->getBaseSize());
    
    // Ставим в очередь копирование буффера
    vkCmdCopyBuffer(_commandBuffer, srcBuffer->getBuffer(), dstBuffer->getBuffer(), 1, &copyRegion);
}


void VulkanCommandBuffer::cmdPipelineBarrier(VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage,
                                             VulkanImageBarrierInfo* imageInfo, uint32_t imageInfoCount,
                                             VulkanBufferBarrierInfo* bufferInfo, uint32_t bufferInfoCount,
                                             VulkanMemoryBarrierInfo* memoryInfo, uint32_t memoryInfoCount){
    
    std::vector<VkImageMemoryBarrier> imageBarriers(imageInfoCount);
    for (uint32_t i = 0; i < imageInfoCount; i++) {
        _usedObjects.insert(imageInfo[i].image);
        
        memset(&imageBarriers[i], 0, sizeof(VkImageMemoryBarrier));
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarriers[i].oldLayout = imageInfo[i].oldLayout;  // Старый лаяут (способ использования)
        imageBarriers[i].newLayout = imageInfo[i].newLayout;  // Новый лаяут (способ использования)
        imageBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // Очередь не меняется
        imageBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // Очередь не меняется
        imageBarriers[i].image = imageInfo[i].image->getImage();  // Изображение, которое меняется
        imageBarriers[i].srcAccessMask = imageInfo[i].srcAccessBarrier;
        imageBarriers[i].dstAccessMask = imageInfo[i].dstAccessBarrier;
        imageBarriers[i].subresourceRange.baseArrayLayer = 0;
        imageBarriers[i].subresourceRange.layerCount = 1;
        imageBarriers[i].subresourceRange.baseMipLevel = imageInfo[i].startMipmapLevel;
        imageBarriers[i].subresourceRange.levelCount = imageInfo[i].levelsCount;    // Сколько уровней надо конвертить?? Как параметр для мипмапов VK_REMAINING_MIP_LEVELS
        imageBarriers[i].subresourceRange.aspectMask = imageInfo[i].aspectFlags;
        
        imageInfo[i].image->setNewLayout(imageInfo[i].newLayout);
    }
    
    // Закидываем в очередь барьер конвертации использования для изображения
    vkCmdPipelineBarrier(_commandBuffer,
                         srcStage,
                         dstStage,
                         0,
                         0, nullptr,
                         0, nullptr,
                         imageBarriers.size(), (imageBarriers.size() > 0) ? imageBarriers.data() : nullptr);
}

