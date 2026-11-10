#include "SpritePass.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera/Camera.hpp"
#include "Culling/FrustumCulling.hpp"
#include "ECS/Component/SpriteComponent.hpp"
#include "ECS/Component/TransformsComponent.hpp"
#include "GeometryPass.hpp"
#include "Resource/Types/Texture.hpp"
#include "TextureTable.hpp"
#include "Utils/Log.hpp"
#include "Utils/VulkanHelpers.hpp"

namespace GNC {

namespace {

constexpr vk::PipelineStageFlags2 DepthStages =
    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;

// shaders/sprite.slang: CORNERS
constexpr std::uint32_t VerticesPerSprite = 6;

// The direction depth is measured along. A camera looking down an axis, as
// the 2D editor's does, has components like cos(90°) in its front, which is
// 4e-8 in float, not 0: left in, sprites at one depth would sort by where
// they are across the view, and swap as it pans.
glm::vec3 SortAxis(const Camera& camera) {
    glm::vec3 axis = camera.GetFront();
    for (int i = 0; i < 3; ++i) {
        if (std::abs(axis[i]) < 1e-6f) axis[i] = 0.0f;
    }
    return axis;
}

} // namespace

SpritePass::SpritePass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                       vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, FrustumCulling* culling,
                       const TextureTable& textures, GeometryPass* gPass)
    : RenderPass(name), context(renderContext), extent(renderExtent), cullingSystem(culling), textureTable(textures), geometryPass(gPass) {
    const vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
    output = Attachment(context, extent, OutputFormat, usage, vk::ImageAspectFlagBits::eColor);
    drawOrder = Attachment(context, extent, DrawOrderFormat, usage, vk::ImageAspectFlagBits::eColor);

    CreateSpriteBuffers();
    CreatePipeline(globalSetLayout, shaders);
}

void SpritePass::BeginPass(const FrameContext& frame) {
    const GBuffer& gBuffer = geometryPass->GetGBuffer();

    // The pass's own targets: the previous frame's post-process pass may
    // still be reading them. The entity ids and depth: the fragment shader
    // stage chains these after the transitions the geometry pass ended on.
    // Depth keeps its read-only layout: it is tested, never written.
    const auto toColorAttachment = [](const Attachment& attachment, vk::ImageLayout oldLayout) {
        return makeImageBarrier(*attachment.image, vk::ImageAspectFlagBits::eColor,
            oldLayout, vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite);
    };
    const std::array barriers {
        toColorAttachment(output, vk::ImageLayout::eUndefined),
        toColorAttachment(drawOrder, vk::ImageLayout::eUndefined),
        toColorAttachment(gBuffer.entityId, vk::ImageLayout::eShaderReadOnlyOptimal),
        makeImageBarrier(*gBuffer.depth.image, vk::ImageAspectFlagBits::eDepth,
            vk::ImageLayout::eDepthStencilReadOnlyOptimal, vk::ImageLayout::eDepthStencilReadOnlyOptimal,
            DepthStages | vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            DepthStages, vk::AccessFlagBits2::eDepthStencilAttachmentRead)
    };
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = static_cast<std::uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data()
    });

    // In the order of the SV_Target outputs in shaders/sprite.slang. The ids
    // keep what the meshes wrote.
    const std::array colorInfos {
        vk::RenderingAttachmentInfo {
            .imageView = *output.view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = vk::ClearColorValue(std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f })
        },
        vk::RenderingAttachmentInfo {
            .imageView = *gBuffer.entityId.view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore
        },
        vk::RenderingAttachmentInfo {
            .imageView = *drawOrder.view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = vk::ClearColorValue(std::array<std::uint32_t, 4> { 0, 0, 0, 0 })
        }
    };
    const vk::RenderingAttachmentInfo depthInfo {
        .imageView = *gBuffer.depth.view,
        .imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eNone
    };
    frame.commandBuffer.beginRendering(vk::RenderingInfo {
        .renderArea = { { 0, 0 }, extent },
        .layerCount = 1,
        .colorAttachmentCount = static_cast<std::uint32_t>(colorInfos.size()),
        .pColorAttachments = colorInfos.data(),
        .pDepthAttachment = &depthInfo
    });
}

