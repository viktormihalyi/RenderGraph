#include "VulkanUtils.hpp"

#include "ImageData.hpp"
#include "Commands.hpp"
#include "SingleTimeCommand.hpp"

#include <array>
#include <cstring>
#include <optional>
#include <sstream>
#include <string>

#include "spdlog/spdlog.h"

namespace RG {

std::string GetVersionString (uint32_t version)
{
    return fmt::format ("{}.{}.{}", VK_VERSION_MAJOR (version), VK_VERSION_MINOR (version), VK_VERSION_PATCH (version));
}


void TransitionImageLayout (const DeviceExtra& device, const Image& image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    SingleTimeCommand commandBuffer (device);
    commandBuffer.SetName (device, "TransitionImageLayout - SingleTimeCommandBuffer");
    commandBuffer.Record<CommandTranstionImage> (image, oldLayout, newLayout);
}


void CopyBufferToImage (const DeviceExtra& device, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth)
{
    SingleTimeCommand commandBuffer (device);
    commandBuffer.SetName (device, "CopyBufferToImage - SingleTimeCommandBuffer");
    CopyBufferToImage (commandBuffer, buffer, image, width, height, depth);
}


void CopyBufferToImage (CommandBuffer& commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth)
{
    VkBufferImageCopy region               = {};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = { 0, 0, 0 };
    region.imageExtent                     = { width, height, depth };

    commandBuffer.Record<CommandCopyBufferToImage> (
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        std::vector<VkBufferImageCopy> { region });
}


void CopyBuffer (const DeviceExtra& device, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    CopyBufferPart (device, srcBuffer, dstBuffer, size, 0);
}


void CopyBufferPart (const DeviceExtra& device, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize offset)
{
    SingleTimeCommand commandBuffer (device);
    commandBuffer.SetName (device, "CopyBufferPart - SingleTimeCommandBuffer");

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset    = offset;
    copyRegion.dstOffset    = 0;
    copyRegion.size         = size;

    commandBuffer.Record<CommandCopyBuffer> (srcBuffer, dstBuffer, std::vector<VkBufferCopy> { copyRegion });
}


// copy image to cpu and compare with a reference
bool AreImagesEqual (const DeviceExtra& device, const Image& image, const std::filesystem::path& expectedImage, uint32_t layerIndex)
{
    ImageData actualImage (device, image, layerIndex);

    ImageData expectedImageData (expectedImage);

    return actualImage == expectedImage;
}


std::thread SaveImageToFileAsync (const DeviceExtra& device, const Image& image, const std::filesystem::path& filePath, uint32_t layerIndex)
{
    return std::thread ([=, &device, &image] () {
        ImageData (device, image, layerIndex).SaveTo (filePath);
    });
};


uint32_t GetCompontentCountFromFormat (VkFormat format)
{
    switch (format) {
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8_UINT:
            return 1;
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R8G8_UINT:
            return 2;
        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R8G8B8_UINT:
            return 3;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R8G8B8A8_UINT:
            return 4;
        default:
            RG_BREAK ();
            return 4;
    }
}


uint32_t GetEachCompontentSizeFromFormat (VkFormat format)
{
    switch (format) {
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
            return 1;

        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 4;

        default:
            RG_BREAK ();
            return 1;
    }
}

} // namespace RG