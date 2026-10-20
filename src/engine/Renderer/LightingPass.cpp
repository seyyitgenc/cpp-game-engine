#include "LightingPass.hpp"

#include <array>
#include <cstdint>

#include "GeometryPass.hpp"
#include "Utils/VulkanHelpers.hpp"

namespace GNC {

namespace {

// Bindings in set 1, as in shaders/lighting_pass.slang.
enum GBufferBinding : std::uint32_t {
    BaseColorBinding = 0,
    NormalBinding = 1,
    MaterialBinding = 2,
    EmissiveBinding = 3,
    DepthBinding = 4,
    GBufferBindingCount
};

} // namespace

LightingPass::LightingPass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                           vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, GeometryPass* gPass)
    : RenderPass(name), context(renderContext), extent(renderExtent), geometryPass(gPass) {
    output = Attachment(context, extent, OutputFormat,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor);

    CreateDescriptorSet();
    CreatePipeline(globalSetLayout, shaders);
}

void LightingPass::BeginPass(const FrameContext& frame) {
    // Same reasoning as the G-buffer in GeometryPass::BeginPass: the previous
    // frame's post-process pass may still be reading this image.
    const vk::ImageMemoryBarrier2 barrier = makeImageBarrier(*output.image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite);
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    });

    // eDontCare: the fullscreen triangle writes every pixel, background
    // included, so there is nothing worth clearing or loading.
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

void LightingPass::Render(const FrameContext& frame) {
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

    const std::array descriptorSets { frame.globalSet, *gBufferSet };
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, descriptorSets, nullptr);

    // Fullscreen triangle; the vertex shader makes the corners from the index.
    commandBuffer.draw(3, 1, 0, 0);
}

void LightingPass::EndPass(const FrameContext& frame) {
    frame.commandBuffer.endRendering();

    const vk::ImageMemoryBarrier2 barrier = makeImageBarrier(*output.image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead);
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    });
}

void LightingPass::CreateDescriptorSet() {
    const vk::raii::Device& device = *context.device;

    // Sampled images without samplers: the shader reads them with Load(),
    // which fetches one texel and never filters.
    std::array<vk::DescriptorSetLayoutBinding, GBufferBindingCount> bindings;
    for (std::uint32_t i = 0; i < GBufferBindingCount; ++i) {
        bindings[i] = vk::DescriptorSetLayoutBinding {
            .binding = i,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        };
    }
    gBufferSetLayout = vk::raii::DescriptorSetLayout(device, vk::DescriptorSetLayoutCreateInfo {
        .bindingCount = static_cast<std::uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    });

    const vk::DescriptorPoolSize poolSize {
        .type = vk::DescriptorType::eSampledImage,
        .descriptorCount = GBufferBindingCount
    };
    // eFreeDescriptorSet: a vk::raii::DescriptorSet frees itself into its pool.
    descriptorPool = vk::raii::DescriptorPool(device, vk::DescriptorPoolCreateInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    });

    const vk::DescriptorSetLayout setLayout = *gBufferSetLayout;
    vk::raii::DescriptorSets sets(device, vk::DescriptorSetAllocateInfo {
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &setLayout
    });
    gBufferSet = std::move(sets.front());

    // Layouts are the ones GeometryPass::EndPass leaves the images in.
    const GBuffer& gBuffer = geometryPass->GetGBuffer();
    std::array<vk::DescriptorImageInfo, GBufferBindingCount> imageInfos;
    imageInfos[BaseColorBinding] = { .imageView = *gBuffer.baseColor.view, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    imageInfos[NormalBinding] = { .imageView = *gBuffer.normal.view, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    imageInfos[MaterialBinding] = { .imageView = *gBuffer.material.view, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    imageInfos[EmissiveBinding] = { .imageView = *gBuffer.emissive.view, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    imageInfos[DepthBinding] = { .imageView = *gBuffer.depth.view, .imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal };

    std::array<vk::WriteDescriptorSet, GBufferBindingCount> writes;
    for (std::uint32_t i = 0; i < GBufferBindingCount; ++i) {
        writes[i] = vk::WriteDescriptorSet {
            .dstSet = *gBufferSet,
            .dstBinding = i,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .pImageInfo = &imageInfos[i]
        };
    }
    device.updateDescriptorSets(writes, {});
}

void LightingPass::CreatePipeline(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders) {
    const vk::raii::Device& device = *context.device;

    const std::array setLayouts { globalSetLayout, *gBufferSetLayout };
    pipelineLayout = vk::raii::PipelineLayout(device, vk::PipelineLayoutCreateInfo {
        .setLayoutCount = static_cast<std::uint32_t>(setLayouts.size()),
        .pSetLayouts = setLayouts.data()
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

    // Every light is summed inside the shader's loop, so the triangle is drawn
    // once and simply overwrites the target.
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

    // No depth attachment, so no depth-stencil state either.
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
