#ifndef DEBUGUTILSMESSENGER_HPP
#define DEBUGUTILSMESSENGER_HPP

#include <vulkan/vulkan.h>

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"

#include <functional>

namespace RG {

class RENDERGRAPH_DLL_EXPORT DebugUtilsMessenger : public VulkanObject {
public:
    using Callback = std::function<void (VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                         VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData)>;

    struct Settings {
        bool verbose;
        bool info;
        bool warning;
        bool error;

        bool general;
        bool validation;
        bool perforamnce;
    };

private:
    VkInstance                                instance;
    RG::MovablePtr<VkDebugUtilsMessengerEXT> handle;
    Callback                                  callback;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (
        VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void*                                       userData);

public:
    static const Settings defaultSettings;
    static const Settings noPerformance;
    static const Settings all;

    DebugUtilsMessenger (VkInstance instance, const Callback& callback, const Settings& settings = defaultSettings);

    DebugUtilsMessenger (DebugUtilsMessenger&&) = default;
    DebugUtilsMessenger& operator= (DebugUtilsMessenger&&) = default;

    virtual ~DebugUtilsMessenger () override;

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT; }

    operator VkDebugUtilsMessengerEXT () const { return handle; }
};

} // namespace RG

#endif