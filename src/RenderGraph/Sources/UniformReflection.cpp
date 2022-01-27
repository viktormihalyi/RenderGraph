#include "UniformReflection.hpp"
#include "Drawable.hpp"
#include "ShaderPipeline.hpp"
#include "ComputeShaderPipeline.hpp"
#include "Operation.hpp"
#include "ShaderReflectionToDescriptor.hpp"

#include "VulkanWrapper/Framebuffer.hpp"
#include "VulkanWrapper/GraphicsPipeline.hpp"
#include "VulkanWrapper/ComputePipeline.hpp"
#include "VulkanWrapper/PipelineLayout.hpp"
#include "VulkanWrapper/RenderPass.hpp"
#include "VulkanWrapper/PipelineLayout.hpp"
#include "VulkanWrapper/DescriptorSet.hpp"
#include "VulkanWrapper/DescriptorSetLayout.hpp"

#include "Utils/Event.hpp"
#include "Utils/Utils.hpp"

#include "spdlog/spdlog.h"

namespace RG {


UniformReflection::UniformReflection (RG::ConnectionSet& connectionSet, const ResourceCreator& resourceCreator)
{
    // TODO properly handle swapchain recreate

    CreateGraphResources (connectionSet, resourceCreator);
    CreateGraphConnections (connectionSet);
}


void UniformReflection::Flush (uint32_t frameIndex)
{
    RG::ForEach<RG::CPUBufferResource> (bufferObjectResources, [&] (const std::shared_ptr<RG::CPUBufferResource>& bufferObjectRes) {
        const std::shared_ptr<RG::Refl::IBufferData> bufferObjectData = udatas.at (bufferObjectRes->GetUUID ());

        bufferObjectRes->GetMapping (frameIndex).Copy (bufferObjectData->GetData (), bufferObjectData->GetSize ());
    });
}


void UniformReflection::CreateGraphResources (const RG::ConnectionSet& connectionSet, const ResourceCreator& resourceCreator)
{
    // RG_ASSERT (!graph.operations.empty ());

    const auto CreateBufferObjectResource = [&] (const std::shared_ptr<RG::Operation>& op, const RG::ShaderModule& shaderModule, const std::shared_ptr<RG::Refl::BufferObject>& bufferObject, BufferObjectSelector& bufferObjectsel) {

        bool treatAsOutput = false;

        std::shared_ptr<RG::DescriptorBindableBufferResource> bufferObjectRes = resourceCreator (op, shaderModule, bufferObject, treatAsOutput);

        if (bufferObjectRes == nullptr)
            return;

        if (connectionSet.GetNodeByName (bufferObject->name) != nullptr) {
            spdlog::trace ("[UniformReflection] Skipping buffer object named \"{}\" because it already exists.", bufferObject->name);
            return;
        }

        bufferObjectRes->SetName (bufferObject->name);
        bufferObjectRes->SetDebugInfo ("Made by UniformReflection.");

        std::shared_ptr<RG::Refl::BufferDataInternal> bufferObjectData = std::make_unique<RG::Refl::BufferDataInternal> (bufferObject);
        bufferObjectsel.Set (bufferObject->name, bufferObjectData);

        bufferObjectConnections.push_back (std::make_tuple (op, bufferObject, bufferObjectRes, shaderModule.GetShaderKind (), treatAsOutput));
        bufferObjectResources.push_back (bufferObjectRes);
        udatas.insert ({ bufferObjectRes->GetUUID (), bufferObjectData });
    };

    const auto CreateBufferObjectsFromShader = [&] (const std::shared_ptr<Operation>& op, const RG::ShaderModule& shaderModule, ShaderKindSelector& newShaderKindSelector) {
        BufferObjectSelector newBufferObjectSelector;
        for (const std::shared_ptr<RG::Refl::BufferObject>& ubo : shaderModule.GetReflection ().ubos) {
            CreateBufferObjectResource (op, shaderModule, ubo, newBufferObjectSelector);
        }
        for (const std::shared_ptr<RG::Refl::BufferObject>& storageBuffer : shaderModule.GetReflection ().storageBuffers) {
            CreateBufferObjectResource (op, shaderModule, storageBuffer, newBufferObjectSelector);
        }
        newShaderKindSelector.Set (shaderModule.GetShaderKind (), std::move (newBufferObjectSelector));
    };

    RG::ForEach<RG::RenderOperation> (connectionSet.GetNodesByInsertionOrder (), [&] (const std::shared_ptr<RG::RenderOperation>& renderOp) {
        ShaderKindSelector newShaderKindSelector;
        renderOp->GetShaderPipeline ()->IterateShaders ([&] (const RG::ShaderModule& shaderModule) {
            CreateBufferObjectsFromShader (renderOp, shaderModule, newShaderKindSelector);
        });
        selectors.emplace (renderOp->GetUUID (), std::move (newShaderKindSelector));
    });

    RG::ForEach<RG::ComputeOperation> (connectionSet.GetNodesByInsertionOrder (), [&] (const std::shared_ptr<RG::ComputeOperation>& computeOp) {
        ShaderKindSelector newShaderKindSelector;
        computeOp->compileSettings.computeShaderPipeline->IterateShaders ([&] (const RG::ShaderModule& shaderModule) {
            CreateBufferObjectsFromShader (computeOp, shaderModule, newShaderKindSelector);
        });
        selectors.emplace (computeOp->GetUUID (), std::move (newShaderKindSelector));
    });
}


void UniformReflection::CreateGraphConnections (RG::ConnectionSet& connectionSet)
{
    RG_ASSERT (!bufferObjectConnections.empty ());

    for (auto& [operation, bufferObject, resource, shaderKind, treatAsOutput] : bufferObjectConnections) {

        if (treatAsOutput) {
            connectionSet.Add (operation, resource);
        } else {
            connectionSet.Add (resource, operation);
        }

        if (auto renderOp = std::dynamic_pointer_cast<RG::RenderOperation> (operation)) {
            auto& table = renderOp->compileSettings.descriptorWriteProvider;
            table->bufferInfos.push_back ({ bufferObject->name, shaderKind, resource->GetBufferForFrameProvider (), 0, resource->GetBufferSize () });
        } else if (auto computeOp = std::dynamic_pointer_cast<RG::ComputeOperation> (operation)) {
            auto& table = computeOp->compileSettings.descriptorWriteProvider;
            table->bufferInfos.push_back ({ bufferObject->name, shaderKind, resource->GetBufferForFrameProvider (), 0, resource->GetBufferSize () });
        } else {
            RG_BREAK ();
        }
    }

    bufferObjectConnections.clear ();
}


void UniformReflection::PrintDebugInfo ()
{
    for (auto& [uuid, selector] : selectors) {
        printf ("%s\n", uuid.GetValue ().c_str ());
        for (auto& [shaderKind, bufferObjectSelector] : selector.bufferObjectSelectors) {
            printf ("\t%s\n", ShaderKindToString (shaderKind).c_str ());
            for (auto& [name, bufferObject] : bufferObjectSelector.udatas) {
                printf ("\t\t%s (%d) - ", name.c_str (), bufferObject->GetSize ());

                for (int32_t i = bufferObject->GetSize () - 1; i >= 0; --i) {
                    printf ("%02x ", bufferObject->GetData ()[i]);
                }
                printf ("\n");
            }
        }
    }
}


ImageMap::ImageMap () = default;


std::shared_ptr<ReadOnlyImageResource> ImageMap::FindByName (const std::string& name) const
{
    for (auto& im : images) {
        if (im.first.name == name) {
            return im.second;
        }
    }
    return nullptr;
}


void ImageMap::Put (const RG::Refl::Sampler& sampler, const std::shared_ptr<ReadOnlyImageResource>& res)
{
    images.emplace_back (sampler, res);
}


ImageMap CreateEmptyImageResources (RG::ConnectionSet& connectionSet)
{
    return CreateEmptyImageResources (connectionSet, [&] (const RG::Refl::Sampler&) { return std::nullopt; });
}


ImageMap CreateEmptyImageResources (RG::ConnectionSet& connectionSet, const ExtentProviderForImageCreate& extentProvider)
{
    ImageMap result;

    const auto nodes = connectionSet.GetNodesByInsertionOrder ();
    RG::ForEach<RG::RenderOperation> (nodes, [&] (const std::shared_ptr<RG::RenderOperation>& renderOp) {
        renderOp->GetShaderPipeline ()->IterateShaders ([&] (const RG::ShaderModule& shaderModule) {
            for (const RG::Refl::Sampler& sampler : shaderModule.GetReflection ().samplers) {
                std::shared_ptr<ReadOnlyImageResource> imgRes;

                const std::optional<CreateParams> providedExtent = extentProvider (sampler);
                if (!providedExtent.has_value ()) {
                    continue;
                }

                const glm::uvec3 extent = providedExtent.has_value () ? std::get<0> (*providedExtent) : glm::uvec3 { 512, 512, 512 };
                const VkFormat   format = providedExtent.has_value () ? std::get<1> (*providedExtent) : VK_FORMAT_R8_SRGB;
                const VkFilter   filter = providedExtent.has_value () ? std::get<2> (*providedExtent) : VK_FILTER_LINEAR;

                const uint32_t layerCount = sampler.arraySize;

                switch (sampler.type) {
                    case RG::Refl::Sampler::Type::Sampler1D:
                        RG_ASSERT (!providedExtent.has_value () || (extent.x != 0 && extent.y == 0 && extent.z == 0));
                        imgRes = std::make_unique<ReadOnlyImageResource> (format, filter, extent.x, 1, 1, layerCount);
                        break;
                    case RG::Refl::Sampler::Type::Sampler2D:
                        RG_ASSERT (!providedExtent.has_value () || (extent.x != 0 && extent.y != 0 && extent.z == 0));
                        imgRes = std::make_unique<ReadOnlyImageResource> (format, filter, extent.x, extent.y, 1, layerCount);
                        break;
                    case RG::Refl::Sampler::Type::Sampler3D:
                        RG_ASSERT (!providedExtent.has_value () || (extent.x != 0 && extent.y != 0 && extent.z != 0));
                        imgRes = std::make_unique<ReadOnlyImageResource> (format, filter, extent.x, extent.y, extent.z, layerCount);
                        break;
                    default:
                        RG_BREAK_STR ("unexpected sampler type");
                        break;
                }

                if (RG_ERROR (imgRes == nullptr)) {
                    continue;
                }

                imgRes->SetName (sampler.name);
                imgRes->SetDebugInfo ("Made by ImageMap.");

                result.Put (sampler, imgRes);

                connectionSet.Add (imgRes);
                connectionSet.Add (renderOp);
                auto& table = renderOp->compileSettings.descriptorWriteProvider;
                table->imageInfos.push_back ({ sampler.name, shaderModule.GetShaderKind (), imgRes->GetSamplerProvider (), imgRes->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
            }
        });
    });

    return result;
}

} // namespace RG
