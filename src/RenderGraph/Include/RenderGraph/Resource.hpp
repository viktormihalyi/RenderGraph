#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Utils/Event.hpp"
#include "RenderGraph/Utils/Timer.hpp"

#include "RenderGraph/VulkanWrapper/Event.hpp"
#include "RenderGraph/VulkanWrapper/GraphicsPipeline.hpp"
#include "RenderGraph/VulkanWrapper/Utils/BufferTransferable.hpp"

#include "RenderGraph/GraphSettings.hpp"
#include "RenderGraph/Node.hpp"
#include "RenderGraph/DescriptorBindable.hpp"

#include <vulkan/vulkan.h>

#include <optional>


namespace VW {
class Event;
}

namespace GVK {
class SwapchainProvider;
class Image;
class ImageView2D;
class MemoryMapping;
class Buffer;
class ImageViewBase;
class Sampler;
class ImageTransferable;
class BufferTransferable;
class InheritedImage;
}

namespace RG {

class GraphSettings;
class Operation;


class RENDERGRAPH_DLL_EXPORT Resource : public Node {
public:
    virtual ~Resource () = default;

    virtual void Compile (const GraphSettings&) = 0;

    virtual void OnPreRead (uint32_t /* resourceIndex */, GVK::CommandBuffer&) {};
    virtual void OnPreWrite (uint32_t /* resourceIndex */, GVK::CommandBuffer&) {};
    virtual void OnPostWrite (uint32_t /* resourceIndex */, GVK::CommandBuffer&) {};
    virtual void OnGraphExecutionStarted (uint32_t /* resourceIndex */, GVK::CommandBuffer&) {};
    virtual void OnGraphExecutionEnded (uint32_t /* resourceIndex */, GVK::CommandBuffer&) {};
};


class RENDERGRAPH_DLL_EXPORT DescriptorBindableBufferResource : public Resource, public DescriptorBindableBuffer {
public:
    virtual ~DescriptorBindableBufferResource () = default;
};


class RENDERGRAPH_DLL_EXPORT ImageResource : public Resource {
public:
    virtual ~ImageResource ();

public:
    virtual VkImageLayout            GetInitialLayout () const                = 0;
    virtual VkImageLayout            GetFinalLayout () const                  = 0;
    virtual VkFormat                 GetFormat () const                       = 0;
    virtual uint32_t                 GetLayerCount () const                   = 0;
    virtual std::vector<GVK::Image*> GetImages () const                       = 0;
    virtual std::vector<GVK::Image*> GetImages (uint32_t resourceIndex) const = 0;


    std::function<VkFormat ()> GetFormatProvider () const;
};


class RENDERGRAPH_DLL_EXPORT OneTimeCompileResource : public ImageResource {
private:
    bool compiled;

protected:
    OneTimeCompileResource ();

public:
    virtual ~OneTimeCompileResource ();

    void Compile (const GraphSettings& settings) override;

    virtual void CompileOnce (const GraphSettings&) = 0;
};


class RENDERGRAPH_DLL_EXPORT WritableImageResource : public ImageResource, public DescriptorBindableImage {
protected:
    class RENDERGRAPH_DLL_EXPORT SingleImageResource final {
    public:
        static const VkFormat FormatRGBA;
        static const VkFormat FormatRGB;

        std::unique_ptr<GVK::Image>                    image;
        std::vector<std::unique_ptr<GVK::ImageView2D>> imageViews;

        SingleImageResource (const GVK::DeviceExtra& device, uint32_t width, uint32_t height, uint32_t arrayLayers, VkFormat format = FormatRGBA, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);
    };

public:
    const VkFilter filter;
    const VkFormat format;
    const uint32_t width;
    const uint32_t height;
    const uint32_t arrayLayers;

    VkImageLayout initialLayout; // TODO temporary
    VkImageLayout finalLayout;   // TODO temporary

    std::vector<std::unique_ptr<SingleImageResource>> images;
    std::unique_ptr<GVK::Sampler>                     sampler;

public:
    WritableImageResource (VkFilter filter, uint32_t width, uint32_t height, uint32_t arrayLayers, VkFormat format = SingleImageResource::FormatRGBA);

    WritableImageResource (uint32_t width, uint32_t height);

    virtual ~WritableImageResource ();

    virtual void Compile (const GraphSettings& graphSettings) override;

    // overriding ImageResource
    virtual VkImageLayout GetInitialLayout () const override;

    virtual VkImageLayout GetFinalLayout () const override;

    virtual VkFormat GetFormat () const override;
    virtual uint32_t GetLayerCount () const override;

    virtual std::vector<GVK::Image*> GetImages () const override;

    virtual std::vector<GVK::Image*> GetImages (uint32_t resourceIndex) const override;

    // overriding DescriptorBindableImage
    virtual VkImageView GetImageViewForFrame (uint32_t resourceIndex, uint32_t layerIndex) override;
    virtual VkSampler   GetSampler () override;
};


class RENDERGRAPH_DLL_EXPORT SingleWritableImageResource : public WritableImageResource {
private:
    std::unique_ptr<VW::Event> readWriteSync;

public:
    using WritableImageResource::WritableImageResource;

