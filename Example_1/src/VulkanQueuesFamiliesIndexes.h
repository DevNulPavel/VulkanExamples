#ifndef VULKAN_FAMILIES_QUEUE_INDEXES_H
#define VULKAN_FAMILIES_QUEUE_INDEXES_H

struct VulkanQueuesFamiliesIndexes {
    int renderQueuesFamilyIndex;     // Индекс семейства очередей отрисовки
    int renderQueuesFamilyQueuesCount;// Количество очередей в семействе
    int presentQueuesFamilyIndex;    // Индекс семейства очередей отображения
    int presentQueuesFamilyQueuesCount;// Количество очередей в семействе
    
    VulkanQueuesFamiliesIndexes();
    bool isComplete() const; 
};

#endif
