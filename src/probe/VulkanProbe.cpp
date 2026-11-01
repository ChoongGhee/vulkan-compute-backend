#include "probe/VulkanProbe.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanBuffer.h"

#include <array>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace edge::probe {

int runVulkanProbe() {
    try {
        vulkan::VulkanContext context;

        std::cout << "[OK] Vulkan initialized\n";
        std::cout << "GPU: " << context.deviceName() << '\n';
        std::cout << "Queue Family: "
                  << context.queueFamilyIndex() << '\n';
        std::cout << "[OK] Compute Queue acquired\n";

        // 1. 테스트용 CPU 데이터
        std::array<float, 4> input = {
            0.0f, 64.0f, 128.0f, 255.0f
        };

        std::array<float, 4> output{};

        // 2. Vulkan Storage Buffer 생성
        vulkan::VulkanBuffer buffer(
            context,
            sizeof(input),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        );

        std::cout << "Buffer Size: "
                  << buffer.size() << '\n';
        std::cout << "Allocation Size: "
                  << buffer.allocationSize() << '\n';
        std::cout << "Alignment: "
                  << buffer.alignment() << '\n';
        std::cout << "Memory Type Index: "
                  << buffer.memoryTypeIndex() << '\n';

        // 3. Buffer에 데이터 기록
        buffer.upload(input.data(), sizeof(input));

        // 4. Buffer에서 데이터 읽기
        buffer.download(output.data(), sizeof(output));

        // 5. 결과 검증
        if (input != output) {
            throw std::runtime_error(
                "Buffer data mismatch"
            );
        }

        for (size_t i = 0; i < output.size(); ++i) {
            std::cout << "Buffer[" << i << "] = "
                      << output[i] << '\n';
        }

        std::cout << "[OK] Buffer write/read verified\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }
}

}