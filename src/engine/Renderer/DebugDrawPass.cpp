#include "DebugDrawPass.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>
#include <variant>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GeometryPass.hpp"
#include "PostProcessPass.hpp"
#include "Utils/VulkanHelpers.hpp"

namespace GNC {

namespace {

constexpr vk::PipelineStageFlags2 DepthStages =
    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;

constexpr vk::ShaderStageFlags PushConstantStages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

// Ends one strip and starts the next (primitive restart, 32-bit indices).
constexpr std::uint32_t RestartIndex = std::numeric_limits<std::uint32_t>::max();

constexpr std::uint32_t SphereStacks = 12;
constexpr std::uint32_t SphereSlices = 24;

} // namespace

DebugDrawPass::DebugDrawPass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                             vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, const DebugDraw& drawList,
                             GeometryPass* gPass, PostProcessPass* pPass)
    : RenderPass(name), context(renderContext), extent(renderExtent), debugDraw(drawList), geometryPass(gPass), postProcessPass(pPass) {
    // ---
    // Box
    // ---
    std::vector<glm::vec3> positions;
    for (std::uint32_t corner = 0; corner < 8; ++corner) {
        positions.emplace_back(corner & 1 ? 1.0f : -1.0f, corner & 2 ? 1.0f : -1.0f, corner & 4 ? 1.0f : -1.0f);
    }
    unitBox = MakeMesh(positions, BoxStrip);

    // ------
    // Sphere
    // ------
    // One strip for each band between two latitudes.
    positions.clear();
    for (std::uint32_t stack = 0; stack <= SphereStacks; ++stack) {
        const float polar = glm::pi<float>() * static_cast<float>(stack) / SphereStacks;
        for (std::uint32_t slice = 0; slice <= SphereSlices; ++slice) {
            const float azimuth = glm::two_pi<float>() * static_cast<float>(slice) / SphereSlices;
            positions.emplace_back(std::sin(polar) * std::cos(azimuth), std::cos(polar), std::sin(polar) * std::sin(azimuth));
        }
    }
    std::vector<std::uint32_t> indices;
    for (std::uint32_t stack = 0; stack < SphereStacks; ++stack) {
        for (std::uint32_t slice = 0; slice <= SphereSlices; ++slice) {
            indices.push_back(stack * (SphereSlices + 1) + slice);
            indices.push_back((stack + 1) * (SphereSlices + 1) + slice);
        }
        indices.push_back(RestartIndex);
    }
    unitSphere = MakeMesh(positions, indices);

    CreatePipeline(globalSetLayout, shaders);
}

void DebugDrawPass::Execute(const FrameContext& frame) {
    // A frame in flight may still draw a hull mesh it used. One drawn
    // MaxFramesInFlight frames ago is done: its frame's fence was waited on.
    ++frameNumber;
    std::erase_if(hullMeshes, [&](const auto& entry) { return frameNumber - entry.second.lastDrawn >= MaxFramesInFlight; });

    // Without shapes, the scene image stays as the post-process pass left it.
    if (debugDraw.GetVolumes().empty()) return;
    RenderPass::Execute(frame);
}

void DebugDrawPass::BeginPass(const FrameContext& frame) {
    const Attachment& scene = postProcessPass->GetOutput();
    const Attachment& depth = geometryPass->GetGBuffer().depth;

    // The fragment shader stage chains these after the transitions the
    // post-process and geometry passes ended on. Depth keeps its read-only
    // layout: it is tested, never written.
    const std::array barriers {
        makeImageBarrier(*scene.image, vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite),
        makeImageBarrier(*depth.image, vk::ImageAspectFlagBits::eDepth,
            vk::ImageLayout::eDepthStencilReadOnlyOptimal, vk::ImageLayout::eDepthStencilReadOnlyOptimal,
            DepthStages | vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            DepthStages, vk::AccessFlagBits2::eDepthStencilAttachmentRead)
    };
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = static_cast<std::uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data()
    });

    const vk::RenderingAttachmentInfo colorInfo {
        .imageView = *scene.view,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore
    };
    const vk::RenderingAttachmentInfo depthInfo {
        .imageView = *depth.view,
        .imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eNone
    };
    frame.commandBuffer.beginRendering(vk::RenderingInfo {
        .renderArea = { { 0, 0 }, extent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorInfo,
        .pDepthAttachment = &depthInfo
    });
}

void DebugDrawPass::Render(const FrameContext& frame) {
    const vk::raii::CommandBuffer& commandBuffer = frame.commandBuffer;

    draws.clear();
    for (const DebugDraw::Volume& volume : debugDraw.GetVolumes()) {
        glm::mat4 model = volume.model;
        const ShapeMesh& mesh = GetMesh(volume, model);
        draws.push_back({ &mesh, DebugDrawPushConstants { .model = model, .color = volume.color } });
    }

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

    // Every shape twice: where the scene hides it, faint, then where it does
    // not.
    const std::array<std::pair<vk::CompareOp, float>, 2> layers {{
        { vk::CompareOp::eGreater, HiddenOpacity },
        { vk::CompareOp::eLessOrEqual, 1.0f }
    }};
    for (const auto& [compareOp, opacity] : layers) {
        commandBuffer.setDepthCompareOp(compareOp);

        const ShapeMesh* bound = nullptr;
        for (const Draw& draw : draws) {
            if (draw.mesh != bound) {
                commandBuffer.bindVertexBuffers(0, *draw.mesh->vertexBuffer, vk::DeviceSize { 0 });
                commandBuffer.bindIndexBuffer(*draw.mesh->indexBuffer, 0, vk::IndexType::eUint32);
                bound = draw.mesh;
            }

            DebugDrawPushConstants constants = draw.constants;
            constants.color.a *= opacity;
            commandBuffer.pushConstants<DebugDrawPushConstants>(*pipelineLayout, PushConstantStages, 0, constants);
            commandBuffer.drawIndexed(draw.mesh->indexCount, 1, 0, 0, 0);
        }
    }
}

