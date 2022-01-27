#include "Surface.hpp"

#include "Utils/Assert.hpp"

#include <stdexcept>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// windows.h has to come before vulkan
#include <vulkan/vulkan_win32.h>
#endif

namespace GVK {

Surface::Surface (VkInstance instance, VkSurfaceKHR&& handle)
    : instance (instance)
    , handle (handle)
{
}


#if WIN32

Surface::Surface (PlatformSpecificSelector, VkInstance instance, void* hwnd)
    : instance (instance)
    , handle (VK_NULL_HANDLE)
{
    VkWin32SurfaceCreateInfoKHR info = {};
    info.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hinstance                   = GetModuleHandle (nullptr);
    info.hwnd                        = static_cast<HWND> (hwnd);

    if (GVK_ERROR (vkCreateWin32SurfaceKHR (instance, &info, nullptr, &handle) != VK_SUCCESS)) {
        throw std::runtime_error ("failed to create win32 VkSurface");
    }
}

#else

Surface::Surface (PlatformSpecificSelector, VkInstance instance, void* handle)
    : instance (instance)
    , handle (VK_NULL_HANDLE)
{
    GVK_BREAK_STR ("no implementation for this platform");
    throw std::runtime_error ("unsupported platform for creating VkSurface");
}

#endif


Surface::~Surface ()
{
    if (handle != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR (instance, handle, nullptr);
        handle = nullptr;
    }
}

} // namespace GVK
