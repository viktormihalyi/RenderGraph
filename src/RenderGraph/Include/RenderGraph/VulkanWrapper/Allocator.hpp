#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"

#pragma warning (push, 0)
#include "vk_mem_alloc.h"
#pragma warning(pop)

#include <stdexcept>

namespace GVK {

class RENDERGRAPH_DLL_EXPORT Allocator : public VulkanObject {
private:
    GVK::MovablePtr<VmaAllocator> handle;

public:
    Allocator (VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
    Allocator (Allocator&&) = default;
    Allocator& operator= (Allocator&&) = default;

    virtual ~Allocator () override;

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { throw std::runtime_error ("Cannot name VmaAllocator."); }

    operator VmaAllocator () const { return handle; }
};

} // namespace GVK

#endif