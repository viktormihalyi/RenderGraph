#ifndef DEVICE_HPP
#define DEVICE_HPP

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"

#include <vulkan/vulkan.h>
#include <vector>

namespace RG {

class RENDERGRAPH_DLL_EXPORT Device : public Nonmovable {
public:
    Device () = default;
    
    virtual ~Device ();

    Device (const Device&) = delete;
    Device& operator= (const Device&) = delete;

    virtual      operator VkDevice () const = 0;
    virtual void Wait () const              = 0;
};


class RENDERGRAPH_DLL_EXPORT DeviceObject : public VulkanObject, public Device {
private:
    VkPhysicalDevice          physicalDevice;
    RG::MovablePtr<VkDevice> handle;

public:
    DeviceObject (VkPhysicalDevice physicalDevice, std::vector<uint32_t> queueFamilyIndices, std::vector<const char*> requestedDeviceExtensions);

    virtual ~DeviceObject () override;

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_DEVICE; }

    virtual operator VkDevice () const override
    {
        return handle;
    }

    virtual void Wait () const override
    {
        vkDeviceWaitIdle (handle);
    }

private:
    uint32_t FindMemoryType (uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
};

} // namespace RG

#endif