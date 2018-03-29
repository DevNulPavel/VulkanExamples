// Windows
#ifdef _MSVC_LANG 
    #define NOMINMAX
    #include <windows.h>
#endif

#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <thread>
#include <chrono>
#include <algorithm>
#include <limits>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// STB image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// TinyObj
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "VulkanRender.h"
#include "CommonDefines.h"
#include "CommonConstants.h"
#include "Vertex.h"
#include "Figures.h"
#include "UniformBuffer.h"
#include "VulkanHelpers.h"


#define APPLICATION_SAMPLING_VALUE VK_SAMPLE_COUNT_1_BIT

struct FamiliesQueueIndexes;


GLFWwindow* window = nullptr;



VkFence vulkanFence = VK_NULL_HANDLE;


std::vector<VkFramebuffer> vulkanSwapChainFramebuffers;
VkCommandPool vulkanCommandPool = VK_NULL_HANDLE;
std::vector<VkCommandBuffer> vulkanCommandBuffers;
VkBuffer vulkanVertexBuffer = VK_NULL_HANDLE;
VkDeviceMemory vulkanVertexBufferMemory = VK_NULL_HANDLE;
VkBuffer vulkanIndexBuffer = VK_NULL_HANDLE;
VkDeviceMemory vulkanIndexBufferMemory = VK_NULL_HANDLE;
VkBuffer vulkanUniformStagingBuffer = VK_NULL_HANDLE;
VkDeviceMemory vulkanUniformStagingBufferMemory = VK_NULL_HANDLE;
VkBuffer vulkanUniformBuffer = VK_NULL_HANDLE;
VkDeviceMemory vulkanUniformBufferMemory = VK_NULL_HANDLE;
VkDescriptorPool vulkanDescriptorPool = VK_NULL_HANDLE;
VkDescriptorSet vulkanDescriptorSet = VK_NULL_HANDLE;
VkImage vulkanTextureImage = VK_NULL_HANDLE;
VkDeviceMemory vulkanTextureImageMemory = VK_NULL_HANDLE;
VkImageView vulkanTextureImageView = VK_NULL_HANDLE;
VkSampler vulkanTextureSampler = VK_NULL_HANDLE;
VkImage vulkanDepthImage = VK_NULL_HANDLE;
VkDeviceMemory vulkanDepthImageMemory = VK_NULL_HANDLE;
VkImageView vulkanDepthImageView = VK_NULL_HANDLE;
std::vector<Vertex> vulkanVertices;
std::vector<uint32_t> vulkanIndices;
size_t vulkanTotalVertexesCount = 0;
size_t vulkanTotalIndexesCount = 0;
uint32_t vulkanImageIndex = 0;


////////////////////////////////////////////////////////////////////////////////////////////////

float rotateAngle = 0.0f;



// Создаем преграды для проверки завершения комманд отрисовки
void createFences(){
    VkFenceCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkFenceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Изначально создаем вытавленным
    VkResult fenceCreateStatus = vkCreateFence(RenderI->vulkanLogicalDevice->getDevice(), &createInfo, nullptr, &vulkanFence);
    if (fenceCreateStatus != VK_SUCCESS) {
        printf("Failed to create fence!");
        fflush(stdout);
        throw std::runtime_error("Failed to create fence!");
    }
}


