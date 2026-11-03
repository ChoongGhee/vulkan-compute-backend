cmake_minimum_required(VERSION 3.20)

project(VulkanComputeBackend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

# Vulkan 기능 라이브러리
add_library(edge_vulkan STATIC
    src/vulkan/VulkanContext.cpp
    src/vulkan/VulkanBuffer.cpp
)

target_include_directories(edge_vulkan PRIVATE
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

target_include_directories(vulkan_probe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(vulkan_probe PRIVATE
    edge_vulkan
)

# GLSL 컴파일러 검색
find_program(GLSLANG_VALIDATOR
    NAMES glslangValidator
    REQUIRED
)

set(NORMALIZE_SHADER
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/normalize.comp"
)

set(NORMALIZE_SPV
    "${CMAKE_CURRENT_BINARY_DIR}/shaders/normalize.comp.spv"
)

# GLSL → SPIR-V 변환
add_custom_command(
    OUTPUT "${NORMALIZE_SPV}"

    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/shaders"

    COMMAND "${GLSLANG_VALIDATOR}"
        -V
        --target-env vulkan1.2
        -S comp
        -o "${NORMALIZE_SPV}"
        "${NORMALIZE_SHADER}"

    DEPENDS "${NORMALIZE_SHADER}"

    COMMENT "Compiling normalize compute shader"
    VERBATIM
)

# Shader 빌드 타깃
add_custom_target(normalize_shader
    DEPENDS "${NORMALIZE_SPV}"
)

# vulkan_probe 빌드 전에 Shader 컴파일
add_dependencies(vulkan_probe normalize_shader)