#ifndef DEVICEEXTRA_HPP
#define DEVICEEXTRA_HPP

#include <memory>

#include "Instance.hpp"
#include "CommandPool.hpp"
#include "Device.hpp"
#include "Queue.hpp"

#pragma warning (push, 0)
#include "vk_mem_alloc.h"
#pragma warning(pop)

namespace RG {

class RENDERGRAPH_DLL_EXPORT DeviceExtra : public Device {
public:
    Instance&    instance;
    Device&      device;
    CommandPool& commandPool;
    Queue&       graphicsQueue;
    Queue&       presentationQueue;
    VmaAllocator allocator;

    DeviceExtra (Instance& instance, Device& device, CommandPool& commandPool, VmaAllocator allocator, Queue& graphicsQueue, Queue& presentationQueue = dummyQueue)
        : instance (instance)
        , device (device)
        , commandPool (commandPool)
        , graphicsQueue (graphicsQueue)
        , presentationQueue (presentationQueue)
        , allocator (allocator)
    {
    }

    virtual ~DeviceExtra () override;

    const Instance&    GetInstance () const { return instance; }
    const Device&      GetDevice () const { return device; }
    const CommandPool& GetCommandPool () const { return commandPool; }
    const Queue&       GetGraphicsQueue () const { return graphicsQueue; }
    const Queue&       GetPresentationQueue () const { return presentationQueue; }
    VmaAllocator       GetAllocator () const { return allocator; }

    Instance&    GetInstance () { return instance; }
    Device&      GetDevice () { return device; }
    CommandPool& GetCommandPool () { return commandPool; }
    Queue&       GetGraphicsQueue () { return graphicsQueue; }
    Queue&       GetPresentationQueue () { return presentationQueue; }

    // implementing Device
    virtual      operator VkDevice () const override { return device; }
    virtual void Wait () const override { device.Wait (); }
};

} // namespace RG

#endif