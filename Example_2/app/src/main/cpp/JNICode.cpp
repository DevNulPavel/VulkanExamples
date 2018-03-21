#include <jni.h>
#include <cstring>
#include <stdexcept>
#include "VulkanCodeWrapper/vulkan_wrapper.h"
#include "VulkanDevice.h"

//JNIEXPORT jstring
//JNICALL


////////////////////////////////////////////////////////////////////////////////////
VulkanDevice* vulkanDevice = nullptr;


////////////////////////////////////////////////////////////////////////////////////

extern "C" {

JNICALL
void Java_com_example_devnul_vulkanexample_MainActivity_init(JNIEnv *env, jobject) {
    // Инициализируем указатели на функции Vulkan
    int status = InitVulkan();
    if (status == 0){
        printf("Failed to InitVulkan!\n");
        fflush(stdout);
        throw std::runtime_error("Failed to InitVulkan!");
    }

    vulkanDevice = new VulkanDevice();
    vulkanDevice->createVulkanInstance();
    vulkanDevice->setupDebugCallback();
}

JNICALL
void Java_com_example_devnul_vulkanexample_MainActivity_draw(JNIEnv *env, jobject) {
}

JNICALL
void Java_com_example_devnul_vulkanexample_MainActivity_destroy(JNIEnv *env, jobject) {
    delete vulkanDevice;
}

}