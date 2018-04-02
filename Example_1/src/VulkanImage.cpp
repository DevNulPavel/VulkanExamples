#include "VulkanImage.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "VulkanHelpers.h"
#include "Helpers.h"


VulkanImage::VulkanImage():
    _image(VK_NULL_HANDLE),
    _imageMemory(VK_NULL_HANDLE),
    _format(VK_FORMAT_UNDEFINED),
    _size(VkExtent2D{0, 0}),
    _needDestroy(false){
}

VulkanImage::VulkanImage(VkImage image, VkFormat format, VkExtent2D size):
    _image(image),
    _imageMemory(VK_NULL_HANDLE),
    _format(format),
    _size(size),
    _tiling(VK_IMAGE_TILING_OPTIMAL),
    _layout(VK_IMAGE_LAYOUT_UNDEFINED),
    _usage(0),
    _properties(0),
    _mipmapsCount(1),
    _needDestroy(false){
    // Удаляется извне
}

VulkanImage::VulkanImage(VulkanLogicalDevicePtr device, VkImage image, VkFormat format, VkExtent2D size, bool needDestroy):
    _logicalDevice(device),
    _image(image),
    _imageMemory(VK_NULL_HANDLE),
    _format(format),
    _size(size),
    _tiling(VK_IMAGE_TILING_OPTIMAL),
    _layout(VK_IMAGE_LAYOUT_UNDEFINED),
    _usage(0),
    _properties(0),
    _mipmapsCount(1),
    _needDestroy(needDestroy){
}

VulkanImage::VulkanImage(VulkanLogicalDevicePtr logicDevice,
                         uint32_t width, uint32_t height,
                         VkFormat format,
                         VkImageTiling tiling,
                         VkImageLayout layout,
                         VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties,
                         uint32_t mipmapsCount):
    _logicalDevice(logicDevice),
    _image(VK_NULL_HANDLE),
    _imageMemory(VK_NULL_HANDLE),
    _format(format),
    _size({width, height}),
    _tiling(tiling),
    _layout(layout),
    _usage(usage),
    _properties(properties),
    _mipmapsCount(mipmapsCount),
    _needDestroy(true){
        
    createImage(width, height,
                format,
                tiling,
                layout,
                usage,
                properties,
                mipmapsCount);
}

VulkanImage::~VulkanImage(){
    if (_needDestroy && _logicalDevice) {
        if (_image) {
            vkDestroyImage(_logicalDevice->getDevice(), _image, nullptr);
        }
        if (_imageMemory) {
            vkFreeMemory(_logicalDevice->getDevice(), _imageMemory, nullptr);
        }
    }
}

VkImage VulkanImage::getImage() const{
    return _image;
}

VkDeviceMemory VulkanImage::getImageMemory() const{
    return _imageMemory;
}

VkFormat VulkanImage::getBaseFormat() const{
    return _format;
}

VkExtent2D VulkanImage::getBaseSize() const{
    return _size;
}

VulkanLogicalDevicePtr VulkanImage::getBaseDevice() const{
    return _logicalDevice;
}

VkImageTiling VulkanImage::getBaseTiling() const{
    return _tiling;
}

VkImageLayout VulkanImage::getBaseLayout() const{
    return _layout;
}

VkImageUsageFlags VulkanImage::getBaseUsage() const{
    return _usage;
}

VkMemoryPropertyFlags VulkanImage::getBaseProperties() const{
    return _properties;
}

uint32_t VulkanImage::getBaseMipmapsCount() const{
    return _mipmapsCount;
}

void VulkanImage::setNewLayout(VkImageLayout layout){
    _layout = layout;
}

// Создаем изображение
void VulkanImage::createImage(uint32_t width, uint32_t height,
                              VkFormat format,
                              VkImageTiling tiling,
                              VkImageLayout layout,
                              VkImageUsageFlags usage,
                              VkMemoryPropertyFlags properties,
                              uint32_t mipmapsCount) {
    
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
    imageInfo.mipLevels = mipmapsCount; // Уровней мипмапинга всего?
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;          // Формат данных текстуры
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = layout; // Текстура заранее с нужными данными
    imageInfo.usage = usage;    // Флаги использования текстуры
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;  // Семплирование данной текстуры
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Режим междевайного доступа
    
    // Создаем изображение
    if (vkCreateImage(_logicalDevice->getDevice(), &imageInfo, nullptr, &_image) != VK_SUCCESS) {
        LOG("Failed to create image!\n");
        throw std::runtime_error("Failed to create image!");
    }
    
    // Запрашиваем информацию о требованиях памяти для текстуры
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_logicalDevice->getDevice(), _image, &memRequirements);
    
    // Подбираем нужный тип аллоцируемой памяти для требований и возможностей
    uint32_t memoryType = findMemoryType(_logicalDevice->getBasePhysicalDevice()->getDevice(), memRequirements.memoryTypeBits, properties);
    VkMemoryAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkMemoryAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;    // Размер аллоцируемой памяти
    allocInfo.memoryTypeIndex = memoryType;             // Тип памяти
    
    if (vkAllocateMemory(_logicalDevice->getDevice(), &allocInfo, nullptr, &_imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }
    
    // Цепляем к картинке буффер памяти
    vkBindImageMemory(_logicalDevice->getDevice(), _image, _imageMemory, 0);
}

// Получаем лаяут картинки
VkSubresourceLayout VulkanImage::getSubresourceLayout(VkImageAspectFlags aspect, uint32_t mipLevel) const{
    // Описание подресурса для изображения
    VkImageSubresource subresource = {};
    memset(&subresource, 0, sizeof(VkImageSubresource));
    subresource.aspectMask = aspect;
    subresource.mipLevel = mipLevel;
    subresource.arrayLayer = 0;
    
    // Создание лаяута для подресурса
    VkSubresourceLayout stagingImageLayout;
    vkGetImageSubresourceLayout(_logicalDevice->getDevice(),
                                _image,
                                &subresource,
                                &stagingImageLayout);
    return stagingImageLayout;
}

// Загружаем данные в картинку
void VulkanImage::uploadDataToImage(VkImageAspectFlags aspect, uint32_t mipLevel, unsigned char* imageSourceData, size_t dataSize){
    // Создание лаяута для подресурса
    VkSubresourceLayout stagingImageLayout = getSubresourceLayout(aspect, mipLevel);
    
    // Мапим данные текстуры в адресное пространство CPU
    void* mappedData = nullptr;
    vkMapMemory(_logicalDevice->getDevice(), _imageMemory, 0, stagingImageLayout.size, 0, &mappedData);
    
    // Копируем целиком или построчно в зависимости от размера и выравнивания на GPU
    if (stagingImageLayout.rowPitch == static_cast<VkDeviceSize>(_size.width * 4)) {
        memcpy(mappedData, imageSourceData, dataSize);
    } else {
        uint8_t* dataBytes = reinterpret_cast<uint8_t*>(mappedData);
        
        for (size_t y = 0; y < static_cast<size_t>(_size.height); y++) {
            memcpy(&dataBytes[y * stagingImageLayout.rowPitch],
                   &imageSourceData[y * _size.width * 4],
                   _size.width * 4);
        }
    }
    
    // Размапим данные
    vkUnmapMemory(_logicalDevice->getDevice(), _imageMemory);
}

