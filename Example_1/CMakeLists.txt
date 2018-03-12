set(PROJECT_NAME "VulkanDemo1")
set(EXECUTABLE_NAME "VulkanDemo1")
set(OSX_MOLTEN_RELATIVE_PATH "../../MoltenVK/Package/Release/MoltenVK")


####################################################
# Настройка CMake сборки
####################################################
cmake_minimum_required(VERSION 3.7 FATAL_ERROR)


# Информация о текущих настройках
function(print_current_cmake_config)
	# https://cmake.org/Wiki/CMake_Useful_Variables
	message("******** Project compilation settings ***********")
	message("System = ${CMAKE_SYSTEM}")
	message("System name = ${CMAKE_SYSTEM_NAME}")
	message("System version = ${CMAKE_SYSTEM_VERSION}")
	message("CMake Generator = ${CMAKE_GENERATOR}")
	message("CMake build type = ${CMAKE_BUILD_TYPE}")
	message("CC compiler = ${CMAKE_C_COMPILER}")
	message("CC Flags (Debug) = ${CMAKE_C_FLAGS_DEBUG}")
	message("CC Flags (Release) = ${CMAKE_C_FLAGS_RELEASE}")
endfunction()

# Цвета
option(CMAKE_COLOR_MAKEFILE "Color makefile" ON)

# Оставляем толкьо релизную и дебажную сборку
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

# Инициализация проекта
project(${PROJECT_NAME})

enable_testing()


####################################################
# Библиотеки в системе
####################################################
message("******** System libs search ***********")

# На MacOS используем MoltenVK (https://github.com/KhronosGroup/MoltenVK)
# Возможно - надо поправить файлик FindVulkan.cmake в glfw/cmake/modules/
# убрав при этом поиск неправильный
# elseif (APPLE__)
#     find_library(VULKAN_LIBRARY vulkan HINTS
#         "$ENV{VULKAN_SDK}/macOS/lib")
#     find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS
#         "$ENV{VULKAN_SDK}/macOS/include")
# else()

if(APPLE___)
	set(MONTLE_FULL_PATH "${CMAKE_SOURCE_DIR}/${OSX_MOLTEN_RELATIVE_PATH}")
	set(MONTLE_INCLUDE_PATH "${MONTLE_FULL_PATH}/include")
	set(MONTLE_LIBRARIES_PATH "${MONTLE_FULL_PATH}/macOS")
	set(MONTLE_DYNAMIC_LIB_PATH "${MONTLE_LIBRARIES_PATH}/libMoltenVK.dylib")
	set(MONTLE_FRAMEWORK_PATH "${MONTLE_LIBRARIES_PATH}/MoltenVK.framework")

	message("Montle include path: ${MONTLE_INCLUDE_PATH}")
	message("Montle dynamic lib path: ${MONTLE_DYNAMIC_LIB_PATH}")
	message("Montle framework path: ${MONTLE_FRAMEWORK_PATH}")

	# Указываем, где искать кастомные фреймворки (для GLFW)! 
	# Возможно - надо поправить файлик FindVulkan.cmake в glfw/cmake/modules/
	# "find_library(VULKAN_LIBRARY vulkan.1" -> "find_library(VULKAN_LIBRARY MoltenVK"
	# set(CMAKE_FRAMEWORK_PATH ${CMAKE_FRAMEWORK_PATH} "${MONTLE_LIBRARIES_PATH}")

	# Инклюды библиотеки
	include_directories(${MONTLE_INCLUDE_PATH})

	# Список фреймворков
	set(LIBS_LINK_NAMES ${LIBS_LINK_NAMES}
		"-framework Metal"
		"-framework IOSurface"
		"-framework QuartzCore"
		"-framework IOKit"
		"-framework Foundation"
		"-framework MoltenVK")

	# Путь поиска фреймворков
	set(FRAMEWORKS_PATHES ${FRAMEWORKS_PATHES} "-F${MONTLE_LIBRARIES_PATH}")

else()
	# https://cmake.org/cmake/help/v3.7/module/FindVulkan.html
	find_package(Vulkan)
	if(Vulkan_FOUND)
		include_directories(${Vulkan_INCLUDE_DIRS})
	    set(LIBS_LINK_NAMES ${LIBS_LINK_NAMES} ${Vulkan_LIBRARIES})
	    message("Vulkan includes: ${Vulkan_INCLUDE_DIRS}")
	    message("Vulkan libs: ${Vulkan_LIBRARIES}")
	else()
		message(FATAL_ERROR "Vulkan lib not found!")
	endif()
