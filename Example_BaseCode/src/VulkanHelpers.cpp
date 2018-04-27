#include "VulkanHelpers.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <cmath>
#include <algorithm>

// STB image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Helpers.h"


// Подбираем тип памяти буффера вершин
uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    // Запрашиваем типы памяти физического устройства
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
    
    // Найдем тип памяти, который подходит для самого буфера
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        uint32_t testFilter = (1 << i);
        uint32_t testProperties = memProperties.memoryTypes[i].propertyFlags;
        if ((typeFilter & testFilter) && ((testProperties & properties) == properties)) {
            return i;
        }
    }
    
    LOG("Failed to find suitable memory type!\n");
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
    
    LOG("Failed to find supported format!\n");
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
    
    VulkanImageBarrierInfo info;
    info.image = image;
    info.oldLayout = oldLayout;
    info.newLayout = newLayout;
    info.startMipmapLevel = startMipmapLevel;
    info.levelsCount = levelsCount;
    info.aspectFlags = aspectFlags;
    info.srcAccessBarrier = srcAccessBarrier;
    info.dstAccessBarrier = dstAccessBarrier;
    
    commandBuffer->cmdPipelineBarrier(srcStage, dstStage,
                                      &info, 1,
                                      nullptr, 0,
                                      nullptr, 0);
    
    image->setNewLayout(newLayout);
}

