#ifndef VULKANTESTENVIRONMENT_HPP
#define VULKANTESTENVIRONMENT_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/VulkanWrapper/DebugUtilsMessenger.hpp"
#include "RenderGraph/VulkanWrapper/Swapchain.hpp"
#include "RenderGraph/VulkanWrapper/Surface.hpp"

#include <memory>

#include <optional>

namespace GVK {
class Instance;
class PhysicalDevice;
class Device;
class Queue;
class CommandPool;
class DeviceExtra;
class Allocator;
class Surface;
} // namespace GVK

namespace RG {

class Window;
class VulkanEnvironment;

class RENDERGRAPH_DLL_EXPORT Presentable : public GVK::SwapchainProvider {
private:
    std::unique_ptr<Window>    window;
    std::unique_ptr<GVK::Surface>   surface;
    std::unique_ptr<GVK::Swapchain> swapchain;

public:
    Presentable (VulkanEnvironment& env, std::unique_ptr<GVK::Surface>&& surface, std::unique_ptr<GVK::SwapchainSettingsProvider>&& settingsProvider);
    Presentable (VulkanEnvironment& env, Window& window, std::unique_ptr<GVK::SwapchainSettingsProvider>&& settingsProvider);
    Presentable (VulkanEnvironment& env, std::unique_ptr<Window>&& window, std::unique_ptr<GVK::SwapchainSettingsProvider>&& settingsProvider);

    virtual GVK::Swapchain& GetSwapchain () override;

    const GVK::Surface& GetSurface () const;

    bool HasWindow () const;
    Window& GetWindow ();

    std::optional<double> GetRefreshRate () const;
};


RENDERGRAPH_DLL_EXPORT
void defaultDebugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT,
                           VkDebugUtilsMessageTypeFlagsEXT,
                           const VkDebugUtilsMessengerCallbackDataEXT*);

class RENDERGRAPH_DLL_EXPORT VulkanEnvironment {
public:
    std::unique_ptr<GVK::Instance>            instance;
    std::unique_ptr<GVK::DebugUtilsMessenger> messenger;
    std::unique_ptr<GVK::PhysicalDevice>      physicalDevice;
    std::unique_ptr<GVK::Device>              device;
    std::unique_ptr<GVK::Queue>               graphicsQueue;
    std::unique_ptr<GVK::Queue>               presentQueue;
    std::unique_ptr<GVK::CommandPool>         commandPool;
    std::unique_ptr<GVK::DeviceExtra>         deviceExtra;
    std::unique_ptr<GVK::Allocator>           allocator;

    VulkanEnvironment (std::optional<GVK::DebugUtilsMessenger::Callback> callback           = defaultDebugCallback,
                       const std::vector<const char*>&              instanceExtensions = {},
                       const std::vector<const char*>&              deviceExtensions = {});

    virtual ~VulkanEnvironment ();

    void Wait () const;

    bool CheckForPhsyicalDeviceSupport (const Presentable&);
};

} // namespace RG


#endif