// Создаем пулл комманд
void createCommandPool() {
    int vulkanRenderQueueFamilyIndex = RenderI->vulkanPhysicalDevice->getQueuesFamiliesIndexes().renderQueueFamilyIndex;
    //int vulkanPresentQueueFamilyIndex = RenderI->vulkanQueuesFamiliesIndexes.presentQueueFamilyIndex;
    
    // Информация о пуле коммандных буфферов
    VkCommandPoolCreateInfo poolInfo = {};
    memset(&poolInfo, 0, sizeof(VkCommandPoolCreateInfo));
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = vulkanRenderQueueFamilyIndex;   // Пулл будет для семейства очередей рендеринга
    poolInfo.flags = 0; // Optional
    
    if (vkCreateCommandPool(RenderI->vulkanLogicalDevice->getDevice(), &poolInfo, nullptr, &vulkanCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

// Запуск коммандного буффера на получение комманд
VkCommandBuffer beginSingleTimeCommands() {
    // Параметр level определяет, будет ли выделенный буфер команд первичным или вторичным буфером команд:
    // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Может быть передан очереди для исполнения, но не может быть вызван из других буферов команд.
    // VK_COMMAND_BUFFER_LEVEL_SECONDARY: не может быть передан непосредственно, но может быть вызван из первичных буферов команд.
    VkCommandBufferAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkCommandBufferAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // Первичный буффер, которыый будет исполняться сразу
    allocInfo.commandPool = vulkanCommandPool;      // Пул комманд
    allocInfo.commandBufferCount = 1;
    
    // Аллоцируем коммандный буффер для задач, который будут закидываться в очередь
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(RenderI->vulkanLogicalDevice->getDevice(), &allocInfo, &commandBuffer);
    
    // Параметр flags определяет, как использовать буфер команд. Возможны следующие значения:
    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: Буфер команд будет перезаписан сразу после первого выполнения.
    // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: Это вторичный буфер команд, который будет в единственном render pass.
    // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: Буфер команд может быть представлен еще раз, если он так же уже находится в ожидании исполнения.
    
    // Настройки запуска коммандного буффера
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    // Запускаем буффер комманд
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    return commandBuffer;
}

// Завершение коммандного буффера
void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    // Заканчиваем прием комманд
    vkEndCommandBuffer(commandBuffer);
    
    // Структура с описанием отправки в буффер
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    // Отправляем задание на отрисовку в буффер отрисовки
    vkQueueSubmit(RenderI->vulkanRenderQueue->getQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    
    // TODO: Ожидание передачи комманды в очередь на GPU???
    // Как альтернативу - можно использовать Fence
    vkQueueWaitIdle(RenderI->vulkanRenderQueue->getQueue());
    /*std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();
    vkQueueWaitIdle(vulkanGraphicsQueue);
    std::chrono::high_resolution_clock::time_point time2 = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::duration elapsed = time2 - time1;
    int64_t elapsedMicroSec = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    printf("Wait duration (vkQueueWaitIdle(vulkanGraphicsQueue)): %lldmicroSec\n", elapsedMicroSec);
    fflush(stdout);*/
    
    // Удаляем коммандный буффер
    vkFreeCommandBuffers(RenderI->vulkanLogicalDevice->getDevice(), vulkanCommandPool, 1, &commandBuffer);
}

// Есть ли поддержка трафарета в формате
bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

// Перевод изображения из одного лаяута в другой (из одного способа использования в другой)
void transitionImageLayout(VkCommandBuffer commandBuffer,
                           VkImage image,
                           VkFormat format,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout,
                           VkImageSubresourceRange* mipSubRange,
                           VkPipelineStageFlagBits srcStage,
                           VkPipelineStageFlagBits dstStage,
                           VkAccessFlags srcAccessBarrier,
                           VkAccessFlags dstAccessBarrier) {
    // Создаем коммандный буффер
    //VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    // Создаем барьер памяти для картинок
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;  // Старый лаяут (способ использования)
    barrier.newLayout = newLayout;  // Новый лаяут (способ использования)
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // Очередь не меняется
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // Очередь не меняется
    barrier.image = image;  // Изображение, которое меняется
    
    if (mipSubRange != VK_NULL_HANDLE) {
        barrier.subresourceRange = *mipSubRange;
    }else{
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;    // Изображение использовалось для цвета
        barrier.subresourceRange.baseMipLevel = 0;  // 0 левел мипмапов
        barrier.subresourceRange.levelCount = 1;    // 1 уровень мипмапов
        barrier.subresourceRange.baseArrayLayer = 0;    // Базовый уровень
        barrier.subresourceRange.layerCount = 1;        // 1 уровень
    }
    
    // Настраиваем условия ожиданий для конвертации
    barrier.srcAccessMask = srcAccessBarrier;
    barrier.dstAccessMask = dstAccessBarrier;
    
    // TODO: ???
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        
        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    
    // Закидываем в очередь барьер конвертации использования для изображения
    vkCmdPipelineBarrier(commandBuffer,
                         srcStage, // Закидываем на верх пайплайна VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                         dstStage, // Закидываем на верх пайплайна VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
    
    //endSingleTimeCommands(commandBuffer);
}

// Закидываем в очередь операцию копирования текстуры
void copyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height) {
    // TODO: Надо ли для группы операций с текстурами каждый раз создавать коммандный буффер?? Может быть можно все делать в одном?
    // Создаем коммандный буффер
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    // Описание ресурса
    VkImageSubresourceLayers subResource = {};
    memset(&subResource, 0, sizeof(VkImageSubresourceLayers));
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Текстура с цветом
    subResource.layerCount = 1; // Всего 1н слой
    subResource.baseArrayLayer = 0; // 0й слой
    subResource.mipLevel = 0;   // 0й уровень мипмаппинга
    
    // Регион копирования текстуры
    VkImageCopy region = {};
    memset(&region, 0, sizeof(VkImageCopy));
    region.srcSubresource = subResource;
    region.dstSubresource = subResource;
    region.srcOffset = {0, 0, 0};
    region.dstOffset = {0, 0, 0};
    region.extent.width = width;
    region.extent.height = height;
    region.extent.depth = 1;
    
    // Создаем задачу на копирование данных
    vkCmdCopyImage(commandBuffer,
                   srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &region);
    
    // Завершаем буффер комманд
    endSingleTimeCommands(commandBuffer);
}

// Обновляем лаяут текстуры глубины на правильный
void updateDepthTextureLayout(){
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    // Конвертируем в формат, пригодный для глубины
    transitionImageLayout(commandBuffer,
                          vulkanDepthImage,
                          vulkanDepthFormat,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                          nullptr,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          0,
                          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
    endSingleTimeCommands(commandBuffer);
}

void generateMipmapsForImage(VkImage image, uint32_t miplevels, int32_t width, int32_t height){
    // Generate the mip chain
    // ---------------------------------------------------------------
    // We copy down the whole mip chain doing a blit from mip-1 to mip
    // An alternative way would be to always blit from the first mip level and sample that one down
    VkCommandBuffer blitCmd = beginSingleTimeCommands();

    // Copy down mips from n-1 to n
    for (int32_t i = 1; i < miplevels; i++){
        VkImageBlit imageBlit = {};

        // Source
        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel = i-1;
        imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
        imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
        imageBlit.srcOffsets[1].z = 1;

        // Destination
        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstOffsets[1].x = int32_t(width >> i);
        imageBlit.dstOffsets[1].y = int32_t(height >> i);
        imageBlit.dstOffsets[1].z = 1;

        VkImageSubresourceRange mipSubRange = {};
        mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        mipSubRange.baseMipLevel = i;
        mipSubRange.levelCount = 1;
        mipSubRange.layerCount = 1;

        // Transiton current mip level to transfer dest
        transitionImageLayout(blitCmd,
                              image,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              &mipSubRange,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_HOST_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT);
 
        // Blit from previous level
        vkCmdBlitImage(blitCmd,
                       image,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &imageBlit,
                       VK_FILTER_LINEAR);

        // Transiton current mip level to transfer source for read in next iteration
        transitionImageLayout(blitCmd,
                              image,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              &mipSubRange,
                              VK_PIPELINE_STAGE_HOST_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT);
    }

    // After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.layerCount = 1;
    subresourceRange.levelCount = miplevels;
    transitionImageLayout(blitCmd,
                          image,
                          VK_FORMAT_R8G8B8A8_UNORM,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          &subresourceRange,
                          VK_PIPELINE_STAGE_HOST_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_ACCESS_TRANSFER_WRITE_BIT,
                          VK_ACCESS_TRANSFER_READ_BIT);
    
    endSingleTimeCommands(blitCmd);
}

// Создание текстуры из изображения
void createTextureImage() {
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    stbi_uc* pixels = stbi_load("res/textures/chalet.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); // STBI_rgb_alpha STBI_default
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    
    // Указатели на временную текстуру и целевую
    VkImage stagingImage = VK_NULL_HANDLE;
    VkDeviceMemory stagingImageMemory = VK_NULL_HANDLE;
    
    // VK_IMAGE_TILING_LINEAR - специально, для исходного изображения
    // http://vulkanapi.ru/2016/12/17/vulkan-api-%D1%83%D1%80%D0%BE%D0%BA-45/
    createImage(texWidth, texHeight,
                VK_FORMAT_R8G8B8A8_UNORM,           // Формат текстуры
                VK_IMAGE_TILING_LINEAR,             // Тайлинг
                VK_IMAGE_LAYOUT_PREINITIALIZED,     // Чтобы данные не уничтожились при первом использовании - используем PREINITIALIZED (must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED)
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,    // Используется для передачи в другую текстуру данных
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // Настраиваем работу с памятью так, чтобы было доступно на CPU
                1,
                stagingImage,
                stagingImageMemory);
    
    // Описание подресурса для изображения
    VkImageSubresource subresource = {};
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.mipLevel = 0;
    subresource.arrayLayer = 0;
    
    // Создание лаяута для подресурса
    VkSubresourceLayout stagingImageLayout;
    vkGetImageSubresourceLayout(RenderI->vulkanLogicalDevice->getDevice(), stagingImage, &subresource, &stagingImageLayout);
    
    // Мапим данные текстуры в адресное пространство CPU
    void* data = nullptr;
    vkMapMemory(RenderI->vulkanLogicalDevice->getDevice(), stagingImageMemory, 0, imageSize, 0, &data);
    
    // Копируем целиком или построчно в зависимости от размера и выравнивания на GPU
    if (stagingImageLayout.rowPitch == texWidth * 4) {
        memcpy(data, pixels, (size_t) imageSize);
    } else {
        uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);
        
        for (int y = 0; y < texHeight; y++) {
            memcpy(&dataBytes[y * stagingImageLayout.rowPitch],
                   &pixels[y * texWidth * 4],
                   texWidth * 4);
        }
    }
    
    // Размапим данные
    vkUnmapMemory(RenderI->vulkanLogicalDevice->getDevice(), stagingImageMemory);
    
    uint32_t mipmapLevels = floor(log2(std::max(texWidth, texHeight))) + 1;
    
    // Создаем рабочее изображение для последующего использования
    createImage(texWidth, texHeight,
                VK_FORMAT_R8G8B8A8_UNORM,   // Формат картинки
                VK_IMAGE_TILING_OPTIMAL,    // тайлинг
                VK_IMAGE_LAYOUT_UNDEFINED,   // Лаяут использования (must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED)
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,   // Используется как получаетель + для отрисовки
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,    // Хранится только на GPU
                mipmapLevels,
                vulkanTextureImage,
                vulkanTextureImageMemory);
    
    // Конвертирование исходной буфферной текстуры с данными в формат копирования на GPU
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        transitionImageLayout(commandBuffer,
                              stagingImage,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_PREINITIALIZED,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              nullptr,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_ACCESS_HOST_WRITE_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT);
        endSingleTimeCommands(commandBuffer);
    }
    
    // Конвертирование конечной буфферной текстуры с данными в формат получателя
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        transitionImageLayout(commandBuffer,
                              vulkanTextureImage,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // Без мипмапов - VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                              nullptr,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT);
        endSingleTimeCommands(commandBuffer);
    }
    
    // Копируем данные в пределах GPU из временной текстуры в целевую
    copyImage(stagingImage, vulkanTextureImage, texWidth, texHeight);
    
    // Генерируем мипмапы для текстуры
    generateMipmapsForImage(vulkanTextureImage, mipmapLevels, texWidth, texHeight);
    
    // Конвертируем использование текстуры в оптимальное для рендеринга
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        transitionImageLayout(commandBuffer,
                              vulkanTextureImage,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              nullptr,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT,
                              VK_ACCESS_SHADER_READ_BIT);
        endSingleTimeCommands(commandBuffer);
    }
    
    // Удаляем временные объекты
    vkDestroyImage(RenderI->vulkanLogicalDevice->getDevice(), stagingImage, nullptr);
    vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), stagingImageMemory, nullptr);
    
    // Учищаем буффер данных картинки
    stbi_image_free(pixels);
}

