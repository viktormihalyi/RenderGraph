#ifndef PHYSICALDEVICE_HPP
#define PHYSICALDEVICE_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"

#include <vulkan/vulkan.h>

#include <memory>
#include <optional>
#include <vector>
#include <set>
#include <string>

namespace RG {

class RENDERGRAPH_DLL_EXPORT PhysicalDevice final : public Noncopyable {
public:
    struct QueueFamilies {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> presentation;
        std::optional<uint32_t> transfer;
        std::optional<uint32_t> compute;
    };

private:
    VkInstance                        instance;
    RG::MovablePtr<VkPhysicalDevice> handle;
    std::set<std::string>             requestedDeviceExtensionSet;
    QueueFamilies                     queueFamilies;

    std::unique_ptr<VkPhysicalDeviceProperties> properties;
    std::unique_ptr<VkPhysicalDeviceFeatures>   features;

public:
    PhysicalDevice (VkInstance                   instance,
                    VkSurfaceKHR                 surface, // VK_NULL_HANDLE is accepted
                    const std::set<std::string>& requestedDeviceExtensionSet);

    virtual ~PhysicalDevice () override;

    bool CheckSurfaceSupported (VkSurfaceKHR surface) const;

    operator VkPhysicalDevice () const { return handle; }

    QueueFamilies GetQueueFamilies () const { return queueFamilies; }

    VkPhysicalDeviceProperties GetProperties () const
    {
        VkPhysicalDeviceProperties result;
        vkGetPhysicalDeviceProperties (handle, &result);
        return result;
    }

    VkPhysicalDeviceFeatures GetFeatures () const
    {
        VkPhysicalDeviceFeatures result;
        vkGetPhysicalDeviceFeatures (handle, &result);
        return result;
    }
};

} // namespace RG

#endif