#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <vulkan/vulkan.h>

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"

#include <vector>

namespace RG {

class CommandBuffer;

class RENDERGRAPH_DLL_EXPORT Queue : public Noncopyable, public Nonmovable {
private:
    RG::MovablePtr<VkQueue> handle;

public:
    Queue (VkDevice device, uint32_t index)
    {
        vkGetDeviceQueue (device, index, 0, &handle); // only one queue per device
    }

    Queue (VkQueue handle)
        : handle (handle)
    {
    }

    virtual ~Queue () override
    {
        handle = nullptr;
    }

    operator VkQueue () const
    {
        return handle;
    }

    void Wait () const
    {
        vkQueueWaitIdle (handle);
    }

    void Submit (const std::vector<VkSemaphore>&          waitSemaphores,
                 const std::vector<VkPipelineStageFlags>& waitDstStageMasks,
                 const std::vector<CommandBuffer*>&       commandBuffers,
                 const std::vector<VkSemaphore>&          signalSemaphores,
                 VkFence                                  fenceToSignal) const;

    void Submit (const std::vector<VkSemaphore>&          waitSemaphores,
                 const std::vector<VkPipelineStageFlags>& waitDstStageMasks,
                 const std::vector<CommandBuffer>&       commandBuffers,
                 const std::vector<VkSemaphore>&          signalSemaphores,
                 VkFence                                  fenceToSignal) const;
};

RENDERGRAPH_DLL_EXPORT extern Queue dummyQueue;

} // namespace RG

#endif