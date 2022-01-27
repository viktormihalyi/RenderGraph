#ifndef DESCRIPTORPOOL_HPP
#define DESCRIPTORPOOL_HPP

#include <vulkan/vulkan.h>

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"
#include "VulkanObject.hpp"

namespace GVK {

class /* RENDERGRAPH_DLL_EXPORT */ DescriptorPool : public VulkanObject {
private:
    VkDevice         device;
    VkDescriptorPool handle;

public:
    DescriptorPool (VkDevice device, std::vector<VkDescriptorPoolSize> poolSizes, uint32_t maxSets)
        : device (device)
        , handle (VK_NULL_HANDLE)
    {
        if (GVK_ERROR (poolSizes.empty ())) {
            throw std::runtime_error ("empty pool");
        }

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount              = static_cast<uint32_t> (poolSizes.size ());
        poolInfo.pPoolSizes                 = poolSizes.data ();
        poolInfo.maxSets                    = maxSets;

        if (GVK_ERROR (vkCreateDescriptorPool (device, &poolInfo, nullptr, &handle) != VK_SUCCESS)) {
            throw std::runtime_error ("failed to create descriptor pool");
        }
    }

    DescriptorPool (DescriptorPool&&) = default;
    DescriptorPool& operator= (DescriptorPool&&) = default;

    virtual ~DescriptorPool () override
    {
        vkDestroyDescriptorPool (device, handle, nullptr);
        handle = nullptr;
    }

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT; };

    operator VkDescriptorPool () const
    {
        return handle;
    }
};

} // namespace GVK

#endif