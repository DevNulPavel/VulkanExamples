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
	message("CXX compiler = ${CMAKE_CXX_COMPILER}")
	message("CC Flags (Debug) = ${CMAKE_C_FLAGS_DEBUG}")
	message("CC Flags (Release) = ${CMAKE_C_FLAGS_RELEASE}")
	message("CXX Flags (Debug) = ${CMAKE_CXX_FLAGS_DEBUG}")
	message("CXX Flags (Release) = ${CMAKE_CXX_FLAGS_RELEASE}")
endfunction()

# Модули поиска
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

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
# Возможно - надо поправить файлик FindVulkan.cmake в glfw/cmake/modules/ + в папке CMake проекта
# убрав при этом поиск неправильный поиск библиотеки
# elseif (APPLE)
#     find_library(VULKAN_LIBRARY MoltenVK HINTS
#         "$ENV{VULKAN_SDK}/lib")
#     find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS
#         "$ENV{VULKAN_SDK}/include")
# else()

# Стандартный поиск плохо работает на Mac с MoltenVK https://cmake.org/cmake/help/v3.7/module/FindVulkan.html, 
# Как результат - требует установки переменной окружения VULKAN_SDK, используется кастомный поиск Vulkan из CMake папки (взято из GLFW)
find_package(Vulkan)
if(VULKAN_FOUND)
	include_directories(${VULKAN_INCLUDE_DIR})
    set(LIBS_LINK_NAMES ${LIBS_LINK_NAMES} ${VULKAN_LIBRARY})
    message("Vulkan includes: ${VULKAN_INCLUDE_DIR}")
    message("Vulkan libs: ${VULKAN_LIBRARY}")
else()
	message(FATAL_ERROR "Vulkan lib not found!")
endif()

# Список фреймворков OSX для MoltenVK
if(APPLE)
	set(LIBS_LINK_NAMES ${LIBS_LINK_NAMES}
		"-framework Metal"
		"-framework IOSurface"
		"-framework QuartzCore"
		"-framework IOKit"
		"-framework Foundation")
endif()

##############
# Системные библиотеки
if(UNIX)
	set(LIBS_LINK_NAMES ${LIBS_LINK_NAMES} m)
endif()



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
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_VULKAN_STATIC "Use the Vulkan loader statically linked into application" ON)
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
# Не кастомные флаги компиляции
if(APPLE)
	set(PROJECT_LINK_FLAGS "-stdlib=libc++")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -Wall -g")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -Wall")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -Wall -g -stdlib=libc++ -std=c++11")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -Wall -stdlib=libc++ -std=c++11")
elseif (UNIX AND NOT APPLE)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -Wall -g")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -Wall -g -std=c++11")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -Wall -std=c++11")
elseif (MSVC)
	# Сразу отключаем ненужные предупреждения на Windows
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)

	# микрософту не нужен флаг -std=c++11
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /O0 /Wall /g")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /O2 /Wall")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /O0 /Wall /g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /O2 /Wall")

	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:msvcrt.lib")
	SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:msvcrt.lib")

	message("Microsoft VS flags")
elseif (WIN32)
    # микрософту не нужен флаг -std=c++11 +  -stdlib=libc++??
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -Wall -g -std=c99")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -Wall -std=c99")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -Wall -g -std=c++11")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -Wall -std=c++11")
	message("MinGW flags")
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
	src/CommonConstants.h)

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
if(MSVC)
	add_executable(${EXECUTABLE_NAME} WIN32 ${ALL_SOURCES})
else()
	add_executable(${EXECUTABLE_NAME} ${ALL_SOURCES})
endif()

# Конкретные пути к бинарнику
set_target_properties(${EXECUTABLE_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
)

# Библиотеки, с которыми будем ликовать
target_link_libraries(${EXECUTABLE_NAME} ${LIBS_LINK_NAMES})

# Создаем символьную ссылку на ресурсы (Для исполняемго файла, после выбора)
if (WIN32)
    string(REGEX REPLACE "/" "\\\\" SOURCE_RES_PATH "${CMAKE_SOURCE_DIR}/res")
    string(REGEX REPLACE "/" "\\\\" RESULT_RES_PATH "${CMAKE_BINARY_DIR}/res/")
    message("COPY RES COMMAND = xcopy ${SOURCE_RES_PATH} ${RESULT_RES_PATH} /y")
    add_custom_command (
        TARGET ${EXECUTABLE_NAME}
        POST_BUILD
        COMMAND xcopy ${SOURCE_RES_PATH} ${RESULT_RES_PATH} /y
    )
else()
    add_custom_command (
        TARGET ${EXECUTABLE_NAME}
        POST_BUILD
        COMMAND rm -f ${CMAKE_BINARY_DIR}/res
        COMMAND ln -s ../res ${CMAKE_BINARY_DIR}/res
    )
endif()
