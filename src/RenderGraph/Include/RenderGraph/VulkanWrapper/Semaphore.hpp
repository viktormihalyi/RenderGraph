#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

#include <vulkan/vulkan.h>

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"

namespace RG {

class /* RENDERGRAPH_DLL_EXPORT */ Semaphore : public VulkanObject {
private:
    VkDevice                     device;
    RG::MovablePtr<VkSemaphore> handle;

    static VkSemaphore CreateSemaphore (VkDevice device)
    {
        VkSemaphore           handle;
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreInfo.flags                 = 0;
        if (RG_ERROR (vkCreateSemaphore (device, &semaphoreInfo, nullptr, &handle) != VK_SUCCESS)) {
            throw std::runtime_error ("failed to create semaphore");
        }
        return handle;
    }

public:
    Semaphore (VkDevice device)
        : device (device)
        , handle (CreateSemaphore (device))
    {
    }

    Semaphore (Semaphore&&) = default;
    Semaphore& operator= (Semaphore&&) = default;

    virtual ~Semaphore () override
    {
        vkDestroySemaphore (device, handle, nullptr);
        handle = nullptr;
    }

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_SEMAPHORE; }

    operator VkSemaphore () const
    {
        return handle;
    }
};

} // namespace RG

#endif