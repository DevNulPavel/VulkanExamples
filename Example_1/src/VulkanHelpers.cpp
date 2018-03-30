#include "VulkanHelpers.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <cmath>

// STB image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


// Подбираем тип памяти буффера вершин
uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    // Запрашиваем типы памяти физического устройства
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
    
    // Найдем тип памяти, который подходит для самого буфера
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    printf("Failed to find suitable memory type!\n");
    fflush(stdout);
    throw std::runtime_error("Failed to find suitable memory type!");
}


// Подбираем формат текстуры в зависимости от доступных на устройстве
VkFormat findSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        // Запрашиваем информацию для формата
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    
    printf("Failed to find supported format!\n");
    fflush(stdout);
    throw std::runtime_error("Failed to find supported format!");
}

// Есть ли поддержка трафарета в формате
bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

// Перевод изображения из одного лаяута в другой (из одного способа использования в другой)
void transitionImageLayout(VulkanCommandBufferPtr commandBuffer,
                           VulkanImagePtr image,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout,
                           uint32_t startMipmapLevel,
                           uint32_t levelsCount,
                           VkImageAspectFlags aspectFlags,
                           VkPipelineStageFlagBits srcStage,
                           VkPipelineStageFlagBits dstStage,
                           VkAccessFlags srcAccessBarrier,
                           VkAccessFlags dstAccessBarrier) {
    
    VkImageSubresourceRange range;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    range.baseMipLevel = startMipmapLevel;
    range.levelCount = levelsCount;    // TODO: Сколько уровней надо конвертить?? Как параметр для мипмапов
    range.aspectMask = aspectFlags;
    
    // Создаем барьер памяти для картинок
    VkImageMemoryBarrier barrier = {};
    memset(&barrier, 0, sizeof(VkImageMemoryBarrier));
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;  // Старый лаяут (способ использования)
    barrier.newLayout = newLayout;  // Новый лаяут (способ использования)
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // Очередь не меняется
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // Очередь не меняется
    barrier.image = image->getImage();  // Изображение, которое меняется
    barrier.srcAccessMask = srcAccessBarrier;
    barrier.dstAccessMask = dstAccessBarrier;
    barrier.subresourceRange = range;
    
    // Закидываем в очередь барьер конвертации использования для изображения
    vkCmdPipelineBarrier(commandBuffer->getBuffer(),
                         srcStage, // Закидываем на верх пайплайна VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                         dstStage, // Закидываем на верх пайплайна VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
    
    image->setNewLayout(newLayout);
}

// Закидываем в очередь операцию копирования текстуры
void copyImage(VulkanCommandBufferPtr commandBuffer, VulkanImagePtr srcImage, VulkanImagePtr dstImage, VkImageAspectFlags aspectMask, uint32_t mipLevel) {
    // TODO: Надо ли для группы операций с текстурами каждый раз создавать коммандный буффер?? Может быть можно все делать в одном?
    
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
    vkCmdCopyImage(commandBuffer->getBuffer(),
                   srcImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dstImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &region);
}

// Создаем мипмапы для картинок
void generateMipmapsForImage(VulkanCommandBufferPtr commandBuffer, VulkanImagePtr image){
    // Generate the mip chain
    // ---------------------------------------------------------------
    // We copy down the whole mip chain doing a blit from mip-1 to mip
    // An alternative way would be to always blit from the first mip level and sample that one down
    
    // Copy down mips from n-1 to n
    for (int32_t i = 1; i < image->getBaseMipmapsCount(); i++){
        // Transiton current mip level to transfer dest
        transitionImageLayout(commandBuffer,
                              image,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_ASPECT_COLOR_BIT,
                              i, 1,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_HOST_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT);
        
        VkImageBlit imageBlit = {};
        memset(&imageBlit, 0, sizeof(VkImageBlit));
        // Source
        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel = i-1;
        imageBlit.srcOffsets[1].x = int32_t(image->getBaseSize().width >> (i - 1));
        imageBlit.srcOffsets[1].y = int32_t(image->getBaseSize().height >> (i - 1));
        imageBlit.srcOffsets[1].z = 1;
        
        // Destination
        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstOffsets[1].x = int32_t(image->getBaseSize().width >> i);
        imageBlit.dstOffsets[1].y = int32_t(image->getBaseSize().height >> i);
        imageBlit.dstOffsets[1].z = 1;
        
        // Blit from previous level
        vkCmdBlitImage(commandBuffer->getBuffer(),
                       image->getImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image->getImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &imageBlit,
                       VK_FILTER_LINEAR);
        
        // Transiton current mip level to transfer source for read in next iteration
        transitionImageLayout(commandBuffer,
                              image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              VK_IMAGE_ASPECT_COLOR_BIT,
                              i, 1,
                              VK_PIPELINE_STAGE_HOST_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT);
    }
    
    // TODO: Fix it
    // After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
    transitionImageLayout(commandBuffer,
                          image,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          0, image->getBaseMipmapsCount(),
                          VK_PIPELINE_STAGE_HOST_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_ACCESS_TRANSFER_WRITE_BIT,
                          VK_ACCESS_TRANSFER_READ_BIT);
}

// Запуск коммандного буффера на получение комманд
VulkanCommandBufferPtr beginSingleTimeCommands(VulkanLogicalDevicePtr device, VulkanCommandPoolPtr pool) {
    VulkanCommandBufferPtr buffer = std::make_shared<VulkanCommandBuffer>(device, pool);
    buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    return buffer;
}

// Завершение коммандного буффера + отправка в очередь
void endAndQueueSingleTimeCommands(VulkanCommandBufferPtr commandBuffer, VulkanQueuePtr queue) {
    commandBuffer->end();
    queue->submitBuffer(commandBuffer);
    queue->wait();
}

// Создание текстуры из изображения на диске
VulkanImagePtr createTextureImage(VulkanLogicalDevicePtr device, VulkanQueuePtr queue, VulkanCommandPoolPtr pool, const std::string& path) {
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); // STBI_rgb_alpha STBI_default
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    
    if (!pixels) {
        printf("Failed to load texture image %s!", path.c_str());
        fflush(stdout);
        throw std::runtime_error("Failed to load texture image!");
    }
    
    // VK_IMAGE_TILING_LINEAR - специально, для исходного изображения
    // http://vulkanapi.ru/2016/12/17/vulkan-api-%D1%83%D1%80%D0%BE%D0%BA-45/
    VulkanImagePtr staggingImage = std::make_shared<VulkanImage>(device,
                                                                 texWidth, texHeight,
                                                                 VK_FORMAT_R8G8B8A8_UNORM,           // Формат текстуры
                                                                 VK_IMAGE_TILING_LINEAR,             // Тайлинг
                                                                 VK_IMAGE_LAYOUT_PREINITIALIZED,     // Чтобы данные не уничтожились при первом использовании - используем PREINITIALIZED (must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED)
                                                                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT,    // Используется для передачи в другую текстуру данных // TODO: For test
                                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // Настраиваем работу с памятью так, чтобы было доступно на CPU
                                                                 1);
    
    // Отгружаем данные во временную текстуру
    staggingImage->uploadDataToImage(VK_IMAGE_ASPECT_COLOR_BIT, 0, static_cast<unsigned char*>(pixels), imageSize);
    
    uint32_t mipmapLevels = floor(log2(std::max(texWidth, texHeight))) + 1;
    
    // Создаем рабочее изображение для последующего использования
    VulkanImagePtr resultImage = std::make_shared<VulkanImage>(device,
                                                               texWidth, texHeight,
                                                               VK_FORMAT_R8G8B8A8_UNORM,      // Формат текстуры
                                                               VK_IMAGE_TILING_OPTIMAL,       // Тайлинг
                                                               VK_IMAGE_LAYOUT_UNDEFINED,       // Лаяут использования (must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED)
                                                               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,   // Используется как получаетель + для отрисовки
                                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,    // Хранится только на GPU
                                                               mipmapLevels);
    
    // Конвертирование исходной буфферной текстуры с данными в формат копирования на GPU
    {
        VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
        transitionImageLayout(commandBuffer,
                              staggingImage,
                              VK_IMAGE_LAYOUT_PREINITIALIZED,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              0, resultImage->getBaseMipmapsCount(),
                              VK_IMAGE_ASPECT_COLOR_BIT,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_ACCESS_HOST_WRITE_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT);
        endAndQueueSingleTimeCommands(commandBuffer, queue);
    }
    
    // Конвертирование конечной буфферной текстуры в формат получателя
    {
        VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
        transitionImageLayout(commandBuffer,
                              resultImage,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // Без мипмапов - VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, C - VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                              0, resultImage->getBaseMipmapsCount(),
                              VK_IMAGE_ASPECT_COLOR_BIT,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT);
        endAndQueueSingleTimeCommands(commandBuffer, queue);
    }
    
    // Копируем данные в пределах GPU из временной текстуры в целевую
    {
        VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
        copyImage(commandBuffer, staggingImage, resultImage, VK_IMAGE_ASPECT_COLOR_BIT, 0);
        endAndQueueSingleTimeCommands(commandBuffer, queue);
    }
    
    // Генерируем мипмапы для текстуры
    {
        VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
        generateMipmapsForImage(commandBuffer, resultImage);
        endAndQueueSingleTimeCommands(commandBuffer, queue);
    }
    
    // Конвертируем использование текстуры в оптимальное для рендеринга
    // Генерация мипмапов делает это самостоятельно
    /*{
        VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
        transitionImageLayout(commandBuffer,
                              resultImage,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // Без мипмапов - VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, C - VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                              0, resultImage->getBaseMipmapsCount(),
                              VK_IMAGE_ASPECT_COLOR_BIT,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT,
                              VK_ACCESS_SHADER_READ_BIT);
         endAndQueueSingleTimeCommands(commandBuffer, queue);
     }*/
    
    // Удаляем временные объекты
    staggingImage = nullptr;
    
    // Учищаем буффер данных картинки
    // TODO: Наверное можно удалить раньше
    stbi_image_free(pixels);
    
    return resultImage;
}

