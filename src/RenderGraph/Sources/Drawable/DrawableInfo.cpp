#include "DrawableInfo.hpp"

#include "VulkanWrapper/Commands.hpp"


namespace RG {

void DrawableInfo::Record (RG::CommandBuffer& commandBuffer) const
{
    if (!vertexBuffer.empty ()) {
        std::vector<VkDeviceSize> offsets (vertexBuffer.size (), 0);
        commandBuffer.Record<RG::CommandBindVertexBuffers> (0, static_cast<uint32_t> (vertexBuffer.size ()), vertexBuffer, offsets).SetName ("DrawableInfo");
    }

    if (indexBuffer != VK_NULL_HANDLE) {
        commandBuffer.Record<RG::CommandBindIndexBuffer> (indexBuffer, 0, VK_INDEX_TYPE_UINT16).SetName ("DrawableInfo");
    }

    if (indexBuffer != VK_NULL_HANDLE) {
        commandBuffer.Record<RG::CommandDrawIndexed> (indexCount, instanceCount, 0, 0, 0).SetName ("DrawableInfo");
    } else {
        commandBuffer.Record<RG::CommandDraw> (vertexCount, instanceCount, 0, 0).SetName ("DrawableInfo");
    }
}

} // namespace RG
