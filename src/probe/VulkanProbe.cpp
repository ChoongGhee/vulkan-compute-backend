#include "probe/VulkanProbe.h"
#include "vulkan/VulkanContext.h"

#include <exception>
#include <iostream>

namespace edge::probe {

int runVulkanProbe() {
    try {
        vulkan::VulkanContext context;

        std::cout << "[OK] Vulkan initialized\n";
        std::cout << "GPU: " << context.deviceName() << '\n';
        std::cout << "Queue Family: "
                  << context.queueFamilyIndex() << '\n';
        std::cout << "[OK] Compute Queue acquired\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }
}

}