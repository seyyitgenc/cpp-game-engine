#include "PostProcessPass.hpp"

#include <array>
#include <cstdint>

#include "GeometryPass.hpp"
#include "LightingPass.hpp"
#include "ShaderTypes.hpp"
#include "Utils/VulkanHelpers.hpp"

namespace GNC {

namespace {

// Bindings in set 1, as in shaders/post_process.slang.
enum InputBinding : std::uint32_t {
    HdrColorBinding = 0,
    EntityIdBinding = 1,
    DepthBinding = 2,
    InputBindingCount
};

} // namespace

PostProcessPass::PostProcessPass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                                 vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders,
                                 GeometryPass* gPass, LightingPass* lPass)
    : RenderPass(name), context(renderContext), extent(renderExtent), geometryPass(gPass), lightingPass(lPass) {
    output = Attachment(context, extent, OutputFormat,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor);

    CreateDescriptorSet();
    CreatePipeline(globalSetLayout, shaders);
}

void PostProcessPass::BeginPass(const FrameContext& frame) {
    // The previous frame's ImGui pass (or a platform window) may still be
    // sampling the scene image.
    const vk::ImageMemoryBarrier2 barrier = makeImageBarrier(*output.image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite);
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    });

    // eDontCare: the fullscreen triangle writes every pixel.
    const vk::RenderingAttachmentInfo colorInfo {
        .imageView = *output.view,
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

    // The output is UNORM, so the shader encodes sRGB itself.
    const PostProcessPushConstants pushConstants {
        .exposure = exposure,
        .encodeSrgb = 1u,
        .selectedBegin = selectedIds.begin,
        .selectedEnd = selectedIds.end,
        .hoveredBegin = hoveredIds.begin,
        .hoveredEnd = hoveredIds.end
    };
    commandBuffer.pushConstants<PostProcessPushConstants>(*pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, pushConstants);

    // Fullscreen triangle; the vertex shader makes the corners from the index.
    commandBuffer.draw(3, 1, 0, 0);
}

void PostProcessPass::EndPass(const FrameContext& frame) {
    frame.commandBuffer.endRendering();

    // For ImGui, which samples it in the Viewport panel.
    const vk::ImageMemoryBarrier2 barrier = makeImageBarrier(*output.image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead);
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    });
}

void PostProcessPass::CreateDescriptorSet() {
    const vk::raii::Device& device = *context.device;

    // Read with Load(), one texel per pixel, so sampled images without a
    // sampler, like the G-buffer in the lighting pass.
    std::array<vk::DescriptorSetLayoutBinding, InputBindingCount> bindings;
    for (std::uint32_t i = 0; i < InputBindingCount; ++i) {
        bindings[i] = vk::DescriptorSetLayoutBinding {
            .binding = i,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        };
    }
    inputSetLayout = vk::raii::DescriptorSetLayout(device, vk::DescriptorSetLayoutCreateInfo {
        .bindingCount = static_cast<std::uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    });

    const vk::DescriptorPoolSize poolSize {
        .type = vk::DescriptorType::eSampledImage,
        .descriptorCount = InputBindingCount
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

    // The layouts the lighting and geometry passes leave these in.
    std::array<vk::DescriptorImageInfo, InputBindingCount> imageInfos;
    imageInfos[HdrColorBinding] = { .imageView = *lightingPass->GetOutput().view, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    imageInfos[EntityIdBinding] = { .imageView = *geometryPass->GetGBuffer().entityId.view, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    imageInfos[DepthBinding] = { .imageView = *geometryPass->GetGBuffer().depth.view, .imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal };

    std::array<vk::WriteDescriptorSet, InputBindingCount> writes;
    for (std::uint32_t i = 0; i < InputBindingCount; ++i) {
        writes[i] = vk::WriteDescriptorSet {
            .dstSet = *inputSet,
            .dstBinding = i,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .pImageInfo = &imageInfos[i]
        };
    }
    device.updateDescriptorSets(writes, {});
}

void PostProcessPass::CreatePipeline(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders) {
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
        .pColorAttachmentFormats = &OutputFormat
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
