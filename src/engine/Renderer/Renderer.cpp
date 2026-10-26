#include "Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>

#include "Camera/Camera.hpp"
#include "ECS/Component/BoundingVolumeComponent.hpp"
#include "ECS/Component/LightComponent.hpp"
#include "ECS/Component/TransformsComponent.hpp"
#include "ECS/Entity/Entity.hpp"
#include "Resource/ResourceContext.hpp"
#include "Resource/Types/Shader.hpp"
#include "Utils/VulkanHelpers.hpp"
#include "VulkanContext.hpp"
#include "Utils/Log.hpp"

#include "DebugDrawPass.hpp"
#include "GeometryPass.hpp"
#include "ImGuiPass.hpp"
#include "LightingPass.hpp"
#include "PostProcessPass.hpp"

namespace GNC {

namespace {

// Not through the ResourceManager: it caches by id alone, so both stages of
// "geometry_pass" would collide. The modules are only needed until the
// pipelines exist.
void LoadShader(Shader& shader, const ResourceContext& resourceContext) {
    shader.SetContext(&resourceContext);
    if (!shader.Load()) {
        throw std::runtime_error("Renderer: could not load " + shader.GetSourcePaths().front());
    }
}

// Entity ids in FrameData::pickBuffer.
enum PickSlot : std::uint32_t {
    ClickSlot,
    HoverSlot,
    PickSlotCount
};

// The viewport's fractional size truncates to the scene extent, so a pixel on
// its far edge can land one outside.
vk::BufferImageCopy MakePickCopy(vk::Offset2D pixel, vk::Extent2D extent, PickSlot slot) {
    return vk::BufferImageCopy {
        .bufferOffset = slot * sizeof(std::uint32_t),
        .imageSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .layerCount = 1 },
        .imageOffset = {
            .x = std::clamp(pixel.x, 0, static_cast<std::int32_t>(extent.width) - 1),
            .y = std::clamp(pixel.y, 0, static_cast<std::int32_t>(extent.height) - 1),
            .z = 0
        },
        .imageExtent = { .width = 1, .height = 1, .depth = 1 }
    };
}

// sRGB encoded, as DebugDraw takes them. The selected one is the outline's
// (shaders/post_process.slang: SELECTED_OUTLINE).
const glm::vec4 BoundingVolumeColor { 0.35f, 0.85f, 0.45f, 1.0f };
const glm::vec4 SelectedBoundingVolumeColor { 1.0f, 0.6f, 0.1f, 1.0f };

} // namespace

Renderer::Renderer(const VulkanContext& vulkanContext, vk::Extent2D extent, Camera* cam, const TextureTable& textures)
    : vulkan(vulkanContext),
      context { .device = &vulkan.GetDevice(), .physicalDevice = &vulkan.GetPhysicalDevice() },
      textureTable(textures),
      camera(cam),
      cullingSystem(cam) {
    CreateCommandPool();
    CreateGlobalDescriptorSetLayout();
    CreateDescriptorPool();
    CreateFrameResources();

    swapchain = std::make_unique<Swapchain>(vulkan, extent);
    // Until the Viewport panel says otherwise.
    sceneExtent = swapchain->GetExtent();
    UpdateCameraAspect();

    SetupRenderPasses();
}

Renderer::~Renderer() {
    context.device->waitIdle();
}

void Renderer::SetCamera(Camera* cam) {
    camera = cam;
    cullingSystem.SetCamera(cam);
    UpdateCameraAspect();
}

