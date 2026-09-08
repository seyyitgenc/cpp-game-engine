#pragma once

#include "../Resource.hpp"

#include <string>
#include <vulkan/vulkan_raii.hpp>

// Texture resource
// Owns the four GPU objects a sampled texture needs. Each one is a vk::raii
// handle, so the only thing doUnload() has to get right is the order.
class Texture : public Resource {
private:
    vk::raii::Image image{nullptr};
    vk::raii::DeviceMemory memory{nullptr};
    vk::raii::ImageView imageView{nullptr};
    vk::raii::Sampler sampler{nullptr};

    int width = 0;
    int height = 0;
    int channels = 0;

public:
    explicit Texture(const std::string& id) : Resource(id) {}

    const vk::raii::Image& GetImage() const { return image; }
    const vk::raii::ImageView& GetImageView() const { return imageView; }
    const vk::raii::Sampler& GetSampler() const { return sampler; }

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    int GetChannels() const { return channels; }

protected:
    bool doLoad() override {
        // TODO: discorver extensions automatically, then fetch the loader for it.
        const std::string filePath = "textures/" + GetId() + ".ktx";

        unsigned char* data = LoadImageData(filePath, &width, &height, &channels);
        if (!data) return false;

        CreateVulkanImage(data, width, height, channels);

        // The CPU copy has done its job the moment the staging upload is done.
        FreeImageData(data);
        return true;
    }

    void doUnload() override {
        // Assigning nullptr destroys the handle. Order still matters: the view
        // and the sampler are built on top of the image, and the image sits in
        // the memory allocation, so everything is released the way it was made.
        sampler = nullptr;
        imageView = nullptr;
        image = nullptr;
        memory = nullptr;

        width = height = channels = 0;
    }

private:
    // TODO: implement these out (stb_image for png/jpg, libktx for .ktx)
    unsigned char* LoadImageData(const std::string& /*filePath*/,
                                 int* /*outWidth*/, int* /*outHeight*/, int* /*outChannels*/) {
        return nullptr;
    }

    void FreeImageData(unsigned char* /*data*/) {}

    // TODO: create the vk::raii::Image, back it with device-local memory, copy
    // the pixels in through a staging buffer using the context's transfer
    // queue, then build the view and the sampler.
    void CreateVulkanImage(const unsigned char* /*data*/,
                           int /*imageWidth*/, int /*imageHeight*/, int /*imageChannels*/) {}
};
