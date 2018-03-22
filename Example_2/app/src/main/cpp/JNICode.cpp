#include <jni.h>
#include <cstring>
#include <stdexcept>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "VulkanCodeWrapper/vulkan_wrapper.h"
#include "VulkanDevice.h"
#include "SupportFunctions.h"

//JNIEXPORT jstring
//JNICALL


////////////////////////////////////////////////////////////////////////////////////
VulkanDevice* vulkanDevice = nullptr;


////////////////////////////////////////////////////////////////////////////////////

extern "C" {

JNICALL
void Java_com_example_devnul_vulkanexample_VulkanDrawThread_vulkanInit(JNIEnv *env, jobject thisObj, jobject surface) {
    // Инициализируем указатели на функции Vulkan
    int status = InitVulkan();
    if (status == 0){
        LOGE("Vulkan dynamic library loading failed!");
        throw std::runtime_error("Failed to InitVulkan!");
    }

    ANativeWindow* androidNativeWindow = ANativeWindow_fromSurface(env, surface);

    vulkanDevice = new VulkanDevice();
    vulkanDevice->createVulkanInstance();
    vulkanDevice->setupDebugCallback();
    vulkanDevice->createSurface(androidNativeWindow);
    vulkanDevice->selectPhysicalDevice();
    vulkanDevice->createLogicalDeviceAndQueue();
}

JNICALL
void Java_com_example_devnul_vulkanexample_VulkanDrawThread_vulkanDraw(JNIEnv *env, jobject thisObj) {
}

JNICALL
void Java_com_example_devnul_vulkanexample_VulkanDrawThread_vulkanDestroy(JNIEnv *env, jobject thisObj) {
    delete vulkanDevice;

    vulkanDevice = nullptr;
}

}