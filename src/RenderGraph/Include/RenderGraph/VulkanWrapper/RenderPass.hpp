#ifndef RENDERPASS_HPP
#define RENDERPASS_HPP

#include <vulkan/vulkan.h>

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"

namespace GVK {

class /* RENDERGRAPH_DLL_EXPORT */ RenderPass : public VulkanObject {
private:
    VkDevice                      device;
    GVK::MovablePtr<VkRenderPass> handle;

public:
    RenderPass (VkDevice                                    device,
                const std::vector<VkAttachmentDescription>& attachments,
                const std::vector<VkSubpassDescription>&    subpasses,
                const std::vector<VkSubpassDependency>&     subpassDependencies)
        : device (device)
    {
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount        = static_cast<uint32_t> (attachments.size ());
        renderPassInfo.pAttachments           = attachments.empty () ? nullptr : attachments.data ();
        renderPassInfo.subpassCount           = static_cast<uint32_t> (subpasses.size ());
        renderPassInfo.pSubpasses             = subpasses.empty () ? nullptr : subpasses.data ();
        renderPassInfo.dependencyCount        = static_cast<uint32_t> (subpassDependencies.size ());
        renderPassInfo.pDependencies          = subpassDependencies.empty () ? nullptr : subpassDependencies.data ();

        if (GVK_ERROR (vkCreateRenderPass (device, &renderPassInfo, nullptr, &handle) != VK_SUCCESS)) {
            throw std::runtime_error ("failed to create renderpass");
        }
    }

    RenderPass (RenderPass&&) = default;
    RenderPass& operator= (RenderPass&&) = default;

    virtual ~RenderPass () override
    {
        vkDestroyRenderPass (device, handle, nullptr);
        handle = nullptr;
    }
    
    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_RENDER_PASS; }

    operator VkRenderPass () const
    {
        return handle;
    }
};

} // namespace GVK

#endif