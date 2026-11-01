#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace edge::vulkan {

class VulkanContext;

class VulkanBuffer {
public:
    VulkanBuffer(
        VulkanContext& context,
        VkDeviceSize size,
        VkBufferUsageFlags usage
    );

    ~VulkanBuffer();

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    void upload(const void* data, VkDeviceSize size);
    void download(void* data, VkDeviceSize size) const;

    VkBuffer handle() const noexcept {
        return buffer_;
    }

    VkDeviceSize size() const noexcept {
        return size_;
    }

    VkDeviceSize allocationSize() const noexcept {
        return allocationSize_;
    }

    VkDeviceSize alignment() const noexcept {
        return alignment_;
    }

    uint32_t memoryTypeIndex() const noexcept {
        return memoryTypeIndex_;
    }

private:
    void cleanup() noexcept;

    VulkanContext& context_;

    VkBuffer buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;

    VkDeviceSize size_;
    VkDeviceSize allocationSize_ = 0;
    VkDeviceSize alignment_ = 0;
    uint32_t memoryTypeIndex_ = 0;
};

} // namespace edge::vulkan