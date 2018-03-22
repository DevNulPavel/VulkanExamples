#ifndef VULKANRENDERINFO_H
#define VULKANRENDERINFO_H

#include "VulkanCodeWrapper/vulkan_wrapper.h"


struct VulkanDevice;
struct VulkanVisualizer;


struct VulkanRenderInfo {
public:
    VulkanDevice* vulkanDevice;
    VulkanVisualizer* vulkanVisualizer;
    VkRenderPass vulkanRenderPass;

public:
    VulkanRenderInfo(VulkanDevice* device, VulkanVisualizer* visualizer);
    ~VulkanRenderInfo();

private:
    void createRenderPass();    // Создание рендер-прохода
};


#endif
