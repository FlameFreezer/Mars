cmake_minimum_required(VERSION 4.1.2)

project(MarsEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(mars)
target_sources(mars PUBLIC
	FILE_SET CXX_MODULES
	FILES src/mars.cppm)
add_executable(marsTest app/main.cpp)
target_link_libraries(marsTest PRIVATE mars)
