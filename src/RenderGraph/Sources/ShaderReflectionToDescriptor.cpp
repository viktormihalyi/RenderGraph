#include "ShaderReflectionToDescriptor.hpp"

#include "VulkanWrapper/ShaderReflection.hpp"

#include "Utils/Assert.hpp"
#include "spdlog/spdlog.h"


namespace RG {
namespace FromShaderReflection {

IDescriptorWriteInfoProvider::~IDescriptorWriteInfoProvider () = default;


std::vector<VkDescriptorImageInfo> DescriptorWriteInfoTable::GetDescriptorImageInfos (const std::string& name, GVK::ShaderKind shaderKind, uint32_t layerIndex, uint32_t resourceIndex)
{
    std::vector<ImageEntry> result = imageInfos;
    result.erase (std::remove_if (result.begin (), result.end (), [&] (const auto& entry) {
                      return entry.name != name || entry.shaderKind != shaderKind;
                  }),
                  result.end ());

    std::vector<VkDescriptorImageInfo> resultInfos;

    for (const ImageEntry& entry : result)
        resultInfos.push_back ({ entry.sampler (), entry.imageView (resourceIndex, layerIndex), entry.imageLayout });

    return resultInfos;
}


std::vector<VkDescriptorBufferInfo> DescriptorWriteInfoTable::GetDescriptorBufferInfos (const std::string& name, GVK::ShaderKind shaderKind, uint32_t frameIndex)
{
    std::vector<BufferEntry> result = bufferInfos;
    result.erase (std::remove_if (result.begin (), result.end (), [&] (const auto& entry) {
                      return entry.name != name || entry.shaderKind != shaderKind;
                  }),
                  result.end ());

    std::vector<VkDescriptorBufferInfo> resultInfos;

    for (const BufferEntry& entry : result)
        resultInfos.push_back ({ entry.buffer (frameIndex), entry.offset, entry.range });

    return resultInfos;
}


IUpdateDescriptorSets::~IUpdateDescriptorSets () = default;


static void UpdateDescriptorSetsFromSamplers (const GVK::ShaderModuleReflection& reflection,
                                              VkDescriptorSet                      dstSet,
                                              uint32_t                             frameIndex,
                                              GVK::ShaderKind                      shaderKind,
                                              IDescriptorWriteInfoProvider&        infoProvider,
                                              IUpdateDescriptorSets&               updateInterface)
{
    std::vector<VkDescriptorImageInfo> imgInfos;
    std::vector<VkWriteDescriptorSet>  result;

    imgInfos.reserve (1024);
    result.reserve (1024);

    for (const SR::Sampler& sampler : reflection.samplers) {
        const uint32_t layerCount = sampler.arraySize;
        for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
            const std::vector<VkDescriptorImageInfo> tempImgInfos = infoProvider.GetDescriptorImageInfos (sampler.name, shaderKind, layerIndex, frameIndex);
            if (GVK_ERROR (tempImgInfos.empty ())) {
                spdlog::error ("Combined sampler \"{}\" (layer {}) has no descriptor bound.", sampler.name, layerIndex);
                continue;
            }

            const int32_t currentSize = static_cast<uint32_t> (imgInfos.size ());

            imgInfos.insert (imgInfos.end (), tempImgInfos.begin (), tempImgInfos.end ());

            const int32_t newSize = static_cast<uint32_t> (imgInfos.size ());

            GVK_ASSERT (newSize - currentSize == tempImgInfos.size ());
            GVK_ASSERT (newSize - currentSize > 0);

            VkWriteDescriptorSet write = {};
            write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet               = dstSet;
            write.dstBinding           = sampler.binding;
            write.dstArrayElement      = layerIndex;
            write.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount      = newSize - currentSize;
            write.pBufferInfo          = nullptr;
            write.pImageInfo           = &imgInfos[currentSize];
            write.pTexelBufferView     = nullptr;

            result.push_back (write);
        }
    }

