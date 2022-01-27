#ifndef MEMORYMAPPING_HPP
#define MEMORYMAPPING_HPP

#include <vulkan/vulkan.h>

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"

#pragma warning (push, 0)
#include "vk_mem_alloc.h"
#pragma warning(pop)

#include <cstring>
#include <vector>

namespace RG {

class DeviceMemory;

class RENDERGRAPH_DLL_EXPORT MemoryMapping {
private:
    RG::MovablePtr<VkDevice>       device;
    RG::MovablePtr<VkDeviceMemory> memory;

    RG::MovablePtr<VmaAllocator>  allocator;
    RG::MovablePtr<VmaAllocation> allocationHandle;

    size_t offset;
    size_t size;

    RG::MovablePtr<void*> mappedMemory;

public:
    MemoryMapping (VkDevice device, VkDeviceMemory memory, size_t offset, size_t size);
    MemoryMapping (VkDevice device, const DeviceMemory& memory);
    MemoryMapping (VmaAllocator allocator, VmaAllocation allocationHandle);

    ~MemoryMapping ();

    template<typename T>
    void Copy (const std::vector<T>& obj) const
    {
        RG_ASSERT (sizeof (T) * obj.size () == size);
        memcpy (mappedMemory, obj.data (), sizeof (T) * obj.size ());
    }

    template<typename T>
    void Copy (const T& obj) const
    {
        const size_t copiedObjSize = sizeof (T);
        RG_ASSERT (copiedObjSize <= size);
        memcpy (mappedMemory, &obj, size);
    }

    void Copy (const void* data, size_t copiedSize) const;

    void*  Get () const { return mappedMemory; }
    size_t GetSize () const { return size; }
    size_t GetOffset () const { return offset; }
};

} // namespace RG

#endif