// Создаем вью текстуры
void createTextureImageView() {
    createImageView(vulkanTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, vulkanTextureImageView);
}

// Создание семплера для картинки
void createTextureSampler() {
    // Описание семплирования для текстуры
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;   // Линейное
    samplerInfo.minFilter = VK_FILTER_LINEAR;   // Линейное
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;   // Ограничение по границе
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;   // Ограничение по границе
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;   // Ограничение по границе
    samplerInfo.anisotropyEnable = VK_FALSE;    // Анизотропная фильтрация
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    // Создаем семплер
    if (vkCreateSampler(RenderI->vulkanLogicalDevice->getDevice(), &samplerInfo, nullptr, &vulkanTextureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

// Грузим данные для модели
void loadModel(){
    printf("Model loading started\n");
    fflush(stdout);
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "res/models/chalet.obj")) {
        throw std::runtime_error(err);
    }
    
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };
            
            vertex.color = {1.0f, 1.0f, 1.0f};
            
            vulkanVertices.push_back(vertex);
            vulkanIndices.push_back(vulkanIndices.size());
        }
    }
    
    vulkanTotalVertexesCount = vulkanVertices.size();
    vulkanTotalIndexesCount = vulkanIndices.size();
    
    printf("Model loading complete: %ld vertexes, %ld triangles, %ld indexes\n", vulkanTotalVertexesCount, vulkanTotalVertexesCount/3, vulkanTotalIndexesCount);
    fflush(stdout);
}

