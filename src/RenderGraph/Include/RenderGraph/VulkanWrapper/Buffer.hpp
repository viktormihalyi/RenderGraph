#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vulkan/vulkan.h>

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"

#pragma warning (push, 0)
#include "vk_mem_alloc.h"
#pragma warning(pop)

namespace GVK {

class RENDERGRAPH_DLL_EXPORT Buffer : public VulkanObject {
private:
    VmaAllocator                   allocator;
    GVK::MovablePtr<VkBuffer>      handle;
    GVK::MovablePtr<VmaAllocation> allocationHandle;
    size_t                         size;

public:
    enum class MemoryLocation {
        GPU,
        CPU
    };

    Buffer (VmaAllocator allocator, size_t bufferSize, VkBufferUsageFlags usageFlags, MemoryLocation loc);
    Buffer (Buffer&&) = default;
    Buffer& operator= (Buffer&&) = default;

    virtual ~Buffer () override;
    
    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_BUFFER; }

    operator VkBuffer () const { return handle; }

    operator VmaAllocation () const { return allocationHandle; }
};


class RENDERGRAPH_DLL_EXPORT UniformBuffer : public Buffer {
public:
    UniformBuffer (VmaAllocator allocator, size_t bufferSize, VkBufferUsageFlags usageFlags, MemoryLocation loc)
        : Buffer (allocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | usageFlags, loc)
    {
    }
};


class RENDERGRAPH_DLL_EXPORT StorageBuffer : public Buffer {
public:
    StorageBuffer (VmaAllocator allocator, size_t bufferSize, VkBufferUsageFlags usageFlags, MemoryLocation loc)
        : Buffer (allocator, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | usageFlags, loc)
    {
    }
};


class RENDERGRAPH_DLL_EXPORT IndexBuffer : public Buffer {
public:
    IndexBuffer (VmaAllocator allocator, size_t bufferSize, VkBufferUsageFlags usageFlags, MemoryLocation loc)
        : Buffer (allocator, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | usageFlags, loc)
    {
    }
};


class RENDERGRAPH_DLL_EXPORT VertexBuffer : public Buffer {
public:
    VertexBuffer (VmaAllocator allocator, size_t bufferSize, VkBufferUsageFlags usageFlags, MemoryLocation loc)
        : Buffer (allocator, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | usageFlags, loc)
    {
    }
};

} // namespace GVK

#endif