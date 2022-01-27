#include "DebugReportCallback.hpp"
#include "VulkanFunctionGetter.hpp"

#include "Utils/Assert.hpp"
#include "Utils/StaticInit.hpp"

namespace RG {
    
static VkBool32 cb (VkDebugReportFlagsEXT      /* flags */,
                    VkDebugReportObjectTypeEXT /* objectType */,
                    uint64_t                   /* object */,
                    size_t                     /* location */,
                    int32_t                    /* messageCode */,
                    const char*                /* pLayerPrefix */,
                    const char*                /* pMessage */,
                    void*                      /* pUserData */)
{
    return VK_FALSE;
}


DebugReportCallback::DebugReportCallback (VkInstance instance)
    : instance (instance)
{
    VkDebugReportCallbackCreateInfoEXT createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = cb;
    createInfo.pUserData = this;

    VkResult result = GetVulkanFunction<PFN_vkCreateDebugReportCallbackEXT> (instance, "vkCreateDebugReportCallbackEXT") (instance, &createInfo, nullptr, &handle);

    if (RG_ERROR (result != VK_SUCCESS)) {
        throw std::runtime_error ("Could not create VkDebugReportCallbackEXT.");
    }
}


DebugReportCallback::~DebugReportCallback ()
{
    GetVulkanFunction<PFN_vkDestroyDebugReportCallbackEXT> (instance, "vkDestroyDebugReportCallbackEXT") (instance, handle, nullptr);
    handle = nullptr;
}

} // namespace RG