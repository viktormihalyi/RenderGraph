#include "Operation.hpp"
#include "ShaderReflectionToDescriptor.hpp"

#include "ComputeShaderPipeline.hpp"
#include "Drawable.hpp"
#include "GraphSettings.hpp"
#include "Resource.hpp"
#include "ShaderPipeline.hpp"

#include "VulkanWrapper/CommandBuffer.hpp"
#include "VulkanWrapper/Commands.hpp"
#include "VulkanWrapper/ComputePipeline.hpp"
#include "VulkanWrapper/DescriptorSet.hpp"
#include "VulkanWrapper/DescriptorSetLayout.hpp"
#include "VulkanWrapper/Event.hpp"
#include "VulkanWrapper/Framebuffer.hpp"
#include "VulkanWrapper/GraphicsPipeline.hpp"
#include "VulkanWrapper/Image.hpp"
#include "VulkanWrapper/ImageView.hpp"
#include "VulkanWrapper/PipelineLayout.hpp"
#include "VulkanWrapper/RenderPass.hpp"
#include "VulkanWrapper/ShaderModule.hpp"

#include "spdlog/spdlog.h"

#include <memory>


namespace RG {

RenderOperation::Builder::Builder (VkDevice device)
    : device (device)
{
}


#pragma warning(disable : 26815)

RenderOperation::Builder& RenderOperation::Builder::SetPrimitiveTopology (VkPrimitiveTopology value)
{
    topology = value;
    return *this;
}


RenderOperation::Builder& RenderOperation::Builder::SetVertexShader (const std::string& value)
{
    EnsurePipelineCreated ();
    shaderPipiline->SetVertexShaderFromString (value);
    return *this;
}


RenderOperation::Builder& RenderOperation::Builder::SetFragmentShader (const std::string& value)
{
    EnsurePipelineCreated ();
    shaderPipiline->SetFragmentShaderFromString (value);
    return *this;
}


RenderOperation::Builder& RenderOperation::Builder::SetVertexShader (const std::filesystem::path& value)
{
    EnsurePipelineCreated ();
    shaderPipiline->SetShaderFromSourceFile (value);
    return *this;
}


RenderOperation::Builder& RenderOperation::Builder::SetFragmentShader (const std::filesystem::path& value)
{
    EnsurePipelineCreated ();
    shaderPipiline->SetShaderFromSourceFile (value);
    return *this;
}


RenderOperation::Builder& RenderOperation::Builder::SetVertices (std::unique_ptr<Drawable>&& value)
{
    drawable = std::move (value);
    return *this;
}


RenderOperation::Builder& RenderOperation::Builder::SetBlendEnabled (bool value)
{
    blendEnabled = value;
    return *this;
}


RenderOperation::Builder& RenderOperation::Builder::SetName (const std::string& value)
{
    name = value;
    return *this;
}


RenderOperation::Builder& RenderOperation::Builder::SetClearColor (const glm::vec4& value)
{
    clearColor = value;
    return *this;
}

#pragma warning(disable : 26815)


std::shared_ptr<RenderOperation> RenderOperation::Builder::Build ()
{
    RG_ASSERT (drawable != nullptr);
    RG_ASSERT (shaderPipiline != nullptr);

    std::shared_ptr<RenderOperation> op = std::make_shared<RenderOperation> (std::move (drawable), std::move (shaderPipiline), *topology);

    drawable                         = nullptr;
    shaderPipiline                   = nullptr;
    op->compileSettings.blendEnabled = blendEnabled;

    if (name.has_value ())
        op->SetName (*name);

    return op;
}


void RenderOperation::Builder::EnsurePipelineCreated ()
{
    if (shaderPipiline == nullptr) {
        shaderPipiline = std::make_unique<ShaderPipeline> (device);
    }
}


RenderOperation::RenderOperation (std::unique_ptr<Drawable>&& drawable, std::unique_ptr<ShaderPipeline>&& shaderPipeline, VkPrimitiveTopology topology)
    : compileSettings ({ std::move (drawable), std::move (shaderPipeline), topology })
{
    compileSettings.descriptorWriteProvider = std::make_unique<RG::FromShaderReflection::DescriptorWriteInfoTable> ();
    compileSettings.attachmentProvider      = std::make_unique<RG::FromShaderReflection::AttachmentDataTable> ();
}


namespace {

class DescriptorCounter : public RG::FromShaderReflection::IUpdateDescriptorSets {
public:
    std::vector<VkDescriptorPoolSize> poolSizes;

