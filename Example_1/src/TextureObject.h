#ifndef TEXTURE_OBJECT_H
#define TEXTURE_OBJECT_H

#include "CommonIncludes.h"
#include "CommonDefines.h"

struct texture_object {
    VkSampler sampler;
    
    VkImage image;
    VkImageLayout imageLayout;
    
    VkMemoryAllocateInfo mem_alloc;
    VkDeviceMemory mem;
    VkImageView view;
    int32_t tex_width, tex_height;
};

#endif
