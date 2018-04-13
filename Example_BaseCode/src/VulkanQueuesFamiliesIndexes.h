#ifndef VULKAN_FAMILIES_QUEUE_INDEXES_H
#define VULKAN_FAMILIES_QUEUE_INDEXES_H

#include <cstdint>


struct VulkanQueuesFamiliesIndexes {
    int32_t renderQueuesFamilyIndex;           // Индекс семейства очередей отрисовки
    uint32_t renderQueuesFamilyQueuesCount;     // Количество очередей в семействе
    uint32_t renderQueuesTimeStampValidBits;    // Сколько бит в таймстемпе валидны
    int32_t presentQueuesFamilyIndex;          // Индекс семейства очередей отображения
    uint32_t presentQueuesFamilyQueuesCount;    // Количество очередей в семействе
    uint32_t presentQueuesTimeStampValidBits;   // Сколько бит в таймстемпе валидны
    
    VulkanQueuesFamiliesIndexes();
    bool isComplete() const; 
};

#endif
