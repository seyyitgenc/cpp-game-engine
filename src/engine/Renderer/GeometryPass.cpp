#include "GeometryPass.hpp"

#include <stdexcept>
#include <vector>

#include "Culling/FrustumCulling.hpp"
#include "ECS/Component/MeshComponent.hpp"
#include "ECS/Component/TransformsComponent.hpp"
#include "Material.hpp"
#include "ShaderTypes.hpp"
#include "Utils/VulkanHelpers.hpp"

namespace GNC {

namespace {

// What a MeshComponent without a material draws with.
const Material DefaultMaterial {};

constexpr vk::ShaderStageFlags PushConstantStages =
    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

constexpr vk::PipelineStageFlags2 DepthStages =
    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;

} // namespace

GeometryPass::GeometryPass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                           vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, FrustumCulling* culling)
    : RenderPass(name), context(renderContext), extent(renderExtent), cullingSystem(culling) {
    CreateGBuffer();
    CreatePipeline(globalSetLayout, shaders);
}

GeometryPass::~GeometryPass() = default;

void GeometryPass::BeginPass(const FrameContext& frame) {
    // The previous frame's lighting pass may still be sampling the G-buffer:
    // a write-after-read hazard, so an execution dependency (no src access)
    // is enough. Old contents are discarded (eUndefined).
    std::array<vk::ImageMemoryBarrier2, GBuffer::ColorCount + 1> barriers;
    const auto colorAttachments = gBuffer.ColorAttachments();
    for (std::uint32_t i = 0; i < GBuffer::ColorCount; ++i) {
        barriers[i] = makeImageBarrier(*colorAttachments[i]->image, vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite);
    }
    // Read by the post-process pass, and on pick frames by the pick copy.
    barriers[GBuffer::EntityIdTarget].srcStageMask = vk::PipelineStageFlagBits2::eFragmentShader | vk::PipelineStageFlagBits2::eCopy;
    barriers[GBuffer::ColorCount] = makeImageBarrier(*gBuffer.depth.image, vk::ImageAspectFlagBits::eDepth,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal,
        vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eNone,
        DepthStages, vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite);

    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = static_cast<std::uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data()
    });

    // Lighting masks empty pixels by depth, so the surface attributes are
    // cleared only for frame debuggers. The entity ids must be: 0 is "none".
    std::array<vk::RenderingAttachmentInfo, GBuffer::ColorCount> colorInfos;
    for (std::uint32_t i = 0; i < GBuffer::ColorCount; ++i) {
        colorInfos[i] = vk::RenderingAttachmentInfo {
            .imageView = *colorAttachments[i]->view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = vk::ClearColorValue(std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f })
        };
    }
    colorInfos[GBuffer::EntityIdTarget].clearValue = vk::ClearColorValue(std::array<std::uint32_t, 4> { 0, 0, 0, 0 });

    // Stored, not discarded: the lighting pass rebuilds positions from it.
    vk::RenderingAttachmentInfo depthInfo {
        .imageView = *gBuffer.depth.view,
        .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = vk::ClearDepthStencilValue { .depth = 1.0f, .stencil = 0 }
    };

    frame.commandBuffer.beginRendering(vk::RenderingInfo {
        .renderArea = { { 0, 0 }, extent },
        .layerCount = 1,
        .colorAttachmentCount = static_cast<std::uint32_t>(colorInfos.size()),
        .pColorAttachments = colorInfos.data(),
        .pDepthAttachment = &depthInfo
    });
}

void GeometryPass::Render(const FrameContext& frame) {
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
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, frame.globalSet, nullptr);

    // Culling only lets through active entities that have both a mesh and a
    // transform component.
    const std::vector<Entity*>& entities = cullingSystem->GetVisibleEntities();
    for (std::size_t index = 0; index < entities.size(); ++index) {
        Entity* entity = entities[index];
        const MeshComponent* meshComponent = entity->GetComponent<MeshComponent>();
        const TransformComponent* transform = entity->GetComponent<TransformComponent>();

        // The component can exist before its mesh has been loaded.
        const Mesh* mesh = meshComponent->GetMesh();
        if (!mesh || !mesh->IsLoaded() || mesh->GetIndexCount() == 0) continue;

        const Material& material = meshComponent->GetMaterial() ? *meshComponent->GetMaterial() : DefaultMaterial;

        const GeometryPushConstants pushConstants {
            .model = transform->GetTransformMatrix(),
            .baseColorFactor = material.baseColorFactor,
            .emissive = glm::vec4(material.emissiveFactor * material.emissiveStrength, 0.0f),
            .metallicFactor = material.metallicFactor,
            .roughnessFactor = material.roughnessFactor,
            // Renderer::ReadPick() maps it back through the same list.
            .entityId = static_cast<std::uint32_t>(index + 1)
        };
        commandBuffer.pushConstants<GeometryPushConstants>(*pipelineLayout, PushConstantStages, 0, pushConstants);

        const vk::DeviceSize vertexOffset = 0;
        commandBuffer.bindVertexBuffers(0, *mesh->GetVertexBuffer(), vertexOffset);
        commandBuffer.bindIndexBuffer(*mesh->GetIndexBuffer(), 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(mesh->GetIndexCount(), 1, 0, 0, 0);
    }
}

