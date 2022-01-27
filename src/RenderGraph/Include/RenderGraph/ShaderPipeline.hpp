#ifndef SHADERPIPELINE2_HPP
#define SHADERPIPELINE2_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Utils/MovablePtr.hpp"

#include <vector>
#include <functional>
#include <optional>
#include <memory>
#include <filesystem>
#include <string>

#include <vulkan/vulkan.h>

namespace RG {
enum class ShaderKind : uint8_t;
class ShaderModule;
class ShaderModuleReflection;
class RenderPass;
class GraphicsPipeline;
class PipelineLayout;
class DescriptorSetLayout;
}

namespace RG {

class RENDERGRAPH_DLL_EXPORT ShaderPipeline {
private:
    const VkDevice device;

    std::unique_ptr<RG::ShaderModule> vertexShader;
    std::unique_ptr<RG::ShaderModule> fragmentShader;
    std::unique_ptr<RG::ShaderModule> geometryShader;
    std::unique_ptr<RG::ShaderModule> tessellationEvaluationShader;
    std::unique_ptr<RG::ShaderModule> tessellationControlShader;
    std::unique_ptr<RG::ShaderModule> computeShader;

    std::unique_ptr<RG::ShaderModule>& GetShaderByIndex (uint32_t index);
    std::unique_ptr<RG::ShaderModule>& GetShaderByExtension (const std::string& extension);
    std::unique_ptr<RG::ShaderModule>& GetShaderByKind (RG::ShaderKind kind);

public:
    struct CompileSettings {
        uint32_t                               width;
        uint32_t                               height;
        RG::MovablePtr<VkDescriptorSetLayout> layout;
        std::vector<VkAttachmentReference>     attachmentReferences;
        std::vector<VkAttachmentReference>     inputAttachmentReferences;
        std::vector<VkAttachmentDescription>   attachmentDescriptions;
        VkPrimitiveTopology                    topology;

        std::optional<bool> blendEnabled;
    };


    struct CompileResult {
        std::unique_ptr<RG::RenderPass>       renderPass;
        std::unique_ptr<RG::PipelineLayout>   pipelineLayout;
        std::unique_ptr<RG::GraphicsPipeline> pipeline;

        void Clear ();
    };

public:
    CompileResult   compileResult;
    CompileSettings compileSettings;

    ShaderPipeline (VkDevice device);
    ShaderPipeline (VkDevice device, const std::vector<std::filesystem::path>& pathes);
    ShaderPipeline (VkDevice device, const std::vector<std::pair<RG::ShaderKind, std::string>>& sources);

    ~ShaderPipeline ();

    // settings shaders
    void SetShaderFromSourceString (RG::ShaderKind shaderKind, const std::string& source);
    void SetVertexShaderFromString (const std::string& source);
    void SetFragmentShaderFromString (const std::string& source);

    void SetShaderFromSourceFile (const std::filesystem::path& shaderPath);
    void SetShadersFromSourceFiles (const std::vector<std::filesystem::path>& shaderPath);

    void Compile (CompileSettings&& settings);

    void Reload ();

    void IterateShaders (const std::function<void (RG::ShaderModule&)>& func) const;

    std::vector<VkPipelineShaderStageCreateInfo> GetShaderStages () const;

    std::unique_ptr<RG::DescriptorSetLayout> CreateDescriptorSetLayout (VkDevice device) const;

    const RG::ShaderModuleReflection& GetReflection (RG::ShaderKind kind);
};

} // namespace RG

#endif