#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

namespace edge::vulkan {

class VulkanContext;
class VulkanBuffer;

class VulkanCompute {
public:
    VulkanCompute(
        VulkanContext& context,
        const std::string& spirvPath
    );

    ~VulkanCompute();

    VulkanCompute(const VulkanCompute&) = delete;
    VulkanCompute& operator=(const VulkanCompute&) = delete;

    // Buffer에 담긴 float 원소들을 GPU에서 정규화
    void dispatch(
        VulkanBuffer& buffer,
        uint32_t elementCount
    );

private:
    void createPipeline(const std::string& spirvPath);
    void createDescriptorResources();
    void createCommandResources();
    void cleanup() noexcept;

    VulkanContext& context_;

    // Shader 및 Pipeline
    VkShaderModule shaderModule_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;

    // Buffer 연결
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;

    // GPU 명령 제출 및 완료 확인
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;
};

} // namespace edge::vulkan