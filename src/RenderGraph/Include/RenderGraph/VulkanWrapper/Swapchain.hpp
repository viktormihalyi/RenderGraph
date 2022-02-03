#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Utils/Assert.hpp"
#include "VulkanObject.hpp"
#include "Framebuffer.hpp"
#include "ImageView.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "PhysicalDevice.hpp"

#include <vulkan/vulkan.h>

namespace RG {

class RENDERGRAPH_DLL_EXPORT SwapchainSettingsProvider {
public:
    virtual ~SwapchainSettingsProvider () = default;
    
    virtual VkSurfaceFormatKHR SelectSurfaceFormat (const std::vector<VkSurfaceFormatKHR>&) = 0;
    virtual VkPresentModeKHR   SelectPresentMode (const std::vector<VkPresentModeKHR>&)     = 0;
    virtual VkExtent2D         SelectExtent (const VkSurfaceCapabilitiesKHR&)               = 0;
    virtual uint32_t           SelectImageCount (const VkSurfaceCapabilitiesKHR&)           = 0;
};


class RENDERGRAPH_DLL_EXPORT DefaultSwapchainSettings : public SwapchainSettingsProvider {
public:
    virtual ~DefaultSwapchainSettings () override = default;

    virtual VkSurfaceFormatKHR SelectSurfaceFormat (const std::vector<VkSurfaceFormatKHR>& formats) override;
    virtual VkPresentModeKHR   SelectPresentMode (const std::vector<VkPresentModeKHR>& modes) override;
    virtual VkExtent2D         SelectExtent (const VkSurfaceCapabilitiesKHR& capabilities) override;
    virtual uint32_t           SelectImageCount (const VkSurfaceCapabilitiesKHR& capabilities) override;
};

class RENDERGRAPH_DLL_EXPORT DefaultSwapchainSettingsSingleImage : public DefaultSwapchainSettings {
public:
    virtual ~DefaultSwapchainSettingsSingleImage () override = default;

    virtual uint32_t SelectImageCount (const VkSurfaceCapabilitiesKHR& capabilities) override;
};

class RENDERGRAPH_DLL_EXPORT DefaultSwapchainSettingsMaxImages : public DefaultSwapchainSettings {
public:
    virtual ~DefaultSwapchainSettingsMaxImages () override = default;

    virtual uint32_t SelectImageCount (const VkSurfaceCapabilitiesKHR& capabilities) override;
};

class RENDERGRAPH_DLL_EXPORT Swapchain {
public:
    virtual ~Swapchain ();

    virtual VkFormat                                         GetImageFormat () const  = 0;
    virtual uint32_t                                         GetImageCount () const   = 0;
    virtual uint32_t                                         GetWidth () const        = 0;
    virtual uint32_t                                         GetHeight () const       = 0;
    virtual std::vector<VkImage>                             GetImages () const       = 0;
    virtual std::vector<std::unique_ptr<InheritedImage>>     GetImageObjects () const = 0;
    virtual const std::vector<std::unique_ptr<ImageView2D>>& GetImageViews () const   = 0;

    virtual uint32_t GetNextImageIndex (VkSemaphore signalSemaphore, VkFence fence = VK_NULL_HANDLE) const              = 0;
    virtual void     Present (VkQueue queue, uint32_t imageIndex, const std::vector<VkSemaphore>& waitSemaphores) const = 0;
    virtual bool     SupportsPresenting () const                                                                        = 0;
    virtual void     Recreate ()                                                                                        = 0;
};

class RENDERGRAPH_DLL_EXPORT OutOfDateSwapchain : public std::runtime_error {
public:
    const Swapchain& swapchain;
    OutOfDateSwapchain (const Swapchain& swapchain)
        : std::runtime_error { "out of date swapchain detected" }
        , swapchain { swapchain }
    {
    }
};

class RENDERGRAPH_DLL_EXPORT RealSwapchain : public Swapchain, public VulkanObject {
public:
    static const VkImageUsageFlags ImageUsage;

private:
    struct CreateSettings {
        VkPhysicalDevice                           physicalDevice;
        VkDevice                                   device;
        VkSurfaceKHR                               surface;
        std::unique_ptr<SwapchainSettingsProvider> settings;
    };

    struct CreateResult {
        RG::MovablePtr<VkSwapchainKHR> handle;

        uint32_t           imageCount;
        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR   presentMode;
        VkExtent2D         extent;

