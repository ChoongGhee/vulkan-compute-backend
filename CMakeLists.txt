cmake_minimum_required(VERSION 3.20)

project(EdgeImagePreprocessingBackend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

add_executable(vulkan_probe
    src/main.cpp
)

target_link_libraries(vulkan_probe
    PRIVATE Vulkan::Vulkan
)