    if (!result.empty ())
        updateInterface.UpdateDescriptorSets (result);
}


static void UpdateDescriptorSetsFromUBOs (const GVK::ShaderModuleReflection& reflection,
                                          VkDescriptorSet                      dstSet,
                                          uint32_t                             frameIndex,
                                          GVK::ShaderKind                      shaderKind,
                                          IDescriptorWriteInfoProvider&        infoProvider,
                                          IUpdateDescriptorSets&               updateInterface)
{
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet>   result;

    bufferInfos.reserve (1024);
    result.reserve (1024);

    for (const std::shared_ptr<SR::BufferObject>& ubo : reflection.ubos) {
        const std::vector<VkDescriptorBufferInfo> tempBufferInfos = infoProvider.GetDescriptorBufferInfos (ubo->name, shaderKind, frameIndex);
        if (GVK_ERROR (tempBufferInfos.empty ())) {
            spdlog::error ("Uniform block \"{}\" has no descriptor bound.", ubo->name);
            continue;
        }

        const int32_t currentSize = static_cast<uint32_t> (bufferInfos.size ());
        
        bufferInfos.insert (bufferInfos.end (), tempBufferInfos.begin (), tempBufferInfos.end ());

        const int32_t newSize = static_cast<uint32_t> (bufferInfos.size ());

        GVK_ASSERT (newSize - currentSize == tempBufferInfos.size ());
        GVK_ASSERT (newSize - currentSize > 0);

        VkWriteDescriptorSet write = {};
        write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet               = dstSet;
        write.dstBinding           = ubo->binding;
        write.dstArrayElement      = 0;
        write.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount      = newSize - currentSize;
        write.pBufferInfo          = &bufferInfos[currentSize];
        write.pImageInfo           = nullptr;
        write.pTexelBufferView     = nullptr;

        result.push_back (write);
    }

    if (!result.empty ())
        updateInterface.UpdateDescriptorSets (result);
}


static void UpdateDescriptorSetsFromStorageBuffers (const GVK::ShaderModuleReflection& reflection,
                                                     VkDescriptorSet                      dstSet,
                                                     uint32_t                             frameIndex,
                                                     GVK::ShaderKind                      shaderKind,
                                                     IDescriptorWriteInfoProvider&        infoProvider,
                                                     IUpdateDescriptorSets&               updateInterface)
{
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet>   result;

    bufferInfos.reserve (1024);
    result.reserve (1024);

    for (const std::shared_ptr<SR::BufferObject>& storageBuffer : reflection.storageBuffers) {
        const std::vector<VkDescriptorBufferInfo> tempBufferInfos = infoProvider.GetDescriptorBufferInfos (storageBuffer->name, shaderKind, frameIndex);
        if (GVK_ERROR (tempBufferInfos.empty ())) {
            spdlog::error ("Uniform block \"{}\" has no descriptor bound.", storageBuffer->name);
            continue;
        }

        const int32_t currentSize = static_cast<uint32_t> (bufferInfos.size ());

        bufferInfos.insert (bufferInfos.end (), tempBufferInfos.begin (), tempBufferInfos.end ());

        const int32_t newSize = static_cast<uint32_t> (bufferInfos.size ());

        GVK_ASSERT (newSize - currentSize == tempBufferInfos.size ());
        GVK_ASSERT (newSize - currentSize > 0);

        VkWriteDescriptorSet write = {};
        write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet               = dstSet;
        write.dstBinding           = storageBuffer->binding;
        write.dstArrayElement      = 0;
        write.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount      = newSize - currentSize;
        write.pBufferInfo          = &bufferInfos[currentSize];
        write.pImageInfo           = nullptr;
        write.pTexelBufferView     = nullptr;

        result.push_back (write);
    }

    if (!result.empty ())
        updateInterface.UpdateDescriptorSets (result);
}


static void UpdateDescriptorSetsFromInputAttachments (const GVK::ShaderModuleReflection& reflection,
                                                      VkDescriptorSet                      dstSet,
                                                      uint32_t                             frameIndex,
                                                      GVK::ShaderKind                      shaderKind,
                                                      IDescriptorWriteInfoProvider&        infoProvider,
                                                      IUpdateDescriptorSets&               updateInterface)
{
    std::vector<VkDescriptorImageInfo>  imgInfos;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet>   result;

    imgInfos.reserve (1024);
    bufferInfos.reserve (1024);
    result.reserve (1024);

    for (const SR::SubpassInput& subpassInput : reflection.subpassInputs) {
        const uint32_t layerCount = subpassInput.arraySize;
        for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
            const std::vector<VkDescriptorImageInfo> tempImgInfos = infoProvider.GetDescriptorImageInfos (subpassInput.name, shaderKind, layerIndex, frameIndex);
            if (GVK_ERROR (tempImgInfos.empty ())) {
                spdlog::error ("Input attachment \"{}\" (layer {}) has no descriptor bound.", subpassInput.name, layerIndex);
                continue;
            }

            const int32_t currentSize = static_cast<uint32_t> (imgInfos.size ());

            imgInfos.insert (imgInfos.end (), tempImgInfos.begin (), tempImgInfos.end ());

            const int32_t newSize = static_cast<uint32_t> (imgInfos.size ());

            GVK_ASSERT (newSize - currentSize == tempImgInfos.size ());
            GVK_ASSERT (newSize - currentSize > 0);

            VkWriteDescriptorSet write = {};
            write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet               = dstSet;
            write.dstBinding           = subpassInput.binding;
            write.dstArrayElement      = layerIndex;
            write.descriptorType       = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            write.descriptorCount      = newSize - currentSize;
            write.pBufferInfo          = nullptr;
            write.pImageInfo           = &imgInfos[currentSize];
            write.pTexelBufferView     = nullptr;

            result.push_back (write);
        }
    }

