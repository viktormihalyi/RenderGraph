#include "Sampler.hpp"

#include "Utils/Assert.hpp"

#include <stdexcept>

namespace GVK {

Sampler::Sampler (VkDevice device, VkFilter filter, VkSamplerAddressMode addressMode)
    : device (device)
    , filter (filter)
{
    VkSamplerCreateInfo createInfo     = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter               = filter;
    createInfo.minFilter               = filter;
    createInfo.addressModeU            = addressMode;
    createInfo.addressModeV            = addressMode;
    createInfo.addressModeW            = addressMode;
    createInfo.anisotropyEnable        = VK_FALSE;
    createInfo.maxAnisotropy           = 16;
    createInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable           = VK_FALSE;
    createInfo.compareOp               = VK_COMPARE_OP_NEVER;
    createInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias              = 0.0f;
    createInfo.minLod                  = 0.0f;
    createInfo.maxLod                  = 0.0f;

    if (GVK_ERROR (vkCreateSampler (device, &createInfo, nullptr, &handle) != VK_SUCCESS)) {
        throw std::runtime_error ("failed to create texture sampler!");
    }
}


Sampler::~Sampler ()
{
    vkDestroySampler (device, handle, nullptr);
    handle = nullptr;
}

} // namespace GVK