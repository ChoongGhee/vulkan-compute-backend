#include <vulkan/vulkan.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main() {
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    bool success = false;

    try {
        // 1. Vulkan API 오류 확인
        auto check = [](VkResult result, const char* operation) {
            if (result != VK_SUCCESS) {
                throw std::runtime_error(
                    std::string(operation) + " failed: " +
                    std::to_string(result)
                );
            }
        };

        // 2. Vulkan Instance 생성
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Edge Preprocessing";
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo instanceInfo{};
        instanceInfo.sType =
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo = &appInfo;

        check(vkCreateInstance(
            &instanceInfo, nullptr, &instance
        ), "vkCreateInstance");

        std::cout << "[OK] Vulkan Instance created\n";

        // 3. 사용 가능한 Physical Device 조회
        uint32_t deviceCount = 0;

        check(vkEnumeratePhysicalDevices(
            instance, &deviceCount, nullptr
        ), "vkEnumeratePhysicalDevices(count)");

        std::vector<VkPhysicalDevice> physicalDevices(
            deviceCount
        );

        if (deviceCount > 0) {
            check(vkEnumeratePhysicalDevices(
                instance, &deviceCount, physicalDevices.data()
            ), "vkEnumeratePhysicalDevices(list)");
        }

        std::cout << "Devices found: " << deviceCount << "\n";

        // 4. V3DV와 Compute Queue가 있는 장치 선택
        VkPhysicalDevice selectedGpu = VK_NULL_HANDLE;
        uint32_t selectedQueueFamily = 0;

        for (VkPhysicalDevice gpu : physicalDevices) {
            VkPhysicalDeviceDriverProperties driver{};
            driver.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

            VkPhysicalDeviceProperties2 properties{};
            properties.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            properties.pNext = &driver;

            vkGetPhysicalDeviceProperties2(gpu, &properties);

            std::cout << "\nGPU: "
                      << properties.properties.deviceName
                      << "\nDriver: " << driver.driverName
                      << "\n";

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(
                gpu, &queueFamilyCount, nullptr
            );

            std::vector<VkQueueFamilyProperties> queueFamilies(
                queueFamilyCount
            );

            if (queueFamilyCount > 0) {
                vkGetPhysicalDeviceQueueFamilyProperties(
                    gpu, &queueFamilyCount, queueFamilies.data()
                );
            }

            for (uint32_t i = 0; i < queueFamilyCount; ++i) {
                const auto& queue = queueFamilies[i];

                bool supportsCompute =
                    (queue.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;

                std::cout << "  Queue Family " << i
                          << " | Count: " << queue.queueCount
                          << " | Compute: "
                          << (supportsCompute ? "YES" : "NO")
                          << "\n";

                if (driver.driverID == VK_DRIVER_ID_MESA_V3DV &&
                    supportsCompute &&
                    queue.queueCount > 0) {
                    selectedGpu = gpu;
                    selectedQueueFamily = i;
                }
            }
        }

        if (selectedGpu == VK_NULL_HANDLE) {
            throw std::runtime_error(
                "No V3DV GPU with Compute Queue found"
            );
        }

        // 5. Logical Device 및 Queue 생성
        float priority = 1.0f;

        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = selectedQueueFamily;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;

        check(vkCreateDevice(
            selectedGpu, &deviceInfo, nullptr, &device
        ), "vkCreateDevice");

        VkQueue computeQueue = VK_NULL_HANDLE;

        vkGetDeviceQueue(
            device, selectedQueueFamily, 0, &computeQueue
        );

        if (computeQueue == VK_NULL_HANDLE) {
            throw std::runtime_error("Compute Queue is null");
        }

        std::cout << "\n[OK] V3DV selected"
                  << "\n[OK] Logical Device created"
                  << "\n[OK] Compute Queue acquired"
                  << "\nQueue Family Index: "
                  << selectedQueueFamily << "\n";

        success = true;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
    }

    // 6. 생성한 Vulkan 객체 해제
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
    }

    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
    }

    return success ? 0 : 1;
}
