#pragma once

#include "../Resource.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Utils/FileSystem.hpp"

namespace GNC {

// Shader resource
// One SPIR-V module. The stage is part of its identity: a base name exists
// once per stage on disk.
class Shader : public Resource {
private:
    vk::raii::ShaderModule shaderModule{nullptr};
    vk::ShaderStageFlagBits stage;

public:
    Shader(const std::string& id, vk::ShaderStageFlagBits shaderStage)
        : Resource(id), stage(shaderStage) {}

    const vk::raii::ShaderModule& GetShaderModule() const { return shaderModule; }
    vk::ShaderStageFlagBits GetStage() const { return stage; }

    std::vector<std::string> GetSourcePaths() const override { return {SourcePath().string()}; }

protected:
    bool doLoad() override {
        const std::filesystem::path filePath = SourcePath();

        std::vector<char> code;
        if (!ReadFile(filePath, code)) return false;

        // SPIR-V is 32-bit words.
        if (code.empty() || code.size() % 4 != 0) return false;

        CreateShaderModule(code);
        return true;
    }

    void doUnload() override { shaderModule = nullptr; }

private:
    std::filesystem::path SourcePath() const {
        return FileSystem::getPath("shaders/" + GetId() + StageExtension(stage) + ".spv");
    }

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

    // A path, not a string: on Windows ifstream then opens it through the
    // wide API, so non-ASCII directories work.
    bool ReadFile(const std::filesystem::path& filePath, std::vector<char>& buffer) {
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

} // namespace GNC
