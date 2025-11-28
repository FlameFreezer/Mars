# Project Details
cmake_minimum_required(VERSION 4.1.2)

project(MarsEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#set(CMAKE_COMPILE_WARNING_AS_ERROR True)
#set(CMAKE_LINK_WARNING_AS_ERROR True)

# Ensure all build objects are placed in the same directory
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Set up mars target library
add_library(Mars)
target_sources(Mars PUBLIC src/mars.cpp)

# Set up SDL3
add_subdirectory(libraries/SDL EXCLUDE_FROM_ALL)

# Link libraries to mars
target_link_libraries(Mars PRIVATE SDL3::SDL3 vulkan)
target_include_directories(Mars PUBLIC 
    "${CMAKE_SOURCE_DIR}/libraries/SDL/include" 
    "$ENV{VULKAN_SDK}/include" 
    "${CMAKE_SOURCE_DIR}/include"
)
target_link_directories(Mars PUBLIC
    "$ENV{VULKAN_SDK}/lib"
)

# Test Application
add_executable(marsTest app/main.cpp)
target_include_directories(marsTest PUBLIC include)
target_link_libraries(marsTest PRIVATE Mars)