void DebugDrawPass::EndPass(const FrameContext& frame) {
    frame.commandBuffer.endRendering();

    // For ImGui, which samples it in the Viewport panel.
    const vk::ImageMemoryBarrier2 barrier = makeImageBarrier(*postProcessPass->GetOutput().image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead);
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    });
}

DebugDrawPass::ShapeMesh DebugDrawPass::MakeMesh(std::span<const glm::vec3> positions, std::span<const std::uint32_t> indices) const {
    auto fill = [&](const void* data, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& memory) {
        createBuffer(*context.device, *context.physicalDevice, size, usage,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, memory);
        std::memcpy(memory.mapMemory(0, size), data, static_cast<std::size_t>(size));
        memory.unmapMemory();
    };

    ShapeMesh mesh;
    fill(positions.data(), positions.size_bytes(), vk::BufferUsageFlagBits::eVertexBuffer, mesh.vertexBuffer, mesh.vertexMemory);
    fill(indices.data(), indices.size_bytes(), vk::BufferUsageFlagBits::eIndexBuffer, mesh.indexBuffer, mesh.indexMemory);
    mesh.indexCount = static_cast<std::uint32_t>(indices.size());
    return mesh;
}

const DebugDrawPass::ShapeMesh& DebugDrawPass::GetMesh(const DebugDraw::Volume& volume, glm::mat4& model) {
    if (const auto* box = std::get_if<AABBBoundingBox3D>(volume.volume)) {
        model = glm::scale(glm::translate(model, box->Center()), box->Extents());
        return unitBox;
    }
    if (const auto* sphere = std::get_if<BoundingSphere>(volume.volume)) {
        model = glm::scale(glm::translate(model, sphere->center), glm::vec3(sphere->radius));
        return unitSphere;
    }

    // Each triangle a strip of its own.
    auto [entry, added] = hullMeshes.try_emplace(volume.id);
    if (added) {
        const ConvexHull& hull = std::get<ConvexHull>(*volume.volume);
        std::vector<std::uint32_t> indices;
        indices.reserve(hull.triangles.size() * 4);
        for (const glm::uvec3& triangle : hull.triangles) {
            indices.insert(indices.end(), { triangle.x, triangle.y, triangle.z, RestartIndex });
        }
        entry->second.mesh = MakeMesh(hull.vertices, indices);
    }
    entry->second.lastDrawn = frameNumber;
    return entry->second.mesh;
}

void DebugDrawPass::CreatePipeline(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders) {
    const vk::raii::Device& device = *context.device;

    const vk::PushConstantRange pushConstantRange {
        .stageFlags = PushConstantStages,
        .offset = 0,
        .size = sizeof(DebugDrawPushConstants)
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

    const vk::VertexInputBindingDescription binding {
        .binding = 0,
        .stride = sizeof(glm::vec3),
        .inputRate = vk::VertexInputRate::eVertex
    };
    const vk::VertexInputAttributeDescription attribute { .location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = 0 };
    const vk::PipelineVertexInputStateCreateInfo vertexInput {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding,
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = &attribute
    };
    const vk::PipelineInputAssemblyStateCreateInfo inputAssembly {
        .topology = vk::PrimitiveTopology::eTriangleStrip,
        .primitiveRestartEnable = vk::True
    };
    const vk::PipelineViewportStateCreateInfo viewportState { .viewportCount = 1, .scissorCount = 1 };

    // Each triangle's edges only (fillModeNonSolid). A width of 1 needs no
    // wideLines feature.
    const vk::PipelineRasterizationStateCreateInfo rasterizer {
        .polygonMode = vk::PolygonMode::eLine,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .lineWidth = 1.0f
    };

    const vk::PipelineMultisampleStateCreateInfo multisampling { .rasterizationSamples = vk::SampleCountFlagBits::e1 };

    // The compare op is set per layer.
    const vk::PipelineDepthStencilStateCreateInfo depthStencil {
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::False,
        .depthCompareOp = vk::CompareOp::eLessOrEqual
    };

    // Over the scene by the shape's alpha. The scene image's own alpha stays
    // as it is: ImGui blends the Viewport image by it.
    const vk::PipelineColorBlendAttachmentState blendAttachment {
        .blendEnable = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eZero,
        .dstAlphaBlendFactor = vk::BlendFactor::eOne,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                        | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
    const vk::PipelineColorBlendStateCreateInfo colorBlending {
        .attachmentCount = 1,
        .pAttachments = &blendAttachment
    };

    const std::array dynamicStates { vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eDepthCompareOp };
    const vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    const vk::PipelineRenderingCreateInfo renderingInfo {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &PostProcessPass::OutputFormat,
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
