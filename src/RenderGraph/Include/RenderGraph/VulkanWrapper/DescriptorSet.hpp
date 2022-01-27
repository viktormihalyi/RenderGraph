#ifndef DESCRIPTORSET_HPP
#define DESCRIPTORSET_HPP

#include <vulkan/vulkan.h>

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"

#include "DescriptorPool.hpp"
#include "DescriptorSetLayout.hpp"

namespace RG {

class /* RENDERGRAPH_DLL_EXPORT */ DescriptorSet : public VulkanObject {
private:
    VkDevice                         device;
    VkDescriptorPool                 descriptorPool;
    RG::MovablePtr<VkDescriptorSet> handle;

public:
    DescriptorSet (VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout)
        : device (device)
        , descriptorPool (descriptorPool)
        , handle (VK_NULL_HANDLE)
    {
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool              = descriptorPool;
        allocInfo.descriptorSetCount          = 1;
        allocInfo.pSetLayouts                 = &layout;

        if (RG_ERROR (vkAllocateDescriptorSets (device, &allocInfo, &handle) != VK_SUCCESS)) {
            throw std::runtime_error ("failed to allocate descriptor sets!");
        }
    }

    DescriptorSet (DescriptorSet&&) = default;
    DescriptorSet& operator= (DescriptorSet&&) = default;

    DescriptorSet (VkDevice device, const DescriptorPool& descriptorPool, const DescriptorSetLayout& layout)
        : DescriptorSet (device, descriptorPool.operator VkDescriptorPool (), layout)
    {
    }

    virtual ~DescriptorSet () override
    {
        // only free for VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
        // vkFreeDescriptorSets (device, descriptorPool, 1, &handle);
        handle = nullptr;
    }

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_DESCRIPTOR_SET; }

    mutable std::vector<std::tuple<uint32_t, VkDescriptorType, VkWriteDescriptorSet>> writtenInfos;

    operator VkDescriptorSet () const
    {
        return handle;
    }
};

} // namespace RG

#endif