// Копирование буффера
void copyBuffer(VulkanCommandBufferPtr commandBuffer, VulkanBufferPtr srcBuffer, VulkanBufferPtr dstBuffer) {
    // Ставим в очередь копирование буффера
    VkBufferCopy copyRegion = {};
    memset(&copyRegion, 0, sizeof(VkBufferCopy));
    copyRegion.size = static_cast<VkDeviceSize>(srcBuffer->getBaseSize());
    vkCmdCopyBuffer(commandBuffer->getBuffer(), srcBuffer->getBuffer(), dstBuffer->getBuffer(), 1, &copyRegion);
}


// Создание буфферов
VulkanBufferPtr createBufferForData(VulkanLogicalDevicePtr device, VulkanQueuePtr queue, VulkanCommandPoolPtr pool, unsigned char* data, size_t bufferSize){
    
    // Создание временного буффера для передачи данных
    VulkanBufferPtr staggingBuffer = std::make_shared<VulkanBuffer>(device,
                                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,    // Хранится в оперативке CPU
                                                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // Буффер может быть использован как источник данных для копирования
                                                                    bufferSize);
    // Отгружаем данные
    staggingBuffer->uploadDataToBuffer(data, bufferSize);
    
    // Создаем рабочий буффер
    VulkanBufferPtr resultBuffer = std::make_shared<VulkanBuffer>(device,
                                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,   // Хранится на видео-карте
                                                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  // Буффер может принимать данные + для отрисовки используется
                                                                  bufferSize);
    
    // Ставим задачу на копирование буфферов
    VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
    copyBuffer(commandBuffer, staggingBuffer, resultBuffer);
    endAndQueueSingleTimeCommands(commandBuffer, queue);
    
    // Удаляем временный буффер, если есть
    staggingBuffer = nullptr;
    
    return resultBuffer;
}

