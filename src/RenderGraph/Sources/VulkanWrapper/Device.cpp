#include "Device.hpp"

#include <vector>
#include <stdexcept>


namespace GVK {


Device::~Device () = default;


uint32_t DeviceObject::FindMemoryType (uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memProperties = {};
    vkGetPhysicalDeviceMemoryProperties (physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error ("failed to find suitable memory type!");
}


DeviceObject::DeviceObject (VkPhysicalDevice physicalDevice, std::vector<uint32_t> queueFamilyIndices, std::vector<const char*> requestedDeviceExtensions)
    : physicalDevice (physicalDevice)
    , handle (VK_NULL_HANDLE)
{
    const float queuePriority = 1.0f;
    
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    for (uint32_t queueFamilyIndex : queueFamilyIndices) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex        = queueFamilyIndex;
        queueCreateInfo.queueCount              = 1;
        queueCreateInfo.pQueuePriorities        = &queuePriority;
        queueCreateInfos.push_back (queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.shaderInt64 = VK_TRUE;

    VkDeviceCreateInfo createInfo      = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t> (queueCreateInfos.size ());
    createInfo.pQueueCreateInfos       = queueCreateInfos.data ();
    createInfo.pEnabledFeatures        = &deviceFeatures;
    createInfo.enabledExtensionCount   = static_cast<uint32_t> (requestedDeviceExtensions.size ());
    createInfo.ppEnabledExtensionNames = requestedDeviceExtensions.data ();
    createInfo.enabledLayerCount       = 0;       // deprecated
    createInfo.ppEnabledLayerNames     = nullptr; // deprecated

    if (GVK_ERROR (vkCreateDevice (physicalDevice, &createInfo, nullptr, &handle) != VK_SUCCESS)) {
        throw std::runtime_error ("failed to create device");
    }
}


DeviceObject::~DeviceObject ()
{
    if (handle != nullptr) {
        vkDeviceWaitIdle (handle);
        vkDestroyDevice (handle, nullptr);
        handle = nullptr;
    }
}


} // namespace GVK
