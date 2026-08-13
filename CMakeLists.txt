# Project Details
cmake_minimum_required(VERSION 4.1.2)

project(MarsEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ensure all build objects are placed in the same directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_CXX_SCAN_FOR_MODULES Off)

# Add libraries to build
set(SDL_STATIC False)
set(SDL_SHARED True)
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendored/glm EXCLUDE_FROM_ALL)

option(MARS_DEBUG_ALL "Have all systems output debug info" false)
option(MARS_DEBUG_INPUT "Have input systems output debug info" false)

if(NOT DEFINED ${MARS_ECS_MAX_ENTITIES})
    set(MARS_ECS_MAX_ENTITIES 512)
endif()
if(NOT DEFINED ${MARS_RENDERER_MAX_MESHES})
    set(MARS_RENDERER_MAX_MESHES 128)
endif()
if(NOT DEFINED ${MARS_RENDERER_MAX_TEXTURES})
    set(MARS_RENDERER_MAX_TEXTURES 256)
endif()

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
add_custom_command(OUTPUT sprite.spv
    COMMAND slangc "${MARS_SHADER_PATH}/sprite.slang"
        -target spirv
        -profile spirv_1_4
        -emit-spirv-directly
        -fvk-use-entrypoint-name
        -entry vertMain
        -entry fragMain
        -o sprite.spv
    DEPENDS "${MARS_SHADER_PATH}/sprite.slang"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>"
    COMMENT "Compiling sprite shader"
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
    MARS_SOURCE_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
target_sources(Mars PRIVATE "${MARS_SOURCE_FILES}" "shader2d.spv" "shader3d.spv" "sprite.spv")

target_compile_definitions(Mars PUBLIC 
    MARS_SHADERS_PATH="${CMAKE_BINARY_DIR}/$<CONFIGURATION>/"
	MARS_ECS_MAX_ENTITIES=${MARS_ECS_MAX_ENTITIES}
	MARS_RENDERER_MAX_MESHES=${MARS_RENDERER_MAX_MESHES}
	MARS_RENDERER_MAX_TEXTURES=${MARS_RENDERER_MAX_TEXTURES}
)

macro(mars_pass_debug_flag debug_flag)
    if(${debug_flag})
        target_compile_definitions(Mars PUBLIC ${debug_flag}=${${debug_flag}})
    endif()
endmacro()

mars_pass_debug_flag(MARS_DEBUG_ALL)
mars_pass_debug_flag(MARS_DEBUG_INPUT)

# Link libraries to mars
target_link_libraries(Mars PRIVATE SDL3::SDL3 glm::glm)

if(WIN32)
    target_link_libraries(Mars PRIVATE vulkan-1)
else()
    target_link_libraries(Mars PRIVATE vulkan)
endif()

target_include_directories(Mars PUBLIC 
    "${PROJECT_SOURCE_DIR}/vendored/SDL/include" 
    "${PROJECT_SOURCE_DIR}/vendored/glm"
    "${PROJECT_SOURCE_DIR}/thirdparty/stb"
    "$ENV{VULKAN_SDK}/include"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/src/renderer"
    "${PROJECT_SOURCE_DIR}/src/ecs"
)

# Build for Debug Symbols
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")
        target_compile_options(Mars PUBLIC -g)
    endif()
endif()
