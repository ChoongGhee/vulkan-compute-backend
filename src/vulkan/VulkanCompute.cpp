
#include "vulkan/VulkanCompute.h"

#include "vulkan/VulkanBuffer.h"
#include "vulkan/VulkanContext.h"

#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace edge::vulkan {

namespace {

constexpr uint32_t kLocalSizeX = 64;

void check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            std::string(operation) + " failed: " +
            std::to_string(result)
        );
    }
}

std::vector<uint32_t> loadSpirv(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file) {
        throw std::runtime_error(
            "Cannot open SPIR-V file: " + path
        );
    }

    const std::streamoff byteCount = file.tellg();

    if (byteCount <= 0 || byteCount % 4 != 0) {
        throw std::runtime_error("Invalid SPIR-V file size");
    }

    std::vector<uint32_t> code(
        static_cast<size_t>(byteCount) / sizeof(uint32_t)
    );

    file.seekg(0);
    file.read(
        reinterpret_cast<char*>(code.data()),
        static_cast<std::streamsize>(byteCount)
    );

    if (!file) {
        throw std::runtime_error("Failed to read SPIR-V");
    }

    return code;
}

} // namespace

VulkanCompute::VulkanCompute(
    VulkanContext& context,
    const std::string& spirvPath
) : context_(context) {
    try {
        createPipeline(spirvPath);
        createDescriptorResources();
        createCommandResources();
    } catch (...) {
        cleanup();
        throw;
    }
}

VulkanCompute::~VulkanCompute() {
    cleanup();
}

// 1. SPIR-V를 로딩하고 Compute Pipeline 생성
void VulkanCompute::createPipeline(
    const std::string& spirvPath
) {
    const auto code = loadSpirv(spirvPath);
    VkDevice device = context_.device();

    VkShaderModuleCreateInfo shaderInfo{};
    shaderInfo.sType =
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = code.size() * sizeof(uint32_t);
    shaderInfo.pCode = code.data();

    check(
        vkCreateShaderModule(
            device, &shaderInfo, nullptr, &shaderModule_
        ),
        "vkCreateShaderModule"
    );

    // Shader의 set=0, binding=0과 대응
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    check(
        vkCreateDescriptorSetLayout(
            device, &layoutInfo,
            nullptr, &descriptorSetLayout_
        ),
        "vkCreateDescriptorSetLayout"
    );

    // Shader의 push_constant elementCount와 대응
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(uint32_t);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;

    check(
        vkCreatePipelineLayout(
            device, &pipelineLayoutInfo,
            nullptr, &pipelineLayout_
        ),
        "vkCreatePipelineLayout"
    );

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule_;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType =
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = pipelineLayout_;

    check(
        vkCreateComputePipelines(
            device, VK_NULL_HANDLE, 1,
            &pipelineInfo, nullptr, &pipeline_
        ),
        "vkCreateComputePipelines"
    );
}

// 2. Shader에 Buffer를 연결할 Descriptor 준비
void VulkanCompute::createDescriptorResources() {
    VkDevice device = context_.device();

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    check(
        vkCreateDescriptorPool(
            device, &poolInfo, nullptr, &descriptorPool_
        ),
        "vkCreateDescriptorPool"
    );

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout_;

    check(
        vkAllocateDescriptorSets(
            device, &allocInfo, &descriptorSet_
        ),
        "vkAllocateDescriptorSets"
    );
}

// 3. GPU 명령을 기록할 공간과 Fence 준비
void VulkanCompute::createCommandResources() {
    VkDevice device = context_.device();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType =
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = context_.queueFamilyIndex();

    check(
        vkCreateCommandPool(
            device, &poolInfo, nullptr, &commandPool_
        ),
        "vkCreateCommandPool"
    );

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    check(
        vkAllocateCommandBuffers(
            device, &allocInfo, &commandBuffer_
        ),
        "vkAllocateCommandBuffers"
    );

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    check(
        vkCreateFence(
            device, &fenceInfo, nullptr, &fence_
        ),
        "vkCreateFence"
    );
}

