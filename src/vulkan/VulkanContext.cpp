
#include "vulkan/VulkanContext.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace edge::vulkan {

namespace {

// 이 파일 내부에서만 사용하는 오류 검사 함수
void check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            std::string(operation) + " failed: " +
            std::to_string(result)
        );
    }
}

} // namespace

// 생성자: Vulkan 초기화
VulkanContext::VulkanContext() {
    try {
        createInstance();
        selectPhysicalDevice();
        createLogicalDevice();
    } catch (...) {
        cleanup();
        throw;
    }
}

// 소멸자: Vulkan 리소스 해제
VulkanContext::~VulkanContext() {
    cleanup();
}

// 1. Vulkan Instance 생성
void VulkanContext::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Edge Preprocessing";
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType =
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    check(
        vkCreateInstance(
            &createInfo, nullptr, &instance_
        ),
        "vkCreateInstance"
    );
}

// 2. 실제 V3DV GPU와 Compute Queue Family 선택
void VulkanContext::selectPhysicalDevice() {
    uint32_t deviceCount = 0;

    check(
        vkEnumeratePhysicalDevices(
            instance_, &deviceCount, nullptr
        ),
        "vkEnumeratePhysicalDevices(count)"
    );

    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan devices found");
    }

    std::vector<VkPhysicalDevice> gpus(deviceCount);

    check(
        vkEnumeratePhysicalDevices(
            instance_, &deviceCount, gpus.data()
        ),
        "vkEnumeratePhysicalDevices(list)"
    );

    for (VkPhysicalDevice gpu : gpus) {
        VkPhysicalDeviceDriverProperties driver{};
        driver.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

        VkPhysicalDeviceProperties2 properties{};
        properties.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        properties.pNext = &driver;

        vkGetPhysicalDeviceProperties2(gpu, &properties);

        // CPU 기반 llvmpipe를 선택하지 않도록 검사
        if (driver.driverID != VK_DRIVER_ID_MESA_V3DV) {
            continue;
        }

        uint32_t queueCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(
            gpu, &queueCount, nullptr
        );

        std::vector<VkQueueFamilyProperties> queues(
            queueCount
        );

        if (queueCount > 0) {
            vkGetPhysicalDeviceQueueFamilyProperties(
                gpu, &queueCount, queues.data()
            );
        }

        for (uint32_t i = 0; i < queueCount; ++i) {
            bool supportsCompute =
                (queues[i].queueFlags &
                 VK_QUEUE_COMPUTE_BIT) != 0;

            if (supportsCompute && queues[i].queueCount > 0) {
                physicalDevice_ = gpu;
                queueFamilyIndex_ = i;
                deviceName_ = properties.properties.deviceName;
                return;
            }
        }
    }

    throw std::runtime_error(
        "No V3DV GPU with Compute Queue found"
    );
}

// 3. Logical Device 및 Compute Queue 생성
void VulkanContext::createLogicalDevice() {
    float priority = 1.0f;

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType =
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamilyIndex_;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType =
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;

    check(
        vkCreateDevice(
            physicalDevice_, &createInfo,
            nullptr, &device_
        ),
        "vkCreateDevice"
    );

    vkGetDeviceQueue(
        device_, queueFamilyIndex_, 0, &computeQueue_
    );

    if (computeQueue_ == VK_NULL_HANDLE) {
        throw std::runtime_error("Compute Queue is null");
    }
}

// 4. 리소스 해제
void VulkanContext::cleanup() noexcept {
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
        computeQueue_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
        physicalDevice_ = VK_NULL_HANDLE;
    }
}

} // namespace edge::vulkan
