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
    computeShader = GVK::ShaderModule::CreateFromGLSLFile (device, path);
}


ComputeShaderPipeline::ComputeShaderPipeline (VkDevice device, const std::string& source)
    : device (device)
{
    computeShader = GVK::ShaderModule::CreateFromGLSLString (device, GVK::ShaderKind::Compute, source);
}


void ComputeShaderPipeline::Compile (CompileSettings&& settings_)
{
    compileSettings = std::move (settings_);
    compileResult.Clear ();

    compileResult.pipelineLayout = std::unique_ptr<GVK::PipelineLayout> (new GVK::PipelineLayout (device, { compileSettings.layout }));

    compileResult.pipeline = std::unique_ptr<GVK::ComputePipeline> (new GVK::ComputePipeline (
        device,
        *compileResult.pipelineLayout,
        *computeShader));
}


void ComputeShaderPipeline::IterateShaders (const std::function<void(const GVK::ShaderModule&)> iterator) const
{
    if (computeShader != nullptr)
        iterator (*computeShader);
}


std::unique_ptr<GVK::DescriptorSetLayout> ComputeShaderPipeline::CreateDescriptorSetLayout (VkDevice device_) const
{
    return std::make_unique<GVK::DescriptorSetLayout> (device_, RG::FromShaderReflection::GetLayout (computeShader->GetReflection (), computeShader->GetShaderKind ()));
}

} // namespace RG
