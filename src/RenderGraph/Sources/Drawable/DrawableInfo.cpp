#include "DrawableInfo.hpp"

#include "VulkanWrapper/Commands.hpp"


namespace RG {

void DrawableInfo::Record (GVK::CommandBuffer& commandBuffer) const
{
    if (!vertexBuffer.empty ()) {
        std::vector<VkDeviceSize> offsets (vertexBuffer.size (), 0);
        commandBuffer.Record<GVK::CommandBindVertexBuffers> (0, static_cast<uint32_t> (vertexBuffer.size ()), vertexBuffer, offsets).SetName ("DrawableInfo");
    }

    if (indexBuffer != VK_NULL_HANDLE) {
        commandBuffer.Record<GVK::CommandBindIndexBuffer> (indexBuffer, 0, VK_INDEX_TYPE_UINT16).SetName ("DrawableInfo");
    }

    if (indexBuffer != VK_NULL_HANDLE) {
        commandBuffer.Record<GVK::CommandDrawIndexed> (indexCount, instanceCount, 0, 0, 0).SetName ("DrawableInfo");
    } else {
        commandBuffer.Record<GVK::CommandDraw> (vertexCount, instanceCount, 0, 0).SetName ("DrawableInfo");
    }
}

} // namespace RG