    uint32_t multiplier = 1;

    virtual void UpdateDescriptorSets (const std::vector<VkWriteDescriptorSet>& writes) override
    {
        for (const VkWriteDescriptorSet& write : writes) {
            VkDescriptorPoolSize& poolSize = GetPoolSizeByType (write.descriptorType);
            poolSize.descriptorCount += write.descriptorCount * multiplier;
        }
    }

private:
    VkDescriptorPoolSize& GetPoolSizeByType (VkDescriptorType type)
    {
        for (VkDescriptorPoolSize& poolSize : poolSizes) {
            if (poolSize.type == type) {
                return poolSize;
            }
        }

        poolSizes.push_back ({ type, 0 });
        return poolSizes.back ();
    }
};


class DescriptorWriter : public RG::FromShaderReflection::IUpdateDescriptorSets {
public:
    VkDevice device = VK_NULL_HANDLE;

    virtual void UpdateDescriptorSets (const std::vector<VkWriteDescriptorSet>& writes) override
    {
        for (const auto& write : writes) {
            spdlog::trace ("DescriptorWriter: dstBinding = {}, dstArrayElement = {}, descriptorCount = {}, descriptorType = {}",
                           write.dstBinding, write.dstArrayElement, write.descriptorCount, write.descriptorType);

            if (write.pImageInfo != nullptr) {
                for (uint32_t i = 0; i < write.descriptorCount; ++i) {
                    spdlog::trace ("DescriptorWriter: Image");
                }
            } else if (write.pBufferInfo != nullptr) {
                for (uint32_t i = 0; i < write.descriptorCount; ++i) {
                    spdlog::trace ("DescriptorWriter: Buffer");
                }
            }
        }
        vkUpdateDescriptorSets (device, static_cast<uint32_t> (writes.size ()), writes.data (), 0, nullptr);
    }
};


template<typename ShaderPipelineType>
static Operation::Descriptors CompileOperationDescriptors (const GraphSettings&                                    graphSettings,
                                                           RG::FromShaderReflection::IDescriptorWriteInfoProvider& writeInfoProvider,
                                                           const ShaderPipelineType&                               shaderPipeline)
{
    Operation::Descriptors result;

    result.descriptorSetLayout = shaderPipeline.CreateDescriptorSetLayout (graphSettings.GetDevice ());

    DescriptorCounter descriptorCounter;
    descriptorCounter.multiplier = graphSettings.framesInFlight;
    shaderPipeline.IterateShaders ([&] (const RG::ShaderModule& shaderModule) {
        RG::FromShaderReflection::WriteDescriptors (shaderModule.GetReflection (), VK_NULL_HANDLE, 0, shaderModule.GetShaderKind (), writeInfoProvider, descriptorCounter);
    });

    if (!descriptorCounter.poolSizes.empty ()) {
        result.descriptorPool = std::make_unique<RG::DescriptorPool> (graphSettings.GetDevice (), descriptorCounter.poolSizes, graphSettings.framesInFlight);

        DescriptorWriter descriptorWriter;
        descriptorWriter.device = graphSettings.GetDevice ();

        for (uint32_t resourceIndex = 0; resourceIndex < graphSettings.framesInFlight; ++resourceIndex) {
            std::unique_ptr<RG::DescriptorSet> descriptorSet = std::make_unique<RG::DescriptorSet> (graphSettings.GetDevice (), *result.descriptorPool, *result.descriptorSetLayout);

            shaderPipeline.IterateShaders ([&] (const RG::ShaderModule& shaderModule) {
                RG::FromShaderReflection::WriteDescriptors (shaderModule.GetReflection (), *descriptorSet, resourceIndex, shaderModule.GetShaderKind (), writeInfoProvider, descriptorWriter);
            });

            result.descriptorSets.push_back (std::move (descriptorSet));
        }
    }

    return result;
}

} // namespace


void RenderOperation::Compile (const GraphSettings& /* graphSettings */)
{
    RG_BREAK ();
    throw std::runtime_error ("RenderOperations should be compiled with extent.");
}


void RenderOperation::CompileWithExtent (const GraphSettings& graphSettings, uint32_t width, uint32_t height)
{
    compileResult.descriptors = CompileOperationDescriptors (graphSettings, *compileSettings.descriptorWriteProvider, *compileSettings.pipeline);

    std::vector<std::vector<VkImageView>> imageViews;
    for (uint32_t resourceIndex = 0; resourceIndex < graphSettings.framesInFlight; ++resourceIndex) {
        imageViews.push_back (RG::FromShaderReflection::GetImageViews (GetShaderPipeline ()->GetReflection (RG::ShaderKind::Fragment), RG::ShaderKind::Fragment, resourceIndex, *compileSettings.attachmentProvider));
    }

    const std::vector<VkAttachmentReference>   attachmentReferences      = RG::FromShaderReflection::GetAttachmentReferences (GetShaderPipeline ()->GetReflection (RG::ShaderKind::Fragment), RG::ShaderKind::Fragment, *compileSettings.attachmentProvider);
    const std::vector<VkAttachmentReference>   inputAttachmentReferences = RG::FromShaderReflection::GetInputAttachmentReferences (GetShaderPipeline ()->GetReflection (RG::ShaderKind::Fragment), RG::ShaderKind::Fragment, *compileSettings.attachmentProvider, static_cast<uint32_t> (attachmentReferences.size ()));
    const std::vector<VkAttachmentDescription> attachmentDescriptions    = RG::FromShaderReflection::GetAttachmentDescriptions (GetShaderPipeline ()->GetReflection (RG::ShaderKind::Fragment), RG::ShaderKind::Fragment, *compileSettings.attachmentProvider);

    ShaderPipeline::CompileSettings pipelineSettings { width,
                                                       height,
                                                       compileResult.descriptors.descriptorSetLayout->operator VkDescriptorSetLayout (),
                                                       attachmentReferences,
                                                       inputAttachmentReferences,
                                                       attachmentDescriptions,
                                                       compileSettings.topology,
                                                       compileSettings.blendEnabled };

    GetShaderPipeline ()->Compile (std::move (pipelineSettings));

    for (uint32_t resourceIndex = 0; resourceIndex < graphSettings.framesInFlight; ++resourceIndex) {
        compileResult.framebuffers.push_back (std::make_unique<RG::Framebuffer> (graphSettings.GetDevice (),
                                                                                  *GetShaderPipeline ()->compileResult.renderPass,
                                                                                  imageViews[resourceIndex],
                                                                                  width,
                                                                                  height));
    }

    compileResult.width  = width;
    compileResult.height = height;
}


void RenderOperation::Record (const ConnectionSet&, uint32_t resourceIndex, RG::CommandBuffer& commandBuffer)
{
    uint32_t outputCount = 0;
    for (const auto& output : GetShaderPipeline ()->GetReflection (RG::ShaderKind::Fragment).outputs) {
        outputCount += output.arraySize;
    }

    VkClearValue clearColor     = {};
    clearColor.color.float32[0] = 0.0f;
    clearColor.color.float32[1] = 0.0f;
    clearColor.color.float32[2] = 0.0f;
    clearColor.color.float32[3] = 1.0f;
    clearColor                  = compileSettings.clearColor.has_value ()
                                      ? VkClearValue {
                           compileSettings.clearColor->x,
                           compileSettings.clearColor->y,
                           compileSettings.clearColor->z,
                           compileSettings.clearColor->w
                       }
                                      : VkClearValue { 0.0f, 0.0f, 0.0f, 1.0f };

    std::vector<VkClearValue> clearValues (outputCount, clearColor);

    RG_ASSERT (GetShaderPipeline () != nullptr);

    commandBuffer.Record<RG::CommandBeginRenderPass> (*GetShaderPipeline ()->compileResult.renderPass,
                                                       *compileResult.framebuffers[resourceIndex],
                                                       VkRect2D { { 0, 0 }, { compileResult.width, compileResult.height } },
                                                       clearValues,
                                                       VK_SUBPASS_CONTENTS_INLINE)
        .SetName ("RenderOperation - Renderpass Begin");

    commandBuffer.Record<RG::CommandBindPipeline> (VK_PIPELINE_BIND_POINT_GRAPHICS, *GetShaderPipeline ()->compileResult.pipeline).SetName ("RenderOperation - Bind");

    if (!compileResult.descriptors.descriptorSets.empty ()) {
        VkDescriptorSet dsHandle = *compileResult.descriptors.descriptorSets[resourceIndex];

        commandBuffer.Record<RG::CommandBindDescriptorSets> (
                         VK_PIPELINE_BIND_POINT_GRAPHICS,
                         *GetShaderPipeline ()->compileResult.pipelineLayout,
                         0,
                         std::vector<VkDescriptorSet> { dsHandle },
                         std::vector<uint32_t> {})
            .SetName ("RenderOperation - DescriptionSet");
    }

    RG_ASSERT (compileSettings.drawable != nullptr);
    compileSettings.drawable->Record (commandBuffer);

    commandBuffer.Record<RG::CommandEndRenderPass> ().SetName ("RenderOperation - Renderpass End");
}


VkImageLayout RenderOperation::GetImageLayoutAtStartForInputs (Resource&)
{
    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}


VkImageLayout RenderOperation::GetImageLayoutAtEndForInputs (Resource&)
{
    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}


VkImageLayout RenderOperation::GetImageLayoutAtStartForOutputs (Resource& res)
{
    if (auto sw = dynamic_cast<SwapchainImageResource*> (&res)) {
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    } else {
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
}


VkImageLayout RenderOperation::GetImageLayoutAtEndForOutputs (Resource& res)
{
    if (auto sw = dynamic_cast<SwapchainImageResource*> (&res)) {
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    } else {
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
}


ComputeOperation::ComputeOperation (uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    : groupCountX { groupCountX }
    , groupCountY { groupCountY }
    , groupCountZ { groupCountZ }
{
    compileSettings.descriptorWriteProvider = std::make_unique<RG::FromShaderReflection::DescriptorWriteInfoTable> ();
    compileSettings.attachmentProvider      = std::make_unique<RG::FromShaderReflection::AttachmentDataTable> ();
}


void ComputeOperation::Compile (const GraphSettings& graphSettings)
{
    compileResult.descriptors = CompileOperationDescriptors (graphSettings, *compileSettings.descriptorWriteProvider, *compileSettings.computeShaderPipeline);

    const RG::ShaderModule& computeShader = *compileSettings.computeShaderPipeline->computeShader;

    const std::vector<VkAttachmentReference>   attachmentReferences      = RG::FromShaderReflection::GetAttachmentReferences (computeShader.GetReflection (), RG::ShaderKind::Compute, *compileSettings.attachmentProvider);
    const std::vector<VkAttachmentReference>   inputAttachmentReferences = RG::FromShaderReflection::GetInputAttachmentReferences (computeShader.GetReflection (), RG::ShaderKind::Compute, *compileSettings.attachmentProvider, static_cast<uint32_t> (attachmentReferences.size ()));
    const std::vector<VkAttachmentDescription> attachmentDescriptions    = RG::FromShaderReflection::GetAttachmentDescriptions (computeShader.GetReflection (), RG::ShaderKind::Compute, *compileSettings.attachmentProvider);

    ComputeShaderPipeline::CompileSettings pipelineSettings { compileResult.descriptors.descriptorSetLayout->operator VkDescriptorSetLayout (),
                                                              attachmentReferences,
                                                              inputAttachmentReferences,
                                                              attachmentDescriptions };

    compileSettings.computeShaderPipeline->Compile (std::move (pipelineSettings));
}


void ComputeOperation::CompileWithExtent (const GraphSettings&, uint32_t, uint32_t)
{
    RG_BREAK ();
    throw std::runtime_error ("ComputeOperations should be compiled without extent.");
}


void ComputeOperation::Record (const ConnectionSet&, uint32_t resourceIndex, RG::CommandBuffer& commandBuffer)
{
    commandBuffer.Record<RG::CommandBindPipeline> (VK_PIPELINE_BIND_POINT_COMPUTE, *compileSettings.computeShaderPipeline->compileResult.pipeline).SetName ("ComputeOperation - Bind");

    if (!compileResult.descriptors.descriptorSets.empty ()) {
        VkDescriptorSet dsHandle = *compileResult.descriptors.descriptorSets[resourceIndex];

        commandBuffer.Record<RG::CommandBindDescriptorSets> (
                         VK_PIPELINE_BIND_POINT_COMPUTE,
                         *compileSettings.computeShaderPipeline->compileResult.pipelineLayout,
                         0,
                         std::vector<VkDescriptorSet> { dsHandle },
                         std::vector<uint32_t> {})
            .SetName ("ComputeOperation - DescriptionSet");
    }

    commandBuffer.Record<RG::CommandDispatch> (groupCountX, groupCountY, groupCountZ).SetName ("ComputeOperation - CommandDispatch");
}


} // namespace RG
