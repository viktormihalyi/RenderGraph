#include "ComputeShaderPipeline.hpp"
#include "ShaderReflectionToDescriptor.hpp"
#include "ShaderReflectionToVertexAttribute.hpp"

#include "VulkanWrapper/DescriptorSetLayout.hpp"
#include "VulkanWrapper/ComputePipeline.hpp"
#include "VulkanWrapper/PipelineLayout.hpp"
#include "VulkanWrapper/RenderPass.hpp"
#include "VulkanWrapper/ShaderModule.hpp"

#include "Utils/Assert.hpp"

#include <stdexcept>


namespace RG {


ComputeShaderPipeline::~ComputeShaderPipeline ()
{
}


void ComputeShaderPipeline::CompileResult::Clear ()
{
    pipelineLayout.reset ();
    pipeline.reset ();
}


ComputeShaderPipeline::ComputeShaderPipeline (VkDevice device)
    : device (device)
{
}


ComputeShaderPipeline::ComputeShaderPipeline (VkDevice device, const std::filesystem::path& path)
    : device (device)
{
    computeShader = RG::ShaderModule::CreateFromGLSLFile (device, path);
}


ComputeShaderPipeline::ComputeShaderPipeline (VkDevice device, const std::string& source)
    : device (device)
{
    computeShader = RG::ShaderModule::CreateFromGLSLString (device, RG::ShaderKind::Compute, source);
}


void ComputeShaderPipeline::Compile (CompileSettings&& settings_)
{
    compileSettings = std::move (settings_);
    compileResult.Clear ();

    compileResult.pipelineLayout = std::unique_ptr<RG::PipelineLayout> (new RG::PipelineLayout (device, { compileSettings.layout }));

    compileResult.pipeline = std::unique_ptr<RG::ComputePipeline> (new RG::ComputePipeline (
        device,
        *compileResult.pipelineLayout,
        *computeShader));
}


void ComputeShaderPipeline::IterateShaders (const std::function<void(const RG::ShaderModule&)> iterator) const
{
    if (computeShader != nullptr)
        iterator (*computeShader);
}


std::unique_ptr<RG::DescriptorSetLayout> ComputeShaderPipeline::CreateDescriptorSetLayout (VkDevice device_) const
{
    return std::make_unique<RG::DescriptorSetLayout> (device_, RG::FromShaderReflection::GetLayout (computeShader->GetReflection (), computeShader->GetShaderKind ()));
}

} // namespace RG
