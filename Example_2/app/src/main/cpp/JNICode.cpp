#include <jni.h>
#include <cstring>
#include <stdexcept>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <vulkan_wrapper.h>
#include "VulkanDevice.h"
#include "VulkanVisualizer.h"
#include "VulkanRenderInfo.h"
#include "VulkanModelInfo.h"
#include "SupportFunctions.h"

//JNIEXPORT jstring
//JNICALL


////////////////////////////////////////////////////////////////////////////////////
VulkanDevice* vulkanDevice = nullptr;
VulkanVisualizer* vulkanVisualizer = nullptr;
VulkanRenderInfo* vulkanRenderInfo = nullptr;
VulkanModelInfo* vulkanModelInfo = nullptr;


////////////////////////////////////////////////////////////////////////////////////

extern "C" {

JNICALL
void Java_com_example_devnul_vulkanexample_VulkanDrawThread_vulkanInit(JNIEnv *env,
                                                                       jobject thisObj,
                                                                       jobject surface,
                                                                       jint width, jint height,
                                                                       jobject assetManager) {
    // Инициализируем указатели на функции Vulkan
    int status = InitVulkan();
    if (status == 0){
        LOGE("Vulkan dynamic library loading failed!");
        throw std::runtime_error("Failed to InitVulkan!");
    }

    ANativeWindow* androidNativeWindow = ANativeWindow_fromSurface(env, surface);
    AAssetManager* androidNativeAssetManager = AAssetManager_fromJava(env, assetManager);

    // Создание устройства
    vulkanDevice = new VulkanDevice(androidNativeWindow, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    // Создание свопчейна
    vulkanVisualizer = new VulkanVisualizer(vulkanDevice);

    // Рендер инфо
    vulkanRenderInfo = new VulkanRenderInfo(vulkanDevice, vulkanVisualizer, androidNativeAssetManager);

    // Создаем фреймбуфферы для рендер-прохода
    vulkanVisualizer->createFramebuffers(vulkanRenderInfo);

    // Данные о модели
    vulkanModelInfo = new VulkanModelInfo(vulkanDevice, vulkanVisualizer, vulkanRenderInfo, androidNativeAssetManager);
}

JNICALL
void Java_com_example_devnul_vulkanexample_VulkanDrawThread_vulkanDraw(JNIEnv *env, jobject thisObj) {
}

JNICALL
void Java_com_example_devnul_vulkanexample_VulkanDrawThread_vulkanDestroy(JNIEnv *env, jobject thisObj) {
    delete vulkanModelInfo;
    delete vulkanRenderInfo;
    delete vulkanVisualizer;
    delete vulkanDevice;

    vulkanModelInfo = nullptr;
    vulkanRenderInfo = nullptr;
    vulkanVisualizer = nullptr;
    vulkanDevice = nullptr;
}

}