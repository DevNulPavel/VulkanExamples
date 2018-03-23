cmake_minimum_required(VERSION 3.4.1)

add_library(vulkan_cpp_code

             SHARED

             src/main/cpp/JNICode.cpp
             src/main/cpp/VulkanDevice.h
             src/main/cpp/VulkanDevice.cpp
             src/main/cpp/VulkanVisualizer.h
             src/main/cpp/VulkanVisualizer.cpp
             src/main/cpp/VulkanRenderInfo.h
             src/main/cpp/VulkanRenderInfo.cpp
             src/main/cpp/VulkanModelInfo.h
             src/main/cpp/VulkanModelInfo.cpp
             src/main/cpp/SupportFunctions.h
             src/main/cpp/SupportFunctions.cpp
             src/main/cpp/UniformBuffer.h
             src/main/cpp/UniformBuffer.cpp
             src/main/cpp/Vertex.cpp
             src/main/cpp/Vertex.h)

# GLM library
include_directories(src/main/cpp_libs/glm/)

# STB
include_directories(src/main/cpp_libs/stb_image/)

# Tiny OBJ
include_directories(src/main/cpp_libs/tiny_obj_loader/)

# VulkanWrapper
add_library(vulkan_wrapper
            STATIC
            src/main/cpp_libs/vulkan_wrapper/vulkan_wrapper.h
            src/main/cpp_libs/vulkan_wrapper/vulkan_wrapper.cpp)
include_directories(src/main/cpp_libs/vulkan_wrapper/)

# Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -std=c++11 -Wall -Werror -DVK_USE_PLATFORM_ANDROID_KHR -DNDEBUG")

# System libs search
find_library(LOG_LIB log)
find_library(ANDROID_LIB android)

# Link with libs
target_link_libraries(vulkan_cpp_code ${LOG_LIB} ${ANDROID_LIB} vulkan_wrapper)