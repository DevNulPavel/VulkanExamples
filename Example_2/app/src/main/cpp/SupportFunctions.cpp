#include "SupportFunctions.h"

#include <cstring>
#include <cstdlib>
#include <stdexcept>


// Читаем побайтово файлик
std::vector<unsigned char> readFile(AAssetManager* androidAssetManager, const std::string& filename) {
    AAsset* file = AAssetManager_open(androidAssetManager, filename.c_str(), AASSET_MODE_BUFFER);
    if(file == NULL){
        LOGE("Failed to read file %s", filename.c_str());
        throw std::runtime_error("Failed to read shader file!");
    }

    off_t fileLength = AAsset_getLength(file);

    std::vector<unsigned char> buffer;
    buffer.resize(static_cast<size_t >(fileLength));

    AAsset_read(file, buffer.data(), buffer.size());

    AAsset_close(file);

    return buffer;
}

// Подбираем тип памяти под свойства
uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    // Запрашиваем типы памяти физического устройства
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

    // Найдем тип памяти, который подходит для самого буфера
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

// Создаем изображение
void createImage(VkDevice vulkanLogicalDevice, VkPhysicalDevice physDevice,
                 uint32_t width, uint32_t height,
                 VkFormat format, VkImageTiling tiling,
                 VkImageLayout layout, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties,
                 VkImage& image, VkDeviceMemory& imageMemory) {

    // Удаляем старые объекты
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(vulkanLogicalDevice, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    if (imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(vulkanLogicalDevice, imageMemory, nullptr);
        imageMemory = VK_NULL_HANDLE;
    }

    // Для поля initialLayout есть только два возможных значения:
    // VK_IMAGE_LAYOUT_UNDEFINED: Не и используется GPU и первое изменение (transition) отбросит все тексели.
    // VK_IMAGE_LAYOUT_PREINITIALIZED: Не и используется GPU, но первое изменение (transition) сохранит тексели.
    // Первый вариант подходит для изображений, которые будут использоваться в качестве вложений, как буфер цвета и глубины.
    // В таком случае не нужно заботиться о данных инициализации, т.к. они скорее всего будут очищены проходом рендера до начала использования. А если же вы хотите заполнить данные, такие как текстуры, тогда используйте второй вариант:

    // Информация об изображении
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D; // 2D текстура
    imageInfo.extent.width = width;     // Ширина
    imageInfo.extent.height = height;   // Высота
    imageInfo.extent.depth = 1;         // Глубина
    imageInfo.mipLevels = 1;            // Без мипмапов
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;          // Формат данных текстуры
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = layout; // Текстура заранее с нужными данными
    imageInfo.usage = usage;    // Флаги использования текстуры
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;  // Семплирование данной текстуры
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Режим междевайного доступа

    // Создаем изображение
    if (vkCreateImage(vulkanLogicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        LOGE("Failed to create image!");
        throw std::runtime_error("Failed to create image!");
    }

    // Запрашиваем информацию о требованиях памяти для текстуры
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vulkanLogicalDevice, image, &memRequirements);

    // Подбираем нужный тип аллоцируемой памяти для требований и возможностей
    uint32_t memoryType = findMemoryType(physDevice, memRequirements.memoryTypeBits, properties);
    VkMemoryAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkMemoryAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;    // Размер аллоцируемой памяти
    allocInfo.memoryTypeIndex = memoryType;             // Тип памяти

    if (vkAllocateMemory(vulkanLogicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    // Цепляем к картинке буффер памяти
    vkBindImageMemory(vulkanLogicalDevice, image, imageMemory, 0);
}

// Создание вью для изображения
void createImageView(VkDevice vulkanLogicalDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView) {
    // Удаляем старый объект, если есть
    if(imageView != VK_NULL_HANDLE){
        vkDestroyImageView(vulkanLogicalDevice, imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }

    // Описание вьюшки
    VkImageViewCreateInfo viewInfo = {};
    memset(&viewInfo, 0, sizeof(VkImageViewCreateInfo));
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image; // Изображение
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 2D
    viewInfo.format = format;   // Формат вьюшки
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдольным компонентам??
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдольным компонентам??
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдольным компонентам??
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдольным компонентам??
    viewInfo.subresourceRange.aspectMask = aspectFlags; // Использование вью текстуры
    viewInfo.subresourceRange.baseMipLevel = 0; // 0 мипмаплевел
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    // Создаем имедж вью
    if (vkCreateImageView(vulkanLogicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        LOGE("Failed to create texture image view!");
        throw std::runtime_error("Failed to create texture image view!");
    }
}