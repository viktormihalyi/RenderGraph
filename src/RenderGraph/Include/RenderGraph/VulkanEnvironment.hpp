#ifndef VULKANTESTENVIRONMENT_HPP
#define VULKANTESTENVIRONMENT_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/VulkanWrapper/DebugUtilsMessenger.hpp"
#include "RenderGraph/VulkanWrapper/Swapchain.hpp"
#include "RenderGraph/VulkanWrapper/Surface.hpp"

#include <memory>

#include <optional>

namespace RG {
class Instance;
class PhysicalDevice;
class Device;
class Queue;
class CommandPool;
class DeviceExtra;
class Allocator;
class Surface;
} // namespace RG

namespace RG {

class Window;
class VulkanEnvironment;

class RENDERGRAPH_DLL_EXPORT Presentable : public RG::SwapchainProvider {
private:
    std::unique_ptr<Window>    window;
    std::unique_ptr<RG::Surface>   surface;
    std::unique_ptr<RG::Swapchain> swapchain;

public:
    Presentable (VulkanEnvironment& env, std::unique_ptr<RG::Surface>&& surface, std::unique_ptr<RG::SwapchainSettingsProvider>&& settingsProvider);
    Presentable (VulkanEnvironment& env, Window& window, std::unique_ptr<RG::SwapchainSettingsProvider>&& settingsProvider);
    Presentable (VulkanEnvironment& env, std::unique_ptr<Window>&& window, std::unique_ptr<RG::SwapchainSettingsProvider>&& settingsProvider);

    virtual RG::Swapchain& GetSwapchain () override;

    const RG::Surface& GetSurface () const;

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
    std::unique_ptr<RG::Instance>            instance;
    std::unique_ptr<RG::DebugUtilsMessenger> messenger;
    std::unique_ptr<RG::PhysicalDevice>      physicalDevice;
    std::unique_ptr<RG::Device>              device;
    std::unique_ptr<RG::Queue>               graphicsQueue;
    std::unique_ptr<RG::Queue>               presentQueue;
    std::unique_ptr<RG::CommandPool>         commandPool;
    std::unique_ptr<RG::DeviceExtra>         deviceExtra;
    std::unique_ptr<RG::Allocator>           allocator;

    VulkanEnvironment (std::optional<RG::DebugUtilsMessenger::Callback> callback           = defaultDebugCallback,
                       const std::vector<const char*>&              instanceExtensions = {},
                       const std::vector<const char*>&              deviceExtensions = {});

    virtual ~VulkanEnvironment ();

    void Wait () const;

    bool CheckForPhsyicalDeviceSupport (const Presentable&);
};

} // namespace RG


#endif