#include "Image.hpp"
#include "Commands.hpp"

#include "Utils/Assert.hpp"

#include "spdlog/spdlog.h"

namespace GVK {

const VkImageLayout Image::INITIAL_LAYOUT = VK_IMAGE_LAYOUT_UNDEFINED;

Image::Image (VkImage handle, VkDevice device, uint32_t width, uint32_t height, uint32_t depth, VkFormat format, uint32_t arrayLayers)
    : handle (handle)
    , allocator (VK_NULL_HANDLE)
    , allocationHandle (VK_NULL_HANDLE)
    , device (device)
    , format (format)
    , width (width)
    , height (height)
    , depth (depth)
    , arrayLayers (arrayLayers)
{
}


Image::Image (VmaAllocator      allocator,
              VkImageType       imageType,
              uint32_t          width,
              uint32_t          height,
              uint32_t          depth,
              VkFormat          format,
              VkImageTiling     tiling,
              VkImageUsageFlags usage,
              uint32_t          arrayLayers,
              MemoryLocation    loc)
    : device (VK_NULL_HANDLE)
    , handle (VK_NULL_HANDLE)
    , allocator (allocator)
    , allocationHandle (VK_NULL_HANDLE)
    , format (format)
    , width (width)
    , height (height)
    , depth (depth)
    , arrayLayers (arrayLayers)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags             = 0;
    imageInfo.imageType         = imageType;
    imageInfo.extent.width      = width;
    imageInfo.extent.height     = height;
    imageInfo.extent.depth      = depth;
    imageInfo.mipLevels         = 1;
    imageInfo.arrayLayers       = arrayLayers;
    imageInfo.format            = format;
    imageInfo.tiling            = tiling;
    imageInfo.initialLayout     = INITIAL_LAYOUT;
    imageInfo.usage             = usage;
    imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = (loc == MemoryLocation::GPU) ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_COPY;
    
    if (loc == MemoryLocation::CPU) {
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }

    if (GVK_ERROR (vmaCreateImage (allocator, &imageInfo, &allocInfo, &handle, &allocationHandle, nullptr) != VK_SUCCESS)) {
        spdlog::critical ("VkImage creation failed.");
        throw std::runtime_error ("failed to create image!");
    }

    spdlog::trace ("VkImage created: {}, uuid: {}.", handle, GetUUID ().GetValue ());
}


Image::Image (ImageBuilder& imageBuilder)
    : Image (imageBuilder.allocator,
             *imageBuilder.imageType,
             *imageBuilder.width,
             *imageBuilder.height,
             *imageBuilder.depth,
             *imageBuilder.format,
             *imageBuilder.tiling,
             imageBuilder.usage,
             *imageBuilder.arrayLayers,
             *imageBuilder.loc)
{
}


Image::~Image ()
{
    if (handle == VK_NULL_HANDLE) {
        return;
    }

    if (device != VK_NULL_HANDLE) {
        vkDestroyImage (device, handle, nullptr);
    } else if (allocator != VK_NULL_HANDLE) {
        vmaDestroyImage (allocator, handle, allocationHandle);
    }
    handle = nullptr;
}


VkImageMemoryBarrier Image::GetBarrier (VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) const
{
    return GetBarrier (oldLayout, newLayout, srcAccessMask, dstAccessMask, 0, arrayLayers);
}


VkImageMemoryBarrier Image::GetBarrier (VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t layerIndex) const
{
    return GetBarrier (oldLayout, newLayout, srcAccessMask, dstAccessMask, layerIndex, 1);
}


VkImageMemoryBarrier Image::GetBarrier (VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t baseArrayLayer, uint32_t layerCount) const
{
    VkImageMemoryBarrier barrier            = {};
    barrier.pNext                           = nullptr;
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = handle;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
    barrier.subresourceRange.layerCount     = layerCount;
    barrier.srcAccessMask                   = srcAccessMask;
    barrier.dstAccessMask                   = dstAccessMask;
    return barrier;
}


VkBufferImageCopy Image::GetFullBufferImageCopy () const
{
    VkBufferImageCopy result               = {};
    result.bufferOffset                    = 0;
    result.bufferRowLength                 = 0;
    result.bufferImageHeight               = 0;
    result.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    result.imageSubresource.mipLevel       = 0;
    result.imageSubresource.baseArrayLayer = 0;
    result.imageSubresource.layerCount     = arrayLayers;
    result.imageOffset                     = { 0, 0, 0 };
    result.imageExtent                     = { width, height, depth };
    return result;
}


VkBufferImageCopy Image::GetFullBufferImageCopyLayer (uint32_t layer) const
{
    VkBufferImageCopy result               = {};
    result.bufferOffset                    = 0;
    result.bufferRowLength                 = 0;
    result.bufferImageHeight               = 0;
    result.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    result.imageSubresource.mipLevel       = 0;
    result.imageSubresource.baseArrayLayer = layer;
    result.imageSubresource.layerCount     = 1;
    result.imageOffset                     = { 0, 0, 0 };
    result.imageExtent                     = { width, height, depth };
    return result;
}


void Image::CmdCopyToBuffer (CommandBuffer& commandBuffer, VkBuffer buffer) const
{
    VkBufferImageCopy region = GetFullBufferImageCopy ();
    commandBuffer.Record<CommandCopyImageToBuffer> (
        handle,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        buffer,
        std::vector<VkBufferImageCopy> { region });
}


void Image::CmdCopyLayerToBuffer (CommandBuffer& commandBuffer, uint32_t layerIndex, VkBuffer buffer) const
{
    VkBufferImageCopy region = GetFullBufferImageCopyLayer (layerIndex);
    commandBuffer.Record<CommandCopyImageToBuffer> (
        handle,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        buffer,
        std::vector<VkBufferImageCopy> { region });
}


void Image::CmdCopyBufferToImage (CommandBuffer& commandBuffer, VkBuffer buffer) const
{
    VkBufferImageCopy region = GetFullBufferImageCopy ();
    commandBuffer.Record<CommandCopyBufferToImage> (
        buffer,
        handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        std::vector<VkBufferImageCopy> { region });
}


void Image::CmdCopyBufferPartToImage (CommandBuffer& commandBuffer, VkBuffer buffer, VkBufferImageCopy region) const
{
    commandBuffer.Record<CommandCopyBufferToImage> (buffer,
                                                    handle,
                                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                    std::vector<VkBufferImageCopy> { region });
}


Image ImageBuilder::Build () const
{
    const bool allSet = imageType.has_value () &&
                        width.has_value () &&
                        height.has_value () &&
                        depth.has_value () &&
                        format.has_value () &&
                        tiling.has_value () &&
                        usage != 0 &&
                        arrayLayers.has_value () &&
                        loc.has_value ();

    if (GVK_ERROR (!allSet)) {
        throw std::runtime_error ("not all values set");
    }

    return Image (
        allocator,
        *imageType,
        *width,
        *height,
        *depth,
        *format,
        *tiling,
        usage,
        *arrayLayers,
        *loc);
}

} // namespace GVK
