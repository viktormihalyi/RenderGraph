#include "PhysicalDevice.hpp"
#include "VulkanUtils.hpp"

#include "Utils/BuildType.hpp"
#include "Utils/Utils.hpp"

#include <set>
#include <functional>


namespace RG {


static std::optional<uint32_t> AcceptFirstPresentSupport (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<VkQueueFamilyProperties>& queueFamilies)
{
    if (surface == VK_NULL_HANDLE) {
        return std::nullopt;
    }

    uint32_t i = 0;
    for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
        (void)queueFamily; // TODO
     
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR (physicalDevice, i, surface, &presentSupport);
        if (presentSupport) {
            return i;
        }

        i++;
    }

    return std::nullopt;
}


static auto AcceptFirstWithFlag (VkQueueFlagBits flagbits)
{
    return [=] (VkPhysicalDevice, VkSurfaceKHR, const std::vector<VkQueueFamilyProperties>& props) -> std::optional<uint32_t> {
        uint32_t i = 0;
        for (const auto& p : props) {
            if (p.queueFlags & flagbits) {
                return i;
            }
            ++i;
        }
        return std::nullopt;
    };
}


static PhysicalDevice::QueueFamilies FindQueueFamilyIndices (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    PhysicalDevice::QueueFamilies result;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties (physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies (queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties (physicalDevice, &queueFamilyCount, queueFamilies.data ());

    result.graphics     = AcceptFirstWithFlag (VK_QUEUE_GRAPHICS_BIT) (physicalDevice, surface, queueFamilies);
    result.presentation = AcceptFirstPresentSupport (physicalDevice, surface, queueFamilies);
    result.compute      = AcceptFirstWithFlag (VK_QUEUE_COMPUTE_BIT) (physicalDevice, surface, queueFamilies);
    result.transfer     = AcceptFirstWithFlag (VK_QUEUE_TRANSFER_BIT) (physicalDevice, surface, queueFamilies);

    if (result.presentation) {
        RG_ASSERT (result.graphics == result.presentation); // TODO handle different queue indices ...
    }

    return result;
}


struct PhysicalDeviceCandidate {
    VkPhysicalDevice                   handle;
    VkPhysicalDeviceProperties         properties;
    VkPhysicalDeviceFeatures           features;
    std::vector<VkExtensionProperties> extensionProperties;
};


static std::vector<PhysicalDeviceCandidate> GetPhysicalDeviceCandidates (VkInstance instance)
{
    const std::vector<VkPhysicalDevice> physicalDevices = [instance] {
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices (instance, &physicalDeviceCount, nullptr);
        std::vector<VkPhysicalDevice> result (physicalDeviceCount);
        vkEnumeratePhysicalDevices (instance, &physicalDeviceCount, result.data ());
        return result;
    }();

    std::vector<PhysicalDeviceCandidate> result;

    for (VkPhysicalDevice physicalDevice : physicalDevices) {
        uint32_t deviceExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties (physicalDevice, nullptr, &deviceExtensionCount, nullptr);
        std::vector<VkExtensionProperties> supportedDeviceExtensions (deviceExtensionCount);
        vkEnumerateDeviceExtensionProperties (physicalDevice, nullptr, &deviceExtensionCount, supportedDeviceExtensions.data ());

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties (physicalDevice, &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures (physicalDevice, &features);

        result.push_back (PhysicalDeviceCandidate { physicalDevice, properties, features, supportedDeviceExtensions });
    }

    return result;
}


static VkPhysicalDevice FindPhysicalDevice (VkInstance instance, const std::set<std::string>& requestedDeviceExtensionSet)
{
    std::vector<PhysicalDeviceCandidate> candidates = GetPhysicalDeviceCandidates (instance);

    const auto RemoveBadCandidate = [&] (const std::function<bool (const PhysicalDeviceCandidate&)>& filterFunc) {
        candidates.erase (std::remove_if (candidates.begin (), candidates.end (), filterFunc), candidates.end ());
    };

    RemoveBadCandidate ([&] (const PhysicalDeviceCandidate& candidate) {
        auto extensionNameAccessor = [] (const VkExtensionProperties& props) { return props.extensionName; };

        const std::set<std::string> supportedDeviceExtensionSet   = RG::ToSet<VkExtensionProperties, std::string> (candidate.extensionProperties, extensionNameAccessor);
        const std::set<std::string> unsupportedDeviceExtensionSet = RG::SetDiff (requestedDeviceExtensionSet, supportedDeviceExtensionSet);

        return !unsupportedDeviceExtensionSet.empty ();
    });

    RemoveBadCandidate ([] (const PhysicalDeviceCandidate& candidate) {
        return candidate.features.shaderInt64 == VK_FALSE;
    });

    std::optional<PhysicalDeviceCandidate> selectedCandidate;

    for (const PhysicalDeviceCandidate& candidate : candidates) {
        if (candidate.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            selectedCandidate = candidate;
            break;
        }
    }

    if (!selectedCandidate.has_value () && !candidates.empty ())
        selectedCandidate = candidates[0];

    if RG_ERROR (!selectedCandidate.has_value ())
        throw std::runtime_error ("No physical device available");

    return selectedCandidate->handle;
}


PhysicalDevice::PhysicalDevice (VkInstance instance, VkSurfaceKHR surface, const std::set<std::string>& requestedDeviceExtensionSet)
    : instance (instance)
    , requestedDeviceExtensionSet (requestedDeviceExtensionSet)
    , handle (FindPhysicalDevice (instance, requestedDeviceExtensionSet))
    , queueFamilies (FindQueueFamilyIndices (handle, surface))
{
    if constexpr (IsDebugBuild) {
        properties = std::make_unique<VkPhysicalDeviceProperties> (GetProperties ());
        features   = std::make_unique<VkPhysicalDeviceFeatures> (GetFeatures ());
    }
}


PhysicalDevice::~PhysicalDevice ()
{
    handle = nullptr;
}


bool PhysicalDevice::CheckSurfaceSupported (VkSurfaceKHR surface) const
{
    PhysicalDevice findForSurface { instance, surface, requestedDeviceExtensionSet };
    return handle == findForSurface.handle;
}

} // namespace RG
