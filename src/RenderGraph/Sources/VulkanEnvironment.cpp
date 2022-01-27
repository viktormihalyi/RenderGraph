#include "VulkanEnvironment.hpp"

#include "Window.hpp"

#include "Utils/BuildType.hpp"
#include "Utils/CommandLineFlag.hpp"
#include "Utils/StaticInit.hpp"
#include "Utils/Timer.hpp"

#include "VulkanWrapper/Allocator.hpp"
#include "VulkanWrapper/DebugUtilsMessenger.hpp"
#include "VulkanWrapper/DeviceExtra.hpp"
#include "VulkanWrapper/Instance.hpp"
#include "VulkanWrapper/Surface.hpp"
#include "VulkanWrapper/VulkanWrapper.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include <iomanip>


static RG::CommandLineOnOffFlag disableValidationLayersFlag (std::vector<std::string> { "--disableValidationLayers", "-v" }, "Disables Vulkan validation layers.");
static RG::CommandLineOnOffFlag logVulkanVersionFlag ("--logVulkanVersion");


namespace RG {

Presentable::Presentable (VulkanEnvironment& env, std::unique_ptr<RG::Surface>&& surface, std::unique_ptr<RG::SwapchainSettingsProvider>&& settingsProvider)
    : surface (std::move (surface))
    , window (nullptr)
{
    // TODO this is kind of ugly
    // when creating a swapchain, we must query if presentation is supported for the surface
    // but we create a physicaldevice first and create the swapchains later
    // so we _hope_ it is supported

    RG_VERIFY (env.CheckForPhsyicalDeviceSupport (*this));

    swapchain = std::make_unique<RG::RealSwapchain> (*env.physicalDevice, *env.device, *this->surface, std::move (settingsProvider));
}


Presentable::Presentable (VulkanEnvironment& env, Window& window, std::unique_ptr<RG::SwapchainSettingsProvider>&& settingsProvider)
    : Presentable (env, std::make_unique<RG::Surface> (*env.instance, window.GetSurface (*env.instance)), std::move (settingsProvider))
{
}


Presentable::Presentable (VulkanEnvironment& env, std::unique_ptr<Window>&& window, std::unique_ptr<RG::SwapchainSettingsProvider>&& settingsProvider)
    : Presentable (env, std::make_unique<RG::Surface> (*env.instance, window->GetSurface (*env.instance)), std::move (settingsProvider))
{
    this->window = std::move (window);
}


RG::Swapchain& Presentable::GetSwapchain ()
{
    return *swapchain;
}


const RG::Surface& Presentable::GetSurface () const
{
    return *surface;
}


bool Presentable::HasWindow () const
{
    return window != nullptr;
}


Window& Presentable::GetWindow ()
{
    RG_ASSERT (window != nullptr);
    return *window;
}


std::optional<double> Presentable::GetRefreshRate () const
{
    if (window != nullptr) {
        return window->GetRefreshRate ();
    }

    return std::nullopt;
}


const std::unordered_map<VkObjectType, const char*> VkObjectTypeToStringMap {
    { VK_OBJECT_TYPE_UNKNOWN, "VK_OBJECT_TYPE_UNKNOWN" },
    { VK_OBJECT_TYPE_INSTANCE, "VK_OBJECT_TYPE_INSTANCE" },
    { VK_OBJECT_TYPE_PHYSICAL_DEVICE, "VK_OBJECT_TYPE_PHYSICAL_DEVICE" },
    { VK_OBJECT_TYPE_DEVICE, "VK_OBJECT_TYPE_DEVICE" },
    { VK_OBJECT_TYPE_QUEUE, "VK_OBJECT_TYPE_QUEUE" },
    { VK_OBJECT_TYPE_SEMAPHORE, "VK_OBJECT_TYPE_SEMAPHORE" },
    { VK_OBJECT_TYPE_COMMAND_BUFFER, "VK_OBJECT_TYPE_COMMAND_BUFFER" },
    { VK_OBJECT_TYPE_FENCE, "VK_OBJECT_TYPE_FENCE" },
    { VK_OBJECT_TYPE_DEVICE_MEMORY, "VK_OBJECT_TYPE_DEVICE_MEMORY" },
    { VK_OBJECT_TYPE_BUFFER, "VK_OBJECT_TYPE_BUFFER" },
    { VK_OBJECT_TYPE_IMAGE, "VK_OBJECT_TYPE_IMAGE" },
    { VK_OBJECT_TYPE_EVENT, "VK_OBJECT_TYPE_EVENT" },
    { VK_OBJECT_TYPE_QUERY_POOL, "VK_OBJECT_TYPE_QUERY_POOL" },
    { VK_OBJECT_TYPE_BUFFER_VIEW, "VK_OBJECT_TYPE_BUFFER_VIEW" },
    { VK_OBJECT_TYPE_IMAGE_VIEW, "VK_OBJECT_TYPE_IMAGE_VIEW" },
    { VK_OBJECT_TYPE_SHADER_MODULE, "VK_OBJECT_TYPE_SHADER_MODULE" },
    { VK_OBJECT_TYPE_PIPELINE_CACHE, "VK_OBJECT_TYPE_PIPELINE_CACHE" },
    { VK_OBJECT_TYPE_PIPELINE_LAYOUT, "VK_OBJECT_TYPE_PIPELINE_LAYOUT" },
    { VK_OBJECT_TYPE_RENDER_PASS, "VK_OBJECT_TYPE_RENDER_PASS" },
    { VK_OBJECT_TYPE_PIPELINE, "VK_OBJECT_TYPE_PIPELINE" },
    { VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT" },
    { VK_OBJECT_TYPE_SAMPLER, "VK_OBJECT_TYPE_SAMPLER" },
    { VK_OBJECT_TYPE_DESCRIPTOR_POOL, "VK_OBJECT_TYPE_DESCRIPTOR_POOL" },
    { VK_OBJECT_TYPE_DESCRIPTOR_SET, "VK_OBJECT_TYPE_DESCRIPTOR_SET" },
    { VK_OBJECT_TYPE_FRAMEBUFFER, "VK_OBJECT_TYPE_FRAMEBUFFER" },
    { VK_OBJECT_TYPE_COMMAND_POOL, "VK_OBJECT_TYPE_COMMAND_POOL" },
    { VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION, "VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION" },
    { VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE, "VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE" },
    { VK_OBJECT_TYPE_SURFACE_KHR, "VK_OBJECT_TYPE_SURFACE_KHR" },
    { VK_OBJECT_TYPE_SWAPCHAIN_KHR, "VK_OBJECT_TYPE_SWAPCHAIN_KHR" },
    { VK_OBJECT_TYPE_DISPLAY_KHR, "VK_OBJECT_TYPE_DISPLAY_KHR" },
    { VK_OBJECT_TYPE_DISPLAY_MODE_KHR, "VK_OBJECT_TYPE_DISPLAY_MODE_KHR" },
    { VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT, "VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT" },
    { VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT, "VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT" },
    { VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR" },
    { VK_OBJECT_TYPE_VALIDATION_CACHE_EXT, "VK_OBJECT_TYPE_VALIDATION_CACHE_EXT" },
    { VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV, "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV" },
    { VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL, "VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL" },
    { VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR, "VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR" },
    { VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV, "VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV" },
    { VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT, "VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT" },
    { VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR, "VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR" },
    { VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_KHR, "VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_KHR" },
};


static const char* VkObjectTypeToString (VkObjectType objectType)
{
    auto found = VkObjectTypeToStringMap.find (objectType);
    if (found != VkObjectTypeToStringMap.end ())
        return found->second;

    return "[UNKNOWN]";
}


// from https://www.lunarg.com/wp-content/uploads/2018/05/Vulkan-Debug-Utils_05_18_v1.pdf
void defaultDebugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                           VkDebugUtilsMessageTypeFlagsEXT             messageType,
                           const VkDebugUtilsMessengerCallbackDataEXT* callbackData)
{
    const bool isShaderPrintf = std::string (callbackData->pMessageIdName) == std::string ("UNASSIGNED-DEBUG-PRINTF");

    const bool allow = messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT || isShaderPrintf;

    if (!allow) {
        return;
    }

    if (isShaderPrintf) {
        const std::string messageStr (callbackData->pMessage);
        
        const size_t firstDividerPos = messageStr.find ("|");
        RG_ASSERT (firstDividerPos != std::string::npos);

        const std::string postFirstDivider = messageStr.substr (firstDividerPos + 2);

        const size_t secondDividerPos = postFirstDivider.find ("|");
        RG_ASSERT (secondDividerPos != std::string::npos);

        const std::string postSecondDivider = postFirstDivider.substr (secondDividerPos + 2);

        spdlog::info ("SHADER PRINTF: {}", postSecondDivider);
        return;
    }

    const auto SeverityFlagBitsToString = [] (VkDebugUtilsMessageSeverityFlagBitsEXT value) -> const char* {
        if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
            return "VERBOSE";
        if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            return "INFO";
        if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            return "WARNING";
        if (value & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            return "ERROR";

        RG_BREAK ();
        return "unknown";
    };

    const auto MessageTypeToString = [] (VkDebugUtilsMessageTypeFlagsEXT value) -> const char* {
        if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
            return "GENERAL";
        if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            return "VALIDATION";
        if (value & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
            return "PERFORMANCE";

        RG_BREAK ();
        return "unknown";
    };
    
    std::string message = fmt::format ("Vulkan Debug Callback: [{}] [{}] (id: {}) (id name: {})\n\t{}",
                                       SeverityFlagBitsToString (messageSeverity),
                                       MessageTypeToString (messageType),
                                       callbackData->messageIdNumber,
                                       callbackData->pMessageIdName,
                                       callbackData->pMessage);

    if (callbackData->objectCount > 0) {
        message += fmt::format ("\n\n\tObjects - {}", callbackData->objectCount);
    }

    for (size_t i = 0; i < callbackData->objectCount; ++i) {
        message += fmt::format ("\n\t\tObject[{}] - type: \"{}\", handle: \"{}\"",
                                i,
                                VkObjectTypeToString (callbackData->pObjects[i].objectType),
                                reinterpret_cast<void*> (callbackData->pObjects[i].objectHandle));

        if (callbackData->pObjects[i].pObjectName != nullptr) {
            message += fmt::format (", name: \"{}\"", callbackData->pObjects[i].pObjectName);
        }

        message += "\n";
    }

    RG_ASSERT (callbackData->cmdBufLabelCount == 0);

    spdlog::info (message);
}


void VulkanEnvironment::Wait () const
{
    graphicsQueue->Wait ();
    device->Wait ();
}


DebugOnlyStaticInit apiVersionLogger ([] () {
    if (logVulkanVersionFlag.IsFlagOn ()) {
        uint32_t apiVersion;
        vkEnumerateInstanceVersion (&apiVersion);
        spdlog::info ("vulkan api version: {}", RG::GetVersionString (apiVersion));
    }
});


VulkanEnvironment::VulkanEnvironment (std::optional<RG::DebugUtilsMessenger::Callback> callback, const std::vector<const char*>& instanceExtensions, const std::vector<const char*>& deviceExtensions)
{
    RG::InstanceSettings is = (IsDebugBuild) ? RG::instanceDebugMode : RG::instanceReleaseMode;
    is.extensions.insert (is.extensions.end (), instanceExtensions.begin (), instanceExtensions.end ());

    instance = std::make_unique<RG::Instance> (is);

    if constexpr (IsDebugBuild)
        if (callback.has_value () && !disableValidationLayersFlag.IsFlagOn ())
            messenger = std::make_unique<RG::DebugUtilsMessenger> (*instance, *callback, RG::DebugUtilsMessenger::noPerformance);

    const VkSurfaceKHR physicalDeviceSurfaceHandle = VK_NULL_HANDLE;

    physicalDevice = std::make_unique<RG::PhysicalDevice> (*instance, physicalDeviceSurfaceHandle, std::set<std::string> {});

    if (logVulkanVersionFlag.IsFlagOn ()) {
        VkPhysicalDeviceProperties properties = physicalDevice->GetProperties ();

        auto DeviceTypeToString = [] (VkPhysicalDeviceType type) -> std::string {
            switch (type) {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER: return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
                case VK_PHYSICAL_DEVICE_TYPE_CPU: return "VK_PHYSICAL_DEVICE_TYPE_CPU";
                default:
                    RG_BREAK ();
                    return "<unknown>";
            }
        };


        spdlog::info ("physical device api version: {}", RG::GetVersionString (properties.apiVersion));
        spdlog::info ("physical device driver version: {} ({})", RG::GetVersionString (properties.driverVersion), properties.driverVersion);
        spdlog::info ("device name: {}", properties.deviceName);
        spdlog::info ("device type: {}", DeviceTypeToString (properties.deviceType));
        spdlog::info ("device id: {}", properties.deviceID);
        spdlog::info ("vendor id: {}", properties.vendorID);

        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties (*physicalDevice, VK_FORMAT_R32_SFLOAT, &props);
        vkGetPhysicalDeviceFormatProperties (*physicalDevice, VK_FORMAT_R32G32B32_SFLOAT, &props);
        vkGetPhysicalDeviceFormatProperties (*physicalDevice, VK_FORMAT_R32G32B32_SFLOAT, &props);
    }

    device = std::make_unique<RG::DeviceObject> (*physicalDevice, std::vector<uint32_t> { *physicalDevice->GetQueueFamilies ().graphics }, deviceExtensions);

    allocator = std::make_unique<RG::Allocator> (*instance, *physicalDevice, *device);

    graphicsQueue = std::make_unique<RG::Queue> (*device, *physicalDevice->GetQueueFamilies ().graphics);

    commandPool = std::make_unique<RG::CommandPool> (*device, *physicalDevice->GetQueueFamilies ().graphics);

    deviceExtra = std::make_unique<RG::DeviceExtra> (*instance, *device, *commandPool, *allocator, *graphicsQueue);

    commandPool->SetName (*deviceExtra, "VulkanEnvironment CommandPool");
    static_cast<RG::DeviceObject*> (device.get ())->SetName (*deviceExtra, "VulkanEnvironment DeviceObject");
}


VulkanEnvironment::~VulkanEnvironment ()
{
    Wait ();
}


bool VulkanEnvironment::CheckForPhsyicalDeviceSupport (const Presentable& presentable)
{
    const RG::Surface& surface = presentable.GetSurface ();
    return physicalDevice->CheckSurfaceSupported (surface);
}

} // namespace RG