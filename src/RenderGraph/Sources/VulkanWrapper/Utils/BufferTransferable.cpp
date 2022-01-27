#include "BufferTransferable.hpp"

#include "VulkanWrapper/CommandBuffer.hpp"
#include "VulkanWrapper/Utils/SingleTimeCommand.hpp"
#include "VulkanWrapper/Commands.hpp"
#include "VulkanWrapper/Utils/VulkanUtils.hpp"

#include <cmath>

namespace GVK {

struct ShaderType {
    uint32_t size;
    uint32_t alignment;
};

static ShaderType vec1 {4, 4};
static ShaderType vec2 {8, 8};
static ShaderType vec3 {12, 16};
static ShaderType vec4 {12, 16};

template<uint32_t SIZE>
static ShaderType vec1Array {4 * SIZE, 32};
template<uint32_t SIZE>
static ShaderType vec2Array {8 * SIZE, 32};
template<uint32_t SIZE>
static ShaderType vec3Array {12 * SIZE, 32};
template<uint32_t SIZE>
static ShaderType vec4Array {16 * SIZE, 32};


static ShaderType GetShaderTypeFromFormat (VkFormat format)
{
    switch (format) {
        case VK_FORMAT_R32_SFLOAT: return vec1;
        case VK_FORMAT_R32G32_SFLOAT: return vec2;
        case VK_FORMAT_R32G32B32_SFLOAT: return vec3;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return vec4;
        case VK_FORMAT_R32_UINT: return vec1;
        case VK_FORMAT_R32G32_UINT: return vec2;
        case VK_FORMAT_R32G32B32_UINT: return vec3;
        case VK_FORMAT_R32G32B32A32_UINT: return vec4;

        default:
            GVK_BREAK ();
            throw std::runtime_error ("unhandled VkFormat value");
    }
}


VertexInputInfo::VertexInputInfo (const std::vector<VkFormat>& vertexInputFormats, VkVertexInputRate inputRate)
    : size (0)
{
    uint32_t location = 0;
    size              = 0;

    uint32_t attributeSize = 0;

    for (VkFormat format : vertexInputFormats) {
        VkVertexInputAttributeDescription attrib;

        attrib.binding  = 0;
        attrib.location = location;
        attrib.format   = format;
        attrib.offset   = attributeSize;
        attributes.push_back (attrib);

        ++location;
        size += GetShaderTypeFromFormat (format).size;
        attributeSize += GetShaderTypeFromFormat (format).size;
    }

    VkVertexInputBindingDescription bindingDescription = {};

    bindingDescription           = {};
    bindingDescription.binding   = 0;
    bindingDescription.stride    = size;
    bindingDescription.inputRate = inputRate;

    bindings = {bindingDescription};
}


void BufferTransferable::TransferFromCPUToGPU (const void* data, size_t size) const
{
    GVK_ASSERT (size == bufferSize);
    bufferCPUMapping.Copy (data, size);
    CopyBuffer (device, bufferCPU, bufferGPU, bufferSize);
}


void BufferTransferable::TransferFromGPUToCPU () const
{
    CopyBuffer (device, bufferGPU, bufferCPU, bufferSize);
}


void BufferTransferable::TransferFromGPUToCPU (VkDeviceSize size, VkDeviceSize offset) const
{
    CopyBufferPart (device, bufferGPU, bufferCPU, size, offset);
}


void ImageTransferable::CopyLayer (VkImageLayout currentImageLayout, const void* data, size_t size, uint32_t layerIndex, std::optional<VkImageLayout> nextLayout) const
{
    bufferCPUMapping.Copy (data, size);

    SingleTimeCommand commandBuffer (device);

    commandBuffer.Record<CommandTranstionImage> (*imageGPU, currentImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region               = {};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = layerIndex;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = { 0, 0, 0 };
    region.imageExtent                     = { imageGPU->GetWidth (), imageGPU->GetHeight (), imageGPU->GetDepth () };

    imageGPU->CmdCopyBufferPartToImage (commandBuffer, bufferCPU, region);

    if (nextLayout.has_value ()) {
        commandBuffer.Record<CommandTranstionImage> (*imageGPU, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, *nextLayout);
    }
}


Image1DTransferable::Image1DTransferable (const DeviceExtra& device, VkFormat format, uint32_t width, VkImageUsageFlags usageFlags)
    : ImageTransferable (device, width * GetCompontentCountFromFormat (format) * GetEachCompontentSizeFromFormat (format))
{
    imageGPU = std::make_unique<Image1D> (device.GetAllocator (), Image::MemoryLocation::GPU, width, format, VK_IMAGE_TILING_OPTIMAL, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags);
}


Image2DTransferable::Image2DTransferable (const DeviceExtra& device, VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usageFlags, uint32_t arrayLayers)
    : ImageTransferable (device, width * height * GetCompontentCountFromFormat (format) * GetEachCompontentSizeFromFormat (format))
{
    imageGPU = std::make_unique<Image2D> (device.GetAllocator (), Image::MemoryLocation::GPU, width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags, arrayLayers);
}


Image2DTransferableLinear::Image2DTransferableLinear (const DeviceExtra& device, VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usageFlags, uint32_t arrayLayers)
    : ImageTransferable (device, width * height * GetCompontentCountFromFormat (format) * GetEachCompontentSizeFromFormat (format))
{
    imageGPU = std::make_unique<Image2D> (device.GetAllocator (), Image::MemoryLocation::GPU, width, height, format, VK_IMAGE_TILING_LINEAR, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags, arrayLayers);
}


Image3DTransferable::Image3DTransferable (const DeviceExtra& device, VkFormat format, uint32_t width, uint32_t height, uint32_t depth, VkImageUsageFlags usageFlags)
    : ImageTransferable (device, width * height * depth * GetCompontentCountFromFormat (format) * GetEachCompontentSizeFromFormat (format))
{
    imageGPU = std::make_unique<Image3D> (device.GetAllocator (), Image::MemoryLocation::GPU, width, height, depth, format, VK_IMAGE_TILING_OPTIMAL, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags);
}

}