void GeometryPass::EndPass(const FrameContext& frame) {
    frame.commandBuffer.endRendering();

    // To the lighting and post-process passes' fragment shaders; the writes
    // must be made visible, so both sides carry an access mask.
    std::array<vk::ImageMemoryBarrier2, GBuffer::ColorCount + 1> barriers;
    const auto colorAttachments = gBuffer.ColorAttachments();
    for (std::uint32_t i = 0; i < GBuffer::ColorCount; ++i) {
        barriers[i] = makeImageBarrier(*colorAttachments[i]->image, vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead);
    }
    barriers[GBuffer::ColorCount] = makeImageBarrier(*gBuffer.depth.image, vk::ImageAspectFlagBits::eDepth,
        vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eDepthStencilReadOnlyOptimal,
        DepthStages, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead);

    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = static_cast<std::uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data()
    });
}

void GeometryPass::CreateGBuffer() {
    // D32_SFLOAT is not mandatory, though every desktop GPU has it: fail here
    // rather than with a validation error at image creation.
    // TODO: maybe there should be a fallback for it?
    const vk::FormatFeatureFlags depthFeatures =
        vk::FormatFeatureFlagBits::eDepthStencilAttachment | vk::FormatFeatureFlagBits::eSampledImage;
    const vk::FormatProperties depthProperties = context.physicalDevice->getFormatProperties(GBuffer::DepthFormat);
    if ((depthProperties.optimalTilingFeatures & depthFeatures) != depthFeatures) {
        throw std::runtime_error("GeometryPass: D32_SFLOAT cannot be used as a sampled depth attachment on this GPU");
    }

    const vk::ImageUsageFlags colorUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
    const vk::ImageUsageFlags depthUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
    const vk::ImageAspectFlags color = vk::ImageAspectFlagBits::eColor;

    gBuffer.baseColor = Attachment(context, extent, GBuffer::BaseColorFormat, colorUsage, color);
    gBuffer.normal = Attachment(context, extent, GBuffer::NormalFormat, colorUsage, color);
    gBuffer.material = Attachment(context, extent, GBuffer::MaterialFormat, colorUsage, color);
    gBuffer.emissive = Attachment(context, extent, GBuffer::EmissiveFormat, colorUsage, color);
    gBuffer.entityId = Attachment(context, extent, GBuffer::EntityIdFormat, colorUsage | vk::ImageUsageFlagBits::eTransferSrc, color);
    gBuffer.depth = Attachment(context, extent, GBuffer::DepthFormat, depthUsage, vk::ImageAspectFlagBits::eDepth);
}

void GeometryPass::CreatePipeline(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders) {
    const vk::raii::Device& device = *context.device;

    const vk::PushConstantRange pushConstantRange {
        .stageFlags = PushConstantStages,
        .offset = 0,
        .size = sizeof(GeometryPushConstants)
    };
    pipelineLayout = vk::raii::PipelineLayout(device, vk::PipelineLayoutCreateInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &globalSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    });

    // slangc names every entry point "main" in the SPIR-V it writes.
    const std::array stages {
        vk::PipelineShaderStageCreateInfo { .stage = vk::ShaderStageFlagBits::eVertex, .module = shaders.vertex, .pName = "main" },
        vk::PipelineShaderStageCreateInfo { .stage = vk::ShaderStageFlagBits::eFragment, .module = shaders.fragment, .pName = "main" }
    };

    const auto bindingDescription = Vertex::GetBindingDescription();
    const auto attributeDescriptions = Vertex::GetAttributeDescriptions();
    const vk::PipelineVertexInputStateCreateInfo vertexInput {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };

    const vk::PipelineInputAssemblyStateCreateInfo inputAssembly { .topology = vk::PrimitiveTopology::eTriangleList };

    // Viewport and scissor are dynamic, so a resize does not need a new pipeline.
    const vk::PipelineViewportStateCreateInfo viewportState { .viewportCount = 1, .scissorCount = 1 };

    // Counter-clockwise, as authored (glTF). Vulkan's Y axis pointing down would
    // mirror the winding, but Camera's projection flips Y back.
    const vk::PipelineRasterizationStateCreateInfo rasterizer {
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .lineWidth = 1.0f
    };

    const vk::PipelineMultisampleStateCreateInfo multisampling { .rasterizationSamples = vk::SampleCountFlagBits::e1 };

    const vk::PipelineDepthStencilStateCreateInfo depthStencil {
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess
    };

    // Surface attributes are written as they are; blending them is meaningless.
    std::array<vk::PipelineColorBlendAttachmentState, GBuffer::ColorCount> blendAttachments;
    blendAttachments.fill(vk::PipelineColorBlendAttachmentState {
        .blendEnable = vk::False, // TODO: check this out why false?
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                        | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    });
    const vk::PipelineColorBlendStateCreateInfo colorBlending {
        .attachmentCount = static_cast<std::uint32_t>(blendAttachments.size()),
        .pAttachments = blendAttachments.data()
    };

    const std::array dynamicStates { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    const vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // Dynamic rendering: no VkRenderPass, the pipeline just names the formats
    // it will render into.
    const vk::PipelineRenderingCreateInfo renderingInfo {
        .colorAttachmentCount = GBuffer::ColorCount,
        .pColorAttachmentFormats = GBuffer::ColorFormats.data(),
        .depthAttachmentFormat = GBuffer::DepthFormat
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
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = *pipelineLayout
    };
    pipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

} // namespace GNC
