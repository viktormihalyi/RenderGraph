#include "TestEnvironment.hpp"

#include "RenderGraph/Drawable/DrawableInfo.hpp"
#include "RenderGraph/GraphSettings.hpp"
#include "RenderGraph/Operation.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "RenderGraph/Resource.hpp"
#include "RenderGraph/ShaderPipeline.hpp"
#include "RenderGraph/VulkanEnvironment.hpp"
#include "RenderGraph/Window/Window.hpp"

#include "RenderGraph/Utils/Utils.hpp"

#include "RenderGraph/VulkanWrapper/Allocator.hpp"
#include "RenderGraph/VulkanWrapper/DeviceExtra.hpp"
#include "RenderGraph/VulkanWrapper/Utils/ImageData.hpp"
#include "RenderGraph/VulkanWrapper/Utils/VulkanUtils.hpp"
#include "RenderGraph/VulkanWrapper/VulkanWrapper.hpp"
#include "RenderGraph/VulkanWrapper/Commands.hpp"

#include <iostream>
#include <memory>
#include <string>


using RenderGraphAbstractionTest = HeadlessTestEnvironment;


TEST_F (RenderGraphAbstractionTest, NoRG)
{
    GVK::DeviceExtra& device = *env->deviceExtra;

    const std::string vertSrc = passThroughVertexShader;

    auto sp = std::make_unique<RG::ShaderPipeline> (device);
    sp->SetVertexShaderFromString (vertSrc);
    sp->SetFragmentShaderFromString (R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 outColor;

void main () {
    outColor = vec4 (1, 0, 0, 0.5);
}
    )");

    auto sp2 = std::make_unique<RG::ShaderPipeline> (device);
    sp2->SetVertexShaderFromString (vertSrc);
    sp2->SetFragmentShaderFromString (R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 outColor;

void main () {
    outColor = vec4 (0, 0, 1, 0.5);
}
    )");

    GVK::Image2D renderTarget (*env->allocator,
                               GVK::Image2D::MemoryLocation::GPU,
                               512,
                               512,
                               VK_FORMAT_R8G8B8A8_SRGB,
                               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                               1);

    GVK::ImageView2D renderTargetView (*env->device, renderTarget, 0, 1);
    GVK::ImageView2D renderTargetView2 (*env->device, renderTarget, 0, 1);

    GVK::DescriptorPool pool (device,
                              std::vector<VkDescriptorPoolSize> {
                                  { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
                                  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 },
                              },
                              1);

    GVK::DescriptorSetLayout setLayout (device, {});
    GVK::DescriptorSet       set (device, pool, setLayout);

    VkAttachmentDescription attDesc1 = {};
    attDesc1.flags                   = 0;
    attDesc1.format                  = VK_FORMAT_R8G8B8A8_SRGB;
    attDesc1.samples                 = VK_SAMPLE_COUNT_1_BIT;
    attDesc1.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc1.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc1.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc1.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc1.initialLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attDesc1.finalLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription attDesc2 = {};
    attDesc2.flags                   = 0;
    attDesc2.format                  = VK_FORMAT_R8G8B8A8_SRGB;
    attDesc2.samples                 = VK_SAMPLE_COUNT_1_BIT;
    attDesc2.loadOp                  = VK_ATTACHMENT_LOAD_OP_LOAD;
    attDesc2.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc2.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc2.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc2.initialLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attDesc2.finalLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attRef1 = {};
    attRef1.attachment            = 0;
    attRef1.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attRef2 = {};
    attRef2.attachment            = 0;
    attRef2.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    RG::ShaderPipeline::CompileSettings shaderPipelineSettings;
    shaderPipelineSettings.width                  = 512;
    shaderPipelineSettings.height                 = 512;
    shaderPipelineSettings.layout                 = setLayout.operator VkDescriptorSetLayout ();
    shaderPipelineSettings.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    shaderPipelineSettings.attachmentDescriptions = { attDesc1 };
    shaderPipelineSettings.attachmentReferences   = { attRef1 };
    shaderPipelineSettings.blendEnabled           = false;

    RG::ShaderPipeline::CompileSettings shaderPipelineSettings2;
    shaderPipelineSettings2.width                  = 512;
    shaderPipelineSettings2.height                 = 512;
    shaderPipelineSettings2.layout                 = setLayout.operator VkDescriptorSetLayout ();
    shaderPipelineSettings2.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    shaderPipelineSettings2.attachmentDescriptions = { attDesc2 };
    shaderPipelineSettings2.attachmentReferences   = { attRef2 };
    shaderPipelineSettings2.blendEnabled           = true;

    sp->Compile (std::move (shaderPipelineSettings));
    sp2->Compile (std::move (shaderPipelineSettings2));

    GVK::Framebuffer fb (*env->device, *sp->compileResult.renderPass, { renderTargetView.operator VkImageView () }, 512, 512);
    GVK::Framebuffer fb2 (*env->device, *sp2->compileResult.renderPass, { renderTargetView2.operator VkImageView () }, 512, 512);

    VkClearValue clearValue     = {};
    clearValue.color.float32[0] = 0.0f;
    clearValue.color.float32[1] = 0.0f;
    clearValue.color.float32[2] = 0.0f;
    clearValue.color.float32[3] = 1.0f;

    VkMemoryBarrier flushAllMemory = {};
    flushAllMemory.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    flushAllMemory.srcAccessMask   = VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
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
    flushAllMemory.dstAccessMask = flushAllMemory.srcAccessMask;

    std::vector<GVK::CommandBuffer> commandBuffers;

    VkImageMemoryBarrier transition            = {};
    transition.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transition.pNext                           = nullptr;
    transition.srcAccessMask                   = flushAllMemory.srcAccessMask;
    transition.dstAccessMask                   = flushAllMemory.srcAccessMask;
    transition.oldLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    transition.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    transition.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    transition.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    transition.image                           = renderTarget;
    transition.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    transition.subresourceRange.baseMipLevel   = 0;
    transition.subresourceRange.levelCount     = 1;
    transition.subresourceRange.baseArrayLayer = 0;
    transition.subresourceRange.layerCount     = 1;

    {
        GVK::CommandBuffer commandBuffer (*env->device, *env->commandPool);

        commandBuffer.Begin ();
        commandBuffer.Record<GVK::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, std::vector<VkMemoryBarrier> { flushAllMemory }, std::vector<VkBufferMemoryBarrier> {}, std::vector<VkImageMemoryBarrier> { transition });
        commandBuffer.Record<GVK::CommandBeginRenderPass> (*sp->compileResult.renderPass, fb, VkRect2D { { 0, 0 }, { 512, 512 } }, std::vector<VkClearValue> { clearValue }, VK_SUBPASS_CONTENTS_INLINE);
        commandBuffer.Record<GVK::CommandBindPipeline> (VK_PIPELINE_BIND_POINT_GRAPHICS, *sp->compileResult.pipeline);
        commandBuffer.Record<GVK::CommandDraw> (6, 1, 0, 0);
        commandBuffer.Record<GVK::CommandEndRenderPass> ();
        commandBuffer.Record<GVK::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, std::vector<VkMemoryBarrier> { flushAllMemory }, std::vector<VkBufferMemoryBarrier> {}, std::vector<VkImageMemoryBarrier> { transition });
        commandBuffer.Record<GVK::CommandBeginRenderPass> (*sp2->compileResult.renderPass, fb, VkRect2D { { 0, 0 }, { 512, 512 } }, std::vector<VkClearValue> { clearValue }, VK_SUBPASS_CONTENTS_INLINE);
        commandBuffer.Record<GVK::CommandBindPipeline> (VK_PIPELINE_BIND_POINT_GRAPHICS, *sp2->compileResult.pipeline);
        commandBuffer.Record<GVK::CommandDraw> (6, 1, 0, 0);
        commandBuffer.Record<GVK::CommandEndRenderPass> ();
        commandBuffer.Record<GVK::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, std::vector<VkMemoryBarrier> { flushAllMemory }, std::vector<VkBufferMemoryBarrier> {}, std::vector<VkImageMemoryBarrier> { transition });
        commandBuffer.End ();

        commandBuffers.push_back (std::move (commandBuffer));
    }

    {
        GVK::SingleTimeCommand cmd (*env->device, *env->commandPool, *env->graphicsQueue);

        transition.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        transition.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        cmd.Record<GVK::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                                 VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                                 std::vector<VkMemoryBarrier> { flushAllMemory },
                                                 std::vector<VkBufferMemoryBarrier> {},
                                                 std::vector<VkImageMemoryBarrier> { transition });
    }

    env->graphicsQueue->Submit ({}, {}, commandBuffers, {}, VK_NULL_HANDLE);
    env->graphicsQueue->Wait ();

    GVK::ImageData img (device, renderTarget, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    GVK::ImageData refimg (ReferenceImagesFolder / "pink.png");

    EXPECT_TRUE (refimg == img);
}


