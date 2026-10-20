#include "PostProcessPass.hpp"

#include <array>
#include <cstdint>

#include "LightingPass.hpp"
#include "ShaderTypes.hpp"
#include "Utils/VulkanHelpers.hpp"

namespace GNC {

PostProcessPass::PostProcessPass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                                 vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, LightingPass* lPass,
                                 vk::Format targetFormat, bool targetIsSrgb)
    : RenderPass(name), context(renderContext), extent(renderExtent), lightingPass(lPass), encodeSrgb(!targetIsSrgb) {
    CreateDescriptorSet();
    CreatePipeline(globalSetLayout, shaders, targetFormat);
}

void PostProcessPass::BeginPass(const FrameContext& frame) {
    // The swapchain image only becomes usable once the acquire semaphore is
    // signaled. The submit waits on it at eColorAttachmentOutput, so starting
    // this barrier's src scope at that same stage chains the two: the layout
    // transition happens after the wait. The old contents are not needed.
    const vk::ImageMemoryBarrier2 barrier = makeImageBarrier(frame.targetImage, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite);
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    });

    // eDontCare: the fullscreen triangle writes every pixel.
    const vk::RenderingAttachmentInfo colorInfo {
        .imageView = frame.targetView,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eDontCare,
        .storeOp = vk::AttachmentStoreOp::eStore
    };
    frame.commandBuffer.beginRendering(vk::RenderingInfo {
        .renderArea = { { 0, 0 }, extent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorInfo
    });
}

void PostProcessPass::Render(const FrameContext& frame) {
    const vk::raii::CommandBuffer& commandBuffer = frame.commandBuffer;

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    commandBuffer.setViewport(0, vk::Viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    });
    commandBuffer.setScissor(0, vk::Rect2D { { 0, 0 }, extent });

    const std::array descriptorSets { frame.globalSet, *inputSet };
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, descriptorSets, nullptr);

    const PostProcessPushConstants pushConstants {
        .exposure = exposure,
        .encodeSrgb = encodeSrgb ? 1u : 0u
    };
    commandBuffer.pushConstants<PostProcessPushConstants>(*pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, pushConstants);

    // Fullscreen triangle; the vertex shader makes the corners from the index.
    commandBuffer.draw(3, 1, 0, 0);
}

void PostProcessPass::EndPass(const FrameContext& frame) {
    frame.commandBuffer.endRendering();

    // Present waits on the semaphore the submit signals after everything in
    // it, so nothing on the GPU side has to wait for this barrier: it only
    // changes the layout.
    const vk::ImageMemoryBarrier2 barrier = makeImageBarrier(frame.targetImage, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone);
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    });
}

void PostProcessPass::CreateDescriptorSet() {
    const vk::raii::Device& device = *context.device;

    // Read with Load(), one texel per pixel, so a sampled image without a
    // sampler, like the G-buffer in the lighting pass.
    const vk::DescriptorSetLayoutBinding binding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eSampledImage,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment
    };
    inputSetLayout = vk::raii::DescriptorSetLayout(device, vk::DescriptorSetLayoutCreateInfo {
        .bindingCount = 1,
        .pBindings = &binding
    });

    const vk::DescriptorPoolSize poolSize {
        .type = vk::DescriptorType::eSampledImage,
        .descriptorCount = 1
    };
    // eFreeDescriptorSet: a vk::raii::DescriptorSet frees itself into its pool.
    descriptorPool = vk::raii::DescriptorPool(device, vk::DescriptorPoolCreateInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    });

    const vk::DescriptorSetLayout setLayout = *inputSetLayout;
    vk::raii::DescriptorSets sets(device, vk::DescriptorSetAllocateInfo {
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &setLayout
    });
    inputSet = std::move(sets.front());

    // The layout LightingPass::EndPass leaves its output in.
    const vk::DescriptorImageInfo imageInfo {
        .imageView = *lightingPass->GetOutput().view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    device.updateDescriptorSets(vk::WriteDescriptorSet {
        .dstSet = *inputSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eSampledImage,
        .pImageInfo = &imageInfo
    }, {});
}

void PostProcessPass::CreatePipeline(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, vk::Format targetFormat) {
    const vk::raii::Device& device = *context.device;

    const std::array setLayouts { globalSetLayout, *inputSetLayout };
    const vk::PushConstantRange pushConstantRange {
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = sizeof(PostProcessPushConstants)
    };
    pipelineLayout = vk::raii::PipelineLayout(device, vk::PipelineLayoutCreateInfo {
        .setLayoutCount = static_cast<std::uint32_t>(setLayouts.size()),
        .pSetLayouts = setLayouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    });

    // slangc names every entry point "main" in the SPIR-V it writes.
    const std::array stages {
        vk::PipelineShaderStageCreateInfo { .stage = vk::ShaderStageFlagBits::eVertex, .module = shaders.vertex, .pName = "main" },
        vk::PipelineShaderStageCreateInfo { .stage = vk::ShaderStageFlagBits::eFragment, .module = shaders.fragment, .pName = "main" }
    };

    // No vertex buffer: the vertex shader builds the triangle from its index.
    const vk::PipelineVertexInputStateCreateInfo vertexInput {};
    const vk::PipelineInputAssemblyStateCreateInfo inputAssembly { .topology = vk::PrimitiveTopology::eTriangleList };
    const vk::PipelineViewportStateCreateInfo viewportState { .viewportCount = 1, .scissorCount = 1 };

    const vk::PipelineRasterizationStateCreateInfo rasterizer {
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .lineWidth = 1.0f
    };

    const vk::PipelineMultisampleStateCreateInfo multisampling { .rasterizationSamples = vk::SampleCountFlagBits::e1 };

    const vk::PipelineColorBlendAttachmentState blendAttachment {
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                        | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
    const vk::PipelineColorBlendStateCreateInfo colorBlending {
        .attachmentCount = 1,
        .pAttachments = &blendAttachment
    };

    const std::array dynamicStates { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    const vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    const vk::PipelineRenderingCreateInfo renderingInfo {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &targetFormat
    };

    const vk::GraphicsPipelineCreateInfo pipelineInfo {
        .pNext = &renderingInfo,
        .stageCount = static_cast<std::uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = *pipelineLayout
    };
    pipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

} // namespace GNC
