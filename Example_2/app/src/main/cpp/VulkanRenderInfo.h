#ifndef VULKANRENDERINFO_H
#define VULKANRENDERINFO_H

#include <vector>
#include <string>
#include <android/asset_manager.h>
#include <vulkan_wrapper.h>


struct VulkanDevice;
struct VulkanVisualizer;


struct VulkanRenderInfo {
public:
    VulkanDevice* vulkanDevice;
    VulkanVisualizer* vulkanVisualizer;
    AAssetManager* androidAssetManager;
    VkRenderPass vulkanRenderPass;
    VkDescriptorSetLayout vulkanDescriptorSetLayout;
    VkShaderModule vulkanVertexShader;
    VkShaderModule vulkanFragmentShader;
    VkPipelineLayout vulkanPipelineLayout;
    VkPipeline vulkanPipeline;
    VkCommandPool vulkanCommandPool;

public:
    VulkanRenderInfo(VulkanDevice* device, VulkanVisualizer* visualizer, AAssetManager* assetManager);
    ~VulkanRenderInfo();

    // Запуск коммандного буффера на получение комманд
    VkCommandBuffer beginSingleTimeCommands();
    // Завершение коммандного буффера
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    // Перевод изображения из одного лаяута в другой (из одного способа использования в другой)
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    // Закидываем в очередь операцию копирования текстуры
    void queueCopyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height);
    // Копирование буффера
    void queueCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

private:
    // Стадии загрузки
    void createRenderPass();    // Создание рендер-прохода
    void createUniformDescriptorSetLayout(); // Создаем дескриптор для буффера юниформов
    void createGraphicsPipeline();    // Создание пайплайна отрисовки
    void createRenderCommandPool();   // Создаем пулл комманд
    void updateDepthTextureLayout();  // Обновляем лаяут текстуры глубины на правильный

    ////////////////

    // Из байткода исходника создаем шейдерный модуль
    void createShaderModule(const std::vector<unsigned char>& code, VkShaderModule& shaderModule);
};


#endif