bool Renderer::Render(const std::vector<Entity*>& entities) {
    FrameData& frame = frames[currentFrame];

    // Once this slot's fence signals, the GPU is done with its command buffer
    // and uniform buffers.
    while (context.device->waitForFences(*frame.inFlightFence, vk::True, std::numeric_limits<std::uint64_t>::max()) == vk::Result::eTimeout) {
    }
    ReadPick(frame);

    // Reset the fence only after acquire succeeds: if it fails and the frame
    // is skipped, an unsignaled fence would block the next wait forever.
    const std::optional<std::uint32_t> imageIndex = swapchain->AcquireNextImage(*frame.imageAvailableSemaphore);
    if (!imageIndex) return false;

    context.device->resetFences(*frame.inFlightFence);

    UpdateCameraBuffer(frame);
    UpdateLightsBuffer(frame, entities);

    cullingSystem.CullScene(entities);
    OrderForOutlines();
    if (showBoundingVolumes) AddBoundingVolumes();

    frame.commandBuffer.reset();
    frame.commandBuffer.begin(vk::CommandBufferBeginInfo { .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    const FrameContext frameContext {
        .commandBuffer = frame.commandBuffer,
        .globalSet = *frame.globalSet,
        .frameIndex = currentFrame,
        .targetImage = swapchain->GetImage(*imageIndex),
        .targetView = swapchain->GetImageView(*imageIndex)
    };
    renderPassManager.Execute(frameContext);
    debugDraw.Clear();
    RecordPick(frame);

    // Present waits on the submit's semaphore, so this barrier only changes
    // the layout.
    const vk::ImageMemoryBarrier2 presentBarrier = makeImageBarrier(frameContext.targetImage, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone);
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &presentBarrier
    });

    frame.commandBuffer.end();

    // The swapchain image is first written at eColorAttachmentOutput (by
    // ImGuiPass), so the acquire wait goes there.
    const vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::Semaphore imageAvailable = *frame.imageAvailableSemaphore;
    const vk::Semaphore renderFinished = swapchain->GetRenderFinishedSemaphore(*imageIndex);
    const vk::CommandBuffer commandBuffer = *frame.commandBuffer;
    vulkan.GetQueue().submit(vk::SubmitInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAvailable,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinished
    }, *frame.inFlightFence);

    const bool presented = swapchain->Present(vulkan.GetQueue(), *imageIndex);

    currentFrame = (currentFrame + 1) % MaxFramesInFlight;
    return presented;
}

void Renderer::Resize(vk::Extent2D extent) {
    context.device->waitIdle();

    // The old swapchain stays alive until the new one exists, so the driver
    // can hand its resources over.
    swapchain = std::make_unique<Swapchain>(vulkan, extent, swapchain.get());

    // ImGuiPass is sized for the swapchain.
    RebuildRenderPasses();
}

void Renderer::SetSceneExtent(vk::Extent2D extent) {
    extent.width = std::max(extent.width, 1u);
    extent.height = std::max(extent.height, 1u);
    if (extent == sceneExtent) return;

    context.device->waitIdle();
    sceneExtent = extent;
    RebuildRenderPasses();
}

vk::ImageView Renderer::GetSceneView() const {
    return *postProcessPass->GetOutput().view;
}

void Renderer::SetHoverPixel(std::optional<vk::Offset2D> pixel) {
    hoverPixel = pixel;
    if (!pixel) hoveredEntity = nullptr;
}

void Renderer::ForgetEntity(const Entity* entity) {
    // nullptr, not erased: the ids index these lists.
    for (FrameData& frame : frames) {
        std::ranges::replace(frame.pickEntities, entity, nullptr);
    }
    if (pickResult == entity) pickResult.reset();
    if (hoveredEntity == entity) hoveredEntity = nullptr;
    if (outlinedSelected == entity) outlinedSelected = nullptr;
    if (outlinedHovered == entity) outlinedHovered = nullptr;
}

void Renderer::CreateCommandPool() {
    // eResetCommandBuffer lets each frame reset just its own command buffer.
    vk::CommandPoolCreateInfo poolInfo {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = vulkan.GetQueueFamilyIndex()
    };
    commandPool = vk::raii::CommandPool(*context.device, poolInfo);
}

// Set 0 (shaders/common/frame.slang): binding 0 the camera (fragment stage
// too, for lighting), binding 1 the lights.
void Renderer::CreateGlobalDescriptorSetLayout() {
    const std::array bindings {
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
        },
        vk::DescriptorSetLayoutBinding {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        }
    };
    vk::DescriptorSetLayoutCreateInfo layoutInfo {
        .bindingCount = static_cast<std::uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };
    globalSetLayout = vk::raii::DescriptorSetLayout(*context.device, layoutInfo);
}

void Renderer::CreateDescriptorPool() {
    // Two uniform buffers (camera, lights) per frame.
    vk::DescriptorPoolSize poolSize {
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 2 * MaxFramesInFlight
    };
    // eFreeDescriptorSet is required: a vk::raii::DescriptorSet frees
    // itself back into its pool when it is destroyed.
    vk::DescriptorPoolCreateInfo poolInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MaxFramesInFlight,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };
    descriptorPool = vk::raii::DescriptorPool(*context.device, poolInfo);
}

