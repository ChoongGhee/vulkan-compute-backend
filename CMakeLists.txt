cmake_minimum_required(VERSION 3.20)

project(VulkanComputeBackend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

# Vulkan 기능 라이브러리
add_library(edge_vulkan STATIC
    src/vulkan/VulkanContext.cpp
)

target_include_directories(edge_vulkan PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(edge_vulkan PUBLIC
    Vulkan::Vulkan
)

# 실행 프로그램
add_executable(vulkan_probe
    src/main.cpp
    src/probe/VulkanProbe.cpp
)

target_link_libraries(vulkan_probe PRIVATE
    edge_vulkan
)