    virtual void Compile (const GraphSettings& graphSettings) override;

    virtual void OnPreRead (uint32_t resourceIndex, GVK::CommandBuffer& commandBuffer) override;

    virtual void OnPreWrite (uint32_t resourceIndex, GVK::CommandBuffer& commandBuffer) override;

    virtual void OnPostWrite (uint32_t resourceIndex, GVK::CommandBuffer& commandBuffer) override;
};


class RENDERGRAPH_DLL_EXPORT GPUBufferResource : public DescriptorBindableBufferResource {
public:
    size_t size;
    
    std::vector<std::unique_ptr<GVK::BufferTransferable>> buffers;

public:
    GPUBufferResource (size_t size);

    virtual ~GPUBufferResource () override;

    virtual void Compile (const GraphSettings& settings) override;

    // overriding DescriptorBindableImage
    virtual VkBuffer GetBufferForFrame (uint32_t) override;

    virtual size_t GetBufferSize () override;

    void TransferFromCPUToGPU (uint32_t resourceIndex, const void* data, size_t size) const;

    void TransferFromGPUToCPU (uint32_t resourceIndex) const;

    void TransferFromGPUToCPU (uint32_t resourceIndex, VkDeviceSize size, VkDeviceSize offset) const;
};


class RENDERGRAPH_DLL_EXPORT ReadOnlyImageResource : public OneTimeCompileResource, public DescriptorBindableImage {
public:
    std::unique_ptr<GVK::ImageTransferable> image;
    std::unique_ptr<GVK::ImageViewBase>     imageView;
    std::unique_ptr<GVK::Sampler>           sampler;

    const VkFormat format;
    const VkFilter filter;
    const uint32_t width;
    const uint32_t height;
    const uint32_t depth;
    const uint32_t layerCount;

public:
    ReadOnlyImageResource (VkFormat format, VkFilter filter, uint32_t width, uint32_t height = 1, uint32_t depth = 1, uint32_t layerCount = 1);

    ReadOnlyImageResource (VkFormat format, uint32_t width, uint32_t height = 1, uint32_t depth = 1, uint32_t layerCount = 1);

    virtual ~ReadOnlyImageResource () override;

    // overriding OneTimeCompileResource
    virtual void CompileOnce (const GraphSettings& settings) override;

    // overriding ImageResource
    virtual VkImageLayout GetInitialLayout () const override;
    virtual VkImageLayout GetFinalLayout () const override;
    virtual VkFormat      GetFormat () const override;
    virtual uint32_t      GetLayerCount () const override;

    virtual std::vector<GVK::Image*> GetImages () const override;

    virtual std::vector<GVK::Image*> GetImages (uint32_t) const override;

    // overriding DescriptorBindableImage
    virtual VkImageView GetImageViewForFrame (uint32_t, uint32_t) override;
    virtual VkSampler   GetSampler () override;

    template<typename T>
    void CopyTransitionTransfer (const std::vector<T>& pixelData)
    {
        CopyLayer (pixelData, 0);
    }

    template<typename T>
    void CopyLayer (const std::vector<T>& pixelData, uint32_t layerIndex)
    {
        image->CopyLayer (VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pixelData.data (), pixelData.size () * sizeof (T), layerIndex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
};


class RENDERGRAPH_DLL_EXPORT SwapchainImageResource : public ImageResource, public DescriptorBindableImage {
public:
    std::vector<std::unique_ptr<GVK::ImageView2D>>    imageViews;
    GVK::SwapchainProvider&                           swapchainProv;
    std::vector<std::unique_ptr<GVK::InheritedImage>> inheritedImages;

public:
    SwapchainImageResource (GVK::SwapchainProvider& swapchainProv);

    virtual ~SwapchainImageResource ();

    // overriding Resource
    virtual void Compile (const GraphSettings& graphSettings) override;

    // overriding ImageResource
    virtual VkImageLayout GetInitialLayout () const override;
    virtual VkImageLayout GetFinalLayout () const override;
    virtual VkFormat      GetFormat () const override;
    virtual uint32_t      GetLayerCount () const override;

    virtual std::vector<GVK::Image*> GetImages () const override;

    virtual std::vector<GVK::Image*> GetImages (uint32_t resourceIndex) const override;

    // overriding DescriptorBindableImage
    virtual VkImageView GetImageViewForFrame (uint32_t resourceIndex, uint32_t) override;
    virtual VkSampler   GetSampler () override;
};


class RENDERGRAPH_DLL_EXPORT CPUBufferResource : public DescriptorBindableBufferResource {
public:
    const uint32_t                                   size;
    std::vector<std::unique_ptr<GVK::Buffer>>        buffers;
    std::vector<std::unique_ptr<GVK::MemoryMapping>> mappings;

public:
    CPUBufferResource (uint32_t size);

public:
    virtual ~CPUBufferResource ();

    // overriding Resource
    virtual void Compile (const GraphSettings& graphSettings) override;

    // overriding DescriptorBindableBuffer
    virtual VkBuffer GetBufferForFrame (uint32_t resourceIndex) override;
    virtual size_t GetBufferSize () override;

    GVK::MemoryMapping& GetMapping (uint32_t resourceIndex);
};


} // namespace RG

#endif