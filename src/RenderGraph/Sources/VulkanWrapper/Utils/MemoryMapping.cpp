#include "MemoryMapping.hpp"

#include "DeviceMemory.hpp"

#include <stdexcept>

namespace RG {


MemoryMapping::MemoryMapping (VkDevice device, VkDeviceMemory memory, size_t offset, size_t size)
    : device (device)
    , allocator (VK_NULL_HANDLE)
    , allocationHandle (VK_NULL_HANDLE)
    , memory (memory)
    , offset (offset)
    , size (size)
    , mappedMemory (nullptr)
{
    if (RG_ERROR (vkMapMemory (device, memory, offset, size, 0, &mappedMemory) != VK_SUCCESS)) {
        throw std::runtime_error ("failed to map memory");
    }
}


MemoryMapping::MemoryMapping (VkDevice device, const DeviceMemory& memory)
    : MemoryMapping (device, memory, 0, memory.GetSize ())
{
}


MemoryMapping::MemoryMapping (VmaAllocator allocator, VmaAllocation allocationHandle)
    : device (VK_NULL_HANDLE)
    , memory (VK_NULL_HANDLE)
    , allocator (allocator)
    , allocationHandle (allocationHandle)
    , offset (0)
    , size (0)
    , mappedMemory (nullptr)
{
    VmaAllocationInfo allocInfo = {};
    vmaGetAllocationInfo (allocator, allocationHandle, &allocInfo);
    size = allocInfo.size;

    if (RG_ERROR (vmaMapMemory (allocator, allocationHandle, &mappedMemory) != VK_SUCCESS)) {
        throw std::runtime_error ("failed to map memory");
    }
}


MemoryMapping::~MemoryMapping ()
{
    if (device != VK_NULL_HANDLE) {
        vkUnmapMemory (device, memory);
    } else if (allocator != VK_NULL_HANDLE) {
        vmaUnmapMemory (allocator, allocationHandle);
    }
    mappedMemory = nullptr;
}


void MemoryMapping::Copy (const void* data, size_t copiedSize) const
{
    if (RG_ERROR (copiedSize > size)) {
        throw std::runtime_error ("overflow");
    }

    memcpy (mappedMemory, reinterpret_cast<const uint8_t*> (data), copiedSize);
}


} // namespace RG