TEST_F (RenderGraphAbstractionTest, Operation_Resource)
{
    const std::string frag1 = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 outColor;

void main () {
    outColor = vec4 (1, 0, 0, 0.5);
}
    )";

    const std::string frag2 = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 outColor;

void main () {
    outColor = vec4 (0, 0, 1, 0.5);
}
    )";

    std::shared_ptr<RG::RenderOperation> renderOp = RG::RenderOperation::Builder (GetDevice ())
                                                        .SetVertices (std::make_unique<RG::DrawableInfo> (1, 6))
                                                        .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                        .SetVertexShader (passThroughVertexShader)
                                                        .SetFragmentShader (frag1)
                                                        .Build ();

    std::shared_ptr<RG::RenderOperation> renderOp2 = RG::RenderOperation::Builder (GetDevice ())
                                                         .SetVertices (std::make_unique<RG::DrawableInfo> (1, 6))
                                                         .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                         .SetVertexShader (passThroughVertexShader)
                                                         .SetFragmentShader (frag2)
                                                         .Build ();


    std::shared_ptr<RG::WritableImageResource> renderTarget = std::make_shared<RG::WritableImageResource> (VK_FILTER_LINEAR, 512, 512, 1, VK_FORMAT_R8G8B8A8_SRGB);

    RG::GraphSettings s;
    s.framesInFlight = 1;
    s.device         = env->deviceExtra.get ();

    renderTarget->Compile (s);

    auto& aTable = renderOp->compileSettings.attachmentProvider;
    aTable->table.push_back ({ "outColor", GVK::ShaderKind::Fragment, { [] () -> VkFormat { return VK_FORMAT_R8G8B8A8_SRGB; }, VK_ATTACHMENT_LOAD_OP_CLEAR, renderTarget->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } });

    s.connectionSet.Add (renderOp, renderTarget);

    auto& aTable2 = renderOp2->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "outColor", GVK::ShaderKind::Fragment, { [] () -> VkFormat { return VK_FORMAT_R8G8B8A8_SRGB; }, VK_ATTACHMENT_LOAD_OP_LOAD, renderTarget->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } });

    s.connectionSet.Add (renderOp2, renderTarget);


    renderOp->CompileWithExtent (s, 512, 512);
    renderOp2->CompileWithExtent (s, 512, 512);

    VkClearValue clearValue     = {};
    clearValue.color.float32[0] = 0.0f;
    clearValue.color.float32[1] = 0.0f;
    clearValue.color.float32[2] = 0.0f;
    clearValue.color.float32[3] = 1.0f;

    VkMemoryBarrier flushAllMemory = {};
    flushAllMemory.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    flushAllMemory.srcAccessMask   = VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
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
    flushAllMemory.dstAccessMask = flushAllMemory.srcAccessMask;

    std::vector<GVK::CommandBuffer> commandBuffers;

    VkImageMemoryBarrier transition            = {};
    transition.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transition.pNext                           = nullptr;
    transition.srcAccessMask                   = flushAllMemory.srcAccessMask;
    transition.dstAccessMask                   = flushAllMemory.srcAccessMask;
    transition.oldLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    transition.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    transition.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    transition.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    transition.image                           = *renderTarget->images[0]->image;
    transition.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    transition.subresourceRange.baseMipLevel   = 0;
    transition.subresourceRange.levelCount     = 1;
    transition.subresourceRange.baseArrayLayer = 0;
    transition.subresourceRange.layerCount     = 1;

    {
        GVK::CommandBuffer commandBuffer (*env->device, *env->commandPool);

        commandBuffer.Begin ();
        commandBuffer.Record<GVK::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, std::vector<VkMemoryBarrier> { flushAllMemory }, std::vector<VkBufferMemoryBarrier> {}, std::vector<VkImageMemoryBarrier> { transition });
        commandBuffer.Record<GVK::CommandBeginRenderPass> (*renderOp->compileSettings.pipeline->compileResult.renderPass, *renderOp->compileResult.framebuffers[0], VkRect2D { { 0, 0 }, { 512, 512 } }, std::vector<VkClearValue> { clearValue }, VK_SUBPASS_CONTENTS_INLINE);
        commandBuffer.Record<GVK::CommandBindPipeline> (VK_PIPELINE_BIND_POINT_GRAPHICS, *renderOp->compileSettings.pipeline->compileResult.pipeline);
        commandBuffer.Record<GVK::CommandDraw> (6, 1, 0, 0);
        commandBuffer.Record<GVK::CommandEndRenderPass> ();
        commandBuffer.Record<GVK::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, std::vector<VkMemoryBarrier> { flushAllMemory }, std::vector<VkBufferMemoryBarrier> {}, std::vector<VkImageMemoryBarrier> { transition });
        commandBuffer.Record<GVK::CommandBeginRenderPass> (*renderOp2->compileSettings.pipeline->compileResult.renderPass, *renderOp2->compileResult.framebuffers[0], VkRect2D { { 0, 0 }, { 512, 512 } }, std::vector<VkClearValue> { clearValue }, VK_SUBPASS_CONTENTS_INLINE);
        commandBuffer.Record<GVK::CommandBindPipeline> (VK_PIPELINE_BIND_POINT_GRAPHICS, *renderOp2->compileSettings.pipeline->compileResult.pipeline);
        commandBuffer.Record<GVK::CommandDraw> (6, 1, 0, 0);
        commandBuffer.Record<GVK::CommandEndRenderPass> ();
        commandBuffer.Record<GVK::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, std::vector<VkMemoryBarrier> { flushAllMemory }, std::vector<VkBufferMemoryBarrier> {}, std::vector<VkImageMemoryBarrier> { transition });
        commandBuffer.End ();

        commandBuffers.push_back (std::move (commandBuffer));
    }

    {
        GVK::SingleTimeCommand cmd (*env->device, *env->commandPool, *env->graphicsQueue);

        transition.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        transition.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        cmd.Record<GVK::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                                 VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                                 std::vector<VkMemoryBarrier> { flushAllMemory },
                                                 std::vector<VkBufferMemoryBarrier> {},
                                                 std::vector<VkImageMemoryBarrier> { transition });
    }

    env->graphicsQueue->Submit ({}, {}, commandBuffers, {}, VK_NULL_HANDLE);
    env->graphicsQueue->Wait ();

    GVK::ImageData img (GetDeviceExtra (), *renderTarget->images[0]->image, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    GVK::ImageData refimg (ReferenceImagesFolder / "pink.png");

    EXPECT_TRUE (refimg == img);
}


