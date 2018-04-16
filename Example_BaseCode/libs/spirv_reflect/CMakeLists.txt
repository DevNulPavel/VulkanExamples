cmake_minimum_required(VERSION 3.0)

project(spirv-reflect)


set(CMAKE_CXX_STANDARD 14)


set(SPIRV_REFLECT_SPIRV_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/spirv/unified1" PARENT_SCOPE)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/spirv/unified1")


add_library(spirv-reflect 
            STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect.h
            ${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect.c)

# add_subdirectory(util/stripper)
