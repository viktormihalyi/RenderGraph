#include "Instance.hpp"

#include "Utils/Assert.hpp"
#include "Utils/CommandLineFlag.hpp"

#include "Utils/Utils.hpp"
#include "VulkanUtils.hpp"

#include "spdlog/spdlog.h"

#include <sstream>

namespace RG {

const InstanceSettings instanceDebugMode { { VK_EXT_DEBUG_UTILS_EXTENSION_NAME }, { "VK_LAYER_KHRONOS_validation" } };
const InstanceSettings instanceReleaseMode { {}, {} };


RG::CommandLineOnOffFlag enableShaderPrintfFlag { "--enableShaderPrintf", "Enables debugPrintfEXT in shaders. (And turns off GPU validation.)" };


static VkInstance CreateInstance (const std::vector<const char*>& instanceExtensions, const std::vector<const char*>& instanceLayers)
{
    auto extensionNameAccessor = [] (const VkExtensionProperties& props) { return props.extensionName; };
    auto layerNameAccessor     = [] (const VkLayerProperties& props) { return props.layerName; };

    const std::set<std::string> requiredExtensionSet = RG::ToSet<const char*, std::string> (instanceExtensions);

    // supported extensions
    std::set<std::string> supportedExtensionSet;
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties (nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> supportedExtensions (extensionCount);
        vkEnumerateInstanceExtensionProperties (nullptr, &extensionCount, supportedExtensions.data ());

        supportedExtensionSet = RG::ToSet<VkExtensionProperties, std::string> (supportedExtensions, extensionNameAccessor);
    }

    // check if the required extensions are supported
    {
        const std::set<std::string> unsupportedExtensionSet = RG::SetDiff (requiredExtensionSet, supportedExtensionSet);
        if (RG_ERROR (!unsupportedExtensionSet.empty ())) {
            spdlog::critical ("VkIntance: not all instance extensions are supported.");
            throw std::runtime_error ("not all instance extensions are supported");
        }
    }


    std::set<std::string> requiredValidationLayerSet = RG::ToSet<const char*, std::string> (instanceLayers);

    // supported validation layers
    std::set<std::string> supportedValidationLayerSet;
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties (&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers (layerCount);
        vkEnumerateInstanceLayerProperties (&layerCount, availableLayers.data ());

        supportedValidationLayerSet = RG::ToSet<VkLayerProperties, std::string> (availableLayers, layerNameAccessor);
    }

    // check if the required validation layers are supported
    {
        const std::set<std::string> unsupportedValidationLayerSet = RG::SetDiff (requiredValidationLayerSet, supportedValidationLayerSet);
        if (RG_ERROR (!unsupportedValidationLayerSet.empty ())) {
            spdlog::critical ("VkIntance: not all validation layers are supported.");
            throw std::runtime_error ("not all validation layers are supported");
        }
    }


    VkApplicationInfo appInfo  = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "GearsVk";
    appInfo.applicationVersion = VK_MAKE_VERSION (1, 0, 0);
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION (1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_2;

    const std::vector<VkValidationFeatureEnableEXT> gpuAssistedValidationFeatures = {
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
    };

    const std::vector<VkValidationFeatureEnableEXT> debugPrintfFeature = {
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    };

    const std::vector<VkValidationFeatureEnableEXT> selectedValidationFeatures = enableShaderPrintfFlag.IsFlagOn () ? debugPrintfFeature : gpuAssistedValidationFeatures;

    VkValidationFeaturesEXT validationFeatures       = {};
    validationFeatures.pNext                         = nullptr;
    validationFeatures.sType                         = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validationFeatures.enabledValidationFeatureCount = static_cast<uint8_t> (selectedValidationFeatures.size ());
    validationFeatures.pEnabledValidationFeatures    = selectedValidationFeatures.data ();

    VkInstanceCreateInfo createInfo    = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext                   = &validationFeatures;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = static_cast<uint32_t> (instanceExtensions.size ());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data ();
    createInfo.enabledLayerCount       = static_cast<uint32_t> (instanceLayers.size ());
    createInfo.ppEnabledLayerNames     = instanceLayers.data ();

    VkInstance instance = VK_NULL_HANDLE;
    VkResult   result   = vkCreateInstance (&createInfo, nullptr, &instance);
    if (RG_ERROR (result != VK_SUCCESS)) {
        spdlog::critical ("VkInstance creation failed.");
        throw std::runtime_error ("failed to create vulkan instance");
    }

    return instance;
}


Instance::Instance (const std::vector<const char*>& instanceExtensions, const std::vector<const char*>& instanceLayers)
    : handle (CreateInstance (instanceExtensions, instanceLayers))
{
    spdlog::trace ("VkInstance created: {}, uuid: {}.", handle, GetUUID ().GetValue ());
}


Instance::Instance (const InstanceSettings& settings)
    : Instance (settings.extensions, settings.layers)
{
}


Instance::~Instance ()
{
    vkDestroyInstance (handle, nullptr);
    handle = nullptr;
}

} // namespace RG
