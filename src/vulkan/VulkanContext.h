#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

namespace edge::vulkan {

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    // Vulkan 리소스의 중복 소유 방지
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    // 다른 내부 모듈에서 사용할 장치 정보
    VkPhysicalDevice physicalDevice() const {
        return physicalDevice_;
    }

    VkDevice device() const {
        return device_;
    }

    VkQueue computeQueue() const {
        return computeQueue_;
    }

    uint32_t queueFamilyIndex() const {
        return queueFamilyIndex_;
    }

    const std::string& deviceName() const {
        return deviceName_;
    }

private:
    void createInstance();
    void selectPhysicalDevice();
    void createLogicalDevice();
    void cleanup() noexcept;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue computeQueue_ = VK_NULL_HANDLE;

    uint32_t queueFamilyIndex_ = 0;
    std::string deviceName_;
};

} // namespace edge::vulkan