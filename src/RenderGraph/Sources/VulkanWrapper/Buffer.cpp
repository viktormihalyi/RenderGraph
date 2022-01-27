#include "Buffer.hpp"

#include "spdlog/spdlog.h"


namespace RG {


Buffer::Buffer (VmaAllocator allocator, size_t bufferSize, VkBufferUsageFlags usageFlags, MemoryLocation loc)
    : allocator (allocator)
    , handle (VK_NULL_HANDLE)
    , allocationHandle (VK_NULL_HANDLE)
    , size (bufferSize)
{
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size               = bufferSize;
    bufferInfo.usage              = usageFlags;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = (loc == MemoryLocation::GPU) ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_COPY;

    if (loc == MemoryLocation::CPU) {
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }

    if (RG_ERROR (vmaCreateBuffer (allocator, &bufferInfo, &allocInfo, &handle, &allocationHandle, nullptr) != VK_SUCCESS)) {
        spdlog::critical ("VkBuffer creation failed.");
        throw std::runtime_error ("failed to create vma buffer");
    }

    spdlog::trace ("VkBuffer created: {}, uuid: {}.", handle, GetUUID ().GetValue ());
}


Buffer::~Buffer ()
{
    if (allocator != VK_NULL_HANDLE) {
        vmaDestroyBuffer (allocator, handle, allocationHandle);
    }

    handle = nullptr;
}

} // namespace RG