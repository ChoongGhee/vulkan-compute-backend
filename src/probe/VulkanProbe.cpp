#include "probe/VulkanProbe.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanBuffer.h"
#include "vulkan/VulkanCompute.h"

#include <array>
#include <cmath>
#include <cstdint>
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
        std::cout << "[OK] Buffer uploaded\n";

        // 4. SPIR-V Shader와 Compute Pipeline 생성
        vulkan::VulkanCompute compute(
            context,
            "build/shaders/normalize.comp.spv"
        );

        std::cout << "[OK] Compute Pipeline created\n";

        // 5. GPU에서 Normalize 실행
        compute.dispatch(
            buffer,
            static_cast<uint32_t>(input.size())
        );

        // 6. GPU 결과를 CPU로 가져오기
        buffer.download(output.data(), sizeof(output));

        // 7. CPU 기준값과 비교
        for (size_t i = 0; i < input.size(); ++i) {
            const float expected = input[i] / 255.0f;
            const float actual = output[i];

            std::cout << "GPU[" << i << "] = "
                      << actual
                      << " | Expected = " << expected << '\n';

            if (std::abs(actual - expected) > 1e-5f) {
                throw std::runtime_error(
                    "GPU normalize result mismatch"
                );
            }
        }

        std::cout << "[OK] GPU normalize verified\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }
}

} // namespcae edge::probe