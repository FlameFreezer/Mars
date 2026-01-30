# Project Details
cmake_minimum_required(VERSION 4.1.2)

project(MarsEngine)

set(CMAKE_CXX_STANDARD 23)
#set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ensure all build objects are placed in the same directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Add libraries to build
set(SDL_STATIC True)
set(SDL_SHARED False)
add_subdirectory(libraries/SDL EXCLUDE_FROM_ALL)
add_subdirectory(libraries/glm EXCLUDE_FROM_ALL)

# Shader Compilation
set(MARS_SHADER_PATH "${CMAKE_SOURCE_DIR}/shaders")
add_custom_command(OUTPUT slang.spv
    COMMAND slangc "${MARS_SHADER_PATH}/shader.slang" 
        -target spirv 
        -profile spirv_1_4 
        -emit-spirv-directly 
        -fvk-use-entrypoint-name 
        -entry vertMain 
        -entry fragMain 
        -o slang.spv
    DEPENDS "${MARS_SHADER_PATH}/shader.slang"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>"
    COMMENT "Compiling shaders"
    CODEGEN
    VERBATIM
)
add_custom_target(marsShaders DEPENDS slang.spv)

# Set compiler flags for Mars library (these cause issues when building SDL3 
#   so I have to set them after it it built)
set(CMAKE_COMPILE_WARNING_AS_ERROR True)
set(CMAKE_LINK_WARNING_AS_ERROR True)

# Set up mars target library
add_library(Mars)
add_dependencies(Mars marsShaders)
file(GLOB_RECURSE MARS_MODULE_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cppm")
file(GLOB_RECURSE MARS_SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
target_sources(Mars PUBLIC 
    FILE_SET mars_module 
    TYPE CXX_MODULES 
    BASE_DIRS src
    FILES "${MARS_MODULE_FILES}"    
    PRIVATE "${MARS_SOURCE_FILES}"
)
target_compile_definitions(Mars PUBLIC MARS_ASSETS_PATH="${CMAKE_SOURCE_DIR}/assets/")

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
    "${CMAKE_SOURCE_DIR}/libraries/SDL/include" 
    "${CMAKE_SOURCE_DIR}/libraries/GLM"
    "$ENV{VULKAN_SDK}/include" 
    "${CMAKE_SOURCE_DIR}/libraries"
)
target_link_directories(Mars INTERFACE
    "$ENV{VULKAN_SDK}/lib"
)

# Test Application
add_executable(marsTest app/main.cpp)
target_link_libraries(marsTest PRIVATE Mars SDL3::SDL3 glm::glm)
target_include_directories(marsTest PUBLIC
    "${CMAKE_SOURCE_DIR}/libraries/SDL/include"
    "${CMAKE_SOURCE_DIR}/libraries/GLM"
    "${CMAKE_SOURCE_DIR}/src"
)

# Build for Debug Symbols
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")
        add_compile_options(-g)
    endif()
endif()
