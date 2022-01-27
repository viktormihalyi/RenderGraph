#include "Queue.hpp"

#include "CommandBuffer.hpp"

#include "spdlog/spdlog.h"


namespace RG {

Queue dummyQueue (VK_NULL_HANDLE);


void Queue::Submit (const std::vector<VkSemaphore>&          waitSemaphores,
                    const std::vector<VkPipelineStageFlags>& waitDstStageMasks,
                    const std::vector<CommandBuffer>&       commandBuffers,
                    const std::vector<VkSemaphore>&          signalSemaphores,
                    VkFence                                  fenceToSignal) const
{
    std::vector<VkCommandBuffer> submittedCmdBufferHandles;
    submittedCmdBufferHandles.reserve (commandBuffers.size ());

    for (const CommandBuffer& cmd : commandBuffers) {
        submittedCmdBufferHandles.push_back (cmd.GetHandle ());
    }

    VkSubmitInfo result         = {};
    result.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    result.waitSemaphoreCount   = static_cast<uint32_t> (waitSemaphores.size ());
    result.pWaitSemaphores      = waitSemaphores.data ();
    result.pWaitDstStageMask    = waitDstStageMasks.data ();
    result.commandBufferCount   = static_cast<uint32_t> (submittedCmdBufferHandles.size ());
    result.pCommandBuffers      = submittedCmdBufferHandles.data ();
    result.signalSemaphoreCount = static_cast<uint32_t> (signalSemaphores.size ());
    result.pSignalSemaphores    = signalSemaphores.data ();

    vkQueueSubmit (handle, 1, &result, fenceToSignal);
}


void Queue::Submit (const std::vector<VkSemaphore>&          waitSemaphores,
                    const std::vector<VkPipelineStageFlags>& waitDstStageMasks,
                    const std::vector<CommandBuffer*>&       commandBuffers,
                    const std::vector<VkSemaphore>&          signalSemaphores,
                    VkFence                                  fenceToSignal) const
{
    std::vector<VkCommandBuffer> submittedCmdBufferHandles;
    submittedCmdBufferHandles.reserve (commandBuffers.size ());

    for (CommandBuffer* cmd : commandBuffers) {
        RG_ASSERT (cmd != nullptr);
        submittedCmdBufferHandles.push_back (cmd->GetHandle ());
    }

    VkSubmitInfo result         = {};
    result.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    result.waitSemaphoreCount   = static_cast<uint32_t> (waitSemaphores.size ());
    result.pWaitSemaphores      = waitSemaphores.data ();
    result.pWaitDstStageMask    = waitDstStageMasks.data ();
    result.commandBufferCount   = static_cast<uint32_t> (submittedCmdBufferHandles.size ());
    result.pCommandBuffers      = submittedCmdBufferHandles.data ();
    result.signalSemaphoreCount = static_cast<uint32_t> (signalSemaphores.size ());
    result.pSignalSemaphores    = signalSemaphores.data ();

    vkQueueSubmit (handle, 1, &result, fenceToSignal);

    if (spdlog::get_level () <= spdlog::level::trace) {
        std::string ss;
        for (CommandBuffer* cmd : commandBuffers) {
            ss += fmt::format ("{}", static_cast<void*> (cmd->GetHandle ()));
            const bool isLast = cmd == commandBuffers.back ();
            if (!isLast) {
                ss += ", ";
            }
        }
        spdlog::trace ("VkQueue: Submitted command buffers: {}.", ss);
    }
}

}