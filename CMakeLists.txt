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

# Shader Code Compilation
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
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Compiling shaders..."
    CODEGEN
    VERBATIM
)
add_custom_target(shaders DEPENDS slang.spv)

# Set up mars target library
add_library(Mars)
add_dependencies(Mars shaders)
target_sources(Mars PUBLIC 
    FILE_SET mars_module 
    TYPE CXX_MODULES 
    FILES src/mars.cppm src/error.cppm src/renderer.cppm
    PRIVATE src/mars.cpp
)

# Set up SDL3
add_subdirectory(libraries/SDL EXCLUDE_FROM_ALL)

# Set up GLM
add_subdirectory(libraries/GLM EXCLUDE_FROM_ALL)

# Link libraries to mars
target_link_libraries(Mars PRIVATE SDL3::SDL3 vulkan glm::glm)

target_include_directories(Mars PUBLIC 
    "${CMAKE_SOURCE_DIR}/libraries/SDL/include" 
    "${CMAKE_SOURCE_DIR}/libraries/GLM"
    "$ENV{VULKAN_SDK}/include" 
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
)

target_compile_options(Mars PUBLIC -g)
target_compile_options(marsTest PUBLIC -g)
