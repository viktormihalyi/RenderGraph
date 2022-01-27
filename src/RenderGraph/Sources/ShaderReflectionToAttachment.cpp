#include "ShaderReflectionToAttachment.hpp"

#include "RenderGraph/Utils/Assert.hpp"

#include "spdlog/spdlog.h"


namespace RG {
namespace FromShaderReflection {

IAttachmentProvider::~IAttachmentProvider () = default;


std::optional<AttachmentDataTable::AttachmentData> AttachmentDataTable::GetAttachmentData (const std::string& name, GVK::ShaderKind shaderKind)
{
    for (const auto& entry : table) {
        if (entry.name == name && entry.shaderKind == shaderKind) {
            return entry.data;
        }
    }

    return std::nullopt;
}


std::vector<VkImageView> GetImageViews (const GVK::ShaderModuleReflection& reflection, GVK::ShaderKind shaderKind, uint32_t resourceIndex, IAttachmentProvider& attachmentProvider)
{
    std::vector<VkImageView> imageViews;

    for (const SR::Output& output : reflection.outputs) {
        const std::optional<IAttachmentProvider::AttachmentData> attachmentData = attachmentProvider.GetAttachmentData (output.name, shaderKind);

        if (GVK_ERROR (!attachmentData.has_value ())) {
            spdlog::error ("Attachment \"{}\" (location: {}, layerCount: {}) is not set.", output.name, output.location, output.arraySize);
            continue;
        }

        const uint32_t layerCount = output.arraySize;

        for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
            imageViews.push_back (attachmentData->imageView (resourceIndex, layerIndex));
        }
    }

    for (const SR::SubpassInput& subpassInput : reflection.subpassInputs) {
        const std::optional<IAttachmentProvider::AttachmentData> attachmentData = attachmentProvider.GetAttachmentData (subpassInput.name, shaderKind);

        if (GVK_ERROR (!attachmentData.has_value ())) {
            spdlog::error ("Attachment \"{}\" (binding: {}, layerCount: {}) is not set.", subpassInput.name, subpassInput.binding, subpassInput.arraySize);
            continue;
        }

        const uint32_t layerCount = subpassInput.arraySize;

        for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
            imageViews.push_back (attachmentData->imageView (resourceIndex, layerIndex));
        }
    }

    return imageViews;
}


std::vector<VkAttachmentReference> GetAttachmentReferences (const GVK::ShaderModuleReflection& reflection, GVK::ShaderKind shaderKind, IAttachmentProvider& attachmentProvider)
{
    std::vector<VkAttachmentReference> result;

    for (const SR::Output& output : reflection.outputs) {
        const std::optional<IAttachmentProvider::AttachmentData> attachmentData = attachmentProvider.GetAttachmentData (output.name, shaderKind);

        if (GVK_ERROR (!attachmentData.has_value ())) {
            spdlog::error ("Attachment \"{}\" (location: {}, layerCount: {}) is not set.", output.name, output.location, output.arraySize);
            continue;
        }

        const uint32_t layerCount = output.arraySize;

        for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
            VkAttachmentReference attachmentReference = {};
            attachmentReference.attachment            = output.location + layerIndex;
            attachmentReference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            result.push_back (attachmentReference);
        }
    }

    return result;
}


std::vector<VkAttachmentReference> GetInputAttachmentReferences (const GVK::ShaderModuleReflection& reflection, GVK::ShaderKind shaderKind, IAttachmentProvider& attachmentProvider, uint32_t startIndex)
{
    std::vector<VkAttachmentReference> result;

    for (const SR::SubpassInput& subpassInput : reflection.subpassInputs) {
        const std::optional<IAttachmentProvider::AttachmentData> attachmentData = attachmentProvider.GetAttachmentData (subpassInput.name, shaderKind);

        if (GVK_ERROR (!attachmentData.has_value ())) {
            spdlog::error ("Input attachment \"{}\" (binding: {}, layerCount: {}) is not set.", subpassInput.name, subpassInput.binding, subpassInput.arraySize);
            continue;
        }

        const uint32_t layerCount = subpassInput.arraySize;

        for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
            VkAttachmentReference attachmentReference = {};
            attachmentReference.attachment            = startIndex + subpassInput.binding + layerIndex;
            attachmentReference.layout                = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            result.push_back (attachmentReference);
        }
    }

    return result;
}


std::vector<VkAttachmentDescription> GetAttachmentDescriptions (const GVK::ShaderModuleReflection& reflection, GVK::ShaderKind shaderKind, IAttachmentProvider& attachmentProvider)
{
    std::vector<VkAttachmentDescription> result;

    for (const SR::Output& output : reflection.outputs) {
        const std::optional<IAttachmentProvider::AttachmentData> attachmentData = attachmentProvider.GetAttachmentData (output.name, shaderKind);

        if (GVK_ERROR (!attachmentData.has_value ())) {
            spdlog::error ("Attachment \"{}\" (location: {}, layerCount: {}) is not set.", output.name, output.location, output.arraySize);
            continue;
        }

        const uint32_t layerCount = output.arraySize;

        for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
            VkAttachmentDescription attachmentDescription = {};
            attachmentDescription.flags                   = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
            attachmentDescription.format                  = attachmentData->format ();
            attachmentDescription.samples                 = VK_SAMPLE_COUNT_1_BIT;
            attachmentDescription.loadOp                  = attachmentData->loadOp;
            attachmentDescription.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescription.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescription.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescription.initialLayout           = attachmentData->initialLayout;
            attachmentDescription.finalLayout             = attachmentData->finalLayout;

            result.push_back (attachmentDescription);
        }
    }

    for (const SR::SubpassInput& subpassInput : reflection.subpassInputs) {
        const std::optional<IAttachmentProvider::AttachmentData> attachmentData = attachmentProvider.GetAttachmentData (subpassInput.name, shaderKind);

        if (GVK_ERROR (!attachmentData.has_value ())) {
            spdlog::error ("Attachment \"{}\" (binding: {}, layerCount: {}) is not set.", subpassInput.name, subpassInput.binding, subpassInput.arraySize);
            continue;
        }

        const uint32_t layerCount = subpassInput.arraySize;

        for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
            VkAttachmentDescription attachmentDescription = {};
            attachmentDescription.flags                   = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
            attachmentDescription.format                  = attachmentData->format ();
            attachmentDescription.samples                 = VK_SAMPLE_COUNT_1_BIT;
            attachmentDescription.loadOp                  = attachmentData->loadOp;
            attachmentDescription.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescription.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescription.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescription.initialLayout           = attachmentData->initialLayout;
            attachmentDescription.finalLayout             = attachmentData->finalLayout;

            result.push_back (attachmentDescription);
        }
    }

    return result;
}

} // namespace FromShaderReflection
} // namespace RG
