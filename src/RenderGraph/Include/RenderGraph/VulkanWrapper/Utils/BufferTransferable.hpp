#ifndef BUFFER_TRANSFERABLE_HPP
#define BUFFER_TRANSFERABLE_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/VulkanWrapper/Buffer.hpp"
#include "RenderGraph/VulkanWrapper/DeviceExtra.hpp"
#include "RenderGraph/VulkanWrapper/Image.hpp"
#include "RenderGraph/VulkanWrapper/Utils/MemoryMapping.hpp"
#include <memory>

#include <cstring>

namespace RG {

class RENDERGRAPH_DLL_EXPORT BufferTransferable final {
public:
    const DeviceExtra& device;

    size_t bufferSize;

    Buffer        bufferGPU;
    Buffer        bufferCPU;
    MemoryMapping bufferCPUMapping;

    BufferTransferable (const DeviceExtra& device, size_t bufferSize, VkBufferUsageFlags usageFlags)
        : device (device)
        , bufferSize (bufferSize)
        , bufferGPU (device.GetAllocator (), bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags, Buffer::MemoryLocation::GPU)
        , bufferCPU (device.GetAllocator (), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | usageFlags, Buffer::MemoryLocation::CPU)
        , bufferCPUMapping (device.GetAllocator (), bufferCPU)
    {
    }

    void TransferFromCPUToGPU (const void* data, size_t size) const;

    void TransferFromGPUToCPU () const;
    void TransferFromGPUToCPU (VkDeviceSize size, VkDeviceSize offset) const;

    VkBuffer GetBufferToBind () const
    {
        return bufferGPU;
    }
};


class RENDERGRAPH_DLL_EXPORT ImageTransferable {
private:
    const DeviceExtra& device;

    Buffer        bufferCPU;
    MemoryMapping bufferCPUMapping;

public:
    std::unique_ptr<Image> imageGPU;

protected:
    ImageTransferable (const DeviceExtra& device, size_t bufferSize)
        : device (device)
        , bufferCPU (device.GetAllocator (), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, Buffer::MemoryLocation::CPU)
        , bufferCPUMapping (device.GetAllocator (), bufferCPU)
    {
    }

public:
    virtual ~ImageTransferable () = default;

    void CopyTransitionTransfer (VkImageLayout currentImageLayout, const void* data, size_t size, std::optional<VkImageLayout> nextLayout = std::nullopt) const
    {
        CopyLayer (currentImageLayout, data, size, 0, nextLayout);
    }

    void CopyLayer (VkImageLayout currentImageLayout, const void* data, size_t size, uint32_t layerIndex, std::optional<VkImageLayout> nextLayout = std::nullopt) const;

    VkImage GetImageToBind () const
    {
        return *imageGPU;
    }
};


class RENDERGRAPH_DLL_EXPORT Image1DTransferable final : public ImageTransferable {
public:
    Image1DTransferable (const DeviceExtra& device, VkFormat format, uint32_t width, VkImageUsageFlags usageFlags);
    virtual ~Image1DTransferable () override = default;
};


class RENDERGRAPH_DLL_EXPORT Image2DTransferable final : public ImageTransferable {
public:
    Image2DTransferable (const DeviceExtra& device, VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usageFlags, uint32_t arrayLayers = 1);
    virtual ~Image2DTransferable () override = default;
};


class RENDERGRAPH_DLL_EXPORT Image2DTransferableLinear final : public ImageTransferable {
public:
    Image2DTransferableLinear (const DeviceExtra& device, VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usageFlags, uint32_t arrayLayers = 1);
    virtual ~Image2DTransferableLinear () override = default;
};


class RENDERGRAPH_DLL_EXPORT Image3DTransferable final : public ImageTransferable {
public:
    Image3DTransferable (const DeviceExtra& device, VkFormat format, uint32_t width, uint32_t height, uint32_t depth, VkImageUsageFlags usageFlags);
    virtual ~Image3DTransferable () override = default;
};


class RENDERGRAPH_DLL_EXPORT VertexInputInfo final {
public:
    uint32_t                                       size;
    std::vector<VkVertexInputAttributeDescription> attributes;
    std::vector<VkVertexInputBindingDescription>   bindings;