void Renderer::CreateFrameResources() {
    const vk::raii::Device& device = *context.device;

    vk::CommandBufferAllocateInfo commandBufferInfo {
        .commandPool = *commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MaxFramesInFlight
    };
    vk::raii::CommandBuffers commandBuffers(device, commandBufferInfo);

    std::array<vk::DescriptorSetLayout, MaxFramesInFlight> setLayouts;
    setLayouts.fill(*globalSetLayout);
    vk::DescriptorSetAllocateInfo setInfo {
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = MaxFramesInFlight,
        .pSetLayouts = setLayouts.data()
    };
    vk::raii::DescriptorSets descriptorSets(device, setInfo);

    for (std::uint32_t i = 0; i < MaxFramesInFlight; ++i) {
        FrameData& frame = frames[i];

        frame.commandBuffer = std::move(commandBuffers[i]);

        // Created signaled, so the first wait in Render() returns at once.
        frame.inFlightFence = vk::raii::Fence(device, vk::FenceCreateInfo { .flags = vk::FenceCreateFlagBits::eSignaled });
        frame.imageAvailableSemaphore = vk::raii::Semaphore(device, vk::SemaphoreCreateInfo {});

        createBuffer(device, *context.physicalDevice, sizeof(CameraUniform),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            frame.cameraBuffer, frame.cameraMemory);
        // Never unmapped by hand: freeing the memory unmaps it.
        frame.cameraMapped = frame.cameraMemory.mapMemory(0, sizeof(CameraUniform));

        createBuffer(device, *context.physicalDevice, sizeof(LightsUniform),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            frame.lightsBuffer, frame.lightsMemory);
        frame.lightsMapped = frame.lightsMemory.mapMemory(0, sizeof(LightsUniform));

        const vk::DeviceSize pickSize = PickSlotCount * sizeof(std::uint32_t);
        createBuffer(device, *context.physicalDevice, pickSize,
            vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            frame.pickBuffer, frame.pickMemory);
        frame.pickMapped = frame.pickMemory.mapMemory(0, pickSize);

        frame.globalSet = std::move(descriptorSets[i]);

        const vk::DescriptorBufferInfo cameraInfo {
            .buffer = *frame.cameraBuffer,
            .offset = 0,
            .range = sizeof(CameraUniform)
        };
        const vk::DescriptorBufferInfo lightsInfo {
            .buffer = *frame.lightsBuffer,
            .offset = 0,
            .range = sizeof(LightsUniform)
        };
        const std::array writes {
            vk::WriteDescriptorSet {
                .dstSet = *frame.globalSet,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &cameraInfo
            },
            vk::WriteDescriptorSet {
                .dstSet = *frame.globalSet,
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &lightsInfo
            }
        };
        device.updateDescriptorSets(writes, {});
    }
}

// Host coherent: the following submit makes the write visible. Y is already
// flipped in Camera::GetProjectionMatrix().
void Renderer::UpdateCameraBuffer(FrameData& frame) {
    if (!camera) return;

    const CameraUniform uniform = camera->GetUniform();
    std::memcpy(frame.cameraMapped, &uniform, sizeof(uniform));
}

// Lights are gathered from all entities, not just the visible ones: a
// light outside the view still lights what is inside it.
void Renderer::UpdateLightsBuffer(FrameData& frame, const std::vector<Entity*>& entities) {
    LightsUniform uniform;
    uniform.ambientColor = ambientColor;

    for (Entity* entity : entities) {
        if (!entity->IsActiveInHierarchy()) continue;

        const LightComponent* light = entity->GetComponent<LightComponent>();
        if (!light || !light->IsActive()) continue;

        if (uniform.lightCount == MaxLights) {
            if (!warnedAboutLightLimit) {
                Log::writeFormatted(Log::Warning, "Renderer: more than %u lights in the scene, the rest are ignored", MaxLights);
                warnedAboutLightLimit = true;
            }
            break;
        }

        uniform.lights[uniform.lightCount++] = MakeGpuLight(*light, entity->GetComponent<TransformComponent>());
    }

    std::memcpy(frame.lightsMapped, &uniform, sizeof(uniform));
}

