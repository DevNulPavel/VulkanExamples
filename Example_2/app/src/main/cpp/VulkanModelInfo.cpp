#include "VulkanModelInfo.h"
#include <cstring>
#include <stdexcept>
#include <istream>
#include <streambuf>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "VulkanDevice.h"
#include "VulkanVisualizer.h"
#include "VulkanRenderInfo.h"
#include "SupportFunctions.h"


VulkanModelInfo::VulkanModelInfo(VulkanDevice* device, VulkanVisualizer* visualizer, VulkanRenderInfo* renderInfo, AAssetManager* assetManager):
    vulkanDevice(device),
    vulkanVisualizer(visualizer),
    vulkanRenderInfo(renderInfo),
    androidAssetManager(assetManager),
    vulkanTextureImage(VK_NULL_HANDLE),
    vulkanTextureImageMemory(VK_NULL_HANDLE),
    vulkanTextureImageView(VK_NULL_HANDLE),
    vulkanTextureSampler(VK_NULL_HANDLE),
    vulkanVertexBuffer(VK_NULL_HANDLE),
    vulkanVertexBufferMemory(VK_NULL_HANDLE),
    vulkanIndexBuffer(VK_NULL_HANDLE),
    vulkanIndexBufferMemory(VK_NULL_HANDLE){

    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
}

VulkanModelInfo::~VulkanModelInfo(){
    vkFreeMemory(vulkanDevice->vulkanLogicalDevice, vulkanIndexBufferMemory, nullptr);
    vkDestroyBuffer(vulkanDevice->vulkanLogicalDevice, vulkanIndexBuffer, nullptr);
    vkFreeMemory(vulkanDevice->vulkanLogicalDevice, vulkanVertexBufferMemory, nullptr);
    vkDestroyBuffer(vulkanDevice->vulkanLogicalDevice, vulkanVertexBuffer, nullptr);
    vkDestroySampler(vulkanDevice->vulkanLogicalDevice, vulkanTextureSampler, nullptr);
    vkDestroyImageView(vulkanDevice->vulkanLogicalDevice, vulkanTextureImageView, nullptr);
    vkDestroyImage(vulkanDevice->vulkanLogicalDevice, vulkanTextureImage, nullptr);
    vkFreeMemory(vulkanDevice->vulkanLogicalDevice, vulkanTextureImageMemory, nullptr);
}

