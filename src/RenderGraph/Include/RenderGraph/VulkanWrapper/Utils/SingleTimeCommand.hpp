#ifndef SINGLETIMECOMMAND_HPP
#define SINGLETIMECOMMAND_HPP

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/VulkanWrapper/CommandBuffer.hpp"
#include "RenderGraph/VulkanWrapper/DeviceExtra.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"

#include <vulkan/vulkan.h>

namespace RG {

class /* RENDERGRAPH_DLL_EXPORT */ SingleTimeCommand final : public CommandBuffer {
private:
    const Queue& queue;

public:
    SingleTimeCommand (VkDevice device, VkCommandPool commandPool, const Queue& queue)
        : CommandBuffer (device, commandPool)
        , queue (queue)
    {
        Begin (VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    }

    SingleTimeCommand (const DeviceExtra& device)
        : SingleTimeCommand (device, device.GetCommandPool (), device.GetGraphicsQueue ())
    {
    }

    virtual ~SingleTimeCommand () override
    {
        End ();

        queue.Submit ({}, {}, { this }, {}, VK_NULL_HANDLE);
        queue.Wait ();
    }
};

} // namespace RG

#endif