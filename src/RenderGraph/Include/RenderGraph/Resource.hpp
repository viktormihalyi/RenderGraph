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

namespace RG {
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

    virtual void OnPreRead (uint32_t /* resourceIndex */, RG::CommandBuffer&) {};
    virtual void OnPreWrite (uint32_t /* resourceIndex */, RG::CommandBuffer&) {};
    virtual void OnPostWrite (uint32_t /* resourceIndex */, RG::CommandBuffer&) {};
    virtual void OnGraphExecutionStarted (uint32_t /* resourceIndex */, RG::CommandBuffer&) {};
    virtual void OnGraphExecutionEnded (uint32_t /* resourceIndex */, RG::CommandBuffer&) {};
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
    virtual std::vector<RG::Image*> GetImages () const                       = 0;
    virtual std::vector<RG::Image*> GetImages (uint32_t resourceIndex) const = 0;


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

        std::unique_ptr<RG::Image>                    image;
        std::vector<std::unique_ptr<RG::ImageView2D>> imageViews;

        SingleImageResource (const RG::DeviceExtra& device, uint32_t width, uint32_t height, uint32_t arrayLayers, VkFormat format = FormatRGBA, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);
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
    std::unique_ptr<RG::Sampler>                     sampler;

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

    virtual std::vector<RG::Image*> GetImages () const override;

    virtual std::vector<RG::Image*> GetImages (uint32_t resourceIndex) const override;

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

    virtual void OnPreRead (uint32_t resourceIndex, RG::CommandBuffer& commandBuffer) override;

    virtual void OnPreWrite (uint32_t resourceIndex, RG::CommandBuffer& commandBuffer) override;

    virtual void OnPostWrite (uint32_t resourceIndex, RG::CommandBuffer& commandBuffer) override;
};


class RENDERGRAPH_DLL_EXPORT GPUBufferResource : public DescriptorBindableBufferResource {
public:
    size_t size;
    
    std::vector<std::unique_ptr<RG::BufferTransferable>> buffers;

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
    std::unique_ptr<RG::ImageTransferable> image;
    std::unique_ptr<RG::ImageViewBase>     imageView;
    std::unique_ptr<RG::Sampler>           sampler;

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

    virtual std::vector<RG::Image*> GetImages () const override;

    virtual std::vector<RG::Image*> GetImages (uint32_t) const override;

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
    std::vector<std::unique_ptr<RG::ImageView2D>>    imageViews;
    RG::SwapchainProvider&                           swapchainProv;
    std::vector<std::unique_ptr<RG::InheritedImage>> inheritedImages;

public:
    SwapchainImageResource (RG::SwapchainProvider& swapchainProv);

    virtual ~SwapchainImageResource ();

    // overriding Resource
    virtual void Compile (const GraphSettings& graphSettings) override;

    // overriding ImageResource
    virtual VkImageLayout GetInitialLayout () const override;
    virtual VkImageLayout GetFinalLayout () const override;
    virtual VkFormat      GetFormat () const override;
    virtual uint32_t      GetLayerCount () const override;

    virtual std::vector<RG::Image*> GetImages () const override;

    virtual std::vector<RG::Image*> GetImages (uint32_t resourceIndex) const override;

    // overriding DescriptorBindableImage
    virtual VkImageView GetImageViewForFrame (uint32_t resourceIndex, uint32_t) override;
    virtual VkSampler   GetSampler () override;
};


class RENDERGRAPH_DLL_EXPORT CPUBufferResource : public DescriptorBindableBufferResource {
public:
    const uint32_t                                   size;
    std::vector<std::unique_ptr<RG::Buffer>>        buffers;
    std::vector<std::unique_ptr<RG::MemoryMapping>> mappings;

public:
    CPUBufferResource (uint32_t size);

public:
    virtual ~CPUBufferResource ();

    // overriding Resource
    virtual void Compile (const GraphSettings& graphSettings) override;

    // overriding DescriptorBindableBuffer
    virtual VkBuffer GetBufferForFrame (uint32_t resourceIndex) override;
    virtual size_t GetBufferSize () override;

    RG::MemoryMapping& GetMapping (uint32_t resourceIndex);
};


} // namespace RG

#endif