// Создание текстуры из изображения
void VulkanModelInfo::createTextureImage() {
    std::vector<unsigned char> imageData = readFile(androidAssetManager, "textures/chalet.jpg");

    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    stbi_uc* pixels = stbi_load_from_memory((stbi_uc*)imageData.data(), static_cast<int>(imageData.size()), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); // STBI_rgb_alpha STBI_default
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth * texHeight * 4);

    imageData.clear();

    if (!pixels) {
        LOGE("Failed to load texture image!");
        throw std::runtime_error("Failed to load texture image!");
    }

    // Указатели на временную текстуру и целевую
    VkImage stagingImage = VK_NULL_HANDLE;
    VkDeviceMemory stagingImageMemory = VK_NULL_HANDLE;

    // VK_IMAGE_TILING_LINEAR - специально, для исходного изображения
    // http://vulkanapi.ru/2016/12/17/vulkan-api-%D1%83%D1%80%D0%BE%D0%BA-45/
    createImage(vulkanDevice->vulkanLogicalDevice, vulkanDevice->vulkanPhysicalDevice,
                static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight),
                VK_FORMAT_R8G8B8A8_UNORM,           // Формат текстуры
                VK_IMAGE_TILING_LINEAR,             // Тайлинг
                VK_IMAGE_LAYOUT_PREINITIALIZED,     // Чтобы данные не уничтожились при первом использовании - используем PREINITIALIZED (must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED)
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,    // Используется для передачи в другую текстуру данных
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // Настраиваем работу с памятью так, чтобы было доступно на CPU
                stagingImage,
                stagingImageMemory);

    // Описание подресурса для изображения
    VkImageSubresource subresource = {};
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.mipLevel = 0;
    subresource.arrayLayer = 0;

    // Создание лаяута для подресурса
    VkSubresourceLayout stagingImageLayout;
    vkGetImageSubresourceLayout(vulkanDevice->vulkanLogicalDevice, stagingImage, &subresource, &stagingImageLayout);

    // Мапим данные текстуры в адресное пространство CPU
    void* data = nullptr;
    vkMapMemory(vulkanDevice->vulkanLogicalDevice, stagingImageMemory, 0, imageSize, 0, &data);

    // Копируем целиком или построчно в зависимости от размера и выравнивания на GPU
    if (stagingImageLayout.rowPitch == texWidth * 4) {
        memcpy(data, pixels, (size_t) imageSize);
    } else {
        uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);

        for (int y = 0; y < texHeight; y++) {
            memcpy(&dataBytes[y * stagingImageLayout.rowPitch],
                   &pixels[y * texWidth * 4],
                   static_cast<size_t>(texWidth * 4));
        }
    }

    // Размапим данные
    vkUnmapMemory(vulkanDevice->vulkanLogicalDevice, stagingImageMemory);

    // Создаем рабочее изображение для последующего использования
    createImage(vulkanDevice->vulkanLogicalDevice, vulkanDevice->vulkanPhysicalDevice,
                static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight),
                VK_FORMAT_R8G8B8A8_UNORM,   // Формат картинки
                VK_IMAGE_TILING_OPTIMAL,    // тайлинг
                VK_IMAGE_LAYOUT_UNDEFINED,   // Лаяут использования (must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED)
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,   // Используется как получаетель + для отрисовки
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,    // Хранится только на GPU
                vulkanTextureImage,
                vulkanTextureImageMemory);

    // Конвертирование исходной буфферной текстуры с данными в формат копирования на GPU
    vulkanRenderInfo->transitionImageLayout(stagingImage,
                                            VK_FORMAT_R8G8B8A8_UNORM,
                                            VK_IMAGE_LAYOUT_PREINITIALIZED,
                                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    // Конвертирование конечной буфферной текстуры с данными в формат получателя
    vulkanRenderInfo->transitionImageLayout(vulkanTextureImage,
                                            VK_FORMAT_R8G8B8A8_UNORM,
                                            VK_IMAGE_LAYOUT_UNDEFINED,
                                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Копируем данные в пределах GPU из временной текстуры в целевую
    vulkanRenderInfo->queueCopyImage(stagingImage, vulkanTextureImage, texWidth, texHeight);

    // Конвертируем использование текстуры в оптимальное для рендеринга
    vulkanRenderInfo->transitionImageLayout(vulkanTextureImage,
                                            VK_FORMAT_R8G8B8A8_UNORM,
                                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Удаляем временные объекты
    vkDestroyImage(vulkanDevice->vulkanLogicalDevice, stagingImage, nullptr);
    vkFreeMemory(vulkanDevice->vulkanLogicalDevice, stagingImageMemory, nullptr);

    // Учищаем буффер данных картинки
    stbi_image_free(pixels);
}

// Создаем вью текстуры
void VulkanModelInfo::createTextureImageView() {
    createImageView(vulkanDevice->vulkanLogicalDevice,
                    vulkanTextureImage,
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    vulkanTextureImageView);
}

// Создание семплера для картинки
void VulkanModelInfo::createTextureSampler() {
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
    if (vkCreateSampler(vulkanDevice->vulkanLogicalDevice, &samplerInfo, nullptr, &vulkanTextureSampler) != VK_SUCCESS) {
        LOGE("Failed to create texture sampler!");
        throw std::runtime_error("Failed to create texture sampler!");
    }
}

// Грузим данные для модели
void VulkanModelInfo::loadModel(){
    LOGE("Model loading started\n");

    std::vector<unsigned char> modelData = readFile(androidAssetManager, "models/chalet.obj");

    // Потоковый буфферд для данных
    struct MemBuf: std::streambuf {
        MemBuf(unsigned char* begin, unsigned char* end) {
            this->setg((char*)begin, (char*)begin, (char*)end);
        }
    };
    MemBuf sbuf(modelData.data(), modelData.data()+modelData.size());
    std::istream dataStream(&sbuf);

    // Читаем Obj
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &dataStream, NULL, true);
    if (!loaded) {
        throw std::runtime_error(err);
    }

    // Удаляем данные файла
    modelData.clear();

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
            vulkanIndices.push_back(static_cast<uint32_t>(vulkanIndices.size()));
        }
    }

    vulkanTotalVertexesCount = vulkanVertices.size();
    vulkanTotalIndexesCount = vulkanIndices.size();

    LOGE("Model loading complete: %ld vertexes, %ld triangles, %ld indexes\n", vulkanTotalVertexesCount, vulkanTotalVertexesCount/3, vulkanTotalIndexesCount);
}

