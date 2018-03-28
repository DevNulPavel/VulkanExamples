#ifndef VULKAN_FAMILIES_QUEUE_INDEXES_H
#define VULKAN_FAMILIES_QUEUE_INDEXES_H

struct VulkanQueuesFamiliesIndexes {
    int renderQueueFamilyIndex;     // Индекс семейства очередей отрисовки
    int renderQueueFamilyQueuesCount;// Количество очередей в семействе
    int presentQueueFamilyIndex;    // Индекс семейства очередей отображения
    int presentQueueFamilyQueuesCount;// Количество очередей в семействе
    
    VulkanQueuesFamiliesIndexes(){
        renderQueueFamilyIndex = -1;
        renderQueueFamilyQueuesCount = 0;
        presentQueueFamilyIndex = -1;
        presentQueueFamilyQueuesCount = 0;
    }
    bool isComplete(){
        return (renderQueueFamilyIndex >= 0) && (presentQueueFamilyIndex >= 0);
    }
};

#endif