    if (!result.empty ())
        updateInterface.UpdateDescriptorSets (result);
}


void WriteDescriptors (const GVK::ShaderModuleReflection& reflection,
                       VkDescriptorSet                      dstSet,
                       uint32_t                             frameIndex,
                       GVK::ShaderKind                      shaderKind,
                       IDescriptorWriteInfoProvider&        infoProvider,
                       IUpdateDescriptorSets&               updateInterface)
{
    UpdateDescriptorSetsFromSamplers (reflection, dstSet, frameIndex, shaderKind, infoProvider, updateInterface);
    UpdateDescriptorSetsFromUBOs (reflection, dstSet, frameIndex, shaderKind, infoProvider, updateInterface);
    UpdateDescriptorSetsFromStorageBuffers (reflection, dstSet, frameIndex, shaderKind, infoProvider, updateInterface);
    UpdateDescriptorSetsFromInputAttachments (reflection, dstSet, frameIndex, shaderKind, infoProvider, updateInterface);
}


static VkShaderStageFlags GetShaderStageFromShaderKind (GVK::ShaderKind shaderKind)
{
    switch (shaderKind) {
        case GVK::ShaderKind::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
        case GVK::ShaderKind::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
        case GVK::ShaderKind::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
        case GVK::ShaderKind::TessellationControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case GVK::ShaderKind::TessellationEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case GVK::ShaderKind::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
        default: GVK_BREAK_STR ("unexpected shaderkind type"); return VK_SHADER_STAGE_ALL;
    }
}


std::vector<VkDescriptorSetLayoutBinding> GetLayout (const GVK::ShaderModuleReflection& reflection, GVK::ShaderKind shaderKind)
{
    std::vector<VkDescriptorSetLayoutBinding> result;

    for (const SR::Sampler& sampler : reflection.samplers) {
        VkDescriptorSetLayoutBinding bin = {};
        bin.binding                      = sampler.binding;
        bin.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bin.descriptorCount              = sampler.arraySize;
        bin.stageFlags                   = GetShaderStageFromShaderKind (shaderKind);
        bin.pImmutableSamplers           = nullptr;
        result.push_back (bin);
    }

    for (const std::shared_ptr<SR::BufferObject>& ubo : reflection.ubos) {
        VkDescriptorSetLayoutBinding bin = {};
        bin.binding                      = ubo->binding;
        bin.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bin.descriptorCount              = 1;
        bin.stageFlags                   = GetShaderStageFromShaderKind (shaderKind);
        bin.pImmutableSamplers           = nullptr;
        result.push_back (bin);
    }

    for (const std::shared_ptr<SR::BufferObject>& storageBuffer : reflection.storageBuffers) {
        VkDescriptorSetLayoutBinding bin = {};
        bin.binding                      = storageBuffer->binding;
        bin.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bin.descriptorCount              = 1;
        bin.stageFlags                   = GetShaderStageFromShaderKind (shaderKind);
        bin.pImmutableSamplers           = nullptr;
        result.push_back (bin);
    }

    for (const SR::SubpassInput& subpassInput : reflection.subpassInputs) {
        VkDescriptorSetLayoutBinding bin = {};
        bin.binding                      = subpassInput.binding;
        bin.descriptorType               = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bin.descriptorCount              = 1;
        bin.stageFlags                   = GetShaderStageFromShaderKind (shaderKind);
        bin.pImmutableSamplers           = nullptr;
        result.push_back (bin);
    }

    return result;
}

} // namespace FromShaderReflection
} // namespace RG
