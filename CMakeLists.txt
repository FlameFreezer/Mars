# Project Details
cmake_minimum_required(VERSION 4.1.2)

project(MarsEngine)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

# Set up mars target library
add_library(Mars)
target_sources(Mars PRIVATE src/mars.c)

# Set up SDL3
#add_subdirectory(libraries/SDL EXCLUDE_FROM_ALL)
add_subdirectory(libraries/SDL)
set(SDL_SHARED ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

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
add_executable(marsTest app/main.c)
target_include_directories(marsTest PUBLIC include)
target_link_libraries(marsTest PRIVATE Mars)