// Создаем буффер нужного размера
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    // Удаляем старое, если есть
    if(bufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), bufferMemory, nullptr);
        bufferMemory = VK_NULL_HANDLE;
    }
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(RenderI->vulkanLogicalDevice->getDevice(), buffer, nullptr);
        buffer = VK_NULL_HANDLE;
    }
    
    // VK_SHARING_MODE_EXCLUSIVE: изображение принадлежит одному семейству в один момент времени и должно быть явно передано другому семейству. Данный вариант обеспечивает наилучшую производительность.
    // VK_SHARING_MODE_CONCURRENT: изображение может быть использовано несколькими семействами без явной передачи.
    
    // Описание формата буффера
    VkBufferCreateInfo bufferInfo = {};
    memset(&bufferInfo, 0, sizeof(VkBufferCreateInfo));
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;     // Размер буффера
    bufferInfo.usage = usage;   // Использование данного буффера
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // изображение принадлежит одному семейству в один момент времени и должно быть явно передано другому семейству. Данный вариант обеспечивает наилучшую производительность.
    
    // Непосредственно создание буффера
    if (vkCreateBuffer(RenderI->vulkanLogicalDevice->getDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }
    
    // Запрашиваем данные о необходимой памяти
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(RenderI->vulkanLogicalDevice->getDevice(), buffer, &memRequirements);
    
    // Настройки аллокации буффера
    uint32_t memoryTypeIndex = findMemoryType(RenderI->vulkanPhysicalDevice->getDevice(),
                                              memRequirements.memoryTypeBits,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkMemoryAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkMemoryAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    
    // Выделяем память для буффера
    if (vkAllocateMemory(RenderI->vulkanLogicalDevice->getDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }
    
    // Подцепляем память к буфферу
    // Последний параметр – смещение в области памяти. Т.к. эта память выделяется специально для буфера вершин, смещение просто 0.
    // Если же оно не будет равно нулю, то значение должно быть кратным memRequirements.alignment.
    vkBindBufferMemory(RenderI->vulkanLogicalDevice->getDevice(), buffer, bufferMemory, 0);
}

// Копирование буффера
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    // Запускаем буффер
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    // Ставим в очередь копирование буффера
    VkBufferCopy copyRegion = {};
    memset(&copyRegion, 0, sizeof(VkBufferCopy));
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    
    // Заканчиваем буффер
    endSingleTimeCommands(commandBuffer);
}

// Создание буфферов вершин
void createVertexBuffer(){
    // Размер буфферов
    VkDeviceSize bufferSize = sizeof(vulkanVertices[0]) * vulkanVertices.size();
    
    // Создание временного буффера для передачи данных
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // Буффер может быть использован как источник данных для копирования
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,    // Хранится в оперативке CPU
                 stagingBuffer, stagingBufferMemory);
    
    // Маппим видео-память в адресное пространство оперативной памяти
    void* data = nullptr;
    vkMapMemory(RenderI->vulkanLogicalDevice->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    
    // Копируем вершины в память
    memcpy(data, vulkanVertices.data(), (size_t)bufferSize);
    
    // Размапим
    vkUnmapMemory(RenderI->vulkanLogicalDevice->getDevice(), stagingBufferMemory);
    
    // Создаем рабочий буффер
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  // Буффер может принимать данные + для отрисовки используется
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,   // Хранится на видео-карте
                 vulkanVertexBuffer, vulkanVertexBufferMemory);
    
    // Ставим задачу на копирование буфферов
    copyBuffer(stagingBuffer, vulkanVertexBuffer, bufferSize);
    
    // Удаляем временный буффер, если есть
    if(stagingBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), stagingBufferMemory, nullptr);
        stagingBufferMemory = VK_NULL_HANDLE;
    }
    if (stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(RenderI->vulkanLogicalDevice->getDevice(), stagingBuffer, nullptr);
        stagingBuffer = VK_NULL_HANDLE;
    }
    
    // Чистим исходные данные
    vulkanVertices.clear();
}