endif()

##############
# Системные библиотеки
set(LIBS_LINK_NAMES ${LIBS_LINK_NAMES} m)


####################################################
# Библиотеки из исходников
####################################################
message("******** Source libraries search ***********")

##############
# GLM library (Header Only mode)
message("GLM added")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libs/glm")

##############
# GLFW library
# Устанавливаем нужные нам настройки
message("GLFW added")
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_VULKAN_STATIC "Use the Vulkan loader statically linked into application" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
if (WIN32)
    option(GLFW_USE_HYBRID_HPG "Force use of high-performance GPU on hybrid systems" ON)
endif()
if (APPLE)
    option(GLFW_USE_CHDIR "Make glfwInit chdir to Contents/Resources" ON)
    option(GLFW_USE_MENUBAR "Populate the menu bar on first window creation" ON)
    option(GLFW_USE_RETINA "Use the full resolution of Retina displays" ON)
endif()
if (UNIX AND NOT APPLE)
    option(GLFW_USE_WAYLAND "Use Wayland for window creation" OFF)
    option(GLFW_USE_MIR     "Use Mir for window creation" OFF)
endif()
if (MSVC)
    option(USE_MSVC_RUNTIME_LIBRARY_DLL "Use MSVC runtime library DLL" ON)
endif()

# Добавляем библиотеку
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw/include")
set(LIBS_LINK_NAMES ${LIBS_LINK_NAMES} glfw ${GLFW_LIBRARIES})
message("GLFW link libraries list: ${GLFW_LIBRARIES}")


####################################################
# Настройка сборки текущего проекта
####################################################
# Сразу отключаем ненужные предупреждения на Windows
if (MSYS)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif ()

# Не кастомные флаги компиляции
if(APPLE)
	set(PROJECT_LINK_FLAGS "-stdlib=libc++")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -Wall -g")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -Wall")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -Wall -g -stdlib=libc++ -std=c++11")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -Wall -stdlib=libc++ -std=c++11")
endif ()
if (UNIX AND NOT APPLE)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -Wall -g")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -Wall -g -std=c++11")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -Wall -std=c++11")
endif ()
if (MSYS)
	# микрософту не нужен флаг -std=c++11
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /O0 /Wall /g")
    set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /O2 /Wall")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /O0 /Wall /g")
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /O2 /Wall")
endif ()
if (WIN32)
    # микрософту не нужен флаг -std=c++11 +  -stdlib=libc++??
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -Wall -g -std=c99")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -Wall -std=c99")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -Wall -g -std=c++11")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -Wall -std=c++11")
endif ()

# Вывод информации о настройке копиляции (Функция в CMake/Helpers.cmake)
print_current_cmake_config()



####################################################
# Исходники
####################################################
set(ALL_SOURCES 
	src/main.cpp 
	src/CommonIncludes.h
	src/CommonDefines.h
	src/VulkanInstance.h
	src/VulkanInstance.cpp
	src/SwapchainImageResources.h
	src/SwapchainImageResources.cpp
	src/TextureObject.h
	src/TextureObject.cpp)

source_group("Sources" FILES ${ALL_SOURCES})

# тип сборки
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Release" FORCE)
endif()


####################################################
# Генерация
####################################################
message("******** Generating started ***********")
# Из найденных исходников выставляем генерацию бинарника
add_executable(${EXECUTABLE_NAME} ${ALL_SOURCES})

# Конкретные пути к бинарнику
set_target_properties(${EXECUTABLE_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
)

if(APPLE__)
	# Чтобы xcode находил фреймворк Molten
	set_target_properties(${EXECUTABLE_NAME} PROPERTIES COMPILE_FLAGS ${FRAMEWORKS_PATHES})
	set_target_properties(${EXECUTABLE_NAME} PROPERTIES LINK_FLAGS ${FRAMEWORKS_PATHES})
endif()

# Библиотеки, с которыми будем ликовать
target_link_libraries(${EXECUTABLE_NAME} ${LIBS_LINK_NAMES})