// 4. Buffer 연결 → 명령 기록 → GPU 제출 → 완료 대기
void VulkanCompute::dispatch(
    VulkanBuffer& buffer,
    uint32_t elementCount
) {
    if (elementCount == 0 ||
        elementCount > buffer.size() / sizeof(float)) {
        throw std::invalid_argument(
            "Invalid compute element count"
        );
    }

    VkDevice device = context_.device();

    // 실제 Buffer를 Shader의 binding=0에 연결
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer.handle();
    bufferInfo.offset = 0;
    bufferInfo.range = buffer.size();

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet_;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    // 기존 명령 버퍼 재사용을 위해 초기화
    check(
        vkResetCommandBuffer(commandBuffer_, 0),
        "vkResetCommandBuffer"
    );

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags =
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    check(
        vkBeginCommandBuffer(commandBuffer_, &beginInfo),
        "vkBeginCommandBuffer"
    );

    // GPU에서 실행할 Pipeline 연결
    vkCmdBindPipeline(
        commandBuffer_,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipeline_
    );

    // Pipeline과 Buffer의 Descriptor 연결
    vkCmdBindDescriptorSets(
        commandBuffer_,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipelineLayout_,
        0, 1, &descriptorSet_,
        0, nullptr
    );

    // Shader에 처리할 원소 개수 전달
    vkCmdPushConstants(
        commandBuffer_,
        pipelineLayout_,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(uint32_t), &elementCount
    );

    // 64개 Invocation이 하나의 Workgroup
    const uint32_t groupCount =
        1 + (elementCount - 1) / kLocalSizeX;

    // GPU에 계산 명령을 기록
    vkCmdDispatch(commandBuffer_, groupCount, 1, 1);

    // GPU Shader Write 결과를 CPU Host Read에 가시화
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

    vkCmdPipelineBarrier(
        commandBuffer_,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT,
        0,
        1, &barrier,
        0, nullptr,
        0, nullptr
    );

    check(
        vkEndCommandBuffer(commandBuffer_),
        "vkEndCommandBuffer"
    );

    check(
        vkResetFences(device, 1, &fence_),
        "vkResetFences"
    );

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer_;

    // 기록한 GPU 작업을 실제 Queue에 제출
    check(
        vkQueueSubmit(
            context_.computeQueue(),
            1, &submitInfo, fence_
        ),
        "vkQueueSubmit"
    );

    // GPU 작업 완료까지 대기 (최대 10초)
    check(
        vkWaitForFences(
            device, 1, &fence_,
            VK_TRUE, 10'000'000'000ULL
        ),
        "vkWaitForFences"
    );
}

// RAII: 생성한 리소스를 역순으로 해제
void VulkanCompute::cleanup() noexcept {
    VkDevice device = context_.device();

    // 예외 상황에서 제출 중인 작업이 남았을 경우까지 고려
    vkDeviceWaitIdle(device);

    if (fence_ != VK_NULL_HANDLE) {
        vkDestroyFence(device, fence_, nullptr);
        fence_ = VK_NULL_HANDLE;
    }

    if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
        commandBuffer_ = VK_NULL_HANDLE;
    }

    if (descriptorPool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool_, nullptr);
        descriptorPool_ = VK_NULL_HANDLE;
        descriptorSet_ = VK_NULL_HANDLE;
    }

    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }

    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(
            device, pipelineLayout_, nullptr
        );
        pipelineLayout_ = VK_NULL_HANDLE;
    }

    if (descriptorSetLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(
            device, descriptorSetLayout_, nullptr
        );
        descriptorSetLayout_ = VK_NULL_HANDLE;
    }

    if (shaderModule_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(
            device, shaderModule_, nullptr
        );
        shaderModule_ = VK_NULL_HANDLE;
    }
}

} // namespace edge::vulkan
