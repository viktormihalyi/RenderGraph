#ifndef RENDERGRAPH_SHADERREFLECTIONTOATTACHMENT_HPP
#define RENDERGRAPH_SHADERREFLECTIONTOATTACHMENT_HPP

#include "RenderGraphExport.hpp"

#include "RenderGraph/VulkanWrapper/ShaderModule.hpp"

#include <vulkan/vulkan.h>

#include <functional>
#include <optional>


namespace RG {
namespace FromShaderReflection {

class RENDERGRAPH_DLL_EXPORT IAttachmentProvider {
public:
    virtual ~IAttachmentProvider ();

    struct AttachmentData {
        std::function<VkFormat ()>                      format;
        VkAttachmentLoadOp                              loadOp;
        std::function<VkImageView (uint32_t, uint32_t)> imageView;
        VkImageLayout                                   initialLayout;
        VkImageLayout                                   finalLayout;
    };

    virtual std::optional<AttachmentData> GetAttachmentData (const std::string& name, GVK::ShaderKind shaderKind) = 0;
};

class RENDERGRAPH_DLL_EXPORT AttachmentDataTable : public IAttachmentProvider {
public:
    struct AttachmentDataEntry {
        std::string     name;
        GVK::ShaderKind shaderKind;
        AttachmentData  data;
    };

    std::vector<AttachmentDataEntry> table;

    virtual ~AttachmentDataTable () override = default;

    virtual std::optional<AttachmentData> GetAttachmentData (const std::string& name, GVK::ShaderKind shaderKind) override;
};


RENDERGRAPH_DLL_EXPORT
std::vector<VkImageView> GetImageViews (const GVK::ShaderModuleReflection& reflection, GVK::ShaderKind shaderKind, uint32_t resourceIndex, IAttachmentProvider& attachmentProvider);


RENDERGRAPH_DLL_EXPORT
std::vector<VkAttachmentReference> GetAttachmentReferences (const GVK::ShaderModuleReflection& reflection, GVK::ShaderKind shaderKind, IAttachmentProvider& attachmentProvider);


RENDERGRAPH_DLL_EXPORT
std::vector<VkAttachmentReference> GetInputAttachmentReferences (const GVK::ShaderModuleReflection& reflection, GVK::ShaderKind shaderKind, IAttachmentProvider& attachmentProvider, uint32_t startIndex);


RENDERGRAPH_DLL_EXPORT
std::vector<VkAttachmentDescription> GetAttachmentDescriptions (const GVK::ShaderModuleReflection& reflection, GVK::ShaderKind shaderKind, IAttachmentProvider& attachmentProvider);


} // namespace FromShaderReflection
} // namespace RG

#endif
