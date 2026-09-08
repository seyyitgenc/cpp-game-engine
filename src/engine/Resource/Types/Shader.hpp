#pragma once

#include "../Resource.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

// Shader resource
// Wraps one SPIR-V module. The stage is part of the resource's identity because
// the same base name usually exists once per stage on disk.
class Shader : public Resource {
private:
    vk::raii::ShaderModule shaderModule{nullptr};
    vk::ShaderStageFlagBits stage;

public:
    Shader(const std::string& id, vk::ShaderStageFlagBits shaderStage)
        : Resource(id), stage(shaderStage) {}

    const vk::raii::ShaderModule& GetShaderModule() const { return shaderModule; }
    vk::ShaderStageFlagBits GetStage() const { return stage; }

protected:
    bool doLoad() override {
        const std::string filePath = "shaders/" + GetId() + StageExtension(stage) + ".spv";

        std::vector<char> code;
        if (!ReadFile(filePath, code)) return false;

        // SPIR-V is a stream of 32-bit words; anything else is a truncated or
        // non-SPIR-V file and Vulkan would reject it anyway.
        if (code.empty() || code.size() % 4 != 0) return false;

        CreateShaderModule(code);
        return true;
    }

    void doUnload() override { shaderModule = nullptr; }

private:
    static const char* StageExtension(vk::ShaderStageFlagBits shaderStage) {
        switch (shaderStage) {
            case vk::ShaderStageFlagBits::eVertex:                 return ".vert";
            case vk::ShaderStageFlagBits::eFragment:               return ".frag";
            case vk::ShaderStageFlagBits::eCompute:                return ".comp";
            case vk::ShaderStageFlagBits::eGeometry:               return ".geom";
            case vk::ShaderStageFlagBits::eTessellationControl:    return ".tesc";
            case vk::ShaderStageFlagBits::eTessellationEvaluation: return ".tese";
            default:                                               return "";
        }
    }

    bool ReadFile(const std::string& filePath, std::vector<char>& buffer) {
        // Seek to the end on open so the size is known before the read.
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) return false;

        const std::streamsize fileSize = file.tellg();
        if (fileSize <= 0) return false;

        buffer.resize(static_cast<std::size_t>(fileSize));
        file.seekg(0);
        file.read(buffer.data(), fileSize);

        return file.good() || file.eof();
    }

    void CreateShaderModule(const std::vector<char>& code) {
        vk::ShaderModuleCreateInfo createInfo{};
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const std::uint32_t*>(code.data());

        shaderModule = vk::raii::ShaderModule(GetDevice(), createInfo);
    }
};