// Создаем буффер нужного размера
void VulkanModelInfo::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    // Удаляем старое, если есть
    if(bufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(vulkanDevice->vulkanLogicalDevice, bufferMemory, nullptr);
        bufferMemory = VK_NULL_HANDLE;
    }
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(vulkanDevice->vulkanLogicalDevice, buffer, nullptr);
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
    if (vkCreateBuffer(vulkanDevice->vulkanLogicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    // Запрашиваем данные о необходимой памяти
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkanDevice->vulkanLogicalDevice, buffer, &memRequirements);

    // Настройки аллокации буффера
    uint32_t memoryTypeIndex = findMemoryType(vulkanDevice->vulkanPhysicalDevice,
                                              memRequirements.memoryTypeBits,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkMemoryAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkMemoryAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    // Выделяем память для буффера
    if (vkAllocateMemory(vulkanDevice->vulkanLogicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    // Подцепляем память к буфферу
    // Последний параметр – смещение в области памяти. Т.к. эта память выделяется специально для буфера вершин, смещение просто 0.
    // Если же оно не будет равно нулю, то значение должно быть кратным memRequirements.alignment.
    vkBindBufferMemory(vulkanDevice->vulkanLogicalDevice, buffer, bufferMemory, 0);
}

// Создание буфферов вершин
void VulkanModelInfo::createVertexBuffer(){
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
    vkMapMemory(vulkanDevice->vulkanLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);

    // Копируем вершины в память
    memcpy(data, vulkanVertices.data(), (size_t)bufferSize);

    // Размапим
    vkUnmapMemory(vulkanDevice->vulkanLogicalDevice, stagingBufferMemory);

    // Создаем рабочий буффер
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  // Буффер может принимать данные + для отрисовки используется
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,   // Хранится на видео-карте
                 vulkanVertexBuffer, vulkanVertexBufferMemory);

    // Ставим задачу на копирование буфферов
    vulkanRenderInfo->queueCopyBuffer(stagingBuffer, vulkanVertexBuffer, bufferSize);

    // Удаляем временный буффер, если есть
    if(stagingBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(vulkanDevice->vulkanLogicalDevice, stagingBufferMemory, nullptr);
        stagingBufferMemory = VK_NULL_HANDLE;
    }
    if (stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(vulkanDevice->vulkanLogicalDevice, stagingBuffer, nullptr);
        stagingBuffer = VK_NULL_HANDLE;
    }

    // Чистим исходные данные
    vulkanVertices.clear();
}

// Создание буффера индексов
void VulkanModelInfo::createIndexBuffer() {
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
    vkMapMemory(vulkanDevice->vulkanLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);

    // Копируем данные
    memcpy(data, vulkanIndices.data(), (size_t)bufferSize);

    // Размапим память
    vkUnmapMemory(vulkanDevice->vulkanLogicalDevice, stagingBufferMemory);

    // Создаем рабочий буффер
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, // Используется как получатель + индексный буффер
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,   // Хранится только на GPU
                 vulkanIndexBuffer, vulkanIndexBufferMemory);

    // Ставим задачу на копирование буфферов
    vulkanRenderInfo->queueCopyBuffer(stagingBuffer, vulkanIndexBuffer, bufferSize);

    // Удаляем временный буффер, если есть
    if(stagingBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(vulkanDevice->vulkanLogicalDevice, stagingBufferMemory, nullptr);
        stagingBufferMemory = VK_NULL_HANDLE;
    }
    if (stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(vulkanDevice->vulkanLogicalDevice, stagingBuffer, nullptr);
        stagingBuffer = VK_NULL_HANDLE;
    }

    // Чистим исходные данные
    vulkanIndices.clear();
}