void SpritePass::Render(const FrameContext& frame) {
    CollectSprites();
    if (sprites.empty()) return;

    // Host coherent, and this frame's slot: its previous submission is done.
    const SpriteBuffer& spriteBuffer = spriteBuffers[frame.frameIndex];
    batches.clear();
    for (std::uint32_t index = 0; index < sprites.size(); ++index) {
        spriteBuffer.mapped[index] = sprites[index].gpu;

        if (batches.empty() || batches.back().texture != sprites[index].texture) {
            batches.push_back(Batch { .texture = sprites[index].texture, .first = index, .count = 0 });
        }
        ++batches.back().count;
    }

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
    const std::array descriptorSets { frame.globalSet, textureTable.GetSet(), *spriteBuffer.set };
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, descriptorSets, nullptr);

    // The shader finds a vertex's sprite from its index, so each batch draws
    // from its first sprite's vertices on.
    for (const Batch& batch : batches) {
        commandBuffer.pushConstants<SpritePushConstants>(*pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0,
            SpritePushConstants { .texture = batch.texture });
        commandBuffer.draw(VerticesPerSprite * batch.count, 1, VerticesPerSprite * batch.first, 0);
    }
}

void SpritePass::EndPass(const FrameContext& frame) {
    frame.commandBuffer.endRendering();

    // To the post-process pass's fragment shader.
    const auto toSampled = [](const Attachment& attachment) {
        return makeImageBarrier(*attachment.image, vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead);
    };
    const std::array barriers { toSampled(output), toSampled(drawOrder), toSampled(geometryPass->GetGBuffer().entityId) };
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = static_cast<std::uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data()
    });
}

void SpritePass::CollectSprites() {
    sprites.clear();

    const std::vector<Entity*>& entities = cullingSystem->GetVisibleEntities();
    // Culling lets nothing through without a camera.
    const Camera* camera = cullingSystem->GetCamera();
    if (entities.empty() || !camera) return;

    const glm::vec3 axis = SortAxis(*camera);
    for (std::size_t index = 0; index < entities.size(); ++index) {
        Entity* entity = entities[index];
        const SpriteComponent* sprite = entity->GetComponent<SpriteComponent>();
        if (!sprite) continue;

        if (sprites.size() == MaxSprites) {
            if (!warnedAboutSpriteLimit) {
                Log::writeFormatted(Log::Warning, "SpritePass: more than %u sprites in view, the rest are not drawn", MaxSprites);
                warnedAboutSpriteLimit = true;
            }
            break;
        }

        const glm::mat4 world = entity->GetComponent<TransformComponent>()->GetWorldMatrix();
        const glm::vec2 size = sprite->GetSize();
        const Texture* texture = sprite->GetTexture();
        const bool flipX = sprite->GetFlipX();
        const bool flipY = sprite->GetFlipY();
        sprites.push_back(Sprite {
            .layer = sprite->GetLayer(),
            .depth = glm::dot(glm::vec3(world[3]) - camera->GetPosition(), axis),
            .entity = entity->GetId(),
            .texture = texture ? texture->GetTableSlot() : TextureTable::White,
            .gpu = GpuSprite {
                .model = glm::scale(glm::translate(world, glm::vec3(-0.5f * size, 0.0f)), glm::vec3(size, 1.0f)),
                .color = glm::vec4(sprite->GetColor(), sprite->GetOpacity()),
                // The square's y runs up, the image's v down.
                .uvRect = { flipX ? 1.0f : 0.0f, flipY ? 0.0f : 1.0f, flipX ? -1.0f : 1.0f, flipY ? 1.0f : -1.0f },
                // As GeometryPass numbers them.
                .entityId = static_cast<std::uint32_t>(index + 1)
            }
        });
    }

    std::ranges::sort(sprites, [](const Sprite& a, const Sprite& b) {
        if (a.layer != b.layer) return a.layer < b.layer;
        if (a.depth != b.depth) return a.depth > b.depth;
        return a.entity < b.entity;
    });
}

