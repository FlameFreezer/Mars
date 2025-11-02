# Project Details
cmake_minimum_required(VERSION 4.1.2)

project(MarsEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set up mars target library
add_library(Mars)
target_sources(Mars PUBLIC
	FILE_SET CXX_MODULES
	FILES src/mars.cppm)

# Set up Vulkan module
find_package(Vulkan REQUIRED)
add_library(VulkanCppModule)
add_library(Vulkan::cppm ALIAS VulkanCppModule)

set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/include")

target_include_directories(VulkanCppModule PRIVATE "${VULKAN_INCLUDE_DIR}")
target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)
set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)
target_sources(VulkanCppModule
	PUBLIC
	FILE_SET cxx_modules TYPE CXX_MODULES
	BASE_DIRS "${VULKAN_INCLUDE_DIR}"
	FILES "${VULKAN_INCLUDE_DIR}/vulkan/vulkan.cppm")

# Set up SDL3
add_subdirectory(include/SDL EXCLUDE_FROM_ALL)

# Link libraries to mars
target_link_libraries(Mars Vulkan::cppm SDL3::SDL3)
target_include_directories(Mars 
	PRIVATE include/SDL/include)

# Test Application
add_executable(marsTest app/main.cpp)
target_link_libraries(marsTest PRIVATE Mars)
