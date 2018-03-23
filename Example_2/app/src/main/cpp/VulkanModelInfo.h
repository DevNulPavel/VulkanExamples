#ifndef EXAMPLE_2_VULKANMODELINFO_H
#define EXAMPLE_2_VULKANMODELINFO_H

#include <vector>
#include <string>
#include <android/asset_manager.h>
#include <vulkan_wrapper.h>
#include "Vertex.h"

struct VulkanDevice;
struct VulkanVisualizer;
struct VulkanRenderInfo;

struct VulkanModelInfo {
public:
    VulkanDevice* vulkanDevice;
    VulkanVisualizer* vulkanVisualizer;
    VulkanRenderInfo* vulkanRenderInfo;
    AAssetManager* androidAssetManager;
    VkImage vulkanTextureImage;
    VkDeviceMemory vulkanTextureImageMemory;
    VkImageView vulkanTextureImageView;
    VkSampler vulkanTextureSampler;
    std::vector<Vertex> vulkanVertices;
    std::vector<uint32_t> vulkanIndices;
    size_t vulkanTotalVertexesCount = 0;
    size_t vulkanTotalIndexesCount = 0;
    VkBuffer vulkanVertexBuffer;
    VkDeviceMemory vulkanVertexBufferMemory;
    VkBuffer vulkanIndexBuffer;
    VkDeviceMemory vulkanIndexBufferMemory;

public:
    VulkanModelInfo(VulkanDevice* device, VulkanVisualizer* visualizer, VulkanRenderInfo* renderInfo, AAssetManager* assetManager);
    ~VulkanModelInfo();

private:
    // Стадии инициализации
    void createTextureImage();        // Создание текстуры из изображения
    void createTextureImageView();    // Создаем вью текстуры
    void createTextureSampler();      // Создание семплера для картинки
    void loadModel();                 // Грузим данные для модели
    void createVertexBuffer();        // Создание буфферов вершин
    void createIndexBuffer();         // Создание буффера индексов

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    // Создаем буффер нужного размера
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
};


#endif
