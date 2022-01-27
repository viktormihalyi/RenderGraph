#ifndef RENDERGRAPH_DESCRIPTORBINDABLE_HPP
#define RENDERGRAPH_DESCRIPTORBINDABLE_HPP

#include <cstdint>
#include <functional>
#include <vulkan/vulkan.h>

#include "Node.hpp"

namespace RG {

class DescriptorBindableBuffer {
public:
    virtual ~DescriptorBindableBuffer () = default;

    virtual VkBuffer GetBufferForFrame (uint32_t frameIndex) = 0;

    std::function<VkBuffer (uint32_t)> GetBufferForFrameProvider ()
    {
        return [=] (uint32_t frameIndex) -> VkBuffer {
            return GetBufferForFrame (frameIndex);
        };
    }

    virtual size_t GetBufferSize () = 0;
};

class DescriptorBindableImage {
public:
    virtual ~DescriptorBindableImage () = default;

    virtual VkImageView GetImageViewForFrame (uint32_t resourceIndex, uint32_t layerIndex) = 0;

    virtual VkSampler GetSampler () = 0;

    std::function<VkImageView (uint32_t resourceIndex, uint32_t layerIndex)> GetImageViewForFrameProvider ()
    {
        return [=] (uint32_t resourceIndex, uint32_t layerIndex) -> VkImageView {
            return GetImageViewForFrame (resourceIndex, layerIndex);
        };
    }

    std::function<VkSampler ()> GetSamplerProvider ()
    {
        return [=] () -> VkSampler {
            return GetSampler ();
        };
    }
};

} // namespace RG

#endif
