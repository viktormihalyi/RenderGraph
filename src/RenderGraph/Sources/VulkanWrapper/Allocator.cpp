#include "Allocator.hpp"
#include "Utils/Assert.hpp"

#include "spdlog/spdlog.h"

#include <stdexcept>

#include <vulkan/vulkan.h>


namespace RG {

Allocator::Allocator (VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
    : handle { VK_NULL_HANDLE }
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice         = physicalDevice;
    allocatorInfo.device                 = device;
    allocatorInfo.instance               = instance;

    if (RG_ERROR (vmaCreateAllocator (&allocatorInfo, &handle) != VK_SUCCESS)) {
        spdlog::critical ("VmaAllocator creation failed.");
        throw std::runtime_error ("failed to create vma allocator");
    }

    spdlog::trace ("VmaAllocator created: {}, uuid: {}.", handle, GetUUID ().GetValue ());
}


Allocator::~Allocator ()
{
    vmaDestroyAllocator (handle);
    handle = nullptr;
}

} // namespace RG
