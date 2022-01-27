#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"

#pragma warning (push, 0)
#include "vk_mem_alloc.h"
#pragma warning(pop)

#include <vulkan/vulkan.h>
#include <optional>

namespace RG {

class ImageBuilder;
class CommandBuffer;

class RENDERGRAPH_DLL_EXPORT Image : public VulkanObject {
public:
    static const VkImageLayout INITIAL_LAYOUT;

    static constexpr VkFormat R    = VK_FORMAT_R8_UINT;
    static constexpr VkFormat RG   = VK_FORMAT_R8G8_UINT;
    static constexpr VkFormat RGB  = VK_FORMAT_R8G8B8_UINT;
    static constexpr VkFormat RGBA = VK_FORMAT_R8G8B8A8_UINT;

    enum class MemoryLocation {
        GPU,
        CPU
    };

protected:
    RG::MovablePtr<VkImage> handle;

private:
    VkDevice      device;
    VmaAllocator  allocator;
    VmaAllocation allocationHandle;

    VkFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t arrayLayers;

protected:
    // for InheritedImage
    Image (VkImage handle, VkDevice device, uint32_t width, uint32_t height, uint32_t depth, VkFormat format, uint32_t arrayLayers);

public:
    Image (VmaAllocator      allocator,
           VkImageType       imageType,
           uint32_t          width,
           uint32_t          height,
           uint32_t          depth,
           VkFormat          format,
           VkImageTiling     tiling,
           VkImageUsageFlags usage,
           uint32_t          arrayLayers,
           MemoryLocation    loc);

    Image (ImageBuilder&);

    Image (Image&&) = default;
    Image& operator= (Image&&) = default;

    virtual ~Image () override;

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_IMAGE; }

    VkFormat GetFormat () const { return format; }
    uint32_t GetArrayLayers () const { return arrayLayers; }
    uint32_t GetWidth () const { return width; }
    uint32_t GetHeight () const { return height; }
    uint32_t GetDepth () const { return depth; }

    operator VkImage () const { return handle; }
    operator VmaAllocation () const { return allocationHandle; }

    VkBufferImageCopy GetFullBufferImageCopy () const;
    VkBufferImageCopy GetFullBufferImageCopyLayer (uint32_t layerIndex) const;

    VkImageMemoryBarrier GetBarrier (VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) const;
    VkImageMemoryBarrier GetBarrier (VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t layerIndex) const;
    VkImageMemoryBarrier GetBarrier (VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t baseArrayLayer, uint32_t layerCount) const;

    void CmdCopyToBuffer (CommandBuffer& commandBuffer, VkBuffer buffer) const;
    void CmdCopyLayerToBuffer (CommandBuffer& commandBuffer, uint32_t layerIndex, VkBuffer buffer) const;

    void CmdCopyBufferToImage (CommandBuffer& commandBuffer, VkBuffer buffer) const;

    void CmdCopyBufferPartToImage (CommandBuffer& commandBuffer, VkBuffer buffer, VkBufferImageCopy region) const;
};


// used for handling swapchain images as Image
class RENDERGRAPH_DLL_EXPORT InheritedImage : public Image {
public:
    InheritedImage (VkImage handle, uint32_t width, uint32_t height, uint32_t depth, VkFormat format, uint32_t arrayLayers)
        : Image (handle, VK_NULL_HANDLE, width, height, depth, format, arrayLayers)
    {
    }

    virtual ~InheritedImage () override
    {
        handle = nullptr;
    }
};


class RENDERGRAPH_DLL_EXPORT ImageBuilder {
public:
    const VmaAllocator allocator;

    std::optional<VkImageType>           imageType;
    std::optional<uint32_t>              width;
    std::optional<uint32_t>              height;
    std::optional<uint32_t>              depth;
    std::optional<VkFormat>              format;
    std::optional<VkImageTiling>         tiling;
    VkImageUsageFlags                    usage;
    std::optional<uint32_t>              arrayLayers;
    std::optional<Image::MemoryLocation> loc;

    ImageBuilder (VmaAllocator allocator)
        : allocator (allocator)
        , usage (0)
    {
    }

    // clang-format off
    
#pragma warning(disable:26815)

    ImageBuilder& SetMemoryLocation (Image::MemoryLocation value) { loc = value; return *this; }
    ImageBuilder& SetCPU () { loc = Image::MemoryLocation::CPU; return *this; }
    ImageBuilder& SetGPU () { loc = Image::MemoryLocation::GPU; return *this; }

    ImageBuilder& SetImageType (VkImageType value) { imageType = value; return *this; }
    ImageBuilder& Set1D () { imageType = VK_IMAGE_TYPE_1D; return *this; }
    ImageBuilder& Set2D () { imageType = VK_IMAGE_TYPE_2D; return *this; }
    ImageBuilder& Set3D () { imageType = VK_IMAGE_TYPE_3D; return *this; }

    ImageBuilder& SetWidth (uint32_t value) { width = value; return *this; }
    ImageBuilder& SetHeight (uint32_t value) { height = value; return *this; }
    ImageBuilder& SetDepth (uint32_t value) { depth = value; return *this; }

    ImageBuilder& SetFormat (VkFormat value) { format = value; return *this; }

    ImageBuilder& SetTiling (VkImageTiling value) { tiling = value; return *this; }
    ImageBuilder& SetTilingLinear () { tiling = VK_IMAGE_TILING_LINEAR; return *this; }
    ImageBuilder& SetTilingOptimal () { tiling = VK_IMAGE_TILING_OPTIMAL; return *this; }

    ImageBuilder& AddUsageFlag (VkImageUsageFlags flag) { usage |= flag; return *this; }

    ImageBuilder& SetLayers (uint32_t value) { arrayLayers = value; return *this; }

#pragma warning(default:26815)

    // clang-format on

    Image Build () const;
};


class RENDERGRAPH_DLL_EXPORT Image1D : public Image {
public:
    Image1D (VmaAllocator allocator, MemoryLocation loc, uint32_t width, VkFormat format, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkImageUsageFlags usage = 0, uint32_t arrayLayers = 1)
        : Image (allocator, VK_IMAGE_TYPE_1D, width, 1, 1, format, tiling, usage, arrayLayers, loc)
    {
    }
};


class RENDERGRAPH_DLL_EXPORT Image2D : public Image {
public:
    Image2D (VmaAllocator allocator, MemoryLocation loc, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkImageUsageFlags usage = 0, uint32_t arrayLayers = 1)
        : Image (allocator, VK_IMAGE_TYPE_2D, width, height, 1, format, tiling, usage, arrayLayers, loc)
    {
    }
};


class RENDERGRAPH_DLL_EXPORT Image3D : public Image {
public:
    Image3D (VmaAllocator allocator, MemoryLocation loc, uint32_t width, uint32_t height, uint32_t depth, VkFormat format, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkImageUsageFlags usage = 0)
        : Image (allocator, VK_IMAGE_TYPE_3D, width, height, depth, format, tiling, usage, 1, loc)
    {
    }
};

} // namespace RG

#endif
