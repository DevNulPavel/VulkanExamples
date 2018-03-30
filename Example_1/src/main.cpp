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
    vkDestroyDescriptorPool(RenderI->vulkanLogicalDevice->getDevice(), vulkanDescriptorPool, nullptr);
    vkDestroyBuffer(RenderI->vulkanLogicalDevice->getDevice(), vulkanUniformStagingBuffer, nullptr);
    vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), vulkanUniformStagingBufferMemory, nullptr);
    vkDestroyBuffer(RenderI->vulkanLogicalDevice->getDevice(), vulkanUniformBuffer, nullptr);
    vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), vulkanUniformBufferMemory, nullptr);
    vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), vulkanIndexBufferMemory, nullptr);
    vkDestroyBuffer(RenderI->vulkanLogicalDevice->getDevice(), vulkanIndexBuffer, nullptr);
    vkFreeMemory(RenderI->vulkanLogicalDevice->getDevice(), vulkanVertexBufferMemory, nullptr);
    vkDestroyBuffer(RenderI->vulkanLogicalDevice->getDevice(), vulkanVertexBuffer, nullptr);

    
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

