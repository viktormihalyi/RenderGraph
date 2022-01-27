#ifndef RENDERGRAPH_SHADERREFLECTIONTODESCRIPTOR_HPP
#define RENDERGRAPH_SHADERREFLECTIONTODESCRIPTOR_HPP

#include "RenderGraphExport.hpp"

#include "RenderGraph/VulkanWrapper/ShaderModule.hpp"

#include <vulkan/vulkan.h>

#include <functional>
#include <string>
#include <vector>
#include <cstdint>


namespace RG {
namespace FromShaderReflection {

class RENDERGRAPH_DLL_EXPORT IDescriptorWriteInfoProvider {
public:
    virtual ~IDescriptorWriteInfoProvider ();

    virtual std::vector<VkDescriptorImageInfo>  GetDescriptorImageInfos (const std::string& name, RG::ShaderKind shaderKind, uint32_t layerIndex, uint32_t frameIndex) = 0;
    virtual std::vector<VkDescriptorBufferInfo> GetDescriptorBufferInfos (const std::string& name, RG::ShaderKind shaderKind, uint32_t frameIndex)                     = 0;
};


class RENDERGRAPH_DLL_EXPORT DescriptorWriteInfoTable : public IDescriptorWriteInfoProvider {
public:
    struct ImageEntry {
        std::string                                     name;
        RG::ShaderKind                                 shaderKind;
        std::function<VkSampler ()>                     sampler;
        std::function<VkImageView (uint32_t, uint32_t)> imageView;
        VkImageLayout                                   imageLayout;
    };

    struct BufferEntry {
        std::string                        name;
        RG::ShaderKind                    shaderKind;
        std::function<VkBuffer (uint32_t)> buffer;
        VkDeviceSize                       offset;
        VkDeviceSize                       range;
    };

    std::vector<ImageEntry> imageInfos;
    std::vector<BufferEntry> bufferInfos;

    virtual ~DescriptorWriteInfoTable () override = default;

    virtual std::vector<VkDescriptorImageInfo> GetDescriptorImageInfos (const std::string& name, RG::ShaderKind shaderKind, uint32_t layerIndex, uint32_t frameIndex) override;

    virtual std::vector<VkDescriptorBufferInfo> GetDescriptorBufferInfos (const std::string& name, RG::ShaderKind shaderKind, uint32_t frameIndex) override;
};


class RENDERGRAPH_DLL_EXPORT IUpdateDescriptorSets {
public:
    virtual ~IUpdateDescriptorSets ();

    virtual void UpdateDescriptorSets (const std::vector<VkWriteDescriptorSet>& writes) = 0;
};


RENDERGRAPH_DLL_EXPORT
void WriteDescriptors (const RG::ShaderModuleReflection& reflection,
                       VkDescriptorSet                      dstSet,
                       uint32_t                             frameIndex,
                       RG::ShaderKind                      shaderKind,
                       IDescriptorWriteInfoProvider&        infoProvider,
                       IUpdateDescriptorSets&               updateInterface);


RENDERGRAPH_DLL_EXPORT
std::vector<VkDescriptorSetLayoutBinding> GetLayout (const RG::ShaderModuleReflection& reflection, RG::ShaderKind shaderKind);

} // namespace FromShaderReflection
} // namespace RG

#endif
