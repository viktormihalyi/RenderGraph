#include "ShaderPipeline.hpp"
#include "ShaderReflectionToDescriptor.hpp"
#include "ShaderReflectionToVertexAttribute.hpp"

#include "VulkanWrapper/DescriptorSetLayout.hpp"
#include "VulkanWrapper/GraphicsPipeline.hpp"
#include "VulkanWrapper/PipelineLayout.hpp"
#include "VulkanWrapper/RenderPass.hpp"
#include "VulkanWrapper/ShaderModule.hpp"

#include "Utils/Assert.hpp"
#include "Utils/BuildType.hpp"
#include "Utils/MultithreadedFunction.hpp"
#include "Utils/Timer.hpp"

#include <stdexcept>


namespace RG {


ShaderPipeline::~ShaderPipeline ()
{
}


void ShaderPipeline::CompileResult::Clear ()
{
    renderPass.reset ();
    pipelineLayout.reset ();
    pipeline.reset ();
}


std::unique_ptr<GVK::ShaderModule>& ShaderPipeline::GetShaderByIndex (uint32_t index)
{
    switch (index) {
        case 0: return vertexShader;
        case 1: return fragmentShader;
        case 2: return geometryShader;
        case 3: return tessellationEvaluationShader;
        case 4: return tessellationControlShader;
        case 5: return computeShader;
        default:
            GVK_BREAK ();
            throw std::runtime_error ("no");
    }
}


std::unique_ptr<GVK::ShaderModule>& ShaderPipeline::GetShaderByExtension (const std::string& extension)
{
    if (extension == ".vert") {
        return vertexShader;
    } else if (extension == ".frag") {
        return fragmentShader;
    } else if (extension == ".geom") {
        return geometryShader;
    } else if (extension == ".tese") {
        return tessellationEvaluationShader;
    } else if (extension == ".tesc") {
        return tessellationControlShader;
    } else if (extension == ".comp") {
        return computeShader;
    }

    GVK_BREAK ();
    throw std::runtime_error ("bad shader extension");
}


const GVK::ShaderModuleReflection& ShaderPipeline::GetReflection (GVK::ShaderKind kind)
{
    return GetShaderByKind (kind)->GetReflection ();
}


std::unique_ptr<GVK::ShaderModule>& ShaderPipeline::GetShaderByKind (GVK::ShaderKind kind)
{
    switch (kind) {
        case GVK::ShaderKind::Vertex: return vertexShader;
        case GVK::ShaderKind::Fragment: return fragmentShader;
        case GVK::ShaderKind::TessellationControl: return tessellationControlShader;
        case GVK::ShaderKind::TessellationEvaluation: return tessellationEvaluationShader;
        case GVK::ShaderKind::Geometry: return geometryShader;
        case GVK::ShaderKind::Compute: return computeShader;
    }

    GVK_BREAK ();
    throw std::runtime_error ("unknown shader kind");
}


ShaderPipeline::ShaderPipeline (VkDevice device)
    : device (device)
{
}


ShaderPipeline::ShaderPipeline (VkDevice device, const std::vector<std::filesystem::path>& pathes)
    : ShaderPipeline (device)
{
    SetShadersFromSourceFiles (pathes);
}


ShaderPipeline::ShaderPipeline (VkDevice device, const std::vector<std::pair<GVK::ShaderKind, std::string>>& sources)
    : ShaderPipeline (device)
{
    for (auto [kind, source] : sources) {
        GetShaderByKind (kind) = GVK::ShaderModule::CreateFromGLSLString (device, kind, source);
    }
}


std::vector<VkPipelineShaderStageCreateInfo> ShaderPipeline::GetShaderStages () const
{
    std::vector<VkPipelineShaderStageCreateInfo> result;

    if (vertexShader != nullptr)
        result.push_back (vertexShader->GetShaderStageCreateInfo ());
    if (fragmentShader != nullptr)
        result.push_back (fragmentShader->GetShaderStageCreateInfo ());
    if (geometryShader != nullptr)
        result.push_back (geometryShader->GetShaderStageCreateInfo ());
    if (tessellationEvaluationShader != nullptr)
        result.push_back (tessellationEvaluationShader->GetShaderStageCreateInfo ());
    if (tessellationControlShader != nullptr)
        result.push_back (tessellationControlShader->GetShaderStageCreateInfo ());
    if (computeShader != nullptr)
        result.push_back (computeShader->GetShaderStageCreateInfo ());

    return result;
}


void ShaderPipeline::SetShaderFromSourceString (GVK::ShaderKind shaderKind, const std::string& source)
{
    GVK_ASSERT (GetShaderByKind (shaderKind) == nullptr);
    GetShaderByKind (shaderKind) = GVK::ShaderModule::CreateFromGLSLString (device, shaderKind, source);
}


void ShaderPipeline::SetVertexShaderFromString (const std::string& source)
{
    SetShaderFromSourceString (GVK::ShaderKind::Vertex, source);
}


void ShaderPipeline::SetFragmentShaderFromString (const std::string& source)
{
    SetShaderFromSourceString (GVK::ShaderKind::Fragment, source);
}


void ShaderPipeline::SetShaderFromSourceFile (const std::filesystem::path& shaderPath)
{
    // assert on overwriting shader
    GVK_ASSERT (GetShaderByExtension (shaderPath.extension ().string ()) == nullptr);

    GetShaderByExtension (shaderPath.extension ().string ()) = GVK::ShaderModule::CreateFromGLSLFile (device, shaderPath);
}


void ShaderPipeline::SetShadersFromSourceFiles (const std::vector<std::filesystem::path>& shaderPath)
{
    MultithreadedFunction d (static_cast<uint32_t> (shaderPath.size ()), [&] (uint32_t /* threadCount */, uint32_t threadIndex) {
        SetShaderFromSourceFile (shaderPath[threadIndex]);
    });
}


void ShaderPipeline::Compile (CompileSettings&& settings_)
{
    compileSettings = std::move (settings_);
    compileResult.Clear ();

    const auto instancedVertexProvider = [] (const std::string&) { return false; };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t> (compileSettings.attachmentReferences.size ());
    subpass.pColorAttachments    = compileSettings.attachmentReferences.data ();
    subpass.inputAttachmentCount = static_cast<uint32_t> (compileSettings.inputAttachmentReferences.size ());
    subpass.pInputAttachments    = compileSettings.inputAttachmentReferences.data ();

    VkSubpassDependency dependency = {};
    dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass          = 0;
    dependency.dependencyFlags     = 0;
    dependency.srcStageMask        = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.dstStageMask        = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.srcAccessMask       = VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
                               VK_ACCESS_INDEX_READ_BIT |
                               VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                               VK_ACCESS_UNIFORM_READ_BIT |
                               VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
                               VK_ACCESS_SHADER_READ_BIT |
                               VK_ACCESS_SHADER_WRITE_BIT |
                               VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                               VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_TRANSFER_READ_BIT |
                               VK_ACCESS_TRANSFER_WRITE_BIT;
    dependency.dstAccessMask = dependency.srcAccessMask;

    VkSubpassDependency dependency2 = dependency;
    dependency.srcSubpass           = 0;
    dependency.dstSubpass           = VK_SUBPASS_EXTERNAL;

    compileResult.renderPass     = std::unique_ptr<GVK::RenderPass> (new GVK::RenderPass (device, compileSettings.attachmentDescriptions, { subpass }, { dependency, dependency2 }));
    compileResult.pipelineLayout = std::unique_ptr<GVK::PipelineLayout> (new GVK::PipelineLayout (device, { compileSettings.layout }));

    const std::vector<VkVertexInputAttributeDescription> attribs  = RG::FromShaderReflection::GetVertexAttributes (vertexShader->GetReflection (), instancedVertexProvider);
    const std::vector<VkVertexInputBindingDescription>   bindings = RG::FromShaderReflection::GetVertexBindings (vertexShader->GetReflection (), instancedVertexProvider);

    compileResult.pipeline = std::unique_ptr<GVK::GraphicsPipeline> (new GVK::GraphicsPipeline (
        device,
        compileSettings.width,
        compileSettings.height,
        static_cast<uint32_t> (compileSettings.attachmentReferences.size ()),
        *compileResult.pipelineLayout,
        *compileResult.renderPass,
        GetShaderStages (),
        bindings,
        attribs,
        compileSettings.topology,
        compileSettings.blendEnabled.has_value () ? *compileSettings.blendEnabled : true));
}


void ShaderPipeline::Reload ()
{
    MultithreadedFunction reloader (5, [&] (uint32_t, uint32_t threadIndex) {
        std::unique_ptr<GVK::ShaderModule>& currentShader = GetShaderByIndex (threadIndex);

        if (currentShader != nullptr) {
            currentShader->Reload ();
        }
    });
}


void ShaderPipeline::IterateShaders (const std::function<void (GVK::ShaderModule&)>& func) const
{
    if (vertexShader) {
        func (*vertexShader);
    }
    if (fragmentShader) {
        func (*fragmentShader);
    }
    if (geometryShader) {
        func (*geometryShader);
    }
    if (tessellationEvaluationShader) {
        func (*tessellationEvaluationShader);
    }
    if (tessellationControlShader) {
        func (*tessellationControlShader);
    }
    if (computeShader) {
        func (*computeShader);
    }
}


std::unique_ptr<GVK::DescriptorSetLayout> ShaderPipeline::CreateDescriptorSetLayout (VkDevice device_) const
{
    std::vector<VkDescriptorSetLayoutBinding> layout;

    if constexpr (IsDebugBuild) {
        std::vector<std::string> shaderSources;
        IterateShaders ([&] (GVK::ShaderModule& shaderModule) {
            shaderSources.push_back (shaderModule.GetSourceCode ());
        });
    }

    IterateShaders ([&] (GVK::ShaderModule& shaderModule) {
        const std::vector<VkDescriptorSetLayoutBinding> layoutPart = RG::FromShaderReflection::GetLayout (shaderModule.GetReflection (), shaderModule.GetShaderKind ());
        layout.insert (layout.end (), layoutPart.begin (), layoutPart.end ());
    });

    return std::make_unique<GVK::DescriptorSetLayout> (device_, layout);
}

} // namespace RG
