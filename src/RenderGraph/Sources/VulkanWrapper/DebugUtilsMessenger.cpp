#include "DebugUtilsMessenger.hpp"
#include "VulkanFunctionGetter.hpp"

namespace RG {

const DebugUtilsMessenger::Settings DebugUtilsMessenger::defaultSettings {
    true,
    true,
    true,
    true,

    true,
    true,
    true,
};


const DebugUtilsMessenger::Settings DebugUtilsMessenger::noPerformance {
    true,
    true,
    true,
    true,

    true,
    true,
    false,
};


const DebugUtilsMessenger::Settings DebugUtilsMessenger::all {
    true,
    true,
    true,
    true,

    true,
    true,
    true,
};


VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessenger::debugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                                   VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                                   const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                                   void*                                       userData)
{
    if (RG_VERIFY (userData != nullptr)) {
        reinterpret_cast<DebugUtilsMessenger*> (userData)->callback (messageSeverity, messageType, callbackData);
    }

    return VK_FALSE;
}


DebugUtilsMessenger::DebugUtilsMessenger (VkInstance instance, const Callback& callback, const Settings& settings)
    : instance (instance)
    , handle (VK_NULL_HANDLE)
    , callback (callback)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    
    createInfo.flags = 0;

    createInfo.messageSeverity =
        ((settings.verbose) ? VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT : 0) |
        ((settings.info) ? VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT : 0) |
        ((settings.warning) ? VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT : 0) |
        ((settings.error) ? VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT : 0);

    createInfo.messageType =
        ((settings.general) ? VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT : 0) |
        ((settings.validation) ? VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT : 0) |
        ((settings.perforamnce) ? VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT : 0);

    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData       = this;

    VkResult result = GetVulkanFunction<PFN_vkCreateDebugUtilsMessengerEXT> (instance, "vkCreateDebugUtilsMessengerEXT") (instance, &createInfo, nullptr, &handle);
    if (RG_ERROR (result != VK_SUCCESS)) {
        throw std::runtime_error ("failed to create debug utils messenger");
    }
}


DebugUtilsMessenger::~DebugUtilsMessenger ()
{
    GetVulkanFunction<PFN_vkDestroyDebugUtilsMessengerEXT> (instance, "vkDestroyDebugUtilsMessengerEXT") (instance, handle, nullptr);
    handle = nullptr;
}

} // namespace RG