// Создаем мипмапы для картинок
void generateMipmapsForImage(VulkanCommandBufferPtr commandBuffer, VulkanImagePtr image){
    // Generate the mip chain
    // ---------------------------------------------------------------
    // We copy down the whole mip chain doing a blit from mip-1 to mip
    // An alternative way would be to always blit from the first mip level and sample that one down
    
	transitionImageLayout(commandBuffer,
                          image,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          0, 1,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_ACCESS_TRANSFER_READ_BIT,
                          VK_ACCESS_TRANSFER_WRITE_BIT);

    // Copy down mips from n-1 to n
    for (int32_t i = 1; i < static_cast<int32_t>(image->getBaseMipmapsCount()); i++){
        // Transiton current mip level to transfer dest
        transitionImageLayout(commandBuffer,
                              image,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              i, 1,
							  VK_IMAGE_ASPECT_COLOR_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
							  VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT);
        
        VkImageBlit imageBlit = {};
        memset(&imageBlit, 0, sizeof(VkImageBlit));
        // Source
        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel = i-1;
        imageBlit.srcOffsets[0].x = 0;
        imageBlit.srcOffsets[0].y = 0;
        imageBlit.srcOffsets[0].z = 0;
        imageBlit.srcOffsets[1].x = int32_t(image->getBaseSize().width >> (i - 1));
        imageBlit.srcOffsets[1].y = int32_t(image->getBaseSize().height >> (i - 1));
        imageBlit.srcOffsets[1].z = 1;
        
        // Destination
        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstOffsets[0].x = 0;
        imageBlit.dstOffsets[0].y = 0;
        imageBlit.dstOffsets[0].z = 0;
        imageBlit.dstOffsets[1].x = int32_t(image->getBaseSize().width >> i);
        imageBlit.dstOffsets[1].y = int32_t(image->getBaseSize().height >> i);
        imageBlit.dstOffsets[1].z = 1;
        
        // Делаем blit
        commandBuffer->cmdBlitImage(imageBlit, image, image);
        
        // Transiton current mip level to transfer source for read in next iteration
        transitionImageLayout(commandBuffer,
                              image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              i, 1,
							  VK_IMAGE_ASPECT_COLOR_BIT,
			                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT);
    }
    
    // After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
    transitionImageLayout(commandBuffer,
                          image,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          0, image->getBaseMipmapsCount(),
					      VK_IMAGE_ASPECT_COLOR_BIT,
		                  VK_PIPELINE_STAGE_TRANSFER_BIT,
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

// Завершение коммандного буффера + отправка в очередь c ОЖИДАНИЕМ завершения
void endAndQueueWaitSingleTimeCommands(VulkanCommandBufferPtr commandBuffer, VulkanQueuePtr queue) {
    commandBuffer->end();
    queue->submitBuffer(commandBuffer);
    queue->wait();
}

// Завершение коммандного буффера + отправка в очередь
void endSingleTimeCommands(VulkanCommandBufferPtr commandBuffer, VulkanQueuePtr queue) {
    commandBuffer->end();
    queue->submitBuffer(commandBuffer);
}

// Создание текстуры из изображения на диске
VulkanImagePtr createTextureImage(VulkanLogicalDevicePtr device, VulkanQueuePtr queue, VulkanCommandPoolPtr pool, const std::string& path) {
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); // STBI_rgb_alpha STBI_default
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    
    if (!pixels) {
        LOG("Failed to load texture image %s!", path.c_str());
        throw std::runtime_error("Failed to load texture image!");
    }
    
    VkFormat imagesFormat = VK_FORMAT_R8G8B8A8_UNORM;
    
    // VK_IMAGE_TILING_LINEAR - специально, для исходного изображения
    // http://vulkanapi.ru/2016/12/17/vulkan-api-%D1%83%D1%80%D0%BE%D0%BA-45/
    VulkanImagePtr staggingImage = std::make_shared<VulkanImage>(device,
                                                                 VkExtent2D{static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)},
                                                                 imagesFormat,           // Формат текстуры
                                                                 VK_IMAGE_TILING_LINEAR,             // Тайлинг
                                                                 VK_IMAGE_LAYOUT_PREINITIALIZED,     // Чтобы данные не уничтожились при первом использовании - используем PREINITIALIZED (must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED)
                                                                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT,    // Используется для передачи в другую текстуру данных
                                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // Настраиваем работу с памятью так, чтобы было доступно на CPU
                                                                 1);
    
    // Отгружаем данные во временную текстуру
    staggingImage->uploadDataToImage(VK_IMAGE_ASPECT_COLOR_BIT, 0, static_cast<unsigned char*>(pixels), imageSize);
    
    // Учищаем буффер данных картинки
    stbi_image_free(pixels);
    pixels = nullptr;
    
    // Получим информацию об формате будущей картинки
    VkImageFormatProperties properties = {};
    vkGetPhysicalDeviceImageFormatProperties(device->getBasePhysicalDevice()->getDevice(),
                                             imagesFormat,
                                             VK_IMAGE_TYPE_2D,
                                             VK_IMAGE_TILING_OPTIMAL,
                                             VK_IMAGE_USAGE_SAMPLED_BIT,
                                             0,
                                             &properties);
    
    // Определяем уровни мипмапов
    uint32_t mipmapLevels = std::min((uint32_t)floor(log2(std::max(texWidth, texHeight))) + (uint32_t)1, properties.maxMipLevels);
    
    // Создаем рабочее изображение для последующего использования
    VulkanImagePtr resultImage = std::make_shared<VulkanImage>(device,
                                                               VkExtent2D{static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)},
                                                               imagesFormat,      // Формат текстуры
                                                               VK_IMAGE_TILING_OPTIMAL,       // Тайлинг
                                                               VK_IMAGE_LAYOUT_UNDEFINED,       // Лаяут использования (must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED)
															                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,   // Используется как получаетель + для отрисовки
                                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,    // Хранится только на GPU
                                                               mipmapLevels);
    
    // Надо ли для группы операций с текстурами каждый раз создавать коммандный буффер?? Может быть можно все делать в одном?
    VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
    
    // Конвертирование исходной буфферной текстуры с данными в формат копирования на GPU
    {
        //VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
        transitionImageLayout(commandBuffer,
                              staggingImage,
                              VK_IMAGE_LAYOUT_PREINITIALIZED,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              0, staggingImage->getBaseMipmapsCount(),
                              VK_IMAGE_ASPECT_COLOR_BIT,
							  VK_PIPELINE_STAGE_HOST_BIT,
							  VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_ACCESS_HOST_WRITE_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT);
        //endAndQueueSingleTimeCommands(commandBuffer, queue);
    }
    
    // Конвертирование конечной буфферной текстуры в формат получателя
    {
        //VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
        transitionImageLayout(commandBuffer,
                              resultImage,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              0, resultImage->getBaseMipmapsCount(),
                              VK_IMAGE_ASPECT_COLOR_BIT,
							  VK_PIPELINE_STAGE_TRANSFER_BIT,
							  VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT);
        //endAndQueueSingleTimeCommands(commandBuffer, queue);
    }
    
    // Копируем данные в пределах GPU из временной текстуры в целевую
    {
        //VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
        commandBuffer->cmdCopyImage(staggingImage, resultImage, VK_IMAGE_ASPECT_COLOR_BIT, 0);
        //endAndQueueSingleTimeCommands(commandBuffer, queue);
    }
    
    // Генерируем мипмапы для текстуры
    if (mipmapLevels > 1){
        //VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
        generateMipmapsForImage(commandBuffer, resultImage);
        //endAndQueueSingleTimeCommands(commandBuffer, queue);
  	} else {
  		// Конвертируем использование текстуры в оптимальное для рендеринга
  		// Генерация мипмапов делает это самостоятельно
  		//VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
  		transitionImageLayout(commandBuffer,
                                resultImage,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                0, resultImage->getBaseMipmapsCount(),
                                VK_IMAGE_ASPECT_COLOR_BIT,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_ACCESS_SHADER_READ_BIT);
  		//endAndQueueSingleTimeCommands(commandBuffer, queue);
  	}

    endAndQueueWaitSingleTimeCommands(commandBuffer, queue);
    
    // Удаляем временные объекты
    staggingImage = nullptr;
    
    
    return resultImage;
}

// Создание буфферов
VulkanBufferPtr createBufferForData(VulkanLogicalDevicePtr device, VulkanQueuePtr queue, VulkanCommandPoolPtr pool, VkBufferUsageFlagBits usage, unsigned char* data, size_t bufferSize){
    
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
                                                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,  // Буффер может принимать данные + для отрисовки используется
                                                                  bufferSize);
    
    // Ставим задачу на копирование буфферов
    VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(device, pool);
    commandBuffer->cmdCopyAllBuffer(staggingBuffer, resultBuffer);
    endAndQueueWaitSingleTimeCommands(commandBuffer, queue);
    
    // Удаляем временный буффер, если есть
    staggingBuffer = nullptr;
    
    return resultBuffer;
}

