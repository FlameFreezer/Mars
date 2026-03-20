# Project Details
cmake_minimum_required(VERSION 4.1.2)

project(MarsEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ensure all build objects are placed in the same directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Add libraries to build
set(SDL_STATIC True)
set(SDL_SHARED False)
add_subdirectory(libraries/SDL EXCLUDE_FROM_ALL)
add_subdirectory(libraries/glm EXCLUDE_FROM_ALL)

# Shader Compilation
set(MARS_SHADER_PATH "${PROJECT_SOURCE_DIR}/shaders")
add_custom_command(OUTPUT shader2d.spv
    COMMAND slangc "${MARS_SHADER_PATH}/shader2d.slang" 
        -target spirv 
        -profile spirv_1_4 
        -emit-spirv-directly 
        -fvk-use-entrypoint-name 
        -entry vertMain 
        -entry fragMain 
        -o shader2d.spv
    DEPENDS "${MARS_SHADER_PATH}/shader2d.slang"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>"
    COMMENT "Compiling 2D shaders"
    CODEGEN
    VERBATIM
)
add_custom_command(OUTPUT shader3d.spv
    COMMAND slangc "${MARS_SHADER_PATH}/shader3d.slang" 
        -target spirv 
        -profile spirv_1_4 
        -emit-spirv-directly 
        -fvk-use-entrypoint-name 
        -entry vertMain 
        -entry fragMain 
        -o shader3d.spv
    DEPENDS "${MARS_SHADER_PATH}/shader3d.slang"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>"
    COMMENT "Compiling 3D shaders"
    CODEGEN
    VERBATIM
)

# Set compiler flags for Mars library (these cause issues when building SDL3 
#   so I have to set them after it it built)
set(CMAKE_COMPILE_WARNING_AS_ERROR True)
set(CMAKE_LINK_WARNING_AS_ERROR True)

# Set up mars target library
add_library(Mars)
file(GLOB_RECURSE 
    MARS_MODULE_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cppm")
file(GLOB_RECURSE 
    MARS_SOURCE_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
target_sources(Mars 
    PUBLIC 
    FILE_SET mars_module 
    TYPE CXX_MODULES 
    BASE_DIRS src
    FILES "${MARS_MODULE_FILES}"   
    PRIVATE "${MARS_SOURCE_FILES}" "shader2d.spv" "shader3d.spv"
)

# Link libraries to mars
target_link_libraries(Mars PRIVATE SDL3::SDL3 glm::glm)

if(WIN32)
    target_link_libraries(Mars PRIVATE vulkan-1)
else()
    target_link_libraries(Mars PRIVATE vulkan)
endif()

if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "Environment variable \"VULKAN_SDK\" not defined! Set this to the path to the Vulkan SDK installed on your system.")
endif()

target_include_directories(Mars PUBLIC 
    "${PROJECT_SOURCE_DIR}/libraries/SDL/include" 
    "${PROJECT_SOURCE_DIR}/libraries/glm"
    "${PROJECT_SOURCE_DIR}/libraries/stb"
    "$ENV{VULKAN_SDK}/include" 
    "${PROJECT_SOURCE_DIR}/include"
)
target_link_directories(Mars INTERFACE
    "$ENV{VULKAN_SDK}/lib"
)

# Build for Debug Symbols
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")
        target_compile_options(Mars PUBLIC -g)
    endif()
endif()