TEST_F (RenderGraphAbstractionTest, CommandBuffer)
{
    const std::string frag1 = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 outColor;

void main () {
    outColor = vec4 (1, 0, 0, 0.5);
}
    )";

    const std::string frag2 = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 outColor;

void main () {
    outColor = vec4 (0, 0, 1, 0.5);
}
    )";

    std::shared_ptr<RG::WritableImageResource> renderTarget = std::make_shared<RG::WritableImageResource> (VK_FILTER_LINEAR, 512, 512, 1, VK_FORMAT_R8G8B8A8_SRGB);

    std::shared_ptr<RG::RenderOperation> renderOp = RG::RenderOperation::Builder (GetDevice ())
                                                        .SetVertices (std::make_unique<RG::DrawableInfo> (1, 6))
                                                        .SetVertexShader (passThroughVertexShader)
                                                        .SetFragmentShader (frag1)
                                                        .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                        .Build ();

    std::shared_ptr<RG::RenderOperation> renderOp2 = RG::RenderOperation::Builder (GetDevice ())
                                                         .SetVertices (std::make_unique<RG::DrawableInfo> (1, 6))
                                                         .SetVertexShader (passThroughVertexShader)
                                                         .SetFragmentShader (frag2)
                                                         .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                         .Build ();

    RG::GraphSettings s;

    s.framesInFlight = 1;
    s.device         = env->deviceExtra.get ();

    auto& aTable = renderOp->compileSettings.attachmentProvider;
    aTable->table.push_back ({ "outColor", GVK::ShaderKind::Fragment, { renderTarget->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, renderTarget->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, renderTarget->GetFinalLayout () } });

    s.connectionSet.Add (renderOp, renderTarget);

    auto& aTable2 = renderOp2->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "outColor", GVK::ShaderKind::Fragment, { renderTarget->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_LOAD, renderTarget->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, renderTarget->GetFinalLayout () } });

    s.connectionSet.Add (renderOp2, renderTarget);

    RG::RenderGraph rg;

    rg.Compile (std::move (s));

    VkClearValue clearValue     = {};
    clearValue.color.float32[0] = 0.0f;
    clearValue.color.float32[1] = 0.0f;
    clearValue.color.float32[2] = 0.0f;
    clearValue.color.float32[3] = 1.0f;

    VkMemoryBarrier flushAllMemory = {};
    flushAllMemory.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    flushAllMemory.srcAccessMask   = VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
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
    flushAllMemory.dstAccessMask = flushAllMemory.srcAccessMask;


    std::vector<GVK::CommandBuffer> commandBuffers;

    const VkImageMemoryBarrier transition = renderTarget->images[0]->image->GetBarrier (VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                                                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                                                        flushAllMemory.srcAccessMask,
                                                                                        flushAllMemory.srcAccessMask);

    {
        GVK::CommandBuffer commandBuffer (*env->device, *env->commandPool);

        commandBuffer.Begin ();
        commandBuffer.Record<GVK::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, std::vector<VkMemoryBarrier> { flushAllMemory }, std::vector<VkBufferMemoryBarrier> {}, std::vector<VkImageMemoryBarrier> { transition });
        commandBuffer.Record<GVK::CommandBeginRenderPass> (*renderOp->compileSettings.pipeline->compileResult.renderPass, *renderOp->compileResult.framebuffers[0], VkRect2D { { 0, 0 }, { 512, 512 } }, std::vector<VkClearValue> { clearValue }, VK_SUBPASS_CONTENTS_INLINE);
        commandBuffer.Record<GVK::CommandBindPipeline> (VK_PIPELINE_BIND_POINT_GRAPHICS, *renderOp->compileSettings.pipeline->compileResult.pipeline);
        commandBuffer.Record<GVK::CommandDraw> (6, 1, 0, 0);
        commandBuffer.Record<GVK::CommandEndRenderPass> ();
        commandBuffer.Record<GVK::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, std::vector<VkMemoryBarrier> { flushAllMemory }, std::vector<VkBufferMemoryBarrier> {}, std::vector<VkImageMemoryBarrier> { transition });
        commandBuffer.Record<GVK::CommandBeginRenderPass> (*renderOp2->compileSettings.pipeline->compileResult.renderPass, *renderOp2->compileResult.framebuffers[0], VkRect2D { { 0, 0 }, { 512, 512 } }, std::vector<VkClearValue> { clearValue }, VK_SUBPASS_CONTENTS_INLINE);
        commandBuffer.Record<GVK::CommandBindPipeline> (VK_PIPELINE_BIND_POINT_GRAPHICS, *renderOp2->compileSettings.pipeline->compileResult.pipeline);
        commandBuffer.Record<GVK::CommandDraw> (6, 1, 0, 0);
        commandBuffer.Record<GVK::CommandEndRenderPass> ();
        commandBuffer.Record<GVK::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, std::vector<VkMemoryBarrier> { flushAllMemory }, std::vector<VkBufferMemoryBarrier> {}, std::vector<VkImageMemoryBarrier> {});
        commandBuffer.End ();

        commandBuffers.push_back (std::move (commandBuffer));
    }

    ASSERT_TRUE (commandBuffers[0].recordedAbstractCommands.size () == rg.commandBuffers[0].recordedAbstractCommands.size ());
    for (size_t i = 0; i < rg.commandBuffers[0].recordedAbstractCommands.size (); ++i) {
        EXPECT_TRUE (rg.commandBuffers[0].recordedAbstractCommands[i]->IsEquivalent (*commandBuffers[0].recordedAbstractCommands[i])) << i;
    }

    //env->graphicsQueue->Submit ({}, {}, commandBuffers, {}, VK_NULL_HANDLE);
    env->graphicsQueue->Submit ({}, {}, rg.commandBuffers, {}, VK_NULL_HANDLE);

    env->graphicsQueue->Wait ();

    GVK::ImageData img (GetDeviceExtra (), *renderTarget->images[0]->image, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    GVK::ImageData refimg (ReferenceImagesFolder / "pink.png");

    EXPECT_TRUE (refimg == img);
}


