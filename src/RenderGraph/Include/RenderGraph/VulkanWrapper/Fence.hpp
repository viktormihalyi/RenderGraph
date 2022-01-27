#ifndef FENCE_HPP
#define FENCE_HPP

#include <vulkan/vulkan.h>

#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"

namespace GVK {


class RENDERGRAPH_DLL_EXPORT Fence : public VulkanObject {
private:
    VkDevice                 device;
    GVK::MovablePtr<VkFence> handle;

public:
    Fence (VkDevice device, bool signaled = true);
    Fence (Fence&&) = default;
    Fence& operator= (Fence&&) = default;

    virtual ~Fence () override;

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_FENCE; }

    operator VkFence () const { return handle; }

    void Wait () const;

    void Reset () const;

private:
    void WaitImpl () const;
};

} // namespace GVK

#endif