    VertexInputInfo (const std::vector<VkFormat>& vertexInputFormats, VkVertexInputRate inputRate);
};


class /* RENDERGRAPH_DLL_EXPORT */ VertexBufferTransferableUntyped {
public:
    std::vector<uint8_t>     data;
    const VertexInputInfo    info;
    const BufferTransferable buffer;
    const uint32_t           vertexSize;

    VertexBufferTransferableUntyped (const DeviceExtra& device, uint32_t vertexSize, uint32_t maxVertexCount, const std::vector<VkFormat>& vertexInputFormats, VkVertexInputRate inputRate)
        : info (vertexInputFormats, inputRate)
        , buffer (device, info.size * maxVertexCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
        , vertexSize (vertexSize)
    {
        data.resize (vertexSize * maxVertexCount);
    }

    void Flush () const
    {
        buffer.TransferFromCPUToGPU (data.data (), data.size ());
    }

    void Bind (VkCommandBuffer commandBuffer) const
    {
        VkBuffer buffers[1] = { buffer.GetBufferToBind () };
        uint64_t offsets[1] = { 0 };
        vkCmdBindVertexBuffers (commandBuffer, 0, 1, buffers, offsets);
    }
};


template<typename VertexType>
class /* RENDERGRAPH_DLL_EXPORT */ VertexBufferTransferable : public VertexBufferTransferableUntyped {
public:
    VertexBufferTransferable (const DeviceExtra&           device,
                              uint32_t                     maxVertexCount,
                              const std::vector<VkFormat>& vertexInputFormats,
                              VkVertexInputRate            inputRate)
        : VertexBufferTransferableUntyped (device, sizeof (VertexType), maxVertexCount, vertexInputFormats, inputRate)
    {
    }

    void operator= (const std::vector<VertexType>& copiedData)
    {
        const size_t copiedBytes = copiedData.size () * sizeof (VertexType);
        RG_ASSERT (copiedBytes <= data.size ());
        memcpy (data.data (), copiedData.data (), copiedBytes);
    }
};


template<typename IndexType, VkIndexType VulkanIndexType>
class /* RENDERGRAPH_DLL_EXPORT */ IndexBufferTransferableBase {
public:
    std::vector<IndexType>   data;
    const BufferTransferable buffer;

    IndexBufferTransferableBase (const DeviceExtra& device, uint32_t maxIndexCount)
        : buffer (device, sizeof (IndexType) * maxIndexCount, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
    {
        data.resize (maxIndexCount);
    }

    void Flush () const
    {
        buffer.TransferFromCPUToGPU (data.data (), sizeof (IndexType) * data.size ());
    }

    void operator= (const std::vector<IndexType>& copiedData)
    {
        RG_ASSERT (copiedData.size () == data.size ());
        memcpy (data.data (), copiedData.data (), copiedData.size () * sizeof (IndexType));
    }

    void Bind (VkCommandBuffer commandBuffer) const
    {
        vkCmdBindIndexBuffer (commandBuffer, buffer.GetBufferToBind (), 0, VulkanIndexType);
    }
};

using IndexBufferTransferable    = IndexBufferTransferableBase<uint16_t, VK_INDEX_TYPE_UINT16>;
using IndexBufferTransferableU   = std::unique_ptr<IndexBufferTransferableBase<uint16_t, VK_INDEX_TYPE_UINT16>>;
using IndexBufferTransferable32  = IndexBufferTransferableBase<uint32_t, VK_INDEX_TYPE_UINT32>;
using IndexBufferTransferable32U = std::unique_ptr<IndexBufferTransferableBase<uint32_t, VK_INDEX_TYPE_UINT32>>;

} // namespace RG

#endif