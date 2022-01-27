#ifndef SAMPLER_HPP
#define SAMPLER_HPP

#include "RenderGraph/RenderGraphExport.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"

#include <vulkan/vulkan.h>

namespace RG {

class RENDERGRAPH_DLL_EXPORT Sampler : public VulkanObject {
private:
    VkDevice                   device;
    RG::MovablePtr<VkSampler> handle;
    VkFilter                   filter;

public:
    Sampler (VkDevice device, VkFilter filter, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

    Sampler (Sampler&&) = default;
    Sampler& operator= (Sampler&&) = default;

    virtual ~Sampler () override;

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_SAMPLER; }

    operator VkSampler () const { return handle; }
};

} // namespace RG

#endif