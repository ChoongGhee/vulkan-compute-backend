#include "probe/VulkanProbe.h"

#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanBuffer.h"
#include "vulkan/VulkanCompute.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace edge::probe {

namespace {

constexpr uint32_t kLocalSizeX = 64;
constexpr size_t kGuardElements = 64;
constexpr float kGuardValue = 999.0f;
constexpr float kTolerance = 1e-5f;

// 하나의 데이터 크기에 대해 GPU Normalize를 검증
void verifyNormalize(
    vulkan::VulkanContext& context,
    vulkan::VulkanCompute& compute,
    uint32_t elementCount
) {
    // 실제 데이터 뒤에 64개의 Guard 원소 추가
    const size_t totalCount =
        static_cast<size_t>(elementCount) + kGuardElements;

    std::vector<float> input(totalCount, kGuardValue);
    std::vector<float> output(totalCount, 0.0f);

    // 0~255 범위의 다양한 입력값 생성
    for (uint32_t i = 0; i < elementCount; ++i) {
        input[i] = static_cast<float>(
            (i * 17u + 19u) % 256u
        );
    }

    // 마지막 실제 원소도 반드시 검증되도록 설정
    input[elementCount - 1] = 255.0f;

    const VkDeviceSize bufferBytes =
        static_cast<VkDeviceSize>(totalCount) * sizeof(float);

    vulkan::VulkanBuffer buffer(
        context,
        bufferBytes,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    );

    // CPU → Vulkan 메모리
    buffer.upload(input.data(), bufferBytes);

    // GPU에서 elementCount개만 정규화
    compute.dispatch(buffer, elementCount);

    // Vulkan 메모리 → CPU
    buffer.download(output.data(), bufferBytes);

    // 1. 실제 데이터가 정확히 정규화됐는지 확인
    for (uint32_t i = 0; i < elementCount; ++i) {
        const float expected = input[i] / 255.0f;
        const float actual = output[i];

        if (!std::isfinite(actual) ||
            std::abs(actual - expected) > kTolerance) {
            throw std::runtime_error(
                "Normalize mismatch at index " +
                std::to_string(i) +
                " (count=" + std::to_string(elementCount) + ")"
            );
        }
    }

    // 2. 범위 밖 Guard 데이터가 변경되지 않았는지 확인
    for (size_t i = elementCount; i < totalCount; ++i) {
        if (output[i] != kGuardValue) {
            throw std::runtime_error(
                "Guard data modified at index " +
                std::to_string(i)
            );
        }
    }

    const uint32_t groupCount =
        1 + (elementCount - 1) / kLocalSizeX;

    std::cout
        << "[PASS] Elements=" << elementCount
        << " | Workgroups=" << groupCount
        << " | Guard=OK\n";
}

} // namespace

int runVulkanProbe() {
    try {
        vulkan::VulkanContext context;

        std::cout << "[OK] Vulkan initialized\n";
        std::cout << "GPU: " << context.deviceName() << '\n';

        // Pipeline은 한 번만 생성하고 재사용
        vulkan::VulkanCompute compute(
            context,
            "build/shaders/normalize.comp.spv"
        );

        std::cout << "[OK] Compute Pipeline created\n";

        const std::array<uint32_t, 11> testSizes = {
            1, 4, 63, 64, 65,
            127, 128, 129,
            1024, 65536, 1048576
        };

        for (uint32_t count : testSizes) {
            verifyNormalize(context, compute, count);
        }

        std::cout
            << "[OK] All compute boundary tests passed\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }
}

} // namespcae edge::probe