GpuLight Renderer::MakeGpuLight(const LightComponent& light, const TransformComponent* transform) {
    GpuLight gpuLight;
    gpuLight.type = static_cast<std::uint32_t>(light.GetType());
    gpuLight.color = light.GetColor();
    gpuLight.intensity = light.GetIntensity();
    gpuLight.range = light.GetRange();

    // Column 3 is the translation, column 2 the local Z axis (scaled, hence
    // normalize).
    if (transform) {
        const glm::mat4 world = transform->GetWorldMatrix();
        gpuLight.position = glm::vec3(world[3]);
        gpuLight.direction = glm::normalize(-glm::vec3(world[2]));
    }

    // KHR_lights_punctual's cone falloff,
    //   saturate((cos(angle) - cos(outer)) / (cos(inner) - cos(outer)))
    // rearranged into cos(angle) * scale + offset so the shader does one
    // multiply-add. The floor on the divisor keeps inner == outer finite.
    const float cosOuter = std::cos(light.GetOuterConeAngle());
    const float cosInner = std::cos(light.GetInnerConeAngle());
    gpuLight.spotAngleScale = 1.0f / std::max(cosInner - cosOuter, 1e-4f);
    gpuLight.spotAngleOffset = -cosOuter * gpuLight.spotAngleScale;

    return gpuLight;
}

void Renderer::RecordPick(FrameData& frame) {
    std::array<vk::BufferImageCopy, PickSlotCount> copies;
    std::uint32_t copyCount = 0;
    if (pickRequest) {
        copies[copyCount++] = MakePickCopy(*pickRequest, sceneExtent, ClickSlot);
        frame.clickPending = true;
        pickRequest.reset();
    }
    if (hoverPixel) {
        copies[copyCount++] = MakePickCopy(*hoverPixel, sceneExtent, HoverSlot);
        frame.hoverPending = true;
    }
    if (copyCount == 0) return;

    const vk::Image entityIds = *geometryPass->GetGBuffer().entityId.image;
    const vk::ImageMemoryBarrier2 toCopy = makeImageBarrier(entityIds, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal,
        vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferRead);
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo { .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &toCopy });

    frame.commandBuffer.copyImageToBuffer(entityIds, vk::ImageLayout::eTransferSrcOptimal, *frame.pickBuffer,
        vk::ArrayProxy<const vk::BufferImageCopy>(copyCount, copies.data()));

    // The fence wait alone does not make the copy visible to the host.
    const vk::MemoryBarrier2 toHost {
        .srcStageMask = vk::PipelineStageFlagBits2::eCopy,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eHost,
        .dstAccessMask = vk::AccessFlagBits2::eHostRead
    };
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo { .memoryBarrierCount = 1, .pMemoryBarriers = &toHost });

    // What GeometryPass numbered the ids by.
    frame.pickEntities = cullingSystem.GetVisibleEntities();
}

void Renderer::ReadPick(FrameData& frame) {
    if (!frame.clickPending && !frame.hoverPending) return;

    std::array<std::uint32_t, PickSlotCount> ids {};
    std::memcpy(ids.data(), frame.pickMapped, sizeof(ids));
    auto toEntity = [&](std::uint32_t id) -> Entity* {
        return id > 0 && id <= frame.pickEntities.size() ? frame.pickEntities[id - 1] : nullptr;
    };

    if (frame.clickPending) pickResult = toEntity(ids[ClickSlot]);
    // Not once the cursor has left the scene since.
    if (frame.hoverPending && hoverPixel) hoveredEntity = toEntity(ids[HoverSlot]);

    frame.pickEntities.clear();
    frame.clickPending = false;
    frame.hoverPending = false;
}

// GeometryPass numbers the ids by the visible list, so this order puts each
// outlined subtree in one run of ids: selected only, selected and hovered,
// then hovered only.
void Renderer::OrderForOutlines() {
    std::vector<Entity*>& visible = cullingSystem.GetVisibleEntities();
    auto selected = [this](const Entity* entity) { return entity->IsInSubtree(outlinedSelected); };
    auto hovered = [this](const Entity* entity) { return entity->IsInSubtree(outlinedHovered); };

    const auto selectedEnd = std::stable_partition(visible.begin(), visible.end(), selected);
    const auto bothBegin = std::stable_partition(visible.begin(), selectedEnd, std::not_fn(hovered));
    const auto hoveredEnd = std::stable_partition(selectedEnd, visible.end(), hovered);

    auto toId = [&](std::vector<Entity*>::iterator it) { return static_cast<std::uint32_t>(it - visible.begin() + 1); };
    postProcessPass->SetOutlinedIds({ toId(visible.begin()), toId(selectedEnd) }, { toId(bothBegin), toId(hoveredEnd) });
}