// Создание буффера индексов
void createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(vulkanIndices[0]) * vulkanIndices.size();
    
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    
    // Создаем временный буффер
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  // Буффер используется как исходник для копирования
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // Видимый из CPU
                 stagingBuffer, stagingBufferMemory);
    
    // Маппим видео-память в адресное пространство оперативной памяти
    void* data = nullptr;
    vkMapMemory(RenderI->vulkanLogicalDevice->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    
    // Копируем данные
    memcpy(data, vulkanIndices.data(), (size_t)bufferSize);
    
    // Размапим память
    vkUnmapMemory(RenderI->vulkanLogicalDevice->getDevice(), stagingBufferMemory);
    
    // Создаем рабочий буффер
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, // Используется как получатель + индексный буффер
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,   // Хранится только на GPU
                 vulkanIndexBuffer, vulkanIndexBufferMemory);
    
    // Ставим задачу на копирование буфферов
    copyBuffer(stagingBuffer, vulkanIndexBuffer, bufferSize);
    
    // Удаляем временный буффер, если есть
    if(stagingBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), stagingBufferMemory, nullptr);
        stagingBufferMemory = VK_NULL_HANDLE;
    }
    if (stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(RenderI->vulkanLogicalDevice->getDevice(), stagingBuffer, nullptr);
        stagingBuffer = VK_NULL_HANDLE;
    }
    
    // Чистим исходные данные
    vulkanIndices.clear();
}

// Создаем буффер юниформов
void createUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    // Буффер для юниформов для CPU
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  // Используется как исходник для передачи на отрисовку
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,    // Доступно для изменения на GPU
                 vulkanUniformStagingBuffer, vulkanUniformStagingBufferMemory);
    // Буффер для юниформов на GPU
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, // Испольузется как получаетель + юниформ буффер
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,   // Хранится только на GPU
                 vulkanUniformBuffer, vulkanUniformBufferMemory);
}

