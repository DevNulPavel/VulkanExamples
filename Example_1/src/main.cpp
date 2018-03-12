#include "CommonIncludes.h"
#include "CommonDefines.h"
#include "VulkanInstance.h"

#include <windows.h>

#define STRINGIFY(_STR_) (#_STR_)


int __main(int argc, char** argv) {
    glfwInit();
    
    if (glfwVulkanSupported()){
        printf("Vulkan available!!!");
    }
    
    //PFN_vkCreateInstance pfnCreateInstance = (PFN_vkCreateInstance)glfwGetInstanceProcAddress(NULL, "vkCreateInstance");
    
    return 0;
}

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
	printf("Windows start");
	__main(1, NULL);
	fflush(stdout);
	if (glfwVulkanSupported()) {
		printf("Vulkan available!!!");
		MessageBox(NULL, lpCmdLine, "Vulkan available!!!", 0);
	}
	MessageBox(NULL, lpCmdLine, "WinMain Demo", 0);
	return 0;
}

