#include "Commands.hpp"

namespace GVK {

const VkAccessFlags CommandPipelineBarrierFull::flushAll = VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
                                                           VK_ACCESS_INDEX_READ_BIT |
                                                           VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                                                           VK_ACCESS_UNIFORM_READ_BIT |
                                                           VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
                                                           VK_ACCESS_SHADER_READ_BIT |
                                                           VK_ACCESS_SHADER_WRITE_BIT |
                                                           VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                                           VK_ACCESS_TRANSFER_READ_BIT |
                                                           VK_ACCESS_TRANSFER_WRITE_BIT;


void CommandBindVertexBuffers::Record (CommandBuffer& commandBuffer)
{
    vkCmdBindVertexBuffers (commandBuffer.GetHandle (), firstBinding, bindingCount, pBuffers.data (), pOffsets.data ());
}


std::string CommandBindVertexBuffers::ToString () const
{
    return std::string ("vkCmdBindVertexBuffers (") + std::to_string (firstBinding) + ", " + std::to_string (bindingCount) + ", [buffers], [offsets])";
}


void CommandPipelineBarrier::Record (CommandBuffer& commandBuffer)
{
    vkCmdPipelineBarrier (
        commandBuffer.GetHandle (),
        srcStageMask,
        dstStageMask,
        0, // TODO
        static_cast<uint32_t> (memoryBarriers.size ()), memoryBarriers.data (),
        static_cast<uint32_t> (bufferMemoryBarriers.size ()), bufferMemoryBarriers.data (),
        static_cast<uint32_t> (imageMemoryBarriers.size ()), imageMemoryBarriers.data ());
}

} // namespace GVK
