#include "Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

#include "Camera/Camera.hpp"
#include "ECS/Component/LightComponent.hpp"
#include "ECS/Component/TransformsComponent.hpp"
#include "ECS/Entity/Entity.hpp"
#include "Resource/ResourceContext.hpp"
#include "Resource/Types/Shader.hpp"
#include "Utils/VulkanHelpers.hpp"
#include "VulkanContext.hpp"
#include "Utils/Log.hpp"

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

} // namespace

Renderer::Renderer(const VulkanContext& vulkanContext, vk::Extent2D extent, Camera* cam)
    : vulkan(vulkanContext),
      context { .device = &vulkan.GetDevice(), .physicalDevice = &vulkan.GetPhysicalDevice() },
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

    // Reset the fence only after acquire succeeds: if it fails and the frame
    // is skipped, an unsignaled fence would block the next wait forever.
    const std::optional<std::uint32_t> imageIndex = swapchain->AcquireNextImage(*frame.imageAvailableSemaphore);
    if (!imageIndex) return false;

    context.device->resetFences(*frame.inFlightFence);

    UpdateCameraBuffer(frame);
    UpdateLightsBuffer(frame, entities);

    cullingSystem.CullScene(entities);

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
        if (!entity->IsActive()) continue;

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

    // From the matrix, so this keeps working once transforms have parents.
    // Column 3 is the translation, column 2 the local Z axis (scaled, hence
    // normalize).
    if (transform) {
        const glm::mat4 world = transform->GetTransformMatrix();
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

void Renderer::UpdateCameraAspect() {
    if (!camera) return;

    camera->SetAspectRatio(static_cast<float>(sceneExtent.width) / static_cast<float>(sceneExtent.height));
}

void Renderer::RebuildRenderPasses() {
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
    for (Shader* shader : { &geometryVertex, &geometryFragment, &lightingVertex, &lightingFragment, &postVertex, &postFragment }) {
        LoadShader(*shader, shaderContext);
    }

    auto geometryPass = renderPassManager.AddRenderPass<GeometryPass>("GeometryPass",
        context, sceneExtent, *globalSetLayout,
        PassShaders { *geometryVertex.GetShaderModule(), *geometryFragment.GetShaderModule() },
        &cullingSystem);

    auto lightingPass = renderPassManager.AddRenderPass<LightingPass>("LightingPass",
        context, sceneExtent, *globalSetLayout,
        PassShaders { *lightingVertex.GetShaderModule(), *lightingFragment.GetShaderModule() },
        geometryPass);
    // Without dependencies, pass order is hash map order.
    lightingPass->AddDependency(geometryPass->GetName());

    postProcessPass = renderPassManager.AddRenderPass<PostProcessPass>("PostProcessPass",
        context, sceneExtent, *globalSetLayout,
        PassShaders { *postVertex.GetShaderModule(), *postFragment.GetShaderModule() },
        lightingPass);
    postProcessPass->AddDependency(lightingPass->GetName());

    // After the scene image it shows.
    auto imguiPass = renderPassManager.AddRenderPass<ImGuiPass>("ImGuiPass", swapchain->GetExtent());
    imguiPass->AddDependency(postProcessPass->GetName());

    ++sceneVersion;
}

} // namespace GNC