// Only what culling let through: the rest is off screen. Culling lets
// through only entities with a transform.
void Renderer::AddBoundingVolumes() {
    for (Entity* entity : cullingSystem.GetVisibleEntities()) {
        auto* bounds = entity->GetComponent<BoundingVolumeComponent>();
        const BoundingVolume* volume = bounds && bounds->IsActive() ? bounds->GetVolume() : nullptr;
        if (!volume) continue;

        const glm::vec4& color = entity->IsInSubtree(outlinedSelected) ? SelectedBoundingVolumeColor : BoundingVolumeColor;
        debugDraw.AddVolume(*volume, bounds->GetFitId(), entity->GetComponent<TransformComponent>()->GetWorldMatrix(), color);
    }
}

void Renderer::UpdateCameraAspect() {
    if (!camera) return;

    camera->SetAspectRatio(static_cast<float>(sceneExtent.width) / static_cast<float>(sceneExtent.height));
}

void Renderer::RebuildRenderPasses() {
    geometryPass = nullptr;
    postProcessPass = nullptr;
    renderPassManager.Clear();

    UpdateCameraAspect();
    SetupRenderPasses();
}

void Renderer::SetupRenderPasses() {
    const ResourceContext shaderContext { .device = context.device, .physicalDevice = context.physicalDevice };

    Shader geometryVertex("geometry_pass", vk::ShaderStageFlagBits::eVertex);
    Shader geometryFragment("geometry_pass", vk::ShaderStageFlagBits::eFragment);
    Shader lightingVertex("lighting_pass", vk::ShaderStageFlagBits::eVertex);
    Shader lightingFragment("lighting_pass", vk::ShaderStageFlagBits::eFragment);
    Shader postVertex("post_process", vk::ShaderStageFlagBits::eVertex);
    Shader postFragment("post_process", vk::ShaderStageFlagBits::eFragment);
    Shader debugVertex("debug_draw", vk::ShaderStageFlagBits::eVertex);
    Shader debugFragment("debug_draw", vk::ShaderStageFlagBits::eFragment);
    for (Shader* shader : { &geometryVertex, &geometryFragment, &lightingVertex, &lightingFragment, &postVertex, &postFragment,
                            &debugVertex, &debugFragment }) {
        LoadShader(*shader, shaderContext);
    }

    geometryPass = renderPassManager.AddRenderPass<GeometryPass>("GeometryPass",
        context, sceneExtent, *globalSetLayout,
        PassShaders { *geometryVertex.GetShaderModule(), *geometryFragment.GetShaderModule() },
        &cullingSystem, textureTable);

    auto lightingPass = renderPassManager.AddRenderPass<LightingPass>("LightingPass",
        context, sceneExtent, *globalSetLayout,
        PassShaders { *lightingVertex.GetShaderModule(), *lightingFragment.GetShaderModule() },
        geometryPass);
    // Without dependencies, pass order is hash map order.
    lightingPass->AddDependency(geometryPass->GetName());

    postProcessPass = renderPassManager.AddRenderPass<PostProcessPass>("PostProcessPass",
        context, sceneExtent, *globalSetLayout,
        PassShaders { *postVertex.GetShaderModule(), *postFragment.GetShaderModule() },
        geometryPass, lightingPass);
    postProcessPass->AddDependency(geometryPass->GetName());
    postProcessPass->AddDependency(lightingPass->GetName());

    auto debugDrawPass = renderPassManager.AddRenderPass<DebugDrawPass>("DebugDrawPass",
        context, sceneExtent, *globalSetLayout,
        PassShaders { *debugVertex.GetShaderModule(), *debugFragment.GetShaderModule() },
        debugDraw, geometryPass, postProcessPass);
    debugDrawPass->AddDependency(postProcessPass->GetName());

    // After the scene image it shows.
    auto imguiPass = renderPassManager.AddRenderPass<ImGuiPass>("ImGuiPass", swapchain->GetExtent());
    imguiPass->AddDependency(debugDrawPass->GetName());

    ++sceneVersion;
}

} // namespace GNC
