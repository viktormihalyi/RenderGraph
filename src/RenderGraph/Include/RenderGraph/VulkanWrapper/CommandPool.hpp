#ifndef COMMANDPOOL_HPP
#define COMMANDPOOL_HPP

#include <vulkan/vulkan.h>

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"

#include <stdexcept>

namespace GVK {

class /* RENDERGRAPH_DLL_EXPORT */ CommandPool : public VulkanObject, public Nonmovable {
private:
    VkDevice                       device;
    GVK::MovablePtr<VkCommandPool> handle;

public:
    CommandPool (VkDevice device, uint32_t queueIndex)
        : device (device)
        , handle (VK_NULL_HANDLE)
    {
        VkCommandPoolCreateInfo commandPoolInfo = {};
        commandPoolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.queueFamilyIndex        = queueIndex;
        commandPoolInfo.flags                   = 0;
        if (GVK_ERROR (vkCreateCommandPool (device, &commandPoolInfo, nullptr, &handle) != VK_SUCCESS)) {
            throw std::runtime_error ("failed to create commandpool");
        }
    }

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_COMMAND_POOL; }

    virtual ~CommandPool () override
    {
        vkDestroyCommandPool (device, handle, nullptr);
        handle = nullptr;
    }

    operator VkCommandPool () const
    {
        return handle;
    }
};

} // namespace GVK

#endif