void SpritePass::CreateSpriteBuffers() {
    const vk::raii::Device& device = *context.device;

    const vk::DescriptorSetLayoutBinding binding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex
    };
    spriteSetLayout = vk::raii::DescriptorSetLayout(device, vk::DescriptorSetLayoutCreateInfo {
        .bindingCount = 1,
        .pBindings = &binding
    });

    const vk::DescriptorPoolSize poolSize {
        .type = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = MaxFramesInFlight
    };
    // eFreeDescriptorSet: a vk::raii::DescriptorSet frees itself into its pool.
    descriptorPool = vk::raii::DescriptorPool(device, vk::DescriptorPoolCreateInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MaxFramesInFlight,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    });

    std::array<vk::DescriptorSetLayout, MaxFramesInFlight> setLayouts;
    setLayouts.fill(*spriteSetLayout);
    vk::raii::DescriptorSets sets(device, vk::DescriptorSetAllocateInfo {
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = MaxFramesInFlight,
        .pSetLayouts = setLayouts.data()
    });

    const vk::DeviceSize size = sizeof(GpuSprite) * MaxSprites;
    for (std::uint32_t i = 0; i < MaxFramesInFlight; ++i) {
        SpriteBuffer& spriteBuffer = spriteBuffers[i];
        createBuffer(device, *context.physicalDevice, size, vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            spriteBuffer.buffer, spriteBuffer.memory);
        // Never unmapped by hand: freeing the memory unmaps it.
        spriteBuffer.mapped = static_cast<GpuSprite*>(spriteBuffer.memory.mapMemory(0, size));
        spriteBuffer.set = std::move(sets[i]);

        const vk::DescriptorBufferInfo bufferInfo { .buffer = *spriteBuffer.buffer, .offset = 0, .range = size };
        device.updateDescriptorSets(vk::WriteDescriptorSet {
            .dstSet = *spriteBuffer.set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &bufferInfo
        }, {});
    }
}

void SpritePass::CreatePipeline(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders) {
    const vk::raii::Device& device = *context.device;

    const vk::PushConstantRange pushConstantRange {
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = sizeof(SpritePushConstants)
    };
    const std::array setLayouts { globalSetLayout, *textureTable.GetLayout(), *spriteSetLayout };
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

    // No vertex buffer: the vertex shader builds each quad from its index.
    const vk::PipelineVertexInputStateCreateInfo vertexInput {};
    const vk::PipelineInputAssemblyStateCreateInfo inputAssembly { .topology = vk::PrimitiveTopology::eTriangleList };
    const vk::PipelineViewportStateCreateInfo viewportState { .viewportCount = 1, .scissorCount = 1 };

    // Not culled: a sprite shows from behind too, mirrored.
    const vk::PipelineRasterizationStateCreateInfo rasterizer {
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .lineWidth = 1.0f
    };

    const vk::PipelineMultisampleStateCreateInfo multisampling { .rasterizationSamples = vk::SampleCountFlagBits::e1 };

    // Hidden behind meshes, but never by each other: the draw order decides.
    const vk::PipelineDepthStencilStateCreateInfo depthStencil {
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::False,
        .depthCompareOp = vk::CompareOp::eLessOrEqual
    };

    // Premultiplied alpha over the layer, whose own alpha builds up to what
    // it covers of the scene. The ids and the draw order are integers, which
    // cannot blend, so the states differ (independentBlend).
    const vk::ColorComponentFlags allChannels = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                              | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    const vk::PipelineColorBlendAttachmentState overwrite { .blendEnable = vk::False, .colorWriteMask = allChannels };
    const std::array blendAttachments {
        vk::PipelineColorBlendAttachmentState {
            .blendEnable = vk::True,
            .srcColorBlendFactor = vk::BlendFactor::eOne,
            .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eOne,
            .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .alphaBlendOp = vk::BlendOp::eAdd,
            .colorWriteMask = allChannels
        },
        overwrite,
        overwrite
    };
    const vk::PipelineColorBlendStateCreateInfo colorBlending {
        .attachmentCount = static_cast<std::uint32_t>(blendAttachments.size()),
        .pAttachments = blendAttachments.data()
    };

    const std::array dynamicStates { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    const vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    const std::array colorFormats { OutputFormat, GBuffer::EntityIdFormat, DrawOrderFormat };
    const vk::PipelineRenderingCreateInfo renderingInfo {
        .colorAttachmentCount = static_cast<std::uint32_t>(colorFormats.size()),
        .pColorAttachmentFormats = colorFormats.data(),
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
