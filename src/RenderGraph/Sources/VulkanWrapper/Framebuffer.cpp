#include "Framebuffer.hpp"

#include "Utils/Utils.hpp"
#include "Utils/Assert.hpp"

namespace GVK {


Framebuffer::Framebuffer (VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView>& attachments, uint32_t width, uint32_t height)
    : device (device)
    , handle (VK_NULL_HANDLE)
    , width (width)
    , height (height)
{
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.flags                   = 0;
    framebufferInfo.renderPass              = renderPass;
    framebufferInfo.attachmentCount         = static_cast<uint32_t> (attachments.size ());
    framebufferInfo.pAttachments            = attachments.data ();
    framebufferInfo.width                   = width;
    framebufferInfo.height                  = height;
    framebufferInfo.layers                  = 1;

    VkResult result = vkCreateFramebuffer (device, &framebufferInfo, nullptr, &handle);
    if (GVK_ERROR (result != VK_SUCCESS)) {
        throw std::runtime_error ("failed to create framebuffer");
    }
}


Framebuffer::Framebuffer (VkDevice device, VkRenderPass renderPass, std::vector<ImageView2D>&& attachments_, uint32_t width, uint32_t height)
    : device (device)
    , handle (VK_NULL_HANDLE)
    , width (width)
    , height (height)
    , attachments (std::move (attachments_))
{
    const auto handles = [&] () ->std::vector<VkImageView> {
        std::vector<VkImageView> result;
        for (const auto& a : attachments) {
            result.push_back (a.operator VkImageView());
        }
        return result;
    }();

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.flags                   = 0;
    framebufferInfo.renderPass              = renderPass;
    framebufferInfo.attachmentCount         = static_cast<uint32_t> (handles.size ());
    framebufferInfo.pAttachments            = handles.data ();
    framebufferInfo.width                   = width;
    framebufferInfo.height                  = height;
    framebufferInfo.layers                  = 1;

    VkResult result = vkCreateFramebuffer (device, &framebufferInfo, nullptr, &handle);
    if (GVK_ERROR (result != VK_SUCCESS)) {
        throw std::runtime_error ("failed to create framebuffer");
    }
}


Framebuffer::~Framebuffer ()
{
    if (handle != nullptr) {
        vkDestroyFramebuffer (device, handle, nullptr);
        handle = nullptr;
    }
}


} // namespace GVK

