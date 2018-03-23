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

public:
    VulkanRenderInfo(VulkanDevice* device, VulkanVisualizer* visualizer, AAssetManager* assetManager);
    ~VulkanRenderInfo();

private:
    // Стадии загрузки
    void createRenderPass();    // Создание рендер-прохода
    void createUniformDescriptorSetLayout(); // Создаем дескриптор для буффера юниформов
    void createGraphicsPipeline();    // Создание пайплайна отрисовки

    ////////////////

    // Из байткода исходника создаем шейдерный модуль
    void createShaderModule(const std::vector<char>& code, VkShaderModule& shaderModule);
    // Читаем побайтово файлик
    std::vector<char> readFile(const std::string& filename);
};


#endif