TEST_F (RenderGraphAbstractionTest, FullRG)
{
    GVK::DeviceExtra& device = *env->deviceExtra;

    auto sp = std::make_unique<RG::ShaderPipeline> (device);
    sp->SetVertexShaderFromString (passThroughVertexShader);
    sp->SetFragmentShaderFromString (R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 outColor;

void main () {
    outColor = vec4 (1, 0, 0, 0.5);
}
    )");

    auto sp2 = std::make_unique<RG::ShaderPipeline> (device);
    sp2->SetVertexShaderFromString (passThroughVertexShader);
    sp2->SetFragmentShaderFromString (R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 outColor;

void main () {
    outColor = vec4 (0, 0, 1, 0.5);
}
    )");

    std::shared_ptr<RG::WritableImageResource> renderTarget = std::make_shared<RG::WritableImageResource> (VK_FILTER_LINEAR, 512, 512, 1, VK_FORMAT_R8G8B8A8_SRGB);

    std::shared_ptr<RG::RenderOperation> renderOp  = std::make_shared<RG::RenderOperation> (std::make_unique<RG::DrawableInfo> (1, 6), std::move (sp));
    std::shared_ptr<RG::RenderOperation> renderOp2 = std::make_shared<RG::RenderOperation> (std::make_unique<RG::DrawableInfo> (1, 6), std::move (sp2));

    RG::GraphSettings s;

    s.framesInFlight = 1;
    s.device         = env->deviceExtra.get ();

    auto& aTable = renderOp->compileSettings.attachmentProvider;
    aTable->table.push_back ({ "outColor", GVK::ShaderKind::Fragment, { [] () -> VkFormat { return VK_FORMAT_R8G8B8A8_SRGB; }, VK_ATTACHMENT_LOAD_OP_CLEAR, renderTarget->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } });

    s.connectionSet.Add (renderOp, renderTarget);

    auto& aTable2 = renderOp2->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "outColor", GVK::ShaderKind::Fragment, { [] () -> VkFormat { return VK_FORMAT_R8G8B8A8_SRGB; }, VK_ATTACHMENT_LOAD_OP_LOAD, renderTarget->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } });

    s.connectionSet.Add (renderOp2, renderTarget);

    RG::RenderGraph rg;

    rg.Compile (std::move (s));

    env->graphicsQueue->Submit ({}, {}, { &rg.commandBuffers[0] }, {}, VK_NULL_HANDLE);
    env->graphicsQueue->Wait ();

    GVK::ImageData img (device, *renderTarget->images[0]->image, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    GVK::ImageData refimg (ReferenceImagesFolder / "pink.png");

    EXPECT_TRUE (refimg == img);
}