        std::vector<VkImage>                      images;
        std::vector<std::unique_ptr<ImageView2D>> imageViews;
        std::vector<std::unique_ptr<Framebuffer>> framebuffers;

        CreateResult ()
            : handle (VK_NULL_HANDLE)
            , imageCount (0)
            , extent (VkExtent2D { 0, 0 } )
            , presentMode (VK_PRESENT_MODE_IMMEDIATE_KHR)
            , surfaceFormat (VkSurfaceFormatKHR {})
        {
        }

        void Clear ()
        {
            handle     = VK_NULL_HANDLE;
            imageCount = 0;
            images.clear ();
            imageViews.clear ();
            framebuffers.clear ();
        }
    };

    CreateSettings createSettings;
    CreateResult   createResult;

    static CreateResult CreateForResult (const CreateSettings& createSettings);

public:
    RealSwapchain (VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, std::unique_ptr<SwapchainSettingsProvider>&& settings);

    RealSwapchain (RealSwapchain&&) = default;
    RealSwapchain& operator= (RealSwapchain&&) = default;

    virtual void Recreate () override;


    virtual ~RealSwapchain ();

    virtual void* GetHandleForName () const override { return createResult.handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_SWAPCHAIN_KHR; }

    operator VkSwapchainKHR () const { return createResult.handle; }

    virtual VkFormat                                         GetImageFormat () const override { return createResult.surfaceFormat.format; }
    virtual uint32_t                                         GetImageCount () const override { return createResult.imageCount; }
    virtual uint32_t                                         GetWidth () const override { return createResult.extent.width; }
    virtual uint32_t                                         GetHeight () const override { return createResult.extent.height; }
    virtual std::vector<VkImage>                             GetImages () const override { return createResult.images; }
    virtual const std::vector<std::unique_ptr<ImageView2D>>& GetImageViews () const override { return createResult.imageViews; }
    virtual std::vector<std::unique_ptr<InheritedImage>>     GetImageObjects () const override;

    virtual uint32_t GetNextImageIndex (VkSemaphore signalSemaphore, VkFence fenceToSignal = VK_NULL_HANDLE) const override;

    virtual bool SupportsPresenting () const override { return true; }

    virtual void Present (VkQueue queue, uint32_t imageIndex, const std::vector<VkSemaphore>& waitSemaphores) const override;
};


class RENDERGRAPH_DLL_EXPORT ExtentProvider {
public:
    virtual ~ExtentProvider () = default;

    virtual std::pair<uint32_t, uint32_t> GetExtent () = 0;
};


class RENDERGRAPH_DLL_EXPORT SwapchainProvider : public ExtentProvider {
public:
    virtual ~SwapchainProvider () = default;

    virtual Swapchain& GetSwapchain () = 0;

private:
    virtual std::pair<uint32_t, uint32_t> GetExtent () override final
    {
        return { GetSwapchain ().GetWidth (), GetSwapchain ().GetHeight () };
    }
};


class RENDERGRAPH_DLL_EXPORT FakeSwapchain : public Swapchain {
private:
    std::unique_ptr<Image>                    image;
    std::vector<std::unique_ptr<ImageView2D>> imageViews;
    const DeviceExtra&                        device;
    const uint32_t                            width;
    const uint32_t                            height;

public:
    FakeSwapchain (const DeviceExtra& device, uint32_t width, uint32_t height);

    virtual VkFormat             GetImageFormat () const override { return image->GetFormat (); }
    virtual uint32_t             GetImageCount () const override { return 1; }
    virtual uint32_t             GetWidth () const override { return width; }
    virtual uint32_t             GetHeight () const override { return height; }
    virtual std::vector<VkImage> GetImages () const override { return { *image }; }
    virtual void                 Recreate () override {}

    virtual uint32_t GetNextImageIndex (VkSemaphore signalSemaphore, VkFence fenceToSignal = VK_NULL_HANDLE) const override
    {
        RG_ASSERT (signalSemaphore == VK_NULL_HANDLE);
        RG_ASSERT (fenceToSignal == VK_NULL_HANDLE);
        return 0;
    }

    virtual const std::vector<std::unique_ptr<ImageView2D>>& GetImageViews () const override { return imageViews; }

    virtual bool SupportsPresenting () const override { return false; }

    virtual void Present (VkQueue, uint32_t, const std::vector<VkSemaphore>&) const override
    {
        throw std::runtime_error ("fake swapchain cannot present");
    }
};

} // namespace RG

#endif