#ifndef SURFACE_HPP
#define SURFACE_HPP

#include "RenderGraph/RenderGraphExport.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "RenderGraph/VulkanWrapper/VulkanObject.hpp"

#include <vulkan/vulkan.h>

namespace RG {

class RENDERGRAPH_DLL_EXPORT Surface : public VulkanObject {
private:
    VkInstance                    instance;
    RG::MovablePtr<VkSurfaceKHR> handle;

public:
    Surface (VkInstance instance, VkSurfaceKHR&& handle);

    enum PlatformSpecificSelector {
        PlatformSpecific
    };

    Surface (PlatformSpecificSelector, VkInstance instance, void* handle);

    Surface (Surface&&) = default;
    Surface& operator= (Surface&&) = default;

    virtual ~Surface () override;

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_SURFACE_KHR; }

    operator VkSurfaceKHR () const { return handle; }
};

} // namespace RG

#endif