// Создаем пул дескрипторов ресурсов
void createDescriptorPool() {
    // Структура с типами пулов
    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
    // Юниформ буффер
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    // Семплер для текстуры
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;
    
    // Создаем пул
    VkDescriptorPoolCreateInfo poolInfo = {};
    memset(&poolInfo, 0, sizeof(VkDescriptorPoolCreateInfo));
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;
    
    if (vkCreateDescriptorPool(RenderI->vulkanLogicalDevice->getDevice(), &poolInfo, nullptr, &vulkanDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

// Создаем набор дескрипторов ресурсов
void createDescriptorSet() {
    // Настройки аллокатора для дескрипторов ресурсов
    VkDescriptorSetLayout layouts[] = {vulkanDescriptorSetLayout};
    VkDescriptorSetAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkDescriptorSetAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vulkanDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;
    
    // Аллоцируем дескрипторы в пуле
    if (vkAllocateDescriptorSets(RenderI->vulkanLogicalDevice->getDevice(), &allocInfo, &vulkanDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }
    
    // Описание дескриптора юниформа
    VkDescriptorBufferInfo bufferInfo = {};
    memset(&bufferInfo, 0, sizeof(VkDescriptorBufferInfo));
    bufferInfo.buffer = vulkanUniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);
    
    // Описание дескриптора семплера
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = vulkanTextureImageView;
    imageInfo.sampler = vulkanTextureSampler;
    
    // НАстройки дескрипторов
    std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
    
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = vulkanDescriptorSet;   // Набор дескрипторов из пула
    descriptorWrites[0].dstBinding = 0;                 // Биндится на 0м значении в шейдере
    descriptorWrites[0].dstArrayElement = 0;            // 0 элемент
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Тип - юниформ буффер
    descriptorWrites[0].descriptorCount = 1;            // 1н дескриптор
    descriptorWrites[0].pBufferInfo = &bufferInfo;      // Описание буффера
    
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = vulkanDescriptorSet;   // Набор дескрипторов из пула
    descriptorWrites[1].dstBinding = 1;                 // Биндится на 1м значении в шейдере
    descriptorWrites[1].dstArrayElement = 0;            // 0 элемент
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // Тип - семплер
    descriptorWrites[1].descriptorCount = 1;            // 1н дескриптор
    descriptorWrites[1].pImageInfo = &imageInfo;        // Описание изображения
    
    // Обновляем описание дескрипторов на устройстве
    vkUpdateDescriptorSets(RenderI->vulkanLogicalDevice->getDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

// Создаем коммандные буфферы
void createCommandBuffers() {

    // Очистка старых буфферов комманд
    if (vulkanCommandBuffers.size() > 0) {
        vkFreeCommandBuffers(RenderI->vulkanLogicalDevice->getDevice(), vulkanCommandPool, vulkanCommandBuffers.size(), vulkanCommandBuffers.data());
        vulkanCommandBuffers.clear();
    }
    
    // Ресайзим массив
    vulkanCommandBuffers.resize(vulkanSwapChainFramebuffers.size());
    
    // Настройки создания коммандного буффера
    VkCommandBufferAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkCommandBufferAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vulkanCommandPool;  // Создаем в общем пуле комманд
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // Первичный буффер
    allocInfo.commandBufferCount = (uint32_t)vulkanCommandBuffers.size();
    
    // Аллоцируем память под коммандные буфферы
    if (vkAllocateCommandBuffers(RenderI->vulkanLogicalDevice->getDevice(), &allocInfo, vulkanCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
    
    for (size_t i = 0; i < vulkanCommandBuffers.size(); i++) {
        // Параметр flags определяет, как использовать буфер команд. Возможны следующие значения:
        // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: Буфер команд будет перезаписан сразу после первого выполнения.
        // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: Это вторичный буфер команд, который будет в единственном render pass.
        // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: Буфер команд может быть представлен еще раз, если он так же уже находится в ожидании исполнения.
        
        
        // Настройка наследования
        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        memset(&inheritanceInfo, 0, sizeof(VkCommandBufferInheritanceInfo));
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.pNext = nullptr;
        inheritanceInfo.renderPass = vulkanRenderPass;
        inheritanceInfo.subpass = 0;
        inheritanceInfo.framebuffer = vulkanSwapChainFramebuffers[i];
        inheritanceInfo.occlusionQueryEnable = VK_FALSE;
        inheritanceInfo.queryFlags = 0;
        inheritanceInfo.pipelineStatistics = 0;
        
        // Информация о запуске коммандного буффера
        VkCommandBufferBeginInfo beginInfo = {};
        memset(&beginInfo, 0, sizeof(VkCommandBufferBeginInfo));
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Буфер команд может быть представлен еще раз, если он так же уже находится в ожидании исполнения.
        beginInfo.pInheritanceInfo = &inheritanceInfo; // Optional
        
        // Запуск коммандного буффера
        vkBeginCommandBuffer(vulkanCommandBuffers[i], &beginInfo);
        
        // Информация о запуске рендер-прохода
        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        
        VkRenderPassBeginInfo renderPassInfo = {};
        memset(&renderPassInfo, 0, sizeof(VkRenderPassBeginInfo));
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vulkanRenderPass;   // Рендер проход
        renderPassInfo.framebuffer = vulkanSwapChainFramebuffers[i];    // Фреймбуффер смены кадров
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = RenderI->vulkanSwapchain->getSwapChainExtent();
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();
        
        // Запуск рендер-прохода
        // VK_SUBPASS_CONTENTS_INLINE: Команды render pass будут включены в первичный буфер команд и вторичные буферы команд не будут задействованы.
        // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: Команды render pass будут выполняться из вторичных буферов.
        vkCmdBeginRenderPass(vulkanCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        // Устанавливаем пайплайн у коммандного буффера
        vkCmdBindPipeline(vulkanCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline);
        
        // Привязываем вершинный буффер к пайлпайну
        VkBuffer vertexBuffers[] = {vulkanVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vulkanCommandBuffers[i], 0, 1, vertexBuffers, offsets);
        
        // Привязываем индексный буффер к пайплайну
        vkCmdBindIndexBuffer(vulkanCommandBuffers[i], vulkanIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        // Подключаем дескрипторы ресурсов для юниформ буффера
        vkCmdBindDescriptorSets(vulkanCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayout, 0, 1, &vulkanDescriptorSet, 0, nullptr);
        
        // Вызов отрисовки - 3 вершины, 1 инстанс, начинаем с 0 вершины и 0 инстанса
        // vkCmdDraw(vulkanCommandBuffers[i], QUAD_VERTEXES.size(), 1, 0, 0);
        // Вызов поиндексной отрисовки - индексы вершин, один инстанс
        vkCmdDrawIndexed(vulkanCommandBuffers[i], vulkanTotalIndexesCount, 1, 0, 0, 0);
        
        //////////////////////////////////////////////////////////////////////////////////////////////////
        
        /*// Начинаем вводить комманды для следующего подпрохода рендеринга
        vkCmdNextSubpass(vulkanCommandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
        
        // Устанавливаем пайплайн у коммандного буффера
        vkCmdBindPipeline(vulkanCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline);
        
        // Привязываем вершинный буффер к пайлпайну
        vkCmdBindVertexBuffers(vulkanCommandBuffers[i], 0, 1, vertexBuffers, offsets);
        
        // Привязываем индексный буффер к пайплайну
        vkCmdBindIndexBuffer(vulkanCommandBuffers[i], vulkanIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        // Подключаем дескрипторы ресурсов для юниформ буффера
        vkCmdBindDescriptorSets(vulkanCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayout, 0, 1, &vulkanDescriptorSet, 0, nullptr);
        
        // Вызов отрисовки - 3 вершины, 1 инстанс, начинаем с 0 вершины и 0 инстанса
        // vkCmdDraw(vulkanCommandBuffers[i], QUAD_VERTEXES.size(), 1, 0, 0);
        // Вызов поиндексной отрисовки - индексы вершин, один инстанс
        vkCmdDrawIndexed(vulkanCommandBuffers[i], vulkanTotalIndexesCount/2, 1, 0, 0, 0);*/
        
        //////////////////////////////////////////////////////////////////////////////////////////////////
        
        // Заканчиваем рендер проход
        vkCmdEndRenderPass(vulkanCommandBuffers[i]);
        
        /*VkImageMemoryBarrier imageMemoryBarrier = {};
        memset(&imageMemoryBarrier, 0, sizeof(VkImageMemoryBarrier));
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.pNext = nullptr;
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_MEMORY_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageMemoryBarrier.srcQueueFamilyIndex = 0;
        imageMemoryBarrier.dstQueueFamilyIndex = 0;
        imageMemoryBarrier.image = vulkanSwapChainImages[i];
        imageMemoryBarrier.subresourceRange = {VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        
        vkCmdPipelineBarrier(vulkanCommandBuffers[i],
                             VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &imageMemoryBarrier
                             );*/
        
        // Заканчиваем подготовку коммандного буффера
        if (vkEndCommandBuffer(vulkanCommandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

// Вызывается при различных ресайзах окна
void recreateSwapChain() {
    // Ждем завершения работы Vulkan
    vkQueueWaitIdle(RenderI->vulkanRenderQueue->getQueue());
    vkQueueWaitIdle(RenderI->vulkanPresentQueue->getQueue());
    vkDeviceWaitIdle(RenderI->vulkanLogicalDevice->getDevice());
    
    // Заново пересоздаем свопчейны, старые удалятся внутри
    //createSwapChain();
    //getSwapchainImages();
    //createImageViews();
    //createDepthResources();
    //updateDepthTextureLayout();
    //createRenderPass();
    //createFramebuffers();
    //createGraphicsPipeline();
    createCommandBuffers();
}

// Обновляем юниформ буффер
void updateUniformBuffer(float delta){
    rotateAngle += delta * 30.0f;
    
    UniformBufferObject ubo = {};
    memset(&ubo, 0, sizeof(UniformBufferObject));
    ubo.model = glm::rotate(glm::mat4(), glm::radians(rotateAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(0.0f, 3.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), RenderI->vulkanSwapchain->getSwapChainExtent().width / (float)RenderI->vulkanSwapchain->getSwapChainExtent().height, 0.1f, 10.0f);
    
    // GLM был разработан для OpenGL, где координата Y клип координат перевернута,
    // самым простым путем решения данного вопроса будет изменить знак оси Y в матрице проекции
    //ubo.proj[1][1] *= -1;
    
    void* data = nullptr;
    vkMapMemory(RenderI->vulkanLogicalDevice->getDevice(), vulkanUniformStagingBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(RenderI->vulkanLogicalDevice->getDevice(), vulkanUniformStagingBufferMemory);
    
    // Закидываем задачу на копирование буффера
    copyBuffer(vulkanUniformStagingBuffer, vulkanUniformBuffer, sizeof(ubo));
}

// Непосредственно отрисовка кадра
void drawFrame() {
    // Запрашиваем изображение для отображения из swapchain, время ожидания делаем максимальным
    uint32_t swapchainImageIndex = 0;    // Индекс картинки свопчейна
    VkResult result = vkAcquireNextImageKHR(RenderI->vulkanLogicalDevice->getDevice(), RenderI->vulkanSwapchain->getSwapchain(),
                                            std::numeric_limits<uint64_t>::max(),
                                            RenderI->vulkanImageAvailableSemaphore->getSemafore(), // Семафор ожидания доступной картинки
                                            VK_NULL_HANDLE,
                                            &swapchainImageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }
    
    // Проверяем, совпадает ли номер картинки и индекс картинки свопчейна
    if (vulkanImageIndex != swapchainImageIndex) {
        printf("Vulkan image index not equal to swapchain image index!\n");
        fflush(stdout);
    }
    
    // Настраиваем отправление в очередь комманд отрисовки
    // http://vulkanapi.ru/2016/11/14/vulkan-api-%D1%83%D1%80%D0%BE%D0%BA-29-%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D0%B8%D0%BD%D0%B3-%D0%B8-%D0%BF%D1%80%D0%B5%D0%B4%D1%81%D1%82%D0%B0%D0%B2%D0%BB%D0%B5%D0%BD%D0%B8%D0%B5-hello-wo/
    VkSemaphore waitSemaphores[] = {RenderI->vulkanImageAvailableSemaphore->getSemafore()}; // Семафор ожидания картинки для вывода туда графики
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};    // Ждать будем c помощью семафора возможности вывода в буфер цвета
    VkSemaphore signalSemaphores[] = {RenderI->vulkanRenderFinishedSemaphore->getSemafore()}; // Семафор оповещения о завершении рендеринга
    VkSubmitInfo submitInfo = {};
    memset(&submitInfo, 0, sizeof(VkSubmitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;    // Ожидаем доступное изображение, в которое можно было бы записывать пиксели
    submitInfo.pWaitDstStageMask = waitStages;      // Ждать будем возможности вывода в буфер цвета
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanCommandBuffers[vulkanImageIndex]; // Указываем коммандный буффер отрисовки
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    // Синхронизация с ожиданием на CPU завершения очереди выполнения комманд
    /*
    //VkResult fenceStatus = vkGetFenceStatus(vulkanLogicalDevice, vulkanFence);
    //VkResult resetFenceStatus = vkResetFences(vulkanLogicalDevice, 1, &vulkanFence);
    VkResult waitStatus = vkWaitForFences(vulkanLogicalDevice, 1, &vulkanFence, VK_TRUE, std::numeric_limits<uint64_t>::max()-1);
    if (waitStatus == VK_SUCCESS) {
        VkResult resetFenceStatus = vkResetFences(vulkanLogicalDevice, 1, &vulkanFence);
        //printf("Fence waited + reset!\n");
        //fflush(stdout);
    }*/
    
    // Кидаем в очередь задачу на отрисовку с указанным коммандным буффером
    if (vkQueueSubmit(RenderI->vulkanRenderQueue->getQueue(), 1, &submitInfo,  VK_NULL_HANDLE/*vulkanFence*/) != VK_SUCCESS) {
        printf("Failed to submit draw command buffer!\n");
        fflush(stdout);
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
    
    // Можно не получать индекс, а просто делать как в Metal, либо на всякий случай получить индекс на старте
    vulkanImageIndex = (vulkanImageIndex + 1) % RenderI->vulkanSwapchain->getImageViews().size();
    
    // Настраиваем задачу отображения полученного изображения
    VkSwapchainKHR swapChains[] = {RenderI->vulkanSwapchain->getSwapchain()};
    VkPresentInfoKHR presentInfo = {};
    memset(&presentInfo, 0, sizeof(VkPresentInfoKHR));
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // Ожидаем окончания подготовки кадра с помощью семафора
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &swapchainImageIndex;
    
    // Закидываем в очередь задачу отображения картинки
    VkResult presentResult = vkQueuePresentKHR(RenderI->vulkanPresentQueue->getQueue(), &presentInfo);
    
    // В случае проблем - пересоздаем свопчейн
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
        return;
    } else if (presentResult != VK_SUCCESS) {
        printf("failed to present swap chain image!\n");
        throw std::runtime_error("failed to present swap chain image!");
    }
}

// Коллбек, вызываемый при изменении размеров окна приложения
void onGLFWWindowResized(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) return;
    recreateSwapChain();
}

#ifndef _MSVC_LANG
int main(int argc, char** argv) {
#else
int local_main(int argc, char** argv) {
#endif
    glfwInit();
    
    // Говорим GLFW, что не нужно создавать GL контекст
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Окно без изменения размера
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    // Мультисемплинг
    //glfwWindowHint(GLFW_SAMPLES, 4);
    
    // Создаем окно
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowSizeCallback(window, onGLFWWindowResized);
    
    // Проверяем наличие поддержки Vulkan
    int vulkanSupportStatus = glfwVulkanSupported();
    if (vulkanSupportStatus != GLFW_TRUE){
        printf("Vulkan support not found, error 0x%08x\n", vulkanSupportStatus);
		fflush(stdout);
        throw std::runtime_error("Vulkan support not found!");
    }

    // Создаем рендер
    VulkanRender::initInstance(window);
    
    // Создаем преграды для проверки завершения комманд отрисовки
    createFences();
    
    
    
    // Создаем пулл комманд
    createCommandPool();
    
    // Обновляем лаяут текстуры глубины на правильный
    updateDepthTextureLayout();
    
    // Создание текстуры из изображения
    createTextureImage();
    
    // Создание вью для текстуры
    createTextureImageView();
    
    // Создаем семплер для картинки
    createTextureSampler();
    
    // Грузим нашу модель
    loadModel();
    
    // Создание буфферов вершин
    createVertexBuffer();
    
    // Создание индексного буффера
    createIndexBuffer();
    
    // Создаем буффер юниформов
    createUniformBuffer();
    
    // Создаем пул дескрипторов ресурсов
    createDescriptorPool();
    
    // Создаем набор дескрипторов
    createDescriptorSet();
    
    // Создаем коммандные буфферы
    createCommandBuffers();
    
    // Цикл обработки графики
    std::chrono::high_resolution_clock::time_point lastDrawTime = std::chrono::high_resolution_clock::now();
    double lastFrameDuration = 1.0/60.0;
    int totalFrames = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Обновляем юниформы
        updateUniformBuffer(lastFrameDuration);
        
        // Непосредственно отрисовка кадра
        drawFrame();
        
        // Стабилизация времени кадра
        std::chrono::high_resolution_clock::duration curFrameDuration = std::chrono::high_resolution_clock::now() - lastDrawTime;
        std::chrono::high_resolution_clock::duration sleepDuration = std::chrono::milliseconds(static_cast<int>(1.0/60.0 * 1000.0)) - curFrameDuration;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(sleepDuration).count() > 0) {
            std::this_thread::sleep_for(sleepDuration);
        }
        // Расчет времени кадра
        lastFrameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastDrawTime).count() / 1000.0;
        lastDrawTime = std::chrono::high_resolution_clock::now(); // TODO: Возможно - правильнее было бы перетащить в начало цикла
        
        // FPS
        totalFrames++;
        if (totalFrames > 60) {
            totalFrames = 0;
            char outText[64];
            sprintf(outText, "Possible FPS: %d, sleep duration: %lldms", static_cast<int>(1.0/lastFrameDuration), std::chrono::duration_cast<std::chrono::milliseconds>(sleepDuration).count());
            glfwSetWindowTitle(window, outText);
        }
   }
    
    // Ждем завершения работы Vulkan
    vkQueueWaitIdle(RenderI->vulkanRenderQueue->getQueue());
    vkQueueWaitIdle(RenderI->vulkanPresentQueue->getQueue());
    vkDeviceWaitIdle(RenderI->vulkanLogicalDevice->getDevice());
    
    // Очищаем Vulkan
    // Удаляем старые объекты
    vkFreeCommandBuffers(RenderI->vulkanLogicalDevice->getDevice(), vulkanCommandPool, vulkanCommandBuffers.size(), vulkanCommandBuffers.data());
    vkDestroyImage(RenderI->vulkanLogicalDevice->getDevice(), vulkanDepthImage, nullptr);
    vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), vulkanDepthImageMemory, nullptr);
    vkDestroyImageView(RenderI->vulkanLogicalDevice->getDevice(), vulkanDepthImageView, nullptr);
    vkDestroySampler(RenderI->vulkanLogicalDevice->getDevice(), vulkanTextureSampler, nullptr);
    vkDestroyImageView(RenderI->vulkanLogicalDevice->getDevice(), vulkanTextureImageView, nullptr);
    vkDestroyImage(RenderI->vulkanLogicalDevice->getDevice(), vulkanTextureImage, nullptr);
    vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), vulkanTextureImageMemory, nullptr);
    vkDestroyDescriptorPool(RenderI->vulkanLogicalDevice->getDevice(), vulkanDescriptorPool, nullptr);
    vkDestroyBuffer(RenderI->vulkanLogicalDevice->getDevice(), vulkanUniformStagingBuffer, nullptr);
    vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), vulkanUniformStagingBufferMemory, nullptr);
    vkDestroyBuffer(RenderI->vulkanLogicalDevice->getDevice(), vulkanUniformBuffer, nullptr);
    vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), vulkanUniformBufferMemory, nullptr);
    vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), vulkanIndexBufferMemory, nullptr);
    vkDestroyBuffer(RenderI->vulkanLogicalDevice->getDevice(), vulkanIndexBuffer, nullptr);
    vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), vulkanVertexBufferMemory, nullptr);
    vkDestroyBuffer(RenderI->vulkanLogicalDevice->getDevice(), vulkanVertexBuffer, nullptr);
    vkDestroyCommandPool(RenderI->vulkanLogicalDevice->getDevice(), vulkanCommandPool, nullptr);

    
    vkDestroyFence(RenderI->vulkanLogicalDevice->getDevice(), vulkanFence, nullptr);
        
    VulkanRender::destroyRender();
    
    // Очищаем GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}

#ifdef _MSVC_LANG
INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
	local_main(0, NULL);
	return 0;
}
#endif

