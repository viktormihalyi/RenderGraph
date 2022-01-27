#include "ImageView.hpp"


namespace GVK {

ImageViewBase::ImageViewBase (VkDevice device, VkImage image, VkFormat format, VkImageViewType viewType, uint32_t layerIndex, uint32_t layerCount)
    : device (device)
    , format (format)
    , handle (VK_NULL_HANDLE)
{
    VkImageViewCreateInfo createInfo           = {};
    createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.flags                           = 0;
    createInfo.image                           = image;
    createInfo.viewType                        = viewType;
    createInfo.format                          = format;
    createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = layerIndex;
    createInfo.subresourceRange.layerCount     = layerCount;

    VkResult result = vkCreateImageView (device, &createInfo, nullptr, &handle);
    if (GVK_ERROR (result != VK_SUCCESS)) {
        throw std::runtime_error ("failed to create swapchain image views");
    }
}


ImageViewBase::ImageViewBase (VkDevice device, const Image2D& image, VkImageViewType viewType, uint32_t layerIndex)
    : ImageViewBase (device, image, image.GetFormat (), viewType, layerIndex)
{
}


ImageViewBase::~ImageViewBase ()
{
    vkDestroyImageView (device, handle, nullptr);
    handle = nullptr;
}


ImageView1D::ImageView1D (VkDevice device, VkImage image, VkFormat format, uint32_t layerIndex)
    : ImageViewBase (device, image, format, VK_IMAGE_VIEW_TYPE_1D, layerIndex)
{
}


ImageView1D::ImageView1D (VkDevice device, const Image& image, uint32_t layerIndex)
    : ImageView1D (device, image, image.GetFormat (), layerIndex)
{
}


ImageView2D::ImageView2D (VkDevice device, VkImage image, VkFormat format, uint32_t layerIndex, uint32_t layerCount)
    : ImageViewBase (device, image, format, VK_IMAGE_VIEW_TYPE_2D, layerIndex, layerCount)
{
}


ImageView2D::ImageView2D (VkDevice device, const Image& image, uint32_t layerIndex, uint32_t layerCount)
    : ImageView2D (device, image, image.GetFormat (), layerIndex, layerCount)
{
}


ImageView2DArray::ImageView2DArray (VkDevice device, VkImage image, VkFormat format, uint32_t layerIndex, uint32_t layerCount)
    : ImageViewBase (device, image, format, VK_IMAGE_VIEW_TYPE_2D, layerIndex, layerCount)
{
}


ImageView2DArray::ImageView2DArray (VkDevice device, const Image& image, uint32_t layerIndex, uint32_t layerCount)
    : ImageView2DArray (device, image, image.GetFormat (), layerIndex, layerCount)
{
}


ImageView3D::ImageView3D (VkDevice device, VkImage image, VkFormat format, uint32_t layerIndex)
    : ImageViewBase (device, image, format, VK_IMAGE_VIEW_TYPE_3D, layerIndex)
{
}


ImageView3D::ImageView3D (VkDevice device, const Image& image, uint32_t layerIndex)
    : ImageView3D (device, image, image.GetFormat (), layerIndex)
{
}


ImageViewCube::ImageViewCube (VkDevice device, VkImage image, VkFormat format, uint32_t layerIndex)
    : ImageViewBase (device, image, format, VK_IMAGE_VIEW_TYPE_CUBE, layerIndex)
{
}


ImageViewCube::ImageViewCube (VkDevice device, const Image& image, uint32_t layerIndex)
    : ImageViewCube (device, image, image.GetFormat (), layerIndex)
{
}

} // namespace GVK
