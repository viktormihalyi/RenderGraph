#include "Resource.hpp"

#include "VulkanWrapper/Utils/SingleTimeCommand.hpp"
#include "VulkanWrapper/Image.hpp"
#include "VulkanWrapper/ImageView.hpp"
#include "VulkanWrapper/Swapchain.hpp"
#include "VulkanWrapper/Commands.hpp"
#include "VulkanWrapper/Event.hpp"
#include "VulkanWrapper/Sampler.hpp"
#include "VulkanWrapper/Utils/BufferTransferable.hpp"
#include "VulkanWrapper/Utils/VulkanUtils.hpp"

namespace RG {


// sampled formats should always be _SRGB?
const VkFormat WritableImageResource::SingleImageResource::FormatRGBA = VK_FORMAT_R8G8B8A8_SRGB;
const VkFormat WritableImageResource::SingleImageResource::FormatRGB  = VK_FORMAT_R8G8B8_SRGB;


ImageResource::~ImageResource () = default;


std::function<VkFormat ()> ImageResource::GetFormatProvider () const
{
    return [&] () { return GetFormat (); };
}


OneTimeCompileResource::OneTimeCompileResource ()
    : compiled (false)
{
}


OneTimeCompileResource::~OneTimeCompileResource () = default;


void OneTimeCompileResource::Compile (const GraphSettings& settings)
{
    if (!compiled) {
        compiled = true;
        CompileOnce (settings);
    }
}


WritableImageResource::SingleImageResource::SingleImageResource (const RG::DeviceExtra& device, uint32_t width, uint32_t height, uint32_t arrayLayers, VkFormat format, VkImageTiling tiling)
    : image (std::make_unique<RG::Image2D> (device.GetAllocator (), RG::Image::MemoryLocation::GPU,
                                        width, height,
                                        format, tiling,
                                             VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                        arrayLayers))
{
    for (uint32_t layerIndex = 0; layerIndex < arrayLayers; ++layerIndex) {
        imageViews.push_back (std::make_unique<RG::ImageView2D> (device, *image, layerIndex));
    }

    {
        RG::SingleTimeCommand s (device);
        s.Record<RG::CommandTranstionImage> (*image, RG::Image::INITIAL_LAYOUT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }
}


WritableImageResource::WritableImageResource (VkFilter filter, uint32_t width, uint32_t height, uint32_t arrayLayers, VkFormat format)
    : filter (filter)
    , format (format)
    , width (width)
    , height (height)
    , arrayLayers (arrayLayers)
    , initialLayout (VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    , finalLayout (VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
{
}


WritableImageResource::WritableImageResource (uint32_t width, uint32_t height)
    : WritableImageResource (VK_FILTER_LINEAR, width, height, 1)
{
}


WritableImageResource::~WritableImageResource () = default;


void WritableImageResource::Compile (const GraphSettings& graphSettings)
{
    sampler = std::make_unique<RG::Sampler> (graphSettings.GetDevice (), filter);

    images.clear ();
    for (uint32_t resourceIndex = 0; resourceIndex < graphSettings.framesInFlight; ++resourceIndex) {
        images.push_back (std::make_unique<SingleImageResource> (graphSettings.GetDevice (), width, height, arrayLayers, format));
    }
}


VkImageLayout WritableImageResource::GetInitialLayout () const
{
    return initialLayout;
}


VkImageLayout WritableImageResource::GetFinalLayout () const
{
    return finalLayout;
}


VkFormat WritableImageResource::GetFormat () const { return format; }


uint32_t WritableImageResource::GetLayerCount () const { return arrayLayers; }


std::vector<RG::Image*> WritableImageResource::GetImages () const
{
    std::vector<RG::Image*> result;

    for (auto& img : images) {
        result.push_back (img->image.get ());
    }

    return result;
}


std::vector<RG::Image*> WritableImageResource::GetImages (uint32_t resourceIndex) const
{
    return { images[resourceIndex]->image.get () };
}


VkImageView WritableImageResource::GetImageViewForFrame (uint32_t resourceIndex, uint32_t layerIndex)
{
    return *images[resourceIndex]->imageViews[layerIndex];
}


VkSampler   WritableImageResource::GetSampler () { return *sampler; }


void SingleWritableImageResource::Compile (const GraphSettings& graphSettings)
{
    readWriteSync = std::make_unique<VW::Event> (graphSettings.GetDevice ());

    sampler = std::make_unique<RG::Sampler> (graphSettings.GetDevice (), filter);
    images.clear ();
    images.push_back (std::make_unique<SingleImageResource> (graphSettings.GetDevice (), width, height, arrayLayers, GetFormat ()));
}


void SingleWritableImageResource::OnPreRead (uint32_t /* resourceIndex */, RG::CommandBuffer& commandBuffer)
{
    commandBuffer.Record<RG::CommandGeneric> ([&] (VkCommandBuffer commandBuffer) {
        VkEvent handle = *readWriteSync;
        vkCmdWaitEvents (commandBuffer,
                            1,
                            &handle,
                            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                            0, nullptr,
                            0, nullptr,
                            0, nullptr);
    });
}


void SingleWritableImageResource::OnPreWrite (uint32_t /* resourceIndex */, RG::CommandBuffer& commandBuffer)
{
    commandBuffer.Record<RG::CommandGeneric> ([&] (VkCommandBuffer commandBuffer) {
        VkEvent handle = *readWriteSync;
        vkCmdResetEvent (commandBuffer, handle, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
    });
}


void SingleWritableImageResource::OnPostWrite (uint32_t /* resourceIndex */, RG::CommandBuffer& commandBuffer)
{
    commandBuffer.Record<RG::CommandGeneric> ([&] (VkCommandBuffer commandBuffer) {
        VkEvent handle = *readWriteSync;
        vkCmdSetEvent (commandBuffer, handle, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
    });
}


GPUBufferResource::GPUBufferResource (const size_t size)
    : size (size)
{
}


GPUBufferResource::~GPUBufferResource () = default;


void GPUBufferResource::Compile (const GraphSettings& settings)
{
    buffers.reserve (settings.framesInFlight);
    for (uint32_t i = 0; i < settings.framesInFlight; ++i) {
        buffers.push_back (std::make_unique<RG::BufferTransferable> (settings.GetDevice (), size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT));
    }
}


VkBuffer GPUBufferResource::GetBufferForFrame (uint32_t resourceIndex)
{
    return buffers[resourceIndex]->bufferGPU;
}


size_t GPUBufferResource::GetBufferSize ()
{
    return size;
}


void GPUBufferResource::TransferFromCPUToGPU (uint32_t resourceIndex, const void* data, size_t dataSize) const
{
    buffers[resourceIndex]->TransferFromCPUToGPU (data, dataSize);
}


void GPUBufferResource::TransferFromGPUToCPU (uint32_t resourceIndex) const
{
    buffers[resourceIndex]->TransferFromGPUToCPU ();
}


ReadOnlyImageResource::ReadOnlyImageResource (VkFormat format, VkFilter filter, uint32_t width, uint32_t height, uint32_t depth, uint32_t layerCount)
    : format (format)
    , filter (filter)
    , width (width)
    , height (height)
    , depth (depth)
    , layerCount (layerCount)
{
    RG_ASSERT (width > 0);
    RG_ASSERT (height > 0);
    RG_ASSERT (depth > 0);
    RG_ASSERT (layerCount > 0);
}


ReadOnlyImageResource::ReadOnlyImageResource (VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t layerCount)
    : ReadOnlyImageResource (format, VK_FILTER_LINEAR, width, height, depth, layerCount)
{
}


ReadOnlyImageResource::~ReadOnlyImageResource () = default;


void ReadOnlyImageResource::CompileOnce (const GraphSettings& settings)
{
    sampler = std::make_unique<RG::Sampler> (settings.GetDevice (), filter);

    if (height == 1 && depth == 1) {
        image     = std::make_unique<RG::Image1DTransferable> (settings.GetDevice (), format, width, VK_IMAGE_USAGE_SAMPLED_BIT);
        imageView = std::make_unique<RG::ImageView1D> (settings.GetDevice (), *image->imageGPU);
    } else if (depth == 1) {
        if (layerCount == 1) {
            image     = std::make_unique<RG::Image2DTransferable> (settings.GetDevice (), format, width, height, VK_IMAGE_USAGE_SAMPLED_BIT);
            imageView = std::make_unique<RG::ImageView2D> (settings.GetDevice (), *image->imageGPU, 0);
        } else {
            image     = std::make_unique<RG::Image2DTransferable> (settings.GetDevice (), format, width, height, VK_IMAGE_USAGE_SAMPLED_BIT, layerCount);
            imageView = std::make_unique<RG::ImageView2DArray> (settings.GetDevice (), *image->imageGPU, 0, 1);
        }
    } else {
        image     = std::make_unique<RG::Image3DTransferable> (settings.GetDevice (), format, width, height, depth, VK_IMAGE_USAGE_SAMPLED_BIT);
        imageView = std::make_unique<RG::ImageView3D> (settings.GetDevice (), *image->imageGPU);
    }

    RG::SingleTimeCommand s (settings.GetDevice ());
    s.Record<RG::CommandTranstionImage> (*image->imageGPU, RG::Image::INITIAL_LAYOUT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}


VkImageLayout ReadOnlyImageResource::GetInitialLayout () const { return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; }


VkImageLayout ReadOnlyImageResource::GetFinalLayout () const { return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; }


VkFormat      ReadOnlyImageResource::GetFormat () const { return format; }


uint32_t      ReadOnlyImageResource::GetLayerCount () const { return 1; }


std::vector<RG::Image*> ReadOnlyImageResource::GetImages () const
{
    return { image->imageGPU.get () };
}


std::vector<RG::Image*> ReadOnlyImageResource::GetImages (uint32_t) const
{
    return GetImages ();
}


VkImageView ReadOnlyImageResource::GetImageViewForFrame (uint32_t, uint32_t) { return *imageView; }


VkSampler   ReadOnlyImageResource::GetSampler () { return *sampler; }


SwapchainImageResource::SwapchainImageResource (RG::SwapchainProvider& swapchainProv)
    : swapchainProv (swapchainProv)
{
}


SwapchainImageResource::~SwapchainImageResource () = default;


void SwapchainImageResource::Compile (const GraphSettings& graphSettings)
{
    const std::vector<VkImage> swapChainImages = swapchainProv.GetSwapchain ().GetImages ();

    imageViews.clear ();
    inheritedImages.clear ();

    for (size_t i = 0; i < swapChainImages.size (); ++i) {
        imageViews.push_back (std::make_unique<RG::ImageView2D> (graphSettings.GetDevice (), swapChainImages[i], swapchainProv.GetSwapchain ().GetImageFormat ()));
        inheritedImages.push_back (std::make_unique<RG::InheritedImage> (
            swapChainImages[i],
            swapchainProv.GetSwapchain ().GetWidth (),
            swapchainProv.GetSwapchain ().GetHeight (),
            1,
            swapchainProv.GetSwapchain ().GetImageFormat (),
            1));
    }
}


VkImageLayout SwapchainImageResource::GetInitialLayout () const { return VK_IMAGE_LAYOUT_UNDEFINED; }


VkImageLayout SwapchainImageResource::GetFinalLayout () const { return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; }


VkFormat      SwapchainImageResource::GetFormat () const { return swapchainProv.GetSwapchain ().GetImageFormat (); }


uint32_t      SwapchainImageResource::GetLayerCount () const { return 1; }


std::vector<RG::Image*> SwapchainImageResource::GetImages () const
{
    std::vector<RG::Image*> result;

    for (auto& img : inheritedImages) {
        result.push_back (img.get ());
    }

    return result;
}


std::vector<RG::Image*> SwapchainImageResource::GetImages (uint32_t resourceIndex) const
{
    return { inheritedImages[resourceIndex].get () };
}


VkImageView SwapchainImageResource::GetImageViewForFrame (uint32_t resourceIndex, uint32_t) { return *imageViews[resourceIndex]; }


VkSampler   SwapchainImageResource::GetSampler () { return VK_NULL_HANDLE; }


CPUBufferResource::CPUBufferResource (uint32_t size)
    : size (size)
{
}


CPUBufferResource::~CPUBufferResource () = default;


void CPUBufferResource::Compile (const GraphSettings& graphSettings)
{
    mappings.clear ();
    buffers.clear ();

    for (uint32_t i = 0; i < graphSettings.framesInFlight; ++i) {
        buffers.push_back (std::make_unique<RG::UniformBuffer> (graphSettings.GetDevice ().GetAllocator (), size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, RG::Buffer::MemoryLocation::CPU));
        mappings.push_back (std::make_unique<RG::MemoryMapping> (graphSettings.GetDevice ().GetAllocator (), *buffers[buffers.size () - 1]));
    }
}


VkBuffer CPUBufferResource::GetBufferForFrame (uint32_t resourceIndex) { return *buffers[resourceIndex]; }


size_t CPUBufferResource::GetBufferSize () { return size; }


RG::MemoryMapping& CPUBufferResource::GetMapping (uint32_t resourceIndex) { return *mappings[resourceIndex]; }


} // namespace RG
