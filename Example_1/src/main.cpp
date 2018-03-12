#include "CommonIncludes.h"
#include "CommonDefines.h"
#include "VulkanInstance.h"

#define STRINGIFY(_STR_) (#_STR_)

int main(int argc, char** argv) {
    glfwInit();
    
    if (glfwVulkanSupported()){
        printf("Vulkan available!!!");
    }
    
    //PFN_vkCreateInstance pfnCreateInstance = (PFN_vkCreateInstance)glfwGetInstanceProcAddress(NULL, "vkCreateInstance");
    
    return 0;
}

