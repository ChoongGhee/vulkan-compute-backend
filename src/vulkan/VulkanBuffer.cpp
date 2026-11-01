
#include "vulkan/VulkanBuffer.h"
#include "vulkan/VulkanContext.h"

#include <cstring>
#include <stdexcept>
#include <string>

namespace edge::vulkan {

namespace {

// Vulkan API 결과 확인
void check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            std::string(operation) + " failed: " +
            std::to_string(result)
        );
    }
}

// Buffer 요구사항에 맞는 메모리 타입 검색
uint32_t findMemoryType(
    VkPhysicalDevice gpu,
    uint32_t memoryTypeBits,
    VkMemoryPropertyFlags requiredFlags
) {
    VkPhysicalDeviceMemoryProperties properties{};

    vkGetPhysicalDeviceMemoryProperties(gpu, &properties);

    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        bool allowed =
            (memoryTypeBits & (1u << i)) != 0;

        bool supported =
            (properties.memoryTypes[i].propertyFlags &
             requiredFlags) == requiredFlags;

        if (allowed && supported) {
            return i;
        }
    }

    throw std::runtime_error("Suitable memory type not found");
}

} // namespace

// Buffer 및 Device Memory 생성
VulkanBuffer::VulkanBuffer(
    VulkanContext& context,
    VkDeviceSize size,
    VkBufferUsageFlags usage
)
    : context_(context), size_(size) {

    if (size_ == 0 || usage == 0) {
        throw std::invalid_argument(
            "Buffer size and usage must be nonzero"
        );
    }

    try {
        // 1. Buffer 생성
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size_;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        check(
            vkCreateBuffer(
                context_.device(), &bufferInfo,
                nullptr, &buffer_
            ),
            "vkCreateBuffer"
        );

        // 2. 메모리 요구사항 조회
        VkMemoryRequirements requirements{};

        vkGetBufferMemoryRequirements(
            context_.device(),
            buffer_,
            &requirements
        );

        allocationSize_ = requirements.size;
        alignment_ = requirements.alignment;

        // 3. CPU 접근 가능한 메모리 타입 선택
        memoryTypeIndex_ = findMemoryType(
            context_.physicalDevice(),
            requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        // 4. 메모리 할당
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType =
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = allocationSize_;
        allocInfo.memoryTypeIndex = memoryTypeIndex_;

        check(
            vkAllocateMemory(
                context_.device(), &allocInfo,
                nullptr, &memory_
            ),
            "vkAllocateMemory"
        );

        // 5. Buffer와 메모리 연결
        check(
            vkBindBufferMemory(
                context_.device(), buffer_, memory_, 0
            ),
            "vkBindBufferMemory"
        );

    } catch (...) {
        cleanup();
        throw;
    }
}

// CPU 데이터를 Buffer 메모리에 기록
void VulkanBuffer::upload(
    const void* data,
    VkDeviceSize size
) {
    if (data == nullptr || size == 0 || size > size_) {
        throw std::invalid_argument("Invalid upload range");
    }

    void* mapped = nullptr;

    check(
        vkMapMemory(
            context_.device(), memory_,
            0, size, 0, &mapped
        ),
        "vkMapMemory(upload)"
    );

    std::memcpy(mapped, data, static_cast<size_t>(size));

    vkUnmapMemory(context_.device(), memory_);
}

// Buffer 메모리를 CPU로 읽기
void VulkanBuffer::download(
    void* data,
    VkDeviceSize size
) const {
    if (data == nullptr || size == 0 || size > size_) {
        throw std::invalid_argument("Invalid download range");
    }

    void* mapped = nullptr;

    check(
        vkMapMemory(
            context_.device(), memory_,
            0, size, 0, &mapped
        ),
        "vkMapMemory(download)"
    );

    std::memcpy(data, mapped, static_cast<size_t>(size));

    vkUnmapMemory(context_.device(), memory_);
}

// 리소스 해제
void VulkanBuffer::cleanup() noexcept {
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(
            context_.device(), buffer_, nullptr
        );
        buffer_ = VK_NULL_HANDLE;
    }

    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(
            context_.device(), memory_, nullptr
        );
        memory_ = VK_NULL_HANDLE;
    }
}

VulkanBuffer::~VulkanBuffer() {
    cleanup();
}

} // namespace edge::vulkan
